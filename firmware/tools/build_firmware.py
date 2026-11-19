#!/usr/bin/env python3
"""Build the recovered appliance entry with native CC3200 startup and inspect its image.

This produces a complete linked firmware candidate, not a hardware acceptance result.
The independent function-test ELF and test fixtures are never inputs to this link.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
from elftools.elf.elffile import ELFFile
from package_update import encode_update, decode_update

ROOT=Path(__file__).resolve().parents[1]
ORIGIN,END=0x20004000,0x20040000

def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def firmware_version(standalone):
    name='PICO_STANDALONE_VERSION' if standalone else 'PICO_LEGACY_VERSION'
    pattern=r'^#define '+name+r' "((?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*))"$'
    matches=re.findall(pattern,(ROOT/'include/picobrew/version.h').read_text(),re.MULTILINE)
    if len(matches)!=1:raise ValueError('Expected one numeric firmware version for '+name)
    return matches[0]

def inspect_image(elf_path,bin_path):
    raw=bin_path.read_bytes()
    with elf_path.open('rb') as stream:
        elf=ELFFile(stream)
        if elf['e_machine']!='EM_ARM' or elf.elfclass!=32 or not elf.little_endian:
            raise ValueError('Expected little-endian 32-bit ARM ELF')
        table=elf.get_section_by_name('.symtab')
        symbols={s.name:int(s['st_value']) for s in table.iter_symbols() if s['st_shndx']!='SHN_UNDEF'}
        undefined=[s.name for s in table.iter_symbols() if s.name and s['st_shndx']=='SHN_UNDEF']
        if undefined:raise ValueError('Undefined firmware symbols: '+repr(undefined))
        test_symbols=[name for name in symbols if 'fixture' in name or name=='pico_validation_entry' or name.startswith('sdk_validation_')]
        if test_symbols:raise ValueError('Test implementation in production link: '+repr(test_symbols))
        required=('ResetISR','main','setup','loop','pico_application_setup','pico_application_loop',
                  'pico_application_initialize','pico_application_bind_services','pico_recipe_run_step',
                  'pico_temperature_control_timer_isr','pico_heater_pump_timer_isr','g_pfnVectors','_heap','_eheap','_bss','_ebss')
        missing=[name for name in required if name not in symbols]
        if missing:raise ValueError('Missing appliance entry/dependency: '+repr(missing))
        segments=[]
        for seg in elf.iter_segments():
            if seg['p_type']!='PT_LOAD':continue
            start,size,filesize=int(seg['p_paddr']),int(seg['p_memsz']),int(seg['p_filesz'])
            if start<ORIGIN or start+size>END:raise ValueError('Firmware segment outside CC3200 application RAM')
            if int(seg['p_vaddr'])!=start:raise ValueError('Unexpected load/run relocation')
            segments.append({'start':hex(start),'memory_bytes':size,'file_bytes':filesize})
        sp,reset=struct.unpack_from('<II',raw)
        if symbols['g_pfnVectors']!=ORIGIN:raise ValueError('Wrong vector-table location')
        if not (ORIGIN<=sp<=END) or sp&7:raise ValueError('Invalid initial stack pointer')
        if not (symbols['_bss']<sp<=symbols['_ebss']):raise ValueError('Initial stack is not inside native SDK BSS stack storage')
        if not reset&1 or (reset&~1)!=(symbols['ResetISR']&~1):raise ValueError('Wrong reset-vector entry')
        if (elf['e_entry']&~1)!=(reset&~1):raise ValueError('ELF and raw reset entries differ')
        if symbols['_eheap']-symbols['_heap']!=32768:raise ValueError('Unexpected heap reservation')
        if symbols['_eheap']>END:raise ValueError('Heap exceeds physical RAM')
        loaded_end=max(int(s['start'],16)+s['file_bytes'] for s in segments if s['file_bytes'])
        if len(raw)!=loaded_end-ORIGIN:raise ValueError('Raw binary extent differs from loadable ELF data')
        return {'passed':True,'load_address':hex(ORIGIN),'ram_end':hex(END),
            'initial_stack_pointer':hex(sp),'reset_vector':hex(reset),'entry':hex(elf['e_entry']),
            'binary_bytes':len(raw),'bss_bytes':symbols['_ebss']-symbols['_bss'],
            'heap_bytes':symbols['_eheap']-symbols['_heap'],'sdk_stack_bytes':32768,
            'unallocated_ram_bytes':END-symbols['_eheap'],'load_segments':segments,
            'undefined_symbols':[],'test_symbols':[],
            'required_entry_symbols':{k:hex(symbols[k]) for k in required}}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--arm-prefix',default=os.environ.get('PICOBREW_ARM_PREFIX'))
    p.add_argument('--sdk-root',type=Path,default=Path(os.environ.get('PICOBREW_SDK',str(ROOT/'.deps/energia'))))
    p.add_argument('--output-dir',type=Path)
    p.add_argument('--standalone',action='store_true',help='Build the cloud-independent local API image')
    p.add_argument('--skip-archive-build',action='store_true')
    a=p.parse_args()
    out=(a.output_dir or ROOT/('build/standalone/firmware' if a.standalone else 'build/firmware')).resolve()
    out.mkdir(parents=True,exist_ok=True)
    core=out.parent/'core' if a.standalone else ROOT/'build'
    name='pico-s-pro-standalone' if a.standalone else 'pico-pro-recovered'
    common=['--sdk-root',str(a.sdk_root.resolve())]
    if a.arm_prefix:common+=['--arm-prefix',a.arm_prefix]
    if a.standalone:
        common+=['--standalone']
        subprocess.run([sys.executable,str(ROOT/'tools/embed_web.py')],check=True)
    if not a.skip_archive_build:
        subprocess.run([sys.executable,str(ROOT/'tools/build.py'),*common,
                        '--output-dir',str(core),'--archive-only'],check=True)
    log=out/'build.log'
    command=[sys.executable,str(ROOT/'tools/build_sdk_validation.py'),*common,
        '--entry',str(ROOT/'platform/firmware_main.cpp'),
        '--linker',str(ROOT/'platform/pico_cc3200.ld'),'--appliance-image','--output-dir',str(out/'native'),
        '--application-lib',str(core/'libpicobrew.a'),
        '--adapter-lib',str(core/'libpicobrew_energia.a')]
    result=subprocess.run(command,capture_output=True,text=True)
    log.write_text(result.stdout+result.stderr)
    if result.returncode:
        print(result.stdout+result.stderr);return result.returncode
    for suffix in ('.elf','.bin','.map'):
        shutil.copy2(out/'native'/('sdk_validation'+suffix),out/(name+suffix))
    elf,raw=out/(name+'.elf'),out/(name+'.bin')
    layout=inspect_image(elf,raw)
    if a.standalone:
        image=raw.read_bytes()
        forbidden=[b'picobrew.com',b'/API/pico/checkFirmware',b'/API/pico/getFirmware',b'/API/pico/register']
        found=[s.decode('ascii') for s in forbidden if s in image]
        if found:raise ValueError('Cloud protocol remains in standalone image: '+repr(found))
    update=out/(name+'.update.txt')
    body=encode_update(raw.read_bytes())
    if decode_update(body)!=raw.read_bytes():raise ValueError('Update envelope round trip failed')
    update.write_bytes(body)
    native=json.loads((out/'native/validation.json').read_text())
    compiled=json.loads((out/'native/compile_report.json').read_text())
    report={'artifact':('Pico S/Pro standalone local API firmware' if a.standalone else
                        'Complete recovered Pico Pro application with native Energia/TI startup and runtime'),
        'profile':'standalone' if a.standalone else 'legacy',
        'firmware_version':firmware_version(a.standalone),
        'reference_version':firmware_version(False),'source_reconstruction':True,'hardware_executed':False,
        'production_entry':'platform/firmware_main.cpp','application_composition':'src/application.c',
        'image_layout':layout,'elf_sha256':digest(elf),'binary_sha256':digest(raw),
        'update_body_sha256':digest(update),'update_body_bytes':len(body),
        'sdk_link_passed':native['pass'],'sdk_source_files':native['upstream_sources_compiled'],
        'app_sources_sha256':{str(f.relative_to(ROOT)):digest(f) for directory in ('src','include','platform')
                              for f in sorted((ROOT/directory).rglob('*')) if f.is_file()},
        'compiled_sources':[{'source':c['source'],'sha256':c['source_sha256']} for c in compiled],
        'runtime_boundary':native['runtime_boundary'],
        'validation_scope':'Static image, native ABI and complete link checks. Functional ARM comparisons are reported separately. Physical timing, peripheral operation and recovery/installation are not exercised.',
        'files':{'elf':elf.name,'binary':raw.name,'map':name+'.map','update_body':update.name}}
    if a.standalone:
        report['cloud_protocol_strings_absent']=True
        report['api_key_embedded']=False
        report['bounded_feature_heap_payload_bytes'] = 4704
        report['feature_heap_allocations'] = {
            'history_tail': {'maximum_payload_bytes': 3552, 'maximum_live_allocations': 1},
            'optional_telemetry_frame': {'maximum_payload_bytes': 1152, 'maximum_live_allocations': 1},
            'scope': 'Lazy buffers reused within the retained 32768-byte native heap; allocator overhead and other runtime allocations are additional. Allocation failure is reported without changing brewing controls.'}
        manifest=ROOT/'firmware/web/manifest.json'
        report['browser_manifest_sha256']=digest(manifest)
        report['browser_storage']=json.loads(manifest.read_text())
        report['web_sources_sha256']={str(f.relative_to(ROOT)):digest(f)
                                     for f in sorted((ROOT/'web').glob('*')) if f.is_file()}
    (out/'firmware-report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'image':str(raw),'firmware_version':report['firmware_version'],
                     'binary_bytes':layout['binary_bytes'],
                     'unallocated_ram_bytes':layout['unallocated_ram_bytes'],
                     'image_layout_passed':True,'hardware_executed':False},indent=2))
    return 0
if __name__=='__main__':raise SystemExit(main())
