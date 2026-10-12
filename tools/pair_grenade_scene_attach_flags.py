#!/usr/bin/env python3
"""Pair complete original attach/frozen helpers without executing caller services."""
import argparse,csv,json
from pathlib import Path
from oracle_grenade_scene_state import IMAGES,digest
from xbe import Xbe
from functions import _Code,_trace
from research_mp_original_data_u5 import norm
from research_mp_live_locomotion_owners_u5 import raw_prefix_candidates

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 images=[Xbe(path) for path,_,_ in IMAGES];codes=[_Code(i,'.text') for i in images]
 starts=[{int(r['address'],16) for r in csv.DictReader(path.open(),delimiter='\t')} for _,_,path in IMAGES]
 rows=[]
 for entry in (0xbaa80,0xba2a0,0xb82a0,0xb86c0):
  old=[codes[0].at(x) for x in sorted(_trace(codes[0],entry,starts[0]).code)];pos={i.address:n for n,i in enumerate(old)};tokens=[norm(i,images[0],pos) for i in old];matches=[]
  for target in raw_prefix_candidates(codes[0],images[0],codes[1],entry,12):
   try:
    new=[codes[1].at(x) for x in sorted(_trace(codes[1],target,starts[1]).code)];np={i.address:n for n,i in enumerate(new)}
    if [norm(i,images[1],np) for i in new]==tokens:matches.append((target,new))
   except Exception:pass
  assert len(matches)==1,(hex(entry),[hex(x) for x,_ in matches]);target,new=matches[0]
  for label,ins in [('1.1',old),('1.5',new)]:
   (a.output/f'{entry:08x}-{label}.asm').write_text(''.join(f'{i.address:08x} {bytes(i.bytes).hex()} {i.mnemonic} {i.op_str}\n' for i in ins))
  rows.append(dict(old=f'{entry:08x}',new=f'{target:08x}',instructions=len(old),mapping=[dict(old=f'{x.address:08x}',new=f'{y.address:08x}') for x,y in zip(old,new)]))
 report=dict(schema='grenade-scene-attach-flag-pairs-v1',scope=__doc__,tool_sha256=digest(Path(__file__)),images=[dict(path=str(path),sha256=pin) for path,pin,_ in IMAGES],pairs=rows,limits='Whole CPU instruction correspondence only; does not execute or replace physics/world callbacks, infer a parent-to-hide rule, or establish held render visibility. Genuine stores/records remain in the separate prior release capture.')
 assert all(digest(path)==pin for path,pin,_ in IMAGES)
 (a.output/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
if __name__=='__main__':main()
