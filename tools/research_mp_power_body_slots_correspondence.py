#!/usr/bin/env python3
"""Complete authenticated old/new slot-owner instruction correspondence."""
import argparse,json,sys
from pathlib import Path
import research_mp_sandbox_update_functions as base
from oracle_mp_power_body_slots_1_1 import ROOT,OUT,verify,sha
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
ENTRIES={0x1d9090,0x1d9330,0x1d95e0,0x1d98b0,0x1d9f50,0x1da020,
 0x1da3c0,0x1d3a70,0x1dc0c0,0x1d29a0,0x1d4260,0x1d2ca0,0x1d2d20,
 0x1d2da0,0x318900,0x1da4c0,0x1db1c0,0x1d3bb0,0x1c7050,0x1c6f50,
 0x1c72e0,0x1c70d0,0x1dacf0,0x1dadf0,0x317970,0x31b250,0x9d900}
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'later-correspondence/report.json');args=ap.parse_args();verify()
 base.ENTRIES=tuple(sorted(ENTRIES));sys.argv=[sys.argv[0],'--output',str(args.output)];base.main()
 r=json.loads(args.output.read_text());old=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');new=Xbe(base.NEW_DEFAULT);oc,nc=_Code(old,'.text'),_Code(new,'.text')
 mapped={int(q['disc_1_1_entry'],16):int(q['later_entry'],16)for q in r['routines']if q.get('normalized_instruction_and_internal_branch_topology_equal')}
 # Short virtual getters are owned by the previously frozen constructed
 # motion vtables, not by nonunique constant-return search prefixes.
 tables=json.loads((ROOT/'analysis/multiplayer/sandbox-source/power-body-factory/final/later-correspondence/complete-report.json').read_text())['constructed_virtual_tables']
 for t in tables:
  for q in t['slots']:
   a,b=int(q['old_target'],16),int(q['later_target'],16)
   if a in(0x31b250,0x9d900):mapped[a]=b
 for q in r['routines']:
  if q.get('normalized_instruction_and_internal_branch_topology_equal'):
   for x in q['paired_direct_calls']:
    a,b=int(x['old_target'],16),int(x['later_target'],16)
    if a in ENTRIES:
     assert a not in mapped or mapped[a]==b;(mapped.setdefault(a,b))
 for q in r['routines']:
  a=int(q['disc_1_1_entry'],16);assert a in mapped,hex(a);b=mapped[a]
  oi=[oc.at(p)for p in sorted(_trace(oc,a,set(ENTRIES)).code)];ni=[nc.at(p)for p in sorted(_trace(nc,b,set(mapped.values())).code)]
  assert base.full_tokens(oi,old)==base.full_tokens(ni,new),(hex(a),hex(b),len(oi),len(ni))
  q.update(later_entry=f'{b:08x}',old_instructions=len(oi),later_instructions=len(ni),normalized_instruction_and_internal_branch_topology_equal=True)
  for label,ins,image,address in(('disc',oi,old,a),('later',ni,new,b)):
   p=args.output.parent/'instructions'/f'{a:08x}-{label}-{address:08x}.asm'
   for i in ins:assert image.read(i.address,len(i.bytes))==i.bytes
   frozen_write(p,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode());q[label+'_listing']=dict(file=p.name,sha256=sha(p))
 r.update(complete_wrapper_sha256=sha(__file__),short_virtual_getters_resolved_from_frozen_source_vtables=True,later_gameplay_execution_claimed=False)
 frozen_write(args.output.parent/'complete-report.json',encoded(r));print(json.dumps(dict(complete_routines=len(r['routines']),report=sha(args.output.parent/'complete-report.json'))))
if __name__=='__main__':main()
