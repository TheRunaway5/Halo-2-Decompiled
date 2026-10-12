#!/usr/bin/env python3
"""Observe the unchanged whole generic constructor and genuine allocator pressure."""
import argparse,json,os,struct,subprocess
from pathlib import Path
import unicorn
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS,UC_X86_REG_XMM0,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG,UC_X86_REG_MXCSR)
import oracle_mp_object_type_construction_1_1 as prior
from functions import _trace
ROOT=prior.ROOT
ENTRY=0xb8a30
class GenericTrace(prior.TypeTrace):
 def __init__(self,*a,**k):
  self.generic_enabled=False;self.generic_calls=[];self.generic_pending=[];self.generic_order=0;self.generic_stores=[];self.generic_reads=[];self.generic_tags={};self.generic_executed=set();super().__init__(*a,**k)
  self.generic_routine=_trace(self.code,ENTRY,self.starts|{ENTRY});self.generic_addresses=set(self.generic_routine.code)
  self.generic_sites={}
  for p in self.generic_addresses:
   i=self.code.at(p)
   if i.mnemonic=='call'and i.operands[0].type==2:self.generic_sites[p+i.size]=(p,i.operands[0].imm)
  self.g.hook_add(unicorn.UC_HOOK_MEM_WRITE,self.generic_write);self.g.hook_add(unicorn.UC_HOOK_MEM_READ,self.generic_read)
 def cpu(self):
  g=self.g;return dict(registers=self.registers(),esp=f'{g.reg_read(UC_X86_REG_ESP):08x}',eflags=f'{g.reg_read(UC_X86_REG_EFLAGS):08x}',xmm128=[f'{g.reg_read(UC_X86_REG_XMM0+i):032x}'for i in range(8)],fpcw=f'{g.reg_read(UC_X86_REG_FPCW):04x}',fpsw=f'{g.reg_read(UC_X86_REG_FPSW):04x}',fptag=f'{g.reg_read(UC_X86_REG_FPTAG):04x}',mxcsr=f'{g.reg_read(UC_X86_REG_MXCSR):08x}')
 def generic_buffers(self,row):
  result=self.buffers(row)
  if 'scratch'in row:result['S']=self.raw(row['scratch'],0xc8)
  return result
 def generic_label(self,p,n,row):
  pool=self.u(0x4e78d0);heads=self.u(pool+0x44);cap=self.u(pool+0x20);world=self.u(0x4e58c4)
  for name,base,size in [('P',pool,0x4c),('K',heads,cap*12),('W',world,0x84),('D',row['descriptor'],0xc4),('S',row['scratch'],0xc8)]:
   if base<=p and p+n<=base+size:return name,p-base
  for index in range(self.u(pool+0x38)):
   h=heads+index*12;b=self.u(h+8);size=self.u(h+6,2)
   if b and b<=p and p+n<=b+size:return f'U{index}',p-b
  return None
 def generic_write(self,g,access,p,n,value,user):
  ip=g.reg_read(UC_X86_REG_EIP)
  if not self.generic_enabled or ip not in self.generic_addresses:return
  owner=next((q[2]for q in reversed(self.generic_pending) if q[2]['entry']==f'{ENTRY:08x}'),None)
  if owner is None:return
  label=self.generic_label(p,n,owner);row=dict(order=self.generic_order,instruction=f'{ip:08x}',address=f'{p:08x}',bytes=n,before=self.raw(p,n),value_hex=(value&((1<<(n*8))-1)).to_bytes(n,'little').hex(),owner=owner['id'])
  self.generic_order+=1
  if label:row.update(buffer=label[0],offset=label[1])
  else:row['unclassified_retained_or_stack_write']=True
  self.generic_stores.append(row)
 def generic_read(self,g,access,p,n,value,user):
  ip=g.reg_read(UC_X86_REG_EIP)
  if not self.generic_enabled or ip not in self.generic_addresses:return
  owner=next((q[2]for q in reversed(self.generic_pending) if q[2]['entry']==f'{ENTRY:08x}'),None)
  if owner is None:return
  row=dict(order=self.generic_order,instruction=f'{ip:08x}',address=f'{p:08x}',bytes=n,raw_hex=self.raw(p,n),owner=owner['id']);self.generic_order+=1
  label=self.generic_label(p,n,owner)
  if label:row.update(buffer=label[0],offset=label[1])
  else:
   tags=[t for t in self.cache.tags.values()if t['offset']is not None and t['offset']+self.cache.magic<=p and p+n<=t['offset']+self.cache.magic+t['size']]
   if len(tags)==1:
    tag=tags[0];datum=int(tag['datum'],16);at=self.u(self.u(0x4eb114)+(datum&65535)*16+8);key=f'{datum:08x}'
    self.generic_tags.setdefault(key,dict(**tag,address=f'{at:08x}',raw_hex=self.raw(at,tag['size'])))
    row['tag']=key
  self.generic_reads.append(row)
 def generic_finish(self,a):
  sp=self.g.reg_read(UC_X86_REG_ESP)
  for q in reversed(self.generic_pending[:]):
   ret,esp,row=q
   if ret==a and sp>esp:
    row.update(after=self.generic_buffers(row),return_cpu=self.cpu(),return_order=self.generic_order);self.generic_order+=1;self.generic_pending.remove(q)
 def hook(self,g,a,size,user):
  if self.generic_enabled:
   self.generic_finish(a)
   if a in self.generic_addresses:self.generic_executed.add(a)
   sp=g.reg_read(UC_X86_REG_ESP);ret=self.u(sp);callback=ret in self.generic_sites and self.generic_sites[ret][1]==a and not any(q[0]==ret and q[1]==sp for q in self.generic_pending)
   if a==ENTRY or callback:
    row=dict(id=len(self.generic_calls),entry=f'{a:08x}',caller=f'{ret:08x}',phase=self.active,order=self.generic_order,entry_cpu=self.cpu(),stack=[f'{self.u(sp+4+i*4):08x}'for i in range(8)],ancestors=[q[2]['id']for q in self.generic_pending]);self.generic_order+=1
    if a==ENTRY:row.update(descriptor=self.u(sp+4),scratch=sp-0xc8)
    else:
     parent=next(q[2]for q in reversed(self.generic_pending)if q[2]['entry']==f'{ENTRY:08x}');row.update(descriptor=parent['descriptor'],scratch=parent['scratch'],call_site=f'{self.generic_sites[ret][0]:08x}')
    row['before']=self.generic_buffers(row);self.generic_calls.append(row);self.generic_pending.append((ret,sp,row))
  super().hook(g,a,size,user)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--allocator-pressure',action='store_true');a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
 with(out/'host-provider.stderr.log').open('w')as err:
  process=subprocess.Popen(['wine',str(prior.owning.CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=err,text=True,env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(prior.owning.CONTEXT_OUT/'wine-prefix')))
  try:
   data=json.loads(process.stdout.readline());t=prior.owning.prepare(data,trace_type=GenericTrace);t.generic_enabled=True
   assert prior.owning.spawn_and_activate(t,activate=False);t.generic_finish(prior.owning.STOP);assert not t.generic_pending
   before_pressure=t.state();baseline=json.loads((ROOT/'analysis/multiplayer/integration/spawn-source/player-visibility/report.json').read_text());assert t.player_stages==baseline['cases'][2]['stages'][:4]
   pressure=[]
   if a.allocator_pressure:
    constructor=next(q for q in t.generic_calls if q['entry']==f'{ENTRY:08x}')
    scratch=0x6e001000;t.g.mem_map(scratch,0x1000);t.g.mem_write(scratch,bytes.fromhex(constructor['before']['D']))
    # Genuine complete BD170 uses source-owned finite header/body heap. No
    # capacity or allocator result is changed. Raw storage is not a live Unit.
    t.active='explicit_original_body_allocation_pressure';t.track_players=False;t.generic_enabled=False
    for index in range(2049):
     t.run(0xbd170,(0x464,));datum=t.g.reg_read(UC_X86_REG_EAX);pressure.append(f'{datum:08x}')
     if datum==0xffffffff:break
    else:raise AssertionError('Actual bounded pool did not reach a failure')
    t.generic_enabled=True;t.active='original_constructor_after_real_allocator_pressure';t.run(ENTRY,(scratch,));t.generic_finish(prior.owning.STOP);assert not t.generic_pending
    assert t.g.reg_read(UC_X86_REG_EAX)==0xffffffff
    # Release one genuinely allocated raw body through complete BD1F0. The
    # unchanged finite heap then admits a base body but cannot admit all of
    # the biped's extension requests. No allocator result is intercepted.
    t.generic_enabled=False;t.active='explicit_original_pressure_body_release';t.run(0xbd1f0,(int(pressure[-2],16),))
    t.generic_enabled=True;t.active='original_constructor_after_one_real_body_release';t.run(ENTRY,(scratch,));t.generic_finish(prior.owning.STOP);assert not t.generic_pending
    assert t.g.reg_read(UC_X86_REG_EAX)==0xffffffff
   listings=[]
   for name,addresses in [('complete-generic-constructor',t.generic_addresses),('executed-generic-constructor',t.generic_executed)]:
    path=out/(name+'.asm');ins=[t.code.at(q)for q in sorted(addresses)]
    for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes and bytes(t.g.mem_read(i.address,len(i.bytes)))==i.bytes
    path.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins));listings.append(dict(path=path.name,sha256=prior.sha(path),rows=len(ins)))
   prior.save(out/'report.json',dict(schema='original-generic-object-constructor-v1',tool_sha256=prior.sha(__file__),dependency_sha256=prior.sha(ROOT/'tools/oracle_mp_object_type_construction_1_1.py'),calls=t.generic_calls,stores=t.generic_stores,reads=t.generic_reads,tags=t.generic_tags,source_stages=t.player_stages,baseline=before_pressure,allocator_pressure_datums=pressure,after=t.generic_buffers(dict(descriptor=scratch,scratch=constructor['scratch']))if a.allocator_pressure else t.state(),listings=listings,instruction_patches=[],new_success_returns=[],new_unit_fields=[],boundaries=['Actual unchanged source construction in the immutable PlayerTrace context. Platform/options/resource/metadata boundaries remain inherited.','Optional allocator pressure executes complete BD170 until real bounded storage fails. No source capacity/result is changed; these allocated raw bodies are not constructed/live Units.','Private original stack scratch is observed, not supplied gameplay state. No final body/header/action/component copy is a live producer.']))
   print(json.dumps(dict(report=str(out/'report.json'),sha256=prior.sha(out/'report.json'),calls=len(t.generic_calls),stores=len(t.generic_stores),pressure=len(pressure))),flush=True)
  finally:
   if process.poll()is None:process.stdin.write('x');process.stdin.flush();process.stdin.close();process.wait(timeout=60)
   if process.stdout:process.stdout.close()
   assert process.returncode==0,process.returncode
if __name__=='__main__':main()
