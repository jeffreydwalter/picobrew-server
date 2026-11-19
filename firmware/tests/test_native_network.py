#!/usr/bin/env python3
"""Compare custom native SDK TCP connect paths with the original image."""
import argparse,hashlib,json,struct,sys
from pathlib import Path
WORK=Path(__file__).resolve().parents[1]
ROOT=WORK if (WORK/'tools/original_arm.py').is_file() else WORK.parents[1]/'picobrew-application'
sys.path.insert(0,str(ROOT/'tools'))
from original_arm import ArmFirmware
from rebuilt_arm import RebuiltArm

def run(machine,rebuilt,background,scenario):
    events=[];pending=list(scenario.get('connect',[0]));socket=scenario.get('socket',2);index=scenario.get('index',3)
    client=machine.SCRATCH+0x100;address=machine.SCRATCH+0x300
    machine.write(client,bytes(296));machine.write_u32(client+0x14,scenario.get('existing',255));machine.write(address,bytes(4)+bytes([192,168,4,1]))
    def hook(name,original,fn):
        if rebuilt:machine.hook_symbol(name,fn)
        else:machine.hook(original,fn)
    def get_socket(m):events.append(('get_socket',));return index
    def create(m):events.append(('socket',m.reg(0),m.reg(1),m.reg(2)));return socket
    def connect(m):events.append(('connect',m.reg(0),m.read(m.reg(1),m.reg(2)).hex()));return pending.pop(0)
    def close(m):events.append(('close',m.reg(0)));return 0
    def option(m):
        length=m.read_u32(m.reg('sp'));events.append(('option',m.reg(0),m.reg(1),m.reg(2),m.read(m.reg(3),length).hex()));return 0
    def select(m):
        timeout=m.read(m.read_u32(m.reg('sp')),8).hex();events.append(('select',m.reg(0),m.read_u32(m.reg(2)),timeout))
        m.write_u32(m.reg(2),(1<<socket)if scenario.get('isset',True)else 0);return scenario.get('select',1)
    hook('_ZN9WiFiClass9getSocketEv',0x20014ad4,get_socket);hook('sl_Socket',0x20017628,create)
    hook('sl_Connect',0x20017680,connect);hook('sl_Close',0x2001765c,close)
    hook('sl_SetSockOpt',0x2001792c,option);hook('sl_Select',0x20017978,select)
    if rebuilt:result=machine.call_symbol('_ZN14PicoHttpClient11connectPicoE9IPAddresstb',[client,address,80,background])['r0']
    else:result=machine.call(0x20015470 if background else 0x200153ac,[client,address,80])['r0']
    return result,machine.read_u32(client+0x14),events

def bridge_checks(elf):
    cases=0
    for write in [False,True]:
        for count,closed,opened in [(8,0,0),(4,0,0),(-1,0,0),(8,-1,0),(8,0,-1)]:
            m=RebuiltArm(elf);events=[];data=m.SCRATCH;payload=bytes.fromhex('01020304badccdab');m.write(data,payload)
            def fs_open(f):
                events.append(('open',f.cstring(f.reg(0)),f.reg(1),f.read_u32(f.reg(2))))
                f.write_u32(f.reg(3),47);return opened
            def transfer(f):
                events.append(('write'if write else'read',f.reg(0),f.reg(1),f.reg(3)))
                if not write:f.write(f.reg(2),payload)
                return count
            m.hook_symbol('sl_FsOpen',fs_open);m.hook_symbol('sl_FsWrite'if write else'sl_FsRead',transfer)
            m.hook_symbol('sl_FsClose',lambda f:events.append(('close',f.reg(0),f.reg(1),f.reg(2),f.reg(3)))or closed)
            result=m.call_symbol('_ZL10boot_writePvPKh'if write else'_ZL9boot_readPvPh',[0,data])['r0']
            assert result==(0 if count==8 and closed==0 and opened==0 else 0xffffffff)
            assert events[0]==('open',b'/sys/mcubootinfo.bin',0x1000 if write else 0,0)
            if opened==0:assert events[1:]==[('write'if write else'read',47,0,8),('close',47,0,0,0)]
            else:assert len(events)==1
            cases+=1
    for opened,info,closed in [(0,0,0),(-1,0,0),(0,-1,0),(0,0,-1)]:
        m=RebuiltArm(elf);m.call_symbol('_GLOBAL__sub_I_SerFlash');events=[];name=m.SCRATCH;m.write(name,b'test.txt\0')
        def fs_open(f):assert f.reg(2)==0;f.write_u32(f.reg(3),47);return opened
        def get_info(f):f.write(f.reg(2),struct.pack('<HH6I',0,0,4,256,0,0,0,0));return info
        m.hook_symbol('sl_FsOpen',fs_open);m.hook_symbol('sl_FsGetInfo',get_info);m.hook_symbol('sl_FsClose',lambda f:closed)
        result=m.call_symbol('_ZL9file_openPvPKcm',[0,name,0])['r0'];assert result==int(opened==0 and info==0)
        if result:assert m.call_symbol('_ZL10file_closePv',[0])['r0']==int(closed==0)
        cases+=1
    for ready,process in [(True,True),(True,False),(False,True)]:
        m=RebuiltArm(elf);events=[];clock=[0];m.write(m.address('_ZL10hash_ready'),bytes([ready]));data=m.SCRATCH;output=data+0x100;m.write(data,b'checksum')
        m.hook_symbol('PRCMPeripheralClkEnable',lambda f:events.append(('clock',f.reg(0),f.reg(1)))or 0)
        m.hook_symbol('SHAMD5IntRegister',lambda f:events.append(('register',f.reg(0),bool(f.reg(1))))or 0)
        m.hook_symbol('SHAMD5IntEnable',lambda f:events.append(('enable',f.reg(0),f.reg(1)))or 0)
        m.hook_symbol('SHAMD5ConfigSet',lambda f:events.append(('mode',f.reg(0),f.reg(1)))or 0)
        m.hook_symbol('SHAMD5IntDisable',lambda f:events.append(('disable',f.reg(0),f.reg(1)))or 0)
        def now(f):value=clock[0];clock[0]+=5000;return value
        def digest(f):
            value=f.read(f.reg(1),f.reg(2));events.append(('process',f.reg(0),value));f.write(f.reg(3),hashlib.md5(value).digest());return int(process)
        m.hook_symbol('millis',now);m.hook_symbol('SHAMD5DataProcess',digest)
        result=m.call_symbol('_ZL3md5PvPKhjPhm',[0,data,8,output],stack_args=[0])['r0']
        assert result==int(ready and process)
        assert events[:3]==[('clock',0x12,1),('register',0x44035000,True),('enable',0x44035000,15)]
        if ready:
            assert events[3:]==[('mode',0x44035000,0x18),('process',0x44035000,b'checksum'),('disable',0x44035000,15)]
            assert m.read(output,16)==hashlib.md5(b'checksum').digest()
        else:assert len(events)==3
        cases+=1
    return cases

def main():
    p=argparse.ArgumentParser();p.add_argument('--elf',type=Path,default=ROOT/'build/firmware/pico-pro-recovered.elf');p.add_argument('--output',type=Path,default=WORK/'analysis/native_socket_validation.json');a=p.parse_args()
    cases=0
    common=[{},dict(existing=0),dict(index=255),dict(socket=-1),dict(connect=[-1])]
    extra=[dict(connect=[-114,0]),dict(connect=[-114,-1]),dict(connect=[-114],isset=False),dict(connect=[-114],select=0),dict(connect=[-114],select=-1)]
    for background in [False,True]:
        for scenario in common+(extra if background else[]):
            original=run(ArmFirmware(),False,background,scenario);rebuilt=run(RebuiltArm(a.elf),True,background,scenario)
            assert original==rebuilt,(background,scenario,original,rebuilt);cases+=1
    bridge=bridge_checks(a.elf)
    report=dict(passed=True,cases=cases+bridge,checks={'socket_original_differential':cases,'native_filesystem_and_md5_boundaries':bridge},elf=str(a.elf),elf_sha256=hashlib.sha256(a.elf.read_bytes()).hexdigest(),scope='Real production native SDK object methods and bridge; original TCP differential plus controlled SimpleLink filesystem/hardware-MD5 scenarios; no device activity.')
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
