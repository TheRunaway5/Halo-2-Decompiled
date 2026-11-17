#!/usr/bin/env python3
"""Physical corridor publication prefixes and terminal-margin supplementation."""
import argparse,copy,gzip,json,os,struct
from pathlib import Path
import oracle_native_campaign_ai_route_corridor_tu15 as corridor
from oracle_smg_firing_1_1 import Xbe,_Code
from generate_physics_live_support_tu15 import complete_body
reference,search=corridor.reference,corridor.search

def inputs(image):
 rows=[]
 for row in corridor.inputs(image):
  if row['entry'] not in (0x2cdbc0,0x2cdeb0) or not row['name'].startswith('margin_') or '_threshold0.0_t(3.0, 1.0)' not in row['name']:continue
  for x in (2.05,2.4,2.6):
   new=copy.deepcopy(row);new['id']=len(rows);new['name']=row['name']+'_near_edge'+str(x)
   bank=next(b for b in new['banks'] if b['address']==corridor.DATA);data=bytearray.fromhex(bank['before'])
   for at in (corridor.TARGET,corridor.CONTEXT+0x10):struct.pack_into('<f',data,at-corridor.DATA,x)
   for n in range(8):struct.pack_into('<f',data,0x2c+n*0x34+0x14,abs(x-1.))
   bank['before']=data.hex();rows.append(new)
 return rows

def main():
 p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);args=p.parse_args();assert not args.output.exists()
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']);image=Xbe(corridor.NEW_DEFAULT);assert reference.sha(corridor.NEW_DEFAULT)==corridor.NEW_PIN
 meta=reference.metadata(image,corridor.ENTRIES);owned=reference.prepare(image,meta,scratch)
 rows=search.physical(owned,inputs(image),'terminal-margin');assert all(r['completed'] for r in rows)
 seeds=corridor.inputs(image);chosen=[]
 for row in seeds:
  if row['before_fp']!=list(search.FP_PROFILES[0]):continue
  if row['entry'] in (0x2cd3d0,0x2cd440,0x2cd500,0x2cd780,0x2cdeb0) and ('_m3_' in row['name'] or 'append_m10_' in row['name'] or 'search_2cd780_o1_t(3.0, 1.0)' in row['name'] or 'search_2cdeb0_o1_t(3.0, 1.0)' in row['name']):chosen.append(row)
 chosen=search.physical(owned,chosen,'prefix-seeds');seen=set();prefixes=[]
 for row in chosen:
  for ordinal,event in enumerate(row['writes']):
   pc=event['instruction']
   if pc in seen or pc not in (0x2cd426,0x2cd4eb,0x2cd6bd,0x2cd6c7,0x2cd6fe,0x2cd714,0x2cd73a,0x2cd753,0x2cd75d,0x2cd78f,0x2cd7e1,0x2cd810,0x2cd857,0x2cdf01,0x2cdde3):continue
   seen.add(pc);new=copy.deepcopy(row)
   for key in ('reads','writes','executed_sites','calls','boundary','completed'):new.pop(key,None)
   for b in new['banks']:b.pop('after',None)
   new.update(id=len(rows)+len(prefixes),name=row['name']+'_publish_'+hex(pc),fault=dict(phase='after_write',ordinal=ordinal));prefixes.append(new)
 rows+=search.physical(owned,prefixes,'publication-prefixes')
 reached=set().union(*(set(r['executed_sites']) for r in rows));code=_Code(image,'.text');coverage={hex(e):dict(reached=len(set(complete_body(code,e).code)&reached),total=len(complete_body(code,e).code)) for e in corridor.OWNED}
 fixture=dict(image_sha256=corridor.NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,stop_entries=[],bank_encoding='sparse-runs-v1',write_encoding='rep-runs-v1',cases=reference.compact(rows))
 data=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0);args.output.write_bytes(data)
 report=dict(cases=len(rows),prefixes=len(prefixes),coverage=coverage,fixture=str(args.output),fixture_bytes=len(data),fixture_sha256=reference.sha(args.output),owned_scratch=str(scratch),source_sha256={str(p):reference.sha(p) for p in (Path(__file__),Path(corridor.__file__))})
 args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
