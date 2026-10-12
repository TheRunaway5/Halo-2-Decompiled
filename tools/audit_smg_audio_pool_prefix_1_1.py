#!/usr/bin/env python3
"""Independently audit source static owners/raw pool consumers and repeat."""
import hashlib,json
from oracle_smg_audio_pool_prefix_1_1 import ROOT,OUT,PIN,CACHE_PIN,FROZEN,OWNERS,digest
from oracle_smg_firing_1_1 import Xbe
from audit_smg_audio_rng_startup_1_1 import source_only,word

ORACLE_PIN='381b1763cdffb9cbc7b73c25c983697597284d29ac7e4847bd2f092f339906db'

def main():
 assert digest(ROOT/'tools/oracle_smg_audio_pool_prefix_1_1.py')==ORACLE_PIN
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 suite=json.loads((OUT/'frozen/suite.json').read_text())
 rpath=ROOT/suite['measurement']['path'];spath=ROOT/suite['reexecution']['path']
 assert digest(rpath)==suite['measurement']['sha256'] and digest(spath)==suite['reexecution']['sha256']
 r=json.loads(rpath.read_text());s=json.loads(spath.read_text());assert source_only(r)==source_only(s)
 assert r['oracle_sha256']==s['oracle_sha256']==ORACLE_PIN
 assert r['source_sha256']==PIN and r['cache_sha256']==CACHE_PIN
 assert r['clock_mode']=='real_host_clock_measurement' and s['clock_mode']=='verbatim_measured_input_replay'
 assert r['context_mode']=='real_Wine_worker_alive_during_source_execution' and s['context_mode']=='whole_recorded_context_replay'
 assert r['new_raw_pool_storage_provider_used'] and not r['original_Xbox_pool_block_rounding_proven']
 assert not r['new_DirectSound_device_capability_hardware_or_consumption_result_supplied']
 assert not r['whole_original_DirectSound_or_audio_initialization_completed'] and not r['native_gameplay_update_applied']
 x=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');assert hashlib.sha256(x.data).hexdigest()==PIN
 ctxpath=rpath.parent/'raw-host-context.json';assert digest(ctxpath)==r['context_snapshot_sha256']
 ctx=json.loads(ctxpath.read_text());token=ctx['ethread_address']
 table=r['original_static_initializer_table']
 assert table['executed_table_raw_hex']==x.read(0x468724,16).hex()
 assert table['counter_storage_initial_hex']==x.read(0x38D120,16).hex()==bytes(16).hex()
 owners=r['original_statistics_owners'];assert len(owners)==4
 for i,(z,(address,entry,owner,target))in enumerate(zip(owners,OWNERS)):
  assert word(x.read(address,4))==entry and z['table_address']==f'{address:08x}' and z['entry']==f'{entry:08x}'
  assert x.read(entry,11)==b'\xc7\x05'+owner.to_bytes(4,'little')+target.to_bytes(4,'little')+b'\xc3'
  assert z['fragment']==['002d813e','002d814b'] and z['stop_is_exclusive'] and not z['full_CXX_table_or_cinit_executed']
  assert z['before']['owners'][f'{owner:08x}']=='00000000' and z['after']['owners'][f'{owner:08x}']==f'{target:08x}'
  assert z['stores']==[{'phase':f'original_CXX_table_dispatch_{address:08x}','instruction':f'{entry:08x}',
   'address':f'{owner:08x}','size':4,'before_hex':'00000000','after_hex':target.to_bytes(4,'little').hex()}]
  assert z['before']['counter_storage_10_hex']==z['after']['counter_storage_10_hex']==bytes(16).hex()
  if i:assert z['before']==owners[i-1]['after']
 services=r['raw_pool_provider_requests'];blocks=r['retained_raw_pool_blocks'];assert len(services)==4 and len(blocks)==2
 assert [z['kernel_ordinal']for z in services]==[15,23,15,23]
 for allocation,query,block in zip(services[::2],services[1::2],blocks):
  requested=block['requested_bytes'];alias=int(block['guest_alias'],16);capacity=block['raw_capacity_bytes']
  assert allocation['args']==[f'{requested:08x}','626f5344'] and requested in(48,184)
  assert allocation['resulting_guest_alias']==block['guest_alias'] and allocation['raw_capacity_bytes']==capacity==4096
  assert allocation['raw_provider_operation']['actual_errno']==0 and allocation['raw_provider_operation']['host_page_size']==4096
  assert query['args']==[block['guest_alias']] and query['resulting_raw_capacity_bytes']==capacity
  assert allocation['original_call_site']=='00383ed5' and allocation['original_return_site']=='00383edb'
  assert query['original_call_site']=='00383ee2' and query['original_return_site']=='00383ee8'
  assert allocation['provider_writes_no_source_object_counter_device_or_ready_field'] and query['provider_writes_no_source_object_counter_device_or_ready_field']
  assert allocation['Xbox_pool_rounding_not_claimed'] and query['Xbox_pool_rounding_not_claimed']
  assert allocation['raw_initial_bytes_sha256']==block['initial_raw_storage_sha256']==hashlib.sha256(bytes(capacity)).hexdigest()
  writes=[z for z in r['source_pool_stores']if z['instruction']=='00383f03' and alias<=int(z['address'],16)<alias+capacity]
  assert len(writes)==capacity//4
  assert [int(z['address'],16)for z in writes]==list(range(alias,alias+capacity,4))
  assert all(z['size']==4 and z['after_hex']=='00000000'for z in writes)
  raw=bytes.fromhex(block['retained_raw_storage_hex']);assert len(raw)==capacity
  assert raw[:4]==b'pool' and raw[requested:]==bytes(capacity-requested)
 counters=[z for z in r['source_pool_stores']if z['instruction']=='00383eee']
 assert [(z['address'],word(bytes.fromhex(z['before_hex'])),word(bytes.fromhex(z['after_hex'])))for z in counters]==[
  ('0038d120',0,4096),('0038d120',4096,8192)]
 markers=[z for z in r['source_pool_stores']if z['instruction']=='00383feb']
 assert [z['address']for z in markers]==[z['guest_alias']for z in blocks] and all(z['after_hex']=='706f6f6c'for z in markers)
 d=r['source_device_prefix'];before=d['before'];after=d['after'];stop=d['first_unprovided_dependency']
 assert d['original_source_frame_preserved_from_126b20_call_126c90'] and not d['source_driver_constructor_or_DirectSound_return_supplied']
 assert before['source_DirectSound_allocation_statistics_pointer']=='0038d120'
 assert before['source_shared_DirectSound_pointer']=='00000000' and after['source_shared_DirectSound_pointer']=='0b000004'
 assert before['driver_pointer']=='00000000' and after['driver_pointer']=='0a01b7bc'
 assert bytes.fromhex(after['driver_2ad8_hex'])==bytes(0x2AD8)
 assert after['source_sound_initialized_78']==0 and after['source_DirectSound_private_pool']=='00000000'
 device=bytes.fromhex(after['source_DirectSound_2c_hex'])
 assert device==bytes.fromhex(blocks[0]['retained_raw_storage_hex'])[4:48]
 assert word(device)==0x41D760 and word(device,4)==2 and word(device,0x10)==word(device,0x14)==0xB000014
 params=bytes.fromhex(blocks[1]['retained_raw_storage_hex'])[4:184]
 assert word(params)==0x41D724 and word(params,4)==1
 assert params[0x38:0x38+60]==x.read(0x38BE2C,60) and params[0x84:0x84+48]==x.read(0x38BDF8,48)
 assert word(params,0x30)==0x3F and word(params,0x80)==0xFFF
 expected=['002205c0','00383dd7','003804de','00383d6a','003802bf','00383f92','00383ecc','00380772',
  '00383c31','003802bf','00383f92','00383ecc','003832aa']
 assert [z['entry']for z in d['original_device_calls']]==expected
 assert [z.get('return_eax')for z in d['original_device_calls']]==[None,None,'00000001',None,'0b000004','0b000004',
  '0b000000','0b000004',None,'0b001004','0b001004','0b001000',None]
 assert stop['kernel_target']=='80000018' and stop['kernel_ordinal']==24
 assert stop['original_call_site']=='00379ffd' and stop['original_return_site']=='0037a002'
 assert stop['original_call_bytes']==x.read(0x379FFD,5).hex()=='e86a35f6ff'
 assert stop['last_original_instruction_before_kernel']=='002dd56c'
 request=stop['unprovided_nonvolatile_query']
 assert request['ValueIndex']=='00000009' and request['ValueLength']==4 and request['ResultLength_pointer']=='00000000'
 assert not request['source_nonvolatile_settings_value_or_return_supplied']
 statistics=r['retained_original_statistics'];assert word(bytes.fromhex(statistics['counter_storage_10_hex']))==8192
 assert bytes.fromhex(statistics['counter_storage_10_hex'])[4:]==bytes(12)
 assert statistics['original_audio_flags_override_38d148']=='ffffffff' and statistics['effective_audio_flags_38d144']=='00000000'
 lock=bytes.fromhex(after['source_DirectSound_critical_section_1c_hex'])
 assert word(lock,16)==0 and word(lock,20)==1 and word(lock,24)==token
 assert len(d['critical_section_calls'])==1 and len(d['actual_host_atomic_helpers'])==2
 source_stores=[z for z in r['source_pool_stores']if int(z['instruction'],16)<0x400000]
 compatibility_stores=[z for z in r['source_pool_stores']if int(z['instruction'],16)>=0x400000]
 assert [z['instruction']for z in compatibility_stores]==['0940004b','0940004e']
 rows=count=0
 for report,path in((r,rpath),(s,spath)):
  for z in report['instruction_listings']+[report['executed_original_pool_listing']]:
   p=path.parent/'instructions'/z['file'];assert digest(p)==z['sha256']
   for line in p.read_text().splitlines():
    address,raw,*_=line.split();code=bytes.fromhex(raw);assert x.read(int(address,16),len(code))==code
   if report is r:count+=1;rows+=len(p.read_text().splitlines())
 sources=json.loads((ROOT/'analysis/multiplayer/integration/spawn-source/kernel-sync-world/sources.json').read_text())
 header=next(z for z in sources if z['saved']=='XboxDev-xboxkrnl.h')
 assert digest(ROOT/'analysis/multiplayer/integration/spawn-source/kernel-sync-world/XboxDev-xboxkrnl.h')==header['sha256']
 fixture={'oracle_sha256':ORACLE_PIN,'suite_sha256':digest(OUT/'frozen/suite.json'),'source_sha256':PIN,
  'original_static_initializer_table':table,'original_statistics_owners':owners,
  'source_device_prefix':d,'raw_pool_provider_requests':services,'retained_raw_pool_blocks':blocks,
  'retained_original_statistics':statistics,'source_original_pool_stores':source_stores,'compatibility_body_stores':compatibility_stores,
  'measured_clock_calls':r['clock_calls'],'independent_original_reexecution_matches':True,
  'pool_capacity_is_host_provider_measurement_not_retail_rounding':True,
  'partial_lock_ownership_and_objects_retained_no_cleanup_or_unwind_result_supplied':True,
  'boundaries':r['boundaries'],'native_gameplay_update_applied':False}
 p=OUT/'source-fixture.json';p.write_text(json.dumps(fixture,indent=2)+'\n')
 result={'oracle_sha256':ORACLE_PIN,'suite_sha256':digest(OUT/'frozen/suite.json'),'source_fixture_sha256':digest(p),
  'independent_original_reexecutions':1,'original_statistics_initializers_executed':4,'genuine_host_pool_allocations':2,
  'original_pool_size_queries':2,'original_pool_zero_dword_stores':2048,'original_pool_marker_stores':2,
  'compatibility_mapped_capacity_bytes_each':4096,'source_statistics_total_bytes':8192,
  'original_DirectSound_record_constructors_completed':1,'source_parameter_constructor_partial':True,
  'original_instruction_listings':count,'original_instruction_rows_verified':rows,
  'executed_original_source_addresses':r['executed_original_pool_listing']['instructions'],
  'unprovided_kernel_ordinal':24,'unprovided_nonvolatile_value_index':9,'unprovided_nonvolatile_value_length':4,
  'source_driver_pointer':'0a01b7bc','source_shared_device_pointer':'0b000004','original_audio_initialized':0,
  'critical_section_owned_at_dependency':True,'compatibility_body_store_count':len(compatibility_stores),
  'original_DirectSound_success_capability_nonvolatile_or_consumption_outputs_supplied':False,
  'full_original_audio_initializations':0,'earlier_frozen_pins_verified':FROZEN,'primary_kernel_ABI_header':header,
  'native_gameplay_update_applied':False}
 (OUT/'audit.json').write_text(json.dumps(result,indent=2)+'\n')
 print(json.dumps({k:v for k,v in result.items()if k!='earlier_frozen_pins_verified'},indent=2))
if __name__=='__main__':main()
