#!/usr/bin/env python3
"""Audit original loaded SMG bytes/format kernels and the unadmitted driver stop."""
import hashlib,json,struct
from oracle_smg_audio_sample_boundary_1_1 import ROOT,OUT,FROZEN,PIN,CACHE_PIN,digest,ARENA
from oracle_smg_firing_1_1 import Xbe,Cache
from audit_smg_audio_rng_startup_1_1 import source_only

ORACLE_PIN='2f5f915768b96fd5ad3e903ba98f08b7f3be1187d4cd66313a43b6229fb20954'
SHARED_PIN='084c817fa82bb9bc416d9ba847cec05be431abc419e3a1087cdf211fbf47f5af'
def word(b,o=0,n=4):return int.from_bytes(b[o:o+n],'little')
def raw(d,k):return bytes.fromhex(d[k])
def main():
 assert digest(ROOT/'tools/oracle_smg_audio_sample_boundary_1_1.py')==ORACLE_PIN
 earlier_artifacts={}
 for p,h in FROZEN.items():
  assert digest(ROOT/p)==h,p
  if p.endswith('/frozen/suite.json'):
   for z in json.loads((ROOT/p).read_text()).get('artifacts',[]):
    assert digest(ROOT/z['path'])==z['sha256'],z['path'];earlier_artifacts[z['path']]=z['sha256']
 suite=json.loads((OUT/'frozen/suite.json').read_text())
 for z in suite['artifacts']:assert digest(ROOT/z['path'])==z['sha256'],z['path']
 mp=OUT/'measured/report.json';rp=OUT/'reexecution/report.json'
 r=json.loads(mp.read_text());s=json.loads(rp.read_text());assert source_only(r)==source_only(s)
 assert r['oracle_sha256']==s['oracle_sha256']==ORACLE_PIN
 assert r['source_sha256']==s['source_sha256']==PIN and r['cache_sha256']==s['cache_sha256']==CACHE_PIN
 assert r['frozen_prior_pins']==s['frozen_prior_pins']==FROZEN
 assert not r['original_decoded_PCM_packet_submission_or_playback_proven']and not r['native_gameplay_update_applied']
 prior=ROOT/r['whole_context_and_clock_source_report'];old=json.loads(prior.read_text())
 assert digest(prior.parent/'raw-host-context.json')==r['whole_context_sha256']==old['context_snapshot_sha256']
 x=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
 assert hashlib.sha256(x.data).hexdigest()==PIN
 cache=Cache(ROOT/'assets/disc/maps/lockout.map');assert digest(ROOT/'assets/disc/maps/lockout.map')==CACHE_PIN
 shared=ROOT/'assets/disc/maps/shared.map';assert digest(shared)==SHARED_PIN
 expected_wave=bytes.fromhex('6900020044ac0000c8c100004800040002004000')
 expected_compact=bytes.fromhex('6900020444ac000048000000')
 sound_by_name={'ready':0xE5AC0437,'reload':0xE5AD0438};chunks=reads=bytes_read=0
 assert [c['scenario']for c in r['cases']]==['ready','reload']
 for i,c in enumerate(r['cases']):
  owning=c['owning_request_and_unvoiced_remove'];sound=sound_by_name[c['scenario']];definition=c['definition_14']
  assert owning['source_sound_datum']==f'{sound:08x}'and owning['source_allocated_weapon']=='e2700001'
  assert owning['request']['return_eax']=='ffffffff'and owning['removal']['completed']
  assert owning['source_kernel_call_site_explicit']and not owning['ordinary_FP_event_local_binding_complete_map_or_playback_proven']
  assert source_only(c['clock_calls'])==source_only(old['cases'][i]['clock_calls'])
  for z in (definition,c['selected_range_0c'],c['selected_permutation_10']):
   b=raw(z,'raw_hex');o=z['file_offset'];assert b==bytes(cache.data[o:o+len(b)])
   assert int(z['original_va'],16)==o+cache.magic
  assert definition['file_offset']==cache.tags[sound]['offset']and raw(definition,'raw_hex')[3:6]==b'\1\1\1'
  ug=c['ugh_tag'];assert ug['group']=='ugh!'and ug['datum']=='f324119f'
  pitch=c['original_selected_indices']['pitch'];perm=c['original_selected_indices']['permutation']
  assert pitch==perm==c['original_selected_indices']['initial_chunk']==0
  ranges=cache.block(ug['offset']+0x20,12);perms=cache.block(ug['offset']+0x28,16);cc=cache.block(ug['offset']+0x40,12)
  rr=ranges[cache.h(definition['file_offset']+8)+pitch];pp=perms[cache.h(rr+8)+perm]
  assert rr==c['selected_range_0c']['file_offset']and pp==c['selected_permutation_10']['file_offset']
  start=cache.h(pp+12);count=cache.h(pp+14);assert len(c['source_loaded_chunks'])==count==(1 if i==0 else 2)
  total=b''
  for n,z in enumerate(c['source_loaded_chunks']):
   chunks+=1;off=cc[start+n];meta=z['authored_chunk'];assert meta['file_offset']==off
   assert raw(meta,'raw_hex')==bytes(cache.data[off:off+12])
   size=cache.u(off+4)&0x3FFFFFFF;location=cache.u(off)>>30;fileoff=cache.u(off)&0x3FFFFFFF
   assert z['authored_bytes']==size and z['aligned_bytes']==(size+511)&~511
   assert z['file_location']==location==2 and z['file_offset']==fileoff
   assert size%72==0 and z['ready_query']['entry']=='0021a960'and int(z['ready_query']['return_eax'],16)&2
   query=z['data_query'];assert query['entry']=='0021aa60'and query['return_eax']==z['source_buffer_pointer']
   before=z['before_data_query'];after=z['after_data_query'];be=raw(before['selected_chunk'],'cache_entry_10_hex');ae=raw(after['selected_chunk'],'cache_entry_10_hex')
   assert ae[:5]==be[:5]and ae[5]==min(be[5]+1,255)and ae[6:]==be[6:]
   assert before['driver_pointer']==after['driver_pointer']=='00000000'
   assert before['audio_initialized']==after['audio_initialized']==0
   datum=int(after['selected_chunk']['datum'],16);pages=raw(after['page_pool'],'data_hex');page=pages[(datum&0xFFFF)*24:((datum&0xFFFF)+1)*24]
   base=int(after['globals']['005098e0'],16);shift=word(raw(after,'page_allocator_70_hex'),0x34)
   assert int(z['source_buffer_pointer'],16)==base+(word(page,8)<<shift)
   with shared.open('rb')as f:f.seek(fileoff);actual=f.read(z['aligned_bytes'])
   for report,path in((r,mp),(s,rp)):
    zz=report['cases'][i]['source_loaded_chunks'][n]
    aligned=path.parent/'samples'/zz['aligned_file'];compressed=path.parent/'samples'/zz['compressed_file']
    assert digest(aligned)==zz['aligned_sha256']and digest(compressed)==zz['compressed_sha256']
    assert aligned.read_bytes()==actual and compressed.read_bytes()==actual[:size]
   total+=actual[:size]
   if n:
    load=z['load'];assert load['direct_cache_request_and_one_worker_iteration_explicit_inputs']
    assert load['ordinary_stream_prefetch_order_not_executed']and int(load['return_eax'],16)&1 and not int(load['return_eax'],16)&2
   else:assert z['load']is None
  assert c['source_loaded_total_bytes']==len(total)==int(c['original_byte_total_query']['return_eax'],16)==cache.u(pp+8)
  assert c['original_word6_query']['return_eax']==f'{cache.h(pp+6):08x}'
  for report,path in((r,mp),(s,rp)):
   zz=report['cases'][i];p=path.parent/'samples'/zz['selected_permutation_file'];assert digest(p)==zz['selected_permutation_sha256']and p.read_bytes()==total
  assert raw(c,'original_wave_format_14_hex')==expected_wave
  assert raw(c['compact_format_kernel'],'original_compact_0c_hex')==expected_compact
  assert c['codec_instance']=='004865f8'and c['codec_vtable']=='0045e2b8'
  assert raw(c,'original_codec_instance_4_hex')==x.read(0x4865F8,4)
  assert raw(c,'original_codec_vtable_24_hex')==x.read(0x45E2B8,36)
  assert list(struct.unpack('<9I',x.read(0x45E2B8,36)))==[0x27E760,0x27E760,0x2B4C50,0x2B4CE0,0x2B4D60,0x2B4D90,0x2B4DD0,0x187550,0x2B4E10]
  assert raw(c,'original_driver_policy_10_hex')==x.read(0x451220,16)
  assert c['explicit_unadmitted_stream_index']==sum(struct.unpack_from('<3H',x.read(0x451220,16))[:1])==40
  stop=c['actual_driver_stop'];assert stop['instruction']=='002b4975'and raw(stop,'instruction_bytes')==x.read(0x2B4975,3)
  assert stop['driver_pointer']=='00000000'and stop['channel_address']=='0000082c'and stop['first_unexecuted_store_address']=='0000085c'
  assert not stop['source_driver_channel_voice_device_or_success_result_supplied']
  descriptor=raw(stop,'original_stream_descriptor_18_hex');assert word(descriptor)==0 and word(descriptor,4)==2
  assert word(descriptor,8)==int(stop['original_waveformat_pointer'],16)and word(descriptor,12)==0x222840 and word(descriptor,16)==40 and word(descriptor,20)==0
  assert raw(stop,'original_waveformat_14_hex')==expected_wave
  assert not any(z['instruction']=='002b4975'for z in c['sample_stores'])
  assert not any(z['entry']=='002b4ce0'for z in c['sample_calls'])
  assert c['final_source_state']['audio']['driver_pointer']=='00000000'
  assert c['final_source_state']['audio']['audio_initialized']==0
  assert not c['original_cpu_adpcm_decode_or_packet_submission_executed']and not c['driver_voice_device_admission_or_ready_results_supplied']and not c['native_gameplay_update_applied']
  for z in c['cache_provider_calls']:
   assert z['real_host_service']and not z['source_constructor_result_fabricated']
   if z['provider']=='ReadFile':
    assert z['completion_flag_not_written_by_provider']and z['file_offset']%512==z['bytes_read']%512==0
    with shared.open('rb')as f:f.seek(z['file_offset']);b=f.read(z['bytes_read'])
    assert hashlib.sha256(b).hexdigest()==z['raw_sha256'];reads+=1;bytes_read+=len(b)
 assert chunks==reads==3
 listing_count=rows=0
 for report,path in((r,mp),(s,rp)):
  for z in report['instruction_listings']+[report['executed_original_listing']]:
   p=path.parent/'instructions'/z['file'];assert digest(p)==z['sha256']and z['instructions']>0
   for line in p.read_text().splitlines():
    at,bb,*_=line.split();b=bytes.fromhex(bb);assert x.read(int(at,16),len(b))==b
   if report is r:listing_count+=1;rows+=len(p.read_text().splitlines())
 executed={line.split()[0]for line in(mp.parent/'instructions'/r['executed_original_listing']['file']).read_text().splitlines()}
 assert '003840c7'in executed and '00384057'in executed
 assert '002b4975'not in executed and '002b4ce0'not in executed and '003865e6'not in executed and '00386f8b'not in executed
 fixture={'oracle_sha256':ORACLE_PIN,'suite_sha256':digest(OUT/'frozen/suite.json'),
  'source_sha256':PIN,'cache_sha256':CACHE_PIN,'shared_sha256':SHARED_PIN,'independently_reexecuted_original_report':r,
  'reexecuted_source_outputs_equal':True,'compressed_samples_loaded_and_format_kernels_executed':True,
  'driver_voice_CPU_decoding_packet_submission_or_playback_executed':False,'native_gameplay_update_applied':False}
 p=OUT/'source-fixture.json';p.write_text(json.dumps(fixture,indent=2)+'\n')
 result={'source_fixture_sha256':digest(p),'suite_sha256':digest(OUT/'frozen/suite.json'),
  'independent_original_executions':2,'source_cases':2,'authored_compressed_chunks_loaded_and_verified':chunks,
  'aligned_file_reads_verified':reads,'aligned_file_bytes_verified':bytes_read,
  'original_instruction_listings_verified':listing_count,'original_instruction_rows_verified':rows,
  'unique_executed_original_instructions':r['executed_original_listing']['instructions'],
  'earlier_frozen_pins_verified':FROZEN,'earlier_manifest_artifacts_verified':len(earlier_artifacts),
  'driver_voice_CPU_decoder_packet_submission_or_playback_proven':False,'native_gameplay_update_applied':False}
 (OUT/'audit.json').write_text(json.dumps(result,indent=2)+'\n')
 print(json.dumps({k:v for k,v in result.items()if k!='earlier_frozen_pins_verified'},indent=2))
if __name__=='__main__':main()
