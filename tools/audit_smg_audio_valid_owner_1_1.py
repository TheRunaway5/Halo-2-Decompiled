#!/usr/bin/env python3
"""Audit source-created weapon validation and independently replayed audio ownership."""
import hashlib,json,struct
from oracle_smg_audio_valid_owner_1_1 import ROOT,OUT,FROZEN,PIN,CACHE_PIN,digest
from oracle_smg_firing_1_1 import Xbe,Cache
from audit_smg_audio_rng_startup_1_1 import source_only

ORACLE_PIN='e2dc14bfd463dce0e1a2a1362ee745a3f90c9ea27fba98c792df6ad0ec5c974c'
SHARED_PIN='084c817fa82bb9bc416d9ba847cec05be431abc419e3a1087cdf211fbf47f5af'
def word(b,o=0,n=4):return int.from_bytes(b[o:o+n],'little')
def raw(d,k):return bytes.fromhex(d[k])
def main():
 assert digest(ROOT/'tools/oracle_smg_audio_valid_owner_1_1.py')==ORACLE_PIN
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 suite=json.loads((OUT/'frozen/suite.json').read_text())
 for a in suite['artifacts']:assert digest(ROOT/a['path'])==a['sha256'],a['path']
 mp=OUT/'measured/report.json';rp=OUT/'reexecution/report.json'
 r=json.loads(mp.read_text());s=json.loads(rp.read_text());assert source_only(r)==source_only(s)
 assert r['oracle_sha256']==s['oracle_sha256']==ORACLE_PIN
 assert r['source_sha256']==s['source_sha256']==PIN and r['cache_sha256']==CACHE_PIN
 assert r['frozen_prior_pins']==s['frozen_prior_pins']==FROZEN
 assert r['context_mode']=='real_Wine_worker_alive_during_source_execution'and s['context_mode']=='whole_recorded_context_replay'
 assert r['clock_mode']=='real_host_clock_measurement'and s['clock_mode']=='verbatim_measured_input_replay'
 assert not r['native_gameplay_update_applied']
 x=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
 assert hashlib.sha256(x.data).hexdigest()==PIN and digest(ROOT/'assets/disc/maps/lockout.map')==CACHE_PIN
 shared=ROOT/'assets/disc/maps/shared.map';assert digest(shared)==SHARED_PIN
 cache=Cache(ROOT/'assets/disc/maps/lockout.map');reads=total_bytes=0
 assert len(r['cases'])==2
 for c in r['cases']:
  assert not c['object_header_high_water_identity_owner_driver_voice_or_admission_fields_supplied']
  assert not c['ordinary_FP_event_local_binding_complete_map_or_playback_proven']and not c['native_gameplay_update_applied']
  assert c['spawned_player']=='ec700000'and c['spawned_unit']=='e26f0000'and c['source_allocated_weapon']=='e2700001'
  allocated=c['allocated'];pool=raw(allocated,'object_pool_4c_hex')
  assert allocated['object_pool_capacity']==2048 and allocated['object_pool_stride']==12
  assert word(pool,0x20)==2048 and word(pool,0x24)==12 and word(pool,0x38)==word(pool,0x3C)==2
  headers=raw(allocated,'object_pool_data_hex');assert len(headers)==2048*12
  assert word(headers,0,2)==0xE26F and headers[3]==0 and word(headers,12,2)==0xE270 and headers[15]==2
  objs=allocated['world']['objects'];assert [o['datum']for o in objs]==['e26f0000','e2700001']
  assert [o['size']for o in objs]==[5750,1260]
  weapon=bytes.fromhex(objs[1]['raw_hex']);unit=bytes.fromhex(objs[0]['raw_hex'])
  assert word(weapon)==0xE57B0406 and word(weapon,0x154)==0xE26F0000 and word(unit,0x218)==0xE2700001
  records=allocated['player']['records'];assert len(records)==1 and records[0]['datum']=='ec700000'and records[0]['current']=='e26f0000'
  assert [z['entry']for z in c['source_player_stages'][:3]]==['00158c10','0014d350','001509c0']
  assert all(z['completion']=='complete_original_return'for z in c['source_player_stages'][:3])
  # Original source-generated binding and allocator stores, not registry copies.
  stores=c['source_player_stores'];assert any(z['instruction']=='0014e37c'for z in stores)
  assert any(z['instruction']=='000fb18f'and z['kind']=='object'and z['datum']=='e2700001'for z in stores)
  assert len(c['clock_calls'])==5
  assert [z['entry']for z in c['clock_calls']]==['00336983','00337a40','00336983','00337a40','00337a40']
  assert c['initial_rng']['entry']=='001479b0'
  for z in c['clock_calls']:
   assert z['no_source_rng_or_ready_field_written_by_provider'];ns=z['measured_provider_input']['observed_ns']
   if z['entry']=='00336983':assert raw(z,'output_after_hex')==struct.pack('<Q',ns//100+116444736000000000)
   else:assert int(z['return_eax'],16)==(ns//1000000)&0xFFFFFFFF
  assert c['audio_setup'][0]['producer']['stopped_eip']=='002205c0'
  assert [z['instruction']for z in c['audio_boundaries']]==['002217e8','002d8606','0022310c']
  assert all(not z['source_return_device_owner_validity_or_ready_result_supplied']for z in c['audio_boundaries'])
  inp=c['original_caller_static_pointer_inputs']
  assert int(inp['ECX_pointer'],16)==word(x.read(0x46FC28,4))
  assert int(inp['EAX_pointer'],16)==word(x.read(0x46FC48,4))
  assert raw(inp,'position_source_hex')==x.read(int(inp['ECX_pointer'],16),12)
  assert raw(inp,'forward_source_hex')==x.read(int(inp['EAX_pointer'],16),12)
  definition=c['original_authored_callback_input'];ref=definition['reference_file_offset'];st=definition['sound_tag']
  assert raw(definition,'reference_0c_hex')==bytes(cache.data[ref:ref+12])
  assert cache.u(ref+4)==int(c['source_sound_datum'],16)
  assert raw(definition,'sound_definition_14_hex')==bytes(cache.data[st['offset']:st['offset']+20])
  assert definition['ordinary_FP_clock_not_executed']and definition['callback_call_site_and_gain_explicit_inputs']
  request=c['request'];assert request['return_eax']=='ffffffff'
  assert all(z.get('return_eax')is not None for z in c['original_request_calls'])
  ad=next(z for z in c['original_request_calls']if z['entry']=='0018ad70')
  assert ad['registers']['esi']=='e2700001'and ad['registers']['edi']=='0044b14c'
  assert ad['stack'][0]==c['source_sound_datum']and ad['stack'][1]=='00000001'
  dc=next(z for z in c['original_request_calls']if z['entry']=='0018dde0')
  assert int(dc['return_eax'],16)&0xFF==1
  assert c['source_kernel_call_site_explicit']and c['created_source_datum']=='ef730000'
  before=c['before_removal'];after=c['after_removal'];ba=before['audio'];aa=after['audio']
  assert before['object_pool_data_hex']==after['object_pool_data_hex']
  assert before['object_pool_4c_hex']==after['object_pool_4c_hex']
  assert before['world']==after['world']and before['player']==after['player']
  assert ba['owner_pool_60cc_hex']==aa['owner_pool_60cc_hex']and ba['owner_state_244_hex']==aa['owner_state_244_hex']
  assert ba['source_owned_rng_8_hex']==aa['source_owned_rng_8_hex']
  assert ba['driver_pointer']==aa['driver_pointer']=='00000000'and ba['audio_initialized']==aa['audio_initialized']==0
  owner=ba['owner_object_lookup_input'];head=raw(owner,'selected_header_0c_hex')
  assert owner['datum']=='e2700001'and (int(owner['datum'],16)&0xFFFF)<owner['high_water']==2
  assert head==headers[12:24]and word(head,0,2)==0xE270 and head[3]==2 and word(head,8)==int(c['source_weapon_pointer'],16)
  trace=c['ordered_owner_callback_instructions']
  # The valid index and signed salt reach header acceptance; type mask fails.
  for address in('000bbcd9','000bbce1','000bbce4','000bbce7','000bbceb','000bbcf3','000bbcfd','000bbd01','0018e333','0018e334'):
   assert address in trace,address
  assert '000bbd03'not in trace and not any(0xD2270<=int(a,16)<0xD22A4 for a in trace)
  calls=[z for z in c['owner_calls']if z['phase']=='original_full_source_remove_reason3']
  assert [z['entry']for z in calls]==['00128a70','001280b0','0018e2f0','000bbcb0','00128ae0','0016d4c0']
  assert calls[2]['stack'][:3]==['e2700001','ef730000','00000003']
  assert calls[3]['registers']['eax']=='e2700001'and calls[3]['stack'][0]=='00000003'and calls[3]['return_eax']=='00000000'
  assert c['removal']['completed']and all('return_eax'in z for z in calls)
  bs=ba['source_cache'];as0=aa['source_cache'];src=raw(bs,'source_bc_hex')
  assert word(src,0x10)==0xE2700001 and word(src,0x14)==0x44B14C and word(src,0xA8)==0xFFFFFFFF and word(src,0xAC,2)==0xFFFF
  assert raw(bs['selected_chunk'],'cache_entry_10_hex')[4]==1 and raw(as0['selected_chunk'],'cache_entry_10_hex')[4]==0
  assert word(raw(as0,'source_bc_hex'),0,2)==0 and not raw(as0,'source_bc_hex')[4]&2
  assert word(raw(bs['source_pool'],'header_4c_hex'),0x3C)==1 and word(raw(as0['source_pool'],'header_4c_hex'),0x3C)==0
  assert raw(before,'complete_source_pool_data_hex')[0xBC:]==raw(after,'complete_source_pool_data_hex')[0xBC:]
  creation_stores=[z for z in c['ordered_source_lifecycle_stores']if z['phase']=='direct_original_unvoiced_source_creation_from_actual_new_request']
  assert any(z['after_hex']=='010070e2'and int(z['address'],16)==int(c['removal']['before']['source_cache']['created_sources'][0]['pointer'],16)+0x10 for z in creation_stores)
  if c['scenario']=='ready':
   done=c['pending_read_completion'];assert done
   complete=[z for z in done['stores']if z['instruction']=='001223ac']
   assert len(complete)==1 and complete[0]['before_hex']=='00'and complete[0]['after_hex']=='01'
  else:assert c['pending_read_completion']is None
  for z in c['cache_provider_calls']:
   assert z['real_host_service']and not z['source_constructor_result_fabricated']
   if z['provider']=='ReadFile':
    assert z['completion_flag_not_written_by_provider']and z['file_offset']%512==z['bytes_read']%512==0
    with shared.open('rb')as f:f.seek(z['file_offset']);b=f.read(z['bytes_read'])
    assert hashlib.sha256(b).hexdigest()==z['raw_sha256'];reads+=1;total_bytes+=len(b)
 assert reads==2
 listing_count=instruction_rows=0
 for report,path in((r,mp),(s,rp)):
  for z in report['instruction_listings']+[report['executed_original_listing']]:
   p=path.parent/'instructions'/z['file'];assert digest(p)==z['sha256']
   for line in p.read_text().splitlines():
    a,b,*_=line.split();bb=bytes.fromhex(b);assert x.read(int(a,16),len(bb))==bb
   if report is r:listing_count+=1;instruction_rows+=len(p.read_text().splitlines())
 fixture={'oracle_sha256':ORACLE_PIN,'suite_sha256':digest(OUT/'frozen/suite.json'),
  'source_sha256':PIN,'cache_sha256':CACHE_PIN,'shared_sha256':SHARED_PIN,
  'independently_reexecuted_original_cases':r['cases'],'reexecuted_source_outputs_equal':True,
  'valid_weapon_owner_passes_index_and_salt_then_fails_unit_type_mask':True,
  'D2270_executed':False,'full_original_sound_map_driver_voice_or_playback_proven':False,'native_gameplay_update_applied':False}
 p=OUT/'source-fixture.json';p.write_text(json.dumps(fixture,indent=2)+'\n')
 audit={'source_fixture_sha256':digest(p),'suite_sha256':digest(OUT/'frozen/suite.json'),
  'independent_original_cases':2,'source_created_player_unit_and_weapon_cases':2,
  'valid_weapon_owner_callbacks_and_complete_unvoiced_removals':2,
  'original_instruction_listings_verified':listing_count,'original_instruction_rows_verified':instruction_rows,
  'unique_executed_original_instructions':r['executed_original_listing']['instructions'],
  'aligned_file_reads_verified':reads,'aligned_file_bytes_verified':total_bytes,'earlier_frozen_pins_verified':FROZEN,
  'object_header_identity_driver_voice_admission_or_callback_success_supplied':False,
  'D2270_executed':False,'native_gameplay_update_applied':False}
 (OUT/'audit.json').write_text(json.dumps(audit,indent=2)+'\n')
 print(json.dumps({k:v for k,v in audit.items()if k!='earlier_frozen_pins_verified'},indent=2))
if __name__=='__main__':main()
