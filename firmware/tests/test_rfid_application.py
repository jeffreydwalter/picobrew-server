#!/usr/bin/env python3
"""Execute complete UID/cancel/filter-timeout flows through real recovered RFID code."""
import argparse,hashlib,json,struct,sys
from pathlib import Path
from test_rfid import ReaderModel
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from original_arm import ArmFirmware,IMAGE_SHA256
from rebuilt_arm import RebuiltArm
ENTRIES={'initialize':(0x2000c900,'pico_rfid_application_initialize'),
 'uid_wait':(0x20006070,'pico_rfid_read_uid_until_click'),'uid':(0x20006104,'pico_rfid_read_uid'),
 'pack_wait':(0x2000c94c,'pico_rfid_wait_for_picopak'),'pack':(0x2000ca34,'pico_rfid_read_picopak')}
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--elf',type=Path,default=ROOT/'build/application-validation.elf');p.add_argument('--output',type=Path,default=ROOT/'analysis/rfid-application-tests.json');o=p.parse_args()
 names={f['name']:int(f['address'],16) for f in json.loads((ROOT/'reference/function_map.json').read_text())};records=[]
 def execute(operation,scenario,capacity=22,repair=False):
  outputs=[]
  for rebuilt in ((True,) if repair else (False,True)):
   m=RebuiltArm(o.elf) if rebuilt else ArmFirmware()
   if rebuilt:
    m.call_symbol('rfid_fixture_bind_application');reader=m.address('rfid_fixture_reader');application=m.address('rfid_fixture_application');buffer=m.address('rfid_fixture_result')
   else:reader,application,buffer=0x2002d570,0,0x20081000
   model=ReaderModel(scenario);m.write(reader,bytes.fromhex('000102030405060708090a00041200'));m.write(buffer,b'\0'+b'\xa5'*127)
   counters={}
   def value(key,default):
    i=counters.get(key,0);counters[key]=i+1;values=scenario.get(key,default);return values[min(i,len(values)-1)]
   specs=[('pinMode','rfid_fixture_pin_mode',lambda x:model.event('mode',x.reg(1 if rebuilt else 0),x.reg(2 if rebuilt else 1))),('digitalWrite','rfid_fixture_write',lambda x:model.gpio(x.reg(1 if rebuilt else 0),x.reg(2 if rebuilt else 1))),('SPI_transfer_byte','rfid_fixture_transfer',lambda x:model.transfer(x.reg(1))),('SPI_set_bit_order','rfid_fixture_order',lambda x:model.event('order',x.reg(1))),('SPI_set_data_mode','rfid_fixture_mode',lambda x:model.event('spi_mode',x.reg(1))),('delay','rfid_fixture_delay',lambda x:model.event('delay',x.reg(1 if rebuilt else 0))),('pico_encoder_clear_click','rfid_fixture_clear_click',lambda x:model.event('clear_click')),('pico_ui_draw_insert_picopak','rfid_fixture_screen',lambda x:model.event('screen')),('pico_display_flush','rfid_fixture_flush',lambda x:model.event('flush'))]
   def observed(name,key,default):
    def callback(machine):v=value(key,default);model.event(name,v);return v
    return callback
   specs += [('pico_encoder_take_click','rfid_fixture_click',observed('click','clicks',[0,0,1])),('pico_step_filter_is_inserted','rfid_fixture_filter',observed('filter','filters',[1])),('millis','rfid_fixture_clock',observed('time','ticks',[1000,1001,17002]))]
   specs += [('pico_log_serial','rfid_fixture_log',lambda x:model.event('log',*(x.cstring(x.reg(i+(1 if rebuilt else 0))).decode() for i in range(3)))),('pico_report_error','rfid_fixture_error',lambda x:model.event('error',*(x.reg(i+(1 if rebuilt else 0)) for i in range(3))))]
   for old,new,callback in specs:
    if rebuilt:m.hook_symbol(new,callback)
    else:m.hook(names[old],callback)
   old,new=ENTRIES[operation]
   args=[application] if rebuilt else []
   if operation!='initialize':args += [buffer]+([capacity] if rebuilt or operation.startswith('pack') else [])
   result=m.call(m.address(new) if rebuilt else old,args,max_instructions=5000000)
   outputs.append({'return':result['r0'] if operation!='initialize' else None,'reader':m.read(reader,15).hex(),'buffer':m.read(buffer,128).hex(),'events':model.events})
  if repair:
   assert outputs[0]['return']==0
   assert outputs[0]['buffer'][capacity*2:]==('a5'*(128-capacity)) if capacity else outputs[0]['buffer']=='00'+'a5'*127
  elif outputs[0]!=outputs[1]:
   diag={'operation':operation,'scenario':repr(scenario),'original':outputs[0],'rebuilt':outputs[1]};(ROOT/'analysis/rfid-application-failure.json').write_text(json.dumps(diag,indent=2)+'\n');raise AssertionError(str(diag)[:5000])
  records.append({'operation':operation,'scenario':repr(scenario),'capacity':capacity,'repair':repair,'events':len(outputs[0]['events'])})
 execute('initialize',{})
 for operation in ('uid_wait','uid','pack_wait','pack'):
  for uid in ('04112233','04112233445566','04112233445566778899'):
   execute(operation,{'uid':bytes.fromhex(uid)});execute(operation,{'uid':bytes.fromhex(uid),'clicks':[1]})
  execute(operation,{'mode':'timeout','clicks':[0,0,1]})
  execute(operation,{'bad_sak_crc':True,'clicks':[0,1]})
  execute(operation,{'mode':'collision','collision':0x20,'clicks':[1]})
  if operation.startswith('pack'):
   execute(operation,{'filters':[0]});execute(operation,{'mode':'timeout','clicks':[0],'ticks':[0,1,15001,15002]})
 for operation in ('uid_wait','uid','pack_wait','pack'):
  for capacity in (0,1,2,8,20):execute(operation,{'uid':bytes.fromhex('04112233445566778899')},capacity,True)
 report={'passed':len(records),'failed':0,'exact_original_rebuilt_cases':sum(not x['repair'] for x in records),'repair_cases':sum(x['repair'] for x in records),'firmware_sha256':IMAGE_SHA256,'elf_sha256':hashlib.sha256(o.elf.read_bytes()).hexdigest(),'scope':'Application UID reads run through actual RFID register/FIFO/CRC/select code, deterministic SPI model, encoder/filter/time boundaries. Error, cancellation, filter timeout and UID lengths compared; capacity repairs checked with canaries.','cases':records}
 o.output.write_text(json.dumps(report,indent=2)+'\n');print('RFID application:',len(records),'cases passed.')
if __name__=='__main__':main()
