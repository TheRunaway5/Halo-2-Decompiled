#!/usr/bin/env python3
"""Freeze observed live-grenade sources/evidence without pinning mutable shared files."""
import hashlib,json,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
SCOPE=ROOT/'analysis/multiplayer/integration/live-grenade-20261010'
OUT=SCOPE/'frozen'
def digest(p):
 h=hashlib.sha256()
 with p.open('rb')as f:
  for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
 return h.hexdigest()
def encoded(x):return json.dumps(x,sort_keys=True,indent=2)+'\n'
def main():
 assert not OUT.exists(),OUT
 own=['include/halo2/native_grenade.hpp','include/halo2/native_grenade_constants.hpp','src/native_grenade.cpp',
 'tests/multiplayer_grenade_delivery_tests.cpp','tests/native_grenade_timer_tests.cpp',
 'tools/probe_live_grenade_release_20261010.py','tools/probe_native_grenade_timer_20261010.py',
 'tools/freeze_native_grenade_20261010.py','docs/MP_NATIVE_GRENADE_SANDBOX_RESEARCH.md']
 shared=['CMakeLists.txt','include/halo2/{simulation,multiplayer,multiplayer_network,multiplayer_kbm,original_data,collision,model_markers,unit_grenade,prop_sandbox}.hpp',
 'src/{simulation,multiplayer,multiplayer_network,multiplayer_kbm,original_data,collision,model_markers,unit_grenade,prop_sandbox,prop_sandbox_definition}.cpp']
 expanded=[]
 for entry in shared:
  if '{' in entry:
   prefix,tail=entry.split('{');names,suffix=tail.split('}');expanded.extend(prefix+n+suffix for n in names.split(','))
  else:expanded.append(entry)
 observations={}
 for name in own+expanded:
  p=ROOT/name;b=p.read_bytes();target=OUT/'source'/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(b)
  observations[name]=dict(sha256=hashlib.sha256(b).hexdigest(),role='new_scope'if name in own else'individually_observed_shared_snapshot')
 logs=sorted(Path('/tmp').glob('live-grenade-*.log'))+sorted(Path('/tmp').glob('live-grenade-*.stderr'))
 for p in logs:
  target=OUT/'logs'/p.name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(p,target)
 for kind,directory in [('normal','build-animation-metadata-store-20261008'),('sanitizer','build-animation-event-asan-20261008')]:
  target=OUT/kind;target.mkdir(parents=True)
  for name in ['multiplayer_grenade_delivery_tests','native_grenade_timer_tests','libhalo2_systems.a','libhalo2_cache.a','libhalo2_native_synchronization.a','libhalo2_sound_platform.a','CMakeCache.txt']:
   shutil.copy2(ROOT/directory/name,target/name)
  for name in ['multiplayer_grenade_delivery_tests','native_grenade_timer_tests']:
   for leaf in ['link.txt','flags.make']:
    source=ROOT/directory/'CMakeFiles'/(name+'.dir')/leaf
    if source.exists():shutil.copy2(source,target/(name+'.'+leaf))
 commands={
 'cwd':str(ROOT),'original_release_probe':'PYTHONPATH=tools .tools/python/bin/python tools/probe_live_grenade_release_20261010.py',
 'original_timer':[f'PYTHONPATH=tools .tools/python/bin/python tools/probe_native_grenade_timer_20261010.py --output analysis/multiplayer/integration/live-grenade-20261010/{d}'for d in ['timer-measured','timer-repeat']],
 'normal_configure':'cmake -S . -B build-animation-metadata-store-20261008',
 'normal_build':'cmake --build build-animation-metadata-store-20261008 --target multiplayer_grenade_delivery_tests native_grenade_timer_tests -j4',
 'sanitizer_build':'cmake --build build-animation-event-asan-20261008 --target multiplayer_grenade_delivery_tests native_grenade_timer_tests -j4',
 'live_runs':[f'{d}/multiplayer_grenade_delivery_tests assets/disc/maps/lockout.map assets/disc/maps assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'for d in ['build-animation-metadata-store-20261008','build-animation-event-asan-20261008']],
 'timer_runs':[f'{d}/native_grenade_timer_tests analysis/multiplayer/integration/live-grenade-20261010/timer-measured/fixtures.bin'for d in ['build-animation-metadata-store-20261008','build-animation-event-asan-20261008']],
 'boundary':'No GPU here. SDL dummy event queue; actual Lockout and actual loopback UDP. Native animation residency/terrain/capsule adapters. Archives observed after linked tests; uncalled members not audited.'}
 (OUT/'commands.json').write_text(encoded(commands))
 prior=['analysis/multiplayer/integration/grenade-action/manifest.json','analysis/multiplayer/integration/grenade-bounded-phases/manifest.json',
 'analysis/multiplayer/integration/grenade-owned-release/manifest.json','analysis/multiplayer/integration/grenade-sound-pool/manifest.json',
 'analysis/multiplayer/integration/sound-looping-native/final-manifest.json']
 prior_pins={name:digest(ROOT/name)for name in prior}
 assets=['assets/disc/maps/lockout.map','assets/disc/maps/shared.map','assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03',
 '/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe']
 external={name:digest(ROOT/name)for name in assets}
 for leaf in ['report.json','fixtures.bin','tu11.asm','u5.asm']:
  assert (SCOPE/'timer-measured'/leaf).read_bytes()==(SCOPE/'timer-repeat'/leaf).read_bytes(),leaf
 checks={}
 for kind,log in [('normal_live','live-grenade-delivery-final-normal-test.log'),('sanitizer_live','live-grenade-delivery-sanitizer-test-2.log'),
 ('normal_timer','live-grenade-timer-normal-test.log'),('sanitizer_timer','live-grenade-timer-sanitizer-test.log')]:
  text=(OUT/'logs'/log).read_text();assert ('PASS 454'if 'live'in kind else'PASS 540')in text
  assert 'ERROR: AddressSanitizer'not in text and'runtime error:'not in text
  checks[kind]=dict(log=log,passed=True)
 artifact_files={str(p.relative_to(ROOT)):digest(p)for p in SCOPE.rglob('*')if p.is_file()}
 manifest=dict(schema='native-grenade-sandbox-freeze-v1',artifact_files=artifact_files,observed_live_sources=observations,
 prior_manifest_pins=prior_pins,external_assets=external,checks=checks,
 bounds=['New native adapter scope; not whole raw Unit/constructor/Havok/platform/effect playback parity.',
 'Runtime GPU/window/render/audio integration is root-owned and absent from this CPU freeze.',
 'Shared source files are individually observed copies. Linked archive members are not asserted all freshly rebuilt/audited.',
 'Prior manifest digests retained; no claim this new freeze recursively reaudits every earlier protected artifact.'])
 (OUT/'manifest.json').write_text(encoded(manifest))
 mismatches=[p for p,h in artifact_files.items()if digest(ROOT/p)!=h]
 assert not mismatches,mismatches
 (OUT/'audit.json').write_text(encoded(dict(artifact_count=len(artifact_files),prior_manifest_count=len(prior_pins),mismatches=mismatches,
 manifest_sha256=digest(OUT/'manifest.json'),timer_repeat_byte_identical=True)))
 print(encoded(dict(manifest=str(OUT/'manifest.json'),sha256=digest(OUT/'manifest.json'),artifacts=len(artifact_files),mismatches=0)))
if __name__=='__main__':main()
