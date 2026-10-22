#!/usr/bin/env python3
"""Capture exact released-scene CPU evidence without modifying previous freezes."""
import hashlib,json,shutil,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/multiplayer/integration/grenade-scene-20261010'
def digest(p):
 with p.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def main():
 final=BASE/'frozen';stage=BASE/'frozen-preparing';assert not final.exists() and not stage.exists();stage.mkdir()
 observed=[]
 def copy(source,relative):
  source=Path(source);before=digest(source);target=stage/relative;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,target);assert digest(target)==before and digest(source)==before;observed.append(dict(path=str(source),sha256=before,size=source.stat().st_size))
 for path in sorted((ROOT/'include').rglob('*')):
  if path.is_file():copy(path,Path('source')/path.relative_to(ROOT))
 sourcefiles=['src/gameplay/weapons/source_grenade_scene.cpp','src/gameplay/objects/source_object_geometry.cpp','src/gameplay/objects/source_object_geometry.hpp','tests/source_grenade_scene_tests.cpp','docs/MP_GRENADE_SCENE_RESEARCH.md','CMakeLists.txt']
 sourcefiles += ['tools/'+x for x in ('oracle_grenade_scene_state.py','extract_grenade_scene_constructor.py','pair_grenade_scene_attach_flags.py','export_grenade_scene_fixtures.py','build_grenade_scene_validation.py','probe_source_grenade_scene_maps.py','audit_grenade_scene.py','freeze_grenade_scene.py','research_mp_original_data_u5.py','research_mp_live_locomotion_owners_u5.py','inspect_multiplayer.py','inventory_mp_weapon_effects.py','reference/halo2-decompiled/tools/xbe.py','reference/halo2-decompiled/tools/functions.py')]
 for relative in sourcefiles:copy(ROOT/relative,Path('source')/relative)
 evidence=('measured-final','measured-repeat','constructor','constructor-repeat','attach-flags','attach-flags-repeat','fixtures','maps-production-normal-owned','maps-production-normal-owned-repeat','maps-production-sanitizer','maps-production-sanitizer-repeat')
 for directory in evidence:
  label={'maps-production-normal-owned':'maps-production-normal','maps-production-normal-owned-repeat':'maps-production-normal-repeat'}.get(directory,directory)
  for path in sorted((BASE/directory).rglob('*')):
   if path.is_file():copy(path,Path('evidence')/label/path.relative_to(BASE/directory))
 for directory in ('maps-final-normal','maps-final-sanitizer'):
  for path in sorted((BASE/directory).rglob('*')):
   if path.is_file():copy(path,Path('exploratory-abi-mismatch')/directory/path.relative_to(BASE/directory))
 copy(BASE/'native-owned-normal/source_grenade_scene_tests','normal/source_grenade_scene_tests')
 for path in (BASE/'native-owned-normal').iterdir():
  if path.is_file() and path.name!='source_grenade_scene_tests':copy(path,Path('normal')/path.name)
 normal=json.loads((BASE/'native-owned-normal/build.json').read_text())
 for path in normal['observed_archive_sha256']:
  assert digest(Path(path))==normal['observed_archive_sha256'][path];copy(Path(path),Path('normal')/Path(path).name)
 copy(BASE/'normal-owned-bank/provenance.json','normal/owned-bank-provenance.json')
 for path in sorted((BASE/'normal-owned-bank/include').rglob('*')):
  if path.is_file():copy(path,Path('normal/include')/path.relative_to(BASE/'normal-owned-bank/include'))
 san=ROOT/'build-animation-event-asan-20261008'
 copy(san/'source_grenade_scene_tests','sanitizer/source_grenade_scene_tests')
 for path in sorted(san.glob('libhalo2_*.a')):copy(path,Path('sanitizer')/path.name)
 for name in ('CMakeCache.txt','CTestTestfile.cmake'):copy(san/name,Path('sanitizer')/name)
 for name in ('halo2_cache','halo2_systems','halo2_renderer','halo2_shadow_render','halo2_menu_world_storage','halo2_particle_renderer','halo2_sound_platform','halo2_native_synchronization','source_grenade_scene_tests'):
  source=san/'CMakeFiles'/(name+'.dir')
  for filename in ('flags.make','link.txt','build.make','DependInfo.cmake','compiler_depend.make','compiler_depend.ts'):
   path=source/filename
   if path.is_file():copy(path,Path('sanitizer/build')/name/filename)
 for path in (san/'CMakeFiles/source_grenade_scene_tests.dir/tests').glob('*'):
  if path.is_file():copy(path,Path('sanitizer/build/source_grenade_scene_tests/tests')/path.name)
 for path in (san/'CMakeFiles/halo2_renderer.dir/src').glob('source_grenade_scene.cpp.*'):
  if path.is_file():copy(path,Path('sanitizer/build/halo2_renderer/src')/path.name)
 prior=[]
 for path in (ROOT/'analysis/multiplayer/integration/live-authored-audio-20261010/frozen/manifest.json',ROOT/'analysis/multiplayer/integration/live-grenade-20261010/frozen/manifest.json'):
  r=json.loads(path.read_text());prior.append(dict(path=str(path),sha256=digest(path)))
  for row in r.get('artifacts',[]):prior.append(dict(path=str(path.parent/row['path']),sha256=row['sha256']))
  for relative,pin in r.get('artifact_files',{}).items():prior.append(dict(path=str(ROOT/relative),sha256=pin))
 source=json.loads((BASE/'measured-final/report.json').read_text());prior.append(source['prior'])
 maps=json.loads((BASE/'maps-production-normal-owned/report.json').read_text())
 owned=[Path(c['map']) for c in maps['cases']]+[ROOT/'assets/disc/maps/shared.map']+[Path(i['path']) for i in source['images']]
 inputs=[dict(path=str(p),size=p.stat().st_size,sha256=digest(p)) for p in owned]
 assert all(digest(Path(row['path']))==row['sha256'] for row in prior)
 assert all(digest(Path(row['path']))==row['sha256'] for row in observed),'source/build input changed during snapshot'
 artifacts=[dict(path=str(p.relative_to(stage)),size=p.stat().st_size,sha256=digest(p)) for p in sorted(stage.rglob('*')) if p.is_file()]
 manifest=dict(schema='source-grenade-scene-freeze-v1',scope='Verified ordinary released CPU geometry/variant/node/bounds/LOD/light-bank consumer. Held visibility, original world registration and released angular physics are unavailable.',artifacts=artifacts,prior_pins=prior,owned_inputs=inputs,observed_sources=observed,provenance='Normal comparator compiles scene/test/geometry against observed consistent root release archives. Sanitizer is the complete focused CMake target with actual sanitizer flags in Cache, Systems, Renderer and test. Archives/headers are exact observed artifacts; no complete fresh-tree provenance is inferred. Previous failed old-Cache/new-LevelContent ABI attempt is separately retained and excluded from passing evidence.',commands=dict(normal_compile=normal['command'],sanitizer_build=['cmake','--build','build-animation-event-asan-20261008','--target','source_grenade_scene_tests','-j2'],sanitizer_environment=dict(ASAN_OPTIONS='detect_leaks=1',UBSAN_OPTIONS='halt_on_error=1'),map_commands='Literal per-map command/environment/stdout/return code in evidence/maps-production-*/report.json'),expected=dict(original_cases=8,complete_body_pairs=16,complete_bound_returns=4,world_unregister_stops=4,maps=23,owner_view_comparisons_per_map=20,material_banks=2,resident_materials=8,texture_bytes=1366304))
 (stage/'manifest.json').write_text(json.dumps(manifest,sort_keys=True,indent=2)+'\n');stage.rename(final)
 print(final/'manifest.json',digest(final/'manifest.json'),len(artifacts),len(prior),len(inputs))
if __name__=='__main__':main()
