#!/usr/bin/env python3
"""Freeze only the new physics0 evidence, observed build inputs and own files."""
import hashlib,json,shutil,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
SCOPE=ROOT/'analysis/multiplayer/integration/particle-physics-movement-20261010'
OUT=SCOPE/'frozen'
OWN=['include/halo2/particle_movement.hpp','src/rendering/effects/particle_movement.cpp',
     'tests/particle_movement_tests.cpp','tools/oracle_particle_physics_movement_20261010.py',
     'tools/freeze_particle_physics_movement_20261010.py','docs/MP_PARTICLE_PHYSICS_MOVEMENT_RESEARCH.md']
def sha(p):
 h=hashlib.sha256()
 with p.open('rb')as f:
  for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
 return h.hexdigest()
def encoded(v):return json.dumps(v,sort_keys=True,indent=2)+'\n'
def main():
 assert not OUT.exists()
 measured=SCOPE/'measured-v2';repeated=SCOPE/'repeated-v2'
 assert sorted(p.name for p in measured.iterdir())==sorted(p.name for p in repeated.iterdir())
 for p in measured.iterdir():assert p.read_bytes()==(repeated/p.name).read_bytes(),p
 report=json.loads((measured/'report.json').read_text())
 assert len(report['images'])==2 and all(len(i['cases'])==1080 for i in report['images'])
 inputs=report['input_pins']|{
  'assets/disc/maps/lockout.map':'14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9',
  'assets/disc/default.xbe':'03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d',
  'analysis/multiplayer/integration/live-grenade-20261010/frozen/manifest.json':'4e60d9bf5f9dba3564cf2694413a6632f7ba58d76112aff98fa72fe1d319acfd'}
 for p,h in inputs.items():assert sha(ROOT/p)==h,p
 dependencies=subprocess.check_output(['g++','-std=c++20','-MM','-Iinclude', '-Isrc','src/rendering/effects/particle_movement.cpp','tests/particle_movement_tests.cpp'],cwd=ROOT,text=True)
 headers={p for p in dependencies.replace('\\\n',' ').split()if p.startswith('include/')}
 observations={}
 for name in sorted(headers|set(OWN)|{'CMakeLists.txt'}):
  p=ROOT/name;raw=p.read_bytes();target=OUT/'source'/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(raw)
  assert p.read_bytes()==raw,name
  observations[name]=dict(sha256=hashlib.sha256(raw).hexdigest(),role='new_scope'if name in OWN else'individually_observed_dependency')
 logdir=OUT/'logs';logdir.mkdir()
 for p in sorted(Path('/tmp').glob('particle-physics-movement-*.log')):shutil.copy2(p,logdir/p.name)
 checks={}
 for name,token in [('native-test-3','PASS 1080'),('sanitizer-frozen-test','PASS 1080'),('production-test','100% tests passed')]:
  p=logdir/('particle-physics-movement-'+name+'.log');text=p.read_text()
  assert token in text and'ERROR: AddressSanitizer'not in text and'runtime error:'not in text
  checks[name]=dict(log=str(p.relative_to(ROOT)),passed=True)
 commands={
  'cwd':str(ROOT),'original_commands':[f'PYTHONPATH=tools .tools/python/bin/python tools/oracle_particle_physics_movement_20261010.py --output analysis/multiplayer/integration/particle-physics-movement-20261010/{d}'for d in('measured-v2','repeated-v2')],
  'native_run':'particle_movement_tests assets/disc/maps/lockout.map analysis/multiplayer/integration/particle-physics-movement-20261010/measured-v2/fixtures.bin analysis/multiplayer/integration/particle-physics-movement-20261010/measured-v2/context.bin analysis/multiplayer/integration/particle-physics-movement-20261010/measured-v2/stores.bin',
  'normal_build':'g++ -std=c++20 -O2 -ffp-contract=off -Iinclude tests/particle_movement_tests.cpp src/rendering/effects/particle_movement.cpp build-animation-metadata-store-20261008/libhalo2_cache.a -pthread -o particle_movement_tests',
  'sanitizer_build':'g++ -std=c++20 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -ffp-contract=off -Iinclude tests/particle_movement_tests.cpp src/rendering/effects/particle_movement.cpp analysis/multiplayer/integration/particle-physics-movement-20261010/validated/sanitizer/libhalo2_cache.a -pthread -o particle_movement_tests',
  'production_configure':'cmake -S . -B build-animation-metadata-store-20261008',
  'production_build':'cmake --build build-animation-metadata-store-20261008 --target particle_movement_tests -j4',
  'production_test':"ctest --test-dir build-animation-metadata-store-20261008 -R '^original_particle_physics_movement$' --output-on-failure",
  'sanitizer_environment':{'ASAN_OPTIONS':'detect_leaks=1','UBSAN_OPTIONS':'print_stacktrace=1'},
  'compiler':subprocess.check_output(['g++','--version'],text=True),
  'scope':'CPU only. Source-created SMG records + actual canister controller metadata; no full canister birth, collision, platform FP-status or GPU claim.'}
 (OUT/'commands.json').write_text(encoded(commands))
 artifacts={str(p.relative_to(ROOT)):sha(p)for p in SCOPE.rglob('*')if p.is_file()}
 manifest=dict(schema='particle-physics0-freeze-v1',artifact_files=artifacts,
  owned_live_files={p:sha(ROOT/p)for p in OWN},observed_sources=observations,protected_inputs=inputs,
  checks=checks,original_repeat_byte_identical=True,
  bounds=['Shared CMake/headers are individually observed copies, not a whole-repo snapshot.',
          'Linked Cache archives are retained observed dependencies; uncalled members are not claimed independently rebuilt/audited.',
          'Earlier trial reports and metadata-store rejection diagnostics are preserved; final assertions use measured-v2 and the named passing logs.',
          'Prior grenade manifest digest retained; no recursive reaudit claim for unrelated older checkpoints.'])
 (OUT/'manifest.json').write_text(encoded(manifest))
 mismatch=[p for p,h in artifacts.items()if sha(ROOT/p)!=h]
 mismatch +=[p for p,h in manifest['owned_live_files'].items()if sha(ROOT/p)!=h]
 assert not mismatch,mismatch
 (OUT/'audit.json').write_text(encoded(dict(artifacts=len(artifacts),owned_files=len(OWN),protected_inputs=len(inputs),mismatches=mismatch,manifest_sha256=sha(OUT/'manifest.json'))))
 print(encoded(dict(manifest=str(OUT/'manifest.json'),sha256=sha(OUT/'manifest.json'),artifacts=len(artifacts),mismatches=0)))
if __name__=='__main__':main()
