#!/usr/bin/env python3
"""Compare setup/menu dispatch to original ARM and check concrete service wiring."""
import argparse,hashlib,json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from original_arm import ArmFirmware,IMAGE_SHA256
from rebuilt_arm import RebuiltArm
class ResetReached(Exception):pass

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--elf',type=Path,default=ROOT/'build/application-validation.elf');p.add_argument('--output',type=Path,default=ROOT/'analysis/application-tests.json');o=p.parse_args()
 names={f['name']:int(f['address'],16) for f in json.loads((ROOT/'reference/function_map.json').read_text())}
 records=[]
 def execute(operation,**case):
  outputs=[]
  for rebuilt in (False,True):
   m=RebuiltArm(o.elf) if rebuilt else ArmFirmware()
   if rebuilt:
    m.call_symbol('application_fixture_bind');a,offline,session,language,detailed,ui,framebuffer,recipe=struct.unpack('<8I',m.read(m.address('application_fixture_addresses'),32))
    assert m.call_symbol('application_fixture_bindings_complete')['r0']==1
   else:
    a=0;offline,session,language,detailed=0x2002d9c8,0x2002c138,0x2002c164,0x2002d57f
    m.write_u32(0x2002bcd8,0x2002c108)
   m.write(offline,bytes([case.get('offline',0)]));m.write_u32(session,case.get('session_type',0));m.write(language,b'\x02');m.write(detailed,b'\x01')
   events=[];registrations=0;serial=''
   def emit(name,*args):events.append((name,*args))
   def hook_pair(old,new,callback):
    if rebuilt:m.hook_symbol(new,callback)
    else:m.hook(names[old],callback)
   def log(machine):emit('log',*(machine.cstring(machine.reg(i+(1 if rebuilt else 0))).decode() for i in range(3)));return 0
   hook_pair('pico_log_serial','pico_network_log',log)
   def println(machine):
    nonlocal serial
    text=machine.cstring(machine.reg(1)).decode()
    if rebuilt:
     serial+=text
     while '\r\n' in serial:line,serial=serial.split('\r\n',1);emit('serial',line)
    else:emit('serial',text)
    return 1
   hook_pair('Print_println_string','application_fixture_serial',println)
   hook_pair('HardwareSerial_begin','application_fixture_serial_begin',lambda x:emit('serial_begin',x.reg(1)))
   if not rebuilt:
    def formatter(machine):
     fmt=machine.cstring(machine.reg(2)).decode();values=[machine.reg(3),machine.read_u32(machine.reg('sp')),machine.read_u32(machine.reg('sp')+4)]
     text=(fmt%tuple(machine.cstring(v).decode() for v in values[:fmt.count('%s')])).encode()
     machine.write(machine.reg(0),text[:machine.reg(1)-1]+b'\0');return len(text)
    m.hook(names['libc_snprintf'],formatter)
   specs=[('pico_display_initialize','pico_display_initialize','display_init'),('pico_screen_splash','pico_screen_splash','splash'),('pico_actuators_initialize','pico_actuators_initialize','outputs_init'),('pico_temperature_inputs_initialize','pico_device_temperature_inputs_initialize','temperature_init'),('pico_spi_chip_selects_initialize','pico_device_spi_initialize','spi_init'),('pico_step_filter_gpio_initialize','pico_device_filter_input_initialize','filter_init'),('pico_storage_load_user_preferences','pico_storage_load_preferences','preferences'),('pico_control_timers_initialize','pico_control_timers_initialize','timers'),('pico_wifi_ensure_started_thunk','application_fixture_wifi_init','wifi_init'),('pico_detect_stepper_configuration','pico_device_detect_stepper','stepper_detect'),('pico_encoder_clear_click','pico_encoder_clear_click','clear_click'),('pico_screen_device_information','pico_screen_device_information','device_info'),('pico_rfid_initialize','pico_rfid_application_initialize','rfid_init'),('pico_ui_service_menu','pico_ui_service_menu','service_menu'),('pico_display_clear','pico_display_clear','clear'),('pico_wifi_ensure_connection','pico_network_ensure_connection','ensure_wifi'),('pico_api_report_device_state','pico_api_report_state','report_state'),('pico_recovery_delete_state','pico_storage_delete_restart','delete_restart'),('pico_update_firmware','pico_update_run','update'),('pico_run_required_maintenance','pico_recipe_run_required_maintenance','maintenance'),('pico_run_rinse_cycle','pico_recipe_rinse','rinse'),('pico_display_logo','pico_display_logo','logo'),('pico_run_brew_program','pico_recipe_run_brew_program','brew'),('pico_manual_brew_program','pico_recipe_manual_program','manual'),('pico_utilities_menu','pico_utilities_menu','utilities'),('pico_settings_menu','pico_settings_menu','settings')]
   for old,new,tag in specs:hook_pair(old,new,lambda x,tag=tag:(emit(tag),1)[1])
   hook_pair('pico_encoder_initialize','pico_encoder_initialize',lambda x:emit('encoder_init',x.reg(1),x.reg(2),x.reg(3),x.read_u32(x.reg('sp')),x.read_u32(x.reg('sp')+4),x.read_u32(x.reg('sp')+8)))
   hook_pair('pico_wait_gpio19_low','pico_device_service_button',lambda x:(emit('service_button'),case.get('service',0))[1])
   hook_pair('delay','device_fixture_delay',lambda x:emit('delay',x.reg(1 if rebuilt else 0)))
   def reset(machine):emit('reset');raise ResetReached()
   hook_pair('PRCMMCUReset','application_fixture_reset',reset)
   hook_pair('WiFi_status','application_fixture_wifi_status',lambda x:(emit('wifi_status'),case.get('wifi_status',3))[1])
   def register(machine):
    nonlocal registrations
    emit('register');registrations+=1;return int(registrations>case.get('register_failures',0))
   hook_pair('pico_api_register_device','pico_api_register',register)
   hook_pair('pico_report_error','pico_report_error',lambda x:emit('error',x.reg(0),x.reg(1),x.reg(2)))
   hook_pair('pico_parse_restart_session','pico_recipe_session_restore',lambda x:(emit('restore'),case.get('restore',0))[1])
   hook_pair('pico_ui_confirm_continue_or_abort','pico_ui_confirm_continue_or_abort',lambda x:(emit('confirm'),case.get('continue',1))[1])
   hook_pair('pico_screen_loading','pico_screen_loading',lambda x:emit('loading',x.reg(0)))
   hook_pair('pico_run_control_program','pico_recipe_run_control_program',lambda x:emit('control_program',x.reg(1 if rebuilt else 0)))
   hook_pair('pico_run_recipe_steps','pico_recipe_run',lambda x:emit('recipe',x.reg(2 if rebuilt else 1)))
   hook_pair('pico_sousvide_coldbrew_program','pico_recipe_run_cooking_program',lambda x:emit('cooking',x.reg(1 if rebuilt else 0)))
   hook_pair('pico_menu_populate_action_labels','pico_menu_populate_action_labels',lambda x:emit('menu',list(struct.unpack('<6I',x.read(x.reg(0),24))),x.reg(1),x.reg(2),x.reg(3)))
   hook_pair('pico_menu_select_action','pico_menu_select_action',lambda x:(emit('select',x.reg(0),x.reg(1)),case.get('action',12))[1])
   terminal='returned'
   try:result=m.call(m.address('pico_application_'+operation) if rebuilt else (0x200131bc if operation=='setup' else 0x20012e10),[a] if rebuilt else [],max_instructions=1000000)
   except ResetReached:terminal='reset'
   outputs.append({'events':events,'terminal':terminal})
  if outputs[0]!=outputs[1]:
   diag={'operation':operation,'case':case,'original':outputs[0],'rebuilt':outputs[1]};(ROOT/'analysis/application-failure.json').write_text(json.dumps(diag,indent=2)+'\n');raise AssertionError(str(diag)[:6000])
  records.append({'operation':operation,'case':case,'event_count':len(outputs[0]['events']),'terminal':outputs[0]['terminal']})
 for offline in (0,1):
  for restore in (0,1):
   for kind in (0,1,2):
    for continuing in (0,1):execute('setup',offline=offline,restore=restore,session_type=kind,**{'continue':continuing})
 execute('setup',service=1);execute('setup',wifi_status=0);execute('setup',register_failures=2)
 for action in (12,13,14,15,16,17,0,0xffffffff):execute('loop',action=action)
 m=RebuiltArm(o.elf);m.call_symbol('application_fixture_bind');addresses=struct.unpack('<8I',m.read(m.address('application_fixture_addresses'),32));framebuffer=addresses[6]
 m.write(framebuffer,b'\xff'*1024);m.call_symbol('application_fixture_network_clear');assert m.read(framebuffer,1024)==bytes(1024)
 m.write(addresses[3],b'\x02');m.hook_symbol('pico_ui_choose_language',lambda f:3);m.call_symbol('application_fixture_choose_language');assert m.read(addresses[3],1)==b'\x02'
 report={'passed':len(records)+2,'failed':0,'firmware_sha256':IMAGE_SHA256,'elf_sha256':hashlib.sha256(o.elf.read_bytes()).hexdigest(),'source_sha256':hashlib.sha256((ROOT/'src/application.c').read_bytes()).hexdigest(),'scope':'Original/rebuilt startup and six-way main menu orchestration traces, required application service tables and real network-to-display context binding. Domain workflows are intercepted here and tested separately.','hardware_executed':False,'cases':records,'binding_tests':['All device/RFID/UI/phase/recipe/network UI service pointers nonzero','Network UI clear traverses actual context wrapper and native framebuffer implementation','Language Back action preserves current language']}
 o.output.write_text(json.dumps(report,indent=2)+'\n');print('Application:',report['passed'],'boot/menu/binding scenarios passed.')
if __name__=='__main__':main()
