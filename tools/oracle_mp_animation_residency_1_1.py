#!/usr/bin/env python3
"""Execute actual animation resource constructors/capacity and next IO stop.

The successful retained-unit/selector prefix is unchanged. Original124980
owns capacity from its real arena bounds and source1908E0 output. No resource
active byte, loaded descriptor, allocated datum or IO success is assigned.
"""
import argparse,ctypes,hashlib,json,os,subprocess,shutil
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESI,UC_X86_REG_ESP,UC_X86_REG_EIP
import oracle_mp_animation_resolution_1_1 as original
from oracle_smg_firing_1_1 import Trace as SourceTrace
from functions import _trace
ROOT=original.ROOT
EXTRA=(0x124980,0x124a30,0x124f30,0x124bf0,0x124ca0,0x124d00,
 0x124ef0,0x1250c0,0x13f020,0x13eae0,0x13efa0,0x16d390,0x16d4c0,
 0x1908e0,0x190930,0x19d8c0,0x2d7a1a)
ENTRIES=tuple(dict.fromkeys(original.ENTRIES+EXTRA))
PROTECTION=ROOT/'analysis/multiplayer/integration/spawn-source/local-map-protection'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def encoded(v):return(json.dumps(v,sort_keys=True,indent=2)+'\n').encode()
def adapter(out):
 cpp=PROTECTION/'compiled-compatibility-access.cpp';so=PROTECTION/'compiled-compatibility-access.so'
 prior=json.loads((PROTECTION/'report.json').read_text())['compiled_compatibility_access']
 assert sha(cpp)==prior['source_sha256']and sha(so)==prior['library_sha256']
 dest=out/'provider-dependencies';dest.mkdir(parents=True,exist_ok=True)
 for p in(cpp,so):
  q=dest/p.name
  if q.exists():assert q.read_bytes()==p.read_bytes()
  else:shutil.copyfile(p,q)
 lib=ctypes.CDLL(str(dest/so.name));cb=ctypes.CFUNCTYPE(ctypes.c_int,ctypes.c_uint32,ctypes.c_size_t,ctypes.c_uint32,ctypes.POINTER(ctypes.c_uint32))
 lib.contract.argtypes=[ctypes.c_uint32,ctypes.POINTER(ctypes.c_uint32)];lib.contract.restype=ctypes.c_int
 lib.apply.argtypes=[ctypes.c_uint32,ctypes.c_size_t,ctypes.c_uint32,cb];lib.apply.restype=ctypes.c_int
 return lib,cb,prior
class ResidencyTrace(original.ResolutionTrace):
 def __init__(self,data):
  self.new_scope=False;self.applied_access=[];self.residency_stages=[]
  super().__init__(data)
 def snapshot(self):
  return dict(runtime=self.runtime(),physical_arena34_hex=self.raw(0x4ed9f0,0x34),resource_globals1c_hex=self.raw(0x4eb118,0x1c),source_game_options_state4_hex=self.raw(self.u(0x4edf18)+8,4),source_budget_mode4_hex=self.raw(0x4f4860,4))
 def resource_finish(self,a):
  sp=self.g.reg_read(UC_X86_REG_ESP)
  for ret,entry_sp,row in self.resource_pending:
   if self.new_scope and ret==a and sp>entry_sp and row['entry']in('00124980','0013f020'):
    row['complete_state_after']=self.snapshot()
  super().resource_finish(a)
 def hook(self,g,a,size,user):
  if self.new_scope and a==0x800000b6:
   sp=g.reg_read(UC_X86_REG_ESP);base,n,p=(self.u(sp+4+i*4)for i in range(3));ret=self.u(sp)
   assert base%4096==0 and n>0 and n%4096==0
   lib,kind,meta=self.access_adapter;out=(ctypes.c_uint32*4)();assert lib.contract(p,out)==1
   before=bytes(g.mem_read(base,n));regs=self.registers();regions=[dict(start=f'{lo:08x}',end=f'{hi:08x}',permissions=perms)for lo,hi,perms in g.mem_regions()if lo<base+n and hi>=base]
   applied=[];errors=[]
   def apply(addr,count,windows,old):
    try:
     assert addr==base and count==n and windows==out[3]==0x40
     assert regions and all(q['permissions']==7 for q in regions)
     g.mem_protect(addr,count,7);after=[dict(start=f'{lo:08x}',end=f'{hi:08x}',permissions=perms)for lo,hi,perms in g.mem_regions()if lo<base+n and hi>=base]
     assert all(q['permissions']==7 for q in after);old[0]=0x40;applied.append(after);return 1
    except Exception as e:errors.append(str(e));return 0
   actual=lib.apply(base,n,p,kind(apply));assert actual==1 and len(applied)==1 and not errors
   after=bytes(g.mem_read(base,n));assert after==before
   self.applied_access.append(dict(phase=self.active,ordinal=182,return_site=f'{ret:08x}',args=[f'{v:08x}'for v in(base,n,p)],registers_before=regs,stack12_hex=self.raw(sp+4,12),compiled_masks=[f'{v:08x}'for v in out],regions_before=regions,regions_after=applied[0],content_sha256=hashlib.sha256(before).hexdigest(),contents_unchanged=True,void_abi=True,order=self.resource_order,state_before=self.snapshot(),scope='Unchanged pinned primary compatibility conversion/application; actual Unicorn.mem_protect on existing owned pages. No Xbox PTE/cache/hardware parity.'))
   self.resource_order+=1
   g.reg_write(UC_X86_REG_ESP,sp+16);g.reg_write(UC_X86_REG_EIP,ret);assert self.registers()==regs
   return
  if self.new_scope and a in(0x124f30,0x124bf0,0x124ca0,0x124d00,0x19d8c0):
   # Raw inputs are observed before the original body; no lower result applied.
   sp=g.reg_read(UC_X86_REG_ESP);self.io_inputs.append(dict(phase=self.active,entry=f'{a:08x}',registers=self.registers(),stack=[f'{self.u(sp+4+i*4):08x}'for i in range(6)]))
  before=self.snapshot()if self.new_scope and a in(0x124980,0x13f020)else None
  super().hook(g,a,size,user)
  if before is not None:
   row=self.resource_calls[-1];assert row['entry']==f'{a:08x}';row['complete_state_before']=before
 def attempt(self,name,entry,args=(),regs=()):
  self.active=name;self.new_invalid=None
  row=dict(name=name,entry=f'{entry:08x}',args=[f'{v:08x}'for v in args],registers_before=self.registers(),before=self.snapshot(),first_resource_call=len(self.resource_calls),first_resource_store=len(self.resource_stores))
  try:
   self.run(entry,args,regs);self.resource_finish(original.selection.ctor.STOP);self.selection_finish(original.selection.ctor.STOP);self.finish(original.selection.ctor.STOP)
   row['completion']='complete_original_return'
  except Exception as error:
   row.update(completion='unchanged_original_dependency_stop',error=str(error),invalid=self.new_invalid,pending_resource_calls=[q[2]['id']for q in self.resource_pending],pending_selection_calls=[q[2]['id']for q in self.selection_pending])
  row.update(after=self.snapshot(),instruction=f'{self.g.reg_read(UC_X86_REG_EIP):08x}',returned_eax=f'{self.g.reg_read(UC_X86_REG_EAX):08x}',registers_after=self.registers())
  self.residency_stages.append(row);return row['completion']=='complete_original_return'
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
 for f,h in original.FROZEN.items():assert sha(ROOT/f)==h,f
 access=adapter(out);original.ENTRIES=ENTRIES;old=original.ResolutionTrace;original.ResolutionTrace=ResidencyTrace
 with(out/'host-provider.stderr.log').open('w')as err:
  proc=subprocess.Popen(['wine',str(original.selection.ctor.owning.CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=err,text=True,env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(original.selection.ctor.owning.CONTEXT_OUT/'wine-prefix')))
  try:
   data=json.loads(proc.stdout.readline());t=original.execute(data);t.resource_finish(original.selection.ctor.STOP);assert not t.resource_pending
   t.baseline=dict(source_stages=t.player_stages,source_state=t.state(),source_calls=len(t.selection_calls),resource_calls=len(t.resource_calls))
   t.selection_enabled=False;t.resource_scope=True;t.source_raw_storage=True;t.new_scope=True;t.access_adapter=access;t.io_inputs=[]
   for name,entry in [('original_resource_constructor',0x1248d0),('original_resource_enable',0x124af0)]:
    if not t.attempt(name,entry):break
   else:
    # Actual existing options are read by the original mode producer, not
    # converted from a native playercount or chosen allocation success.
    if t.attempt('original_budget_mode_from_actual_options',0x1908e0,regs=((UC_X86_REG_ESI,t.u(0x4edf18)+8),)):
     mode=t.g.reg_read(UC_X86_REG_EAX);t.source_mode=mode
     if t.attempt('original_animation_capacity_callback_from_actual_mode',0x124980,(mode,)):
      # The descriptor is the literal owning source request observed during
      # the successful actual-unit constructor; prior delivery is removed.
      first=t.resource_requests[0];t.source_descriptor=f'{int(first["descriptor"],16):08x}'
      t.residency=False
      t.attempt('original_direct_residency_actual_constructor_descriptor',0x124f30,(int(first['source_stack_argument'],16),),((UC_X86_REG_EAX,int(first['descriptor'],16)),))
   listings=[]
   for e,r in t.resource_routines.items():
    ins=[t.code.at(i)for i in sorted(r.code)]
    for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes and bytes(t.g.mem_read(i.address,len(i.bytes)))==i.bytes
    f=out/'instructions'/f'{e:08x}.asm';raw=''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode();original.selection.save(f,raw)
    listings.append(dict(entry=f'{e:08x}',file=str(f.relative_to(out)),sha256=sha(f),rows=len(ins),executed_rows=sum(i.address in t.resource_executed for i in ins)))
   v=dict(schema='source-animation-residency-owning-v1',tool_sha256=sha(__file__),source_sha256=original.selection.ctor.PIN,cache_sha256=original.selection.ctor.CACHE_PIN,prior_pins=original.FROZEN,baseline=t.baseline,stages=t.residency_stages,resource_calls=t.resource_calls,resource_stores=t.resource_stores,io_inputs=t.io_inputs,source_mode=getattr(t,'source_mode',None),source_descriptor=getattr(t,'source_descriptor',None),host_raw_storage=t.host_storage,applied_access=t.applied_access,compiled_compatibility_access=access[2],actual_preexisting_resource_delivery=t.resource_requests,listings=listings,instruction_patches=[],new_guest_fields=[],supplied_gameplay_or_io_success=[],boundaries=['Unchanged genuine successful unit/selector preparation remains an inherited selected-startup/compatibility/options/resource-delivery boundary; not full title ordering.','Complete1248D0/124AF0 own pools/cache/gates; unchanged184 kernel ABI obtains genuinely owned mmap storage.','Complete1908E0 reads actual existing options; its output is passed to the literal original124980 budget callback. Other190930 callbacks are not skipped and claimed complete: this is a selected callback root, with current original arena state explicit.','Source124980 owns actual backing request/capacity; ordinal182 is a copied pinned compatibility provider with actually applied emulator protection and unchanged contents, not fabricated status or game readiness.','Prior124F30 authored-byte return provider is disabled before new direct actual-descriptor root; source lower calls execute unchanged until the first unavailable owner. No resource residency, IO or loaded descriptor result is supplied.'])
   original.selection.save(out/'report.json',encoded(v));print(json.dumps(dict(report=str(out/'report.json'),sha256=sha(out/'report.json'),stages=[dict(name=q['name'],completion=q['completion'],instruction=q['instruction'])for q in t.residency_stages])))
  finally:
   original.ResolutionTrace=old
   if proc.poll()is None:
    proc.stdin.write('x\n');proc.stdin.flush()
    try:proc.wait(timeout=15)
    except subprocess.TimeoutExpired:proc.kill();proc.wait(timeout=5)
   proc.stdin.close();proc.stdout.close()
 for f,h in original.FROZEN.items():assert sha(ROOT/f)==h,f
if __name__=='__main__':main()
