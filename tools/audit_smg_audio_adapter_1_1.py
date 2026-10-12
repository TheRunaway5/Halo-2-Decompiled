#!/usr/bin/env python3
"""Read-only source-byte/independent-run audit for bounded audio adapter evidence."""
import hashlib,json,struct
from pathlib import Path
from oracle_smg_audio_adapter_1_1 import ROOT,PIN,CACHE_PIN,ENTRIES
from oracle_smg_firing_1_1 import Xbe,Cache
BASE=ROOT/'analysis/multiplayer/map-pack/smg-audio-adapter'
ORACLE_PIN='5e0a4af31c9a7689da2d753a6bb0cb77f244ca35daa92cc702941e4f239c264d'
SUITE_PIN='007109dc2ec4447c016b2c604856f9c2bbc0e4f051b7c8004475213d628ec34a'
PRIOR_PINS={
 'tools/oracle_smg_fp_sound_1_1.py':'44777fe55191fe36ccdef4cca53afa3ab3b5720057bc18229ce8e9698111e4a6',
 'tools/audit_smg_fp_sound_1_1.py':'0527d6faf0e3e7249234ba8c5489ba619dc36742797e4cc438df1592b95660de',
 'docs/SMG_FP_SOUND_1_1_RESEARCH.md':'7198fdfbba13916ac8e3e8b20fd956a0aa3c1669ca43394cb72ccb7fedd87c04',
 'analysis/multiplayer/map-pack/smg-fp-sound/frozen/suite.json':'f290a4fafe336efa899518e460ec7cbcaf6331e017e8c964067727924ae96641',
 'analysis/multiplayer/map-pack/smg-fp-sound/source-fixture.json':'52b5571c3763402c0624688a44a73c51777e5e3541987a5d02d7005b9905360d',
 'analysis/multiplayer/map-pack/smg-fp-sound/audit.json':'c9991cc95fbd488c73da52d34b7314686980aecd3d15965d3c5c7e943d99dbcc',
 'tools/oracle_smg_fp_clock_1_1.py':'7d5548e26d3503ea1057aef3202a9285d7e9fa431d0a943c372788b64282b5d6',
 'tools/oracle_smg_fp_registration_1_1.py':'06c0ddfece364a4361b60219b4f2ab26e98acc693aacccbe16eb89f8d0535e92',
 'tools/oracle_smg_constructor_equipped_1_1.py':'1c6ce60d4576f37548f822bfbd5667f3e97d806926db593cd98cb9d7f6c9eb99',
 'tools/oracle_mp_weapon_attachment_1_1.py':'9895480d4d864d67ee92307d627c04a005e4ffb8f06761be68c9de0d7274563c',
 'tools/oracle_mp_inventory_equipped_1_1.py':'47f56a9236ea2efe870d1ad82af16ace214f4598294f8c826966cced94dee655',
}
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def u(b,o,n=4):return int.from_bytes(b[o:o+n],'little')
def record(cache,r):
 raw=bytes.fromhex(r['raw_hex']);p=r['file_offset']
 assert bytes(cache.data[p:p+len(raw)])==raw
 assert int(r['original_va'],16)==p+cache.magic

def main():
 assert sha(ROOT/'tools/oracle_smg_audio_adapter_1_1.py')==ORACLE_PIN
 assert sha(BASE/'frozen/suite.json')==SUITE_PIN
 for p,h in PRIOR_PINS.items():assert sha(ROOT/p)==h,p
 image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
 cache=Cache(ROOT/'assets/disc/maps/lockout.map')
 assert hashlib.sha256(image.data).hexdigest()==PIN
 assert hashlib.sha256(cache.data).hexdigest()==CACHE_PIN
 suite=json.loads((BASE/'frozen/suite.json').read_text());reports={};listings=insrows=metadata=draws=0
 for case in suite['cases']:
  p=ROOT/case['path'];q=ROOT/case['reexecution_path']
  assert sha(p)==case['sha256'] and p.read_bytes()==q.read_bytes(),case['case']
  r=json.loads(p.read_text());reports[case['case']]=r
  assert r['oracle_sha256']==ORACLE_PIN and r['xbe_sha256']==PIN and r['cache_sha256']==CACHE_PIN
  assert not r['original_sound_availability_forced'] and not r['real_audio_platform_provider_executed']
  assert not r['successful_sound_initialization_or_playback'] and not r['native_gameplay_update_applied']
  assert not set(f'{a:08x}'for a in ENTRIES)&set(r['supplied_services'])
  for x in r['boundaries']:assert not x['platform_result_supplied']
  assert r['final_state']['audio']['initialized_78']==0
  expected=[(384,188),(64,28),(128,212),(16,20),(51,108)]
  for actual,(capacity,stride)in zip(r['final_state']['pools'],expected):
   assert (actual['capacity'],actual['stride'],actual['valid'])==(capacity,stride,1)
  assert r['source_driver_policy_451220_hex']==image.read(0x451220,16).hex()
  system=bytes.fromhex(r['final_state']['audio']['sound_system_20c_hex'])
  assert u(system,0x208,2)==73 and u(system,0x20A,2)==51
  assert [x['instruction']for x in r['boundaries'][:3]]==['0033786f','00337a45','00336983']
  for d in r['rng_stores']:
   assert (int(d['before'],16)*0x19660D+0x3C6EF35F)&0xFFFFFFFF==int(d['after'],16)
   draws+=1
  inventory_path=ROOT/r['instruction_inventory']['path']
  assert sha(inventory_path)==r['instruction_inventory']['sha256']
  inventory=json.loads(inventory_path.read_text())
  for routine in inventory['routines']:
   p=ROOT/routine['path'];assert sha(p)==routine['sha256']
   for line in p.read_text().splitlines():
    address,raw,*_=line.split();b=bytes.fromhex(raw)
    assert image.read(int(address,16),len(b))==b
    insrows+=1
   listings+=1
  for x in r['adapter_metadata']['classes']:record(cache,x);metadata+=1
  table=r['adapter_metadata']['owner_callback_table']
  assert bytes.fromhex(table['raw_hex'])==image.read(int(table['original_va'],16),32)
  for address,raw in r['adapter_metadata']['constants'].items():
   assert image.read(int(address,16),len(bytes.fromhex(raw)))==bytes.fromhex(raw)
  for sound in r['authored_sound']['sounds']:
   for key in ('graph_reference','definition','playback_parameters','scale'):record(cache,sound[key]);metadata+=1
   for pr in sound['ranges']:
    for key in ('range','pitch_parameters'):record(cache,pr[key]);metadata+=1
    for perm in pr['permutations']:
     record(cache,perm['record']);metadata+=1
     for chunk in perm['chunks']:
      record(cache,chunk['record']);metadata+=1
      with (ROOT/chunk['file']).open('rb')as stream:
       stream.seek(chunk['file_offset']);raw=stream.read(chunk['size_from_original_21abd0_mask'])
      assert hashlib.sha256(raw).hexdigest()==chunk['raw_sha256']
 ready=reports['ready_select']['selection_probes'];reload=reports['reload_select']['selection_probes']
 assert [x['selected_permutation']for x in ready]==[0,1]*8
 assert sum(len(x['rng_stores'])for x in ready)==45
 assert [len(x['rng_stores'])for x in ready]==[2,2,2,3,7,2,2,2,2,4,2,6,3,2,2,2]
 assert [x['selected_permutation']for x in reload]==[0]*16
 assert [len(x['rng_stores'])for x in reload]==[0]+[2]*15
 assert reload[0]['before']['ranges'][0]['range_runtime_hex'][10:12]=='00'
 creations=[]
 for case,tick,datum,permutation,gain,total in (
  ('ready_create',1,0xE5AC0437,1,'ae52edc0',4),
  ('reload_create',47,0xE5AD0438,0,'000040c0',2),
  ('empty_reload_create',124,0xE5AD0438,0,'000040c0',2)):
  r=reports[case];c=r['source_creation_probe'];p=c['actual_rejected_fp_request'];b=bytes.fromhex(c['created_source_bc_hex'])
  assert p['tick']==tick and int(p['stack'][0],16)==datum and p['return_eax']=='ffffffff'
  assert c['direct_creation_call_site_supplied'] and not c['original_available_admission_succeeded']
  assert not c['source_creation_kernel']['completed'] and not c['successful_playback_claimed']
  assert r['boundaries'][-1]['instruction']=='0013eae0' and r['boundaries'][-1]['cache_allocator_pointer']=='00000000'
  assert u(b,0xC)==datum and u(b,0x10)==0x10002 and u(b,0x14)==0x44B14C
  assert u(b,0x90)==0 and b[0x94:0x98].hex()==gain and u(b,0x98)==0x47502932
  assert list(b[0x9C:0x9E])==[0,permutation] and u(b,0x9E,2)==0 and u(b,0xAC,2)==0xFFFF
  assert len(c['rng_stores'])==total
  assert u(bytes.fromhex(c['after']['class_fade_360_hex']),0)==0
  assert c['after']['class_fade_initialized_flag']==1
  system=bytes.fromhex(c['after']['audio']['sound_system_20c_hex'])
  assert u(system,0x1F8)==0xFFFFFFFF and u(system,0x200)==0xFFFFFFFF
  assert u(system,0x80)==0 and u(system,0x84)==0
  creations.append(c)
 fixture={'oracle_sha256':ORACLE_PIN,'suite_sha256':SUITE_PIN,'xbe_sha256':PIN,'cache_sha256':CACHE_PIN,
  'source_pool_producer_steps':reports['pools']['source_pool_producer_steps'],
  'metadata':reports['pools']['adapter_metadata'],'authored_sound':reports['pools']['authored_sound'],
  'selector_probes':{'ready':ready,'reload':reload},'creation_kernel_probes':creations,
  'original_initialized_flag_forced':False,'accepted_scheduler_request_or_playback_proven':False,
  'native_gameplay_update_applied':False,
  'scope':'Original kernel/pool/request evidence; direct selection and creation call sites are explicit supplied boundaries. Metadata begins with the bytes of the original cache; full loaded-map runtime postprocessing is not established. Real kernel/cache/audio provider outputs remain unavailable.'}
 fp=BASE/'source-fixture.json';fp.write_text(json.dumps(fixture,indent=2)+'\n')
 report={'oracle_sha256':ORACLE_PIN,'suite_sha256':SUITE_PIN,'source_fixture_sha256':sha(fp),
  'independent_reexecutions':6,'instruction_listings':listings,'original_instruction_rows':insrows,
  'original_metadata_records':metadata,'source_pool_producers':5,'static_policy_voice_slots':73,
  'direct_permutation_probes':32,'creation_kernel_probes':3,'lcg_stores_verified':draws,
  'prior_pins_verified':PRIOR_PINS,'initialized_or_platform_success_forced':False,
  'accepted_sound_scheduler_or_playback_proven':False,'native_gameplay_update_applied':False}
 (BASE/'audit.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
