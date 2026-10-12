#!/usr/bin/env python3
"""Audit original allocator ownership and independently replayed unvoiced removal."""
import hashlib,json,struct
from oracle_smg_audio_owner_pool_1_1 import ROOT,OUT,FROZEN,PIN,CACHE_PIN,digest
from oracle_smg_firing_1_1 import Xbe
from audit_smg_audio_rng_startup_1_1 import source_only

ORACLE_PIN='ac4aadc15659f4dbddb756f7973f9cd0f3e83b47a4b30ee0d2479bb382b9d380'
SHARED_PIN='084c817fa82bb9bc416d9ba847cec05be431abc419e3a1087cdf211fbf47f5af'
def word(b,o,size=4):return int.from_bytes(b[o:o+size],'little')
def raw(s,k):return bytes.fromhex(s[k])

def main():
 assert digest(ROOT/'tools/oracle_smg_audio_owner_pool_1_1.py')==ORACLE_PIN
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 suite=json.loads((OUT/'frozen/suite.json').read_text())
 for row in suite['artifacts']:assert digest(ROOT/row['path'])==row['sha256'],row['path']
 mp=OUT/'measured/report.json';rp=OUT/'reexecution/report.json'
 r=json.loads(mp.read_text());s=json.loads(rp.read_text());assert source_only(r)==source_only(s)
 assert r['context_mode']=='real_Wine_worker_alive_during_source_execution'
 assert s['context_mode']=='whole_recorded_context_replay'
 assert r['clock_mode']=='real_host_clock_measurement'and s['clock_mode']=='verbatim_measured_input_replay'
 x=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
 assert hashlib.sha256(x.data).hexdigest()==PIN and digest(ROOT/'assets/disc/maps/lockout.map')==CACHE_PIN
 shared=ROOT/'assets/disc/maps/shared.map';assert digest(shared)==SHARED_PIN
 assert r['oracle_sha256']==s['oracle_sha256']==ORACLE_PIN
 assert r['frozen_prior_pins']==s['frozen_prior_pins']==FROZEN
 assert not r['native_gameplay_update_applied']
 assert len(r['cases'])==2
 reads=read_bytes=0
 for c in r['cases']:
  assert not c['source_allocator_pool_validity_driver_voice_or_callback_fields_supplied']
  assert not c['complete_owner_map_or_audio_playback_proven']and not c['native_gameplay_update_applied']
  assert c['file_binding']['source_file_registry_owner_unexecuted']
  assert [z['instruction']for z in c['boundaries']]==['002217e8','002d8606','0022310c']
  assert all(not z['source_return_device_owner_validity_or_ready_result_supplied']for z in c['boundaries'])
  assert len(c['clock_calls'])==5
  assert [z['entry']for z in c['clock_calls']]==['00336983','00337a40','00336983','00337a40','00337a40']
  for z in c['clock_calls']:
   assert z['no_source_rng_or_ready_field_written_by_provider']
   inp=z['measured_provider_input'];ns=inp['observed_ns']
   if z['entry']=='00336983':assert raw(z,'output_after_hex')==struct.pack('<Q',ns//100+116444736000000000)
   else:assert int(z['return_eax'],16)==(ns//1000000)&0xFFFFFFFF
  # The frozen original startup, not the inherited test allocator, publishes
  # the exact object that these constructor calls subsequently consume.
  writes=c['owner_stores']
  publish=[z for z in writes if z['instruction']=='00125328'];assert len(publish)==1
  vt=[z for z in writes if z['instruction']=='00125330'];assert len(vt)==1
  assert publish[0]['address']=='0051841c'and word(raw(publish[0],'after_hex'),0)==int(vt[0]['address'],16)
  assert raw(vt[0],'after_hex')==struct.pack('<I',0x45A610)
  assert word(x.read(0x45A610,4),0)==0x125E50
  pre=c['steps'][0]['before'];assert pre['game_arena_used']==0x1294
  assert pre['allocator_pointer']==f'{word(raw(publish[0],"after_hex"),0):08x}'
  assert raw(pre,'allocator_4_hex')==struct.pack('<I',0x45A610)
  ctor=c['steps'][4];reset=c['steps'][5];cleanup=c['cleanup']
  assert ctor['entry']=='0018a200'and ctor['completed']
  assert reset['entry']=='0018a2a0'and not reset['completed']
  assert cleanup['entry']=='00128a70'and cleanup['completed']and cleanup['args']==['00000003']
  before,after=ctor['before'],ctor['after']
  assert before['owner_pool_pointer']==before['owner_state_pointer']=='00000000'
  assert after['game_arena_used']-before['game_arena_used']==0x60CC+0x244
  assert int(after['owner_pool_pointer'],16)==int(before['game_arena'],16)+before['game_arena_used']
  assert int(after['owner_state_pointer'],16)==int(after['owner_pool_pointer'],16)+0x60CC
  pool=raw(after,'owner_pool_60cc_hex');assert len(pool)==0x60CC
  assert pool[:22]==b'object looping sounds\0'
  assert word(pool,0x20)==1024 and word(pool,0x24)==24 and pool[0x29]==0 and pool[0x2A]&4
  assert word(pool,0x44)==int(after['owner_pool_pointer'],16)+0x4C
  assert word(pool,0x48)==int(after['owner_pool_pointer'],16)+0x604C
  assert pool[0x604C:]==bytes(0x80)
  allocs=[z for z in c['owner_calls']if z['phase']==ctor['name']and z['entry']=='00125e50']
  assert len(allocs)==1 and allocs[0]['stack'][0]=='000060cc'
  assert allocs[0]['return_eax']==after['owner_pool_pointer']
  assert allocs[0]['registers']['ecx']==before['allocator_pointer']
  callback_row=raw(after,'owner_lifecycle_callback_row_36_hex')
  assert callback_row==x.read(0x447AA0,36)
  assert [word(callback_row,o)for o in(0,4,8)]==[0x18A200,0x18A280,0x18A2A0]
  original_ctor=[z for z in c['owner_calls']if z['phase']==ctor['name']and z['entry']=='0016d070'][0]
  assert original_ctor['registers']['ebx']=='00000400'and original_ctor['registers']['edx']=='00000018'
  assert original_ctor['registers']['ecx']=='00000000'
  valid=raw(reset['after'],'owner_pool_60cc_hex');assert valid[0x29]==1
  assert word(valid,0x30)==int(after['allocator_pointer'],16)
  assert [word(valid,o)for o in(0x34,0x38,0x3C)]==[0,0,0]
  assert valid[0x604C:]==bytes(0x80)
  assert any(z['instruction']=='0018a2b5'and z['after_hex']=='01'for z in reset['stores'])
  state=raw(reset['after'],'owner_state_244_hex');assert len(state)==0x244
  assert state[:0x20]==b'\xff'*0x20 and state[0x20:0x128]==bytes(0x108)
  assert state[0x128:0x228]==b'\xff'*0x100
  assert state[0x228:0x238]==struct.pack('<IIII',*[0x3F800000]*4)
  assert word(state,0x238)==8000 and state[0x23C:]==bytes(8)
  cb=c['before_cleanup'];ca=c['after_cleanup'];source=raw(cb['source_cache'],'source_bc_hex')
  assert c['creation']['created_original_source_datum']=='ef730000'
  assert c['creation']['actual_rejected_fp_request']['return_eax']=='ffffffff'
  assert c['creation']['creation_kernel_call_site_supplied']and c['creation']['ordinary_available_admission_or_playback_not_claimed']
  assert word(source,0x14)==0x44B14C and word(x.read(0x44B14C,32),0x10)==0x18E2F0
  assert word(source,0xA8)==0xFFFFFFFF and word(source,0xAC,2)==0xFFFF
  calls=[z for z in c['owner_calls']if z['phase']==cleanup['name']]
  assert [z['entry']for z in calls]==['00128a70','001280b0','0018e2f0','000bbcb0','00128ae0','0016d4c0']
  assert all('return_eax'in z for z in calls)
  callback=calls[2];lookup=calls[3]
  assert callback['stack'][:3]==['00010002','ef730000','00000003']
  assert lookup['registers']['eax']=='00010002'and lookup['stack'][0]=='00000003'and lookup['return_eax']=='00000000'
  owner=cb['owner_object_lookup_input']
  assert owner['datum']=='00010002'and owner['stride']==12 and owner['high_water']==2
  assert int(owner['datum'],16)&0xFFFF>=owner['high_water']
  assert 'selected_header_0c_hex'not in owner
  assert raw(cb,'owner_pool_60cc_hex')==raw(ca,'owner_pool_60cc_hex')
  assert raw(cb,'owner_state_244_hex')==raw(ca,'owner_state_244_hex')
  assert raw(cb,'source_owned_rng_8_hex')==raw(ca,'source_owned_rng_8_hex')
  for s0 in [*(z['after']for z in c['steps']),cb,ca,c['retained']]:
   assert s0['driver_pointer']=='00000000'and s0['audio_initialized']==0
  sb,sa=cb['source_cache'],ca['source_cache']
  assert raw(sb['selected_chunk'],'cache_entry_10_hex')[4]==1
  assert raw(sa['selected_chunk'],'cache_entry_10_hex')[4]==0
  assert [word(raw(sb['source_pool'],'header_4c_hex'),o)for o in(0x34,0x38,0x3C)]==[1,1,1]
  assert [word(raw(sa['source_pool'],'header_4c_hex'),o)for o in(0x34,0x38,0x3C)]==[0,0,0]
  assert word(raw(sa,'source_bc_hex'),0,2)==0 and raw(sa['source_pool'],'bitmap_hex')==bytes(len(raw(sa['source_pool'],'bitmap_hex')))
  assert not raw(sa,'source_bc_hex')[4]&2
  if c['scenario']=='ready':
   done=c['pending_read_after_full_cleanup'];assert done and done['entry']=='00122360'
   assert done['one_worker_iteration_call_site_supplied']
   comp=[z for z in done['stores']if z['instruction']=='001223ac']
   assert len(comp)==1 and comp[0]['before_hex']=='00'and comp[0]['after_hex']=='01'
   assert done['after']['source_cache']['globals']['004e9ebc']=='00000000'
  else:assert c['pending_read_after_full_cleanup']is None
  for z in c['cache_provider_calls']:
   assert z['real_host_service']and not z['source_constructor_result_fabricated']
   if z['provider']=='ReadFile':
    assert z['completion_flag_not_written_by_provider']and z['file_offset']%512==0 and z['bytes_read']%512==0
    with shared.open('rb')as f:f.seek(z['file_offset']);b=f.read(z['bytes_read'])
    assert digest_bytes(b)==z['raw_sha256'];reads+=1;read_bytes+=len(b)
  for z in c['raw_providers']:assert z['provider_writes_only_raw_base_and_access']
 assert reads==2
 listing_count=instruction_rows=0;observed=set()
 for report,path in((r,mp),(s,rp)):
  for z in report['instruction_listings']+[report['executed_original_listing']]:
   p=path.parent/'instructions'/z['file'];assert digest(p)==z['sha256']
   for line in p.read_text().splitlines():
    a,b,*_=line.split();bb=bytes.fromhex(b);assert x.read(int(a,16),len(bb))==bb
    if z is report['executed_original_listing']:observed.add(int(a,16))
   if report is r:listing_count+=1;instruction_rows+=len(p.read_text().splitlines())
 assert {0x125328,0x125330,0x125E50,0x18A200,0x18A2B5,0x18E2F0,0xBBCB0,0x16D4C0}<=observed
 assert not {0xD2270,0x18A790,0x2205C0,0x2217E8,0x22310C,0x2D8606,0x336983,0x337A40}&observed
 fixture={'oracle_sha256':ORACLE_PIN,'suite_sha256':digest(OUT/'frozen/suite.json'),
  'source_sha256':PIN,'cache_sha256':CACHE_PIN,'shared_sha256':SHARED_PIN,
  'independently_reexecuted_original_cases':r['cases'],'reexecuted_source_outputs_equal':True,
  'invalid_inherited_owner_index_rejected_before_header_or_type_lookup':True,
  'unexecuted_valid_owner_and_unit_sound_slot_clear_D2270_remain_conditional':True,
  'full_original_sound_map_device_voice_or_playback_proven':False,'native_gameplay_update_applied':False}
 p=OUT/'source-fixture.json';p.write_text(json.dumps(fixture,indent=2)+'\n')
 audit={'source_fixture_sha256':digest(p),'suite_sha256':digest(OUT/'frozen/suite.json'),
  'independent_original_cases':2,'complete_original_unvoiced_remove_returns':2,
  'original_owner_pool_capacity':1024,'original_owner_record_bytes':24,
  'original_owner_pool_raw_bytes':0x60CC,'original_owner_state_raw_bytes':0x244,
  'original_instruction_listings_verified':listing_count,'original_instruction_rows_verified':instruction_rows,
  'unique_executed_original_instructions':r['executed_original_listing']['instructions'],
  'aligned_read_callbacks_verified':reads,'aligned_file_bytes_verified':read_bytes,
  'earlier_frozen_pins_verified':FROZEN,'D2270_executed':False,
  'original_driver_initialized_voice_playback_or_callback_result_supplied':False,
  'native_gameplay_update_applied':False}
 (OUT/'audit.json').write_text(json.dumps(audit,indent=2)+'\n')
 print(json.dumps({k:v for k,v in audit.items()if k!='earlier_frozen_pins_verified'},indent=2))

def digest_bytes(b):return hashlib.sha256(b).hexdigest()
if __name__=='__main__':main()
