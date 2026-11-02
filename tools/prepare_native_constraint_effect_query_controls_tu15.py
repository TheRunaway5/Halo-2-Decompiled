#!/usr/bin/env python3
"""Declared enum/material arguments on unchanged genuine authored rows."""
import argparse,os
from pathlib import Path
from census_native_constraint_runtime_handles_tu15 import Before
from oracle_smg_firing_1_1 import ROOT
from validate_native_contact_agent_dispatch_tu15 import sha,save
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False)
 census=ROOT/'analysis/physics/native-constraint-effect-input-stratified-census-20261101/report.json'
 import json
 source=json.loads(census.read_text());contexts=[]
 for context in source['contexts']:
  m=Before(Path(context['capture']).parent);assert sha(m.metadata)==context['capture_sha256'];queries=[];prototypes={}
  for row in context['positive_queries']:prototypes.setdefault(row['secondary'],row)
  for row in prototypes.values():
   for event in [*range(13,21),0xffffffff]:
    item={k:v for k,v in row.items()if k not in ('retained_candidate','event_row')}
    item.update(source_event_type=row['event_type'],event_type=event,declared_event_type_override=True)
    queries.append(item)
  scenario=m.word(0x4ecc1c);extent=m.word(scenario+0x150);bank=m.word(scenario+0x154);hierarchy=[]
  for row in context['positive_queries']:
   event=row['event_row'];secondary=row['secondary'];count=m.word(event+(0x10 if secondary else 8));candidates=m.word(event+(0x14 if secondary else 0xc))
   keys={m.word(candidates+n*24+0x14,2)for n in range(count)}
   for material in range(extent):
    if material in keys:continue
    current=material;chain=[]
    for _ in range(64):
     if current>=extent or current>=0x8000 or current in chain:break
     chain.append(current)
     if current in keys:
      item={k:v for k,v in row.items()if k not in ('retained_candidate','event_row')}
      item.update(source_material_key=row['material_key'],material_key=material,declared_material_key_override=True,
       actual_before_parent_chain=chain)
      hierarchy.append(item);break
     current=m.word(bank+current*0xb4+8,2)
    if len(hierarchy)>=16:break
   if len(hierarchy)>=16:break
  queries.extend(hierarchy)
  contexts.append(dict(name=context['name'],capture=context['capture'],capture_sha256=context['capture_sha256'],typed_queries=queries,
   declared_event_controls=sum(q.get('declared_event_type_override',False)for q in queries),
   actual_before_parent_chain_controls=len(hierarchy)))
 save(a.output/'report.json',dict(manufactured_rows=False,captured_after_consumed=False,declared_ordinary_arguments=True,
  actual_input_census=dict(path=str(census),sha256=sha(census)),contexts=contexts,owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[]))
 print([(c['name'],c['declared_event_controls'],c['actual_before_parent_chain_controls'])for c in contexts]);return 0
if __name__=='__main__':raise SystemExit(main())
