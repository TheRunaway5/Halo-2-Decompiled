#!/usr/bin/env python3
"""Observe real source-created object heap allocation/growth; no result stubs."""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBX
import oracle_mp_generic_object_constructor_1_1 as prior
from functions import _trace
ROOT=prior.ROOT
OWN=(0x125520,0x18fc80,0x18fce0,0x18fd70,0x18fe60,0x18ff20,0x18ff90,0x190030,0x1900c0)
class Trace(prior.GenericTrace):
 def __init__(self,*args,**kw):
  self.heap_enabled=False;self.heap_calls=[];self.heap_pending=[];self.heap_reads=[];self.heap_writes=[];self.heap_order=0;self.heap_extents={};self.heap_states={};self.heap_output=None;super().__init__(*args,**kw)
  self.heap_routines={e:_trace(self.code,e,self.starts|set(OWN))for e in OWN};self.heap_addresses={a for f in self.heap_routines.values()for a in f.code};self.heap_sites={}
  for p in self.heap_addresses:
   ins=self.code.at(p)
   if ins.mnemonic=='call'and ins.operands[0].type==2:self.heap_sites[p+ins.size]=(p,ins.operands[0].imm)
  self.g.hook_add(unicorn.UC_HOOK_MEM_READ,self.heap_read);self.g.hook_add(unicorn.UC_HOOK_MEM_WRITE,self.heap_write)
 def heap_pointer(self,entry,sp):
  reg={0x18fc80:UC_X86_REG_ESI,0x18fce0:UC_X86_REG_ECX,0x18fe60:UC_X86_REG_EDX,0x190030:UC_X86_REG_EDI}
  if entry in reg:return self.g.reg_read(reg[entry])
  if entry in(0x18fd70,0x18ff20,0x18ff90,0x1900c0):return self.u(sp+4)
  return self.heap_pending[-1][2]['heap']if self.heap_pending else 0
 def heap_snapshot(self,heap):
  arena=self.u(0x4ed650);cursor=self.u(0x4ed654);r=dict(arena=f'{arena:08x}',cursor=cursor,checksum=self.raw(0x4ed65c,4),heap=f'{heap:08x}')
  if heap in self.heap_extents:
   n=self.heap_extents[heap];raw=bytes(self.g.mem_read(heap,n));h=hashlib.sha256(raw).hexdigest();path=self.heap_output/'storage'/f'{h}.bin'
   if not path.exists():path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(raw)
   self.heap_states.setdefault(h,dict(address=f'{heap:08x}',bytes=n,file=str(path.relative_to(self.heap_output)),sha256=h))
   r.update(storage=h,header64=raw[:64].hex())
  pool=self.u(0x4e78d0)
  if pool:
   r['object_pool']=dict(address=f'{pool:08x}',raw76=self.raw(pool,76));heads=self.u(pool+0x44)
   if heads:r['object_headers']=dict(address=f'{heads:08x}',raw24576=self.raw(heads,2048*12))
  return r
 def heap_read(self,g,kind,p,n,value,user):
  pc=g.reg_read(UC_X86_REG_EIP)
  if self.heap_enabled and pc in self.heap_addresses:self.heap_reads.append(dict(order=self.heap_order,instruction=f'{pc:08x}',address=f'{p:08x}',bytes=n,raw_hex=self.raw(p,n)));self.heap_order+=1
 def heap_write(self,g,kind,p,n,value,user):
  pc=g.reg_read(UC_X86_REG_EIP)
  if self.heap_enabled and pc in self.heap_addresses:self.heap_writes.append(dict(order=self.heap_order,instruction=f'{pc:08x}',address=f'{p:08x}',bytes=n,before=self.raw(p,n),after=(value&((1<<(8*n))-1)).to_bytes(n,'little').hex()));self.heap_order+=1
 def heap_finish(self,address):
  sp=self.g.reg_read(UC_X86_REG_ESP)
  for ret,esp,row in reversed(self.heap_pending[:]):
   if address==ret and sp>esp:
    row.update(after=self.heap_snapshot(row['heap']),return_cpu=self.cpu(),return_order=self.heap_order);self.heap_order+=1;self.heap_pending.remove((ret,esp,row))
 def hook(self,g,address,size,user):
  if address==0xb76b0:self.heap_enabled=True
  if self.heap_enabled:
   self.heap_finish(address);sp=g.reg_read(UC_X86_REG_ESP);ret=self.u(sp);callback=ret in self.heap_sites and self.heap_sites[ret][1]==address
   if address in OWN or callback:
    heap=self.heap_pointer(address,sp)
    if address==0x18fc80:self.heap_extents[heap]=(g.reg_read(UC_X86_REG_EBX)+0x53)&0xfffffffc
    row=dict(id=len(self.heap_calls),entry=f'{address:08x}',caller=f'{ret:08x}',phase=self.active,heap=heap,entry_cpu=self.cpu(),stack=[f'{self.u(sp+4+i*4):08x}'for i in range(6)],ancestors=[x[2]['id']for x in self.heap_pending],order=self.heap_order)
    row['before']=self.heap_snapshot(heap);self.heap_calls.append(row);self.heap_pending.append((ret,sp,row));self.heap_order+=1
  super().hook(g,address,size,user)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
 # The producer output directory must exist before the subclass is constructed.
 class Capture(Trace):
  def __init__(self,*args,**kw):super().__init__(*args,**kw);self.heap_output=out
 with(out/'host-provider.stderr.log').open('w')as err:
  process=subprocess.Popen(['wine',str(prior.prior.owning.CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=err,text=True,env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(prior.prior.owning.CONTEXT_OUT/'wine-prefix')))
  try:
   data=json.loads(process.stdout.readline());t=prior.prior.owning.prepare(data,trace_type=Capture);assert prior.prior.owning.spawn_and_activate(t,activate=False);t.heap_finish(prior.prior.owning.STOP);assert not t.heap_pending
   listings=[]
   for e,f in t.heap_routines.items():
    ins=[t.code.at(p)for p in sorted(f.code)];path=out/'instructions'/f'{e:08x}.asm';path.parent.mkdir(exist_ok=True)
    for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes==bytes(t.g.mem_read(i.address,len(i.bytes)))
    path.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins));listings.append(dict(entry=f'{e:08x}',rows=len(ins),sha256=prior.prior.sha(path)))
   prior.prior.save(out/'report.json',dict(schema='original-object-body-heap-owning-v1',tool_sha256=prior.prior.sha(__file__),calls=t.heap_calls,reads=t.heap_reads,stores=t.heap_writes,storage_states=t.heap_states,listings=listings,source_stages=t.player_stages,instruction_patches=[],supplied_allocator_or_growth_results=[],native_unit_body_copies=[],scope='Actual full B76B0-owned125520/18FC80 heap producer followed by unchanged original join/spawn/body growth. Whole compatibility/platform/game-arena/resource prefix boundaries remain inherited and explicit; observed raw initial bytes are not asserted Xbox defaults.'))
   print(json.dumps(dict(report_sha256=prior.prior.sha(out/'report.json'),calls=len(t.heap_calls),stores=len(t.heap_writes),full_storage_states=len(t.heap_states))),flush=True)
  finally:
   if process.poll()is None:process.stdin.write('x');process.stdin.flush();process.stdin.close();process.wait(timeout=60)
   if process.stdout:process.stdout.close()
   assert process.returncode==0
if __name__=='__main__':main()
