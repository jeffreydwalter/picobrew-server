"""Same function-test semantics with hooks restricted to observed entry points.
Unicorn still enforces per-call instruction limits. Unlike the general tracer,
this harness does not invoke Python for every unobserved machine instruction.
"""
from pathlib import Path
import hashlib
from elftools.elf.elffile import ELFFile
from unicorn import Uc,UC_ARCH_ARM,UC_MODE_THUMB,UC_MODE_MCLASS,UC_HOOK_CODE
from original_arm import ArmFirmware as Base,DEFAULT_IMAGE,IMAGE_SHA256
from rebuilt_arm import RebuiltArm as Linked
class FastHooks:
 def setup(self):
  self.uc=Uc(UC_ARCH_ARM,UC_MODE_THUMB|UC_MODE_MCLASS);self.uc.mem_map(0x20000000,0x100000);self.uc.mem_write(self.RETURN,b'\x00\xbe');self.hooks={};self.handles={};self.trace=[];self.instructions=0;self.returned=False
  self.uc.hook_add(UC_HOOK_CODE,self._instruction,begin=self.RETURN,end=self.RETURN)
 def hook(self,address,callback):
  address &=~1
  if address not in self.handles:self.handles[address]=self.uc.hook_add(UC_HOOK_CODE,self._instruction,begin=address,end=address)
  self.hooks[address]=callback
 def unhook(self,address):
  address &=~1
  if address in self.handles:self.uc.hook_del(self.handles.pop(address))
  self.hooks.pop(address,None)
class ArmFirmware(FastHooks,Base):
 _data=Path(DEFAULT_IMAGE).read_bytes()
 assert hashlib.sha256(_data).hexdigest()==IMAGE_SHA256,'Original firmware hash differs from pinned provenance'
 def __init__(self):self.setup();self.uc.mem_write(0x20004000,self._data)
class RebuiltArm(FastHooks,Linked):
 _cache={}
 def __init__(self,path):
  self.setup();path=Path(path);key=(str(path),path.stat().st_mtime_ns)
  if key not in self._cache:
   with path.open('rb') as fp:
    elf=ELFFile(fp);segments=[(s['p_vaddr'],s.data()) for s in elf.iter_segments() if s['p_type']=='PT_LOAD'];symbols={s.name:s['st_value'] for s in elf.get_section_by_name('.symtab').iter_symbols() if s.name and s['st_shndx']!='SHN_UNDEF'}
   self._cache[key]=(segments,symbols)
  segments,self.symbols=self._cache[key]
  for address,data in segments:self.uc.mem_write(address,data)
