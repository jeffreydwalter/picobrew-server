#!/usr/bin/env python3
from ui_test_support import *
results=[];failures=[]
def case(name,label,run):
 try:run();results.append({'function':name,'case':label})
 except Exception as e:failures.append({'function':name,'case':label,'error':str(e)});print('FAIL',name,label,str(e),flush=True)
# Every error-message and instruction-action branch, in all stored languages.
for error in [*range(26),99,-1]:
 for lang in range(3):
  def run(error=error,lang=lang):
   p=Pair(lang);p.call('pico_ui_draw_error',[u32(error)]);p.check_display()
  case('pico_ui_draw_error',f'{error},{lang}',run)
for sid in range(25):
 for program in [0,3,4]:
  for lang in [0,1,2]:
   def run(sid=sid,program=program,lang=lang):
    p=Pair(lang);p.call('pico_ui_run_instruction_screen',[sid,0,program,p.old.SCRATCH],[1]);p.check_display();p.check_labels();offset=p.new.read_u32(p.new.address('ui_fixture_menu_offset'));assert p.old.read(0x2002bc74,92)==p.new.read(p.app+offset,92),'action list mismatch'
   case('pico_ui_run_instruction_screen',f'id={sid},program={program},lang={lang}',run)
# Instruction sequence control: first-screen alarm, continuation and early exit.
for count,actions,null_input in [(0,[],False),(-1,[],False),(1,[23],False),
                                  (3,[23,23,23],False),(3,[23,34],False),
                                  (3,[34],False),(3,[],True)]:
 for alarm in [-1,0,1]:
  def run(count=count,actions=actions,null_input=null_input,alarm=alarm):
   p=Pair();events={'old':[],'new':[]};called={'old':0,'new':0}
   for side,f in [('old',p.old),('new',p.new)]:
    f.write(f.SCRATCH,struct.pack('<III',2,10,24));f.write(f.SCRATCH+64,b'Sequence title\0')
    def instruction(f,side=side):
     events[side].append((f.reg(0),f.reg(1),f.reg(2),f.cstring(f.reg(3)),f.read_u32(f.reg('sp'))));index=called[side];called[side]+=1;return actions[index]
    f.hook(MAP['pico_ui_run_instruction_screen'] if side=='old' else f.address('pico_ui_run_instruction_screen'),instruction)
   r1,r2=p.call('pico_ui_run_instruction_sequence',[0 if null_input else p.old.SCRATCH,u32(count),u32(alarm),4],[p.old.SCRATCH+64,1])
   assert r1['r0']==r2['r0'];assert events['old']==events['new']
  case('pico_ui_run_instruction_sequence',f'count={count},actions={actions},null={null_input},alarm={alarm}',run)
# Rotary text editor: native small/large control glyphs, deletion and bounds.
for sequence,capacity in [([4,5,3,26,98,1],64),([98,98,1],8),([4]*10+[3,5,1],5),([1],64),([3,1],8)]:
 def run(sequence=sequence,capacity=capacity):
  p=Pair();states={'old':{'phase':'render','step':0},'new':{'phase':'render','step':0}}
  for f in [p.old,p.new]:f.write(f.SCRATCH,b'\0'*100)
  def position(f,side):return sequence[states[side]['step']]
  def click(f,side):
   state=states[side]
   if state['phase']=='click':state['phase']='processing';return 255
   return 0
  def flush(f,side):
   state=states[side];(p.old_flush if side=='old' else p.new_flush).append(f.read(0x2002af60 if side=='old' else p.fb,1024))
   if state['phase']=='processing':state['step']+=1;state['phase']='render'
   else:state['phase']='click'
  for side,f in [('old',p.old),('new',p.new)]:
   f.hook(MAP['pico_encoder_get_position'] if side=='old' else f.address('pico_encoder_get_position'),lambda f,side=side:position(f,side));f.hook(MAP['pico_encoder_take_click'] if side=='old' else f.address('pico_encoder_take_click'),lambda f,side=side:click(f,side));f.hook(MAP['pico_display_flush'] if side=='old' else f.address('pico_display_flush'),lambda f,side=side:flush(f,side))
  p.call('pico_encoder_edit_text',[p.old.SCRATCH,capacity],max_instructions=20000000);assert p.old.cstring(p.old.SCRATCH)==p.new.cstring(p.new.SCRATCH);p.check_display();assert p.old.read(0x2002bc28,44)==p.new.read(p.encoder,44)
 case('pico_encoder_edit_text',f'{sequence},capacity={capacity}',run)
for baseline in [0,3,10]:
 for stored in [-1,0,5,10]:
  for selected in [-1,4,9]:
   def run(baseline=baseline,stored=stored,selected=selected):
    p=Pair();p.old.write_u32(p.old.SCRATCH,stored);p.new.write_u32(p.new.SCRATCH,stored);calls={'old':0,'new':0}
    def click(f,side):calls[side]+=1;return 255 if calls[side]>4 else 0
    for side,f in [('old',p.old),('new',p.new)]:f.hook(MAP['pico_encoder_get_position'] if side=='old' else f.address('pico_encoder_get_position'),lambda f:u32(selected));f.hook(MAP['pico_encoder_take_click'] if side=='old' else f.address('pico_encoder_take_click'),lambda f,side=side:click(f,side))
    r1,r2=p.call('pico_encoder_select_adjustment_level',[0,baseline,p.old.SCRATCH,p.old.SCRATCH+8],new_args=[baseline,p.new.SCRATCH,p.new.SCRATCH+8],max_instructions=10000000);assert r1['r0']==r2['r0'];assert p.old.read(p.old.SCRATCH,12)==p.new.read(p.new.SCRATCH,12);p.check_display()
   case('pico_encoder_select_adjustment_level',f'{baseline},{stored},{selected}',run)
for name,values in [('pico_get_temperature_input',[60,100,153,203]),('pico_get_time_input',[0,59,60,90,595])]:
 for celsius in [0,1]:
  for selected in values:
   def run(name=name,selected=selected,celsius=celsius):
    p=Pair(celsius=celsius);calls={'old':0,'new':0}
    def click(f,side):calls[side]+=1;return 255 if calls[side]>2 else 0
    for side,f in [('old',p.old),('new',p.new)]:f.hook(MAP['pico_encoder_get_position'] if side=='old' else f.address('pico_encoder_get_position'),lambda f:selected);f.hook(MAP['pico_encoder_take_click'] if side=='old' else f.address('pico_encoder_take_click'),lambda f,side=side:click(f,side))
    r1,r2=p.call(name,[19 if name.endswith('temperature_input') else 21,p.old.SCRATCH,selected],max_instructions=10000000);assert r1['r0']==r2['r0'];p.check_display();assert p.old.read(0x2002bc28,44)==p.new.read(p.encoder,44)
   case(name,f'value={selected},celsius={celsius}',run)
# Timeout alarm observes exact buzzer pulses, predicates and unsigned deadline.
for click_after,timeout in [(1,60000),(8,60000),(100,0),(100,5000)]:
 def run(click_after=click_after,timeout=timeout):
  p=Pair();events={'old':[],'new':[]};ticks={'old':0,'new':0};clicks={'old':0,'new':0}
  def now(f,side):ticks[side]+=1;return ticks[side]*500
  def click(f,side):clicks[side]+=1;return 255 if clicks[side]>=click_after else 0
  p.old.hook(MAP['pico_buzzer_on'],lambda f:events['old'].append(True));p.old.hook(MAP['pico_buzzer_off'],lambda f:events['old'].append(False));p.new.hook_symbol('ui_fixture_buzzer',lambda f:events['new'].append(bool(f.reg(1))))
  for side,f in [('old',p.old),('new',p.new)]:f.hook(MAP['millis'] if side=='old' else f.address('ui_fixture_millis'),lambda f,side=side:now(f,side));f.hook(MAP['pico_encoder_take_click'] if side=='old' else f.address('pico_encoder_take_click'),lambda f,side=side:click(f,side))
  p.call('pico_alarm_until_press_or_timeout',[0,0,timeout],max_instructions=10000000);assert events['old']==events['new'];assert ticks['old']==ticks['new'];p.check_display()
 case('pico_alarm_until_press_or_timeout',f'click={click_after},timeout={timeout}',run)
# Fatal UI path must actually request shutdown before drawing/reporting and never return.
class HaltReached(Exception):pass
for name,allow in [('pico_halt_heater_malfunction',0),('pico_report_error',0),('pico_report_error',1)]:
 def run(name=name,allow=allow):
  p=Pair();events={'old':[],'new':[]}
  for side,f in [('old',p.old),('new',p.new)]:
   f.hook(MAP['pico_api_report_error'] if side=='old' else f.address('pico_api_error'),lambda f,side=side:events[side].append(('report',f.reg(0 if side=='old' else 1))))
   f.hook(MAP['pico_heat_controller_stop'] if side=='old' else f.address('pico_control_stop'),lambda f,side=side:events[side].append(('control_stop',)))
   f.hook(MAP['pico_actuators_off_except_drain'] if side=='old' else f.address('pico_actuators_off_except_drain'),lambda f,side=side:events[side].append(('outputs_off_except_drain',)))
   f.hook(MAP['pico_steam_pump_off'] if side=='old' else f.address('pico_actuator_steam_off'),lambda f,side=side:events[side].append(('steam_off',)))
   f.hook(MAP['IntMasterEnable'] if side=='old' else f.address('ui_fixture_enable'),lambda f,side=side:events[side].append(('enable',)))
   f.hook(MAP['IntMasterDisable'] if side=='old' else f.address('ui_fixture_disable'),lambda f,side=side:events[side].append(('disable',)))
   f.hook(MAP['WiFi_status'] if side=='old' else f.address('fixture_status'),lambda f:3)
  p.old.hook(MAP['pico_buzzer_on'],lambda f:events['old'].append(('buzzer',True)));p.new.hook_symbol('ui_fixture_buzzer',lambda f:events['new'].append(('buzzer',bool(f.reg(1)))))
  if not allow:
   def halt(f):raise HaltReached()
   p.old.hook(MAP['millis'],halt);p.new.hook_symbol('ui_fixture_millis',halt)
  for side,f in [('old',p.old),('new',p.new)]:
   try:
    f.call(MAP[name] if side=='old' else f.address(name),[] if name=='pico_halt_heater_malfunction' else [19,allow,1],max_instructions=10000000)
    assert allow,'fatal path returned'
   except HaltReached:assert not allow
  assert events['old']==events['new'],(events['old'],events['new']);p.check_display()
 case(name,f'continue={allow}',run)
report={'elf_sha256':hashlib.sha256(ELF.read_bytes()).hexdigest(),'passed_cases':len(results),'failed_cases':len(failures),'tested_functions':sorted(set(x['function'] for x in results)),'failures':failures,'cases':results}
write_report(report,'differential-workflows.json','ui-workflows-tests.json');print(json.dumps({k:report[k] for k in ['passed_cases','failed_cases','failures']},indent=2))
if failures:sys.exit(1)
