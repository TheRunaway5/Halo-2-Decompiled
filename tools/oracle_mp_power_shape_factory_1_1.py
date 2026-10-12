#!/usr/bin/env python3
"""Observe full selected original physical-model initializer without new inputs."""
import argparse,json
from pathlib import Path
import unicorn
from unicorn import x86_const as reg
from oracle_mp_power_body_factory_1_1 import FactoryTrace,ROOT,OUT,OWNER,sha,verify
from oracle_mp_power_body_bridge_1_1 import power
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from oracle_smg_firing_1_1 import STACK
from functions import _trace
ENTRIES={0x1ee380,0x1ee8f0}
class ShapeTrace(FactoryTrace):
 def __init__(self,data):
  self.shape_call=None;self.shape_pending=None;self.shape_reads=[];self.shape_writes=[];self.shape_order=0
  super().__init__(data);self.g.hook_add(unicorn.UC_HOOK_MEM_READ,self.shape_read)
 def hook(self,g,a,size,user):
  if self.shape_pending==a:
   c=self.shape_call;c.update(return_order=self.shape_order,returned_eax=f'{g.reg_read(reg.UC_X86_REG_EAX):08x}',
    raw_after_hex=bytes(g.mem_read(int(c['model_pointer'],16),c['model_size'])).hex())
   self.shape_order+=1;self.shape_pending=None
  if a==0x1ee380:
   assert self.shape_call is None
   sp=g.reg_read(reg.UC_X86_REG_ESP);datum=g.reg_read(reg.UC_X86_REG_EAX);p=self.u(self.u(0x4eb114)+(datum&0xffff)*16+8)
   tag=self.cache.tags[datum];n=tag['size']
   self.shape_call=dict(entry='001ee380',caller=f'{self.u(sp):08x}',registers=self.registers(),stack16=[f'{self.u(sp+4+i*4):08x}'for i in range(16)],
    x87_control_word=f'{g.reg_read(reg.UC_X86_REG_FPCW):04x}',mxcsr=f'{g.reg_read(reg.UC_X86_REG_MXCSR):08x}',model_pointer=f'{p:08x}',model_size=n,raw_before_hex=bytes(g.mem_read(p,n)).hex(),tag=tag,order=self.shape_order)
   self.shape_order+=1;self.shape_pending=self.u(sp)
  super().hook(g,a,size,user)
 def shape_read(self,g,access,address,size,value,user):
  if not self.shape_pending:return
  self.shape_reads.append(dict(order=self.shape_order,instruction=f'{g.reg_read(reg.UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,raw_hex=bytes(g.mem_read(address,size)).hex()));self.shape_order+=1
 def mem_write(self,g,access,address,size,value,user):
  if self.shape_pending:
   self.shape_writes.append(dict(order=self.shape_order,instruction=f'{g.reg_read(reg.UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,before_hex=bytes(g.mem_read(address,size)).hex(),written_hex=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex(),source_stack=STACK-0x100000<=address<STACK+0x100000));self.shape_order+=1
  super().mem_write(g,access,address,size,value,user)
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'shape-measured/report.json');args=ap.parse_args();verify()
 data=json.loads((OWNER/'raw-host-context.json').read_text());t=power.player.prepare(data,trace_type=ShapeTrace);t.selected_physics_fixup(0xe6f80583)
 old=json.loads((OWNER/'report.json').read_text())['selected_original_physics_postprocess'];assert t.selected_fixup==old
 assert t.shape_call and t.shape_pending==t.g.reg_read(reg.UC_X86_REG_EIP)==0x11ded1
 c=t.shape_call;c.update(return_order=t.shape_order,returned_eax=f'{t.g.reg_read(reg.UC_X86_REG_EAX):08x}',raw_after_hex=bytes(t.g.mem_read(int(c['model_pointer'],16),c['model_size'])).hex(),return_destination_is_exclusive_parent_fragment_stop=True)
 t.shape_order+=1;t.shape_pending=None
 args.output.parent.mkdir(parents=True,exist_ok=True);listing=[]
 for a in sorted(ENTRIES):
  f=_trace(t.code,a,t.starts|ENTRIES);ins=[t.code.at(p)for p in sorted(f.code)]
  for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes==bytes(t.g.mem_read(i.address,len(i.bytes)))
  p=args.output.parent/f'{a:08x}.asm';frozen_write(p,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode());listing.append(dict(entry=f'{a:08x}',file=p.name,rows=len(ins),sha256=sha(p)))
 r=dict(schema=1,oracle_sha256=sha(__file__),factory_tool_sha256=sha(ROOT/'tools/oracle_mp_power_body_factory_1_1.py'),prior_manifest_sha256='9b54f4cb04528cfe41057994641651cfacd76ede4f2d1535980db50e68034fa4',selected_initializer_and_all_prior_records_identical=True,call=t.shape_call,reads=t.shape_reads,stores=t.shape_writes,listings=listing,selected_fixup=t.selected_fixup,new_source_fields_or_results_supplied=[],instruction_patches=[],boundaries=['Selected actual phmo tag iterator and original map postprocessor, not complete all-tag/map initialization.','Resident convex vertices/planes and shape headers are source owned. Its local AABB is not a synthetic box contact collider.'])
 verify();frozen_write(args.output,encoded(r));print(json.dumps(dict(report=sha(args.output),reads=len(t.shape_reads),writes=len(t.shape_writes))))
if __name__=='__main__':main()
