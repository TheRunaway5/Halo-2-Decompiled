#!/usr/bin/env python3
"""Unchanged physical201500 navigation filter and its actual character callees."""
import argparse,copy,gzip,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,ROOT
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import oracle_native_campaign_ai_character_tu15 as character
import validate_native_geometry_tu15 as geometry
DEST,MASKS,OBJECTPOOL,OBJECTROWS,BODY=0x100b400,0x100b500,0x100b000,0x100b100,0x100b200
sha=common.sha
def inputs():
 rows=[]
 def convert(base,name,physics='mask',action_count=1,direct_order=True,kinds=7,vehicle=False,destination=DEST):
  row=copy.deepcopy(base);banks=row['banks']
  def add(p,b):banks.append(dict(address=p,before=b.hex()))
  add(DEST,bytes((n*17+0x53)&255 for n in range(32)));add(MASKS,bytes(0x100));add(OBJECTPOOL,bytes(0x50));add(OBJECTROWS,bytes(0x10));add(BODY,bytes(0x40));add(0x4ecbd0,struct.pack('<I',OBJECTPOOL));add(character.REGISTRY+0x48,bytes(4))
  def put(p,v,width=4):
   raw=int(v&((1<<(width*8))-1)).to_bytes(width,'little')
   for bank in banks:
    start=bank['address'];data=bytearray.fromhex(bank['before'])
    if start<=p and p+width<=start+len(data):data[p-start:p-start+width]=raw;bank['before']=data.hex();return
   raise ValueError(('Missing declared filter input',hex(p)))
  put(character.TAGS+0x20,0xffffffff if physics=='none'else 0xeeff0004 if physics=='null'else 0xeeff0003)
  put(character.TAGS+3*0x400+0x4c,0 if physics=='empty'else 1);put(character.TAGS+3*0x400+0x50,MASKS)
  put(MASKS,0x80000029);put(MASKS+4,0x40000017);put(MASKS+8,0x2000000b)
  put(character.TAGS+2*0x400+0x4c,action_count);put(character.TAGS+2*0x400+0x50,MASKS+4)
  for index in(-2,0,1):put(character.ORDERS+index*0x7c+0x6c,int(direct_order));put(character.ORDERS+index*0x7c+0x70,MASKS+8)
  for k in range(3):
   record=character.ACTORS+k*0x888;put(record+0x266,int(vehicle),1);put(record+0x26c,0x99990000);put(record+0x300,0x12340000+k)
  for k in range(4):
   nav=character.NAV+k*0x40;put(nav+0x1a,kinds,2);put(nav+0x1c,0xaabbccdd)
   for p,v in((0x10,0x8234),(0x12,0xffff),(0x14,0x7123),(0x16,0x4567)):put(nav+p,v,2)
   for j in range(2):put(character.VEHICLE+k*0x200+j*0xb4+0xb0,0xabcd+j,2)
  put(OBJECTPOOL+0x44,OBJECTROWS);put(OBJECTROWS+8,BODY);put(BODY,base['vehicle_character'])
  row.update(id=len(rows),name=name,entry=0x201500,destination=destination,matrix=base['actor'],source=0)
  row['gp'][7]=destination;banks.sort(key=lambda b:b['address']);rows.append(row)
 bases=[r for r in character.inputs()if r['character']!=0xffffffff and '_on_null'not in r['name'] and not r['name'].startswith('fault_seed')]
 for n,base in enumerate(bases):convert(base,'filter_'+base['name'],physics=('none','empty','mask','null')[n%4],action_count=n%3-1,direct_order=n%3!=0,kinds=(-32768,-1,0,1,6,7,32767)[n%7],vehicle=n%3==0)
 ordinary=next(r for r in bases if r['entry']==0x1ed100 and r['name'].startswith('inherited_')and r['character']!=0xffffffff)
 # Positive inherited navigation/vehicle blocks with explicit controls.
 ordinary=copy.deepcopy(ordinary)
 for bank in ordinary['banks']:
  if bank['address']==character.TAGS:
   raw=bytearray.fromhex(bank['before'])
   for offset in(0x5c,0xe4):struct.pack_into('<I',raw,offset,2)
   bank['before']=raw.hex()
 for vehicle in(False,True):
  for physics in('none','empty','mask','null'):
   for kinds in(-1,0,1,6,7,32767):convert(ordinary,f'explicit_{vehicle}_{physics}_{kinds}',physics=physics,vehicle=vehicle,kinds=kinds)
 for destination in(character.NAV+0x10,character.ACTORS+0x300,MASKS):convert(ordinary,f'alias_{destination:x}',destination=destination)
 order_character=next(r for r in bases if r['name']=='policy_27c710_none_True_0')
 convert(order_character,'explicit_order_character_mask',direct_order=False,action_count=1)
 for base in(r for r in character.inputs()if r['name'].startswith('fault_seed_fallback')):
  convert(base,'actual_unmapped_action_definition_'+base['name']);rows[-1]['actual_read_fault']=True
 for vehicle in(False,True):convert(ordinary,f'fault_seed_vehicle_{vehicle}',vehicle=vehicle)
 convert(ordinary,'fault_seed_alias',destination=character.NAV+0x10)
 return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',type=Path);a=p.parse_args();assert not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']);image=Xbe(NEW_DEFAULT);common.ENTRIES=(0x201500,0x1ecd00,0x1ed700,0x27c6c0);meta=common.metadata(image);owned=character.prepare(image,meta,scratch);subprocess.run(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_navigation_filter_tu15.cpp'),'-o',str(owned/'observer')],check=True);rows=geometry.physical(owned,inputs(),'complete');prefix=[]
 for seed in(r for r in rows if r['name'].startswith('fault_seed')):
  for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
   for n in range(len(events)):
    # OR[flags],1 is a single read-modify-write instruction. PTRACE cannot
    # independently authenticate a stop between its read and store. Retain its
    # before-read and committed after-write prefixes, not a fictional midpoint.
    if phase=='before_write'and events[n]['instruction']==0x201620:continue
    row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
 rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows if r['completed']));uncovered=[s['instruction']for s in meta['sites']if s['instruction']not in visited]
 fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0))
 repeat=None
 if a.verify:repeat=a.output.read_bytes()==a.verify.read_bytes();assert repeat,'Independent original repeat differs'
 report=dict(passed=not uncovered,cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),original_instructions=sum(r['original_instructions']for r in rows),original_sites=len(visited),uncovered_sites=uncovered,owners=meta['owners'],original_sha256=NEW_PIN,fixture=str(a.output),fixture_bytes=a.output.stat().st_size,fixture_sha256=sha(a.output),repeat_identical=repeat,ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),actual_source_faults=sum(r.get('actual_read_fault',False)for r in rows),owned_scratch=str(scratch),retained_large_artifacts=[],atomic_rmw_limit='201620 OR memory operand: before-read and after-write proven; no PTRACE midpoint or intra-instruction MMIO ordering claimed',source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/oracle_native_campaign_ai_character_tu15.py':sha(ROOT/'tools/oracle_native_campaign_ai_character_tu15.py'),'tools/observe_native_campaign_ai_navigation_filter_tu15.cpp':sha(ROOT/'tools/observe_native_campaign_ai_navigation_filter_tu15.cpp')})
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');assert not uncovered,uncovered;print(f'{len(rows)} physical navigation-filter comparisons, {len(visited)} sites; fixture {a.output.stat().st_size} bytes')
if __name__=='__main__':main()
