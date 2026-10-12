#!/usr/bin/env python3
"""Independent byte/arithmetic audit of measured original boot RNG producers."""
import hashlib,json,struct
from pathlib import Path
from oracle_smg_audio_rng_startup_1_1 import ROOT,OUT,PIN,CACHE_PIN,FROZEN,digest
from oracle_smg_firing_1_1 import Xbe

ORACLE_PIN='c67ddaa11d18fb50a67eea73e213dd926b37f343dee1845d15f37ba702ef9cc6'
def word(b,o=0):return int.from_bytes(b[o:o+4],'little')
def source_only(v):
 if isinstance(v,list):return[source_only(x)for x in v]
 if isinstance(v,dict):return{k:source_only(x)for k,x in v.items()if k not in('provider_mode','clock_mode','context_mode','actual_host_address')}
 return v
def main():
 assert digest(ROOT/'tools/oracle_smg_audio_rng_startup_1_1.py')==ORACLE_PIN
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 suite=json.loads((OUT/'frozen/suite.json').read_text());rpath=ROOT/suite['measurement']['path'];spath=ROOT/suite['reexecution']['path']
 assert digest(rpath)==suite['measurement']['sha256'] and digest(spath)==suite['reexecution']['sha256']
 r=json.loads(rpath.read_text());s=json.loads(spath.read_text());assert source_only(r)==source_only(s)
 x=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
 assert hashlib.sha256(x.data).hexdigest()==PIN and r['source_sha256']==PIN and r['cache_sha256']==CACHE_PIN
 assert r['oracle_sha256']==s['oracle_sha256']==ORACLE_PIN
 assert r['context_mode']=='real_Wine_worker_alive_during_source_execution' and s['context_mode']=='whole_recorded_context_replay'
 assert r['clock_mode']=='real_host_clock_measurement' and s['clock_mode']=='verbatim_measured_input_replay'
 assert r['whole_compatibility_context_transferred_no_pointer_or_selected_field_patch']
 assert not r['original_audio_driver_or_playback_result_supplied'] and not r['native_gameplay_update_applied']
 assert len(r['clock_calls'])==4 and [c['entry']for c in r['clock_calls']]==['00336983','00337a40']*2
 for c in r['clock_calls']:
  v=c['measured_provider_input'];ns=v['observed_ns'];assert c['no_source_rng_or_ready_field_written_by_provider']
  if c['entry']=='00336983':
   assert v['filetime_100ns']==ns//100+116444736000000000
   assert bytes.fromhex(c['output_after_hex'])==struct.pack('<Q',v['filetime_100ns'])
  else:assert v['tick_ms_u32']==(ns//1000000)&0xFFFFFFFF and int(c['return_eax'],16)==v['tick_ms_u32']
 constructors=[z for z in r['rng_calls']if z['entry']=='001479b0'];assert len(constructors)==1
 times=[z for z in r['rng_calls']if z['entry']=='0032803e'];randoms=[z for z in r['rng_calls']if z['entry']=='00328505']
 assert len(times)==len(randoms)==2
 for z,c in zip(times,r['clock_calls'][::2]):assert int(z['return_eax'],16)==c['measured_provider_input']['observed_ns']//1000000000
 assert [int(z['return_eax'],16)for z in randoms]==[41,18467]
 calls=[z['entry']for z in r['rng_calls']]
 assert calls.index('001479b0')<calls.index('0021bc50')<calls.index('002205c0')
 assert len([z for z in r['rng_calls']if z['entry']=='0032e813'])==1
 writes=r['rng_writes'];seed=[z for z in writes if z['instruction']=='0032c66a'];assert len(seed)==1
 assert seed[0]['after_hex']=='01000000'
 updates=[z for z in writes if z['instruction']=='00328519'];assert len(updates)==2
 crt=1
 for z,result in zip(updates,randoms):
  assert word(bytes.fromhex(z['before_hex']))==crt
  crt=(crt*0x343FD+0x269EC3)&0xFFFFFFFF
  assert word(bytes.fromhex(z['after_hex']))==crt and (crt>>16)&0x7FFF==int(result['return_eax'],16)
 local=next(z for z in writes if z['instruction']=='00147a13')
 controller=next(z for z in writes if z['instruction']=='0021bcf6')
 for z,ti,ra,c in zip((local,controller),times,randoms,r['clock_calls'][1::2]):
  expected=int(ti['return_eax'],16)^int(ra['return_eax'],16)^c['measured_provider_input']['tick_ms_u32']
  assert word(bytes.fromhex(z['after_hex']))==expected
 shared=next(z for z in writes if z['instruction']=='001479ec');assert shared['after_hex']=='a8780000'
 ctx_path=rpath.parent/'raw-host-context.json';assert digest(ctx_path)==r['context_snapshot_sha256']
 ctx=json.loads(ctx_path.read_text());assert word(bytes.fromhex(ctx['kpcr_hex']),0x28)==ctx['ethread_address']
 assert word(bytes.fromhex(ctx['ethread_hex']),0x28)==0
 thread_id=word(bytes.fromhex(ctx['ethread_hex']),0x12C)
 getter=next(z for z in r['rng_calls']if z['entry']=='002d8674');assert int(getter['return_eax'],16)==thread_id
 retained=r['retained'];assert retained['compatibility_KTHREAD_thread_id']==thread_id
 assert retained['recorded_real_Wine_thread_id']==ctx['native_thread_id']
 crt_raw=bytes.fromhex(retained['CRT_fallback_88_hex']);assert word(crt_raw)==thread_id and word(crt_raw,4)==0xFFFFFFFF
 assert word(crt_raw,0x14)==crt and word(crt_raw,0x54)==0x469C18
 assert retained['game_rng_pointer']=='82d4828c' and word(bytes.fromhex(retained['game_rng_8_hex']))==0x78A8
 assert word(bytes.fromhex(retained['game_rng_8_hex']),4)==word(bytes.fromhex(local['after_hex']))
 assert word(bytes.fromhex(retained['controller_root_1c_hex']),8)==word(bytes.fromhex(controller['after_hex']))
 assert retained['audio_initialized_78']==0 and r['rows'][-1]['producer']['completion']=='blocked_kernel_import'
 assert r['rows'][-1]['producer']['stopped_eip']=='002205c0'
 assert r['rows'][1]['producer']['completion']==r['rows'][2]['producer']['completion']=='complete_return'
 assert r['rows'][4]['producer']['completion']=='complete_return'
 table=r['original_callback_table'];assert bytes.fromhex(table['raw_hex'])==x.read(0x447428,0x990)
 assert word(bytes.fromhex(table['rng_constructor_hex']))==0x1479B0 and table['rng_row_index']==3
 provider_calls=r['raw_heap_provider_calls'];assert len(provider_calls)==7
 assert all(z['provider_writes_only_raw_base_and_access']for z in provider_calls)
 for z in provider_calls:
  if 'raw_provider_operation'in z:
   op=z['raw_provider_operation'];assert op['actual_errno']==0
   if 'actual_result'in op:assert op['actual_result']==0
  if 'compatibility_NTSTATUS_from_actual_OS_success'in z:assert z['compatibility_NTSTATUS_from_actual_OS_success']=='00000000' and z['retail_Xbox_NTSTATUS_not_claimed']
 count=rows=0
 for report,path in((r,rpath),(s,spath)):
  for z in report['instruction_listings']+[report['executed_original_instruction_listing']]:
   p=path.parent/'instructions'/z['file'];assert digest(p)==z['sha256']
   for line in p.read_text().splitlines():
    a,raw,*_=line.split();b=bytes.fromhex(raw);assert x.read(int(a,16),len(b))==b
   if report is r:count+=1;rows+=len(p.read_text().splitlines())
 p=OUT/'source-fixture.json';p.write_text(json.dumps({'oracle_sha256':ORACLE_PIN,'suite_sha256':digest(OUT/'frozen/suite.json'),
  'measurement':r,'reexecution_source_outputs_equal':True,'boundaries':r['boundaries'],
  'seed_bytes_are_measured_outputs_not_constants_for_native_initialization':True,'native_gameplay_update_applied':False},indent=2)+'\n')
 result={'oracle_sha256':ORACLE_PIN,'source_fixture_sha256':digest(p),'suite_sha256':digest(OUT/'frozen/suite.json'),
  'independent_original_reexecutions':1,'genuine_recorded_host_clock_calls':4,'source_cold_CRT_seed_stores':1,
  'source_CRT_LCG_stores':2,'source_local_seed_stores':1,'source_controller_seed_stores':1,'original_instruction_listings':count,
  'original_instruction_rows_verified':rows,'source_executed_addresses_verified':r['executed_original_instruction_listing']['rows'],
  'source_CRT_rand_outputs':[41,18467],'source_thread_id':thread_id,'separately_recorded_real_host_thread_id':ctx['native_thread_id'],
  'earlier_frozen_pins_verified':FROZEN,'source_audio_initialized':0,'normal_full_Xbox_title_startup_proven':False,'native_gameplay_update_applied':False}
 (OUT/'audit.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items()if k!='earlier_frozen_pins_verified'},indent=2))
if __name__=='__main__':main()
