#!/usr/bin/env python3
"""Freeze actual original vertex-cache construction and its named OS backing."""
import hashlib,json,platform,shutil,sys
from pathlib import Path
import capstone,unicorn
ROOT=Path(__file__).resolve().parents[1]
SCOPE=ROOT/'analysis/multiplayer/integration/canister-vertex-cache-20261010'
OUT=SCOPE/'frozen'
OWN=['tools/oracle_canister_vertex_cache_20261010.py','tools/freeze_canister_vertex_cache_20261010.py','docs/MP_CANISTER_VERTEX_CACHE_RESEARCH.md']
PRIOR=ROOT/'analysis/multiplayer/integration/canister-particle-owner-20261010/frozen/manifest.json'
def sha(p):
 h=hashlib.sha256()
 with Path(p).open('rb')as f:
  for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
 return h.hexdigest()
def encoded(v):return json.dumps(v,sort_keys=True,indent=2)+'\n'
def collect_pins(m):
 result={}
 for bank in ('artifact_files','owned_live_files','protected_inputs','independently_reaudited_physics0_pins'):
  for n,h in m.get(bank,{}).items():
   assert n not in result or result[n]==h,n
   result[n]=h
 return result
def main():
 assert not OUT.exists()
 assert sha(PRIOR)=='f96219d626e8f45824253973e836131aa833a9a963fbc524baffc8ab872f8539'
 prior_pins=collect_pins(json.loads(PRIOR.read_text()))
 for n,h in prior_pins.items():assert sha(ROOT/n)==h,n
 a=SCOPE/'final-measured';b=SCOPE/'final-repeated'
 files=['report.json']+[str(p.relative_to(a))for p in sorted((a/'instructions').glob('*.asm'))]
 assert len(files)==55
 for n in files:assert(a/n).read_bytes()==(b/n).read_bytes(),n
 r=json.loads((a/'report.json').read_text())
 assert r['factory_correlated'] and r['tool_sha256']==sha(ROOT/OWN[0])
 assert len(r['stages'])==16 and sum(s['completed']for s in r['stages'])==15
 assert len(r['calls'])==49 and len(r['stores'])==18335
 assert all(not s['used_services']for s in r['stages'])
 assert len(r['platform_calls'])==len(r['platform_abis'])==len(r['platform_cleanup'])==1
 p=r['platform_calls'][0];abi=r['platform_abis'][0]
 assert (p['requested_size'],p['rounded_size'],p['allocation_type'],p['protection'])==(12,4096,'00101000','00000004')
 assert p['actual_mmap_address']==p['guest_address']=='7e420000'
 assert p['initial_full_hex']=='00'*4096 and not p['allocation_failures']
 assert abi['ordinal']==184 and abi['caller']=='00337875'
 assert (abi['base_after'],abi['size_after'])==('7e420000',4096)
 assert r['platform_cleanup'][0]['actual_munmap_result']==0
 s=r['stages'][4];c=s['contexts_after']
 assert s['name']=='source_complete_decal_constructor' and s['completed']
 assert c['00510c38']['pointer']=='82f0d444' and len(bytes.fromhex(c['00510c38']['raw_hex']))==0x613c
 assert c['004f1f20']['pointer']=='82ef974c' and len(bytes.fromhex(c['004f1f20']['raw_hex']))==0x100cc
 assert c['source_vertex_lookup1000_hex']=='ff'*0x1000
 assert c['source_vertex_tail']['unmasked_source_address']=='83131000'
 assert c['source_vertex_tail']['stored_gpu_offset']=='03131000'
 assert len(bytes.fromhex(c['source_vertex_tail']['raw_hex']))==0x14000
 assert s['platform_after'][0]['raw_hex']=='010000000010130300000000'+'00'*(4096-12)
 reset=r['stages'][5]['contexts_after']['004f1f1c'];raw=bytes.fromhex(reset['raw_hex'])
 assert raw[:0x3800]==b'\xff'*0x3800 and raw[0x3800:0x380c]==b'\xff'*4+b'\0'*8
 stop=r['stages'][-1];assert stop['stopped_eip']=='0013eafa' and stop['invalid']['address']=='00000034'
 request=next(q for q in r['calls']if q['entry']=='0013eae0')
 assert request['caller']=='0012ed48' and request['stack_words'][:3]==['00000000','00000b00','00000005']
 for pool in stop['pools_after']:
  if pool['global_address']in ('00518464','00526514','00526518','0052651c'):assert not pool['occupied']
 inputs=r['protected_inputs']|{str(PRIOR.relative_to(ROOT)):sha(PRIOR)}
 for n,h in inputs.items():assert sha(ROOT/n)==h,n
 OUT.mkdir()
 for name in OWN:
  target=OUT/'source'/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/name,target)
 logs=OUT/'logs';logs.mkdir()
 for n in ('canister-vertex-cache-measured.log','canister-vertex-cache-final-measured.log','canister-vertex-cache-final-repeated.log'):
  shutil.copy2(Path('/tmp')/n,logs/n)
 dependencies={str(p):sha(p)for p in [Path(sys.executable).resolve(),Path('/usr/lib/libc.so.6'),Path(unicorn.__file__).parent/'lib/libunicorn.so.2',Path(capstone.__file__).parent/'lib/libcapstone.so']}
 (OUT/'dependencies.json').write_text(encoded(dict(files=dependencies,python=sys.version,unicorn=unicorn.__version__,capstone=capstone.__version__,libc=platform.libc_ver(),observed_host=r['host'],scope='Real native Linux allocation is a named platform adaptation; Xbox VM/PTE/physical-memory/TLS behavior excluded.')))
 commands=dict(cwd=str(ROOT),environment={'PYTHONPATH':'tools'},commands=[f'PYTHONPATH=tools .tools/python/bin/python tools/oracle_canister_vertex_cache_20261010.py --output {str((p/"report.json").relative_to(ROOT))}'for p in(a,b)],freeze_command='.tools/python/bin/python tools/freeze_canister_vertex_cache_20261010.py',gpu_used=False,production_changed=False)
 (OUT/'commands.json').write_text(encoded(commands))
 summary=dict(roots=16,returns=15,stops=1,calls=49,ordered_nonstack_stores=18335,instruction_files=54,byte_identical_repeat_files=55,actual_native_allocations=1,actual_native_munmaps=1,raw_page_bytes=4096,particle_births=0,stop=dict(instruction='0013eafa',address='00000034',missing_owner='004eda34',caller='0012ed48',arguments=['00000000','00000b00','00000005']),final_report_sha256=sha(a/'report.json'))
 (OUT/'summary.json').write_text(encoded(summary))
 artifacts={str(p.relative_to(ROOT)):sha(p)for p in SCOPE.rglob('*')if p.is_file()}
 manifest=dict(schema='original-canister-vertex-cache-native-platform-freeze-v1',artifact_files=artifacts,owned_live_files={n:sha(ROOT/n)for n in OWN},protected_inputs=inputs,independently_reaudited_canister_and_physics_pins=prior_pins,external_runtime_dependencies=dependencies,summary=summary,bounds=r['boundaries']+['Whole17EC90/17ED50/43880 execute with only the original NT184 raw platform allocation adapted; no complete Xbox VM or native full-effect result claimed.','Next genuine texture-cache stop retains required decal; no particle/class8/collider/audio/rendering component is skipped.'])
 (OUT/'manifest.json').write_text(encoded(manifest))
 pins=artifacts|manifest['owned_live_files']|inputs|prior_pins|dependencies
 mismatch=[n for n,h in pins.items()if sha(ROOT/n)!=h];assert not mismatch,mismatch
 (OUT/'audit.json').write_text(encoded(dict(unique_pins=len(pins),mismatches=mismatch,manifest_sha256=sha(OUT/'manifest.json'))))
 print(encoded(dict(manifest=str(OUT/'manifest.json'),sha256=sha(OUT/'manifest.json'),artifact_files=len(artifacts),unique_pins=len(pins),mismatches=0,summary=summary)))
if __name__=='__main__':main()
