#!/usr/bin/env python3
"""Full authenticated scheduler/retained contact delivery body correspondence."""
import argparse,json,sys,struct
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from capstone.x86 import X86_OP_IMM,X86_OP_MEM
import research_mp_sandbox_update_functions as base
from oracle_mp_power_agent_scheduler_1_1 import ROOT,OUT,verify,sha
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
ENTRIES={0x317650,0x316200,0x316440,0x31e2c0,0x31db70,0x31dee0,0x31e190,
 0x31d5f0,0x31d690,0x31f550,0x31e9d0,0x31c3c0,0x27f0f0,0x27f110,0x148800,
 0x27e3a0,0x1d4160,0x27e760,0x1c5ff0,0x31c980,0x31ca20,0x300870,0x2e68d0,
 0x1d2c70,0x1d2cf0,0x1d2f10,0x2ded30,0x2ffe20}
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'final/later-complete/candidates.json');a=ap.parse_args();verify()
 base.ENTRIES=tuple(sorted(ENTRIES));sys.argv=[sys.argv[0],'--output',str(a.output)];base.main();r=json.loads(a.output.read_text())
 old=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');new=Xbe(base.NEW_DEFAULT);oc,nc=_Code(old,'.text'),_Code(new,'.text')
 mapped={int(q['disc_1_1_entry'],16):int(q['later_entry'],16)for q in r['routines']if q.get('normalized_instruction_and_internal_branch_topology_equal')}
 starts=set(mapped.values());md=Cs(CS_ARCH_X86,CS_MODE_32);md.detail=True;md.skipdata=True;sec=new.section('.text')
 for i in md.disasm(new.section_bytes(sec),sec.va):
  if i.mnemonic=='call'and len(i.operands)==1 and i.operands[0].type==X86_OP_IMM:starts.add(i.operands[0].imm)
 def ins(c,e,st):return[c.at(p)for p in sorted(_trace(c,e,st).code)]
 anchors=[]
 # Actual constructor1C5FF0 writes the BSP contact listener vtable. Its
 # virtual+8 is the observed complete RET4, avoiding ambiguous leaf search.
 owner=0x1c5ff0;target=mapped[owner];oi,ni=ins(oc,owner,ENTRIES),ins(nc,target,starts)
 assert base.full_tokens(oi,old)==base.full_tokens(ni,new)
 stores=[(x,y)for x,y in zip(oi,ni)if x.mnemonic=='mov'and len(x.operands)==2 and x.operands[1].type==X86_OP_IMM and x.operands[1].imm==0x461b9c]
 assert len(stores)==1;x,y=stores[0];nv=y.operands[1].imm
 assert struct.unpack('<I',old.read(0x461b9c+8,4))[0]==0x27e760
 mapped[0x27e760]=struct.unpack('<I',new.read(nv+8,4))[0]
 anchors.append(dict(old_owner=f'{owner:08x}',later_owner=f'{target:08x}',old_store=f'{x.address:08x}',later_store=f'{y.address:08x}',old_vtable='00461b9c',later_vtable=f'{nv:08x}',slot=8,old_target='0027e760',later_target=f'{mapped[0x27e760]:08x}'))
 # Two paired image operands hold relocated function pointers. Follow
 # their actual old/new words, then check the complete pointed-to bodies.
 for owner,address in ((0x1c5ff0,0x469290),(0x300870,0x4693ac)):
  oi,ni=ins(oc,owner,ENTRIES),ins(nc,mapped[owner],starts)
  matches=[(x,y,xo,yo)for x,y in zip(oi,ni)for xo,yo in zip(x.operands,y.operands)if xo.type==yo.type==X86_OP_MEM and not xo.mem.base and xo.mem.disp==address]
  assert len(matches)==1;x,y,xo,yo=matches[0]
  left=struct.unpack('<I',old.read(xo.mem.disp,4))[0];right=struct.unpack('<I',new.read(yo.mem.disp,4))[0]
  assert left in ENTRIES;mapped[left]=right
  anchors.append(dict(kind='paired_original_function_pointer_operand',old_owner=f'{owner:08x}',later_owner=f'{mapped[owner]:08x}',old_site=f'{x.address:08x}',later_site=f'{y.address:08x}',old_operand=f'{xo.mem.disp:08x}',later_operand=f'{yo.mem.disp:08x}',old_target=f'{left:08x}',later_target=f'{right:08x}'))
 while True:
  before=len(mapped)
  for p,b in list(mapped.items()):
   oi,ni=ins(oc,p,ENTRIES),ins(nc,b,starts)
   assert base.full_tokens(oi,old)==base.full_tokens(ni,new),(hex(p),hex(b))
   for x,y in zip(oi,ni):
    if x.mnemonic in('call','jmp')and x.operands[0].type==y.operands[0].type==X86_OP_IMM and x.operands[0].imm in ENTRIES:
     left,right=x.operands[0].imm,y.operands[0].imm;assert left not in mapped or mapped[left]==right;mapped[left]=right
  if len(mapped)==before:break
 for row in r['routines']:
  p=int(row['disc_1_1_entry'],16)
  if p in mapped:continue
  oi=ins(oc,p,ENTRIES);valid=[]
  for value in row['candidates']:
   b=int(value,16);ni=ins(nc,b,starts)
   if base.full_tokens(oi,old)!=base.full_tokens(ni,new):continue
   pairs=[(x,y)for x,y in zip(oi,ni)if x.mnemonic=='call'and x.operands[0].type==y.operands[0].type==X86_OP_IMM]
   if all(mapped.get(x.operands[0].imm,y.operands[0].imm)==y.operands[0].imm for x,y in pairs):valid.append(b)
  assert len(valid)==1,(hex(p),[hex(x)for x in valid]);mapped[p]=valid[0]
 assert ENTRIES==mapped.keys()
 for q in r['routines']:
  p=int(q['disc_1_1_entry'],16);b=mapped[p];oi,ni=ins(oc,p,ENTRIES),ins(nc,b,starts)
  assert base.full_tokens(oi,old)==base.full_tokens(ni,new)
  refs=[];calls=[]
  for x,y in zip(oi,ni):
   for xo,yo in zip(x.operands,y.operands):
    if xo.type==yo.type==X86_OP_MEM and not xo.mem.base and not yo.mem.base and base.containing(old,xo.mem.disp)and base.containing(new,yo.mem.disp):
     width=xo.size;assert width==yo.size;before,after=old.read(xo.mem.disp,width),new.read(yo.mem.disp,width)
     refs.append(dict(old_site=f'{x.address:08x}',later_site=f'{y.address:08x}',old_address=f'{xo.mem.disp:08x}',later_address=f'{yo.mem.disp:08x}',width=width,old_hex=before.hex(),later_hex=after.hex(),bytes_equal=before==after))
   if x.mnemonic=='call'and x.operands[0].type==y.operands[0].type==X86_OP_IMM:
    left,right=x.operands[0].imm,y.operands[0].imm;calls.append(dict(old_site=f'{x.address:08x}',later_site=f'{y.address:08x}',old_target=f'{left:08x}',later_target=f'{right:08x}',mapped_target_matches=mapped.get(left)==right if left in mapped else None))
  q.update(later_entry=f'{b:08x}',old_instructions=len(oi),later_instructions=len(ni),normalized_instruction_and_internal_branch_topology_equal=True,paired_image_memory_operands=refs,paired_direct_calls=calls,all_paired_memory_bytes_equal=all(v['bytes_equal']for v in refs))
  for label,seq,im,entry in(('disc',oi,old,p),('later',ni,new,b)):
   dst=a.output.parent/'instructions'/f'{p:08x}-{label}-{entry:08x}.asm'
   for i in seq:assert im.read(i.address,len(i.bytes))==i.bytes
   frozen_write(dst,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in seq).encode());q[label+'_listing']=dict(file=dst.name,sha256=sha(dst))
 r.update(schema=2,correspondence_tool_sha256=sha(__file__),complete_routine_count=len(mapped),virtual_initializer_anchors=anchors,later_gameplay_executed=False,source_report_sha256=sha(OUT/'final/measured/report.json'))
 dst=a.output.parent/'complete-report.json';frozen_write(dst,encoded(r));print(json.dumps(dict(report=sha(dst),routines=len(mapped))))
if __name__=='__main__':main()
