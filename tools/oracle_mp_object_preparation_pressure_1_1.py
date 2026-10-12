#!/usr/bin/env python3
"""Unchanged biped request preparation and heap-pressure owning paths."""
import argparse,json,os,struct,subprocess
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_MXCSR
import oracle_mp_generic_object_constructor_1_1 as prior
from functions import _trace
ROOT=prior.ROOT
OWN=(0xddac0,0x148360,0x148740,0x148510,0x1482f0,0x22ecb0)
class Trace(prior.GenericTrace):
 def __init__(self,*a,**k):
  self.prep_enabled=False;self.prep_calls=[];self.prep_pending=[];self.prep_order=0;self.prep_reads=[];self.prep_writes=[];super().__init__(*a,**k)
  self.prep_routines={e:_trace(self.code,e,self.starts|set(OWN))for e in OWN};self.prep_addresses={a for f in self.prep_routines.values()for a in f.code};self.prep_sites={}
  for a in self.prep_addresses:
   i=self.code.at(a)
   if i.mnemonic=='call'and i.operands[0].type==2:self.prep_sites[a+i.size]=(a,i.operands[0].imm)
  self.g.hook_add(unicorn.UC_HOOK_MEM_READ,self.prep_read);self.g.hook_add(unicorn.UC_HOOK_MEM_WRITE,self.prep_write)
 def snapshot(self,row):
  r=self.buffers(row);r['globals']={f'{p:08x}':self.raw(p,n)for p,n in((0x486562,2),(0x480d8c,4),(0x480da0,12),(0x518444,4))}
  r['heap_fields']={}
  for slot,offset in((0x480da0,0xf0340),(0x480da4,0x20),(0x480da8,0x20)):
   p=self.u(slot)
   if p:r['heap_fields'][f'{p+offset:08x}']=self.raw(p+offset,4)
  clock=self.u(0x518444);r['clock']=dict(address=f'{clock:08x}',raw12=self.raw(clock,12))
  if row.get('descriptor'):
   definition=self.u(row['descriptor']);tag=self.u(self.u(0x4eb114)+(definition&65535)*16+8)
   r['definition']=dict(datum=f'{definition:08x}',address=f'{tag:08x}',flags264=self.raw(tag+0x264,4),scalar270=self.raw(tag+0x270,4))
  return r
 def prep_read(self,g,access,p,n,value,user):
  ip=g.reg_read(UC_X86_REG_EIP)
  if self.prep_enabled and ip in self.prep_addresses:self.prep_reads.append(dict(order=self.prep_order,instruction=f'{ip:08x}',address=f'{p:08x}',bytes=n,raw_hex=self.raw(p,n)));self.prep_order+=1
 def prep_write(self,g,access,p,n,value,user):
  ip=g.reg_read(UC_X86_REG_EIP)
  if self.prep_enabled and ip in self.prep_addresses:self.prep_writes.append(dict(order=self.prep_order,instruction=f'{ip:08x}',address=f'{p:08x}',bytes=n,before=self.raw(p,n),after=(value&((1<<(8*n))-1)).to_bytes(n,'little').hex()));self.prep_order+=1
 def finish(self,a):
  sp=self.g.reg_read(UC_X86_REG_ESP)
  for ret,esp,row in reversed(self.prep_pending[:]):
   if ret==a and sp>esp:
    row.update(after=self.snapshot(row),return_cpu=self.cpu(),return_order=self.prep_order);self.prep_order+=1;self.prep_pending.remove((ret,esp,row))
 def hook(self,g,a,size,user):
  if self.prep_enabled:
   self.finish(a);sp=g.reg_read(UC_X86_REG_ESP);ret=self.u(sp);callback=ret in self.prep_sites and self.prep_sites[ret][1]==a
   if a in OWN or callback:
    row=dict(id=len(self.prep_calls),entry=f'{a:08x}',caller=f'{ret:08x}',phase=self.active,entry_cpu=self.cpu(),stack=[f'{self.u(sp+4+i*4):08x}'for i in range(5)],ancestors=[q[2]['id']for q in self.prep_pending],order=self.prep_order)
    if a==0xddac0:row['descriptor']=self.u(sp+4)
    elif self.prep_pending and self.prep_pending[-1][2].get('descriptor'):row['descriptor']=self.prep_pending[-1][2]['descriptor']
    row['before']=self.snapshot(row);self.prep_calls.append(row);self.prep_pending.append((ret,sp,row));self.prep_order+=1
  super().hook(g,a,size,user)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
 with(out/'host-provider.stderr.log').open('w')as err:
  process=subprocess.Popen(['wine',str(prior.prior.owning.CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=err,text=True,env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(prior.prior.owning.CONTEXT_OUT/'wine-prefix')))
  try:
   data=json.loads(process.stdout.readline());t=prior.prior.owning.prepare(data,trace_type=Trace);t.prep_enabled=True;assert prior.prior.owning.spawn_and_activate(t,activate=False);t.finish(prior.prior.owning.STOP);assert not t.prep_pending
   baseline=t.state();natural=next(q for q in t.prep_calls if q['entry']=='000ddac0');descriptor=0x6e010000;t.g.mem_map(descriptor,0x1000);raw=bytes.fromhex(natural['before']['D']);definition=natural['before']['definition'];tag=int(definition['address'],16);flags=t.u(tag+0x264);scalar=t.raw(tag+0x270,4)
   controls=[]
   for flag in(flags,flags|1,(flags|1)|16):
    for mxcsr in(0x1f80,0x3f80,0x5f80,0x7f80,0x1fc0):
     t.g.mem_write(descriptor,raw);t.g.mem_write(tag+0x264,struct.pack('<I',flag));t.g.reg_write(UC_X86_REG_MXCSR,mxcsr);t.active=f'explicit_prep_flags_{flag:08x}_mxcsr_{mxcsr:08x}';first=len(t.prep_calls)
     t.run(0xddac0,(descriptor,));t.finish(prior.prior.owning.STOP);assert not t.prep_pending;controls.append(dict(call=first,flag=f'{flag:08x}',mxcsr=f'{mxcsr:08x}',boundary='Only declared raw flag word and CPU rounding/DAZ context varied; request/scalar come from actual source preparation. No live Unit/root is replaced.'))
   t.g.mem_write(tag+0x264,struct.pack('<I',flags));assert t.raw(tag+0x270,4)==scalar;assert t.state()==baseline
   listings=[]
   for e,f in t.prep_routines.items():
    ins=[t.code.at(a)for a in sorted(f.code)];path=out/'instructions'/f'{e:08x}.asm';path.parent.mkdir(exist_ok=True)
    for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes and bytes(t.g.mem_read(i.address,len(i.bytes)))==i.bytes
    path.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins));listings.append(dict(entry=f'{e:08x}',rows=len(ins),sha256=prior.prior.sha(path)))
   prior.prior.save(out/'report.json',dict(schema='source-object-preparation-pressure-v1',tool_sha256=prior.prior.sha(__file__),dependency_sha256=prior.prior.sha(ROOT/'tools/oracle_mp_generic_object_constructor_1_1.py'),calls=t.prep_calls,reads=t.prep_reads,stores=t.prep_writes,controls=controls,listings=listings,source_stages=t.player_stages,instruction_patches=[],new_success_returns=[],live_unit_field_assignments=[],scope='Natural unchanged request preparation and pressure callbacks; explicit separate DDAC0 flag/CPU controls never replace a live Unit. Actual platform/world/resource producers retain all earlier boundaries.'))
   print(json.dumps(dict(report=str(out/'report.json'),sha256=prior.prior.sha(out/'report.json'),calls=len(t.prep_calls),stores=len(t.prep_writes),controls=len(controls))),flush=True)
  finally:
   if process.poll()is None:process.stdin.write('x');process.stdin.flush();process.stdin.close();process.wait(timeout=60)
   if process.stdout:process.stdout.close()
   assert process.returncode==0,process.returncode
if __name__=='__main__':main()
