#!/usr/bin/env python3
"""Extract immutable UI data from the provenance-pinned original image.
No original machine addresses are emitted into application source.
"""
from pathlib import Path
import hashlib,json,struct,sys,re
HERE=Path(__file__).resolve().parent
PROJECT=next((parent for parent in HERE.parents if (parent/'reference/original-0.1.34.bin').is_file()),None)
STAGING=PROJECT is None
if STAGING:
 PROJECT=next(parent/'picobrew-application' for parent in HERE.parents if (parent/'picobrew-application/reference/original-0.1.34.bin').is_file())
ROOT=PROJECT.parent
OUT=HERE.parent if STAGING else PROJECT
SOURCE=OUT/'src' if STAGING else PROJECT/'src/ui'
REPORT=OUT/'reports/assets.json' if STAGING else PROJECT/'analysis/ui-assets.json'
sys.path.insert(0,str(PROJECT/'tools'))
from original_arm import ArmFirmware
REPORT.parent.mkdir(parents=True,exist_ok=True)
b=(PROJECT/'reference/original-0.1.34.bin').read_bytes()
assert hashlib.sha256(b).hexdigest()=='e18c240fa1cf35222674bf3b4fa992a306c6e63b0b99be8793604f199085d702'
def data(a,n): return b[a-0x20004000:a-0x20004000+n]
def word(a): return struct.unpack('<I',data(a,4))[0]
def string(a):return data(a,len(b)).split(b'\0')[0].decode('latin1')
def quoted(s):
 return re.sub(r'\\u00([0-9a-fA-F]{2})',lambda m:'\\%03o'%int(m[1],16),json.dumps(s,ensure_ascii=True))
assets={}
def asset(name,a,n):
 value=data(a,n)
 assert len(value)==n,(name,hex(a),n)
 assets[name]=(a,value)
 return name
asset('pico_ui_builtin_font',0x20028c98,1280)
for n,a,w,h in [('footer_single',0x20023108,8,8),('footer_multiple',0x20028728,8,8),('connection_ready',0x200231a8,24,5),('adjustment_filled',0x20023240,8,8),('connection_failed',0x20023254,24,5),('adjustment_empty',0x2002327c,8,8),('device',0x20028928,40,48),('instruction_18',0x20029e60,128,30),('instruction_11',0x2002a5e0,128,16),('instruction_17',0x2002a910,128,16)]:asset('pico_ui_bitmap_'+n,a,((w+7)//8)*h)
for i in range(10):asset('pico_ui_digit_'+str(i),word(0x2002a8c0+4*i),32)
for i in range(8):asset('pico_ui_setup_'+str(i),word(0x2002aa20+4*i),480)
descriptors={}
for n,a in [('program_primary',0x20023234),('program_secondary',0x20023248),('sousvide_secondary',0x200285a0),('temperature_footer',0x20023264),('temperature_header',0x20023330),('manual_overview',0x20023270)]:
 p,w,h=struct.unpack('<III',data(a,12));name=asset('pico_ui_bitmap_'+n,p,((w+7)//8)*h);descriptors[n]=(name,w,h)
for name,addr in [('instruction_names',0x20023130),('brew_labels',0x20028c34),('titles',0x2002af24)]:
 count={'instruction_names':25,'brew_labels':12,'titles':15}[name]
 globals()[name]=[string(word(addr+4*i)) for i in range(count)]
# Recover the variable argument calls actually made by each instruction screen.
instructions=[]
for sid in range(25):
 langs=[]
 for lang in range(3):
  f=ArmFirmware();lines=[]
  f.hook(0x20004a94,lambda f:None)
  def capture(f):
   count=f.reg(1);ptrs=[f.reg(2),f.reg(3)]+[f.read_u32(f.reg('sp')+4*i) for i in range(max(0,count-2))]
   lines.extend(f.cstring(p).decode('latin1') for p in ptrs[:count])
  f.hook(0x20004ed8,capture)
  f.call(0x20006e64,[f.SCRATCH,sid,lang])
  langs.append(lines)
 instructions.append(langs)
# Literal bitmap pointers otherwise displayed as empty strings by the decompiler.
trace={}
for name,addr,args in [('logo',0x20005578,[]),('timer',0x20007b30,[125,7,1]),('temperature',0x20007c90,[0,0x40590000,1])]:
 f=ArmFirmware();records=[]
 f.hook(0x20004494,lambda f:None)
 def bitmap(f):
  p=f.reg(3);w=f.read_u32(f.reg('sp'))&0xffff;h=f.read_u32(f.reg('sp')+4)&0xffff
  records.append({'address':p,'width':w,'height':h,'x':f.reg(1),'y':f.reg(2)})
 f.hook(0x2001412c,bitmap);f.hook(0x200141aa,bitmap)
 f.call(0x20004540)
 f.call(addr,args)
 trace[name]=records
 for r in records:
  if r['address'] not in [a for a,v in assets.values()]:asset({0x20028600:'pico_ui_bitmap_logo',0x20023320:'pico_ui_bitmap_colon_dot'}[r['address']],r['address'],((r['width']+7)//8)*r['height'])
# Recover editor control glyphs from the first rendered frame of the original
# editor, not by treating decompiler string literals as image pointers.
class FrameDone(Exception):pass
glyphs={};sizes={}
for selected in [1,3,4,98]:
 f=ArmFirmware();records=[]
 f.hook(0x200045ea,lambda f:0);f.hook(0x200045b6,lambda f:selected)
 def bitmap(f):records.append((f.reg(1),f.reg(2),f.reg(3),f.read_u32(f.reg('sp'))&0xffff,f.read_u32(f.reg('sp')+4)&0xffff))
 def done(f):raise FrameDone()
 f.hook(0x2001412c,bitmap);f.hook(0x20004494,done);f.call(0x20004540)
 try:f.call(0x20009c98,[f.SCRATCH,64])
 except FrameDone:pass
 glyphs[selected]=records
 for x,y,address,w,h in records:sizes[address]=((w+7)//8)*h
names={}
for selection,label in [(1,'done'),(3,'backspace'),(98,'space')]:
 for x,y,address,w,h in glyphs[selection]:
  if w==16:names[address]='pico_ui_editor_'+label+'_large'
for selection,records in glyphs.items():
 first=(selection+90)%99
 for x,y,address,w,h in records:
  if w!=8:continue
  index=(first+x//6 if x<54 else first+10+(x-78)//6)%99
  if index in (1,3,98):names[address]='pico_ui_editor_'+{1:'done',3:'backspace',98:'space'}[index]+'_small'
for address,name in names.items():asset(name,address,sizes[address])
f=ArmFirmware();splash=[]
def bitmap(f):splash.append((f.reg(3),f.read_u32(f.reg('sp'))&0xffff,f.read_u32(f.reg('sp')+4)&0xffff))
f.hook(0x2001412c,bitmap);f.hook(0x20004494,lambda f:None);f.call(0x20004540);f.call(0x20009588)
assert len(splash)==1
address,w,h=splash[0];asset('pico_ui_bitmap_splash',address,((w+7)//8)*h)
thermal_help=[]
for language in range(3):
 f=ArmFirmware();lines=[];f.write(0x2002c164,bytes([language]))
 f.hook(0x200052dc,lambda f:lines.append([f.reg(0),f.cstring(f.reg(1)).decode('latin1')]))
 f.call(0x20005438);thermal_help.append(lines)
error_text=[]
mapping=json.loads((OUT/'tests/ui_original_entries.json').read_text())
for error in [*range(25),99]:
 langs=[]
 for language in range(3):
  f=ArmFirmware();lines={};f.write(0x2002c164,bytes([language]))
  f.hook(0x200052dc,lambda f:lines.__setitem__(f.reg(0),f.cstring(f.reg(1)).decode('latin1')))
  # Text scale and separator perform no text-data accesses relevant here.
  # The original entry addresses are recorded separately in the inventory.
  f.hook(mapping['GFX_set_text_size'],lambda f:None)
  f.hook(mapping['pico_ui_draw_separator'],lambda f:None)
  f.call(mapping['pico_ui_draw_error'],[error])
  langs.append([lines[0],lines[1]])
 error_text.append(langs)
source=['/* Immutable native UI assets extracted from original 0.1.34. */','#include "ui_internal.h"']
for name,(addr,value) in assets.items():
 source.append('const uint8_t %s[%d] = {\n%s\n};'%(name,len(value),'\n'.join('    '+', '.join('0x%02x'%x for x in value[i:i+16])+',' for i in range(0,len(value),16))))
for name,(arr,w,h) in descriptors.items():source.append('const PicoBitmap pico_ui_asset_%s = {%s, %d, %d};'%(name,arr,w,h))
source.append('const uint8_t *const pico_ui_digits[10] = {'+', '.join('pico_ui_digit_'+str(i) for i in range(10))+'};')
# Preserve each extracted array for audit, but share identical setup pixels in
# the live table so the production linker can discard two unreferenced copies.
setup_indices=(0,1,2,3,4,5,5,1)
for i,shared in enumerate(setup_indices):
 assert assets['pico_ui_setup_'+str(i)][1]==assets['pico_ui_setup_'+str(shared)][1],(i,shared)
source.append('const uint8_t *const pico_ui_setup_images[8] = {'+', '.join('pico_ui_setup_'+str(i) for i in setup_indices)+'};')
for name,ls,cols in [('instruction_names',instruction_names,1),('brew_labels',brew_labels,3),('titles',titles,3)]:
 dims='[%d]'%len(ls) if cols==1 else '[%d][%d]'%(len(ls)//cols,cols)
 source.append('const char *const pico_ui_%s%s = {'%(name,dims))
 for i in range(0,len(ls),cols):source.append('    '+('' if cols==1 else '{')+', '.join(quoted(x) for x in ls[i:i+cols])+('' if cols==1 else '}')+',')
 source.append('};')
source.append('const PicoInstructionText pico_ui_instructions[25][3] = {')
for langs in instructions:
 source.append('    {')
 for lines in langs:source.append('        {%d, {%s}},'%(len(lines),', '.join(quoted(x) for x in lines) if lines else 'NULL'))
 source.append('    },')
source.append('};')
source.append('const char *const pico_ui_thermal_help[3][6] = {')
for lines in thermal_help:source.append('    {'+', '.join(quoted(text) for row,text in lines)+'},')
source.append('};')
source.append('const char *const pico_ui_error_text[26][3][2] = {')
for langs in error_text:source.append('    {'+', '.join('{'+', '.join(quoted(text) for text in lines)+'}' for lines in langs)+'},')
source.append('};')
(SOURCE/'assets.c').write_text('\n'.join(source)+'\n')
header=['#ifndef PICOBREW_UI_ASSETS_H','#define PICOBREW_UI_ASSETS_H']
for name,(addr,value) in assets.items():header.append('extern const uint8_t %s[%d];'%(name,len(value)))
for name in descriptors:header.append('extern const PicoBitmap pico_ui_asset_%s;'%name)
header+=['extern const uint8_t *const pico_ui_digits[10];','extern const uint8_t *const pico_ui_setup_images[8];','extern const char *const pico_ui_instruction_names[25];','extern const char *const pico_ui_brew_labels[4][3];','extern const char *const pico_ui_titles[5][3];','extern const PicoInstructionText pico_ui_instructions[25][3];','extern const char *const pico_ui_thermal_help[3][6];','extern const char *const pico_ui_error_text[26][3][2];','#endif']
(SOURCE/'ui_assets.h').write_text('\n'.join(header)+'\n')
REPORT.write_text(json.dumps({'image_sha256':hashlib.sha256(b).hexdigest(),'assets':[{'name':n,'source_address':hex(a),'size':len(v),'sha256':hashlib.sha256(v).hexdigest()} for n,(a,v) in assets.items()],'literal_bitmap_traces':trace,'editor_traces':glyphs,'splash_trace':splash,'descriptors':{n:{'array':arr,'width':w,'height':h} for n,(arr,w,h) in descriptors.items()},'instruction_lines':instructions,'thermal_help':thermal_help,'error_text':error_text},indent=2)+'\n')
print('Extracted',len(assets),'assets:',sum(len(v) for a,v in assets.values()),'bytes;',len(instructions)*3,'instruction variants')
