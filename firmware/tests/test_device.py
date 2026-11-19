#!/usr/bin/env python3
"""Execute original and rebuilt auxiliary-board protocols and device workflows."""
import argparse,hashlib,json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from original_arm import ArmFirmware,IMAGE_SHA256
from rebuilt_arm import RebuiltArm
ENTRIES={
 'initialize':(0x2000621c,'pico_device_spi_initialize'),
 'selected':(0x2000624c,'pico_device_spi_selected_byte'),
 'command':(0x2000629c,'pico_device_spi_command'),
 'ready':(0x2000ca5c,'pico_device_stepper_ready'),
 'wait':(0x2000ca7e,'pico_device_wait_stepper'),
 'on':(0x2000cb40,'pico_device_buzzer'),'off':(0x2000cb46,'pico_device_buzzer'),
 'capabilities':(0x2000ccac,'pico_device_stepper_capabilities'),
 'detect':(0x2000ccd8,'pico_device_detect_stepper'),
 'torque':(0x2000cd24,'pico_device_set_torque'),
 'move':(0x2000cd4c,'pico_device_move_compartment'),
 'home':(0x2000d40c,'pico_device_move_home'),
 'filter_init':(0x20005078,'pico_device_filter_input_initialize'),
 'temperature_init':(0x20006666,'pico_device_temperature_inputs_initialize'),
 'filter_wait':(0x20004f88,'pico_device_wait_for_filter'),
 'service':(0x20009674,'pico_device_service_button'),
 'filter_alarm':(0x2000cc00,'pico_device_alarm_while_filter_missing'),
 'click_alarm':(0x2000cc50,'pico_device_alarm_until_encoder_press'),
 'heat_check':(0x2000c880,'pico_device_check_thermoblock_heating')}
RETURNS={'selected','command','ready','wait','on','off','capabilities','torque','move','service','heat_check','temperature_init'}
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--elf',type=Path,default=ROOT/'build/application-validation.elf');p.add_argument('--output',type=Path,default=ROOT/'analysis/device-tests.json');a=p.parse_args()
 names={x['name']:int(x['address'],16) for x in json.loads((ROOT/'reference/function_map.json').read_text())}
 records=[]
 def execute(op,arg=0,**case):
  outputs=[]
  for rebuilt in (False,True):
   m=RebuiltArm(a.elf) if rebuilt else ArmFirmware()
   if rebuilt:
    m.call_symbol('device_fixture_bind');addrs=struct.unpack('<8I',m.read(m.address('device_fixture_addresses'),32))
   else:addrs=(0x2002d2f8,0x2002dac0,0x2002dac4,0x2002bc71,0x2002ac80,0x20081000,0,0)
   ctrl,buzz,cap,legacy,home,detail,device,control=addrs
   m.write(ctrl+0x60,bytes([2]));m.write(ctrl+0x1f8,struct.pack('<d',case.get('temperatures',[250.])[0]));m.write(buzz,bytes([case.get('buzzer',0)]));m.write_u32(cap,0x11223344);m.write(legacy,bytes([case.get('legacy',0)]));m.write(home,bytes([2]));m.write_u32(detail,0x76543210)
   events=[];counts={};tx=[];response_index=0
   def take(key,default):
    i=counts.get(key,0);counts[key]=i+1;values=case.get(key,default);return values[min(i,len(values)-1)]
   def emit(name,*values):events.append((name,*values))
   def callback(name,args):
    nonlocal response_index,tx
    if name=='time':
     val=take('ticks',[1000,2001,3002,4003,5004,20000]);emit(name,val)
     temperatures=case.get('temperatures')
     if temperatures:m.write(ctrl+0x1f8,struct.pack('<d',temperatures[min(counts['ticks']-1,len(temperatures)-1)]))
     return val
    if name=='read':
     val=take('readings',[1,1,1]);emit(name,args[0],val);return val
    if name=='filter':val=take('filters',[0,0,1]);emit(name,val);return val
    if name=='click':val=take('clicks',[0,0,1]);emit(name,val);return val
    if name=='write':
     if args[0]==18 and args[1]==0:tx=[]
    if name=='spi':
     args=[args[0]&255] # SPI.transfer's observable wire value is one byte.
     tx.append(args[0]);emit(name,*args)
     if len(tx)==1:return case.get('selected',0x73)
     if len(tx)==2:
      replies=case.get('replies',[12]);v=replies[min(response_index,len(replies)-1)];response_index+=1;return v
     raise AssertionError(tx)
    emit(name,*args);return 0
   specs=[('millis','device_fixture_millis','time',0,0),('delay','device_fixture_delay','delay',1,0),('pinMode','device_fixture_pin_mode','mode',2,0),('digitalRead','device_fixture_read','read',1,0),('digitalWrite','device_fixture_write','write',2,0),('SPI_set_data_mode','device_fixture_mode','spi_mode',1,1),('SPI_transfer_byte','device_fixture_transfer','spi',1,1),('IntMasterDisable','device_fixture_disable','disable',0,0),('IntMasterEnable','device_fixture_enable','enable',0,0),('pico_display_clear','device_fixture_clear','clear',0,0),('pico_display_flush','device_fixture_flush','flush',0,0),('pico_screen_service_status','device_fixture_service','service',0,0),('pico_screen_error_heading','device_fixture_heading','heading',0,0),('pico_encoder_clear_click','device_fixture_clear_click','clear_click',0,0),('pico_encoder_take_click','device_fixture_click','click',0,0)]
   for old,new,tag,argc,offset in specs:
    def hook(machine,tag=tag,argc=argc,offset=offset):return callback(tag,[machine.reg(i+(1 if rebuilt else offset)) for i in range(argc)])
    if rebuilt:m.hook_symbol(new,hook)
    else:m.hook(names[old],hook)
   def log(machine):return emit('log',*(machine.cstring(machine.reg(i+(1 if rebuilt else 0))).decode() for i in range(3)))
   def error(machine):return emit('error',*(machine.reg(i+(1 if rebuilt else 0)) for i in range(3)))
   if rebuilt:m.hook_symbol('device_fixture_log',log);m.hook_symbol('device_fixture_error',error)
   else:
    m.hook(names['pico_log_serial'],log);m.hook(names['pico_report_error'],error);m.hook(names['pico_assert_nonfatal'],lambda x:0)
    def format_log(machine):
     fmt=machine.cstring(machine.reg(2));assert fmt==b'moveStepperToCompartment %d',fmt
     val=struct.unpack('<i',struct.pack('<I',machine.reg(3)))[0];text=('moveStepperToCompartment %d'%val).encode();machine.write(machine.reg(0),text[:machine.reg(1)-1]+b'\0');return len(text)
    m.hook(names['libc_snprintf'],format_log)
   # These previously verified components are service boundaries in this suite.
   for old,new,tag in [('pico_step_filter_is_inserted','pico_filter_is_inserted','filter'),('pico_heater_on','pico_actuator_heater_on','heater_on'),('pico_heater_off','pico_actuator_heater_off','heater_off'),('pico_heat_controller_stop','pico_control_stop','stop'),('pico_actuators_off_except_drain','pico_actuators_off_except_drain','outputs_off')]:
    f=lambda machine,tag=tag:callback(tag,[])
    if rebuilt:m.hook_symbol(new,f)
    else:m.hook(names[old],f)
   args=[device] if rebuilt else []
   if op in ('command','selected','torque','move'):args.append(arg)
   elif op in ('on','off') and rebuilt:args.append(op=='on')
   elif op=='capabilities':args.append(detail)
   elif op=='heat_check':args.append(control if rebuilt else ctrl)
   address,symbol=ENTRIES[op]
   out=m.call(m.address(symbol) if rebuilt else address,args,max_instructions=4000000)
   outputs.append({'return':out['r0'] if op in RETURNS else None,'state':(m.read(ctrl+0x60,1)+m.read(buzz,1)+m.read(cap,4)+m.read(legacy,1)+m.read(home,1)+m.read(detail,4)).hex(),'events':events,'instructions':out['instructions']})
  for field in ('return','state','events'):
   if outputs[0][field]!=outputs[1][field]:
    diag={'operation':op,'argument':arg,'case':case,'field':field,'original':outputs[0][field],'rebuilt':outputs[1][field]};(ROOT/'analysis/device-failure.json').write_text(json.dumps(diag,indent=2)+'\n');raise AssertionError(str(diag)[:5000])
  records.append({'operation':op,'argument':arg,'case':case,'events':len(outputs[0]['events']),'original_instructions':outputs[0]['instructions']})
 for op in ('initialize','filter_init','temperature_init'):execute(op)
 for v in (0,1,11,0x29,0x37,0x58,255):
  execute('selected',v,selected=v);execute('command',v,replies=[v]);execute('ready',replies=[v])
 for failures in (0,1,2,149,150,151):execute('wait',replies=[0]*failures+[12])
 for op in ('on','off'):
  for initial in (0,1,2):
   for response in (0,2,3,255):execute(op,buzzer=initial,replies=[11,response,12])
   execute(op,buzzer=initial,replies=[0,12])
 for caps in (0,1,2,255):
  execute('capabilities',replies=[0x29,caps]);execute('detect',replies=[0x29,caps])
  execute('detect',legacy=1,replies=[0x29,caps])
 execute('capabilities',replies=[0,12]);execute('detect',replies=[0,12,12,0x29,1])
 for mode in (0,1,2,255,256,0xffffffff):
  execute('torque',mode,replies=[12,0x3b,mode&255,12]);execute('torque',mode,replies=[12,0,12])
 for legacy in (0,1):
  for compartment in (0,1,2,5,6,255,256):
   mapped=(6 if compartment==0 else (compartment-1)&255) if legacy else compartment
   for dwell in (0,1,6,7,255):execute('move',compartment,legacy=legacy,replies=[0x21,(mapped+1)&255,dwell,12])
 execute('move',2,replies=[0,12]);execute('move',2,replies=[0x21,0,12]);execute('home',replies=[0x21,3,1,12])
 for readings in ([0],[1,0],[1,1,1]):execute('service',readings=readings)
 execute('service',readings=[1,0],ticks=[1000,1000,1001]);execute('filter_wait');execute('filter_alarm',replies=[11,2,12,11,3,12],ticks=[0,1,1001,1002]);execute('click_alarm',replies=[11,2,12,11,3,12])
 execute('heat_check',temperatures=[250.]);execute('heat_check',temperatures=[80.,90.,229.,230.]);execute('heat_check',temperatures=[80.,80.,80.],ticks=[0,15000,15001]);execute('heat_check',temperatures=[100.,100.,230.],ticks=[0,20000,20001])
 report={'passed':len(records),'failed':0,'firmware_sha256':IMAGE_SHA256,'elf_sha256':hashlib.sha256(a.elf.read_bytes()).hexdigest(),'source_sha256':hashlib.sha256((ROOT/'src/device.c').read_bytes()).hexdigest(),'scope':'Original and rebuilt device instructions; exact ordered auxiliary SPI, GPIO, clock, UI/error/log boundary events and application state. Filter predicate, actuator and controller-stop functions are previously verified service boundaries.','limits':'Scripted hardware replies; no physical motor, interrupt race or heating validation.','cases':records}
 a.output.write_text(json.dumps(report,indent=2)+'\n');print('Device:',len(records),'exact original/rebuilt ARM scenarios passed.')
if __name__=='__main__':main()
