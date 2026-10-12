#!/usr/bin/env python3
"""Read-only old pins; audit/export only the independent cache provider freeze."""
import hashlib,json,struct
from pathlib import Path
from oracle_smg_audio_cache_1_1 import ROOT,PIN,CACHE_PIN,ENTRIES,SERVICES
from oracle_smg_firing_1_1 import Xbe,Cache
from audit_smg_audio_adapter_1_1 import PRIOR_PINS as EARLIER_PINS
BASE=ROOT/'analysis/multiplayer/map-pack/smg-audio-cache'
ORACLE_PIN='9c546c136b0056a741b1d5c1c519cc476f4cbdaacd6c346bc3661d63fdf262a1'
SUITE_PIN='69baec9c569ba241cdf4fa5f911658d284f9e3241cd606ef659b1898d16eee83'
SHARED_PIN='084c817fa82bb9bc416d9ba847cec05be431abc419e3a1087cdf211fbf47f5af'
PRIOR_PINS={**EARLIER_PINS,
 'tools/oracle_smg_audio_adapter_1_1.py':'5e0a4af31c9a7689da2d753a6bb0cb77f244ca35daa92cc702941e4f239c264d',
 'tools/audit_smg_audio_adapter_1_1.py':'088b1fad8e90ed0f8b649bbeeb5fdeb92410c7f18a2f99e0259b3d846055f302',
 'docs/SMG_AUDIO_ADAPTER_1_1_RESEARCH.md':'9bedf802da03ac8981a3b08b043ad3422a2bd9bfc2363e20d6803b44da24fdb3',
 'analysis/multiplayer/map-pack/smg-audio-adapter/frozen/suite.json':'007109dc2ec4447c016b2c604856f9c2bbc0e4f051b7c8004475213d628ec34a',
 'analysis/multiplayer/map-pack/smg-audio-adapter/source-fixture.json':'525ed0246e4271a14b23c8148739010f18e1c28955889631cc35ed6781300a22',
 'analysis/multiplayer/map-pack/smg-audio-adapter/audit.json':'1af37ba841eb0b983bb3bea7b23d1880fd47ed5c38c8f9f49c521ff283618146',
}
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def u(b,o,n=4):return int.from_bytes(b[o:o+n],'little')
def entry(s):return bytes.fromhex(s['pools'][0]['data_hex'])
def current_entry(s):return bytes.fromhex(s['selected_chunk']['cache_entry_10_hex'])
def check_original_record(c,r):
 b=bytes.fromhex(r['raw_hex']);o=r['file_offset']
 assert bytes(c.data[o:o+len(b)])==b and int(r['original_va'],16)==o+c.magic
def main():
 assert sha(ROOT/'tools/oracle_smg_audio_cache_1_1.py')==ORACLE_PIN
 assert sha(BASE/'frozen/suite.json')==SUITE_PIN
 for p,h in PRIOR_PINS.items():assert sha(ROOT/p)==h,p
 x=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');c=Cache(ROOT/'assets/disc/maps/lockout.map')
 assert hashlib.sha256(x.data).hexdigest()==PIN and hashlib.sha256(c.data).hexdigest()==CACHE_PIN
 shared_path=ROOT/'assets/disc/maps/shared.map';assert sha(shared_path)==SHARED_PIN
 suite=json.loads((BASE/'frozen/suite.json').read_text());reports={};listings=insrows=metadata=readbytes=rng=0
 for case in suite['cases']:
  p=ROOT/case['path'];q=ROOT/case['reexecution_path'];assert sha(p)==case['sha256'] and p.read_bytes()==q.read_bytes()
  r=json.loads(p.read_text());reports[case['case']]=r
  assert r['oracle_sha256']==ORACLE_PIN and r['xbe_sha256']==PIN and r['cache_sha256']==CACHE_PIN
  assert not r['audio_available_forced'] and not r['real_audio_device_or_driver_executed']
  assert not r['original_async_thread_execution_claimed'] and not r['source_cache_fields_or_completion_flags_written_by_fixture']
  assert not r['native_gameplay_update_applied']
  assert not set(f'{a:08x}'for a in ENTRIES)&set(r['supplied_inherited_services'])
  assert [d['instruction']for d in r['boundaries']]==['00336983','00337a45','002d8606']
  assert all(not d['platform_result_supplied']for d in r['boundaries'])
  assert r['boundaries'][-1]['original_stack_args']==['00000000','00004000','00122350','00000000','00000000','00000000']
  initial=r['setup']['state'];pool=bytes.fromhex(initial['pools'][0]['header_hex']);allocator=bytes.fromhex(initial['page_allocator_70_hex'])
  assert initial['audio_initialized']==0 and initial['driver_pointer']=='00000000'
  assert u(pool,0x20)==512 and u(pool,0x24)==16 and pool[0x29]==0 and pool[0x2A]&4
  assert u(pool,0x38)==u(pool,0x3C)==0
  assert u(allocator,0x20)==0x21AAF0 and u(allocator,0x24)==0x21AAB0
  assert [u(allocator,i)for i in(0x2C,0x30,0x34,0x38,0x3C,0x40)]==[2,192,14,1,0xFFFFFFFF,0xFFFFFFFF]
  assert [u(allocator,i)for i in range(0x44,0x64,4)]==[0x7FFFFFFF]*8
  assert allocator[0x68:0x6C]==b'eeew'
  pagepool=bytes.fromhex(initial['page_pool']['header_hex']);assert u(pagepool,0x20)==512 and u(pagepool,0x24)==24 and pagepool[0x29]==1
  assert initial['globals']['005098e0']=='82e45000'
  async_bytes=bytes.fromhex(initial['async_2580_hex']);assert len(async_bytes)==150*64
  for n in range(150):
   b=async_bytes[n*64:(n+1)*64];assert u(b,4)==0xFFFFFFFF
   assert u(b,0x3C)==(0x4E7938+(n+1)*64 if n<149 else 0)
  assert initial['globals']['004e9eb8']=='004e7938' and initial['globals']['004e9ebc']=='00000000'
  assert r['setup']['file_binding_input']['file_sha256']==SHARED_PIN
  provider=r['provider_calls'];assert [d['original_args']for d in provider if d['provider']=='VirtualAlloc']==[
   ['00000000','0000208c','00101000','00000004'],['00000000','000030fc','00101000','00000004']]
  assert all(d['real_host_service'] and not d['source_constructor_result_fabricated']for d in provider)
  assert not any(d['provider']in('CreateThread','GetTickCount','DirectSoundCreate')for d in provider)
  reads=[d for d in provider if d['provider']=='ReadFile'];completion=[d for d in r['writes']if d['instruction']=='001223ac']
  assert len(reads)==len(completion)
  with shared_path.open('rb')as stream:
   for d in reads:
    assert d['file_offset']%512==0 and d['bytes_read']%512==0 and d['completion_flag_not_written_by_provider']
    stream.seek(d['file_offset']);b=stream.read(d['bytes_read']);assert hashlib.sha256(b).hexdigest()==d['raw_sha256'];readbytes+=len(b)
  for d in completion:assert d['size']==1 and d['before_hex']=='00' and d['after_hex']=='01'
  for d in r['rng_stores']:
   assert (int(d['before'],16)*0x19660D+0x3C6EF35F)&0xFFFFFFFF==int(d['after'],16);rng+=1
  states=[initial,r['queued'],r['completed'],r['final_state']]
  for d in r['lifecycle_probes']:states.extend((d['before'],d['after']))
  if r['repeated_probe']:
   d=r['repeated_probe'];states.extend((d['before_second_creation'],d['after_second_creation'],*d['worker_iterations']))
   for z in d['unpin_steps']:states.extend((z['before'],z['after']))
  for s in states:
   assert s['audio_initialized']==0 and s['driver_pointer']=='00000000'
   if 'selected_chunk'in s and s['selected_chunk']['datum']!='ffffffff':
    d=s['selected_chunk'];chunk=bytes.fromhex(d['runtime_0c_hex']);o=int(d['pointer'],16)-c.magic
    assert chunk[:8]==bytes(c.data[o:o+8])
    size=u(chunk,4)&0x3FFFFFFF;assert d['buffer']['authored_bytes']==size
    if current_entry(s)[2]:
     with shared_path.open('rb')as stream:
      stream.seek(u(chunk,0)&0x3FFFFFFF);b=stream.read((size+511)&~511)
     assert d['buffer']['aligned_sha256']==hashlib.sha256(b).hexdigest()
     assert d['buffer']['authored_sha256']==hashlib.sha256(b[:size]).hexdigest()
  inventory_path=ROOT/r['instruction_inventory']['path'];assert sha(inventory_path)==r['instruction_inventory']['sha256']
  for routine in json.loads(inventory_path.read_text())['routines']:
   p=ROOT/routine['path'];assert sha(p)==routine['sha256'];assert routine['host_provider_entry_instructions_substituted']==(int(routine['entry'],16)in SERVICES)
   for line in p.read_text().splitlines():
    a,raw,*_=line.split();b=bytes.fromhex(raw);assert x.read(int(a,16),len(b))==b;insrows+=1
   listings+=1
  for sound in r['authored_sound']['sounds']:
   for key in('graph_reference','definition','playback_parameters','scale'):check_original_record(c,sound[key]);metadata+=1
   for pitch in sound['ranges']:
    for key in('range','pitch_parameters'):check_original_record(c,pitch[key]);metadata+=1
    for permutation in pitch['permutations']:
     check_original_record(c,permutation['record']);metadata+=1
     for d in permutation['chunks']:
      check_original_record(c,d['record']);metadata+=1
      with (ROOT/d['file']).open('rb')as stream:stream.seek(d['file_offset']);b=stream.read(d['size_from_original_21abd0_mask'])
      assert hashlib.sha256(b).hexdigest()==d['raw_sha256']
  created=r['created'];assert created['actual_rejected_fp_request']['return_eax']=='ffffffff' and created['creation_kernel_call_site_supplied']
  assert created['original_listener']==0 and created['created_original_source_datum']=='ef730000'
  assert entry(r['queued'])[2:6]==b'\0\0\1\0'
  assert entry(r['completed'])[2:6]==bytes((1,0,2 if case['case']=='reload_shared'else 1,0))
  assert r['final_state']['globals']['004e9ebc']=='00000000'
  if r['lifecycle_probes']:
   probes={d['name']:d for d in r['lifecycle_probes']}
   for name in('pending_pinned_busy','pending_unpinned_still_busy','loaded_pinned_busy'):assert int(probes[name]['return_eax'],16)&255==1
   assert int(probes['loaded_unpinned_not_busy']['return_eax'],16)&255==0
   for name in('pending_full_capacity_request_rejected','loaded_pinned_full_capacity_request_rejected'):assert probes[name]['return_eax']=='ffffffff'
   assert probes['pending_query_without_pin']['return_eax']=='00000001'
   assert probes['loaded_query_marks_used_without_pin']['return_eax']=='00000002'
   assert current_entry(probes['loaded_query_marks_used_without_pin']['after'])[3]==1
   assert probes['loaded_full_capacity_request_evicts_unpinned_entry']['return_eax']=='e2790000'
   assert r['final_state']['selected_chunk']['datum']=='ffffffff' and not entry(r['final_state'])
   assert any(d['entry']=='0021aaf0'for d in r['calls'])
  elif not r['repeated_probe']:assert entry(r['final_state'])[4]==0
 queued=reports['ready_queued'];shared=reports['reload_shared']
 assert len(queued['repeated_probe']['worker_iterations'])==2 and len(shared['repeated_probe']['worker_iterations'])==1
 assert [d['file_offset']for d in queued['provider_calls']if d['provider']=='ReadFile']==[0x4C46400,0x4C39200]
 assert entry(shared['repeated_probe']['after_second_creation'])[4]==2
 assert [entry(d['after'])[4]for d in shared['repeated_probe']['unpin_steps']]==[1,0]
 assert [int(d['busy_probe']['return_eax'],16)&255 for d in shared['repeated_probe']['unpin_steps']]==[1,0]
 fixture={'oracle_sha256':ORACLE_PIN,'suite_sha256':SUITE_PIN,'xbe_sha256':PIN,'cache_sha256':CACHE_PIN,'shared_sha256':SHARED_PIN,
  'cases':[{ 'case':k,'setup':r['setup'],'created':r['created'],'queued':r['queued'],'completed':r['completed'],
   'lifecycle_probes':r['lifecycle_probes'],'repeated_probe':r['repeated_probe'],'final_state':r['final_state'],
   'source_cache_calls':r['calls'],'source_cache_writes':r['writes'],'real_host_provider_calls':r['provider_calls'],
   'rng_stores':r['rng_stores'],'boundaries':r['boundaries'],'instruction_inventory':r['instruction_inventory']}for k,r in reports.items()],
  'actual_static_backing_file_registration_and_direct_kernel_call_sites_remain_explicit_inputs':True,
  'full_original_async_thread_scheduler_audio_device_or_playback_proven':False,'native_gameplay_update_applied':False}
 p=BASE/'source-fixture.json';p.write_text(json.dumps(fixture,indent=2)+'\n')
 audit={'oracle_sha256':ORACLE_PIN,'suite_sha256':SUITE_PIN,'source_fixture_sha256':sha(p),
  'independent_process_reexecutions':6,'instruction_listings':listings,'original_instruction_rows':insrows,
  'original_metadata_records':metadata,'actual_aligned_read_callbacks':sum(sum(d['provider']=='ReadFile'for d in r['provider_calls'])for r in reports.values()),
  'actual_aligned_file_bytes_verified':readbytes,'original_completion_stores':sum(sum(d['instruction']=='001223ac'for d in r['writes'])for r in reports.values()),
  'original_lifecycle_probes':sum(len(r['lifecycle_probes'])for r in reports.values()),'original_lcg_stores':rng,
  'earlier_pins_verified':PRIOR_PINS,'cache_or_completion_fields_forced':False,'original_async_thread_scheduler_audio_device_or_playback_proven':False,
  'native_gameplay_update_applied':False}
 (BASE/'audit.json').write_text(json.dumps(audit,indent=2)+'\n');print(json.dumps(audit,indent=2))
if __name__=='__main__':main()
