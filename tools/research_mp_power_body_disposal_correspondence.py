#!/usr/bin/env python3
"""Full authenticated old/new disposal owners; original instructions only."""
import argparse,json,sys
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from capstone.x86 import X86_OP_IMM
import research_mp_sandbox_update_functions as base
from oracle_mp_power_body_disposal_1_1 import ROOT,OUT,verify,sha
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
ENTRIES={0x1d3bb0,0x318900,0x318a90,0x317ad0,0x317970,0x31b250,
 0x3126e0,0x1d53c0,0x2e00f0,0x3124b0,0x31c1c0,0x311910,0xdd970,
 0x27f9f0,0x148a70,0x22eb20,0x2df780,0x2dd7f0,0x320d20,0x320d50,0x1d5210}
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'final/later-correspondence/report.json');a=ap.parse_args();verify();base.ENTRIES=tuple(sorted(ENTRIES));sys.argv=[sys.argv[0],'--output',str(a.output)];base.main();r=json.loads(a.output.read_text())
 old=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');new=Xbe(base.NEW_DEFAULT);oc,nc=_Code(old,'.text'),_Code(new,'.text')
 mapped={int(q['disc_1_1_entry'],16):int(q['later_entry'],16)for q in r['routines']if q.get('normalized_instruction_and_internal_branch_topology_equal')}
 # Actual paired constructor vtable stores own the virtual destructor slots;
 # normalized prefix candidates alone cannot disambiguate these wrappers.
 anchors=[]
 def virtual(old_owner,new_owner,old_vtable,slot,old_target):
  oi=[oc.at(p)for p in sorted(_trace(oc,old_owner,ENTRIES).code)]
  ni=[nc.at(p)for p in sorted(_trace(nc,new_owner,set(mapped.values())).code)]
  assert base.full_tokens(oi,old)==base.full_tokens(ni,new)
  rows=[(x,y)for x,y in zip(oi,ni)if x.mnemonic=='mov'and len(x.operands)==2 and x.operands[1].type==X86_OP_IMM and x.operands[1].imm==old_vtable]
  assert len(rows)==1
  x,y=rows[0];new_vtable=y.operands[1].imm
  import struct
  assert struct.unpack('<I',old.read(old_vtable+slot,4))[0]==old_target
  target=struct.unpack('<I',new.read(new_vtable+slot,4))[0];mapped[old_target]=target
  anchors.append(dict(old_owner=f'{old_owner:08x}',later_owner=f'{new_owner:08x}',old_store=f'{x.address:08x}',later_store=f'{y.address:08x}',old_vtable=f'{old_vtable:08x}',later_vtable=f'{new_vtable:08x}',slot=slot,old_target=f'{old_target:08x}',later_target=f'{target:08x}'))
 virtual(0x317970,mapped[0x317970],0x41a950,0,0x317ad0)
 virtual(0x320cc0,0x324810,0x41b7f0,0,0x320d20)
 virtual(0x320cc0,0x324810,0x41b7f0,0x18,0x31b250)
 virtual(0x1d5210,mapped[0x1d5210],0x45cad0,0,0x1d53c0)
 starts=set(mapped.values());md=Cs(CS_ARCH_X86,CS_MODE_32);md.detail=True;md.skipdata=True;sec=new.section('.text')
 for i in md.disasm(new.section_bytes(sec),sec.va):
  if i.mnemonic=='call'and len(i.operands)==1 and i.operands[0].type==X86_OP_IMM:starts.add(i.operands[0].imm)
 while True:
  before=len(mapped)
  for oldentry,newentry in list(mapped.items()):
   oi=[oc.at(p)for p in sorted(_trace(oc,oldentry,ENTRIES).code)];ni=[nc.at(p)for p in sorted(_trace(nc,newentry,starts).code)]
   assert base.full_tokens(oi,old)==base.full_tokens(ni,new),(hex(oldentry),hex(newentry))
   for x,y in zip(oi,ni):
    if x.mnemonic in('call','jmp')and x.operands[0].type==y.operands[0].type==X86_OP_IMM and x.operands[0].imm in ENTRIES:
     p,q=x.operands[0].imm,y.operands[0].imm;assert p not in mapped or mapped[p]==q;mapped[p]=q
  if len(mapped)==before:break
 assert ENTRIES==mapped.keys(),[hex(p)for p in ENTRIES-mapped.keys()]
 for q in r['routines']:
  p=int(q['disc_1_1_entry'],16);b=mapped[p];oi=[oc.at(x)for x in sorted(_trace(oc,p,ENTRIES).code)];ni=[nc.at(x)for x in sorted(_trace(nc,b,starts).code)];assert base.full_tokens(oi,old)==base.full_tokens(ni,new)
  q.update(later_entry=f'{b:08x}',old_instructions=len(oi),later_instructions=len(ni),normalized_instruction_and_internal_branch_topology_equal=True)
  for label,ins,image,entry in(('disc',oi,old,p),('later',ni,new,b)):
   dst=a.output.parent/'instructions'/f'{p:08x}-{label}-{entry:08x}.asm'
   for i in ins:assert image.read(i.address,len(i.bytes))==i.bytes
   frozen_write(dst,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode());q[label+'_listing']=dict(file=dst.name,sha256=sha(dst))
 r.update(complete_disposal_sha256=sha(__file__),complete_routines=len(mapped),short_routines_mapped_from_actual_direct_callers=True,actual_virtual_constructor_anchors=anchors,later_gameplay_execution_claimed=False)
 frozen_write(a.output.parent/'complete-report.json',encoded(r));print(json.dumps(dict(complete_routines=len(mapped),report=sha(a.output.parent/'complete-report.json'))))
if __name__=='__main__':main()
