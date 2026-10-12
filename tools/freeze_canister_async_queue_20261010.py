#!/usr/bin/env python3
"""Freeze only the repeated canister native-queue prefix and exact stops."""
import hashlib,json,platform,shutil,sys
from pathlib import Path
import capstone,unicorn
ROOT=Path(__file__).resolve().parents[1]
SCOPE=ROOT/'analysis/multiplayer/integration/canister-async-queue-20261010'
OUT=SCOPE/'frozen'
OWN=['tools/oracle_canister_async_queue_20261010.py','tools/freeze_canister_async_queue_20261010.py','docs/MP_CANISTER_ASYNC_QUEUE_RESEARCH.md']
PRIOR=ROOT/'analysis/multiplayer/integration/canister-texture-cache-20261010/frozen/manifest.json'
def sha(p):
 h=hashlib.sha256()
 with Path(p).open('rb')as f:
  for b in iter(lambda:f.read(1048576),b''):h.update(b)
 return h.hexdigest()
def encoded(v):return json.dumps(v,sort_keys=True,indent=2)+'\n'
def collect(m):
 result={}
 for bank in('artifact_files','owned_live_files','protected_inputs','independently_reaudited_vertex_and_prior_pins','external_runtime_dependencies'):
  for n,h in m.get(bank,{}).items():
   assert n not in result or result[n]==h,n
   result[n]=h
 return result
def main():
 assert not OUT.exists()
 assert sha(PRIOR)=='22e60be0dbfe8fb3acac7f5f737208ef3f5208ea1dc5b50f015879dd217ccb3b'
 protected=collect(json.loads(PRIOR.read_text()));protected[str(PRIOR.relative_to(ROOT))]=sha(PRIOR)
 for n,h in protected.items():assert sha(ROOT/n)==h,n
 a=SCOPE/'measured';b=SCOPE/'repeated';r=json.loads((a/'report.json').read_text())
 files=['report.json']+[str(p.relative_to(a))for p in sorted((a/'instructions').glob('*.asm'))]
 assert len(files)==90
 for n in files:assert(a/n).read_bytes()==(b/n).read_bytes(),n
 assert r['tool_sha256']==sha(ROOT/OWN[0])and r['factory_correlated']
 assert len(r['stages'])==20 and sum(s['completed']for s in r['stages'])==17
 assert len(r['calls'])==88 and len(r['stores'])==22907 and len(r['instructions'])==89
 assert all(not s['used_services']for s in r['stages'])
 assert [s['stopped_eip']for s in r['stages']if not s['completed']]==['002d8606','0021f366','001a47d0']
 constructor=next(q for q in r['calls']if q['entry']=='001221b0');assert 'return_cpu'not in constructor
 assert r['stages'][0]['unreturned_effect_call_ids']==[constructor['id']]
 assert sum(q['phase']=='source_async_constructor_prefix'for q in r['stores'])==2709
 native=r['native_calls'];assert len(native)==15
 creation=[q for q in native if q.get('source_entry')in('002dc83a','002dc7b9')]
 assert [q['native_handle']for q in creation]==['f5100001','f5100002','f5100003']and all(q['native_error']==0 for q in creation)
 waits=[q for q in native if q.get('source_entry')=='800000ea'];assert len(waits)==4
 assert all(q['actual_wait_state']==0 and q['native_error']==0 and q['translated_status']=='00000000'and q['arguments'][1:]==['00000001','00000000','00000000']for q in waits)
 wrappers=[q for q in native if q.get('entry')=='002dc9c8'];assert len(wrappers)==4
 assert all(q['wrapper_return_cpu']['eax']=='00000000'and q['arguments'][1]=='ffffffff'for q in wrappers)
 release=next(q for q in native if q.get('source_entry')=='002dc814');assert release['actual_released']==1 and release['native_error']==0 and release['previous_count']==0
 assert r['native_objects_retained_until_end']==3 and r['native_owner_destroyed']
 for i,expected in((0,(0,0x4e7938,0,0)),(-2,(1,0x4e7978,0x4e7938,1)),(-1,(1,0x4e7978,0x4e7938,1))):
  c=r['stages'][i]['contexts_after'];raw=bytes.fromhex(c['async_queue_raw_hex']);base=0x4e7924
  u=lambda a:int.from_bytes(raw[a-base:a-base+4],'little')
  assert(u(0x4e7934),u(0x4e9eb8),u(0x4e9ebc),u(0x4e9ec0))==expected
  assert u(0x4e7924)==0 and(u(0x4e7928),u(0x4e792c),u(0x4e7930))==(0xf5100001,0xf5100002,0xf5100003)
  assert c['actual_file_bank_hex']=='00'*0x3860
  if i<0:
   node=raw[0x14:0x54];words=[int.from_bytes(node[n:n+4],'little')for n in range(0,64,4)]
   assert words==[2,0,0,0x80061000,0xc00,0x90d4600,0,0,0,0,0,0,3,0x1a47d0,0x7e42004f,0]
   texture=bytes.fromhex(c['004eda24']['raw_hex']);assert texture[0x4f]==0
 assert r['stages'][-1]['contexts_before']['current_node_hex']=='00000000'and r['stages'][-1]['contexts_after']['current_node_hex']=='38794e00'
 stop=r['stages'][-1]['invalid'];assert stop['payload']=='004e7940'and stop['overlapping_imported_tags']==[]and stop['cache_file_offset']=='01c34800'
 assert stop['payload28_hex'].startswith('0000000000100680000c000000460d09')and len(bytes.fromhex(stop['destination_before_hex']))==0xc00
 for stage in r['stages'][-2:]:
  for p in stage['pools_after']:
   if p['global_address']in('00518464','00526514','00526518','0052651c','004f1f20'):assert not p['occupied']
 for entry,eax in(('0012ed10','00000001'),('00180980','00000000')):
  assert next(q for q in r['calls']if q['entry']==entry)['return_cpu']['eax']==eax
 assert len(r['platform_calls'])==len(r['platform_cleanup'])==4 and all(q['actual_munmap_result']==0 for q in r['platform_cleanup'])
 assert len(r['applied_access_calls'])==1 and r['applied_access_calls'][0]['contents_before_sha256']==r['applied_access_calls'][0]['contents_after_sha256']
 for n,h in r['protected_inputs'].items():assert sha(ROOT/n)==h,n
 OUT.mkdir()
 for n in OWN:
  target=OUT/'source'/n;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/n,target)
 logs=OUT/'logs';logs.mkdir()
 for name in('canister-async-queue-final-measured.log','canister-async-queue-final-repeated.log'):shutil.copy2(Path('/tmp')/name,logs/name)
 deps={str(p):sha(p)for p in[Path(sys.executable).resolve(),Path('/usr/lib/libc.so.6'),Path('/usr/lib/libstdc++.so.6').resolve(),Path('/usr/lib/libgcc_s.so.1'),Path('/usr/bin/g++').resolve(),Path(unicorn.__file__).parent/'lib/libunicorn.so.2',Path(capstone.__file__).parent/'lib/libcapstone.so']}
 (OUT/'dependencies.json').write_text(encoded(dict(files=deps,python=sys.version,unicorn=unicorn.__version__,capstone=capstone.__version__,libc=platform.libc_ver(),observed_host=r['host'])))
 commands=dict(cwd=str(ROOT),environment={'PYTHONPATH':'tools'},commands=[f'PYTHONPATH=tools .tools/python/bin/python tools/oracle_canister_async_queue_20261010.py --output {str((p/"report.json").relative_to(ROOT))}'for p in(a,b)],native_compilation='g++ -std=c++20 -O2 -fPIC -shared -pthread -I analysis/multiplayer/integration/native-worker-threads-20261009/frozen/source/include analysis/multiplayer/integration/canister-async-queue-20261010/native/provider.cpp analysis/multiplayer/integration/native-worker-threads-20261009/frozen/source/src/native_synchronization.cpp -o analysis/multiplayer/integration/canister-async-queue-20261010/native/provider.so',freeze_command='.tools/python/bin/python tools/freeze_canister_async_queue_20261010.py',gpu_used=False,production_changed=False)
 (OUT/'commands.json').write_text(encoded(commands))
 summary=dict(roots=20,returns=17,stops=3,calls=88,ordered_nonstack_stores=22907,instruction_files=89,byte_identical_repeat_files=90,native_sync_objects=3,real_acquisitions=4,real_semaphore_publications=1,real_semaphore_consumptions=1,actual_native_allocations=4,actual_native_munmaps=4,actually_applied_protections=1,particle_births=0,IO_performed=False,ordinary_effect_stop=r['stages'][-2]['invalid'],worker_file_stop=stop,final_report_sha256=sha(a/'report.json'))
 (OUT/'summary.json').write_text(encoded(summary))
 artifacts={str(p.relative_to(ROOT)):sha(p)for p in SCOPE.rglob('*')if p.is_file()}
 m=dict(schema='original-canister-async-queue-real-native-ownership-freeze-v1',artifact_files=artifacts,owned_live_files={n:sha(ROOT/n)for n in OWN},protected_inputs=r['protected_inputs'],independently_reaudited_texture_and_prior_pins=protected,external_runtime_dependencies=deps,summary=summary,bounds=r['boundaries'])
 (OUT/'manifest.json').write_text(encoded(m))
 pins=artifacts|m['owned_live_files']|r['protected_inputs']|protected|deps
 bad=[n for n,h in pins.items()if sha(ROOT/n)!=h];assert not bad,bad
 (OUT/'audit.json').write_text(encoded(dict(unique_pins=len(pins),mismatches=bad,manifest_sha256=sha(OUT/'manifest.json'))))
 print(encoded(dict(manifest=str(OUT/'manifest.json'),sha256=sha(OUT/'manifest.json'),artifact_files=len(artifacts),unique_pins=len(pins),mismatches=0,summary={k:v for k,v in summary.items()if not isinstance(v,dict)})))
if __name__=='__main__':main()
