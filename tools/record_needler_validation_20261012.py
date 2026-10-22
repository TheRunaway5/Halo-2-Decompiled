#!/usr/bin/env python3
"""Record the source closure, original fixtures and completed native checks."""
import hashlib,json,shutil,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
E=ROOT/'build-unit-object-20261011'
SOURCE=ROOT/'.local/needler-mcc-source-20261012'
BUILD=ROOT/'build-needler-mcc-20261012'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def record(p):return dict(path=str(p.resolve()),bytes=p.stat().st_size,sha256=sha(p))
def write(p,d):p.write_text(json.dumps(d,indent=2,sort_keys=True)+'\n')
def main():
 results=json.loads((E/'needler-mcc-final-20261012/results.json').read_text())
 assert len(results)==6 and all(r['exit_code']==0 and r['validation_errors']==0 for r in results)
 logs=['needler-focused-regression-20261012.log','needler-ammunition-movement-final-20261012.log',
       'needler-mcc-focused-20261012.log']
 for name in logs:assert '100% tests passed' in (E/name).read_text()
 assert 'PASS actual native UDP' in (E/'needler-mcc-network-20261012.log').read_text()
 for r in results:assert '\nPASS ' in Path(r['log']).read_text()
 owned=['include/halo2/content.hpp','include/halo2/fixed_effect_event.hpp','include/halo2/muzzle_effects.hpp',
        'include/halo2/effect_sound.hpp','src/assets/content.cpp','src/gameplay/simulation.cpp','src/rendering/effects/muzzle_effects.cpp',
        'src/rendering/effects/effect_sound.cpp','tests/muzzle_effects_tests.cpp','tests/needler_firing_tests.cpp',
        'tests/native_covenant_weapon_runtime_tests.cpp','tests/native_weapon_runtime_tests.cpp']
 equal_owned=[p for p in owned if p!='src/assets/content.cpp']
 assert all(sha(ROOT/p)==sha(SOURCE/p) for p in equal_owned)
 # Shared root independently optimized its tag lookup; compare only our exact
 # authored Needler import hunk, retaining the frozen parent implementation.
 def needler_import(text):
  return text.split('        if(multiplayer && w.weapon_type==2) {',1)[1].split('        if (multiplayer && w.name',1)[0]
 assert needler_import((ROOT/'src/assets/content.cpp').read_text())==needler_import((SOURCE/'src/assets/content.cpp').read_text())
 unrelated=subprocess.run(['diff','-u',str(SOURCE/'src/assets/content.cpp'),str(ROOT/'src/assets/content.cpp')],capture_output=True,text=True)
 (E/'needler-root-content-independent.patch').write_text(unrelated.stdout)
 # Only the explicitly owned runtime hunks were applied to the MCC parent.
 runtime_patch=subprocess.run(['diff','-u',str(ROOT/'analysis/multiplayer/mcc-hud/verified-source/src/multiplayer_runtime.cpp'),
                               str(SOURCE/'src/gameplay/multiplayer/multiplayer_runtime.cpp')],capture_output=True,text=True)
 assert runtime_patch.returncode==1 and 'weather_layers' not in runtime_patch.stdout
 (E/'needler-mcc-runtime-owned.patch').write_text(runtime_patch.stdout)
 for name in ('record_needler_validation_20261012.py','validate_needler_runtime_20261012.py'):
  shutil.copy2(ROOT/'tools'/name,SOURCE/'tools'/name)
 files={}
 for name in ('include','src','shaders','cmake','tests','tools','resources'):
  for p in sorted((SOURCE/name).rglob('*')):
   if p.is_file() and not p.is_symlink() and '__pycache__' not in p.parts:
    files[str(p.relative_to(SOURCE))]=sha(p)
 files['CMakeLists.txt']=sha(SOURCE/'CMakeLists.txt')
 files['docs/MP_COVENANT_WEAPON_FIXES.md']=sha(SOURCE/'docs/MP_COVENANT_WEAPON_FIXES.md')
 shared={}
 for p in sorted((SOURCE/'external/imgui').rglob('*')):
  if p.is_file() and p.suffix in ('.cpp','.h','.inl'):
   shared[str(p.relative_to(SOURCE))]=sha(p)
 manifest=E/'needler-mcc-source-manifest-20261012.json'
 write(manifest,dict(source=str(SOURCE),parent=str(ROOT/'analysis/multiplayer/mcc-hud/verified-source'),
    files=files,shared_external_source=shared,owned_equal_to_root=equal_owned,
    owned_content_import_equal=True,independent_root_content_delta=record(E/'needler-root-content-independent.patch'),
    runtime_owned_patch=record(E/'needler-mcc-runtime-owned.patch')))
 assert all(sha(SOURCE/p)==v for p,v in files.items())
 assert all(sha(SOURCE/p)==v for p,v in shared.items())
 # Preserve the preceding manifest and record its successor separately.
 old_source=ROOT/'.local/unit-object-covenant-source-20261011'
 old_manifest=old_source/'source-manifest-final-20261012.json'
 previous=json.loads(old_manifest.read_text())
 current={p:sha(old_source/p) for p in previous}
 for p in ('tests/needler_firing_tests.cpp','tests/fixtures/needler_firing_effects_original.tsv',
           'tests/fixtures/weapon_fixed_events_original.tsv','tools/oracle_needler_firing_effects_1_1.py',
           'tools/oracle_weapon_fixed_events_1_1.py'):
  current[p]=sha(old_source/p)
 successor=E/'needler-covenant-source-manifest-20261012.json'
 write(successor,dict(parent_manifest=record(old_manifest),source=str(old_source),files=current))
 launchers={}
 for name in ('Halo2-Launcher','Halo2-Title-Launcher'):
  info=subprocess.run([str(ROOT/name),'--launcher-info'],check=True,capture_output=True,text=True).stdout
  assert 'Runtime: '+str(BUILD/'halo2_native') in info
  log=E/('needler-delivered-'+name+'-20261012.log');log.write_text(info)
  launchers[name]=dict(binary=record(ROOT/name),info=record(log))
 originals=[ROOT/'analysis/multiplayer/integration'/name for name in
    ('weapon-fixed-events-20261012.json','needler-firing-effects-original-20261012.json')]
 for p,tool,key in zip(originals,('oracle_weapon_fixed_events_1_1.py','oracle_needler_firing_effects_1_1.py'),('tool_sha256','oracle_sha256')):
  assert json.loads(p.read_text())[key]==sha(ROOT/'tools'/tool)
 result=dict(scope='Normal dry local Needler firing; original instruction fragments plus native integration. Full original game parity is not claimed.',
  source_manifest=record(manifest),covenant_successor_manifest=record(successor),
  runtime=record(BUILD/'halo2_native'),launchers=launchers,
  live_gameplay_xbe=record(Path(next(p for p in results[0]['command']if p.endswith('/default.xbe')))),
  original_reports=[record(p)for p in originals],
  focused_logs=[record(E/name)for name in logs+['needler-mcc-network-20261012.log']],
  live=[dict(**r,evidence=record(Path(r['log'])))for r in results],
  rejected_probe=record(E/'needler-firing-capture-20261012.log'),
  smg_duplicate_sound_red=record(E/'smg-duplicate-sound-red-20261012.log'),
  assets=[record(ROOT/p)for p in ('assets/disc/maps/lockout.map','assets/disc/maps/shared.map',
    'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03','assets/hud-source.tsv','assets/mcc-classic-hud/manifest.json')],
  limits=['Dummy SDL accepted PCM is not physical audible output or original audio mixer identity.',
    'No original Xbox gameplay framebuffer or physical GPU comparison.',
    'Full Unit/pose/Havok, original allocation and attached/world effect ownership remain open.',
    'Fluid/media transitions, multi-user camera/world placement and render-independent emission remain open.',
    'Needler homing, attachment, contrail, impact/supercombine/detonation remain open.',
    'Native LAN retains one effect event per player; offhand and coalesced event histories are incomplete.'])
 target=E/'needler-firing-validation-20261012.json';write(target,result)
 print(json.dumps(dict(report=str(target),sha256=sha(target),source_files=len(files),shared_sources=len(shared),live_cases=len(results))))
if __name__=='__main__':main()
