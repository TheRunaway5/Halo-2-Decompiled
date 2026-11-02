#!/usr/bin/env python3
"""Additive strict CD4A0 parent faults, with separately qualified native children."""
import copy,json,sys
from pathlib import Path
import validate_native_camera_position_tu15 as parent
original=parent.physical.physical
count=0

def physical(image,meta,cases,scratch):
 global count
 rows=original(image,meta,cases,scratch);prefixes=[];code=parent.base._Code(image,'.text')
 for seed in(c for c in rows if 'fault_seed'in c['name']):
  for category in('reads','writes'):
   seen=set();occurrences={}
   for e in seed[category]:
    key=e['address'],e['bytes']if category=='reads'else len(e['written'])//2;occurrences[key]=occurrences.get(key,0)+1
    if e['instruction']in seen:continue
    seen.add(e['instruction']);c=copy.deepcopy(seed);c.update(name=seed['name']+'_'+category+'_'+hex(e['instruction']),stop_pc=e['instruction'],fault=dict(address=key[0],bytes=key[1],occurrence=occurrences[key],access='read'if category=='reads'else'write'));prefixes.append(c)
  for n,e in enumerate(seed['writes']):
   c=copy.deepcopy(seed);c.update(name=seed['name']+'_observer_'+str(n),stop_pc=e['instruction']+code.at(e['instruction']).size,observer_fault=n+1);prefixes.append(c)
 count=len(prefixes);return rows+original(image,meta,prefixes,scratch)
if __name__=='__main__':
 parent.physical.physical=physical
 try:status=parent.main()
 finally:
  p=Path(sys.argv[sys.argv.index('--output')+1])/'report.json'
  if p.exists():
   r=json.loads(p.read_text());r['parent_fault_or_observer_profiles']=count;r['fault_validator_sha256']=parent.base.sha(Path(__file__));parent.base.save(p,r)
 sys.exit(status)
