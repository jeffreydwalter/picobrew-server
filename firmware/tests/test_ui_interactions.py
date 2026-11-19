#!/usr/bin/env python3
from ui_test_support import *
results=[];failures=[]
def case(name,label,run):
 try:run();results.append({'function':name,'case':label})
 except Exception as e:failures.append({'function':name,'case':label,'error':str(e)});print('FAIL',name,label,str(e),flush=True)
# All scalar encoder operations against byte-for-byte owned state.
rng=random.Random(8612)
for i in range(200):
 def run(i=i):
  p=Pair();data=bytearray(rng.randrange(256) for _ in range(44));minimum=rng.randint(-20,10);maximum=minimum+rng.randint(1,30);position=rng.randint(minimum-10,maximum+10)
  struct.pack_into('<iiiii',data,16,minimum,maximum,0,1,position);data[40]=i&1;p.old.write(0x2002bc28,data);p.new.write(p.encoder,data)
  r1,r2=p.call('pico_encoder_get_position',[0x2002bc28],new_args=[p.encoder]);assert r1['r0']==r2['r0'];assert p.old.read(0x2002bc28,44)==p.new.read(p.encoder,44)
 case('pico_encoder_get_position',str(i),run)
for name,extra in [('pico_encoder_set_step',[5]),('pico_encoder_set_range',[u32(-4),19,7]),('pico_encoder_enable_wrap',[]),('pico_encoder_set_position',[4]),('pico_encoder_clear_click',[]),('pico_encoder_take_click',[])]:
 for latch in [0,1,255]:
  def run(name=name,extra=extra,latch=latch):
   p=Pair();data=bytearray(range(44));data[12]=latch;p.old.write(0x2002bc28,data);p.new.write(p.encoder,data);r1,r2=p.call(name,[0x2002bc28,*extra],new_args=[p.encoder,*extra]);assert p.old.read(0x2002bc28,44)==p.new.read(p.encoder,44)
   if name.endswith('take_click'):assert r1['r0']==r2['r0']
  case(name,str(latch),run)
for prev in range(4):
 for stable in range(4):
  for current in range(4):
   for wrap in [0,1]:
    def run(prev=prev,stable=stable,current=current,wrap=wrap):
     p=Pair();data=bytearray(44);data[:3]=bytes([7,8,9]);data[40:44]=bytes([wrap,prev,0,stable]);struct.pack_into('<iiiii',data,16,-1,1,0,1,1);p.old.write(0x2002bc28,data);p.new.write(p.encoder,data)
     p.old.hook(MAP['digitalRead'],lambda f:current//2 if f.reg(0)==7 else current&1);p.new.hook_symbol('ui_fixture_read',lambda f:current//2 if f.reg(1)==7 else current&1)
     p.call('pico_encoder_handle_rotation',[0x2002bc28],new_args=[p.encoder]);assert p.old.read(0x2002bc28,44)==p.new.read(p.encoder,44)
    case('pico_encoder_handle_rotation',f'{prev},{stable},{current},{wrap}',run)
# GPIO/SPI/time/interrupt call order, including transfer expansion.
def hardware_hooks(p):
 old_events=[];new_events=[]
 def add(events,label,args):events.append((label,*args));return 0
 for oldname,newname,label,n in [('digitalWrite','ui_fixture_write','write',2),('pinMode','ui_fixture_pin_mode','mode',2),('delay','ui_fixture_delay','delay',1),('SPI_begin','ui_fixture_spi_begin','spi_begin',0),('SPI_transfer_byte','ui_fixture_spi','spi',1),('IntMasterDisable','ui_fixture_disable','disable',0),('IntMasterEnable','ui_fixture_enable','enable',0)]:
  start=1 if oldname=='SPI_transfer_byte' else 0
  p.old.hook(MAP[oldname],lambda f,label=label,n=n,start=start:add(old_events,label,[f.reg(k+start) for k in range(n)]));p.new.hook_symbol(newname,lambda f,label=label,n=n:add(new_events,label,[f.reg(k+1) for k in range(n)]))
 return old_events,new_events
for what in ['initialize','flush','command','invert0','invert1','encoder_init','button_low','button_high']:
 def run(what=what):
  p=Pair();old_events,new_events=hardware_hooks(p)
  if what=='initialize':p.call('pico_display_initialize');p.check_display()
  elif what=='flush':
   p.old.unhook(MAP['pico_display_flush']);p.new.unhook(p.new.address('pico_display_flush'));data=bytes(rng.randrange(256) for _ in range(1024));p.old.write(0x2002af60,data);p.new.write(p.fb,data);p.call('pico_display_flush')
  elif what=='command':p.old.call(MAP['oled_write_command'],[0x2002bc00,0xa7]);p.new.call_symbol('pico_oled_command',[p.display,0xa7])
  elif what.startswith('invert'):v=int(what[-1]);p.old.call(MAP['OLED_invert_display'],[0x2002bc00,v]);p.new.call_symbol('pico_oled_invert',[p.display,v])
  elif what=='encoder_init':
   p.old.hook(MAP['digitalRead'],lambda f:(old_events.append(('read',f.reg(0))) or 1));p.new.hook_symbol('ui_fixture_read',lambda f:(new_events.append(('read',f.reg(1))) or 1))
   def irq(events,f,new):
    shift=1 if new else 0;address=f.reg(1+shift)&~1;rotation=p.new.address('pico_encoder_rotation_isr')&~1 if new else 0x20004714
    events.append(('attach',f.reg(shift),'rotation' if address==rotation else 'button',f.reg(2+shift)))
   p.old.hook(MAP['attachInterrupt'],lambda f:irq(old_events,f,False));p.new.hook_symbol('ui_fixture_attach',lambda f:irq(new_events,f,True));p.call('pico_encoder_initialize',[0x2002bc28,6,7,8],[9,10,11],new_args=[p.encoder,6,7,8]);assert p.old.read(0x2002bc28,44)==p.new.read(p.encoder,44)
  else:
   value=what=='button_high';p.old.hook(MAP['digitalRead'],lambda f:(old_events.append(('read',f.reg(0))) or value));p.new.hook_symbol('ui_fixture_read',lambda f:(new_events.append(('read',f.reg(1))) or value));p.old.call(0x20004740);p.new.call_symbol('pico_encoder_button_isr');assert p.old.read(0x2002bc28,44)==p.new.read(p.encoder,44)
  assert old_events==new_events,('event order mismatch',old_events[:30],new_events[:30])
 case('hardware_'+what,what,run)
# Formatting reproduces the original single-precision truncation algorithm.
for i,value in enumerate([0.0,-0.0,-0.7,-5.29,5.29,99.99,153.7,123.456]+[rng.uniform(-200,220) for _ in range(50)]):
 for digits in range(4):
  def run(value=value,digits=digits):
   p=Pair();bits=struct.unpack('<I',struct.pack('<f',value))[0];p.call('pico_format_float_decimal',[bits,p.old.SCRATCH,digits]);assert p.old.cstring(p.old.SCRATCH)==p.new.cstring(p.new.SCRATCH),(p.old.cstring(p.old.SCRATCH),p.new.cstring(p.new.SCRATCH))
  case('pico_format_float_decimal',f'{value},{digits}',run)
# Menu interaction scripts inspect the actual menu loop and native drawing.
for mode in ['click','scroll_down','scroll_up','timeout','filter_missing','still_fault','redraw']:
 for count in [1,3,10,22]:
  def run(mode=mode,count=count):
   p=Pair();p.old.unhook(MAP['pico_menu_choose']);p.new.unhook(p.new.address('pico_menu_choose'))
   for i in range(22):data=(f'Choice {i}'.encode()+b'\0').ljust(22,b'\0');p.old.write(0x2002bf24+22*i,data);p.new.write(p.labels+22*i,data)
   ticks={'old':0,'new':0};reads={'old':0,'new':0};clicks={'old':0,'new':0};events={'old':[],'new':[]}
   def now(f,side):ticks[side]+=1;return ticks[side]*1001
   def pos(f,side):
    reads[side]+=1
    if mode=='scroll_down':return min(3+reads[side]//2,3+count-1)
    if mode=='scroll_up':return max(3+count-1-reads[side]//2,3)
    return 3
   def click(f,side):clicks[side]+=1;return 255 if clicks[side]>6 and mode not in ['timeout','still_fault'] else 0
   for side,f in [('old',p.old),('new',p.new)]:
    f.hook(MAP['millis'] if side=='old' else f.address('ui_fixture_millis'),lambda f,side=side:now(f,side))
    f.hook(MAP['pico_encoder_get_position'] if side=='old' else f.address('pico_encoder_get_position'),lambda f,side=side:pos(f,side))
    f.hook(MAP['pico_encoder_take_click'] if side=='old' else f.address('pico_encoder_take_click'),lambda f,side=side:click(f,side))
    f.hook(MAP['pico_controller_pause_until_resume'] if side=='old' else f.address('ui_fixture_pause'),lambda f,side=side:events[side].append('pause'))
    f.hook(MAP['pico_still_has_blocking_status'] if side=='old' else f.address('pico_still_blocked'),lambda f,side=side:(events[side].append('still') or (mode=='still_fault')))
    f.hook(MAP['pico_step_filter_is_inserted'] if side=='old' else f.address('pico_filter_is_inserted'),lambda f:mode!='filter_missing')
   if mode=='filter_missing':p.old.write(0x2002d35a,b'\0');p.new.write(p.app+0x62,b'\0')
   if mode=='still_fault':p.old.write(0x2002d359,b'\x09');p.new.write(p.app+0x61,b'\x09')
   if mode=='redraw':
    p.old.write_u32(0x2002d9d0,0);p.old.write_u32(0x2002d9e0,MAP['pico_screen_changing_step']|1);p.new.write_u32(p.app+p.offsets[12],0);p.new.write_u32(p.app+p.offsets[12]+16,p.new.address('pico_screen_changing_step'))
   r1,r2=p.call('pico_menu_choose_with_timeout',[3,count,count-1 if mode=='scroll_up' else 0,mode=='timeout'],[count-1,mode=='redraw',2],max_instructions=20000000);assert r1['r0']==r2['r0'];assert events['old']==events['new'];assert ticks['old']==ticks['new'];p.check_display();p.check_labels()
  case('pico_menu_choose_with_timeout',f'{mode},count={count}',run)
# Newly identified original carousel/confirmation entry points.
for original,new in [(0x2000477c,'pico_screen_manual_overview_page'),(0x20007e78,'pico_screen_current_target_page'),(0x200074d8,'pico_screen_basic_brew_status'),(0x20008808,'pico_screen_detailed_brew_status'),(0x20008968,'pico_screen_temperature_page'),(0x20009bc0,'pico_screen_current_step_page'),(0x20009b74,'pico_screen_program_status_page'),(0x20005438,'pico_screen_thermal_recovery_help'),(0x2000a9dc,'pico_ui_draw_deep_clean_confirmation')]:
 for lang in range(3):
  for celsius in [0,1]:
   def run(original=original,new=new,lang=lang,celsius=celsius):
    p=Pair(lang,celsius);p.old.call(original);p.new.call_symbol(new);p.check_display()
   case(new,f'language={lang},celsius={celsius}',run)
report={'elf_sha256':hashlib.sha256(ELF.read_bytes()).hexdigest(),'passed_cases':len(results),'failed_cases':len(failures),'tested_functions':sorted(set(x['function'] for x in results)),'failures':failures,'cases':results}
write_report(report,'differential-interactions.json','ui-interactions-tests.json');print(json.dumps({k:report[k] for k in ['passed_cases','failed_cases','failures']},indent=2))
if failures:sys.exit(1)
