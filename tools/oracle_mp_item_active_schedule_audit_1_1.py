#!/usr/bin/env python3
"""Read-only audit of original active-item schedule and stored outputs."""
import argparse,hashlib,json
from pathlib import Path
from oracle_mp_item_active_schedule_1_1 import ROOT,OUT,digest,encoded,frozen_write
from xbe import Xbe

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output-dir',type=Path,default=OUT);args=ap.parse_args();out=args.output_dir
 d=json.loads((out/'report.json').read_text());assert digest(ROOT/'tools/oracle_mp_item_active_schedule_1_1.py')==d['oracle_sha256']
 for p,h in d['source_pins'].items():assert digest(ROOT/p)==h,p
 assert not d['instruction_patches'] and not d['new_supplied_pose_contact_activation_fields']
 owner=json.loads((ROOT/'analysis/multiplayer/integration/spawn-source/player-visibility/report.json').read_text())
 owner_case=next(c for c in owner['cases']if c['case']==d['owning_prefix_correlation']['case'])
 assert d['player_prefix_stages']==owner_case['stages'][:4]
 assert d['owning_prefix_correlation']['byte_identical_original_stage_records']
 x=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');sites=set();instruction_rows=0
 for listing in d['instructions']:
  p=out/listing['file'];assert digest(p)==listing['sha256'];lines=p.read_text().splitlines();assert len(lines)==listing['rows']
  for line in lines:
   a,raw,*_=line.split();a=int(a,16);raw=bytes.fromhex(raw);assert x.read(a,len(raw))==raw;sites.add(a)
  instruction_rows+=len(lines)
 assert all(int(a,16)in sites for a in d['executed_addresses'])
 states=d['states'];objects=0
 for s in states:
  raw=json.dumps({k:v for k,v in s.items()if k not in ('id','sha256')},sort_keys=True,separators=(',',':')).encode()
  assert hashlib.sha256(raw).hexdigest()==s['sha256']
  pool=s['full_object_header_pool'];headers=bytes.fromhex(pool['data_hex']);bitmap=bytes.fromhex(pool['bitmap_hex'])
  assert pool['capacity']==2048 and pool['stride']==12 and len(headers)==2048*12 and len(bitmap)==256
  assert hashlib.sha256(headers).hexdigest()==s['current']['object_pool']['data_sha256']
  assert pool['header4c_hex']==s['current']['object_pool']['header_hex']
  assert len(bytes.fromhex(s['player']['unit_queue1d88_hex']))==0x1d88
  context=s['object_context'];assert context['global_address']=='004e58c4' and context['pointer']==s['current']['object_world']
  assert context['raw84_hex']==s['current']['object_world_hex'] and bytes.fromhex(context['raw84_hex'])[3]==int(context['byte03'],16)
  bydatum={o['datum']:o for o in s['current']['objects']}
  for o in bydatum.values():
   h,b=bytes.fromhex(o['header_hex']),bytes.fromhex(o['body_hex']);assert len(h)==12 and len(b)==o['body_size']
   assert int.from_bytes(h[:2],'little')==int(o['datum'][:4],16)
   assert int.from_bytes(h[6:8],'little')==len(b) and int.from_bytes(h[8:12],'little')==int(o['body_address'],16)
   index=int(o['datum'][4:],16);assert headers[index*12:index*12+12]==h and bitmap[index//8]&1<<(index%8)
   assert b[0x64:0x70].hex()==''.join(int(v,16).to_bytes(4,'little').hex()for v in o['root']['bits'])
   if 'row'in o:assert b[0xaf]==o['row'];assert b[0x130:0x13c].hex()==o['contact_hex']
   objects+=1
  for c in s['current']['registry_chains']:
   for kind in ('collidable','noncollidable'):
    chain=c[kind]
    for i,r in enumerate(chain):
     raw=bytes.fromhex(r['raw_hex']);assert len(raw)==32 and r['object']in bydatum
     assert int.from_bytes(raw[4:8],'little')==int(r['object'],16)
     assert int.from_bytes(raw[8:12],'little')==int(r['next'],16)
     assert r['next']==(chain[i+1]['link']if i+1<len(chain)else'ffffffff')
 stage_names=[r['name']for r in d['stages']]
 assert stage_names==['original_netgame_row_initialization','original_netgame_round_reset',
  'original_authored_weapon_row_0_allocation','original_authored_weapon_row_1_allocation','original_authored_weapon_row_12_allocation',
  'original_source_player_PVS_and_object_activation','original_ordinary_object_pass','original_world_physics_pass',
  'original_dirty_object_pass','original_new_object_pass']
 assert all(r['completion']=='complete_original_return'and r['stopped_eip']=='03000000'for r in d['stages'])
 assert all(not[e for e in r['services']if e.get('service')]for r in d['stages'][5:])
 activated=states[d['stages'][5]['after_state']];final=states[d['stages'][-1]['after_state']]
 assert activated['current']['active_mask_hex']==owner_case['stages'][4]['world']['source_active_mask_hex']
 assert activated['player']['records'][0]['cluster']==4
 assert len(final['current']['objects'])==5
 world={o['row']:o['datum']for o in final['current']['objects']if o.get('row')in(0,1,12)}
 assert world=={0:'e2710002',1:'e2720003',12:'e2730004'}
 for s in (activated,final):
  assert {r['index']:f'{int.from_bytes(bytes.fromhex(r["raw_hex"])[:4],"little"):08x}'for r in s['map_weapon_rows']}==world
 for row,datum in world.items():
  before=next(o for o in activated['current']['objects']if o['datum']==datum);after=next(o for o in final['current']['objects']if o['datum']==datum)
  assert int(before['header_hex'][4:6],16)==0x4b and not int(after['header_hex'][4:6],16)&8
  assert before['owner']==after['owner']=='ffffffff' and before['parent']==after['parent']=='ffffffff'
  assert before['physical_component']==after['physical_component']=='ffffffff'
  assert before['loaded']==after['loaded'] and before['reserve']==after['reserve'] and before['heat_bits']==after['heat_bits']=='00000000'
 assert all('returned_eax'in c and 'return_order'in c and c['order']<c['return_order']for c in d['calls'])
 for c in d['calls']:
  if 'before_state'in c:assert 0<=c['before_state']<len(states)and 0<=c['after_state']<len(states)
 new=[c for c in d['calls']if c['phase']=='original_new_object_pass']
 assert [c['stack'][0]for c in new if c['entry']=='000bd360']==['e26f0000','e2700001','e2710002','e2720003','e2730004']
 assert [c['stack'][0]for c in new if c['entry']=='000f9ea0']==list(world.values())
 for datum in world.values():
  callbacks=[c['target']for c in d['callbacks']if c['phase']=='original_new_object_pass'and c['instruction']=='0010ce3b'and c['datum_argument']==datum]
  assert callbacks==['000f9b10','001021d0']
 for r in d['stores']:
  assert int(r['instruction'],16)in sites
  assert len(bytes.fromhex(r['before_hex']))==len(bytes.fromhex(r['written_hex']))==r['size']
 outer=[c for c in new if c['entry']=='0016b240'];lower=[c for c in new if c['entry']=='001e20c0'];shape=[c for c in new if c['entry']=='00246a50']
 assert len(outer)==2 and len(lower)==3 and len(shape)==2
 assert [int.from_bytes(bytes.fromhex(c['outer_result_hex'])[:4],'little')for c in outer]==[0,1]
 assert [int(c['returned_eax'],16)&255 for c in outer]==[0,1]
 assert [c['visited_leaves']for c in lower]==[[1352],[2243],[2243]]
 assert all(c['outer_mask']=='24909c0d'and c['outer_ignored']==['ffffffff','ffffffff']for c in outer)
 assert [c['source_cluster_a8_count']for c in shape]==[624,96]
 assert all(int.from_bytes(bytes.fromhex(c['actual_cluster_cache']['raw_hex'])[0xa8:0xac],'little')==c['source_cluster_a8_count']and int(c['returned_eax'],16)&255==0 for c in shape)
 assert {'00246aae','00246c26','002e5ce0','002e5ca0'}<=set(d['executed_addresses'])
 context_stores=[r for r in d['stores']if r.get('source_target',{}).get('kind')=='object_context'and r['source_target']['offset']=='03']
 assert [(r['instruction'],r['written_hex'])for r in context_stores]==[
  ('000b7e24','01'),('000b7f43','00'),('000b7f6e','01'),('000b7fef','00'),('000b804f','01'),('000b80d5','00')]
 cache=ROOT/'assets/disc/maps/lockout.map'
 records=[c['actual_cluster_cache']for c in shape]
 for c in outer:
  if 'actual_contact_cache'in c:records.extend(c['actual_contact_cache'][k]for k in('surface_record','plane_record','material_record'))
 with cache.open('rb')as f:
  for r in records:
   raw=bytes.fromhex(r['raw_hex']);f.seek(int(r['file_offset'],16));assert f.read(len(raw))==raw
 final_chains={c['cluster']:[r['object']for r in c['collidable']]for c in final['current']['registry_chains']if c['collidable']}
 assert final_chains=={4:['e26f0000'],9:['e2720003','e2710002','e26f0000'],15:['e2730004']}
 result=dict(schema=1,report_sha256=digest(out/'report.json'),auditor_sha256=digest(__file__),prior_pins=len(d['source_pins']),
  original_player_prefix_records_verified=4,stages=len(d['stages']),calls=len(d['calls']),states=len(states),stored_objects=objects,
  stores=len(d['stores']),callbacks=len(d['callbacks']),original_instruction_files=len(d['instructions']),original_instruction_rows=instruction_rows,
  executed_original_addresses=len(d['executed_addresses']),source_world_item_ticks=3,actual_outer_queries=2,actual_lower_queries=3,
  original_cluster_query_misses=2,actual_terrain_hits=1,object_context_byte03_stores=len(context_stores),scheduled_phase_supplied_game_or_Havok_returns=0,
  retained_full_map_datums_and_unsorted_registry_verified=True,instruction_patches=[])
 frozen_write(out/'audit.json',encoded(result));print(json.dumps(result,sort_keys=True))
if __name__=='__main__':main()
