#!/usr/bin/env python3
"""Full authenticated old/new wrapper owners; original instructions only."""
import argparse,json,sys
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from capstone.x86 import X86_OP_IMM
import research_mp_sandbox_update_functions as base
from oracle_mp_power_body_prepare_1_1 import ROOT,OUT,verify,sha
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
ENTRIES={0x1dacf0,0x1dadf0,0x1dad80,0x184510,0x184800,0x2e2530,0x2e25a0,0x2e2260,0x2e5720,0x2e5750,0x2e5760,0x2e5780}
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'final/later-correspondence/report.json');a=ap.parse_args();verify();base.ENTRIES=tuple(sorted(ENTRIES));sys.argv=[sys.argv[0],'--output',str(a.output)];base.main();r=json.loads(a.output.read_text())
 old=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');new=Xbe(base.NEW_DEFAULT);oc,nc=_Code(old,'.text'),_Code(new,'.text')
 mapped={int(q['disc_1_1_entry'],16):int(q['later_entry'],16)for q in r['routines']if q.get('normalized_instruction_and_internal_branch_topology_equal')}
 starts=set(mapped.values());md=Cs(CS_ARCH_X86,CS_MODE_32);md.detail=True;md.skipdata=True;sec=new.section('.text')
 for i in md.disasm(new.section_bytes(sec),sec.va):
  if i.mnemonic=='call'and len(i.operands)==1 and i.operands[0].type==X86_OP_IMM:starts.add(i.operands[0].imm)
 while True:
  before=len(mapped)
  for oldentry,newentry in list(mapped.items()):
   oi=[oc.at(p)for p in sorted(_trace(oc,oldentry,ENTRIES).code)];ni=[nc.at(p)for p in sorted(_trace(nc,newentry,starts).code)]
   assert base.full_tokens(oi,old)==base.full_tokens(ni,new),(hex(oldentry),hex(newentry))
   for x,y in zip(oi,ni):
    if x.mnemonic=='call'and x.operands[0].type==y.operands[0].type==X86_OP_IMM and x.operands[0].imm in ENTRIES:
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
 # Actual wrapper constructor vtable immediates own its type getter mapping.
 p=mapped[0x2e2530];oi=[oc.at(x)for x in sorted(_trace(oc,0x2e2530,ENTRIES).code)];ni=[nc.at(x)for x in sorted(_trace(nc,p,starts).code)]
 match=[(x,y)for x,y in zip(oi,ni)if x.mnemonic=='mov'and x.op_str=='dword ptr [eax], 0x418930'];assert len(match)==1
 vt=match[0][1].operands[1].imm;target=int.from_bytes(new.read(vt+0x14,4),'little');left=[oc.at(x)for x in sorted(_trace(oc,0xa0120,{0xa0120}).code)];right=[nc.at(x)for x in sorted(_trace(nc,target,{target}).code)];assert base.full_tokens(left,old)==base.full_tokens(right,new)
 virtual=dict(old_vtable='00418930',later_vtable=f'{vt:08x}',old_type_getter='000a0120',later_type_getter=f'{target:08x}',type_return=23,complete_original_getter='\n'.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}'for i in left),complete_later_getter='\n'.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}'for i in right))
 r.update(complete_wrapper_sha256=sha(__file__),complete_routines=len(mapped),short_constructors_mapped_from_actual_direct_callers=True,constructed_wrapper_type_getter=virtual,later_gameplay_execution_claimed=False)
 frozen_write(a.output.parent/'complete-report.json',encoded(r));print(json.dumps(dict(complete_routines=len(mapped),type_getter=virtual,report=sha(a.output.parent/'complete-report.json'))))
if __name__=='__main__':main()
