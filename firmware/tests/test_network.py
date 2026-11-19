#!/usr/bin/env python3
"""Execute recovered ARM C and compare original Thumb at explicit I/O boundaries.

The source ELF never contains the original image. Hardware is represented by
recorded boundary results; this is not appliance or radio emulation.
"""
from pathlib import Path
import argparse,hashlib,itertools,json,random,struct,sys

WORK=Path(__file__).resolve().parents[1]
ROOT=WORK if (WORK/'tools/original_arm.py').is_file() else WORK.parents[1]/'picobrew-application'
sys.path.insert(0,str(ROOT/'tools'))
from original_arm import ArmFirmware
from rebuilt_arm import RebuiltArm

ELF=ROOT/'build/application-validation.elf' if WORK==ROOT else WORK/'build/network.elf'
MAP={x['name']:int(x['address'],16) for x in json.loads((ROOT/'reference/function_map.json').read_text())}

def u32(x):return x&0xffffffff
def word(x):return struct.pack('<I',u32(x))
def f32(x):return struct.unpack('<I',struct.pack('<f',x))[0]
def old():
    f=ArmFirmware()
    for name,address in MAP.items():
        if name.startswith('Print_'):f.hook(address,lambda f:0)
    f.hook(MAP['pico_log_serial'],lambda f:0)
    f.hook(MAP['pico_still_log_error'],lambda f:0)
    f.hook(MAP['pico_assert_nonfatal'],lambda f:0)
    return f

class Harness:
    def __init__(self):
        self.f=RebuiltArm(ELF);self.f.call_symbol('fixture_bind')
        f=self.f;self.n=f.address('fixture_network');self.app=f.address('fixture_app');self.settings=f.address('fixture_settings')
        self.o=struct.unpack('<17I',f.read(f.address('fixture_offsets'),68));self.clock=100;self.tick=1
        self.input=b'';self.pos=0;self.active=True;self.sent=bytearray();self.events=[];self.files={};self.file=None;self.filepos=0
        self.short_write=None;self.open_fail=False;self.close_ok=True;self.boot=bytes(8);self.boot_read_ok=True;self.boot_write_ok=True
        self.init=True;self.init_ok=True;self.wifi_status=3;self.connect_ok=True;self.connecting=False
        self.local=b'\xc0\xa8\x01\x05';self.resolve_result=1;self.resolved=b'\x01\x02\x03\x04';self.decode_ok=True
        self.hook('millis',self.now);self.hook('delay',self.delay);self.hook('serial',lambda f:0)
        self.hook('serial_float',lambda f:None)
        self.hook('initialized',lambda f:int(self.init));self.hook('initialize',lambda f:int(self.init_ok))
        self.hook('connecting',lambda f:int(self.connecting));self.hook('status',lambda f:self.wifi_status)
        self.hook('begin',self.begin);self.hook('disconnect',self.disconnect);self.hook('local',lambda f:int.from_bytes(self.local,'little'))
        self.hook('resolve',self.resolve);self.hook('dns',lambda f:self.events.append(('dns',f.reg(1))))
        self.hook('scan',lambda f:0);self.hook('ssid',lambda f:0);self.hook('security',lambda f:4);self.hook('rssi',lambda f:-50)
        self.hook('mac',lambda f:f.write(f.reg(1),b'\x00\x11\x22\x33\x44\x55'))
        self.hook('md5',self.md5);self.hook('connect',self.connect);self.hook('connected',lambda f:int(self.active))
        self.hook('available',lambda f:int(self.pos<len(self.input)));self.hook('read',self.read);self.hook('write',self.write)
        self.hook('flush',lambda f:self.events.append(('flush',)));self.hook('stop',self.stop)
        self.hook('file_begin',lambda f:self.events.append(('file_begin',)));self.hook('file_open',self.file_open)
        self.hook('file_read',self.file_read);self.hook('file_write',self.file_write);self.hook('file_close',self.file_close)
        self.hook('file_remove',self.file_remove);self.hook('boot_read',self.boot_read);self.hook('boot_write',self.boot_write)
        self.hook('reset',lambda f:self.events.append(('reset',)));self.hook('ping',lambda f:5)
        self.hook('event',lambda f:self.events.append(('ui',f.reg(1),f.cstring(f.reg(2)) if f.reg(2) else None)))
        self.hook('error',lambda f:self.events.append(('error',f.reg(1),f.reg(2),f.reg(3))))
        for name in ['choose_wifi','diagnose','language','units','continue']:
            self.hook(name,lambda f:1)
        self.hook('choose_still',lambda f:0);self.hook('decode',self.decode)
        self.hook('logo',lambda f:self.events.append(('logo',f.read(f.reg(1),1024))))
        self.hook('image',lambda f:self.events.append(('image',f.reg(1),f.read(f.reg(2),1024))))
    def hook(self,name,fn):self.f.hook_symbol('fixture_'+name,fn)
    def now(self,f):value=self.clock;self.clock=u32(self.clock+self.tick);return value
    def delay(self,f):self.clock=u32(self.clock+f.reg(1));self.events.append(('delay',f.reg(1)))
    def begin(self,f):self.events.append(('begin',f.cstring(f.reg(1)),f.cstring(f.reg(2)),f.reg(3)));return self.wifi_status
    def disconnect(self,f):self.events.append(('disconnect',));self.connecting=False
    def resolve(self,f):self.events.append(('resolve',f.cstring(f.reg(1))));f.write(f.reg(2),self.resolved);return self.resolve_result
    def connect(self,f):self.events.append(('connect',f.reg(1),f.reg(2),f.reg(3)));self.active=self.connect_ok;return int(self.connect_ok)
    def read(self,f):
        if self.pos==len(self.input):return -1
        v=self.input[self.pos];self.pos+=1;return v
    def write(self,f):
        data=f.read(f.reg(1),f.reg(2));self.sent.extend(data);return f.reg(2) if self.short_write is None else self.short_write
    def stop(self,f):self.active=False;self.events.append(('stop',))
    def md5(self,f):
        data=f.read(f.reg(1),f.reg(2));self.events.append(('md5',data));f.write(f.reg(3),hashlib.md5(data).digest());return 1
    def file_open(self,f):
        name=f.cstring(f.reg(1)).decode();mode=f.reg(2);self.events.append(('open',name,mode))
        if self.open_fail or (mode in (0,0x1000) and name not in self.files):return 0
        if mode not in (0,0x1000):self.files[name]=b''
        self.file=name;self.filepos=0;return 1
    def file_read(self,f):
        data=self.files[self.file]
        if self.filepos==len(data):return -1
        value=data[self.filepos];self.filepos+=1;return value
    def file_write(self,f):
        data=f.read(f.reg(1),f.reg(2));self.events.append(('file_write',self.file,data))
        count=len(data)if self.short_write is None else self.short_write
        if count>0:self.files[self.file]=self.files[self.file][:self.filepos]+data[:count];self.filepos+=min(count,len(data))
        return count
    def file_close(self,f):self.events.append(('close',self.file));self.file=None;return int(self.close_ok)
    def file_remove(self,f):name=f.cstring(f.reg(1)).decode();self.events.append(('remove',name));self.files.pop(name,None);return 0
    def boot_read(self,f):
        self.events.append(('boot_read',));f.write(f.reg(1),self.boot);return 0 if self.boot_read_ok else -1
    def boot_write(self,f):
        self.events.append(('boot_write',f.read(f.reg(1),8)))
        if self.boot_write_ok:self.boot=f.read(f.reg(1),8);return 0
        return -1
    def decode(self,f):self.events.append(('decode',f.read(f.reg(1),f.reg(2))));return int(self.decode_ok)
    def call(self,name,*args,**kwargs):return self.f.call_symbol(name,[self.n,*args[:3]],stack_args=args[3:],**kwargs)['r0']
    def text(self,value,offset=0):address=self.f.SCRATCH+offset;self.f.write(address,value.encode()+b'\0');return address

def check_pure():
    h=Harness();f=old();count=0;rng=random.Random(354)
    sizes={0,1,0xf000,0x2f800,0xff0000}
    for g in [256,1024,4096,16384,65536]:
        sizes.update(x for x in [g-1,g,g+1,g*255-1,g*255,g*255+1] if x<=0xff0000)
    sizes.update(rng.randrange(0xff0001) for _ in range(250))
    for size,flag in itertools.product(sorted(sizes),[0,1,255,256,4096,0x12345678,0xffffffff]):
        a=f.call(0x20016ce8,[size,flag])['r0'];b=h.f.call_symbol('pico_storage_create_mode',[size,flag])['r0'];assert a==b,(size,flag,a,b);count+=1
    texts=['0.0.0.0','255.255.255.255','...','1..2.','1.2.3','1.2.3.4.5','256.0.0.1','1x.0.0.1','1.2.3.-1','9999999.2.3.4']
    texts += ['.'.join(str(rng.randrange(300)) for _ in range(4)) for _ in range(100)]
    for text in texts:
        f.write(f.SCRATCH,text.encode()+b'\0');f.write(f.SCRATCH+0x400,b'\xa5'*8)
        a=f.call(0x2000661c,[f.SCRATCH,f.SCRATCH+0x400])['r0'];h.f.write(h.f.SCRATCH+0x400,b'\xa5'*4)
        b=h.f.call_symbol('pico_network_parse_ipv4',[h.text(text),h.f.SCRATCH+0x400])['r0']
        assert a==b and f.read(f.SCRATCH+0x404,4)==h.f.read(h.f.SCRATCH+0x400,4),(text,a,b);count+=1
    for text in [''.join(chr(i) for i in range(1,128)),'Mash&Hold 150F','\\test~|!?%','']:
        f.write(f.SCRATCH,text.encode()+b'\0');f.call(MAP['pico_url_encode'],[f.SCRATCH,f.SCRATCH+0x400]);h.f.call_symbol('pico_network_url_encode',[h.text(text),h.f.SCRATCH+0x400,1024]);assert f.cstring(f.SCRATCH+0x400)==h.f.cstring(h.f.SCRATCH+0x400);count+=1
    values=[0.,-0.,0.2,0.3,-0.2,-1.25,152.9999,9999.999,1e-7]+[rng.uniform(-10000,10000)for _ in range(100)]
    for value,digits in itertools.product(values,[0,1,2,3]):
        f.call(0x20005950,[f32(value),f.SCRATCH,digits]);h.f.call_symbol('pico_network_format_float',[f32(value),h.f.SCRATCH,digits]);assert f.cstring(f.SCRATCH)==h.f.cstring(h.f.SCRATCH),(value,digits,f.cstring(f.SCRATCH),h.f.cstring(h.f.SCRATCH));count+=1
    return count

def check_repairs():
    count=0
    for text,capacity in [('#',0),('#',3),('abcdef',4)]:
        h=Harness();destination=h.f.SCRATCH+0x400;h.f.write(destination,b'\xa5'*16)
        result=h.f.call_symbol('pico_network_url_encode',[h.text(text),destination,capacity])['r0']
        assert result==0xffffffff and h.f.read(destination+capacity,16-capacity)==b'\xa5'*(16-capacity)
        if capacity:assert h.f.read(destination+capacity-1,1)==b'\0' or h.f.read(destination,1)==b'\0'
        count+=1
    h=Harness();assert h.f.call_symbol('pico_storage_create_mode',[0xff0001,0])['r0']==0;count+=1
    h=Harness();h.files['long']=b'12345';destination=h.f.SCRATCH+0x400;h.f.write(destination,b'\xa5'*8)
    assert h.call('pico_storage_read_text',h.text('long'),destination,4)==0 and h.f.read(destination,8)==b'123\0'+b'\xa5'*4;count+=1
    h=Harness();h.call('pico_storage_compute_uid');first=h.f.cstring(h.app+h.o[6]);h.call('pico_storage_compute_uid');assert h.f.cstring(h.app+h.o[6])==first;count+=1
    return count

def check_text():
    count=0
    for source in [None,'','a','1234567890123456789012345678901']:
        f=old();h=Harness();original=0 if source is None else f.SCRATCH;rebuilt=0 if source is None else h.text(source)
        if source is not None:f.write(f.SCRATCH,source.encode()+b'\0')
        for entry in [0x20004888,0x200064c4]:
            a=f.call(entry,[original])['r0'];b=h.f.call_symbol('pico_text_is_empty',[rebuilt])['r0'];assert a==b;count+=1
        for capacity,destination_null in itertools.product([0,1,2,8,32],[False,True]):
            original_logs=[];rebuilt_logs=[];dst=0 if destination_null else f.SCRATCH+0x400;newdst=0 if destination_null else h.f.SCRATCH+0x400
            f.write(f.SCRATCH+0x400,b'\xa5'*64);h.f.write(h.f.SCRATCH+0x400,b'\xa5'*64)
            f.hook(MAP['pico_log_serial'],lambda f:original_logs.append(tuple(f.cstring(f.reg(i))for i in [0,1,2]))or 0)
            h.f.hook_symbol('fixture_text_log',lambda f:rebuilt_logs.append(tuple(f.cstring(f.reg(i))for i in [1,2,3]))or 0)
            a=f.call(0x20004ad4,[dst,capacity,original])['r0'];b=h.f.call_symbol('pico_text_copy',[newdst,capacity,rebuilt,h.f.address('fixture_text_log')],stack_args=[0])['r0']
            assert a==b and original_logs==rebuilt_logs and f.read(f.SCRATCH+0x400,64)==h.f.read(h.f.SCRATCH+0x400,64),(source,capacity,destination_null,a,b,original_logs,rebuilt_logs);count+=1
    return count

def check_status():
    count=0
    for values in itertools.product((0,1),repeat=9):
        initialized,init_ok,connected,reconnect_ok,background,resolve,local,cached,dns_ok=values
        f=old();h=Harness();f.write(0x2002daff,bytes([initialized]));f.write_u32(0x2002dacc,0x04030201 if cached else 0);f.write_u32(0x20036ac8,0)
        f.hook(0x20014a70,lambda f:init_ok);f.hook(0x20014dd4,lambda f:3 if connected else 0);f.hook(0x20008b70,lambda f:reconnect_ok)
        def ip(f):f.write(f.reg(0),word(0x200294dc)+word(0x0501a8c0 if local else 0));return f.reg(0)
        f.hook(0x20014bd0,ip);f.hook(0x20008b98,lambda f:dns_ok);f.hook(0x200156f4,lambda f:0)
        h.init=bool(initialized);h.init_ok=bool(init_ok);h.wifi_status=3 if connected else 0;h.local=b'\xc0\xa8\x01\x05'if local else bytes(4);h.active=False
        h.f.write(h.n+h.o[0],b'\x01\x02\x03\x04'if cached else bytes(4));h.f.hook_symbol('pico_network_reconnect',lambda f:reconnect_ok);h.f.hook_symbol('pico_network_resolve_address',lambda f:dns_ok)
        a=f.call(0x2000bc50,[background,resolve])['r0'];b=h.call('pico_network_check_status',background,resolve);assert a==b,(values,a,b);count+=1
    return count

def check_dns():
    count=0
    for initialized,connected,cached,after,start in itertools.product((0,1),(0,1),(0,1),(1,3,60,None,-1),(0,0xffffff00)):
        f=old();h=Harness();clock=[start];calls=[0,0];h.clock=start;h.tick=0;h.init=bool(initialized);h.wifi_status=3 if connected else 0
        f.write(0x2002daff,bytes([initialized]));f.write_u32(0x2002dacc,0x04030201 if cached else 0);h.f.write_u32(h.n+h.o[0],0x04030201 if cached else 0)
        f.hook(0x20014dd4,lambda f:3 if connected else 0);f.hook(0x20018efc,lambda f:clock[0])
        def delay(f):clock[0]=u32(clock[0]+f.reg(0));return 0
        def lookup(f):
            calls[0]+=1
            if after==-1:return -1
            if after is not None and calls[0]>=after:f.write_u32(f.reg(2)+4,0x04030201)
            return 1
        def lookup_new(f):
            calls[1]+=1
            if after==-1:return -1
            if after is not None and calls[1]>=after:f.write_u32(f.reg(2),0x04030201)
            return 1
        f.hook(0x20018f08,delay);f.hook(0x20014fdc,lookup);h.hook('resolve',lookup_new)
        a=f.call(0x20008b98)['r0'];b=h.call('pico_network_resolve_address')
        assert a==b and calls[0]==calls[1] and clock[0]==h.clock,(initialized,connected,cached,after,start,a,b,calls,clock,h.clock);count+=1
    for show in [0,1]:
        h=Harness();h.local=bytes(4);h.tick=0;result=h.call('pico_network_wait_local_ip',1000,show)
        assert result==(0 if show else 1) and sum(e[1]for e in h.events if e[0]=='delay')==1200;count+=1
    return count

def check_framing():
    count=0
    scenarios=[(b'HTTP/1.0 200\r\n\r\n#OK#',b'',64,True,ord('#')),(b'#a|tail',b'',64,False,ord('|')),(b'##',b'prefix',64,True,ord('#')),(b'#abc#',b'',4,True,ord('#')),(b'#abc#',b'',3,True,ord('#'))]
    for incoming,prefix,capacity,close,term in scenarios:
        h=Harness();h.input=incoming;h.f.write(h.f.SCRATCH,prefix+b'\0');h.f.write_u32(h.f.SCRATCH+0x400,len(prefix))
        result=h.call('pico_network_read_response',h.f.SCRATCH,capacity,h.f.SCRATCH+0x400,close,term)
        if len(prefix)+len(incoming.split(b'#',1)[1].split(bytes([term]),1)[0])>=capacity:
            assert result==0 and not h.active
        else:
            f=old();pos=[0];active=[True];clock=[0]
            f.hook(0x200156f4,lambda f:int(active[0]));f.hook(0x20015218,lambda f:int(pos[0]<len(incoming)))
            def read(f):v=incoming[pos[0]];pos[0]+=1;return v
            f.hook(0x20015132,read);f.hook(0x20018efc,lambda f:0);f.hook(0x200151fc,lambda f:0)
            def stop(f):active[0]=False;return 0
            f.hook(0x20015318,stop);f.write(f.SCRATCH,prefix+b'\0');f.write_u32(f.SCRATCH+0x400,len(prefix))
            a=f.call(0x2000bd30,[f.SCRATCH,capacity-1,f.SCRATCH+0x400,close],stack_args=[term])['r0']
            assert result==a and h.f.cstring(h.f.SCRATCH)==f.cstring(f.SCRATCH) and h.active==active[0]
        count+=1
    for incoming in [b'',b'no hash',b'#missing end']:
        h=Harness();h.input=incoming;h.tick=10001;h.f.write_u32(h.f.SCRATCH+0x400,0)
        assert h.call('pico_network_read_response',h.f.SCRATCH,32,h.f.SCRATCH+0x400,True,ord('#'))==0 and not h.active;count+=1
    h=Harness();h.active=False;assert h.call('pico_network_call',h.text('/API/pico/test'),False,False)==1
    assert bytes(h.sent)==b'GET /API/pico/test HTTP/1.0\r\nHost: picobrew.com\nConnection: close\n\n';assert [e for e in h.events if e[0]=='delay']==[('delay',5)]*5;count+=1
    h=Harness();h.active=False;h.short_write=0;assert h.call('pico_network_call',h.text('/test'),False,False)==0;count+=1
    h=Harness();h.active=False;host=h.text('brew.local',0x700);h.f.write_u32(h.n+h.o[1],host);h.input=b'HTTP#';assert h.call('pico_network_call',h.text('/test'),True,False)==1;assert b'Host: brew.local' in h.sent;count+=1
    return count

def check_storage():
    count=0
    for security in [4,5,7]:
        h=Harness();h.f.write(h.settings+h.o[12],bytes([security]));assert h.call('pico_storage_save_wifi')==1
        assert h.files['ssid.txt']==b'Network' and h.files['wifiSecurity.txt']==str(security).encode()
        assert ('wifiPass.txt'in h.files)==(security!=7)
        h.f.write(h.settings,b'x\0');assert h.call('pico_storage_load_wifi')==1 and h.f.cstring(h.settings)==b'Network';count+=1
    for failure in ['open','short','close','oversize']:
        h=Harness();h.open_fail=failure=='open';h.short_write=2 if failure=='short'else None;h.close_ok=failure!='close'
        assert h.call('pico_storage_write_text',h.text('file',0x100),h.text('hello',0x200),4 if failure=='oversize'else 8)==0;count+=1
    h=Harness();h.files['long']=b'123456';dest=h.f.SCRATCH+0x100;h.f.write(dest,b'\xa5'*10)
    assert h.call('pico_storage_read_text',h.text('long'),dest,4)==0 and h.f.read(dest+4,6)==b'\xa5'*6;count+=1
    h=Harness();assert h.call('pico_storage_compute_uid')==1;assert h.f.cstring(h.app+h.o[6])==hashlib.md5(bytes.fromhex('001122334455')).hexdigest().encode();count+=1
    h=Harness();h.clock=60001;h.tick=0;assert h.call('pico_storage_save_recovery',3,4,5,0,False,1)==1
    assert h.files['restart.txt']==b'0:recipe42:0:0:5:3:4:0:1:';assert h.call('pico_storage_save_recovery',3,4,5,0,False,1)==0
    assert h.call('pico_storage_save_recovery',3,4,5,0,True,1)==1;count+=3
    for type in range(7):
        h=Harness();h.f.write_u32(h.app+h.o[7]+0x30,type);assert bool(h.call('pico_storage_save_recovery',0,0,0,0,True,0))==(type<2);count+=1
    return count

def check_storage_differential():
    count=0
    for units,language in itertools.product([0,1],[0,1,2]):
        f=old();h=Harness();captured=[]
        f.write(0x2002c164,bytes([language]));
        # Locate the exact unit global from the original mapped symbols.
        f.write(0x2002d9c9,bytes([units]));h.f.write(h.settings+h.o[12]+1,bytes([language,units]))
        f.hook(MAP['pico_file_write_text'],lambda f:captured.append((f.cstring(f.reg(0)).decode(),f.cstring(f.reg(1)),f.reg(2)))or 1)
        f.call(MAP['pico_settings_save_display']);h.call('pico_storage_save_display')
        assert h.files[captured[0][0]]==captured[0][1],(units,language,captured,h.files);count+=1
    for values in [(0,0,0,0,0,0,0,0),(1,60,4,5,0,25,-10,1),(0,-1,2147483647,-1,1,-1,-1,255)]:
        type,active,drain,step,resume,abv,ibu,calibrated=values;f=old();h=Harness();captured=[]
        f.hook(0x20018efc,lambda f:60001);h.tick=0;h.clock=60001
        f.hook(MAP['pico_file_write_text'],lambda f:captured.append((f.cstring(f.reg(0)).decode(),f.cstring(f.reg(1)),f.reg(2)))or 1)
        f.write(f.SCRATCH,b'recipe42\0');a=f.call(MAP['pico_recovery_save_state'],[f.SCRATCH,active,drain,step],stack_args=[resume,type,abv,ibu,1,calibrated])['r0']
        h.f.write_u32(h.app+h.o[7]+0x30,type);h.f.write_u32(h.app+h.o[8]+0x220,abv);h.f.write_u32(h.app+h.o[8]+0x224,ibu)
        b=h.call('pico_storage_save_recovery',active,drain,step,resume,True,calibrated)
        assert a==b==1 and h.files['restart.txt']==captured[0][1],(values,captured,h.files);count+=1
    for security in [4,5,7]:
        f=old();h=Harness();captured=[];f.write(0x2002c2d0,b'Network\0');f.write(0x2002c290,b'password\0');f.write(0x2002c2f1,bytes([security]));h.f.write(h.settings+h.o[12],bytes([security]))
        f.hook(0x20018f08,lambda f:0);f.hook(MAP['pico_file_write_text'],lambda f:captured.append((f.cstring(f.reg(0)).decode(),f.cstring(f.reg(1)),f.reg(2)))or 1)
        f.call(MAP['pico_settings_save_wifi'],[0,0,0,0]);h.call('pico_storage_save_wifi')
        assert all(h.files[name]==value for name,value,capacity in captured);count+=1
    return count

def check_still():
    count=0
    for text in ['1,2,3,4,5,0,1,0','-1.125,99.9,0.2,4,-5,0,0,1',',,,,,,,','1,2,3,4,5,0,0,0,extra']:
        h=Harness();h.tick=0;h.clock=1234;f=old();f.hook(0x20018efc,lambda f:1234)
        f.write(f.SCRATCH,text.encode()+b'\0');f.write(f.SCRATCH+0x400,b'\xa5'*96);h.f.write(h.f.SCRATCH+0x400,b'\xa5'*96)
        f.call(0x200064d8,[f.SCRATCH+0x400,f.SCRATCH]);assert h.call('pico_still_parse_response',h.f.SCRATCH+0x400,h.text(text))==1
        assert f.read(f.SCRATCH+0x400,96)==h.f.read(h.f.SCRATCH+0x400,96),(text,f.read(f.SCRATCH+0x400,64).hex(),h.f.read(h.f.SCRATCH+0x400,64).hex());count+=1
    for text in ['1,2','123456789012345,2,3,4,5,0,0,0']:
        h=Harness();h.f.write(h.f.SCRATCH+0x400,b'\xa5'*96);assert h.call('pico_still_parse_response',h.f.SCRATCH+0x400,h.text(text))==0;assert h.f.read(h.f.SCRATCH+0x400,96)==b'\xa5'*96;count+=1
    for action,path,method in [(0,'ping','GET'),(1,'configure','POST'),(2,'stillip','GET'),(3,'connecttolan','POST'),(4,'start','POST'),(5,'end','POST'),(6,'data','GET'),(7,'fanon','POST'),(8,'fanoff','POST')]:
        h=Harness();h.input=b'#T#';h.files={'ssid.txt':b'Home','wifiSecurity.txt':b'4','wifiPass.txt':b'secret'}
        assert h.call('pico_still_call',action,h.f.SCRATCH,51)==1
        body=b'ssid=Home&pass=secret'if action==1 else b''
        assert bytes(h.sent)==f'{method} /{path} HTTP/1.0\r\nContent-Length: {len(body)}\r\n\r\n'.encode()+body;count+=1
    h=Harness();h.connect_ok=False;assert h.call('pico_still_send_wifi')==0;count+=1
    h=Harness();assert h.call('pico_still_set_identifier',h.text('x'*16))==0;count+=1
    return count

def check_still_workflows():
    count=0
    for read_ok,failures,status,flag,pressure,temp in itertools.product([0,1],[0,4],[0,9,10],[0,1],[2.,5.],[93.,96.,float('nan')]):
        f=old();h=Harness();original=f.SCRATCH+0x400;rebuilt=h.app
        for m,address in [(f,original),(h.f,rebuilt)]:
            m.write(address,bytes(0x230));m.write_u32(address+4,failures);m.write_u32(address+0x58,3)
            m.write(address+0x61,bytes([status]));m.write(address+0x38,bytes([flag]));m.write(address+0x30,struct.pack('<d',pressure))
            m.write(address+0x18,struct.pack('<d',temp));m.write(address+0x40,struct.pack('<ddd',100.,5.,4.))
        f.hook(MAP['pico_still_read_status'],lambda f:read_ok);h.f.hook_symbol('pico_still_read_status',lambda f:read_ok)
        a=f.call(MAP['pico_still_has_blocking_status'],[original])['r0'];b=h.call('pico_still_blocked')
        assert a==b,(read_ok,failures,status,flag,pressure,temp,a,b);count+=1
    scenarios=[dict(wait=[0]),dict(wait=[1],ip=[0]),dict(wait=[1],ip=[1],home=[1],lan=[0]),
       dict(wait=[1,0],ip=[1],home=[1],lan=[1]),dict(wait=[1,1],ip=[1],home=[1],lan=[1]),
       dict(wait=[1],ip=[1],home=[0],send=[0]),dict(wait=[1,1,1],ip=[1,1,1],home=[0,0,0],send=[1,1]),
       dict(wait=[1,1,1],ip=[1,1],home=[0,1],send=[1],lan=[1])]
    for case in scenarios:
        results=[]
        for rebuilt in [False,True]:
            h=Harness()if rebuilt else None;f=h.f if rebuilt else old();events=[];indices={}
            def take(name):
                index=indices.get(name,0);indices[name]=index+1;values=case.get(name,[1]);return values[min(index,len(values)-1)]
            def wait(f):events.append(('wait',*[f.reg(i)for i in ([1,2,3]if rebuilt else[0,1,2]) ]));return take('wait')
            def ip(f):events.append(('ip',));return take('ip')
            def home(f):
                value=take('home');events.append(('home',value));return 0 if rebuilt and value else 1 if rebuilt else value
            def lan(f):events.append(('lan',));return take('lan')
            def send(f):events.append(('send',));return take('send')
            if rebuilt:
                for name,fn in [('pico_still_wait_response',wait),('pico_still_get_ip',ip),('pico_network_ipv4_is_zero',home),('pico_still_connect_lan',lan),('pico_still_send_wifi',send)]:f.hook_symbol(name,fn)
                h.hook('event',lambda f:events.append(('screen',f.cstring(f.reg(2)))))
                result=h.call('pico_still_configure')
            else:
                for name,fn in [('pico_still_wait_for_response',wait),('pico_still_get_ip_address',ip),('pico_still_has_home_ip',home),('pico_still_connect_to_lan',lan),('pico_still_send_wifi_credentials',send)]:f.hook(MAP[name],fn)
                f.hook(MAP['pico_screen_still_connecting'],lambda f:events.append(('screen',f.cstring(f.reg(0))))or 0)
                result=f.call(MAP['pico_still_configure'])['r0']
            results.append((result,events))
        assert results[0]==results[1],(case,results);count+=1
    return count

def envelope(data):
    text=data.hex();checksum=sum((-1 if i%15==0 else 1)*int(c,16)for i,c in enumerate(text));return str(len(data)).encode()+b','+hashlib.md5(str(checksum).encode()).hexdigest().encode()+b'|'+text.encode()+b'#'

def check_update():
    count=0
    for length in [1,2,247,248,249,496,1024]:
        h=Harness();data=bytes((i*37)&255 for i in range(length));h.input=envelope(data);assert h.call('pico_update_receive',False,max_instructions=3000000)==1
        assert h.files['/sys/mcuimg2.bin']==data and h.boot==bytes.fromhex('00000000badccdab')
        chunks=[len(e[2])for e in h.events if e[0]=='file_write'];assert all(x==248 for x in chunks[:-1]) and chunks[-1]<=248;count+=1
    for failure in ['open','short','checksum','length','header','boot_read','boot_write','close']:
        h=Harness();h.input=envelope(b'hello');h.open_fail=failure=='open';h.short_write=1 if failure=='short'else None
        h.boot_read_ok=failure!='boot_read';h.boot_write_ok=failure!='boot_write';h.close_ok=failure!='close'
        if failure=='checksum':h.input=h.input.replace(b',',b',x',1)
        if failure=='length':h.input=b'6'+h.input[1:]
        if failure=='header':h.input=b'9'*60+b',checksum|ff#'
        assert h.call('pico_update_receive',False)==0,(failure,h.events);count+=1
    for slot in [0,1,2,3]:
        h=Harness();h.boot=bytes([slot,5,6,7])+bytes(4);assert h.call('pico_boot_toggle_application',True)==1
        assert h.boot==bytes([2 if slot==1 else 1,5,6,7])+bytes.fromhex('badccdab');count+=1
    h=Harness();h.boot_write_ok=False;assert h.call('pico_boot_toggle_application',True)==0;count+=1
    return count

def check_update_differential():
    count=0
    for length in [1,247,248,249,496,1024]:
        data=bytes((i*37)&255 for i in range(length));incoming=envelope(data);f=old();h=Harness();h.input=incoming
        position=[0];original_chunks=[];rebuilt_chunks=[];original_sum=[]
        f.hook(MAP['pico_open_firmware_destination'],lambda f:1)
        f.hook(0x200156f4,lambda f:1);f.hook(0x20015218,lambda f:int(position[0]<len(incoming)))
        def read(f):v=incoming[position[0]];position[0]+=1;return v
        f.hook(0x20015132,read);f.hook(0x20018efc,lambda f:0)
        f.hook(MAP['pico_api_close_connection'],lambda f:0);f.hook(MAP['pico_file_close'],lambda f:0)
        def chunk(f):original_chunks.append(f.read(f.reg(0),f.reg(1)));return f.reg(1)
        def checksum(f):original_sum.append(struct.unpack('<i',word(f.reg(1)))[0]);return 1
        f.hook(MAP['pico_write_firmware_chunk'],chunk);f.hook(MAP['pico_validate_ota_checksum'],checksum)
        h.f.hook_symbol('pico_update_open_destination',lambda f:1)
        h.hook('file_write',lambda f:rebuilt_chunks.append(f.read(f.reg(1),f.reg(2)))or f.reg(2))
        a=f.call(MAP['pico_receive_firmware_image'],[0],max_instructions=3000000)['r0'];b=h.call('pico_update_receive',False,max_instructions=3000000)
        assert a==b==1 and original_chunks==rebuilt_chunks and b''.join(rebuilt_chunks)==data
        assert [int(e[1])for e in h.events if e[0]=='md5']==original_sum;count+=1
    return count

def api_original(response=b'T'):
    f=old();requests=[];errors=[]
    f.write(0x2002da9d,b'0123456789abcdef0123456789abcdef\0');f.write(0x2002bcdc,b'recipe42\0')
    program=f.SCRATCH+0x2000;f.write_u32(0x2002bcd8,program);f.write(program,b'session42\0');f.write_u32(program+0x30,0)
    f.hook(MAP['pico_api_open_request'],lambda f:requests.append((f.cstring(f.reg(0)),f.reg(1),f.reg(2)))or 1)
    def read(f):f.write(f.reg(0),response+b'\0');f.write_u32(f.reg(2),len(response));return 1
    f.hook(MAP['pico_api_read_hash_response'],read)
    f.hook(MAP['pico_report_error'],lambda f:errors.append((f.reg(0),f.reg(1),f.reg(2)))or 0)
    for name in ['pico_ui_draw_registration_code','pico_display_flush']:
        f.hook(MAP[name],lambda f:0)
    f.hook(0x20018f08,lambda f:0)
    return f,requests,errors

def check_api():
    count=0
    cases=[('pico_api_register_device','pico_api_register',(),r)for r in [b'T',b'F',b'F:CODE42',b'FF',b'']]
    cases += [('pico_api_get_session_id','pico_api_get_session',('destination',22),r)for r in [b'',b'a',b'x'*21,b'x'*22]]
    cases += [('pico_api_create_picopak','pico_api_create_picopak',(42,),r)for r in [b'',b'x'*20,b'x'*21]]
    cases += [(oldname,'pico_api_still_permission',(method,'identifier'),r)for oldname,method in [('pico_api_can_use_still','canUsePicoStill'),('pico_api_has_cleaned_ack','hasCleanedAck'),('pico_api_set_cleaned_ack','setCleanedAck')]for r in [b'T',b'F',b'bad']]
    for original,recovered,args,response in cases:
        f,requests,errors=api_original(response);h=Harness();h.active=False;h.input=b'HTTP/1.0 200\r\n\r\n#'+response+b'#'
        if recovered=='pico_api_get_session':oldargs=[f.SCRATCH,22];newargs=[h.f.SCRATCH,22]
        elif recovered=='pico_api_still_permission':
            f.write(f.SCRATCH,b'still-id\0');oldargs=[f.SCRATCH];newargs=[h.text(args[0]),h.text('still-id',0x100)]
        else:oldargs=list(args);newargs=list(args)
        a=f.call(MAP[original],oldargs)['r0'];b=h.call(recovered,*newargs)
        assert bool(a)==bool(b),(original,response,a,b)
        actual=bytes(h.sent).split(b' ',2)[1];assert actual==requests[0][0],(original,actual,requests)
        assert errors==[e[1:]for e in h.events if e[0]=='error'],(original,errors,h.events)
        if recovered=='pico_api_get_session'and a:assert f.cstring(f.SCRATCH)==h.f.cstring(h.f.SCRATCH)
        count+=1
    for response,capacity in [(b'1,2,3',8),(b'1,,bad,23,0',22),(b'',8),(b'1,2,3',2)]:
        f,requests,_=api_original(response);h=Harness();h.active=False;h.input=b'#'+response+b'#'
        a=f.call(MAP['pico_api_get_required_actions'],[f.SCRATCH,capacity])['r0'];b=h.call('pico_api_required_actions',h.f.SCRATCH,capacity)
        assert a==b and f.read(f.SCRATCH,a*4)==h.f.read(h.f.SCRATCH,b*4),(response,capacity,a,b);count+=1
    h=Harness();h.active=False;h.input=b'#1,,bad,23,0#';assert h.call('pico_api_required_actions',h.f.SCRATCH,8)==3
    assert struct.unpack('<3I',h.f.read(h.f.SCRATCH,12))==(1,23,0);count+=1
    for kind in [0,3]:
        f,requests,_=api_original();h=Harness();h.active=False
        f.write_u32(f.SCRATCH+0x2030,kind);h.f.write_u32(h.app+h.o[7]+0x30,kind)
        f.write(0x2002d2f8+0x1f0,struct.pack('<d',152.9));f.write(0x2002d2f8+0x1f8,struct.pack('<d',180.3))
        f.write(0x2002d518,struct.pack('<d',.2));f.write_u32(0x2002d2f8+0x1a8,60)
        f.hook(MAP['pico_api_make_wifi_call'],lambda f:requests.append((f.cstring(f.reg(0)),f.reg(1),f.reg(2)))or 1)
        f.write(f.SCRATCH,b'Mash & Hold\0');a=f.call(MAP['pico_wifi_log_event'],[f.SCRATCH,f.SCRATCH])['r0'];b=h.call('pico_api_log_event',h.text('Mash & Hold'),h.f.SCRATCH)
        actual=bytes(h.sent).split(b' ',2)[1];assert a==b==1 and actual==requests[0][0],(kind,actual,requests);count+=1
    h=Harness();h.f.write(h.settings+h.o[13],b'\1');assert h.call('pico_api_log_step',h.text('Mash'))==1 and not h.sent;count+=1
    h=Harness();h.f.write_u32(h.n+h.o[3],5);assert h.call('pico_api_log_event',h.text('Mash'),0)==0 and not h.sent;assert ('disconnect',)in h.events;count+=1
    for associated in [False,True]:
        h=Harness();h.active=False;h.input=b'#id1,Ale|id2,Ale|id3,Stout|#';listing=h.f.address('fixture_listing')
        assert h.call('pico_api_recipe_list',listing,associated)==1
        assert struct.unpack('<4i',h.f.read(listing,16))==(3,0,3,2 if associated else 3)
        assert h.f.cstring(listing+16)==b'id1';assert h.f.cstring(listing+16+19*22)==b'Ale';count+=1
    for pages in [4,5,7,8,15]:
        h=Harness();h.active=False;h.f.write_u32(h.app+h.o[10]+4,pages)
        h.input=b'#valid recipe|'+b'00'*1024+b'|'+(b'11'*1024+b'|')*4+b'#'
        assert h.call('pico_api_fetch_recipe',h.text('recipe42'),-1,-1,max_instructions=3000000)==0
        images=[e for e in h.events if e[0]=='image'];expected=max(0,4-(pages-4))if pages>=4 else 0
        if pages>=15:expected=0
        assert len(images)==expected and h.f.read_u32(h.app+h.o[10]+4)==pages+expected*2,(pages,len(images),expected)
        count+=1
    return count

def main():
    global ELF
    p=argparse.ArgumentParser();p.add_argument('--elf',type=Path,default=ELF);p.add_argument('--output',type=Path,default=WORK/'analysis/network_validation.json');args=p.parse_args();ELF=args.elf
    checks={}
    for name in ['pure','text','status','dns','framing','storage','storage_differential','still','still_workflows','update','update_differential','api','repairs']:
        checks[name]=globals()['check_'+name]();print(name,checks[name],flush=True)
    report={'passed':True,'checks':checks,'total_cases':sum(checks.values()),'elf_sha256':hashlib.sha256(ELF.read_bytes()).hexdigest(),'reference_sha256':hashlib.sha256((ROOT/'reference/original-0.1.34.bin').read_bytes()).hexdigest(),'limitations':'Function-level original Thumb/recompiled ARM differential and bounded I/O scenarios; no radio, flash-device, MCU or appliance test.'}
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
