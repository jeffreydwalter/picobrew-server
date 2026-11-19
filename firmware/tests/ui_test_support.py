#!/usr/bin/env python3
"""Differential ARM execution of independently linked UI and original firmware.
Hardware/network entry points are observed explicitly. This is not appliance
or SDK integration simulation and the ELF is never intended for installation.
"""
from pathlib import Path
import json,struct,sys,random,hashlib
HERE=Path(__file__).resolve().parent
# Works in the standalone project's tests/ and in the reconstruction workspace.
PROJECT=next((parent for parent in HERE.parents if (parent/'reference/original-0.1.34.bin').is_file()),None)
STAGING=PROJECT is None
if STAGING:
 PROJECT=next(parent/'picobrew-application' for parent in HERE.parents if (parent/'picobrew-application/reference/original-0.1.34.bin').is_file())
ROOT=PROJECT.parent
WORK=HERE.parent if STAGING else PROJECT
REPORT_DIR=WORK/'reports' if STAGING else PROJECT/'analysis'
ASSET_MANIFEST=REPORT_DIR/('assets.json' if STAGING else 'ui-assets.json')
sys.path.insert(0,str(PROJECT/'tools'))
from original_arm import words_from_double
from fast_arm import ArmFirmware,RebuiltArm
MAP=json.loads((HERE/'ui_original_entries.json').read_text())
import argparse
_parser=argparse.ArgumentParser(add_help=False)
_parser.add_argument('--elf',type=Path,default=WORK/'build'/('ui.elf' if STAGING else 'application-validation.elf'))
_parser.add_argument('--output',type=Path)
_options,_unused=_parser.parse_known_args()
ELF=_options.elf
def write_report(report,staging_name,final_name):
 output=_options.output or REPORT_DIR/(staging_name if STAGING else final_name)
 output.parent.mkdir(parents=True,exist_ok=True)
 output.write_text(json.dumps(report,indent=2)+'\n')
def u32(x):return x&0xffffffff
def words(data):return struct.unpack('<'+'I'*(len(data)//4),data)
def put_double(f,a,v):f.write(a,struct.pack('<d',v))
class Pair:
 def __init__(self,lang=0,celsius=0,detailed=0):
  self.old=ArmFirmware();self.new=RebuiltArm(ELF);self.new.call_symbol('ui_fixture_bind')
  self.offsets=words(self.new.read(self.new.address('ui_fixture_offsets'),17*4))
  self.ui=self.new.address('ui_fixture');self.app=self.new.address('fixture_app');self.settings=self.new.address('fixture_settings')
  self.display=self.ui+self.offsets[0];self.encoder=self.ui+self.offsets[1];self.labels=self.ui+self.offsets[2];self.fb=self.display+self.offsets[3]
  old=self.old;new=self.new
  old.call(MAP['pico_display_construct']);old.call(MAP['GFX_set_text_colors'],[0x2002bc00,1,0]);old.call(MAP['GFX_set_rotation'],[0x2002bc00,2]);old.write(0x2002af60,b'\0'*1024)
  old.write(0x2002d2f8,new.read(self.app,0x228));old.write(0x2002bcd8,new.read(self.app+self.offsets[9],0x238));old.write_u32(0x2002bcd8,0x2002c108)
  old.write(0x2002d524,new.read(self.app+self.offsets[10],0x4c));old.write(0x2002c108,new.read(self.app+self.offsets[11],0x58))
  # Relocate only declared native string/bitmap pointers into test caller memory.
  for offset in [0x34,0x38,0x3c,0x40,0x44]:
   ptr=new.read_u32(self.app+self.offsets[11]+offset);a=0x20081000+offset*16;old.write(a,new.cstring(ptr)+b'\0');old.write_u32(0x2002c108+offset,a)
  old.write_u32(0x2002c150,0x20023234);old.write_u32(0x2002c154,0x20023248)
  old.write(0x2002bc28,new.read(self.encoder,44));old.write(0x2002bf24,new.read(self.labels,22*22));old.write(0x2002bc74,b'\0'*92)
  for a,b,v in [(0x2002c164,self.settings+self.offsets[6],lang),(0x2002d9c9,self.settings+self.offsets[7],celsius),(0x2002d57f,self.settings+self.offsets[8],detailed)]:old.write(a,bytes([v]));new.write(b,bytes([v]))
  self.old_flush=[];self.new_flush=[];self.events_old=[];self.events_new=[]
  old.hook(MAP['pico_log_serial'],lambda f:None);old.hook(MAP['pico_assert_nonfatal'],lambda f:None)
  new.hook_symbol('pico_network_log',lambda f:None);new.hook_symbol('pico_network_assert',lambda f:None)
  old.hook(MAP['pico_display_flush'],lambda f:self.old_flush.append(f.read(0x2002af60,1024)));new.hook_symbol('pico_display_flush',lambda f:self.new_flush.append(f.read(self.fb,1024)))
  old.hook(MAP['millis'],lambda f:12345);new.hook_symbol('ui_fixture_millis',lambda f:12345)
  old.hook(MAP['delay'],lambda f:None);new.hook_symbol('ui_fixture_delay',lambda f:None)
  old.hook(MAP['IntMasterEnable'],lambda f:False);new.hook_symbol('ui_fixture_enable',lambda f:False)
  old.hook(MAP['IntMasterDisable'],lambda f:False);new.hook_symbol('ui_fixture_disable',lambda f:None)
  old.hook(MAP['pico_display_wait_for_click'],lambda f:None);new.hook_symbol('pico_display_wait_for_click',lambda f:None)
  old.hook(MAP['pico_ui_wait_for_continue'],lambda f:0);new.hook_symbol('pico_ui_wait_for_continue',lambda f:0)
  old.hook(MAP['pico_step_filter_is_inserted'],lambda f:1);new.hook_symbol('pico_filter_is_inserted',lambda f:1)
  old.hook(MAP['pico_menu_choose'],lambda f:0);new.hook_symbol('pico_menu_choose',lambda f:0)
  for f in [old,new]:f.write(f.SCRATCH,b'Test title\0'+b'\0'*500)
 def call(self,name,args=(),stack=(),new_args=None,new_stack=None,max_instructions=1000000):
  r1=self.old.call(MAP[name],args,stack_args=stack,max_instructions=max_instructions);r2=self.new.call_symbol(name,args if new_args is None else new_args,stack_args=stack if new_stack is None else new_stack,max_instructions=max_instructions)
  return r1,r2
 def check_display(self):
  assert self.old.read(0x2002af60,1024)==self.new.read(self.fb,1024),'framebuffer mismatch'
  assert self.old.read(0x2002bc10,8)==self.new.read(self.display+4,8),'cursor/colors mismatch'
  assert self.old_flush==self.new_flush,'flush frame sequence mismatch'
 def check_labels(self):assert self.old.read(0x2002bf24,484)==self.new.read(self.labels,484),'menu labels mismatch'
