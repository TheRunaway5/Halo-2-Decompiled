#!/usr/bin/env python3
"""Independent original request-resolution/resource observer; no new guest inputs."""
import argparse,hashlib,json,os,subprocess,struct,mmap,ctypes
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX
import oracle_mp_animation_selection_1_1 as selection
from functions import _trace
from oracle_smg_firing_1_1 import Trace as SourceTrace
ROOT=selection.ROOT
ENTRIES=(0x12000,0x190690,0x16d2c0,0x1248d0,0x124af0,0x16cff0,0x13e8e0,0x1e13c0,0x1e1230,0x1de9b0,0x124e40,0x124dd0,0x125120)
FROZEN={**selection.FROZEN,
 'tools/oracle_mp_animation_selection_1_1.py':hashlib.sha256((ROOT/'tools/oracle_mp_animation_selection_1_1.py').read_bytes()).hexdigest(),
 'analysis/multiplayer/integration/spawn-source/animation-selection/measured/report.json':'8aecb3c40d0da10dd437c6034d4935e9373dd9bd646dfb67bdde87e653c036b2'}
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
class ResolutionTrace(selection.SelectionTrace):
 def __init__(self,data):
  self.resource_calls=[];self.resource_pending=[];self.resource_stores=[];self.resource_executed=set();self.resource_order=0;self.resource_scope=False;self.source_raw_storage=False;self.host_maps=[];self.host_storage=[];self.host_guest_next=0xb500000
  super().__init__(data)
  self.resource_routines={e:_trace(self.code,e,self.starts|set(ENTRIES))for e in ENTRIES}
  self.resource_addresses={a:e for e,r in self.resource_routines.items()for a in r.code}
 def runtime(self):
  memory={f'{p:08x}':self.raw(p,n)for p,n in((0x518410,2),(0x4eb118,20),(0x5662cc,4),(0x46fcb0,36))}
  for slot in(0x4eb118,0x4eb11c):
   p=self.u(slot)
   if p:
    n=self.u(p+0x20);stride=self.u(p+0x24);assert n<=512 and stride in(12,16)
    memory[f'{p:08x}']=self.raw(p,0x4c+n*stride+((n+31)//32)*4)
  p=self.u(0x4eb124)
  if p:memory[f'{p:08x}']=self.raw(p,0x30fc)
  return memory
 def resource_finish(self,a):
  sp=self.g.reg_read(UC_X86_REG_ESP)
  for q in reversed(self.resource_pending[:]):
   ret,entry_sp,row=q
   if ret!=a or sp<=entry_sp:continue
   row.update(return_eax=f'{self.g.reg_read(UC_X86_REG_EAX):08x}',gates_after=self.raw(0x518410,2),return_order=self.resource_order)
   if'descriptor'in row:row['descriptor_after']=self.raw(int(row['descriptor'],16),20)
   if 'runtime_before'in row:row['runtime_after']=self.runtime()
   if row['entry']=='0016d2c0':row['allocation_pool_return']=f'{self.g.reg_read(UC_X86_REG_EDX):08x}'
   self.resource_order+=1;self.resource_pending.remove(q)
 def hook(self,g,a,size,user):
  if self.source_raw_storage and a==0x33785c:
   # Execute the unchanged XAPI wrapper. Bypass only old Python observers
   # restricted to earlier fixed-storage requests; no source bytes/registers
   # or returned allocation result are supplied at this entry.
   self.services.pop(a,None)
   self.executed.add(a);SourceTrace.hook(self,g,a,size,user);return
  if self.source_raw_storage and a==0x800000b8:
   sp=g.reg_read(UC_X86_REG_ESP);args=[self.u(sp+4+i*4)for i in range(5)]
   bp,zero,ns,flags,protect=args;requested_base,requested_size=self.u(bp),self.u(ns)
   assert requested_base==0 and zero==0 and flags==0x101000 and protect==4
   page=os.sysconf('SC_PAGE_SIZE');assert page==4096
   cap=(requested_size+page-1)//page*page
   storage=mmap.mmap(-1,cap,prot=mmap.PROT_READ|mmap.PROT_WRITE)
   native=ctypes.addressof(ctypes.c_ubyte.from_buffer(storage));address=self.host_guest_next;self.host_guest_next+=cap
   g.mem_map_ptr(address,cap,3,native);self.host_maps.append(storage)
   self.host_storage.append(dict(phase=self.active,kernel_ordinal=184,caller=f'{self.u(sp):08x}',args=[f'{v:08x}'for v in args],requested_base=f'{requested_base:08x}',requested_size=requested_size,resulting_base=f'{address:08x}',resulting_size=cap,host_page_bytes=page,initial_sha256=hashlib.sha256(storage[:]).hexdigest(),adapter_ntstatus='00000000',scope='actual owned mmap raw storage; original XAPI request/body and source constructors own fields. Adapter status0 follows successful host allocation/map; physical Xbox allocation semantics remain unproved.'))
   self.w(bp,address);self.w(ns,cap)
   g.reg_write(UC_X86_REG_EAX,0);g.reg_write(UC_X86_REG_ESP,sp+24);g.reg_write(UC_X86_REG_EIP,self.u(sp));return
  enabled=self.selection_enabled or self.resource_scope
  if enabled:
   self.resource_finish(a)
   if a in self.resource_addresses:self.resource_executed.add(a)
   if a in ENTRIES and(self.selection_pending or self.resource_pending or self.resource_scope):
    sp=g.reg_read(UC_X86_REG_ESP);regs={k:int(v,16)for k,v in self.registers().items()}
    row=dict(id=len(self.resource_calls),entry=f'{a:08x}',caller=f'{self.u(sp):08x}',phase=self.active,order=self.resource_order,registers=regs,stack=[f'{self.u(sp+4+i*4):08x}'for i in range(4)],selection_ancestors=[q[2]['id']for q in self.selection_pending],resource_ancestors=[q[2]['id']for q in self.resource_pending],gates_before=self.raw(0x518410,2),globals_before={f'{p:08x}':self.raw(p,4)for p in(0x4eb118,0x4eb11c,0x4eb124,0x5662cc)})
    if self.u(0x518411,1) and a in(0x124e40,0x125120,0x16d2c0):row['runtime_before']=self.runtime()
    if a==0x124e40:row.update(descriptor=f'{regs["eax"]:08x}',descriptor_before=self.raw(regs['eax'],20))
    self.resource_calls.append(row);self.resource_pending.append((self.u(sp),sp,row));self.resource_order+=1
  count_before=len(self.selection_calls)
  super().hook(g,a,size,user)
  if enabled and a==0x1deb60 and len(self.selection_calls)>count_before:
   row=self.selection_calls[-1];s=[int(v,16)for v in row['stack']]
   assert row['entry']=='001deb60'
   row['output_locations']={'packed':(s[1],4)}
   for k,p in zip(('mode','weapon_class','weapon'),s[5:8]):
    if p:row['output_locations'][k]=(p,4)
   row['outputs_before']=self.outputs(row)
 def mem_write(self,g,access,p,n,value,user):
  if self.selection_enabled or self.resource_scope:
   ip=g.reg_read(UC_X86_REG_EIP)
   if ip in self.resource_addresses:
    self.resource_stores.append(dict(order=self.resource_order,instruction=f'{ip:08x}',address=f'{p:08x}',size=n,before=self.raw(p,n),written=(value&((1<<(8*n))-1)).to_bytes(n,'little').hex(),resource_calls=[q[2]['id']for q in self.resource_pending]));self.resource_order+=1
  super().mem_write(g,access,p,n,value,user)
def execute(data):
 original=selection.SelectionTrace
 try:
  selection.SelectionTrace=ResolutionTrace
  return selection.execute(data)
 finally:selection.SelectionTrace=original

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--producer-probe',action='store_true');p.add_argument('--source-raw-storage',action='store_true');args=p.parse_args()
 for f,h in FROZEN.items():assert sha(ROOT/f)==h,f
 args.output.mkdir(parents=True,exist_ok=True)
 with(args.output/'host-provider.stderr.log').open('w')as err:
  proc=subprocess.Popen(['wine',str(selection.ctor.owning.CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=err,text=True,env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(selection.ctor.owning.CONTEXT_OUT/'wine-prefix')))
  try:
   data=json.loads(proc.stdout.readline());t=execute(data);t.resource_finish(selection.ctor.STOP);assert not t.resource_pending
   prior=json.loads((ROOT/'analysis/multiplayer/integration/spawn-source/animation-selection/measured/report.json').read_text())
   # Only additional literal output observations differ in1deb60 rows.
   normalized=[]
   for row in t.selection_calls:
    c=dict(row)
    if c['entry']=='001deb60':c['output_locations']={};c['outputs_before']={};c['outputs_after']={}
    normalized.append(c)
   assert json.loads(json.dumps(normalized))==prior['calls'];assert t.selection_stores==prior['stores'];assert t.player_stages==prior['source_stages']
   graphs=[]
   for old in prior['graphs']:
    ptr=int(old['pointer'],16);raw=t.raw(ptr,old['bytes']);author=bytes(t.cache.data[ptr-t.cache.magic:ptr-t.cache.magic+old['bytes']]).hex()
    graphs.append(dict(**old,complete_payload_hex=raw,authored_cache_equal=raw==author))
   producer_probe=[]
   if args.producer_probe:
    t.selection_enabled=False;t.resource_scope=True;t.source_raw_storage=args.source_raw_storage
    for name,entry,argv,regs in [('original_resource_constructor',0x1248d0,(),()),('original_resource_enable',0x124af0,(),())]:
     t.active=name;row=dict(name=name,entry=f'{entry:08x}',gates_before=t.raw(0x518410,2),globals_before={f'{p:08x}':t.raw(p,4)for p in(0x4eb118,0x4eb11c,0x4eb124)})
     try:t.run(entry,argv,regs);t.resource_finish(selection.ctor.STOP);row['completion']='complete_original_return'
     except Exception as error:row.update(completion='original_or_observer_stop',error=str(error),instruction=f'{t.g.reg_read(UC_X86_REG_EIP):08x}',invalid=t.new_invalid)
     row.update(runtime_after=t.runtime(),resource_state_after={f'{p:08x}':dict(pointer=f'{t.u(p):08x}',raw4c_hex=t.raw(t.u(p),0x4c)if t.u(p)else None)for p in(0x4eb118,0x4eb11c,0x4eb124)},gates_after=t.raw(0x518410,2),globals_after={f'{p:08x}':t.raw(p,4)for p in(0x4eb118,0x4eb11c,0x4eb124)})
     producer_probe.append(row)
     if row['completion']!='complete_original_return':break
    if producer_probe[-1]['completion']=='complete_original_return':
     # Exact same declared final bind arguments as frozen constructor caller;
     # current source manager/graphs are retained, with no input field writes.
     last=[c for c in t.ctor_calls if c['entry']=='001ceac0'][-1]
     t.active='original_resource_enabled_same_final_bind';t.selection_enabled=True
     reg=last['registers'];row=dict(name=t.active,entry='001ceac0',source_prior_call=last['id'],gates_before=t.raw(0x518410,2),runtime_before=t.runtime(),first_selection_call=len(t.selection_calls),first_resource_call=len(t.resource_calls))
     try:t.run(0x1ceac0,(0,),((selection.ctor.UC_X86_REG_ESI,reg['esi']),(selection.ctor.UC_X86_REG_EDI,reg['edi']),(UC_X86_REG_EDX,reg['edx'])));t.selection_finish(selection.ctor.STOP);t.finish(selection.ctor.STOP);t.resource_finish(selection.ctor.STOP);row['completion']='complete_original_return'
     except Exception as error:row.update(completion='original_or_observer_stop',error=str(error),instruction=f'{t.g.reg_read(UC_X86_REG_EIP):08x}',invalid=t.new_invalid)
     row.update(gates_after=t.raw(0x518410,2),runtime_after=t.runtime(),after_manager_hex=t.raw(reg['esi'],0x90));producer_probe.append(row)
    t.resource_scope=False
   listings=[]
   routines={**t.selection_routines,**t.resource_routines}
   for e,r in routines.items():
    ins=[t.code.at(i)for i in sorted(r.code)]
    for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes and bytes(t.g.mem_read(i.address,len(i.bytes)))==i.bytes
    raw=''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode();f=args.output/'instructions'/f'{e:08x}.asm';selection.save(f,raw)
    listings.append(dict(entry=f'{e:08x}',file=str(f.relative_to(args.output)),sha256=sha(f),rows=len(ins),executed_rows=sum(i.address in(t.selection_executed|t.resource_executed)for i in ins)))
   report=dict(schema='source-animation-resolution-v1',tool_sha256=sha(__file__),frozen_pins=FROZEN,xbe_sha256=selection.ctor.PIN,cache_sha256=selection.ctor.CACHE_PIN,source_stages=t.player_stages,selection_calls=t.selection_calls,selection_stores=t.selection_stores,resource_calls=t.resource_calls,resource_stores=t.resource_stores,graphs=graphs,listings=listings,resource_delivery=t.resource_requests,producer_probe=producer_probe,host_raw_storage=t.host_storage,new_guest_inputs=[],instruction_patches=[],new_supplied_results=t.host_storage,provided_gameplay_results=[],boundaries=['Same unchanged nine owning selector calls and18pure roots as frozen selection. Temporary Pythonclass binding only selects observer; originalguestinstructions/state unchanged.','Inherited explicit124f30 resource-byte delivery remains a provider boundary; this observer does not execute complete resource cache scheduling/retirement/IO.','Runtime gate bytes and complete descriptor/payloads are observed, not assigned. New raw host storage/status outputs are separately recorded; source constructors and allocators own all gameplay readiness, resource status, variants, poses, identities and allocation results.'])
   selection.save(args.output/'report.json',selection.encoded(report));print(json.dumps(dict(report=str(args.output/'report.json'),sha256=sha(args.output/'report.json'),resource_calls=len(t.resource_calls),gates=sorted(set(c['gates_before']for c in t.resource_calls)),resource_stores=len(t.resource_stores))))
   for f,h in FROZEN.items():assert sha(ROOT/f)==h,f
  finally:
   if proc.poll()is None:
    proc.stdin.write('x\n');proc.stdin.flush()
    try:proc.wait(timeout=15)
    except subprocess.TimeoutExpired:proc.kill();proc.wait(timeout=5)
   proc.stdin.close();proc.stdout.close()
if __name__=='__main__':main()
