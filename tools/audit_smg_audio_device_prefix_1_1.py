#!/usr/bin/env python3
"""Audit original device prefix/repetition without supplying kernel success."""
import hashlib,json
from pathlib import Path
from oracle_smg_audio_device_prefix_1_1 import ROOT,OUT,PIN,FROZEN,digest
from oracle_smg_firing_1_1 import Xbe
from audit_smg_audio_rng_startup_1_1 import source_only,word

ORACLE_PIN='5313c91bd67a63aaf4aa3807dfd51fad01460e7dafa5503713cf107f17a81513'
def main():
 assert digest(ROOT/'tools/oracle_smg_audio_device_prefix_1_1.py')==ORACLE_PIN
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 suite=json.loads((OUT/'frozen/suite.json').read_text());rpath=ROOT/suite['measurement']['path'];spath=ROOT/suite['reexecution']['path']
 assert digest(rpath)==suite['measurement']['sha256'] and digest(spath)==suite['reexecution']['sha256']
 r=json.loads(rpath.read_text());s=json.loads(spath.read_text());assert source_only(r)==source_only(s)
 assert r['oracle_sha256']==s['oracle_sha256']==ORACLE_PIN
 assert r['clock_mode']=='real_host_clock_measurement' and s['clock_mode']=='verbatim_measured_input_replay'
 assert r['context_mode']=='real_Wine_worker_alive_during_source_execution' and s['context_mode']=='whole_recorded_context_replay'
 assert r['source_sha256']==PIN and not r['new_raw_pool_device_capability_hardware_or_consumption_service_supplied']
 assert not r['whole_original_DirectSound_or_audio_initialization_completed'] and not r['native_gameplay_update_applied']
 x=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');assert hashlib.sha256(x.data).hexdigest()==PIN
 ctxpath=rpath.parent/'raw-host-context.json';assert digest(ctxpath)==r['context_snapshot_sha256'];ctx=json.loads(ctxpath.read_text())
 token=ctx['ethread_address'];d=r['source_device_prefix'];b=d['before'];a=d['after'];stop=d['first_unprovided_dependency']
 assert d['original_source_frame_preserved_from_126b20_call_126c90'] and not d['source_driver_constructor_or_DirectSound_return_supplied']
 assert stop['kernel_target']=='8000000f' and stop['kernel_ordinal']==15
 assert stop['original_call_site']=='00383ed5' and stop['original_return_site']=='00383edb' and stop['stack'][:2]==['00000030','626f5344']
 assert b['source_DirectSound_critical_section_1c_hex']==x.read(0x38D158,28).hex()
 before=bytes.fromhex(b['source_DirectSound_critical_section_1c_hex']);after=bytes.fromhex(a['source_DirectSound_critical_section_1c_hex'])
 assert after[:16]==before[:16] and word(before,16)==0xFFFFFFFF and word(before,20)==word(before,24)==0
 assert word(after,16)==0 and word(after,20)==1 and word(after,24)==token
 assert a['compatibility_current_thread_token']==f'{token:08x}'
 assert b['driver_pointer']=='00000000' and a['driver_pointer']=='0a01b7bc'
 assert bytes.fromhex(a['driver_2ad8_hex'])==bytes(0x2AD8)
 assert a['source_shared_DirectSound_pointer']=='00000000' and a['source_DirectSound_2c_hex']is None
 assert a['source_DirectSound_private_pool']=='00000000' and a['source_sound_initialized_78']==0
 assert [z['entry']for z in d['original_device_calls']]==['002205c0','00383dd7','003804de','00383d6a','003802bf','00383f92','00383ecc']
 guard=next(z for z in d['original_device_calls']if z['entry']=='003804de');assert guard['return_eax']=='00000001'
 assert all('return_eax'not in z for z in d['original_device_calls']if z is not guard)
 lock=d['critical_section_calls'];assert len(lock)==1
 assert lock[0]['ordinal']==277 and lock[0]['pointer']=='0038d158'
 assert lock[0]['before_hex']==before.hex() and lock[0]['after_hex']==after.hex()
 helpers=d['actual_host_atomic_helpers'];assert len(helpers)==2
 assert helpers[0]['token']==f'{token:08x}' and helpers[1]['kind']=='host_serialized_atomic_increment'
 assert helpers[1]['pointer']=='0038d168' and helpers[1]['before']=='ffffffff' and helpers[1]['after']=='00000000'
 # The raw device-store log includes the declared compiled compatibility body.
 # Split it explicitly; only source bytes are classified original instructions.
 source_stores=[];compatibility_stores=[]
 for z in d['original_device_stores']:
  if int(z['instruction'],16)<0x400000:source_stores.append(z)
  else:compatibility_stores.append(z)
 assert [z['instruction']for z in compatibility_stores]==['0940004b','0940004e']
 assert [z for z in source_stores if z['after_hex']!='00'*z['size']]==[
  {'phase':'resume_original_full_sound_constructor_into_real_device_prefix','instruction':'00220615','address':'00526474','size':4,'before_hex':'00000000','after_hex':'bcb7010a'}]
 rows=count=0
 for report,path in((r,rpath),(s,spath)):
  for z in report['instruction_listings']+[report['executed_original_device_listing']]:
   p=path.parent/'instructions'/z['file'];assert digest(p)==z['sha256']
   for line in p.read_text().splitlines():
    addr,raw,*_=line.split();code=bytes.fromhex(raw);assert x.read(int(addr,16),len(code))==code
   if report is r:count+=1;rows+=len(p.read_text().splitlines())
 sources=json.loads((ROOT/'analysis/multiplayer/integration/spawn-source/kernel-sync-world/sources.json').read_text())
 header=next(z for z in sources if z['saved']=='XboxDev-xboxkrnl.h')
 assert digest(ROOT/'analysis/multiplayer/integration/spawn-source/kernel-sync-world/XboxDev-xboxkrnl.h')==header['sha256']
 fixture={'oracle_sha256':ORACLE_PIN,'suite_sha256':digest(OUT/'frozen/suite.json'),'source_sha256':PIN,
  'source_device_prefix':d,'source_original_stores':source_stores,'compatibility_body_stores':compatibility_stores,
  'original_rng_preparation':r['original_rng_preparation'],'original_rng_writes':r['original_rng_writes'],
  'measured_clock_calls':r['clock_calls'],'independent_original_reexecution_matches':True,
  'partial_lock_ownership_retained_no_cleanup_or_unwind_result_supplied':True,'boundaries':r['boundaries'],
  'native_gameplay_update_applied':False}
 p=OUT/'source-fixture.json';p.write_text(json.dumps(fixture,indent=2)+'\n')
 result={'oracle_sha256':ORACLE_PIN,'suite_sha256':digest(OUT/'frozen/suite.json'),'source_fixture_sha256':digest(p),
  'independent_original_reexecutions':1,'original_instruction_listings':count,'original_instruction_rows_verified':rows,
  'executed_original_device_addresses':r['executed_original_device_listing']['instructions'],'source_guard_returns':1,
  'unprovided_kernel_ordinal':15,'unprovided_pool_requested_bytes':48,'unprovided_pool_tag':'626f5344',
  'source_device_driver_pointer':'0a01b7bc','original_audio_initialized':0,'critical_section_owned_at_dependency':True,
  'compatibility_body_store_count':2,'original_DirectSound_outputs_or_caps_supplied':False,
  'full_original_audio_initializations':0,'earlier_frozen_pins_verified':FROZEN,'primary_pool_ABI_header':header,
  'native_gameplay_update_applied':False}
 (OUT/'audit.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items()if k!='earlier_frozen_pins_verified'},indent=2))
if __name__=='__main__':main()
