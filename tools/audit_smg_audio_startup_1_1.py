#!/usr/bin/env python3
"""Audit only the independent startup measurement/replay and immutable pins."""
import copy,hashlib,json,struct
from pathlib import Path
from oracle_smg_audio_startup_1_1 import ROOT,OUT,PIN,CACHE_PIN,CLOCK_SERVICES,CONTEXT_EXE_PIN
from oracle_smg_firing_1_1 import Xbe
from audit_smg_audio_cache_1_1 import PRIOR_PINS as OLDER
ORACLE_PIN='cbca5876b76195d85071aebfa7f404544f95152a832b9f038deb499d4f4ca6a3'
PRIOR_PINS={**OLDER,
 'tools/oracle_smg_audio_cache_1_1.py':'9c546c136b0056a741b1d5c1c519cc476f4cbdaacd6c346bc3661d63fdf262a1',
 'tools/audit_smg_audio_cache_1_1.py':'74d69c6b3cda4d0a569e8291747184ef9c2f6869c50de75387ad6a0a3c987b74',
 'docs/SMG_AUDIO_CACHE_1_1_RESEARCH.md':'887831f4023661b03ca33cd8c065842e8bad773ee43ff60011b688d6140e3ebf',
 'analysis/multiplayer/map-pack/smg-audio-cache/frozen/suite.json':'69baec9c569ba241cdf4fa5f911658d284f9e3241cd606ef659b1898d16eee83',
 'analysis/multiplayer/map-pack/smg-audio-cache/source-fixture.json':'c1256f79374fed79416b3e4ee89c8e83fd863ae1d25067f10302b16d3e840586',
 'analysis/multiplayer/map-pack/smg-audio-cache/audit.json':'9754cfefa8c3e5e334f07e3400ebb11c1853e7287b184d2bc032534af2045853',
 'tools/oracle_mp_spawn_complete_kpcr.py':'a29845443fe79c75572c127f6e22a6da25cc0a266bc249d073f0d94b6df0b336',
 'docs/MP_COMPLETE_COMPAT_KPCR_RESEARCH.md':'75543439d7bb17e8804a38f2d1eaffdf3b44bae715c0713ea79715f543eedb57',
 'analysis/multiplayer/integration/spawn-source/complete-compat-kpcr/report.json':'f99ab55c6a3de3fbe48087b96d9db31e8a50e38c73b959b263b52b0bec98f8a6',
 'analysis/multiplayer/integration/spawn-source/complete-compat-kpcr/generator.exe':CONTEXT_EXE_PIN}
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def word(b,o):return int.from_bytes(b[o:o+4],'little')
def flt(b,o=0):return struct.unpack_from('<f',b,o)[0]
def f32(v):return struct.unpack('<f',struct.pack('<f',v))[0]
def bits(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def source_only(r):
 if isinstance(r,list):return[source_only(v)for v in r]
 if isinstance(r,dict):return{k:source_only(v)for k,v in r.items()if k not in('provider_mode','producer_mode','snapshot_path','instruction_inventory')}
 return r
def main():
 assert sha(ROOT/'tools/oracle_smg_audio_startup_1_1.py')==ORACLE_PIN
 for p,h in PRIOR_PINS.items():assert sha(ROOT/p)==h,p
 x=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');assert hashlib.sha256(x.data).hexdigest()==PIN
 assert sha(ROOT/'assets/disc/maps/lockout.map')==CACHE_PIN
 suite=json.loads((OUT/'frozen/suite.json').read_text());cases=[];listing_count=instruction_count=clock_count=streams=voices=0
 for case in suite['cases']:
  p=ROOT/case['path'];q=ROOT/case['reexecution_path'];assert sha(p)==case['sha256'] and sha(q)==case['reexecution_sha256']
  r=json.loads(p.read_text());s=json.loads(q.read_text());assert source_only(r)==source_only(s)
  assert r['oracle_sha256']==s['oracle_sha256']==ORACLE_PIN and r['xbe_sha256']==PIN and r['cache_sha256']==CACHE_PIN
  assert r['provider_mode']=='live_host_measurement' and s['provider_mode']=='replay_of_recorded_live_provider_input'
  assert not r['source_controller_seed_initialized'] and not r['audio_available_forced'] and not r['source_audio_initialization_completed']
  assert not r['original_sound_device_capabilities_voices_streams_or_consumption_supplied'] and not r['native_gameplay_update_applied']
  assert all(not z['original_result_supplied']for z in r['boundaries'])
  compat=r['compatibility_context'] is not None
  assert [z['instruction']for z in r['boundaries']]==(
   ['002dac7f','003804f2','002204e6']+['003804f2']*4 if compat else['0032c609','003804de','002204e6']+['003804de']*4)
  if compat:
   z=r['compatibility_context'];assert z['producer_mode']=='real_live_Wine32_worker_held_alive_during_consumers'
   assert s['compatibility_context']['producer_mode']=='replay_of_recorded_whole_real_host_context'
   assert sha(ROOT/z['snapshot_path'])==z['snapshot_sha256'];d=json.loads((ROOT/z['snapshot_path']).read_text())
   assert len(bytes.fromhex(d['kpcr_hex']))==0x280 and len(bytes.fromhex(d['ethread_hex']))==0x140
   assert word(bytes.fromhex(d['kpcr_hex']),0x28)==d['ethread_address'] and word(bytes.fromhex(d['ethread_hex']),0x28)==0
   assert d['fpu_control_word']==0x27f and all(v['actual_ntstatus']==0 for v in d['allocations'])
   assert r['boundaries'][0]['address']=='00000018'
   assert r['boundaries'][1]['original_critical_section_1c_hex']==x.read(0x38D158,0x1C).hex()
  for step in r['steps']:
   a=step['after'];assert a['audio']['initialized_78']==0 and a['rand_fallback_pointer']=='00000000'
   assert a['local_seed']=='00000000' and a['original_process_heap_pointer']=='00000000'
   if a['controller_root_1c_hex'] is not None:assert bytes.fromhex(a['controller_root_1c_hex'])==bytes(0x1C)
  assert bytes.fromhex(r['steps'][1]['after']['driver_2ad8_hex'])==bytes(0x2AD8)
  assert r['steps'][1]['after']['arena_used']-r['steps'][1]['before']['arena_used']==0x2AD8
  assert len(r['clock_calls'])==8
  for z in r['clock_calls']:
   inp=z['measured_provider_input'];ns=inp['observed_ns'];assert z['no_source_rng_or_ready_field_written_by_provider']
   if z['entry']=='00336983':
    assert inp['filetime_100ns']==ns//100+116444736000000000
    assert bytes.fromhex(z['output_after_hex'])==struct.pack('<Q',inp['filetime_100ns'])
   else:assert inp['tick_ms_u32']==(ns//1000000)&0xFFFFFFFF and int(z['return_eax'],16)==inp['tick_ms_u32']
   clock_count+=1
  tc=next(z for z in r['calls']if z['entry']=='0032803e');assert int(tc['return_eax'],16)==r['clock_calls'][0]['measured_provider_input']['observed_ns']//1000000000
  # Source21BCF6 controller seed assignment and source32C66A initial seed
  # assignment are not reached in either case.
  assert not any(z['instruction']in('0021bcf6','0032c66a','00328519')for z in r['writes'])
  for step in r['steps'][3:9]:
   assert step['completed'] and len(step['clock_calls'])==1
   b=bytes.fromhex(step['before']['audio']['sound_system_20c_hex']);a=bytes.fromhex(step['after']['audio']['sound_system_20c_hex'])
   delta=(word(a,0x80)-word(b,0x80))&0xFFFFFFFF;signed=delta if delta<0x80000000 else delta-0x100000000
   assert word(a,0x80)==step['clock_calls'][0]['measured_provider_input']['tick_ms_u32']
   assert word(a,0x84)==(word(b,0x84)+delta)&0xFFFFFFFF
   scaled=f32(f32(signed)*flt(x.read(0x4650E0,4)));assert word(a,0x1E0)==bits(scaled)
   for off in(0x1FC,0x204):assert word(a,off)==bits(f32(flt(b,off)+scaled))
   assert word(a,0x1E4)==word(x.read(0x465030,4),0)
  stream=[z for z in r['calls']if z['entry']=='00383b22'];assert len(stream)==3
  expected=[(0,0x69,1,36,4,2,64),(40,0x69,2,72,4,2,64),(81,1,2,4,16,0,64)]
  for z,(index,fmt,ch,align,bps,extra,samples)in zip(stream,expected):
   b=bytes.fromhex(z['original_stream_descriptor_18_hex']);wave=bytes.fromhex(z['original_waveformat_14_hex'])
   assert [word(b,i)for i in(0,4,12,16,20)]==[0x20000000,2,0x222840,index,0]
   assert int.from_bytes(wave[:2],'little')==fmt and int.from_bytes(wave[2:4],'little')==ch and word(wave,4)==44100
   assert int.from_bytes(wave[12:14],'little')==align and int.from_bytes(wave[14:16],'little')==bps
   assert int.from_bytes(wave[16:18],'little')==extra
   if extra:assert int.from_bytes(wave[18:20],'little')==samples
   assert z['original_device_pointer']=='00000000' and 'return_eax'not in z;streams+=1
  voice=next(z for z in r['calls']if z['entry']=='00383afe');b=bytes.fromhex(voice['original_buffer_descriptor_18_hex'])
  assert [word(b,i)for i in range(0,24,4)]==[24,0x2010,0,0,0,0];assert voice['original_device_pointer']=='00000000';voices+=1
  table=r['original_callback_table'];assert bytes.fromhex(table['raw_hex'])==x.read(0x447428,0x990)
  assert table['sound_row_index']==44 and [word(bytes.fromhex(table['sound_row_24_hex']),i)for i in(0,8)]==[0x126B20,0x126DE0]
  invs=[]
  for report in(r,s):
   inv=ROOT/report['instruction_inventory']['path'];assert sha(inv)==report['instruction_inventory']['sha256'];rows=json.loads(inv.read_text())['routines']
   for row in rows:
    path=ROOT/row['path'];assert sha(path)==row['sha256']
    assert row['host_provider_entry_instructions_substituted']==(int(row['entry'],16)in CLOCK_SERVICES)
    for line in path.read_text().splitlines():
     a,raw,*_=line.split();b=bytes.fromhex(raw);assert x.read(int(a,16),len(b))==b
     if report is r:instruction_count+=1
    if report is r:listing_count+=1
   invs.append([{k:v for k,v in z.items()if k!='path'}for z in rows])
  assert invs[0]==invs[1]
  cases.append({'case':case['case'],'steps':r['steps'],'source_calls':r['calls'],'source_writes':r['writes'],
   'recorded_real_clock_inputs':r['clock_calls'],'boundaries':r['boundaries'],'compatibility_context':r['compatibility_context'],
   'instruction_inventory':r['instruction_inventory']})
 primary=json.loads((OUT/'reference/cxbx-585c49a/provenance.json').read_text())
 for row in primary['files']:assert sha(ROOT/row['path'])==row['sha256']
 sdl=json.loads((OUT/'sdl-provider.json').read_text());assert sha(ROOT/'tools/probe_sdl_audio_provider.py')==sdl['tool_sha256']
 assert not sdl['SDL_driver_forced'] and not sdl['original_source_constructor_or_DirectSound_result_supplied']
 if sdl.get('actual_SDL_device_id',0):assert sdl['actual_audio_driver']not in('dummy','disk') and sdl['actual_device_status']==2 and sdl['actual_device_closed']
 fixture={'oracle_sha256':ORACLE_PIN,'suite_sha256':sha(OUT/'frozen/suite.json'),'xbe_sha256':PIN,'cache_sha256':CACHE_PIN,
  'cases':cases,'actual_host_clock_and_complete_context_snapshots_remain_explicit_provider_inputs':True,
  'reexecution_replays_observed_real_provider_inputs_not_a_second_live_clock_measurement':True,
  'source_seed_device_caps_consumption_availability_or_complete_map_activation_proven':False,'native_gameplay_update_applied':False}
 p=OUT/'source-fixture.json';p.write_text(json.dumps(fixture,indent=2)+'\n')
 audit={'oracle_sha256':ORACLE_PIN,'suite_sha256':sha(OUT/'frozen/suite.json'),'source_fixture_sha256':sha(p),
  'independent_original_reexecutions_of_recorded_actual_provider_inputs':2,'original_instruction_listings':listing_count,
  'original_instruction_rows_verified':instruction_count,'recorded_real_host_clock_calls':clock_count,
  'original_clock_updates':12,'original_unadmitted_stream_requests':streams,'original_unadmitted_voice_requests':voices,
  'earlier_frozen_pins_verified':PRIOR_PINS,'primary_compatibility_sources_verified':primary,
  'successful_genuine_paused_SDL_device_open':bool(sdl.get('actual_SDL_device_id',0)),
  'successful_original_audio_initializations_or_source_seed_initializations':0,
  'native_gameplay_update_applied':False}
 (OUT/'audit.json').write_text(json.dumps(audit,indent=2)+'\n');print(json.dumps({k:v for k,v in audit.items()if k not in('earlier_frozen_pins_verified','primary_compatibility_sources_verified')},indent=2))
if __name__=='__main__':main()
