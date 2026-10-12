#!/usr/bin/env python3
"""Independent full original selected kill-sound prerequisite correspondence."""
import csv,json,struct,sys
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from capstone.x86 import X86_OP_IMM
from pathlib import Path
import research_mp_sandbox_update_functions as original
original.ENTRIES=(0x20e8b0,0x20e920,0x20e960,0x126de0,0x20ef90,0x21b540,0x13fb30,0x33785c,0x1656a0)
if __name__=='__main__':
 original.main();p=Path(sys.argv[sys.argv.index('--output')+1]);r=json.loads(p.read_text())
 old=original.Xbe(original.ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');new=original.Xbe(original.NEW_DEFAULT)
 oc=original._Code(old,'.text');nc=original._Code(new,'.text');by={q['disc_1_1_entry']:q for q in r['routines']}
 call=next(q for q in by['0020ef90']['paired_direct_calls']if q['old_target']=='0021b540')
 needle=struct.pack('<II',int(by['0020e8b0']['later_entry'],16),int(by['0020e920']['later_entry'],16));matches=[]
 for sec in new.sections:
  raw=new.section_bytes(sec);pos=raw.find(needle)
  while pos>=0:matches.append((sec.va+pos,raw[pos:pos+24]));pos=raw.find(needle,pos+1)
 assert len(matches)==1;table,raw=matches[0];reset=struct.unpack_from('<I',raw,8)[0]
 assert old.read(0x447a7c,12)==struct.pack('<III',0x20e8b0,0x20e920,0x20e960)
 owning=[(0x20e960,reset,dict(owner='actual callback record45 following independently paired ctor/dispose',old_table='00447a7c',later_table=f'{table:08x}',later_raw18_hex=raw.hex())),
  (0x21b540,int(call['later_target'],16),dict(owner='actual paired20EF90 direct call',paired_call=call))]
 starts={int(q['address'],16)for q in csv.DictReader((original.ROOT/'analysis/map-pack-update/functions.tsv').open(),delimiter='\t')}|set(original.ENTRIES)
 md=Cs(CS_ARCH_X86,CS_MODE_32);md.detail=True;md.skipdata=True;sec=new.section('.text');newstarts={b for _,b,_ in owning}
 for i in md.disasm(new.section_bytes(sec),sec.va):
  if i.mnemonic=='call'and len(i.operands)==1 and i.operands[0].type==X86_OP_IMM and nc.inside(i.operands[0].imm):newstarts.add(i.operands[0].imm)
 for a,b,owner in owning:
  row=by[f'{a:08x}'];left=original._trace(oc,a,starts);right=original._trace(nc,b,newstarts)
  oi=[oc.at(v)for v in sorted(left.code)];ni=[nc.at(v)for v in sorted(right.code)]
  assert original.full_tokens(oi,old)==original.full_tokens(ni,new)
  assert not left.tables and not right.tables
  row.update(later_entry=f'{b:08x}',owner_derived_correspondence=owner,old_instructions=len(oi),later_instructions=len(ni),
   normalized_instruction_and_internal_branch_topology_equal=True,switch_table_topology_equal=True,switch_tables=[])
  for label,ins,image in(('disc',oi,old),('later',ni,new)):
   dest=p.parent/'instructions'/f'{a:08x}-{label}-{a if label=="disc"else b:08x}.asm'
   for i in ins:assert image.read(i.address,len(i.bytes))==i.bytes
   dest.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins));row[label+'_listing']=dict(file=dest.name,sha256=original.sha(dest))
 r['owner_selection_tool_sha256']=original.sha(__file__);r['instruction_matcher_tool_sha256']=r.pop('tool_sha256')
 p.write_text(json.dumps(r,indent=2)+'\n')
