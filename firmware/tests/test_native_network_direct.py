#!/usr/bin/env python3
"""Direct original/native ARM checks for remaining network platform mappings.

The actual production ELF runs the bridge and, for files, native SLFS methods.
Only hardware hash-register operations, WiFi initialization and SimpleLink
filesystem/ping requests are controlled. Adapted SDK return contracts and
repaired failure behavior are recorded separately from exact comparisons.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools'))
from original_arm import ArmFirmware
from rebuilt_arm import RebuiltArm
from test_native_startup import Layout

ENTRIES = {
    'hash_interrupt': ('0x200047b0', '_ZL14hash_interruptv'),
    'initialize': ('0x20008994', '_ZL10initializePv'),
    'file_remove': ('0x200058c8', '_ZL11file_removePvPKc'),
    'file_close': ('0x20008f40', '_ZL10file_closePv'),
    'ping': ('0x20008c28', '_ZL4pingPv'),
}
ORIGINAL_FLASH = 0x2002e58c


def u32(value):
    return value & 0xffffffff


def signed(value):
    return value if value < 0x80000000 else value-0x100000000


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--elf', type=Path, default=ROOT/'build/firmware/pico-pro-recovered.elf')
    parser.add_argument('--output', type=Path, default=ROOT/'analysis/native-network-direct-tests.json')
    args = parser.parse_args()
    layout = Layout(args.elf)
    results = []

    def record(entry, scenario, kind, **evidence):
        results.append({'entry':entry,'original_address':ENTRIES[entry][0],
                        'replacement_symbol':ENTRIES[entry][1], 'scenario':scenario,
                        'kind':kind, **evidence})

    # All status-mask combinations and both incoming ready-latch values.
    for status in range(16):
        for initial_ready in (0,1):
            outcomes=[]
            for native,machine in [(False,ArmFirmware()),(True,RebuiltArm(args.elf))]:
                events=[]
                flag=machine.address('_ZL10hash_ready') if native else 0x2002d520
                machine.write(flag,bytes([initial_ready]))
                def get_status(f):
                    events.append(('status',f.reg(0),f.reg(1)));return status
                def disable(f):
                    events.append(('disable',f.reg(0),f.reg(1)))
                if native:
                    machine.hook_symbol('SHAMD5IntStatus',get_status)
                    machine.hook_symbol('SHAMD5IntDisable',disable)
                    machine.call_symbol(ENTRIES['hash_interrupt'][1])
                else:
                    machine.hook(0x20019f74,get_status);machine.hook(0x20019fd0,disable)
                    machine.call(int(ENTRIES['hash_interrupt'][0],16))
                outcomes.append((events,machine.read(flag,1)[0]))
            expected=[('status',0x44035000,1)]+[('disable',0x44035000,bit) for bit in (8,4,2,1) if status&bit]
            assert outcomes[0]==outcomes[1]==(expected,int(bool(initial_ready or status&8)))
            record('hash_interrupt',{'status_mask':status,'initial_ready':initial_ready},
                   'original_vs_rebuilt_arm',interrupt_operations=expected,ready=outcomes[0][1])

    # Execute each actual wrapper; the WiFi SDK initializer is the observed edge.
    for initialized in (False,True):
        outcomes=[]
        for native,machine in [(False,ArmFirmware()),(True,RebuiltArm(args.elf))]:
            events=[]
            def init(f):events.append('WiFi.init');return int(initialized)
            if native:
                machine.hook_symbol('_ZN9WiFiClass4initEv',init)
                value=machine.call_symbol(ENTRIES['initialize'][1],[0])['r0']
            else:
                machine.hook(0x20014a70,init)
                value=machine.call(int(ENTRIES['initialize'][0],16))['r0']
            outcomes.append((value,events))
        assert outcomes[0]==outcomes[1]==(int(initialized),['WiFi.init'])
        record('initialize',{'sdk_result':initialized},'original_vs_rebuilt_arm',
               return_value=outcomes[0][0],calls=outcomes[0][1])

    # Private SLFS object fields come from actual compiler DWARF for the native
    # image. Original offsets are separately established from the original code.
    original_fields={'filehandle':0x10,'filesize':0x14,'offset':0x18,'retval':0x1c,'is_write':0x20}
    native_fields={name:layout.offset('SLFS',name) for name in original_fields}
    def file_machine(native,handle):
        machine=RebuiltArm(args.elf) if native else ArmFirmware()
        if native:
            machine.call_symbol('_GLOBAL__sub_I_SerFlash')
            address=machine.address('SerFlash');fields=native_fields
        else:
            address=ORIGINAL_FLASH;fields=original_fields
        for field,value in [('filehandle',handle),('filesize',123),('offset',17),('retval',23)]:
            machine.write_u32(address+fields[field],value)
        machine.write(address+fields['is_write'],b'\x01')
        return machine,address,fields

    for filename in (b'/sys/ssid.txt',b'/sys/restart.txt',b'user-file.txt'):
        for handle,status in [(0,0),(0,-5),(47,0)]:
            outcomes=[]
            for native in (False,True):
                machine,address,fields=file_machine(native,handle);events=[]
                machine.write(machine.SCRATCH,filename+b'\0')
                def remove(f):
                    events.append(('delete',f.cstring(f.reg(0)).decode(),f.reg(1)))
                    return u32(status)
                if native:
                    machine.hook_symbol('sl_FsDel',remove)
                    value=machine.call_symbol(ENTRIES['file_remove'][1],[0,machine.SCRATCH])['r0']
                else:
                    machine.hook(0x20016f6c,remove)
                    value=machine.call(int(ENTRIES['file_remove'][0],16),[machine.SCRATCH])['r0']
                assert machine.read_u32(address+fields['filehandle'])==handle
                outcomes.append((value,events))
            assert outcomes[0][1]==outcomes[1][1]==([] if handle else [('delete',filename.decode(),0)])
            assert outcomes[0][0]==1
            expected=-10004 if handle else status
            assert signed(outcomes[1][0])==expected
            record('file_remove',{'filename':filename.decode(),'open_handle':handle,'simplelink_result':status},
                   'sdk_return_contract',calls=outcomes[0][1],original_return=1,native_return=expected,
                   distinction='Original wrapper always returns 1; native callback preserves SDK 0-success/negative-error status. File operation and filename match.')

    for handle,status in [(0,0),(47,0),(47,-5),(47,-42)]:
        outcomes=[]
        for native in (False,True):
            machine,address,fields=file_machine(native,handle);events=[]
            def close(f):
                events.append(('close',f.reg(0),f.reg(1),f.reg(2),f.reg(3)))
                return u32(status)
            if native:
                machine.hook_symbol('sl_FsClose',close)
                value=machine.call_symbol(ENTRIES['file_close'][1],[0])['r0']
            else:
                machine.hook(0x20016d98,close)
                value=machine.call(int(ENTRIES['file_close'][0],16))['r0']
            state={field:machine.read_u32(address+fields[field]) for field in ('filehandle','filesize','offset')}
            state['is_write']=machine.read(address+fields['is_write'],1)[0]
            outcomes.append((value,events,state))
        assert outcomes[0][1]==outcomes[1][1]==([] if not handle else [('close',47,0,0,0)])
        assert outcomes[0][2]==outcomes[1][2]
        assert outcomes[0][0]==int(bool(handle))
        assert outcomes[1][0]==int(bool(handle) and status==0)
        record('file_close',{'open_handle':handle,'simplelink_result':status},
               'documented_failure_repair' if handle and status else 'normalized_sdk_contract',
               calls=outcomes[0][1],state=outcomes[0][2],original_return=outcomes[0][0],native_return=outcomes[1][0],
               distinction='The legacy close method returns 1 on success, but its wrapper also treats negative failures as true. Native SLFS uses 0 success and the adapter returns false on errors.')

    def run_ping(native,received,status,write_report=True):
        machine=RebuiltArm(args.elf) if native else ArmFirmware();events=[]
        # Make the original's uninitialized result reproducible when the driver
        # returns an error without writing the report; native code zeros it.
        machine.write(machine.STACK-0x200,bytes([0xa5])*0x200)
        def ping(f):
            command=struct.unpack('<IHHIII',machine.read(f.reg(0),20))
            events.append(('ping',command,f.reg(1),f.reg(3)))
            if native:assert machine.read(f.reg(0)+20,12)==bytes(12)
            if write_report:machine.write_u32(f.reg(2)+4,received)
            return u32(status)
        if native:
            machine.hook_symbol('sl_NetAppPingStart',ping)
            value=machine.call_symbol(ENTRIES['ping'][1],[0])['r0']
        else:
            machine.hook(0x20017254,ping)
            value=machine.call(int(ENTRIES['ping'][0],16))['r0']
        return value,events

    for received in (0,1,5,255,256,0x12345678):
        old=run_ping(False,received,0);new=run_ping(True,received,0)
        assert old==new==((received&255),[('ping',(25,56,1000,5,0,0x08080404),2,0)])
        record('ping',{'packets_received':received,'status':0},'original_vs_rebuilt_arm',return_value=old[0],calls=old[1])
    for status,write_report in [(-1,False),(-42,False),(-1,True)]:
        old=run_ping(False,7,status,write_report);new=run_ping(True,7,status,write_report)
        assert old[1]==new[1]
        assert old[0]==(7 if write_report else 0xa5)
        assert new[0]==0
        record('ping',{'packets_received':7 if write_report else None,'status':status},
               'documented_failure_repair',original_return=old[0],native_return=new[0],calls=old[1],
               distinction='Native callback returns 0 for driver errors and initializes the report; original ignores status and may return an unwritten stack byte.')

    kinds={kind:sum(r['kind']==kind for r in results) for kind in sorted({r['kind'] for r in results})}
    report={'passed':True,'cases':len(results),'passed_cases':len(results),'failed_cases':0,
            'elf_sha256':hashlib.sha256(args.elf.read_bytes()).hexdigest(),
            'production_elf_sha256':hashlib.sha256(args.elf.read_bytes()).hexdigest(),
            'original_image_sha256':hashlib.sha256((ROOT/'reference/original-0.1.34.bin').read_bytes()).hexdigest(),
            'test_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
            'checks':kinds,'entry_evidence':[{'original_address':ENTRIES[name][0],
                'replacement_symbol':ENTRIES[name][1],'direct_cases':sum(r['entry']==name for r in results)} for name in ENTRIES],
            'results':results,
            'scope':'Actual original entry and native production bridge execution. File scenarios execute both legacy and native SLFS methods with controlled SimpleLink requests. Hash-register operations, WiFi init and ping driver calls are explicit boundaries. SDK return adaptation and repaired failures are not claimed as identical original behavior. No device activity.'}
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({key:report[key] for key in ('passed','cases','checks','entry_evidence')},indent=2))


if __name__=='__main__':
    main()
