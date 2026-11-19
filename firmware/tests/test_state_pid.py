#!/usr/bin/env python3
"""Check portable application state and Pico PID against original ARM code.

Original SDK constructors/destructor registration are controlled boundaries.
The original global initializer, PID, telemetry clear, menu clear, and numeric
helper instructions execute. Host structures are serialized into the original
32-bit layout explicitly; production code contains none of these addresses.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
WORKSPACE = HERE.parents[1]
PROJECT = HERE.parent if (HERE.parent / 'tools/original_arm.py').exists() else WORKSPACE / 'picobrew-application'
sys.path.insert(0, str(PROJECT / 'tools'))
from original_arm import ArmFirmware, IMAGE_SHA256, words_from_double

RAM_START = 0x2002bc00
RAM_LENGTH = 0x2000
CONTROLLER = 0x2002d2f8
PID_ADDRESS = 0x20081000
IO_ADDRESS = 0x20082000
CALLBACKS = [0x20009ac9, 0x20009bc1, 0x20009c25, 0x20008969, 0x20009be5]

PID_FIELDS = [
    ('displayedProportionalGain',0x00,8), ('displayedIntegralGain',0x08,8),
    ('displayedDerivativeGain',0x10,8), ('proportionalGain',0x18,8),
    ('integralGain',0x20,8), ('derivativeGain',0x28,8), ('direction',0x30,4),
    ('lastUpdateMs',0x40,4), ('integralTerm',0x48,8), ('previousInput',0x50,8),
    ('sampleIntervalMs',0x58,4), ('minimumOutput',0x60,8), ('maximumOutput',0x68,8),
    ('automatic',0x70,1),
]

CONTROLLER_FIELDS = [
    ('still',0x00,0x60), ('compartment',0x60,1), ('status',0x61,1),
    ('phase',0x62,1), ('phase_label',0x63,0x15), ('wort_history',0x78,0x30),
    ('drain_subphase',0xa8,1), ('reserved_0a9',0xa9,7),
    ('shuttle_calibration_complete',0x1a0,1), ('calibration_flag_1a1',0x1a1,1),
    ('reserved_1a2',0x1a2,2), ('trend_seconds',0x1a4,4), ('step_seconds',0x1a8,4),
    ('history_count',0x1ac,4), ('previous_window_wort_fahrenheit',0x1b0,8),
    ('highest_wort_fahrenheit',0x1b8,8), ('reserved_1c0',0x1c0,16),
    ('wort_target_fahrenheit',0x1d0,8), ('thermoblock_target_fahrenheit',0x1d8,8),
    ('wort_pid_output',0x1e0,8), ('thermoblock_pid_output',0x1e8,8),
    ('wort_fahrenheit',0x1f0,8), ('thermoblock_fahrenheit',0x1f8,8),
    ('heater_cycle_started_ms',0x200,4), ('shuttle_on_time_ms',0x204,4),
    ('shuttle_cycle_started_ms',0x208,4), ('heater_on_time_ms',0x20c,4),
    ('drain_phase_started_ms',0x210,4), ('reserved_214',0x214,4),
    ('active_shuttle_scale',0x218,8), ('candidate_shuttle_scale',0x220,8),
]

APP_FIELDS = [
    ('menu',0x2002bc74,92), ('recipe.identifier',0x2002bcdc,21),
    ('recipe.secondary_identifier',0x2002bcf1,21), ('recipe.reserved_2e',0x2002bd06,2),
    ('recipe.steps',0x2002bd08,480), ('recipe.step_count',0x2002bee8,1),
    ('recipe.reserved_211',0x2002bee9,3), ('recipe.integer_metric',0x2002beec,4),
    ('recipe.decimal_metric',0x2002bef0,8), ('recipe.abv_adjustment',0x2002bef8,4),
    ('recipe.ibu_adjustment',0x2002befc,4), ('recipe.abv_adjustment_reference',0x2002bf00,4),
    ('recipe.current_step',0x2002bf04,4), ('recipe.ibu_adjustment_reference',0x2002bf08,4),
    ('control_signals.progress_row',0x2002bf10,1),
    ('control_signals.progress_start_column',0x2002bf11,1),
    ('control_signals.progress_end_column',0x2002bf12,1),
    ('control_signals.progress_glyph_index',0x2002bf13,1),
    ('progress_unknown_04',0x2002bf14,1), ('progress_unknown_08',0x2002bf18,4),
    ('progress_unknown_0c',0x2002bf1c,4), ('control_signals.progress_active',0x2002bf20,1),
    ('screen',0x2002d524,76), ('carousel.page_index',0x2002d9d0,4),
    ('carousel.page_count',0x2002d9d4,4), ('carousel.last_update_ms',0x2002d9d8,4),
    ('carousel.interval_ms',0x2002d9dc,4), ('device_state',0x2002da9c,1),
    ('device_uid',0x2002da9d,33), ('buzzer_enabled',0x2002dac0,1),
    ('stepper_capabilities',0x2002dac4,4),
]

def native_wrapper():
    source = r'''
#include "picobrew/state.h"
#include <string.h>
#include <stdint.h>
static PicoAppState state;
static uint8_t initial[8192];
static uint8_t snapshots[8][8192];
static uint32_t events[8][5], event_count, clocks[2], clock_index;
static double pid_io[3];
static PicoPid lone_pid;
static uint8_t lone_initial[0x78];
static void page0(void){} static void page1(void){} static void page2(void){}
static void page3(void){} static void page4(void){}
static PicoPageCallback pages[5]={page0,page1,page2,page3,page4};
static uint32_t callbacks[5]={0x20009ac9,0x20009bc1,0x20009c25,0x20008969,0x20009be5};
static uint32_t get32(const uint8_t *p){uint32_t v;memcpy(&v,p,4);return v;}
static void put32(uint8_t *p,uint32_t v){memcpy(p,&v,4);}
static uint32_t pointer_id(const volatile void *p){
    if(p==&state.controller.wort_fahrenheit)return 0x2002d4e8;
    if(p==&state.controller.thermoblock_fahrenheit)return 0x2002d4f0;
    if(p==&state.controller.wort_pid_output)return 0x2002d4d8;
    if(p==&state.controller.thermoblock_pid_output)return 0x2002d4e0;
    if(p==&state.controller.wort_target_fahrenheit)return 0x2002d4c8;
    if(p==&state.controller.thermoblock_target_fahrenheit)return 0x2002d4d0;
    if(p==&state.session)return 0x2002c108;
    for(unsigned i=0;i<3;++i)if(p==&pid_io[i])return 0x20082000+8*i;
    return (uint32_t)(uintptr_t)p;
}
static void load_pid(PicoPid *pid,const uint8_t *wire){
'''
    for name, offset, size in PID_FIELDS:
        source += f' memcpy(&pid->{name},wire+{offset},{size});\n'
    source += '''
 pid->input=(volatile double *)(uintptr_t)get32(wire+0x34);
 pid->output=(double *)(uintptr_t)get32(wire+0x38);
 pid->setpoint=(double *)(uintptr_t)get32(wire+0x3c);
}
static void save_pid(const PicoPid *pid,uint8_t *wire){
'''
    for name, offset, size in PID_FIELDS:
        source += f' memcpy(wire+{offset},&pid->{name},{size});\n'
    source += '''
 put32(wire+0x34,pointer_id(pid->input));put32(wire+0x38,pointer_id(pid->output));
 put32(wire+0x3c,pointer_id(pid->setpoint));
}
static void load_state(const uint8_t *wire){
 memset(&state,0,sizeof state); memcpy(initial,wire,8192);
'''
    for name, offset, size in CONTROLLER_FIELDS:
        source += f' memcpy((void *)&state.controller.{name},wire+{CONTROLLER-RAM_START+offset},{size});\n'
    for name, address, size in APP_FIELDS:
        source += f' memcpy((void *)&state.{name},wire+{address-RAM_START},{size});\n'
    source += f'''
 load_pid(&state.controller.wort_pid,wire+{CONTROLLER-RAM_START+0xb0});
 load_pid(&state.controller.thermoblock_pid,wire+{CONTROLLER-RAM_START+0x128});
 state.recipe.program=(PicoProgramContext *)(uintptr_t)get32(wire+0xd8);
 for(unsigned i=0;i<15;++i)state.carousel.pages[i]=(PicoPageCallback)(uintptr_t)get32(wire+0x1de0+4*i);
}}
static void save_state(uint8_t *wire){{
 memcpy(wire,initial,8192);
'''
    for name, offset, size in CONTROLLER_FIELDS:
        source += f' memcpy(wire+{CONTROLLER-RAM_START+offset},(const void *)&state.controller.{name},{size});\n'
    for name, address, size in APP_FIELDS:
        source += f' memcpy(wire+{address-RAM_START},(const void *)&state.{name},{size});\n'
    source += f'''
 save_pid(&state.controller.wort_pid,wire+{CONTROLLER-RAM_START+0xb0});
 save_pid(&state.controller.thermoblock_pid,wire+{CONTROLLER-RAM_START+0x128});
 put32(wire+0xd8,pointer_id(state.recipe.program));
 for(unsigned i=0;i<15;++i){{
  uint32_t v=(uint32_t)(uintptr_t)state.carousel.pages[i];
  for(unsigned j=0;j<5;++j)if(state.carousel.pages[i]==pages[j])v=callbacks[j];
  put32(wire+0x1de0+4*i,v);
 }}
}}
static void event(uint32_t id,uint32_t a,uint32_t b,uint32_t c,uint32_t d){{
 unsigned i=event_count++;if(i>=8)return;events[i][0]=id;events[i][1]=a;events[i][2]=b;events[i][3]=c;events[i][4]=d;
 save_state(snapshots[i]);
}}
static void zero_ip(void *c,void *o){{(void)c;event(1,(uint32_t)(uintptr_t)o,0,0,0);}}
static void http(void *c,void *o){{(void)c;event(2,(uint32_t)(uintptr_t)o,0,0,0);}}
static void dtor(void *c,void *o){{(void)c;event(3,(uint32_t)(uintptr_t)o,0,0,0);}}
static void rfid(void *c,void *o,uint8_t a,uint8_t b,uint8_t d){{(void)c;event(4,(uint32_t)(uintptr_t)o,a,b,d);}}
static void ip(void *c,void *o,uint8_t a,uint8_t b,uint8_t d,uint8_t e){{(void)c;event(5,(uint32_t)(uintptr_t)o,a,b,((uint32_t)d<<8)|e);}}
static uint32_t clock_source(void *c){{(void)c;event(6,0,0,0,0);return clocks[clock_index<2?clock_index++:1];}}
void test_init(const uint8_t *wire,uint32_t t0,uint32_t t1){{
 load_state(wire);event_count=clock_index=0;clocks[0]=t0;clocks[1]=t1;
 PicoPlatformObjects objects={{(void *)0x2002d580,(void *)0x2002d58c,(void *)0x2002c168,(void *)0x2002d570,(void *)0x2002dac8}};
 PicoInitializationOps ops={{zero_ip,http,dtor,rfid,ip,clock_source}};
 pico_application_initialize(&state,&objects,&ops,0,pages);
}}
void test_state(uint8_t *out){{save_state(out);}}
uint32_t test_event_count(void){{return event_count;}}
void test_event(uint32_t n,uint32_t *out,uint8_t *memory){{memcpy(out,events[n],20);memcpy(memory,snapshots[n],8192);}}
void test_pid(unsigned op,const uint8_t *wire,double input,double output,double setpoint,
              double a,double b,double c,int mode,uint32_t now,uint8_t *out,double *io){{
 memcpy(lone_initial,wire,0x78);load_pid(&lone_pid,wire);
 pid_io[0]=input;pid_io[1]=output;pid_io[2]=setpoint;
 lone_pid.input=&pid_io[0];lone_pid.output=&pid_io[1];lone_pid.setpoint=&pid_io[2];
 switch(op){{
 case 0:pico_pid_construct(&lone_pid,&pid_io[0],&pid_io[1],&pid_io[2],a,b,c,mode,3000.0,now);break;
 case 1:pico_pid_compute(&lone_pid,now);break;
 case 2:pico_pid_set_tunings(&lone_pid,a,b,c);break;
 case 3:pico_pid_set_output_limits(&lone_pid,a,b);break;
 case 4:pico_pid_set_mode(&lone_pid,mode);break;
 case 5:pico_pid_set_direction(&lone_pid,mode);break;
 case 6:pico_pid_initialize_integral(&lone_pid);break;
 case 7:pico_pid_disable(&lone_pid);break;
 }}
 memcpy(out,lone_initial,0x78);save_pid(&lone_pid,out);memcpy(io,pid_io,24);
}}
/* Fixed-layout packet keeps the test bridge independent of host/ARM argument
 * register allocation. Doubles are copied to aligned C locals before use. */
void test_pid_packet(const uint8_t *request,uint8_t *result){{
 double values[6],io[3];
 memcpy(values,request+124,48);
 test_pid(get32(request),request+4,values[0],values[1],values[2],
          values[3],values[4],values[5],(int32_t)get32(request+172),
          get32(request+176),result,io);
 memcpy(result+120,io,24);
}}
'''
    return source.replace('test_init(', 'pico_state_test_init(').replace('test_state(', 'pico_state_test_state(').replace('test_event_count(', 'pico_state_test_event_count(').replace('test_event(', 'pico_state_test_event(').replace('test_pid(', 'pico_state_test_pid(').replace('test_pid_packet(', 'pico_state_test_pid_packet(')


class NativeBackend:
    def __init__(self, lib):
        self.lib=lib
        bytep=ctypes.POINTER(ctypes.c_uint8)
        lib.pico_state_test_init.argtypes=[bytep,ctypes.c_uint32,ctypes.c_uint32]
        lib.pico_state_test_state.argtypes=[bytep]
        lib.pico_state_test_event_count.restype=ctypes.c_uint32
        lib.pico_state_test_event.argtypes=[ctypes.c_uint32,ctypes.POINTER(ctypes.c_uint32),bytep]
        lib.pico_state_test_pid.argtypes=[ctypes.c_uint,bytep,*([ctypes.c_double]*6),ctypes.c_int,ctypes.c_uint32,bytep,ctypes.POINTER(ctypes.c_double)]
    def __getattr__(self,name):return getattr(self.lib,'pico_state_'+name)


class RebuiltBackend:
    def __init__(self,elf):
        from rebuilt_arm import RebuiltArm
        self.arm=RebuiltArm(elf)
        self.instructions=0
    def call(self,name,args):
        result=self.arm.call_symbol('pico_state_'+name,args=args,max_instructions=1000000)
        self.instructions+=result['instructions']
        return result['r0']
    def test_init(self,wire,t0,t1):
        self.arm.write(0x20080000,bytes(wire));self.call('test_init',[0x20080000,t0,t1])
    def test_state(self,out):
        self.call('test_state',[0x20080000]);ctypes.memmove(out,self.arm.read(0x20080000,RAM_LENGTH),RAM_LENGTH)
    def test_event_count(self):return self.call('test_event_count',[])
    def test_event(self,n,args,memory):
        self.call('test_event',[n,0x20080000,0x20080100]);ctypes.memmove(args,self.arm.read(0x20080000,20),20)
        ctypes.memmove(memory,self.arm.read(0x20080100,RAM_LENGTH),RAM_LENGTH)
    def test_pid(self,op,wire,input_value,output_value,target,a,b,c,mode,now,result,io):
        request=struct.pack('<I',op)+bytes(wire)+struct.pack('<ddddddiI',input_value,output_value,target,a,b,c,mode,now)
        assert len(request)==180
        self.arm.write(0x20080000,request);self.call('test_pid_packet',[0x20080000,0x20080200])
        data=self.arm.read(0x20080200,144);ctypes.memmove(result,data[:120],120);ctypes.memmove(io,data[120:],24)

def first_difference(left, right):
    return next((i for i,(a,b) in enumerate(zip(left,right)) if a!=b), None)

def check_initializers(lib, image):
    rng = random.Random(0x90f8)
    cases=[]
    for fill in ('bss-zero','patterned-reentry'):
        for t0,t1 in [(0,0),(99,100),(100,101),(0xfffffff0,5),(0xffffffff,0),(1000,2000)]:
            memory=bytearray(RAM_LENGTH) if fill=='bss-zero' else bytearray(rng.randbytes(RAM_LENGTH))
            # The constructor explicitly disables both PIDs before use. Keep their
            # incoming C bool representation valid even in the reentry fixture.
            memory[CONTROLLER-RAM_START+0xb0+0x70]=int(fill=='patterned-reentry')
            memory[CONTROLLER-RAM_START+0x128+0x70]=int(fill=='patterned-reentry')
            firmware=ArmFirmware(image=image)
            firmware.write(RAM_START,memory)
            observed=[]; snapshots=[]; ticks=iter([t0,t1])
            def record(event):
                observed.append(event); snapshots.append(firmware.read(RAM_START,RAM_LENGTH))
            def hook_ip(m):record((1,m.reg(0),0,0,0))
            def hook_http(m):record((2,m.reg(0),0,0,0))
            def hook_dtor(m):
                assert m.reg(1)==0x200156b5 and m.reg(2)==0x200365b8
                record((3,m.reg(0),0,0,0));return 0
            def hook_rfid(m):record((4,m.reg(0),m.reg(1),m.reg(2),m.reg(3)))
            def hook_octets(m):record((5,m.reg(0),m.reg(1),m.reg(2),(m.reg(3)<<8)|m.read_u32(m.reg('sp'))))
            def hook_clock(m):record((6,0,0,0,0));return next(ticks)
            for address,callback in [(0x200194d0,hook_ip),(0x20015688,hook_http),(0x2001ae90,hook_dtor),
                                      (0x20018494,hook_rfid),(0x200194ec,hook_octets),(0x20018efc,hook_clock)]:
                firmware.hook(address,callback)
            result=firmware.call(0x200090f8,max_instructions=20000)
            original=firmware.read(RAM_START,RAM_LENGTH)
            native_input=(ctypes.c_uint8*RAM_LENGTH).from_buffer_copy(memory)
            lib.test_init(native_input,t0,t1)
            native_output=(ctypes.c_uint8*RAM_LENGTH)();lib.test_state(native_output)
            diff=first_difference(bytes(native_output),original)
            assert diff is None,(fill,t0,t1,hex(RAM_START+diff),bytes(native_output)[diff:diff+16].hex(),original[diff:diff+16].hex())
            assert lib.test_event_count()==len(observed)
            for n,event in enumerate(observed):
                args=(ctypes.c_uint32*5)(); snapshot=(ctypes.c_uint8*RAM_LENGTH)()
                lib.test_event(n,args,snapshot)
                assert tuple(args)==event,(n,tuple(args),event)
                diff=first_difference(bytes(snapshot),snapshots[n])
                assert diff is None,(fill,'snapshot',n,hex(RAM_START+diff))
            cases.append({'initial_state':fill,'ticks':[t0,t1],'service_calls':len(observed),
                          'original_instructions':result['instructions']})
    return cases

def pid_fixture(rng, automatic):
    wire=bytearray(rng.randbytes(0x78))
    for name,offset,size in PID_FIELDS:
        if size==8:struct.pack_into('<d',wire,offset,rng.choice([-20.0,0.0,0.125,1.0,5.0,70.0,100.0,3000.0]))
    for i,offset in enumerate([0x34,0x38,0x3c]):struct.pack_into('<I',wire,offset,IO_ADDRESS+8*i)
    struct.pack_into('<i',wire,0x30,rng.choice([0,1]))
    struct.pack_into('<i',wire,0x58,rng.choice([1,100,250,1000]))
    struct.pack_into('<d',wire,0x60,0.0);struct.pack_into('<d',wire,0x68,3000.0)
    wire[0x70]=automatic
    return wire

def check_pid(lib,image):
    rng=random.Random(0x149c8)
    functions=[0x200149c8,0x200146d4,0x200147c0,0x20014890,0x20014962,0x20014982,0x2001491c,0x200146cc]
    names=['construct','compute','set_tunings','set_output_limits','set_mode','set_direction','initialize_integral','disable']
    records=[]
    for op,address in enumerate(functions):
        for i in range(64):
            automatic=0 if op==0 else i%2
            wire=pid_fixture(rng,automatic)
            input_value=rng.choice([-5.0,0.0,70.0,150.0,300.0])
            output_value=rng.choice([-1.0,0.0,0.5,3000.0,4000.0])
            target=rng.choice([0.0,70.0,150.0,300.0])
            a=rng.choice([-1.0,0.0,0.125,1.0,130.0,150.0]);b=rng.choice([-1.0,0.0,0.5,50.0,3000.0]);c=rng.choice([-1.0,0.0,30.0,80.0])
            mode=rng.choice([-1,0,1,2]);now=rng.choice([0,99,100,0xfffffff0,0xffffffff])
            firmware=ArmFirmware(image=image);firmware.write(PID_ADDRESS,wire)
            firmware.write(IO_ADDRESS,struct.pack('<ddd',input_value,output_value,target))
            clock_calls=[]
            firmware.hook(0x20018efc,lambda m:clock_calls.append(now) or now)
            args=[PID_ADDRESS];stack=[]
            if op==0:
                args+=[IO_ADDRESS,IO_ADDRESS+8,IO_ADDRESS+16]
                stack=[*words_from_double(a),*words_from_double(b),*words_from_double(c),mode&0xffffffff,0xdeadbeef,*words_from_double(3000.0)]
            elif op==2:
                args+=[0,*words_from_double(a)];stack=[*words_from_double(b),*words_from_double(c)]
            elif op==3:
                args+=[0,*words_from_double(a)];stack=[*words_from_double(b)]
            elif op in [4,5]:args+=[mode&0xffffffff]
            result=firmware.call(address,args=args,stack_args=stack,max_instructions=20000)
            native_wire=(ctypes.c_uint8*0x78).from_buffer_copy(wire)
            native_result=(ctypes.c_uint8*0x78)();native_io=(ctypes.c_double*3)()
            lib.test_pid(op,native_wire,input_value,output_value,target,a,b,c,mode,now,native_result,native_io)
            expected=firmware.read(PID_ADDRESS,0x78);actual=bytes(native_result)
            difference=first_difference(actual,expected)
            assert difference is None,(names[op],i,hex(difference),a,b,c,mode,actual[difference:difference+16].hex(),expected[difference:difference+16].hex())
            assert bytes(native_io)==firmware.read(IO_ADDRESS,24),(names[op],i,'IO mismatch')
            assert len(clock_calls)==(1 if op==0 or op==1 and automatic else 0)
            records.append({'function':names[op],'case':i,'original_instructions':result['instructions']})
    return records

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root',type=Path)
    parser.add_argument('--image',type=Path,default=PROJECT/'reference/original-0.1.34.bin')
    parser.add_argument('--elf',type=Path,help='Also test the actually linked ARM application')
    parser.add_argument('--write-fixture',type=Path,help='Write deterministic ARM test bridge C')
    parser.add_argument('--output',type=Path,default=HERE/'validation.json')
    args=parser.parse_args()
    if args.write_fixture:
        args.write_fixture.write_text(native_wrapper());return
    source=args.source_root.resolve() if args.source_root else (PROJECT if HERE.name=='tests' else HERE)
    include=source/'include'
    source_files=[source/'src/state.c',source/'src/pid.c'] if (source/'src/state.c').exists() else [source/'state.c',source/'pid.c']
    with tempfile.TemporaryDirectory(prefix='pico-state-') as tmp:
        tmp=Path(tmp);wrapper=tmp/'wrapper.c';wrapper.write_text(native_wrapper())
        library=tmp/'libstate.so'
        subprocess.run(['gcc','-std=gnu11','-O2','-fPIC','-shared','-Wall','-Wextra','-Werror',
                        '-fno-fast-math','-ffp-contract=off','-fexcess-precision=standard',
                        '-I'+str(include),*[str(p) for p in source_files],str(wrapper),'-o',str(library)],check=True)
        lib=NativeBackend(ctypes.CDLL(str(library)))
        initializers=check_initializers(lib,args.image)
        pid=check_pid(lib,args.image)
    rebuilt=None;rebuilt_initializers=[];rebuilt_pid=[]
    if args.elf:
        rebuilt=RebuiltBackend(args.elf)
        rebuilt_initializers=check_initializers(rebuilt,args.image)
        rebuilt_pid=check_pid(rebuilt,args.image)
    report={'passed':True,'image_sha256':IMAGE_SHA256,'initializer_cases':len(initializers),
            'pid_cases':len(pid),'total_cases':len(initializers)+len(pid),'mismatches':0,
            'original_instructions':sum(x['original_instructions'] for x in initializers+pid),
            'source_sha256':{str(p.relative_to(source)):hashlib.sha256(p.read_bytes()).hexdigest() for p in source_files},
            'rebuilt_arm_cases':len(rebuilt_initializers)+len(rebuilt_pid),
            'rebuilt_arm_instructions':rebuilt.instructions if rebuilt else 0,
            'rebuilt_elf_sha256':hashlib.sha256(args.elf.read_bytes()).hexdigest() if args.elf else None,
            'initializer_results':initializers,'pid_results':pid,
            'scope':'Original global initializer/PID/telemetry/menu/numeric instructions versus portable compiled C; 8 KiB of application RAM, service ordering and state at each service boundary compared. SDK construction/registration calls mocked.',
            'limitations':'Finite selected PID inputs; native C struct padding is not device layout. Snapshot tests explicitly serialize named fields/pointers. No interrupt concurrency, hardware or full application boot validation.'}
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ['passed','initializer_cases','pid_cases','total_cases','mismatches','original_instructions','rebuilt_arm_cases','rebuilt_arm_instructions']},indent=2))

if __name__=='__main__':main()
