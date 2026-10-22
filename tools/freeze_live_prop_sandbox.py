#!/usr/bin/env python3
"""Freeze the measured prop integration and literal validation artifacts."""
import hashlib,json,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/multiplayer/sandbox-source/live-prop'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def write(p,b):
 p.parent.mkdir(parents=True,exist_ok=True)
 if p.exists():assert p.read_bytes()==b,p
 else:p.write_bytes(b)
def main():
 measured=OUT/'death-measured';repeat=OUT/'death-repeat'
 files=sorted(p.relative_to(measured)for p in measured.rglob('*')if p.is_file())
 assert files==sorted(p.relative_to(repeat)for p in repeat.rglob('*')if p.is_file())
 for p in files:assert (measured/p).read_bytes()==(repeat/p).read_bytes(),p
 report=json.loads((measured/'report.json').read_text())
 assert sha(measured/'report.json')=='cc2475ab1311310587fa83f358c791d5b036b2e5e9bbafe20beac583dfa42d19'
 assert sha(ROOT/'assets/disc/maps/lockout.map')==report['cache_sha256']
 assert sha(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')==report['source_sha256']
 for p,h in report['prior_pins'].items():assert sha(ROOT/p)==h,p
 for r in report['instructions']:assert sha(measured/r['file'])==r['sha256'],r
 fixture=json.loads((OUT/'death-fixture.json').read_text());assert sha(OUT/'death-fixture.bin')==fixture['fixture_sha256']
 logs=['live-prop-build-17.log','live-prop-test-7.log','live-prop-sanitizer-build-6.log',
  'live-prop-sanitizer-test-3.log','live-prop-all-maps-3.log','live-prop-u5-test-2.log']
 for name in logs:
  raw=(ROOT/'analysis/multiplayer/integration'/name).read_text()
  if 'build' in name:assert 'Built target prop_sandbox_tests' in raw
  elif 'all-maps' in name:assert '21 real maps imported/constructed/stepped' in raw
  else:assert 'Match fire/blast/removal in 57 ticks' in raw
  assert 'ERROR: AddressSanitizer' not in raw and 'runtime error:' not in raw,name
 paths=['include/halo2/prop_sandbox.hpp','src/gameplay/objects/prop_sandbox.cpp','src/gameplay/objects/prop_sandbox_definition.cpp',
  'tests/prop_sandbox_tests.cpp','src/rendering/effects/particle_material.cpp','src/ui/menu_noise_texture.cpp',
  'include/halo2/texture.hpp','include/halo2/content.hpp','src/assets/content.cpp',
  'include/halo2/simulation.hpp','src/gameplay/simulation.cpp','include/halo2/multiplayer.hpp','src/gameplay/multiplayer/multiplayer.cpp',
  'src/network/multiplayer_network.cpp','src/gameplay/multiplayer/multiplayer_runtime.cpp','include/halo2/collision.hpp','src/physics/collision/collision.cpp',
  'include/halo2/unit_damage.hpp','src/gameplay/units/unit_damage.cpp','CMakeLists.txt',
  'tools/oracle_mp_power_module_death_1_1.py','tools/export_mp_power_module_death_fixture.py',
  'tools/inspect_prop_explosion_definitions.py','tools/inspect_prop_explosion_runtime.cpp',
  'tools/freeze_live_prop_sandbox.py','docs/MP_LIVE_PROP_SANDBOX_RESEARCH.md',
  'docs/MP_PROP_EXPLOSION_PARTICLE_ADMISSION.md',
  'build-physics-body-state-20261008/prop_sandbox_tests','build-prop-sandbox-asan-20261010/prop_sandbox_tests']
 paths += ['analysis/multiplayer/integration/'+name for name in logs]
 paths += [str(p.relative_to(ROOT))for p in OUT.rglob('*')if p.is_file()and '/frozen/' not in str(p)]
 entries=[]
 for relative in sorted(set(paths)):
  p=ROOT/relative;raw=p.read_bytes();write(OUT/'frozen/files'/relative,raw)
  entries.append(dict(path=relative,sha256=hashlib.sha256(raw).hexdigest(),bytes=len(raw)))
 audit=dict(schema=1,original_report_and_instruction_repeat_identical=True,
  original_repeat_files=len(files),original_instruction_files=len(report['instructions']),
  preserved_original_prior_pins=len(report['prior_pins']),original_nonstack_ordered_stores=len(report['stores']),
  original_section_fixture_rows=8,native_tests=dict(normal=True,actual_asan_ubsan=True,udp_late_join=True,
  supported_image_data_providers=['MapPack11','TitleUpdate15'],all_real_maps=21,retained_props=390,triangles=15132),
  boundaries=['Original incoming-hit admission is not executed by the explicit original death experiment.',
   'Live geometry uses exact authored posed triangles and the existing host mover, not Havok solver parity.',
   'Area nearest-point/obstruction and enumeration remain declared host services; acceleration/camouflage unavailable.',
   'All seven actual canister materials import with existing BC decoders; world particle lifecycle/controllers/classification8 drawing unavailable.',
   'LAN removal retains literal emitted request/pose/tick; root owns audio/visual event consumption.',
   'Copied shared source is this validation snapshot, not a promise current shared source remains immutable.'])
 write(OUT/'frozen/audit.json',(json.dumps(audit,indent=2,sort_keys=True)+'\n').encode())
 manifest=dict(schema=1,audit_sha256=sha(OUT/'frozen/audit.json'),files=entries)
 write(OUT/'frozen/manifest.json',(json.dumps(manifest,indent=2,sort_keys=True)+'\n').encode())
 print(json.dumps(dict(manifest=str((OUT/'frozen/manifest.json').relative_to(ROOT)),sha256=sha(OUT/'frozen/manifest.json'),**audit)))
if __name__=='__main__':main()
