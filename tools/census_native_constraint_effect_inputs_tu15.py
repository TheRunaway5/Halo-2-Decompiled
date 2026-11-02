#!/usr/bin/env python3
"""Read actual scenario material/event/camera DATA without manufacturing rows."""
import argparse,os
from pathlib import Path
from census_native_constraint_runtime_handles_tu15 import Before
from oracle_smg_firing_1_1 import ROOT
from validate_native_contact_agent_dispatch_tu15 import sha,save
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);contexts=[]
 base=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458'
 for name in ('gemini-column0','gemini-column1','elongation-crates'):
  folder=(base/('iteration-04-'+name)/'original/required-failure-inputs').resolve();m=Before(folder)
  result=dict(name=name,capture=str(m.metadata),capture_sha256=sha(m.metadata),positive_queries=[])
  try:
   scenario=m.word(0x4ecc1c);extent=m.word(scenario+0x150);rows=m.word(scenario+0x154);tags=m.word(0x4f0414)
   assert extent<=4096;result.update(scenario=scenario,materials=extent,material_rows=rows)
   references=[];queries=[];seen=set();positive=0
   for index in range(extent):
    row=rows+index*0xb4;reference=m.word(row+0xb0)
    if reference==0xffffffff:continue
    entry=tags+(reference&0xffff)*16;assert m.word(entry+4)==reference
    definition=m.word(entry+8);count=m.word(definition);bank=m.word(definition+4);assert count<=128
    references.append(dict(material=index,definition_datum=reference,definition=definition,event_count=count))
    for event in range(count):
     record=bank+event*24
     for secondary in (False,True):
      size=m.word(record+(0x10 if secondary else 8));candidates=m.word(record+(0x14 if secondary else 0xc));assert size<=8192
      if not size:continue
      positive+=1
      for n in range(min(size,16)):
       candidate=candidates+n*24;key=m.word(candidate+0x14,2)
       if key>=extent or key>=0x8000:continue
       query=(reference,event,secondary,key)
       if query in seen:continue
       seen.add(query)
       queries.append(dict(definition_datum=reference,event_type=event,secondary=secondary,
        material_key=key,retained_candidate=candidate,event_row=record,source_material=index))
   groups={}
   for query in queries:groups.setdefault((query['secondary'],query['event_type']),[]).append(query)
   sample=[];rank=0
   while len(sample)<64:
    added=False
    for group in groups.values():
     if rank<len(group):sample.append(group[rank]);added=True
     if len(sample)==64:break
    if not added:break
    rank+=1
   result.update(definitions=references,positive_event_rows=positive,unique_positive_queries=len(queries),
    query_groups={str(k):len(v)for k,v in groups.items()},positive_queries=sample)
   users=m.word(0x4f54f0);active=[n for n in range(4)if m.word(users+0xc+n*4)!=0xffffffff]
   result.update(camera_list=users,active_cameras=active,active_bsp=m.word(0x474c24,2))
  except (ValueError,AssertionError,OSError)as error:result['unavailable_actual_backing']=str(error)
  contexts.append(result)
 save(a.output/'report.json',dict(read_only=True,manufactured_rows=False,captured_after_consumed=False,
  contexts=contexts,owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[]))
 print([(r['name'],r.get('materials'),r.get('positive_event_rows'),len(r['positive_queries']),r.get('active_cameras'),r.get('unavailable_actual_backing'))for r in contexts]);return 0
if __name__=='__main__':raise SystemExit(main())
