#!/usr/bin/env python3
"""Audit the independently repeated nonlocal FP owner gate without binding input."""
import json
from oracle_smg_audio_fp_owner_1_1 import ROOT,OUT,FROZEN,PIN,CACHE_PIN,digest
from oracle_smg_firing_1_1 import Xbe
from audit_smg_audio_rng_startup_1_1 import source_only

ORACLE_PIN='7884ca932216a3ad5ba049eeef1ab30e0f9214291e3eeb37daa9faee420e4960'
def word(b,o=0,n=4):return int.from_bytes(b[o:o+n],'little')
def main():
 assert digest(ROOT/'tools/oracle_smg_audio_fp_owner_1_1.py')==ORACLE_PIN
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 suite=json.loads((OUT/'frozen/suite.json').read_text())
 for z in suite['artifacts']:assert digest(ROOT/z['path'])==z['sha256'],z['path']
 a=OUT/'measured/report.json';b=OUT/'reexecution/report.json';r=json.loads(a.read_text());s=json.loads(b.read_text())
 assert source_only(r)==source_only(s)
 assert r['oracle_sha256']==s['oracle_sha256']==ORACLE_PIN
 assert r['source_sha256']==s['source_sha256']==PIN and r['cache_sha256']==s['cache_sha256']==CACHE_PIN
 assert r['frozen_prior_pins']==s['frozen_prior_pins']==FROZEN
 assert not r['view_player_weapon_or_event_inputs_supplied']
 assert not r['ordinary_local_FP_sound_request_executed']and not r['native_gameplay_update_applied']
 x=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
 assert digest(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')==PIN
 assert digest(ROOT/'assets/disc/maps/lockout.map')==CACHE_PIN
 prior=ROOT/r['whole_context_and_clock_source_report'];old=json.loads(prior.read_text())
 assert digest(prior.parent/'raw-host-context.json')==r['whole_context_sha256']==old['context_snapshot_sha256']
 assert source_only(r['clock_calls'])==source_only(old['cases'][0]['clock_calls'][:2])
 assert len(r['clock_calls'])==2 and r['context_mode']=='unchanged_whole_measured_compatibility_context_replay'
 assert [z['entry']for z in r['steps']]==['001677c3','001677e5','001678c1']
 assert all(z['completion']=='complete_original_return'for z in r['steps'])
 allocated=r['allocated'];assert allocated['user_player_datums']==['ffffffff']*4
 records=allocated['world']['player']['records'];assert len(records)==1 and records[0]['datum']=='ec700000'and records[0]['current']=='e26f0000'
 objects=allocated['world']['world']['objects'];assert [z['datum']for z in objects]==['e26f0000','e2700001']
 weapon=bytes.fromhex(objects[1]['raw_hex']);unit=bytes.fromhex(objects[0]['raw_hex'])
 assert word(weapon,0x154)==0xE26F0000 and word(unit,0x218)==0xE2700001
 assert all(k not in r['inherited_supplied_services']for k in('001677c3','001677e5','001678c1','0014f5b0','00168866','00169d8a','00169965','00169e15'))
 init=r['steps'][1]['after'];final=r['steps'][2]['after']
 p=int(init['view_storage_pointer'],16);scratch=int(init['scratch_pointer'],16)
 assert p and scratch and len(bytes.fromhex(init['view_storage_8330_hex']))==0x8330
 assert len(bytes.fromhex(init['scratch_8000_hex']))==0x8000
 view=bytes.fromhex(init['view_storage_8330_hex'])
 for v in range(4):
  assert word(view,v*0x20CC+4)==word(view,v*0x20CC+8)==0xFFFFFFFF
  for h in range(2):assert word(view,v*0x20CC+0x10+h*0x1010)==0xFFFFFFFF
 assert source_only(init)==source_only(final)
 ctor=[z for z in r['calls']if z['phase']=='original_fp_storage_constructor_actual_arena']
 assert [z['entry']for z in ctor]==['001677c3','00125490','001254d0']
 assert int(ctor[1]['return_eax'],16)==p and int(ctor[2]['return_eax'],16)==scratch
 queries=[z for z in r['calls']if z['phase']=='original_fp_observer_nonlocal_source_created_player']
 assert [z['entry']for z in queries]==['001678c1']+['0014f5b0']*4
 assert [z['registers']['ecx']for z in queries[1:]]==['00000000','00000001','00000002','00000003']
 assert all(z['return_eax']=='ffffffff'for z in queries[1:])
 assert not any(z['entry']in('00169d8a','00169965','00169e15','000cd1a0','00168866','001687c2','0018aa90')for z in r['calls'])
 assert not any(z['phase']=='original_fp_observer_nonlocal_source_created_player'and p<=int(z['address'],16)<p+0x8330 for z in r['stores'])
 # Owning pointer publication and reset stores are source writes.
 assert any(z['address']=='004f1198'and word(bytes.fromhex(z['after_hex']))==p for z in r['stores'])
 assert any(z['address']=='004f1194'and word(bytes.fromhex(z['after_hex']))==scratch for z in r['stores'])
 static={z['entry']:z for z in r['instruction_listings']};assert '001687c2'in static and '00169e15'in static
 assert x.read(0x16881F,3)==bytes.fromhex('ff7604')
 assert x.read(0x169E4B,3)==bytes.fromhex('897e04')
 listing_count=rows=0
 for report,path in((r,a),(s,b)):
  for z in report['instruction_listings']+[report['executed_original_listing']]:
   p0=path.parent/'instructions'/z['file'];assert digest(p0)==z['sha256']
   for line in p0.read_text().splitlines():
    at,bb,*_=line.split();raw=bytes.fromhex(bb);assert x.read(int(at,16),len(raw))==raw
   if report is r:listing_count+=1;rows+=len(p0.read_text().splitlines())
 fixture={'oracle_sha256':ORACLE_PIN,'suite_sha256':digest(OUT/'frozen/suite.json'),'independently_reexecuted_original_report':r,
  'reexecuted_source_outputs_equal':True,'source_created_nonlocal_view_queries':4,'ordinary_local_FP_sound_request_executed':False,
  'actual_owning_FP_owner_is_registered_hand_weapon_inspection_only':True,'native_gameplay_update_applied':False}
 p0=OUT/'source-fixture.json';p0.write_text(json.dumps(fixture,indent=2)+'\n')
 result={'source_fixture_sha256':digest(p0),'suite_sha256':digest(OUT/'frozen/suite.json'),
  'independent_original_executions':2,'source_created_FP_constructor_and_reset':True,'original_NONE_view_queries_per_execution':4,
  'original_instruction_listings_verified':listing_count,'original_instruction_rows_verified':rows,
  'unique_executed_original_instructions':r['executed_original_listing']['instructions'],
  'earlier_frozen_pins_verified':FROZEN,'view_player_owner_or_event_binding_supplied':False,
  'ordinary_FP_request_or_audio_playback_executed':False,'native_gameplay_update_applied':False}
 (OUT/'audit.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items()if k!='earlier_frozen_pins_verified'},indent=2))
if __name__=='__main__':main()
