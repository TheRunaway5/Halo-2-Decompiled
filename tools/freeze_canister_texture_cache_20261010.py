#!/usr/bin/env python3
"""Freeze separate genuine texture/decal owner and exact next async stop."""
import hashlib,json,platform,shutil,sys
from pathlib import Path
import capstone,unicorn
ROOT=Path(__file__).resolve().parents[1]
SCOPE=ROOT/'analysis/multiplayer/integration/canister-texture-cache-20261010'
OUT=SCOPE/'frozen'
OWN=['tools/oracle_canister_texture_cache_20261010.py','tools/freeze_canister_texture_cache_20261010.py','docs/MP_CANISTER_TEXTURE_CACHE_RESEARCH.md']
PRIOR=ROOT/'analysis/multiplayer/integration/canister-vertex-cache-20261010/frozen/manifest.json'
CORRECTION=ROOT/'analysis/multiplayer/integration/particle-pool-label-correction-20261010/manifest.json'
def sha(p):
 h=hashlib.sha256()
 with Path(p).open('rb')as f:
  for b in iter(lambda:f.read(1048576),b''):h.update(b)
 return h.hexdigest()
def encoded(v):return json.dumps(v,sort_keys=True,indent=2)+'\n'
def collect(m):
 result={}
 for bank in ('artifact_files','owned_live_files','protected_inputs','independently_reaudited_canister_and_physics_pins','external_runtime_dependencies'):
  for n,h in m.get(bank,{}).items():
   assert n not in result or result[n]==h,n
   result[n]=h
 return result
def main():
 assert not OUT.exists()
 assert sha(PRIOR)=='bc98f424d7a5254814ea38cdeb7e9bf635956b52208dcc3470ba084efd90051b'
 assert sha(CORRECTION)=='0fdd0f3ac9de29046d7056c80e2114c154cc800350429564f3e1a9e219d51a9a'
 protected=collect(json.loads(PRIOR.read_text()))
 for n,h in collect(json.loads(CORRECTION.read_text())).items():
  assert n not in protected or protected[n]==h,n
  protected[n]=h
 for p in(PRIOR,CORRECTION):protected[str(p.relative_to(ROOT))]=sha(p)
 for n,h in protected.items():assert sha(ROOT/n)==h,n
 a=SCOPE/'measured';b=SCOPE/'repeated'
 files=['report.json']+[str(p.relative_to(a))for p in sorted((a/'instructions').glob('*.asm'))]
 assert len(files)==66
 for n in files:assert(a/n).read_bytes()==(b/n).read_bytes(),n
 r=json.loads((a/'report.json').read_text());assert r['tool_sha256']==sha(ROOT/OWN[0]) and r['factory_correlated']
 assert len(r['stages'])==18 and sum(s['completed']for s in r['stages'])==17
 assert len(r['calls'])==68 and len(r['stores'])==19896
 assert all(not s['used_services']for s in r['stages'])
 assert len(r['platform_calls'])==len(r['platform_abis'])==len(r['platform_cleanup'])==4
 assert [(p['requested_size'],p['rounded_size'],p['guest_address'])for p in r['platform_calls']]==[(0x508c,0x6000,'7e420000'),(0x6a8,0x1000,'7e410000'),(0x30fc,0x4000,'7e400000'),(12,4096,'7e3f0000')]
 assert all(p['initial_full_hex']=='00'*p['rounded_size'] and not p['allocation_failures']for p in r['platform_calls'])
 assert all(q['actual_munmap_result']==0 for q in r['platform_cleanup'])
 assert len(r['applied_access_calls'])==1
 access=r['applied_access_calls'][0]
 assert (access['base'],access['size'],access['protection'])==('80061000',0x2ce6000,'00000404')
 assert access['actual_mem_protect_completed'] and access['contents_before_sha256']==access['contents_after_sha256']
 assert access['compiled_compatibility_outputs']==['0000006b','00000440','00000440','00000040']
 assert access['registers_before']==access['registers_after']
 c=r['stages'][1]['contexts_after']
 for g,p,n in [('004eda24','7e420000',0x508c),('004eda28','7e410000',0x6a8),('004eda34','7e400000',0x30fc)]:
  assert c[g]['pointer']==p and len(bytes.fromhex(c[g]['raw_hex']))==n
 assert int.from_bytes(bytes.fromhex(c['texture_globals70_hex'])[0x40:0x44],'little')==0x80061000
 page=next(q for q in r['calls']if q['entry']=='0013eae0');assert page['return_cpu']['eax']=='e2780000'
 request=next(q for q in r['calls']if q['entry']=='0019d8c0');assert request['registers']['ecx']=='890d4600' and request['stack_words'][:6]==['00000c00','80061000','00000000','7e42004f','00000003','00000002']
 invalid=r['stages'][-1]['invalid'];assert invalid['kernel_target']=='800000ea' and invalid['original_call_site']=='002dc8cf' and invalid['original_return_site']=='002dc8d5' and invalid['stack'][:4]==['00000000','00000001','00000000','00000000']
 for p in r['stages'][-1]['pools_after']:
  if p['global_address']in('00518464','00526514','00526518','0052651c'):assert not p['occupied']
 for n,h in r['protected_inputs'].items():assert sha(ROOT/n)==h,n
 OUT.mkdir()
 for n in OWN:
  target=OUT/'source'/n;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/n,target)
 logs=OUT/'logs';logs.mkdir()
 for n in('canister-texture-cache-measured.log','canister-texture-cache-repeated.log'):shutil.copy2(Path('/tmp')/n,logs/n)
 dependencies={str(p):sha(p)for p in [Path(sys.executable).resolve(),Path('/usr/lib/libc.so.6'),Path(unicorn.__file__).parent/'lib/libunicorn.so.2',Path(capstone.__file__).parent/'lib/libcapstone.so']}
 (OUT/'dependencies.json').write_text(encoded(dict(files=dependencies,python=sys.version,unicorn=unicorn.__version__,capstone=capstone.__version__,libc=platform.libc_ver(),observed_host=r['host'],compiled_protection_library='analysis/multiplayer/integration/spawn-source/local-map-protection/compiled-compatibility-access.so')))
 commands=dict(cwd=str(ROOT),environment={'PYTHONPATH':'tools'},commands=[f'PYTHONPATH=tools .tools/python/bin/python tools/oracle_canister_texture_cache_20261010.py --output {str((p/"report.json").relative_to(ROOT))}'for p in(a,b)],freeze_command='.tools/python/bin/python tools/freeze_canister_texture_cache_20261010.py',gpu_used=False,production_changed=False)
 (OUT/'commands.json').write_text(encoded(commands))
 summary=dict(roots=18,returns=17,stops=1,calls=68,ordered_nonstack_stores=19896,instruction_files=65,byte_identical_repeat_files=66,actual_native_allocations=4,actual_native_munmaps=4,actually_applied_protections=1,protected_bytes=0x2ce6000,particle_births=0,stop=invalid,final_report_sha256=sha(a/'report.json'))
 (OUT/'summary.json').write_text(encoded(summary))
 artifacts={str(p.relative_to(ROOT)):sha(p)for p in SCOPE.rglob('*')if p.is_file()}
 m=dict(schema='original-canister-texture-owner-platform-freeze-v1',artifact_files=artifacts,owned_live_files={n:sha(ROOT/n)for n in OWN},protected_inputs=r['protected_inputs'],independently_reaudited_vertex_and_prior_pins=protected,external_runtime_dependencies=dependencies,summary=summary,bounds=r['boundaries']+['Original async queue constructor and actual opened file-token/path owner are not executed here; first wait on NULL handle is retained without a supplied return.'])
 (OUT/'manifest.json').write_text(encoded(m))
 pins=artifacts|m['owned_live_files']|r['protected_inputs']|protected|dependencies
 bad=[n for n,h in pins.items()if sha(ROOT/n)!=h];assert not bad,bad
 (OUT/'audit.json').write_text(encoded(dict(unique_pins=len(pins),mismatches=bad,manifest_sha256=sha(OUT/'manifest.json'))))
 print(encoded(dict(manifest=str(OUT/'manifest.json'),sha256=sha(OUT/'manifest.json'),artifact_files=len(artifacts),unique_pins=len(pins),mismatches=0,summary=summary)))
if __name__=='__main__':main()
