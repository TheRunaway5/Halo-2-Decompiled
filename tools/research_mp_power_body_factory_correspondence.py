#!/usr/bin/env python3
"""Complete actual old/new factory-body correspondence, no user annotations."""
import argparse,json,struct
from pathlib import Path
from capstone.x86 import X86_OP_IMM
import research_mp_sandbox_update_functions as base
from oracle_mp_power_body_factory_1_1 import ROOT,OUT,ENTRIES,verify,sha
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--output',type=Path,default=OUT/'later-correspondence/report.json');args=ap.parse_args();verify()
 base.ENTRIES=tuple(sorted(ENTRIES|{0x1ee380,0x1ee8f0}))
 import sys
 sys.argv=[sys.argv[0],'--output',str(args.output)];base.main();r=json.loads(args.output.read_text())
 old=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');new=Xbe(base.NEW_DEFAULT);oc,nc=_Code(old,'.text'),_Code(new,'.text')
 mapped={int(q['disc_1_1_entry'],16):int(q['later_entry'],16)for q in r['routines']if q.get('normalized_instruction_and_internal_branch_topology_equal')}
 # Resolve short owner leaves from actual paired direct-call sites and
 # original constructed virtual tables, not annotation labels or heuristics.
 for q in r['routines']:
  if q.get('normalized_instruction_and_internal_branch_topology_equal'):
   for x in q['paired_direct_calls']:
    a,b=int(x['old_target'],16),int(x['later_target'],16)
    if a in base.ENTRIES:mapped.setdefault(a,b);assert mapped[a]==b
 tables=[]
 for a,vt in ((0x321320,0x41b8f0),(0x31f970,0x41b770),(0x320cc0,0x41b7f0),(0x1ee380,0x45d188)):
  b=mapped[a];oi=[oc.at(p)for p in sorted(_trace(oc,a,set(base.ENTRIES)).code)];ni=[nc.at(p)for p in sorted(_trace(nc,b,set(mapped.values())).code)]
  assert base.full_tokens(oi,old)==base.full_tokens(ni,new)
  pairs=[(x,y)for x,y in zip(oi,ni)if any(o.type==X86_OP_IMM and o.imm==vt for o in x.operands)]
  assert len(pairs)==(2 if a==0x1ee380 else 1),(hex(a),len(pairs))
  nvt=pairs[0][1].operands[-1].imm;assert all(y.operands[-1].imm==nvt for _,y in pairs)
  slots=[]
  for offset in range(0,0x60,4):
   op=struct.unpack('<I',old.read(vt+offset,4))[0];np=struct.unpack('<I',new.read(nvt+offset,4))[0]
   if op in base.ENTRIES:mapped.setdefault(op,np);assert mapped[op]==np
   slots.append(dict(offset=offset,old_target=f'{op:08x}',later_target=f'{np:08x}'))
  tables.append(dict(old_address=f'{vt:08x}',later_address=f'{nvt:08x}',owner_old=f'{a:08x}',owner_later=f'{b:08x}',slots=slots))
 for q in r['routines']:
  a=int(q['disc_1_1_entry'],16)
  assert a in mapped,hex(a)
  b=mapped[a];oi=[oc.at(p)for p in sorted(_trace(oc,a,set(base.ENTRIES)).code)];ni=[nc.at(p)for p in sorted(_trace(nc,b,set(mapped.values())).code)]
  assert base.full_tokens(oi,old)==base.full_tokens(ni,new),(hex(a),hex(b),len(oi),len(ni))
  q.update(later_entry=f'{b:08x}',old_instructions=len(oi),later_instructions=len(ni),normalized_instruction_and_internal_branch_topology_equal=True)
  for label,ins,image,address in(('disc',oi,old,a),('later',ni,new,b)):
   p=args.output.parent/'instructions'/f'{a:08x}-{label}-{address:08x}.asm'
   for i in ins:assert image.read(i.address,len(i.bytes))==i.bytes
   frozen_write(p,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode());q[label+'_listing']=dict(file=p.name,sha256=sha(p))
 r.update(complete_wrapper_sha256=sha(__file__),constructed_virtual_tables=tables,original_later_gameplay_executed=False)
 frozen_write(args.output.parent/'complete-report.json',encoded(r));print(json.dumps(dict(complete_routines=len(r['routines']),constructed_vtables=len(tables))))
if __name__=='__main__':main()
