#!/usr/bin/env python3
"""Compare relocatable control C with original ARM, including real original PID.

Only clocks, interrupt/timer registers, sensor readings, output/UI boundaries,
and the entry to the non-returning heater-fault handler are mocked.
"""
from pathlib import Path
import argparse
import ctypes as ct
import hashlib
import json
import math
import random
import struct
import subprocess
import sys
import tempfile

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]
PROJECT=HERE.parent if (HERE.parent/'tools/original_arm.py').exists() else ROOT/'picobrew-application'
sys.path.insert(0,str(PROJECT/'tools'))
from original_arm import ArmFirmware, IMAGE_SHA256
from rebuilt_arm import RebuiltArm

CONTROLLER=0x2002d2f8
SIGNAL_ADDRESSES=(0x2002dabe,0x2002bc54,0x2002bf10,0x2002bf11,0x2002bf12,0x2002bf13,0x2002bf20)
ENTRIES=(0x2000e088,0x2000d1c0,0x20005028,0x20005f50,0x20005004,0x20005120,0x20005080,0x2000e088,0x2000d1c0)
U8=ct.c_uint8
U32=ct.c_uint32

def bits32(x): return struct.unpack('<I',struct.pack('<f',x))[0]
def wr32(b,o,v): struct.pack_into('<I',b,o,int(v)&0xffffffff)
def wr64(b,o,v): struct.pack_into('<d',b,o,v)

def initial_controller():
    b=bytearray(0x228)
    b[0x60]=2; b[0x62]=0
    for i,v in enumerate((100.,101.,102.,103.,104.,105.)):wr64(b,0x78+i*8,v)
    wr32(b,0x1a8,600); wr32(b,0x1ac,3)
    for o,v in ((0x1b0,100.),(0x1b8,110.),(0x1d0,150.),(0x1d8,300.),(0x1e0,1500.),
                (0x1e8,2000.),(0x1f0,120.),(0x1f8,250.),(0x218,0.2),(0x220,0.2)):
        wr64(b,o,v)
    wr32(b,0x200,1000);wr32(b,0x204,80);wr32(b,0x208,1000);wr32(b,0x20c,500)
    for p,input_offset,output_offset,target_offset in ((0xb0,0x1f0,0x1e0,0x1d0),(0x128,0x1f8,0x1e8,0x1d8)):
        for o,v in ((0,2.),(8,0.5),(0x10,0.2),(0x18,2.),(0x20,0.05),(0x28,2.),
                    (0x48,50.),(0x50,100.),(0x60,0.),(0x68,3000.)):
            wr64(b,p+o,v)
        for o,v in ((0x34,CONTROLLER+input_offset),(0x38,CONTROLLER+output_offset),
                    (0x3c,CONTROLLER+target_offset),(0x40,1000),(0x58,100)):
            wr32(b,p+o,v)
        b[p+0x70]=1
    return b


class Original:
    def __init__(self,image):
        self.arm=ArmFirmware(image=image) if image else ArmFirmware()
        self.events=[]; self.clock_index=0; self.fatal=False
        hooks={0x20018efc:(1,0),0x20019ade:(2,0),0x20019ad4:(3,0),
               0x2001a590:(4,2),0x2001a59a:(5,2),0x200067a0:(6,0),0x20006680:(7,0),
               0x20005f40:(8,0),0x20005f48:(9,0),0x2000d0e0:(10,0),0x20005f22:(11,0),
               0x20005f12:(12,0),0x20005f1a:(13,0),0x20005284:(14,2),
               0x20009ef4:(15,2),0x20004494:(16,0),0x2000e054:(17,0),
               0x20019d64:(18,2),0x20019df4:(19,1),0x2001a4ec:(20,2),
               0x2001a514:(21,3),0x2001a528:(22,3),0x2001a588:(23,2),0x2001a4dc:(24,2)}
        for address,(tag,argc) in hooks.items():
            self.arm.hook(address,lambda arm,tag=tag,argc=argc:self.callback(arm,tag,argc))

    def callback(self,arm,tag,argc):
        values=[arm.reg(i) for i in range(argc)]+[0]*(3-argc)
        if tag==1:
            value=self.ticks[min(self.clock_index,len(self.ticks)-1)]
            self.clock_index+=1; values=[value,0,0]
        if tag==22:
            values[2]={0x2000e089:0,0x2000d1c1:1}.get(values[2],99)
        self.events.append((tag,*values))
        if tag==1:return value
        if tag==3:return self.enable_result
        if tag==4:return 0x35
        if tag==6:return self.wort
        if tag==7:return self.thermo
        if tag==17:
            self.fatal=True;arm.returned=True;arm.set_reg('lr',arm.RETURN|1);arm.uc.emu_stop()
        return None

    def run(self,operation,raw,signals,ticks,wort,thermo,enable_result,temperature):
        self.events=[];self.clock_index=0;self.fatal=False;self.ticks=ticks
        self.wort=wort;self.thermo=thermo;self.enable_result=enable_result
        self.arm.write(CONTROLLER,raw)
        for addr,value in zip(SIGNAL_ADDRESSES,signals):self.arm.write(addr,bytes([value]))
        args=[]
        if operation in (2,3,4):args=[CONTROLLER]
        elif operation==5:
            low,high=struct.unpack('<II',struct.pack('<d',temperature));args=[0xdeadbeef,0xa5a5a5a5,low,high]
        # Vary incoming registers for ISR calls: the apparent old r0/r1 inputs
        # are only saved scratch. A real fatal callee never returns to use them.
        elif operation in (0,1,7,8):args=[0x13579bdf,0x2468ace0,0x87654321,0xfedcba98]
        result=self.arm.call(ENTRIES[operation],args=args,max_instructions=150000)
        ret=-777 if self.fatal else result['r0'] if operation in (3,5,6) else 0
        return self.arm.read(CONTROLLER,0x228),bytes(self.arm.read(a,1)[0] for a in SIGNAL_ADDRESSES),ret,self.events,self.arm.instructions


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--source-root',type=Path,
                    default=HERE.parent if (HERE.parent/'src/control.c').exists() else None)
    ap.add_argument('--image',type=Path)
    ap.add_argument('--elf',type=Path,help='Compare a newly linked ARM fixture with the original image')
    ap.add_argument('--output',type=Path,default=HERE/'validation.json')
    opts=ap.parse_args()
    if opts.source_root:
        project=opts.source_root.resolve(); include=project/'include'
        control_source=project/'src/control.c';pid_source=project/'src/pid.c'
    else:
        include=HERE/'include';control_source=HERE/'control.c';pid_source=HERE.parent/'state_init/pid.c'
    fixture=HERE/'control_fixture.c' if (HERE/'control_fixture.c').exists() else HERE/'fixture.c'
    with tempfile.TemporaryDirectory(prefix='pico-control-') as temporary:
        so=Path(temporary)/'control.so'
        command=['gcc','-std=c11','-Wall','-Wextra','-Werror','-O2','-fPIC','-shared',
                 '-fno-fast-math','-ffp-contract=off','-I'+str(include),str(control_source),
                 str(pid_source),str(fixture),'-o',str(so)]
        rebuilt=RebuiltArm(opts.elf) if opts.elf else None
        if not rebuilt:
            subprocess.run(command,check=True)
            lib=ct.CDLL(str(so))
            lib.control_fixture_reset.argtypes=[ct.POINTER(U8),ct.POINTER(U8),ct.POINTER(U32),U32,U32,U32,U32]
            lib.control_fixture_run.argtypes=[ct.c_int,ct.c_double];lib.control_fixture_run.restype=ct.c_int
            lib.control_fixture_result.argtypes=[ct.POINTER(U8),ct.POINTER(U8)]
            lib.control_fixture_event_count.restype=U32;lib.control_fixture_events.restype=ct.POINTER(U32)
        original=Original(opts.image)
        records=[]

        def check(operation,description,changes=None,signals=(1,0,2,3,8,0,0),ticks=(1500,1501,1502),
                  wort=bits32(120.),thermo=bits32(250.),enable_result=1,temperature=120.):
            raw=initial_controller()
            for offset,value in (changes or {}).items():
                if isinstance(value,bytes):raw[offset:offset+len(value)]=value
                elif isinstance(value,float):wr64(raw,offset,value)
                else:wr32(raw,offset,value)
            sig=bytes(signals); ticks=tuple(x&0xffffffff for x in ticks)
            if rebuilt:
                data=rebuilt.SCRATCH
                rebuilt.write(data,raw);rebuilt.write(data+0x300,sig)
                rebuilt.write(data+0x400,struct.pack('<'+'I'*len(ticks),*ticks))
                rebuilt.call_symbol('control_fixture_reset',[data,data+0x300,data+0x400,len(ticks)],
                                    stack_args=[wort,thermo,enable_result])
                lo,hi=struct.unpack('<II',struct.pack('<d',temperature))
                c_ret=rebuilt.call_symbol('control_fixture_run',[operation,0,lo,hi])['r0']
                c_ret=ct.c_int32(c_ret).value
                rebuilt.call_symbol('control_fixture_result',[data+0x500,data+0x800])
                c_raw=rebuilt.read(data+0x500,0x228);c_sig=rebuilt.read(data+0x800,7)
                count=rebuilt.call_symbol('control_fixture_event_count')['r0']
                pointer=rebuilt.call_symbol('control_fixture_events')['r0']
                c_events=[struct.unpack('<IIII',rebuilt.read(pointer+i*16,16)) for i in range(count)]
            else:
                c_raw=(U8*len(raw)).from_buffer_copy(raw);c_sig=(U8*len(sig)).from_buffer_copy(sig)
                c_ticks=(U32*len(ticks))(*ticks)
                lib.control_fixture_reset(c_raw,c_sig,c_ticks,len(ticks),wort,thermo,enable_result)
                c_ret=lib.control_fixture_run(operation,temperature)
                lib.control_fixture_result(c_raw,c_sig)
                count=lib.control_fixture_event_count(); ptr=lib.control_fixture_events()
                c_events=[tuple(ptr[i*4+j] for j in range(4)) for i in range(count)]
            a_raw,a_sig,a_ret,a_events,instructions=original.run(operation,raw,sig,ticks,wort,thermo,enable_result,temperature)
            differences={}
            comparable_c=bytearray(c_raw)
            nan_representations=[]
            # Native x86 and the target's soft-float runtime can select different
            # NaN signs/payloads for arithmetic. Preserve exact comparisons for
            # ordinary values; explicitly report this narrower host contract.
            for offset in (() if rebuilt else (0xb0+0x48,0x128+0x48,0x1e0,0x1e8)):
                cv=struct.unpack_from('<d',comparable_c,offset)[0]
                av=struct.unpack_from('<d',a_raw,offset)[0]
                if math.isnan(cv) and math.isnan(av) and comparable_c[offset:offset+8]!=a_raw[offset:offset+8]:
                    nan_representations.append(hex(offset))
                    comparable_c[offset:offset+8]=a_raw[offset:offset+8]
            if bytes(comparable_c)!=a_raw:differences['memory']=[{'offset':hex(i),'c':c_raw[i],'arm':a_raw[i]} for i in range(len(raw)) if comparable_c[i]!=a_raw[i]][:25]
            if bytes(c_sig)!=a_sig:differences['signals']={'c':list(c_sig),'arm':list(a_sig)}
            if c_ret!=a_ret:differences['return']={'c':c_ret,'arm':a_ret}
            if c_events!=a_events:differences['events']={'c':c_events,'arm':a_events}
            if differences:
                report={'case':description,'operation':operation,'differences':differences}
                (HERE/'failure.json').write_text(json.dumps(report,indent=2)+'\n')
                raise AssertionError(json.dumps(report,indent=2))
            records.append({'case':description,'operation':operation,'original_instructions':instructions,
                            'events':count,'fatal_boundary':a_ret==-777,
                            'native_pid_nan_representation_differences':nan_representations})

        for op in (6,):
            for ret in (0,1):check(op,f'timer-init-enable-{ret}',enable_result=ret)
        for op in (0,1,7,8):
            for enabled in (0,1):
                for status in (0,1,3,7,255):
                    check(op,f'op{op}-enabled{enabled}-status{status}',{0x61:bytes([status])},signals=(enabled,0,2,3,8,0,0))
        for op in (2,3):
            for status in (0,1,255):
                for automatic in (0,1):
                    for ret in (0,1):
                        check(op,f'op{op}-status{status}-auto{automatic}-enable{ret}',
                              {0x61:bytes([status]),0xb0+0x70:bytes([automatic]),0x128+0x70:bytes([automatic])},enable_result=ret)
        for count in range(7):check(4,f'history-count-{count}',{0x1ac:count})
        for ticks in ((1000,),(1499,),(1500,),(1501,),(3999,),(4000,),(4001,),(9000,),
                      (0xfffffff0,),(15,)):
            for epoch in (1000,0xfffffff0):
                for output in (-1.,0.,1.,500.,3000.,float('inf'),float('nan')):
                    check(1,f'outputs-t{ticks[0]}-e{epoch}-out{output}',{0x200:epoch,0x208:epoch,0x1e8:output},ticks=ticks)
        for phase in (0,1,4,8,255):
            for seconds in (0,1,0x7fffffff,0x80000000,0xffffffff):
                check(0,f'step-counter-phase{phase}-seconds{seconds}',{0x62:bytes([phase]),0x1a8:seconds},signals=(0,0,0,0,0,0,0))
        for row,start,end,index in ((2,3,8,0),(2,3,8,4),(2,3,8,5),(255,255,0,255),(2,200,250,55)):
            check(0,f'progress-{row}-{start}-{end}-{index}',signals=(0,0,row,start,end,index,1))
        temperatures=(0.,31.999,32.,32.001,194.999,195.,195.001,214.999,215.,215.001,384.999,385.,385.001,float('inf'),-float('inf'),float('nan'))
        for enabled in (0,1):
            for value in temperatures:
                check(0,f'wort-boundary-{value}-enabled{enabled}',{0x1d0:196.},signals=(enabled,0,0,0,0,0,0),wort=bits32(value))
                check(0,f'thermo-boundary-{value}-enabled{enabled}',signals=(enabled,0,0,0,0,0,0),thermo=bits32(value))
        for target in (194.999,195.,195.001):
            check(0,f'target-limit-{target}',{0x1d0:target},wort=bits32(216.))
        for automatic in (0,1):
            for scale in (-1.,-0.,0.,0.1,0.2,1.,100.,float('inf'),float('nan')):
                for trend in (0,28,29,58,59,60,0x7fffffff,0xfffffffe,0xffffffff):
                    check(0,f'scale-{scale}-trend{trend}-auto{automatic}',
                          {0x218:scale,0x1a4:trend,0xb0+0x70:bytes([automatic]),0x128+0x70:bytes([automatic])})
        for phase in (0,1,2,4,8):
            for temperature in (100.,134.999,135.,135.001,150.,180.,180.001,float('nan')):
                for pending in (0,1):
                    check(5,f'classifier-phase{phase}-temperature{temperature}-pending{pending}',
                          {0x62:bytes([phase]),0x1ac:6,0x1b8:200.},signals=(1,pending,0,0,0,0,0),temperature=temperature)
        for pending in (0,1):
            for history in ((100.,)*6,(100.,99.,98.,97.,96.,95.),(100.,101.,102.,103.,104.,105.),
                            (1e20,-1e20,0.,1e20,-1e20,0.),(float('nan'),)*6):
                changes={0x1ac:6}
                changes.update({0x78+i*8:value for i,value in enumerate(history)})
                check(5,f'history-{history}-pending{pending}',changes,signals=(1,pending,0,0,0,0,0))
        rng=random.Random(354)
        for index in range(250):
            op=rng.choice((0,0,0,1,5))
            changes={0x61:bytes([rng.choice((0,0,0,1,3,7))]),0x62:bytes([rng.randrange(9)]),
                     0x1a4:rng.choice((29,59,rng.randrange(100))),0x1ac:rng.randrange(7),
                     0x1d0:rng.uniform(120.,220.),0x1e0:rng.uniform(0.,3000.),
                     0x1e8:rng.uniform(0.,3000.),0x200:rng.randrange(2**32),
                     0x208:rng.randrange(2**32),0x204:rng.randrange(501),0x218:rng.uniform(-0.5,1.)}
            check(op,f'random-{index}',changes,signals=(rng.randrange(2),rng.randrange(2),2,3,8,rng.randrange(5),rng.randrange(2)),
                  ticks=(rng.randrange(2**32),rng.randrange(2**32),rng.randrange(2**32)),
                  wort=bits32(rng.uniform(20.,240.)),thermo=bits32(rng.uniform(20.,400.)),temperature=rng.uniform(50.,250.))
        report={'passed':len(records),'failed':0,'firmware_sha256':IMAGE_SHA256,
                'candidate_engine':'newly built ARM ELF' if rebuilt else 'native host C',
                'elf_sha256':hashlib.sha256(opts.elf.read_bytes()).hexdigest() if rebuilt else None,
                'source_sha256':hashlib.sha256(control_source.read_bytes()).hexdigest(),
                'pid_source_sha256':hashlib.sha256(pid_source.read_bytes()).hexdigest(),
                'original_instructions':sum(x['original_instructions'] for x in records),
                'case_groups':dict((str(op),sum(x['operation']==op for x in records)) for op in range(9)),
                'native_pid_nan_representation_cases':sum(bool(x['native_pid_nan_representation_differences']) for x in records),
                'scope':'Compiled relocatable C including real C PID versus original ARM ISR/control/PID instructions. State bytes, signal values, meaningful return values, and ordered I/O events compared.',
                'limitations':'Hardware/UI boundary hooks; no interrupt concurrency. Native-host mode permits counted arithmetic NaN representation differences; rebuilt-ARM mode requires exact state bytes including NaNs. Fatal cases compare behavior up to entry into original non-returning fault handler. Shuttle-on boundary includes original filter interlock contract. Corrupted negative history indices excluded.',
                'cases':records}
        opts.output.parent.mkdir(parents=True,exist_ok=True);opts.output.write_text(json.dumps(report,indent=2)+'\n')
        print(json.dumps({k:v for k,v in report.items() if k!='cases'},indent=2))

if __name__=='__main__': main()
