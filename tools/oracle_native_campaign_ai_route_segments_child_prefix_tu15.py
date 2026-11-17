#!/usr/bin/env python3
"""Unchanged-original observer stops inside native route children, owned scratch only."""
import copy,gzip,json,os,sys
from pathlib import Path
sys.path.insert(0,'tools')
import oracle_native_campaign_ai_route_segments_tu15 as segment
import oracle_native_campaign_ai_path_search_tu15 as search
import oracle_native_campaign_ai_area_perform_tu15 as reference
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import argparse
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',required=True,type=Path);parser.add_argument('--report',required=True,type=Path);args=parser.parse_args()
scratch=Path(os.environ['HALO2_TEST_SCRATCH']);image=Xbe(NEW_DEFAULT)
meta=reference.metadata(image,segment.ENTRIES);owned=reference.prepare(image,meta,scratch)
inputs=[r for r in segment.inputs(image) if r['name'].startswith('corner_path_position3.0_y3.0_')]
for n,r in enumerate(inputs):r['id']=n
whole=search.physical(owned,inputs,'whole')
prefixes=[]
for row in whole:
 for n,w in enumerate(row['writes']):
  if w['instruction'] not in (0x276dd1,0x2cc6e0,0x2cc783,0x2cc81e,0x2cc8e3):continue
  p=copy.deepcopy(row)
  for k in ('reads','writes','executed_sites','calls','boundary','completed'):p.pop(k,None)
  for bank in p['banks']:bank.pop('after',None)
  p.update(id=len(whole)+len(prefixes),name=row['name']+'_child_after_write_'+str(n),fault=dict(phase='after_write',ordinal=n))
  prefixes.append(p)
rows=whole+search.physical(owned,prefixes,'childprefix')
fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',instruction_patches=[],supplied_original_outcomes=False,bank_encoding='sparse-runs-v1',write_encoding='rep-runs-v1',cases=reference.compact(rows))
p=args.output;p.parent.mkdir(parents=True,exist_ok=True);assert not p.exists();p.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0))
report=dict(cases=len(rows),whole=len(whole),prefixes=len(prefixes),reads=sum(len(r['reads'])for r in rows),stores=sum(len(r['writes'])for r in rows),owned_scratch=str(scratch),fixture=str(p),fixture_sha256=reference.sha(p),retained_large_artifacts=[])
args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(report)
