#!/usr/bin/env python3
"""Differentially execute recovered recipe C and original ARM firmware.

Every boundary is explicit. Original and rebuilt code execute in separate ARM
machines; UI, protocol, mechanical hardware and already-verified controller
routines are controlled services. Tests compare state and the ordered service
trace, and record which scenarios exercise each recovered entry point.
"""
from pathlib import Path
import argparse, hashlib, json, math, random, re, struct, sys
HERE=Path(__file__).resolve().parent
WORK=HERE.parent
ROOT=WORK if (WORK/'tools/original_arm.py').is_file() else WORK.parents[1]/'picobrew-application'
sys.path.insert(0,str(ROOT/'tools'))
from original_arm import ArmFirmware, IMAGE_SHA256
from rebuilt_arm import RebuiltArm
REFERENCE=json.loads((HERE/'recipe_reference.json').read_text())
MAP=REFERENCE['original_functions']
SDK=REFERENCE['library_functions']
NEW=REFERENCE['replacements']
ELF=HERE/'recipe-validation.elf'
U32=lambda x:x&0xffffffff
S32=lambda x:struct.unpack('<i',struct.pack('<I',U32(x)))[0]
PACK=lambda *x:struct.pack('<'+'I'*len(x),*(U32(v) for v in x))
D=lambda x:struct.unpack('<II',struct.pack('<d',x))
F=lambda x:struct.unpack('<I',struct.pack('<f',x))[0]
STRING=0x20084000
ACTIONS=0x20085000
INBUF=0x20086000
OUTBUF=0x20087000
CURSOR=0x20088000

# screen enum -> original rendering/workflow boundary and semantic parameters
SCREEN=[
('pico_display_clear',''),('pico_display_flush',''),('pico_display_logo',''),
('pico_screen_changing_step',''),('pico_screen_setting_up_step','s'),
('pico_display_draw_titled_frame','s'),('pico_screen_preheating_steam','u'),
('pico_screen_starting_program','s'),('pico_screen_show_ending_program','s'),
('pico_screen_loading',''),('pico_screen_loading_picopak',''),
('pico_display_stop_progress_indicator',''),('pico_ui_draw_paused',''),
('pico_ui_draw_skip_step_confirmation',''),('pico_ui_draw_stop_confirmation',''),
('pico_ui_show_still_communication_failure',''),('pico_ui_show_still_temperature_warning',''),
('pico_ui_show_still_completion',''),('pico_ui_still_fan_menu',''),
('pico_ui_require_first_rinse',''),('pico_ui_draw_brew_complete','s'),
('pico_display_wait_for_click',''),('pico_ui_show_rinse_finished',''),
('pico_ui_prompt_deep_clean_setup',''),('pico_screen_ending_process',''),
('pico_screen_show_program_notice',''),('pico_ui_draw_drain_reservoir',''),
('pico_ui_draw_drain_filter',''),('pico_ui_draw_stop_footer',''),('pico_ui_draw_header','s'),
('pico_ui_first_rinse_step','u'),('pico_screen_ending_first_rinse',''),
('pico_ui_rack_beer_step','u'),('pico_ui_draw_descale_heading',''),
('pico_ui_wait_for_stop',''),('pico_ui_draw_instruction_screen','instruction'),
('pico_ui_draw_instruction_with_header','header'),('pico_ui_start_descale',''),
('pico_ui_confirm_still_cleaned',''),('pico_ui_wait_for_filter_insertion','')]
PROMPT=[
('pico_ui_confirm_at_row','u'),('pico_ui_confirm_thermal_recovery','u'),
('pico_ui_choose_binary_option','s'),('pico_ui_select_recipe',''),
('pico_ui_adjust_recipe',''),('pico_ui_confirm_screen',''),
('pico_ui_prompt_vacuum','u'),('pico_ui_descale_step','u'),
('pico_ui_choose_program',''),('pico_ui_walk_program_steps','u'),
('pico_ui_wait_for_continue','')]

class Machine:
 def __init__(self,rebuilt=False):
  self.new=rebuilt
  self.arm=RebuiltArm(ELF) if rebuilt else ArmFirmware()
  if rebuilt:
   off=struct.unpack('<17I',self.arm.read(self.arm.address('recipe_fixture_offsets'),68))
   base=self.arm.address('recipe_fixture_state')
   self.controller=base+off[1];self.signals=base+off[2];self.screen=base+off[3]
   self.carousel=base+off[4];self.session=base+off[5];self.recipe=base+off[6]
   self.device=base+off[8];self.runtime=self.arm.address('recipe_fixture_runtime');self.request=self.arm.address('recipe_fixture_request')
   self.language=self.arm.address('recipe_fixture_language');self.offline=self.arm.address('recipe_fixture_offline')
   self.detailed=self.arm.address('recipe_fixture_detailed');self.legacy=self.arm.address('recipe_fixture_legacy')
   self.title_pointer=self.runtime+off[13]
  else:
   self.controller=0x2002d2f8;self.screen=0x2002d524;self.session=0x2002c108
   self.recipe=0x2002bcd8;self.carousel=0x2002d9d0;self.device=0x2002da9c
   self.language=0x2002c164;self.offline=0x2002d9c8;self.detailed=0x2002d57f;self.legacy=0x2002bc71
   self.title_pointer=0x2002bcd4
  self.install()
 def word(self,i):return self.arm.reg(i) if i<4 else self.arm.read_u32(self.arm.reg('sp')+(i-4)*4)
 def text(self,p):return self.arm.cstring(p,65536).decode('latin1') if p else '<NULL>'
 def b(self,addr):return self.arm.read(addr,1)[0]
 def w(self,addr):return self.arm.read_u32(addr)
 def wb(self,addr,v):self.arm.write(addr,bytes([v&255]))
 def wd(self,addr,v):self.arm.write(addr,struct.pack('<d',v))
 def dbl(self,addr):return struct.unpack('<d',self.arm.read(addr,8))[0]
 def strput(self,addr,value):self.arm.write(addr,value.encode('latin1')+b'\0')
 def queue(self,name,default=0):
  q=self.config.get(name,default)
  n=self.counts.get(name,0);self.counts[name]=n+1
  return q[min(n,len(q)-1)] if isinstance(q,(list,tuple)) else q
 def event(self,name,*values):
  self.events.append((name,*values))
  mutate=self.config.get('on_event')
  if mutate:mutate(self,name,values)
 def orig(self,name,callback):
  if name in MAP:self.arm.hook(MAP[name],lambda a:callback())
 def newhook(self,name,callback):
  self.arm.hook_symbol(name,lambda a:callback())
 def install(self):
  if self.new:
   for name in REFERENCE['services']:
    self.newhook('recipe_fixture_'+name,lambda name=name:self.service(name,1))
   self.newhook('recipe_fixture_millis',self.tick)
   self.newhook('recipe_fixture_delay',lambda:self.service('delay',1))
   self.newhook('recipe_fixture_wort',lambda:self.service('wort',1))
   self.newhook('recipe_fixture_move',lambda:self.service('move',1))
   self.newhook('recipe_fixture_filter_prompt',lambda:self.service('filter_prompt',1))
   for name in ['snprintf','atoi','atof']:
    if name in self.arm.symbols:self.newhook(name,lambda name=name:self.libc(name))
  else:
   self.arm.hook(SDK['millis'],lambda a:self.tick())
   self.arm.hook(SDK['delay'],lambda a:self.service('delay',0))
   self.arm.hook(0x200190fc,lambda a:self.digital_write())
   self.arm.hook(SDK['pico_temperature_read_pin25'],lambda a:self.service('wort',0))
   for name in ['snprintf','atoi','atof']:self.arm.hook(SDK['libc_'+name],lambda a,name=name:self.libc(name))
   for name,newline in [('Print_print_string',False),('Print_println_string',True)]:
    self.arm.hook(SDK[name],lambda a,nl=newline:self.serial(self.text(self.word(1)),nl))
   for name,newline in [('Print_print_signed_alias',False),('Print_println_signed',True)]:
    self.arm.hook(SDK[name],lambda a,nl=newline:self.serial(str(S32(self.word(1))),nl))
   for name,kind in SCREEN:
    self.orig(name,lambda name=name,kind=kind:self.original_screen(name,kind))
   for name,kind in PROMPT:
    self.orig(name,lambda name=name,kind=kind:self.original_prompt(name,kind))
   specs={
    'pico_log_serial':('log',0),'pico_assert_nonfatal':('assertion',0),
    'pico_action_label':('action_label',0),'pico_label_compartment':('compartment_label',0),
    'pico_display_clear_row_from_column':('clear_row',0),
    'pico_display_advance_carousel':('advance_carousel',0),
    'pico_display_refresh_current_page':('refresh_page',0),
    'pico_screen_set_recipe_name':('set_recipe_name',1),'pico_screen_set_step':('set_step_name',1),
    'pico_encoder_take_click':('take_click',0),'pico_encoder_clear_click':('clear_click',0),
    'pico_menu_populate_in_brew_options':('populate_brew_menu',0),
    'pico_menu_populate_post_brew':('populate_post_brew_menu',0),
    'pico_menu_choose_with_timeout':('choose_menu',0),
    'pico_get_temperature_input':('temperature_input',0),'pico_get_time_input':('time_input',0),
    'pico_report_error':('error',0),'pico_wait_for_step_filter':('wait_for_filter',0),
    'pico_prompt_insert_filter_with_alarm':('filter_prompt',0),
    'pico_check_thermoblock_heating':('check_thermoblock_heating',0),
    'pico_stepper_move_home':('move_home',0),'pico_stepper_move_to_compartment':('move',0),
    'pico_recovery_save_current_session':('save_recovery',0),
    'pico_storage_read_restart_record':('read_recovery',0),
    'pico_recovery_delete_state':('delete_recovery',0),
    'pico_api_get_session_id':('get_session_id',0),
    'pico_api_report_device_state':('report_state',0),
    'pico_wifi_log_step':('log_step',0),'pico_wifi_log_progress':('log_progress',0),
    'pico_api_fetch_recipe':('fetch_recipe',0),
    'pico_api_fetch_default_recipe_failed':('fetch_default_recipe_failed',0),
    'pico_rfid_read_picopak':('read_picopak',0),
    'pico_api_get_required_actions':('required_actions',0),
    'pico_settings_save_shuttle_scale':('save_shuttle_scale',0),
    'pico_storage_load_shuttle_scale':('load_shuttle_scale',0),
    'pico_still_read_status':('still_read_status',0),
    'pico_still_check_pressure_limit':('still_pressure_limit',0),
    'pico_still_stop':('still_stop',0),'pico_run_drain_instructions':('run_drain_instructions',0),
    'pico_title_label':('title_label',0),'pico_display_center_text':('center_text',0),
    'pico_display_update_timer':('timer',0),'pico_display_temperature_columns':('temperature_columns',0),
    'pico_display_temperature_summary':('temperature_summary',0),'pico_display_valve_position':('valve_position',0),
    'pico_ui_draw_circulation_status':('circulation_status',0),
    'pico_ui_run_instruction_screen':('instruction_screen',0),
    'pico_ui_run_instruction_sequence':('instruction_sequence',0),
    'pico_menu_select_action':('select_action',0),
    'pico_alarm_until_press_or_timeout':('alarm_until_press_or_timeout',2),
    'pico_api_report_error':('report_error',0),
    'pico_still_initialize_connection':('still_initialize',0),'pico_still_has_home_ip':('still_has_home_ip',0),
    'pico_still_select_access_point_2':('still_choose_access_point',0),
    'pico_still_configure':('still_configure',0),'pico_still_connect_wifi_network':('still_connect_wifi',0),
    'pico_still_start_session_once':('still_start_once',0)}
   self.missing_specs=[name for name in specs if name not in MAP]
   for name,(tag,shift) in specs.items():self.orig(name,lambda tag=tag,shift=shift:self.service(tag,shift))
   self.orig('pico_display_clear_row',lambda:self.event('clear_row',self.word(0),0))
   for name,method in [('pico_api_can_use_still','canUsePicoStill'),('pico_api_has_cleaned_ack','hasCleanedAck'),('pico_api_set_cleaned_ack','setCleanedAck')]:
    self.orig(name,lambda method=method:self.still_permission(method))
   self.orig('pico_menu_populate_from_menu',self.menu_from_struct)
  self.boundary_hooks={}
  shared={
   'pico_step_filter_is_inserted':('pico_filter_is_inserted','filter'),
   'pico_actuators_all_off':('pico_actuators_all_off','all_off'),
   'pico_actuators_off_except_drain':('pico_actuators_off_except_drain','off_except_drain'),
   'pico_steam_and_drain_pumps_off':('pico_actuators_steam_and_drain_off','steam_drain_off'),
   'pico_heater_on':('pico_actuator_heater_on','heater_on'),
   'pico_heater_off':('pico_actuator_heater_off','heater_off'),
   'pico_steam_pump_on':('pico_actuator_steam_on','steam_on'),
   'pico_steam_pump_off':('pico_actuator_steam_off','steam_off'),
   'pico_drain_pump_on':('pico_actuator_drain_on','drain_on'),
   'pico_drain_pump_off':('pico_actuator_drain_off','drain_off'),
   'pico_shuttle_pump_on':('pico_actuator_shuttle_on','shuttle_on'),
   'pico_shuttle_pump_off':('pico_actuator_shuttle_off','shuttle_off'),
   'pico_start_drain_and_steam_pumps':('pico_actuators_start_drain_and_steam','start_drain_steam'),
   'pico_pause_outputs_for_missing_filter':('pico_actuators_pause_for_missing_filter','pause_outputs'),
   'pico_heat_controller_start':('pico_control_start','control_start'),
   'pico_heat_controller_stop':('pico_control_stop','control_stop'),
   'pico_controller_set_brew_phase':('pico_phase_set','phase_set'),
   'pico_controller_clear_status':('pico_phase_clear_status','phase_clear'),
   'pico_controller_pause_until_resume':('pico_phase_pause_until_resume','phase_pause'),
   'pico_controller_start_circulation':('pico_phase_start_circulation','phase_circulate'),
   'pico_shuttle_update_calibration':('pico_phase_update_shuttle_calibration','shuttle_calibration'),
   'pico_finish_brew_step':('pico_phase_finish_step','finish_step')}
  for old,(new,tag) in shared.items():
   if self.new:self.newhook(new,lambda tag=tag:self.core(tag,True))
   else:self.orig(old,lambda tag=tag:self.core(tag,False))
  self.base_hooks=self.arm.hooks.copy()
 def tick(self):
  now=self.queue('ticks',None)
  if now is None:now=U32(self.config.get('clock_start',1000)+(self.counts['ticks']-1)*self.config.get('clock_step',1000))
  self.event('millis',now)
  return now
 def digital_write(self):
  pin,value=self.word(0),self.word(1)
  name={5:'heater',8:'drain',24:'shuttle',27:'steam'}.get(pin)
  if name is None:raise AssertionError(('unexpected pin',pin,value))
  self.event(name+('_on' if value else '_off'));return 0
 def serial(self,text,newline):self.event('serial',text,bool(newline));return 0
 def libc(self,name):
  if name in ['atof','atoi']:
   s=self.text(self.word(0))
   if name=='atof':return D(float(s.strip() or '0'))
   m=re.match(r'\s*([+-]?\d+)',s);return U32(int(m.group(1)) if m else 0)
  dest,capacity,fmt=self.word(0),self.word(1),self.text(self.word(2));idx=3
  def sub(m):
   nonlocal idx
   token=m.group(0)
   if token=='%%':return '%'
   value=self.word(idx);idx+=1
   if token.endswith('s'):return self.text(value)
   if token.endswith(('d','i')):return str(S32(value))
   if token.endswith('u'):return str(value)
   if token.endswith('c'):return chr(value&255)
   raise AssertionError(token)
  out=re.sub(r'%%|%[-+0 #]*\d*(?:\.\d+)?[lh]?[sdicu]',sub,fmt)
  if capacity:self.arm.write(dest,out.encode('latin1')[:capacity-1]+b'\0')
  return len(out)
 def original_screen(self,name,kind):
  if kind=='s':args=(self.text(self.word(0)),)
  elif kind=='u':args=(self.word(0),)
  elif kind=='instruction':args=(self.word(0),self.word(1),self.text(self.word(2)),self.word(3))
  elif kind=='header':args=(self.word(0),self.word(1),self.text(self.word(2)))
  else:args=()
  self.event(name,*args)
 def original_prompt(self,name,kind):
  args=(self.text(self.word(0)),) if kind=='s' else ((self.word(0),) if kind=='u' else ())
  self.event(name,*args)
  return self.queue(name,1)
 def menu_from_struct(self):
  p=self.word(0);count=self.w(p+88)
  return self.populate_actions(list(struct.unpack('<'+'I'*count,self.arm.read(p,count*4))))
 def populate_actions(self,actions):self.event('populate_actions',tuple(actions));return 0
 def still_permission(self,method):self.event('still_permission',method);return self.queue(method,1)
 def service(self,name,shift):
  arg=lambda i:self.word(i+shift)
  text=lambda i:self.text(arg(i))
  if name=='screen':
   tag,kind=SCREEN[arg(0)]
   if kind=='s':values=(text(2),)
   elif kind=='u':values=(arg(1),)
   elif kind=='instruction':values=(arg(1),self.b(self.language),text(2),1)
   elif kind=='header':values=(arg(1),self.b(self.language),text(2))
   else:values=()
   self.event(tag,*values);return 0
  if name=='prompt':
   tag,kind=PROMPT[arg(0)]
   values=(text(2),) if kind=='s' else ((arg(1),) if kind=='u' else ())
   self.event(tag,*values);return self.queue(tag,1)
  if name=='serial':return self.serial(text(0),arg(1))
  if name=='log':self.event(name,text(0),text(1),text(2));return 0
  if name=='assertion':self.event(name,arg(0),text(1));return 0
  if name=='action_label':
   self.event(name,arg(0),arg(1),arg(2));self.strput(STRING,'Action '+str(arg(0)));return STRING
  if name=='title_label':
   self.event(name,arg(0),arg(1));self.strput(STRING,'Title '+str(arg(0)));return STRING
  if name=='compartment_label':
   self.event(name,arg(1));self.strput(arg(0),'Compartment '+str(arg(1)));return 0
  if name in ['set_recipe_name','set_step_name']:
   if name=='set_recipe_name':
    value=text(0);self.event(name,value);self.arm.write(self.screen,value.encode()[:31]+b'\0')
   else:
    value=text(1);self.event(name,arg(0),value);self.arm.write(self.screen+32,value.encode()[:31]+b'\0');self.arm.write_u32(self.screen+64,arg(0))
   return 0
  if name in ['log_step','log_progress','center_text']:
   self.event(name,*( (arg(0),text(1)) if name=='center_text' else (text(0),) ));return 0
  if name=='choose_menu':self.event(name,*(arg(i) for i in range(7)));return self.queue(name,0)
  if name in ['temperature_input','time_input']:
   self.event(name,arg(0),text(1),arg(2));return self.queue(name,arg(2))
  if name=='instruction_screen':
   values=(arg(0),arg(1),arg(2),text(3),arg(4));self.event(name,*values);return self.queue(name,23)
  if name=='instruction_sequence':
   count=arg(1);ids=struct.unpack('<'+'I'*count,self.arm.read(arg(0),count*4))
   self.event(name,ids,count,arg(2),arg(3),text(4),arg(5));return self.queue(name,23)
  if name=='populate_actions':
   return self.populate_actions(list(struct.unpack('<'+'I'*arg(1),self.arm.read(arg(0),arg(1)*4))))
  if name=='still_permission':return self.still_permission(text(0))
  if name=='fetch_recipe':
   self.event(name,text(0),arg(1),arg(2));self.apply_recipe(self.config.get('fetched_steps',[(120,4,2,1,'Fetched')]))
   return self.queue(name,0)
  if name=='fetch_default_recipe_failed':
   self.event(name,text(0));self.apply_recipe(self.config.get('fetched_steps',[(120,4,2,1,'Fetched')]))
   return self.queue(name,0)
  if name=='read_recovery':
   self.event(name)
   if 'recovery_bytes' in self.config:self.arm.write(arg(0),self.config['recovery_bytes'])
   else:self.strput(arg(0),self.config.get('recovery','0:ID:-1:-1:0:2:3:0:0:'))
   return self.queue(name,1)
  if name in ['read_picopak','get_session_id']:
   self.event(name,arg(1));self.strput(arg(0),self.config.get(name+'_text','Identifier'));return self.queue(name,1)
  if name=='required_actions':
   ids=self.config.get('required',[]);self.event(name,arg(1));self.arm.write(arg(0),PACK(*ids));return len(ids)
  if name=='save_recovery':self.event(name,*(arg(i) for i in range(6)));return 1
  if name=='save_shuttle_scale':
   value=self.dbl(self.controller+0x220)
   self.event(name,value);return 1
  if name in ['temperature_columns','temperature_summary','circulation_status']:
   # AAPCS doubles start at an even register/stack-word index.
   if name=='temperature_columns':
    pos=2;row=arg(0);values=[]
    for _ in range(3):values.append(struct.unpack('<d',PACK(self.word(pos),self.word(pos+1)))[0]);pos+=2
    self.event(name,row,*values)
   elif name=='temperature_summary':
    pos=2 if self.new else 0;values=[]
    for _ in range(3):values.append(struct.unpack('<d',PACK(self.word(pos),self.word(pos+1)))[0]);pos+=2
    self.event(name,*values,self.word(pos))
   else:
    pos=2 if self.new else 0;value=struct.unpack('<d',PACK(self.word(pos),self.word(pos+1)))[0]
    self.event(name,value,self.word(pos+2))
   return 0
  if name=='wort':
   value=self.queue(name,self.dbl(self.controller+0x1f0));self.event(name,value);return F(value)
  count={'delay':1,'move':1,'clear_row':2,'error':3,'timer':3,'valve_position':2,
         'select_action':2,'run_drain_instructions':1,'alarm_until_press_or_timeout':1,
         'report_error':1,'still_connect_wifi':1}.get(name,0)
  self.event(name,*(arg(i) for i in range(count)))
  default={'take_click':0,'populate_brew_menu':6,'populate_post_brew_menu':3,
           'check_thermoblock_heating':1,'still_read_status':1,'still_pressure_limit':0,
           'still_has_home_ip':0,'still_choose_access_point':1,'still_configure':1,
           'still_connect_wifi':1,'still_start_once':1,'run_drain_instructions':23,
           'select_action':35}.get(name,0)
  return self.queue(name,default)
 def core(self,name,new):
  if name=='filter':
   result=self.queue(name,1);self.event(name,result);return result
  if name=='pause_outputs':
   pos=1 if new else 0;self.event(name,*(self.word(pos+i) for i in range(3)));return self.queue(name,0)
  if name=='phase_set':
   phase=self.word(1);self.event(name,phase);self.wb(self.controller+0x62,phase);return 0
  if name=='phase_clear':self.event(name);self.wb(self.controller+0x61,0);return 0
  if name=='phase_pause':self.event(name);return 0
  if name=='finish_step':
   self.event(name,self.text(self.word(1)));self.wb(self.controller+0x62,4);self.wb(self.controller+0x61,0);self.wb(self.controller+0xa8,0);return 0
  self.event(name);return self.queue(name,0)
 def reset(self,config=None):
  self.arm.hooks=self.base_hooks.copy()
  if self.new:self.arm.call_symbol('recipe_fixture_reset')
  self.events=[];self.counts={};self.config=config or {}
  self.arm.write(self.controller,bytes(0x228));self.arm.write(self.screen,bytes(76));self.arm.write(self.recipe,bytes(0x238));self.arm.write(self.session,bytes(0x58));self.arm.write(self.carousel,bytes(76))
  self.arm.write_u32(self.recipe,self.session)
  self.strput(INBUF,'Test Program');self.strput(INBUF+100,'Test Step');self.strput(INBUF+200,'Test ID')
  for off in [0x34,0x38,0x3c,0x40,0x44]:self.arm.write_u32(self.session+off,INBUF)
  self.arm.write_u32(self.session+0x20,1);self.arm.write_u32(self.session+0x30,0)
  self.arm.write(ACTIONS,PACK(1,3,6,5,9,11));self.arm.write_u32(self.session+0x50,ACTIONS);self.arm.write_u32(self.session+0x54,6)
  self.arm.write_u32(self.title_pointer,INBUF)
  self.wb(self.controller+0x60,2);self.wb(self.controller+0x62,4)
  self.wd(self.controller+0x1f0,70.0);self.wd(self.controller+0x1f8,230.0)
  self.wd(self.controller+0x1d8,300.0);self.wd(self.controller+0x218,0.2);self.wd(self.controller+0x220,0.2)
  for ptr,value in [(self.language,0),(self.offline,0),(self.detailed,0),(self.legacy,1),(self.device,2)]:self.wb(ptr,value)
  if not self.new:
   for ptr in (0x2002dabe,0x2002bc54,0x2002bf10,0x2002bf11,0x2002bf12,0x2002bf13,0x2002bf20):self.wb(ptr,0)
  for path,value in self.config.get('state',{}).items():
   area,offset,kind=path.split(':');ptr=getattr(self,area)+int(offset,0)
   if kind=='b':self.wb(ptr,value)
   elif kind=='d':self.wd(ptr,value)
   elif kind=='s':self.strput(ptr,value)
   else:self.arm.write_u32(ptr,value)
  if 'steps' in self.config:self.apply_recipe(self.config['steps'])
  self.mock_workflows(self.config.get('mock',[]))
 def mock_workflows(self,names):
  for name in names:
   address=self.arm.address(NEW[name])&~1 if self.new else MAP[name]
   self.arm.hook(address,lambda a,name=name:self.workflow(name))
 def workflow(self,name):
  sh=1 if self.new else 0
  arg=lambda i:self.word(i+sh)
  defaults={'pico_prepare_brew_step_temperature':0,'pico_handle_filter_removal':0}
  values=()
  if name in ['pico_prime_steam_pump','pico_reprime_steam_pump']:
   values=(self.text(self.word(1)),)
   if name=='pico_reprime_steam_pump':values+=(self.text(self.word(2)),self.word(3),self.word(4))
  elif name=='pico_first_rinse_cool_and_flush':
   ptr=self.word(1 if self.new else 3);values=(self.w(ptr),)
   if 'cool_next' in self.config:self.arm.write_u32(ptr,self.queue('cool_next'))
  elif name=='pico_vacuum_compartments':values=(self.word(1),self.word(2))
  elif name in ['pico_end_brew_session','pico_start_session','pico_run_control_program','pico_execute_pause_custom_instructions']:
   values=(arg(0),)
  elif name=='pico_setup_instructions':values=(arg(0),self.text(arg(1)))
  elif name=='pico_prepare_brew_step_temperature':
   values=(self.word(1),self.text(self.word(2)))
   self.wb(self.controller+0x62,self.config.get('prepare_phase',5));self.wb(self.controller+0x61,self.config.get('prepare_status',0))
  elif name=='pico_run_recipe_steps':
   recipe=arg(0);count=self.b(recipe+0x210)
   values=(arg(1),count,self.arm.read(recipe+0x30,count*32).hex())
  elif name=='pico_run_brew_step':
   if self.new:
    ptr=self.word(1);vals=struct.unpack('<10I',self.arm.read(ptr,40));values=(*vals[:3],vals[3]&255,self.text(vals[4]),self.text(vals[5]),vals[6]&255,*vals[7:])
   else:values=(self.word(1),self.word(2),self.word(3),self.word(4)&255,self.text(self.word(5)),self.text(self.word(6)),self.word(7)&255,self.word(8),self.word(9),self.word(10))
  self.event(name,*values)
  return self.queue(name,defaults.get(name,1))
 def apply_recipe(self,steps):
  for i,(target,active,drain,comp,name) in enumerate(steps):
   ptr=self.recipe+0x30+i*32;self.strput(ptr,name);self.wb(ptr+22,comp);self.wb(ptr+23,target)
   self.arm.write_u32(ptr+24,drain);self.arm.write_u32(ptr+28,active)
  self.wb(self.recipe+0x210,len(steps))
 def invoke(self,name,args=(),stack=(),limit=2000000):
  address=self.arm.address(NEW[name])&~1 if self.new else MAP[name]
  self.arm.unhook(address)
  return self.arm.call(address,args,stack_args=stack,max_instructions=limit)
 def program_snapshot(self):
  raw=self.arm.read(self.session,0x34)
  strings=tuple(self.text(self.w(self.session+off)) for off in [0x34,0x38,0x3c,0x40,0x44])
  count=self.w(self.session+0x54);actions=tuple(struct.unpack('<'+'I'*count,self.arm.read(self.w(self.session+0x50),count*4))) if count else ()
  assets=[]
  assetlist=[self.arm.address('recipe_fixture_asset1'),self.arm.address('recipe_fixture_asset2'),self.arm.address('recipe_fixture_asset3')] if self.new else [0x20023234,0x20023248,0x200285a0]
  for off in [0x48,0x4c]:assets.append(assetlist.index(self.w(self.session+off)) if self.w(self.session+off) in assetlist else self.w(self.session+off))
  return raw,strings,actions,assets
 def snapshot(self):
  reference_pages=[0x20009ac9,0x20009bc1,0x20009c25,0x20008969,0x20009be5,
                   0x200074d9,0x20008809,0x2000477d,0x20007e79,0x20009b75,0x2000a241,0x20007699]
  candidates=[self.arm.address('recipe_fixture_page'+str(i)) for i in range(12)] if self.new else reference_pages
  pages=[]
  for i in range(15):
   ptr=self.w(self.carousel+16+i*4);pages.append(candidates.index(ptr) if ptr in candidates else ptr)
  signals=self.arm.read(self.signals,7) if self.new else bytes(self.b(x) for x in (0x2002dabe,0x2002bc54,0x2002bf10,0x2002bf11,0x2002bf12,0x2002bf13,0x2002bf20))
  return (self.arm.read(self.controller,0x228),self.arm.read(self.recipe+4,0x234),
          self.arm.read(self.screen,76),self.b(self.device),self.b(self.detailed),
          self.program_snapshot(),self.arm.read(self.carousel,16),tuple(pages),signals)

records=[]
def verify(label,name,run,config=None,program=False):
 original.reset(config);rebuilt.reset(config)
 try:
  a=run(original);b=run(rebuilt)
 except Exception as e:
  print('FAILED EXECUTION:',label,name,str(e));print('orig tail',original.events[-20:]);print('new tail',rebuilt.events[-20:]);raise
 if a!=b:raise AssertionError((label,'return',a,b))
 sa=original.program_snapshot() if program else original.snapshot()
 sb=rebuilt.program_snapshot() if program else rebuilt.snapshot()
 if sa!=sb:
  for i,(x,y) in enumerate(zip(sa,sb)):
   if x!=y:
    if isinstance(x,bytes): print('state diff',i,[(hex(j),u,v) for j,(u,v) in enumerate(zip(x,y)) if u!=v][:30])
    else:print('state diff',i,x,y)
  raise AssertionError((label,'state'))
 if original.events!=rebuilt.events:
  for i,(x,y) in enumerate(zip(original.events,rebuilt.events)):
   if x!=y:print('event diff',i,x,y);print('orig',original.events[max(0,i-2):i+8]);print('new',rebuilt.events[max(0,i-2):i+8]);break
  print('trace lengths',len(original.events),len(rebuilt.events))
  raise AssertionError((label,'events'))
 records.append({'name':label,'original_name':name,'original_address':hex(MAP[name]),'events':len(original.events),'result':a})

def test_pure():
 rng=random.Random(0x5049434f)
 for ident in range(10):
  for language in range(5):
   def call(m):
    args=[m.runtime,m.session,ident,language] if m.new else [m.session,ident,language]
    m.invoke('pico_program_initialize_context',args);return 0
   verify(f'program initialization {ident}/{language}','pico_program_initialize_context',call,program=True)
 for case in range(250):
  text=''.join(rng.choice('abc123,') for _ in range(rng.randrange(0,48)))
  text=text+',' if case%3==0 else text
  start=rng.randrange(0,len(text)+1);limit=rng.randrange(start,len(text)+3);delim=rng.choice(',a')
  def call(m):
   m.strput(INBUF,text);m.arm.write(OUTBUF,b'Z'*64);m.arm.write_u32(CURSOR,start);m.strput(CURSOR+8,delim)
   args=[INBUF,OUTBUF,CURSOR,ord(delim) if m.new else CURSOR+8]
   result=m.invoke('pico_parse_delimited_field',args,[limit])['r0']
   return result,m.w(CURSOR),m.arm.read(OUTBUF,64).hex()
  verify(f'delimiter {case}','pico_parse_delimited_field',call)
 responses=['', 'A/', 'Beer/1,2,4.5,80,', 'Beer/1,2,4.5,80,135,5,2,1,Mash,',
            'Beer/1,2,4.5,80,135,5,2,1,Mash,0,2,3,6,Drain,',
            'Beer/-1,-1,1.125,-19,257,-1,-2,258,Long'+('X'*35)+',',
            'Beer/1,2,4.5,80,135,5,2,1,Mash,TAIL']
 responses += ['Beer/1,2,4.5,80,'+''.join(f'{i%256},{i},{i+2},{i%9},Step{i},' for i in range(n)) for n in [1,14,15,16,20]]
 responses += [responses[4][:i] for i in range(len(responses[4]))]
 for i,response in enumerate(responses):
  for offline in range(2):
   def call(m):
    m.strput(INBUF,response);m.wb(m.offline,offline)
    return m.invoke('pico_recipe_parse_server_response',[m.runtime,INBUF,len(response)] if m.new else [INBUF,len(response)])['r0']
   verify(f'recipe parser {i}/offline{offline}','pico_recipe_parse_server_response',call)
 for temperature in [0,1,135,255,256,-1,-256]:
  def call(m):
   m.invoke('pico_recipe_initialize_cleaning_steps',[m.runtime,m.recipe,U32(temperature)] if m.new else [m.recipe,U32(temperature)]);return 0
  verify(f'cleaning recipe {temperature}','pico_recipe_initialize_cleaning_steps',call)
 for loc in range(6):
  verify(f'location {loc}','pico_location_table_value',lambda m:m.invoke('pico_location_table_value',[m.runtime,loc] if m.new else [loc])['r0'])
 for instruction in [0,9,10,14,20,34]:
  for option in [0,1]:
   verify(f'instruction {instruction}/{option}','pico_execute_current_program_instruction',lambda m:m.invoke('pico_execute_current_program_instruction',[m.runtime,instruction,option] if m.new else [instruction,option])['r0'])
 verify('prompt parameters','pico_prompt_manual_temperature_duration',lambda m:(m.invoke('pico_prompt_manual_temperature_duration',[m.runtime] if m.new else []),0)[1],{'temperature_input':140,'time_input':95})
 for program in [3,4]:
  for answers in [[23,23],[34],[26],[23,34]]:
   cfg={'state':{'session:0x20:w':program,'session:0x30:w':2 if program==3 else 4},'instruction_screen':answers}
   verify(f'pause instructions {program}/{answers}','pico_execute_pause_custom_instructions',lambda m:m.invoke('pico_execute_pause_custom_instructions',[m.runtime,program] if m.new else [program])['r0'],cfg)


def void_call(name,*extra):
 def call(m):
  args=[m.runtime,*extra] if m.new else list(extra)
  m.invoke(name,args[:4],args[4:]);return 0
 return call

def test_utilities():
 for filter_value in [0,1]:
  cfg={'filter':filter_value,'state':{'controller:0x62:b':2}}
  verify(f'filter removal {filter_value}','pico_handle_filter_removal',lambda m:m.invoke('pico_handle_filter_removal',[m.runtime] if m.new else [m.controller])['r0'],cfg)
 for filter_value in [0,1]:
  for heating in [0,1]:
   cfg={'filter':filter_value,'check_thermoblock_heating':heating}
   verify(f'prime {filter_value}/{heating}','pico_prime_steam_pump',lambda m:m.invoke('pico_prime_steam_pump',[m.runtime if m.new else m.controller,INBUF])['r0'],cfg)
 for calibrated in [0,1,7]:
  for heating in [0,1]:
   for logging in [0,1]:
    for restart in [0,1]:
     cfg={'state':{'controller:0x1a0:b':calibrated},'check_thermoblock_heating':heating,'filter':0}
     verify(f'reprime {calibrated}/{heating}/{logging}/{restart}','pico_reprime_steam_pump',lambda m:m.invoke('pico_reprime_steam_pump',[m.runtime if m.new else m.controller,INBUF,INBUF+100,logging],[restart])['r0'],cfg)
 cfg={'state':{'controller:0x1f8:d':250.0},'clock_step':25001}
 verify('reprime sixteen logged cooling intervals','pico_reprime_steam_pump',lambda m:m.invoke('pico_reprime_steam_pump',[m.runtime if m.new else m.controller,INBUF,INBUF+100,1],[0])['r0'],cfg)
 verify('restore idle','pico_restore_idle_hardware',void_call('pico_restore_idle_hardware'))
 for accepted in [0,1]:
  for first,comp in [(1,1),(2,6),(3,7)]:
   for paused in [0,1]:
    cfg={'pico_ui_prompt_vacuum':accepted,'take_click':[0,0,0,1],'pause_outputs':paused,'clock_step':5000}
    verify(f'vacuum {accepted}/{first}/{comp}/{paused}','pico_vacuum_compartments',lambda m:(m.invoke('pico_vacuum_compartments',[m.runtime if m.new else 0,comp,first]),0)[1],cfg)
 for filter_values in [[1],[0,1]]:
  for ticks in [[0,1,999,1000,1001,2001],[0xfffffff0,0xffffffff,10,1000,5000,10000]]:
   cfg={'filter':filter_values,'take_click':[0]*6+[1],'ticks':ticks}
   verify(f'circulation {filter_values}/{ticks}','pico_run_circulation',lambda m:(m.invoke('pico_run_circulation',[m.runtime if m.new else m.controller+0x1f0]),0)[1],cfg)
 for filter_values in [[1],[0,1]]:
  verify(f'drain reservoir {filter_values}','pico_drain_reservoir',void_call('pico_drain_reservoir'),{'filter':filter_values,'take_click':[0,0,1]})
 for minimum,automatic in [(0,1),(1,1),(2,1),(0,0),(1,0)]:
  cfg={'clock_step':1000,'take_click':[0,0,0,1]}
  verify(f'flush {minimum}/{automatic}','pico_flush_compartments',void_call('pico_flush_compartments',minimum,automatic),cfg)
 verify('flush wrapper','pico_flush_compartments_until_press',void_call('pico_flush_compartments_until_press'),{'take_click':[0,1]})
 for steam in [0,1]:
  for target in [180.0,200.25]:
   def thermal(m,tag,args,target=target):
    if tag=='heater_on':m.wd(m.controller+0x1f8,target-20)
    if tag=='shuttle_on':m.wd(m.controller+0x1f8,target-35)
   cfg={'state':{'controller:0x1f8:d':100.0},'on_event':thermal}
   verify(f'heat flush {steam}/{target}','pico_heat_flush_compartments',lambda m:(m.invoke('pico_heat_flush_compartments',[m.runtime,steam,*D(target)] if m.new else [steam,0,*D(target)]),0)[1],cfg)
 cfg={'state':{'controller:0x1f8:d':50.0},'clock_step':10000}
 verify('heat flush malfunction','pico_heat_flush_compartments',lambda m:(m.invoke('pico_heat_flush_compartments',[m.runtime,1,*D(180.)] if m.new else [1,0,*D(180.)]),0)[1],cfg)
 for accepted in [23,34]:
  for paused in [0,1]:
   cfg={'instruction_sequence':accepted,'pause_outputs':[paused,0],'clock_step':10000,'mock':['pico_vacuum_compartments']}
   verify(f'rinse {accepted}/{paused}','pico_run_rinse_cycle',lambda m:(m.invoke('pico_run_rinse_cycle',[m.runtime if m.new else m.recipe]),0)[1],cfg)
 for cooling in [0,1]:
  for paused in [0,1]:
   def thermal(m,tag,args,cooling=cooling):
    if tag=='timer' and cooling:m.wd(m.controller+0x1f8,200.0)
   cfg={'state':{'controller:0x1f8:d':300.0},'clock_step':10000,'pause_outputs':[paused,0],'on_event':thermal}
   def call(m):
    m.arm.write_u32(CURSOR,6)
    result=m.invoke('pico_first_rinse_cool_and_flush',[m.runtime,CURSOR] if m.new else [0,0,0,CURSOR])['r0']
    return result,m.w(CURSOR)
   verify(f'first rinse cooling {cooling}/{paused}','pico_first_rinse_cool_and_flush',call,cfg)
 for temperature in [80.,150.,230.,300.]:
  def thermal(m,tag,args,temperature=temperature):
   if temperature==150 and tag=='timer' and args[-1]==0:m.wd(m.controller+0x1f8,230.)
  cfg={'state':{'controller:0x1f8:d':temperature},'clock_step':10000,'on_event':thermal}
  verify(f'first rinse heating {temperature}','pico_first_rinse_heat_test',lambda m:m.invoke('pico_first_rinse_heat_test',[m.runtime] if m.new else [])['r0'],cfg)
 verify('drain step filter','pico_drain_step_filter',void_call('pico_drain_step_filter'))
 for heating in [0,1]:
  for cooling in [[0,0],[0,1],[1,1]]:
   for temperature in [100.,225.,230.,240.]:
    cfg={'state':{'controller:0x1f8:d':temperature},'clock_step':20000,'pause_outputs':[1,0],
         'mock':['pico_first_rinse_heat_test','pico_first_rinse_cool_and_flush','pico_vacuum_compartments'],
         'pico_first_rinse_heat_test':heating,'pico_first_rinse_cool_and_flush':cooling,'cool_next':[4,5]}
    verify(f'first rinse program {heating}/{cooling}/{temperature}','pico_run_first_rinse',void_call('pico_run_first_rinse'),cfg)
 verify('rack beer','pico_run_rack_beer',void_call('pico_run_rack_beer'))
 for temperature in [110.,120.,125.,130.,140.]:
  for confirm in [[1],[1,1,1,1,0,1]]:
   cfg={'state':{'controller:0x1f8:d':temperature},'filter':[0,1,1,0,0,1],
        'pico_ui_descale_step':confirm,'take_click':[0,1,1],'mock':['pico_reprime_steam_pump']}
   verify(f'descale {temperature}/{confirm}','pico_run_descale_program',void_call('pico_run_descale_program'),cfg)

def test_sessions_programs():
 for duration in [0,1000,3000,5000,0xffffffff]:
  for drain,shuttle in [(0,0),(1,0),(1,1)]:
   cfg={'clock_step':1000,'pause_outputs':[1,0]}
   verify(f'end outputs {duration}/{drain}/{shuttle}','pico_end_brew_run_outputs_for',
    lambda m:(m.invoke('pico_end_brew_run_outputs_for',[m.runtime,duration,INBUF,drain] if m.new else [duration,INBUF,drain,shuttle],[shuttle] if m.new else []),0)[1],cfg)
 for logging in [0,1]:
  for session in [0,1,2,3,4,5]:
   for temperature in [210.,230.,250.,300.]:
    cfg={'clock_step':10000,'state':{'controller:0x1f8:d':temperature,'session:0x30:w':session,'controller:0x1a0:b':logging}}
    verify(f'end session {logging}/{session}/{temperature}','pico_end_brew_session',void_call('pico_end_brew_session',logging),cfg)
 for requested in [0,1]:
  for success in [0,1]:
   verify(f'start session {requested}/{success}','pico_start_session',lambda m:m.invoke('pico_start_session',[m.runtime,requested] if m.new else [requested])['r0'],{'get_session_id':success})
 for logging in [0,1]:
  for offline in [0,1]:
   for choice in [0,1]:
    for primed in [0,1]:
     cfg={'mock':['pico_prime_steam_pump','pico_end_brew_session'],'pico_prime_steam_pump':primed,
          'pico_ui_choose_binary_option':choice,'filter':0}
     def call(m):
      m.wb(m.offline,offline)
      return m.invoke('pico_setup_instructions',[m.runtime,logging,INBUF] if m.new else [logging,INBUF])['r0']
     verify(f'setup {logging}/{offline}/{choice}/{primed}','pico_setup_instructions',call,cfg)
 records_to_restore=['0:PACK:-1:-1:0:2:3:0:0:','1:CLEAN:0:0:0:4:5:1:1:',
                     '1:CLEAN:1:2:5:1:2:0:0:','0:PACK:2:3:0:-1:-2:-1:-1:',
                     '0:PACK:-1:-1:-1:2:3:0:0:','1:CLEAN:0:0:6:2:3:0:0:',
                     '2:UNSUPPORTED:0:0:0:0:0:0:0:','6:INVALID:0:0:0:0:0:0:0:',
                     '-1:INVALID:0:0:0:0:0:0:0:']
 for record in records_to_restore:
  for fetch_error in [0,1]:
   cfg={'recovery':record,'fetch_recipe':fetch_error}
   verify(f'restore {record}/{fetch_error}','pico_parse_restart_session',lambda m:m.invoke('pico_parse_restart_session',[m.runtime,m.recipe] if m.new else [m.recipe])['r0'],cfg)
 verify('restart absent','pico_parse_restart_session',lambda m:m.invoke('pico_parse_restart_session',[m.runtime,m.recipe] if m.new else [m.recipe])['r0'],{'read_recovery':0})
 for session in range(6):
  for action in [23,28,29,34]:
   for filter_value in [0,1]:
    cfg={'state':{'session:0x30:w':session,'session:0x24:w':260,'session:0x28:w':95},
         'instruction_screen':action,'filter':filter_value,'mock':['pico_run_brew_step','pico_end_brew_session']}
    verify(f'post session {session}/{action}/{filter_value}','pico_run_post_session_instructions',void_call('pico_run_post_session_instructions'),cfg)
 step_sets=[[],[(135,3,2,1,'Single')],[(135,3,2,2,'Heating'),(120,5,0,8,'Pause'),(0,4,5,3,'Drain'),(20,8,1,9,'Ignored')],
            [(100,0xffffffff,0xfffffffe,2,'ABCDEFGHIJKLMNOPQRSTU')],[(0,0,0,6,'No time')]]
 for session in range(6):
  for steps in step_sets:
   for successful in [0,1]:
    cfg={'state':{'session:0x30:w':session,'session:0x20:w':3},'steps':steps,
         'mock':['pico_setup_instructions','pico_run_brew_step','pico_end_brew_session','pico_execute_pause_custom_instructions'],
         'pico_run_brew_step':successful}
    verify(f'recipe run {session}/{len(steps)}/{successful}','pico_run_recipe_steps',lambda m:m.invoke('pico_run_recipe_steps',[m.runtime,m.recipe,successful] if m.new else [m.recipe,successful])['r0'],cfg)
 for success in [0,1]:
  cfg={'steps':[(0,0,0,8,'Pause')],'mock':['pico_setup_instructions','pico_run_brew_step','pico_end_brew_session','pico_execute_pause_custom_instructions'],
       'pico_setup_instructions':success,'pico_execute_pause_custom_instructions':0}
  verify(f'recipe early return {success}','pico_run_recipe_steps',lambda m:m.invoke('pico_run_recipe_steps',[m.runtime,m.recipe,1] if m.new else [m.recipe,1])['r0'],cfg)
 for device in [3,4,7,8]:
  for session in range(6):
   for success in [0,1]:
    cfg={'state':{'session:0x30:w':session},'mock':['pico_run_recipe_steps','pico_vacuum_compartments','pico_run_rinse_cycle'],
         'pico_run_recipe_steps':success,'get_session_id':success}
    verify(f'control program {device}/{session}/{success}','pico_run_control_program',lambda m:m.invoke('pico_run_control_program',[m.runtime,device] if m.new else [device])['r0'],cfg)
 for confirmed in [0,1]:
  cfg={'pico_ui_confirm_screen':confirmed,'mock':['pico_run_control_program']}
  verify(f'deep clean {confirmed}','pico_run_deep_clean_program',void_call('pico_run_deep_clean_program'),cfg)
 verify('circulation program builds heated cleaning','pico_run_circulation_program',void_call('pico_run_circulation_program'),{'mock':['pico_run_control_program']})
 for required in [[],[7],[9],[1,7,2,9,7]]:
  verify(f'maintenance {required}','pico_run_required_maintenance',void_call('pico_run_required_maintenance'),{'required':required,'mock':['pico_run_deep_clean_program']})
 for program in [3,4,6]:
  for actions in [[26],[23,23,23],[25,26],[34],[0,25,23,26]]:
   cfg={'instruction_screen':actions,'temperature_input':257,'time_input':123,'mock':['pico_run_control_program']}
   verify(f'cooking program {program}/{actions}','pico_sousvide_coldbrew_program',void_call('pico_sousvide_coldbrew_program',program),cfg)
 for rfid in [0,1]:
  for fetch_failure in [0,1]:
   for selection in [0,1]:
    for adjustment in [0,1]:
     cfg={'read_picopak':rfid,'fetch_default_recipe_failed':fetch_failure,'pico_ui_select_recipe':selection,
          'pico_ui_adjust_recipe':adjustment,'mock':['pico_run_recipe_steps','pico_run_rinse_cycle'],'pico_run_recipe_steps':selection}
     verify(f'brew program {rfid}/{fetch_failure}/{selection}/{adjustment}','pico_run_brew_program',lambda m:(m.invoke('pico_run_brew_program',[m.runtime,m.recipe] if m.new else [m.recipe]),0)[1],cfg)
 for actions in [[35],[34],[19,21,27,35],[0,27,19,27,35]]:
  for successful in [0,1]:
   cfg={'select_action':actions,'temperature_input':200,'time_input':75,'pico_run_brew_step':successful,
        'mock':['pico_prime_steam_pump','pico_run_brew_step','pico_end_brew_session','pico_run_circulation']}
   verify(f'manual program {actions}/{successful}','pico_manual_brew_program',void_call('pico_manual_brew_program'),cfg)
 for sequence in [[34],[23,34]]:
  cfg={'instruction_sequence':sequence,'mock':['pico_prime_steam_pump','pico_end_brew_session','pico_run_circulation']}
  verify(f'manual abort {sequence}','pico_manual_brew_program',void_call('pico_manual_brew_program'),cfg)
 for program in [0,1,2]:
  for mode in ['normal','walk_abort','not_found','disallowed','unclean','configure_fail','start_fail']:
   cfg={'pico_ui_choose_program':program,'mock':['pico_run_recipe_steps','pico_vacuum_compartments']}
   cfg.update({'walk_abort':{'pico_ui_walk_program_steps':1},'not_found':{'still_choose_access_point':0},
               'disallowed':{'canUsePicoStill':0},'unclean':{'hasCleanedAck':0},
               'configure_fail':{'still_configure':0},'start_fail':{'still_start_once':0}}.get(mode,{}))
   # Walk-program returns zero for a completed sequence.
   cfg.setdefault('pico_ui_walk_program_steps',0)
   verify(f'still program {program}/{mode}','pico_run_still_program',lambda m:m.invoke('pico_run_still_program',[m.runtime] if m.new else [])['r0'],cfg)

def test_prepare_and_step():
 for target in [0,-1,135,200]:
  for wort in [50.,135.,139.999,140.,140.001,205.,230.]:
   for thermoblock in [50.,150.,209.999,210.,230.]:
    cfg={'state':{'controller:0x1f0:d':wort,'controller:0x1f8:d':thermoblock},
         'clock_step':5000,'take_click':1,'filter':[0,1]}
    verify(f'prepare temperature {target}/{wort}/{thermoblock}','pico_prepare_brew_step_temperature',
           lambda m:(m.invoke('pico_prepare_brew_step_temperature',[m.runtime if m.new else m.controller,U32(target),INBUF]),0)[1],cfg)
 for filter_values in [[1,0,1],[0,0,1],[1,1,0,1]]:
  cfg={'state':{'controller:0x1f0:d':50.,'controller:0x1f8:d':150.},'filter':filter_values,
       'clock_step':5000,'take_click':[0,1]}
  verify(f'prepare heat filter recovery {filter_values}','pico_prepare_brew_step_temperature',
         lambda m:(m.invoke('pico_prepare_brew_step_temperature',[m.runtime if m.new else m.controller,135,INBUF]),0)[1],cfg)
 def step(request):
  target,active,drain,comp,logging,remaining,index,session=request
  def call(m):
   # A dedicated action list allows one click to end non-timed cases deterministically.
   actions=m.config.get('actions',[4]);m.arm.write(ACTIONS,PACK(*actions));m.arm.write_u32(m.session+0x50,ACTIONS);m.arm.write_u32(m.session+0x54,len(actions))
   if m.new:
    m.arm.write(m.request,PACK(target,active,drain,comp,INBUF,INBUF+100,logging,remaining,index,session))
    return m.invoke('pico_run_brew_step',[m.runtime,m.request])['r0']
   return m.invoke('pico_run_brew_step',[m.controller,target,active,drain],[comp,INBUF,INBUF+100,logging,U32(remaining),U32(index),session])['r0']
  return call
 def cfg(**kwargs):
  out={'mock':['pico_prepare_brew_step_temperature','pico_handle_filter_removal','pico_reprime_steam_pump','pico_drain_step_filter'],
       'take_click':1,'prepare_phase':5}
  out.update(kwargs);return out
 for phase in [0,1,2,3,4,5,6,7,8,255]:
  for target in [0,135,200]:
   for temperature in [50.,135.,140.,215.,350.]:
    configuration=cfg(prepare_phase=phase,state={'controller:0x1f0:d':temperature,'controller:0x1f8:d':temperature})
    verify(f'step phase {phase}/{target}/{temperature}','pico_run_brew_step',step((target,0,0,2,1,5,0,0)),configuration)
 for compartment in [0,1,2,3,6,7,8]:
  for session in range(6):
   for active in [0,1,0xffffffff]:
    configuration=cfg(choose_menu=[0],state={'controller:0x1f8:d':200.})
    verify(f'step initialization {compartment}/{session}/{active}','pico_run_brew_step',step((180,active,2,compartment,0,100,3,session)),configuration)
 for subphase in range(9):
  for interval in [1999,2000,2001,14999,15000,15001,29999,30000,30001,44999,45000,45001,60001]:
   configuration=cfg(prepare_phase=3,clock_start=interval,clock_step=0,state={'controller:0xa8:b':subphase})
   verify(f'drain subphase {subphase}/{interval}','pico_run_brew_step',step((0,0,0,1,1,10,2,0)),configuration)
 for drain in [0,1,2,15,60,100000,0x7fffffff,0x80000000,0xffffffff]:
  configuration=cfg(prepare_phase=3)
  verify(f'drain duration compensation {drain}','pico_run_brew_step',step((0,1,drain,1,1,10,2,0)),configuration)
 for status in range(12):
  for retried in [0,1]:
   for confirmed in [0,1]:
    for reprime in [0,1]:
     configuration=cfg(prepare_phase=5,prepare_status=status,pico_ui_confirm_thermal_recovery=confirmed,
                       state={'controller:0x1a1:b':retried,'controller:0x04:w':0,'controller:0x58:w':1})
     configuration['pico_reprime_steam_pump']=reprime
     verify(f'status handling {status}/{retried}/{confirmed}/{reprime}','pico_run_brew_step',step((135,0,0,2,1,10,2,0)),configuration)
 for action in range(13):
  for session in [0,1,2,3]:
   for confirmed in [0,1]:
    configuration=cfg(actions=[action],pico_ui_confirm_at_row=confirmed,temperature_input=260)
    verify(f'step action {action}/{session}/{confirmed}','pico_run_brew_step',step((135,0,0,2,1,10,2,session)),configuration)
 for phase in [0,2]:
  for action in [2,6]:
   configuration=cfg(prepare_phase=phase,actions=[action,4],choose_menu=[0,1],take_click=1,state={'controller:0x1f0:d':50. if phase==0 else 200.})
   verify(f'drain action while heating {phase}/{action}','pico_run_brew_step',step((135,1,0,2,1,10,2,0)),configuration)
 for received in [0,1]:
  for pressure in [0,1]:
   for status in [0,8,9,10]:
    for warm in [0,1]:
     configuration=cfg(prepare_status=status,still_read_status=received,still_pressure_limit=pressure,
                       state={'controller:0x04:w':2,'controller:0x58:w':2,'controller:0x18:d':100.+warm,'controller:0x40:d':100.},clock_start=5000)
     verify(f'still step poll {received}/{pressure}/{status}/{warm}','pico_run_brew_step',step((180,0,0,2,1,10,2,3)),configuration)
 for phase in [1,5,7]:
  for active,drain in [(0,0),(1,0),(1,1),(2,2)]:
   configuration=cfg(prepare_phase=phase,take_click=0,clock_step=10000)
   verify(f'timed step completion {phase}/{active}/{drain}','pico_run_brew_step',step((135,active,drain,2,1,active+drain,2,0)),configuration)
 for session in [2,4]:
  configuration=cfg(choose_menu=[1,0],actions=[4],take_click=1)
  verify(f'indefinite hold then cancel {session}','pico_run_brew_step',step((135,0xffffffff,0,1,1,10,2,session)),configuration)

hardening_records=[]
def test_hardening():
 # The original's out-of-bounds behavior is intentionally not executed as an oracle.
 headers=['R'*n+'/1,2,3.5,4,120,1,2,1,Step,' for n in [100,101,1000,10000]]
 fields=['Beer/'+('1'*n)+',2,3.5,4,120,1,2,1,Step,' for n in [100,101,1000]]
 fields += ['Beer/1,2,3.5,4,120,1,2,1,'+('S'*n)+',' for n in [100,101,1000,10000]]
 for i,text in enumerate(headers+fields):
  rebuilt.reset();before=rebuilt.arm.read(rebuilt.controller,0x228)
  rebuilt.strput(INBUF,text)
  result=rebuilt.invoke('pico_recipe_parse_server_response',[rebuilt.runtime,INBUF,len(text)])['r0']
  assert result==0 and rebuilt.arm.read(rebuilt.controller,0x228)==before
  assert rebuilt.b(rebuilt.recipe+0x210)<=15
  hardening_records.append({'name':f'oversized recipe field {i}','result':result})
 for i,(raw,length) in enumerate([(b'No NUL here',10),(b'Embedded\0tail',13),(b'Ignored\0',-1)]):
  rebuilt.reset();rebuilt.arm.write(INBUF,raw+b'X'*20)
  result=rebuilt.invoke('pico_recipe_parse_server_response',[rebuilt.runtime,INBUF,U32(length)])['r0']
  assert result==0;hardening_records.append({'name':f'invalid text extent {i}','result':result})
 rebuilt.reset()
 result=rebuilt.invoke('pico_recipe_parse_server_response',[rebuilt.runtime,0,0])['r0']
 assert result==0;hardening_records.append({'name':'null recipe text','result':result})
 record='0:PACK:-1:-1:0:2:3:0:0:'
 invalid=[record[:i] for i in range(len(record))]
 invalid += ['0:'+('I'*21)+':0:0:0:0:0:0:0:', '0:PACK:'+('1'*11)+':0:0:0:0:0:0:']
 for i,record in enumerate(invalid):
  rebuilt.reset({'recovery':record})
  result=rebuilt.invoke('pico_parse_restart_session',[rebuilt.runtime,rebuilt.recipe])['r0']
  assert result==0;hardening_records.append({'name':f'invalid restart record {i}','result':result})
 rebuilt.reset({'recovery_bytes':b'X'*101})
 result=rebuilt.invoke('pico_parse_restart_session',[rebuilt.runtime,rebuilt.recipe])['r0']
 assert result==0;hardening_records.append({'name':'unterminated restart record','result':result})

def test_identifiers_and_logging():
 for ident in range(7):
  def call(m):
   m.invoke('pico_program_initialize_context',[m.runtime,m.session,ident,0] if m.new else [m.session,ident,0])
   m.invoke('pico_program_log_configuration',[m.runtime,m.session,1] if m.new else [m.session,1]);return 0
  verify(f'program logging {ident}','pico_program_log_configuration',call,program=True)
 for identifier in ['X','','LONGER IDENTIFIER']:
  cfg={'read_picopak_text':identifier,'pico_ui_adjust_recipe':0,
       'state':{'session:0:s':'Previous longer identifier'}}
  verify(f'identifier copy brew {identifier}','pico_run_brew_program',lambda m:(m.invoke('pico_run_brew_program',[m.runtime,m.recipe] if m.new else [m.recipe]),0)[1],cfg)
  cfg={'recovery':f'0:{identifier}:0:0:0:1:2:0:0:',
       'state':{'session:0:s':'Previous longer identifier'}}
  verify(f'identifier copy restore {identifier}','pico_parse_restart_session',lambda m:m.invoke('pico_parse_restart_session',[m.runtime,m.recipe] if m.new else [m.recipe])['r0'],cfg)
if __name__=='__main__':
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('--elf',type=Path,default=ELF)
 parser.add_argument('--output',type=Path,default=WORK/'validation-arm.json')
 options=parser.parse_args();ELF=options.elf
 original=Machine();rebuilt=Machine(True)
 assert not original.missing_specs,original.missing_specs
 test_pure()
 test_utilities()
 test_sessions_programs()
 test_prepare_and_step()
 test_identifiers_and_logging()
 test_hardening()
 source_directory=WORK if (WORK/'recipe.c').exists() else ROOT/'src'
 report={'reference_sha256':IMAGE_SHA256,'validation_elf_sha256':hashlib.sha256(ELF.read_bytes()).hexdigest(),
         'source_sha256':{name:hashlib.sha256((source_directory/name).read_bytes()).hexdigest()
                          for name in ['recipe.c','recipe_programs.c','recipe_utilities.c']},
         'fixture_sha256':hashlib.sha256((HERE/'recipe_fixture.c').read_bytes()).hexdigest(),
         'differential_cases':len(records),'covered_original_functions':len({x['original_name'] for x in records}),
         'boundary_contract':'UI, network, clock, mechanics, and previously verified core controller/phase routines are explicit controlled boundaries. The original and candidate run in separate ARM emulator instances.',
         'cases':records,'bounded_input_checks':hardening_records}
 options.output.parent.mkdir(parents=True,exist_ok=True)
 options.output.write_text(json.dumps(report,indent=2)+'\n')
 print('PASS',len(records),'differential cases across',report['covered_original_functions'],
       'original entry points; plus',len(hardening_records),'bounded-input checks')
