#!/usr/bin/env python3
"""Audit the independent source-created Lockout prop/collision dependency freeze."""
import argparse, hashlib, json, struct
from pathlib import Path
from oracle_mp_sandbox_owner_1_1 import ROOT, FROZEN, PIN, CACHE_PIN
from oracle_smg_firing_1_1 import Xbe, _Code

def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--suite',type=Path,default=ROOT/'analysis/multiplayer/sandbox-source/owner-creation/frozen/suite.json');args=ap.parse_args()
 suite=json.loads(args.suite.read_text());count=0
 for p,h in suite['files'].items():assert sha(ROOT/p)==h,p;count+=1
 for p,h in FROZEN.items():assert sha(ROOT/p)==h,p
 prior=json.loads((ROOT/'analysis/multiplayer/sandbox-source/frozen/suite.json').read_text())
 # Earlier artifact manifest uses an array of file/path records.
 old=prior.get('files',prior.get('artifacts'))
 if isinstance(old,dict):
  for p,h in old.items():assert sha(ROOT/p)==(h if isinstance(h,str)else h['sha256']),p
 else:
  for r in old:assert sha(ROOT/r.get('path',r.get('file')))==r['sha256'],r
 image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');code=_Code(image,'.text')
 reports={};rows=0;addresses=set();stores=0
 for name in ('power-serialized','power-fixed','lift-fixed'):
  a=json.loads((ROOT/suite['reports'][name]['measured']).read_text());b=json.loads((ROOT/suite['reports'][name]['repeat']).read_text());assert a==b,name
  assert a['oracle_sha256']==sha(ROOT/'tools/oracle_mp_sandbox_owner_1_1.py')
  assert a['xbe_sha256']==PIN and a['cache_sha256']==CACHE_PIN
  assert a['native_gameplay_update_applied']is False and a['full_original_scene_world_contact_explosion_lift_or_hardware_parity_proven']is False
  assert a['case']['object_header_body_pose_contacts_flags_or_allocation_success_supplied']is False
  for listing in a['instruction_listings']:
   p=(ROOT/suite['reports'][name]['measured']).parent/'instructions'/listing['file'];assert sha(p)==listing['sha256']
   for line in p.read_text().splitlines():
    s=line.split();address=int(s[0],16);raw=bytes.fromhex(s[1]);assert image.read(address,len(raw))==raw;rows+=1
  for s in a['source_executed_addresses']:
   address=int(s,16);i=code.at(address);assert i is not None and image.read(address,len(i.bytes))==i.bytes;addresses.add(address)
  for s in a['case']['ordered_stores']:
   assert code.at(int(s['instruction'],16))is not None;stores+=1
  reports[name]=a
 s=reports['power-serialized'];assert not s['case']['completed']and s['case']['stopped_eip']=='001d9994'
 assert s['case']['invalid']['address']=='0000000a'and s['selected_original_physics_postprocess']is None
 p=reports['power-fixed'];assert p['case']['completed']and p['case']['returned_eax']=='e26f0000'
 assert p['case']['after']['pool_count']==1 and p['case']['after']['objects'][0]['component']=='e1680000'
 fix=p['selected_original_physics_postprocess'];assert fix['tag']['datum']=='e7090594'and not fix['whole_map_postprocess_claimed']
 r=next(x for x in fix['records']if x['bank']=='038');assert struct.unpack_from('<I',bytes.fromhex(r['before_hex']),0x38)[0]==4
 assert struct.unpack_from('<I',bytes.fromhex(r['after_hex']),0x38)[0]==0x8025cc40
 hits=[]
 assert len(p['direct_source_collision_cases'])==9
 for q in p['direct_source_collision_cases']:
  assert q['completed']and q['used_services']==[]and q['original_16a560_converted_mask']==9
  assert q['direct_selected_object_query_not_full_world_broadphase']is True
  raw=bytes.fromhex(q['output5c_hex']);hit=int(q['return_eax'],16)&255
  if hit:
   assert q['explicit_query_side_offset']==0 and struct.unpack_from('<H',raw,0x24)[0]==86
   assert struct.unpack_from('<I',raw,0x40)[0]==0xe26f0000
   hits.append({'axis':q['explicit_query_axis'],'fraction_bits':raw[4:8][::-1].hex(),'output5c_hex':raw.hex()})
  else:assert q['explicit_query_side_offset']!=0 and raw[4:8].hex()=='0000803f'
 assert [x['fraction_bits']for x in hits]==['3eef9db2','3ef62b6b','3ecb923b']
 l=reports['lift-fixed'];assert not l['case']['completed']and l['case']['stopped_eip']=='0016d2c3'
 assert l['case']['invalid']['address']=='00000034'
 assert l['case']['after']['pool_count']==1 and l['case']['after']['objects'][0]['definition']=='e6ed0578'
 assert l['case']['after']['objects'][0]['component']=='e1680000'
 out={'suite_sha256':sha(args.suite),'artifact_count':count,'earlier_direct_pins_verified':len(FROZEN),'independently_repeated_original_cases':3,'source_instruction_rows_verified':rows,'unique_executed_original_addresses':len(addresses),'ordered_source_stores_checked':stores,'source_created_prop_queries':9,'center_hits':hits,'offset_misses':6,'no_native_gameplay_contact_explosion_lift_hardware_or_latest_update_claim':True}
 target=args.suite.parent/'audit.json';target.write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out))
if __name__=='__main__':main()
