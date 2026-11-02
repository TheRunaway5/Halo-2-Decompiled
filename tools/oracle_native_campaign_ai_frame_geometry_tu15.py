#!/usr/bin/env python3
"""Physical unchanged TU1.5 direction/inverse-frame/difference reference.

Inputs extend the independently declared point-geometry matrix. No code,
dependency return or stopped original outcome is replaced or resumed.
"""
import argparse,copy,gzip,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import oracle_native_campaign_ai_point_geometry_tu15 as points
import validate_native_geometry_tu15 as geometry
NEW=(0x145140,0x145200,0x218fe0,0x218880,0x218960,0x218eb0)
ENTRIES=NEW+(0x145070,0x219030,0x218780,0x218b20)
sha=common.sha
def prepare(image,meta,scratch):
 p=points.prepare(image,meta,scratch)
 r=subprocess.run(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_frame_geometry_tu15.cpp'),'-o',str(p/'observer')],text=True,capture_output=True);assert not r.returncode,r.stderr
 return p
def rows(image):
 result=[]
 for old in points.inputs(image):
  entries={0x145070:(0x145140,0x145200),0x219030:(0x218fe0,),0x218780:(0x218880,0x218960),0x218b20:(0x218960,),0x218d00:(0x218eb0,)}[old['entry']]
  for entry in entries:
   r=copy.deepcopy(old);r.update(id=len(result),entry=entry,name=f'{entry:x}_'+old['name']);r['banks'].append(dict(address=0x44ac8c,before=image.read(0x44ac8c,36).hex()))
   # Metadata-alias destinations publish all twelve bytes, including adjacent
   # ordinary retained RAM. Do not omit those stores from physical provenance.
   for bank in r['banks']:
    if bank['address'] in (0x4ecbd0,0x505e78):bank['before']+='00'*8
   g=r['gp'];source,destination,matrix=r['point_source'],r['destination'],r['point_matrix'];r['matrix']=0;r['source']=0
   if entry==0x145140:g[0]=source;g[1]=matrix;g[2]=destination
   elif entry==0x145200:g[1]=matrix;g[2]=destination;g[6]=source
   elif entry==0x218fe0:g[0]=destination;g[2]=source;r['matrix']=r['variant']
   elif entry==0x218880:g[0]=r['frame'];g[6]=destination;g[7]=source
   elif entry==0x218960:g[0]=r['frame'];g[1]=source;g[3]=destination
   elif entry==0x218eb0:g[0]=destination;g[1]=source;g[3]=r['second']
   result.append(r)
 # Distinguish masked comparisons/scaling and late matrix read failures from
 # a conventional eager affine calculation. These are real retained operands.
 def put(r,address,value,width=4):
  for b in r['banks']:
   raw=bytearray.fromhex(b['before']);start=b['address']
   if start<=address and address+width<=start+len(raw):raw[address-start:address-start+width]=int(value&((1<<(width*8))-1)).to_bytes(width,'little');b['before']=raw.hex();return
  raise ValueError(hex(address))
 for entry in (0x145140,0x145200):
  seed=next(r for r in result if r['entry']==entry and 'finite' in r['name'])
  for offset in (0,4,8,12,0x10,0x14,0x18,0x1c,0x20,0x24,0x28,0x2c,0x30):
   for value in (0,0x80000000,1,0x7f7fffff,0x7f812345,0xffc54321):
    r=copy.deepcopy(seed);r.update(id=len(result),name=f'field_{entry:x}_{offset:x}_{value:x}');put(r,points.MATRIX+offset,value);result.append(r)
 for entry in (0x218880,0x218960):
  seed=next(r for r in result if r['entry']==entry and 'finite' in r['name'])
  for scale in (0,0x80000000,1,0x3f800000,0x7f800000,0x7fc12345,0x7f812345):
   r=copy.deepcopy(seed);r.update(id=len(result),name=f'frame_scale_{entry:x}_{scale:x}');put(r,points.BODY+0x200,scale);result.append(r)
  for destination in (points.FRAMES,points.FRAMES+4,points.BODY+0x114,0x4ecbd0,0x505e78):
   r=copy.deepcopy(seed);r.update(id=len(result),name=f'alias_metadata_{entry:x}_{destination:x}',destination=destination);r['gp'][6 if entry==0x218880 else 3]=destination;result.append(r)
 # All published direction/inverse roots and the two difference associations
 # get complete before-read, before-write and after-write prefix cohorts.
 for entry in NEW:
  choices=[r for r in result if r['entry']==entry and ('finite' in r['name'] or 'alias_' in r['name'] or 'scale_0' in r['name'])]
  for n,seed in enumerate(choices[:1]+[r for r in choices if 'alias_' in r['name']][:1]+[r for r in choices if r['name'].endswith('scale_0')][:1]):
   r=copy.deepcopy(seed);r.update(id=len(result),name=f'fault_seed_{entry:x}_{n}');result.append(r)
 return result
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',type=Path);a=p.parse_args();assert not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']);image=Xbe(NEW_DEFAULT);common.ENTRIES=ENTRIES;meta=common.metadata(image);owned=prepare(image,meta,scratch);complete=geometry.physical(owned,rows(image),'complete');prefix=[]
 for seed in (r for r in complete if r['name'].startswith('fault_seed')):
  for phase,events in (('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
   for n in range(len(events)):
    r=copy.deepcopy(seed);r.update(id=len(complete)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(r)
 allrows=complete+geometry.physical(owned,prefix,'prefix');faults=[]
 for entry in NEW:
  seed=next(r for r in complete if r['entry']==entry and 'finite' in r['name']);r=copy.deepcopy(seed);r.update(id=len(allrows)+len(faults),name=f'actual_missing_source_{entry:x}',expected_fault=True,point_source=0xdead0000)
  r['gp'][{0x145140:0,0x145200:6,0x218fe0:2,0x218880:7,0x218960:1,0x218eb0:1}[entry]]=0xdead0000;faults.append(r)
 allrows+=geometry.physical(owned,faults,'actual-faults')
 visited=set().union(*(set(r['executed_sites'])for r in complete))
 from oracle_smg_firing_1_1 import _Code
 from generate_physics_live_support_tu15 import complete_body
 code=_Code(image,'.text');newsites=set().union(*(set(complete_body(code,e).code)for e in NEW));uncovered=sorted(newsites-visited);assert not uncovered,uncovered
 fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=allrows);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0))
 repeat=None
 if a.verify:repeat=a.verify.read_bytes()==a.output.read_bytes();assert repeat,'Physical original repeat differs'
 report=dict(passed=True,cases=len(allrows),complete_calls=len(complete),fault_prefixes=len(prefix),actual_faults=len(faults),new_original_sites=len(newsites),all_reached_sites=len(visited),uncovered_new_sites=uncovered,ordered_reads=sum(len(r['reads'])for r in allrows),ordered_stores=sum(len(r['writes'])for r in allrows),original_instructions=sum(r['original_instructions']for r in allrows),original_sha256=NEW_PIN,fixture=str(a.output),fixture_bytes=a.output.stat().st_size,fixture_sha256=sha(a.output),repeat_identical=repeat,owners=meta['owners'],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_campaign_ai_frame_geometry_tu15.cpp':sha(ROOT/'tools/observe_native_campaign_ai_frame_geometry_tu15.cpp')},owned_scratch=str(scratch),retained_large_artifacts=[],scope_limits=['Internal native semantic APIs, not arbitrary numeric-call GP/SIMD scratch parity','All actual frame permutations use retained axis0..2 and variant0|1 tables; private inverse result has exactly XYZ coordinates','Masked SIMD exceptions only; original occupied x87 state remains unchanged','Full navigation search/update/live route remains open'])
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(f'{len(allrows)} physical frame geometry cases, {len(newsites)} new sites, fixture{a.output.stat().st_size}B')
if __name__=='__main__':main()
