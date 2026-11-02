#!/usr/bin/env python3
"""Additive exact read/write/observer prefix proof for native E18D0 composition."""
import copy,inspect,json,sys
from pathlib import Path
import validate_native_camera_adjustment_tu15 as base
physical=base.physical
original=physical.physical
profile_count=0

def qualify(image,metadata,cases,scratch):
 global profile_count
 rows=original(image,metadata,cases,scratch)
 prefixes=[];code=base._Code(image,'.text')
 for seed in(c for c in rows if c['name'].startswith('fault_seed')):
  for category in('reads','writes'):
   seen=set();counts={}
   for event in seed[category]:
    key=event['address'],event['bytes']if category=='reads'else len(event['written'])//2
    counts[key]=counts.get(key,0)+1
    if event['instruction']in seen:continue
    seen.add(event['instruction']);c=copy.deepcopy(seed)
    c.update(name=seed['name']+'_'+category+'_'+hex(event['instruction']),stop_pc=event['instruction'],fault=dict(address=key[0],bytes=key[1],occurrence=counts[key],access='read'if category=='reads'else'write'))
    prefixes.append(c)
  for n,event in enumerate(seed['writes']):
   c=copy.deepcopy(seed);c.update(name=seed['name']+'_observer_'+str(n),stop_pc=event['instruction']+code.at(event['instruction']).size,observer_fault=n+1);prefixes.append(c)
 profile_count=len(prefixes)
 return rows+original(image,metadata,prefixes,scratch)

if __name__=='__main__':
 physical.physical=qualify
 curve=None
 if '--curve-source'in sys.argv:
  n=sys.argv.index('--curve-source');curve=Path(sys.argv[n+1]).resolve();del sys.argv[n:n+2]
 source=inspect.getsource(base.main).replace("assert all(c['boundary_gp'][4]==physical.SP+16 for c in rows)","assert all(c['boundary_gp'][4]==physical.SP+16 for c in rows if 'fault'not in c and 'observer_fault'not in c)")
 if curve:
  old="for unit in UNITS:shutil.copyfile(ROOT/unit,project/Path(unit).name)"
  assert source.count(old)==1
  source=source.replace(old,"for unit in UNITS:shutil.copyfile(Path("+repr(str(curve))+")if unit=='src/core/native_tu15_function_curves.cpp'else ROOT/unit,project/Path(unit).name)")
 namespace=dict(base.__dict__);exec(source,namespace)
 try:status=namespace['main']()
 finally:
  if '--output'in sys.argv:
   p=Path(sys.argv[sys.argv.index('--output')+1])/'report.json'
   if p.exists():
    r=json.loads(p.read_text());r.update(fault_or_observer_profiles=profile_count,additive_validator_sha256=base.sha(Path(__file__)))
    if curve:r['private_curve_candidate']=str(curve);r['private_curve_candidate_sha256']=base.sha(curve);r.get('source_hashes',{})['src/core/native_tu15_function_curves.cpp']=base.sha(curve)
    base.save(p,r)
 sys.exit(status)
