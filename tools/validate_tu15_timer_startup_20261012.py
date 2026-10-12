#!/usr/bin/env python3
"""Freeze and validate the connected original TU1.5 timer prerequisites."""
import argparse,contextlib,hashlib,io,json,shutil
from pathlib import Path
import validate_tu15_status_startup_20261012 as parent
ROOT=Path(__file__).resolve().parents[1]
E=ROOT/'analysis/multiplayer/sandbox-source/tu15-timer-startup-20261012'
P=ROOT/'analysis/multiplayer/sandbox-source/tu15-status-startup-20261012'
P_PIN='060ca30ada87f8e914ae21e808925f01cbf4996e2f1bd60ff5d5ff38de36678f'
OWN=('include/halo2/source_tu15_timer.hpp','src/source_tu15_timer.cpp','src/source_tu15_timer_runtime_generated.cpp',
 'tests/source_tu15_timer_startup_tests.cpp','tests/source_tu15_timer_procedures_tests.cpp','cmake/SourceTu15Timer.cmake',
 'tools/generate_tu15_timer_runtime_20261012.py','tools/oracle_tu15_timer_startup_20261012.py',
 'tools/oracle_tu15_timer_procedures_20261012.py','tools/validate_tu15_timer_startup_20261012.py',
 'tools/capture_tu15_timer_exports_20261012.py','tools/capture_tu15_timer_exports_boot_20261012.py',
 'tools/capture_tu15_timer_dependencies_20261012.py','tools/capture_tu15_timer_dependencies_boot_20261012.py',
 'tools/capture_tu15_kernel_timer_text_20261012.py','tools/capture_tu15_kernel_timer_text_boot_20261012.py',
 'tools/research_tu15_timer_owner_20261012.py','docs/PHYSICS_TU15_TIMER_STARTUP_20261012.md')
TESTS=parent.TESTS|{'original_tu15_fresh_startup_timer','original_tu15_timer_procedures'}
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def read(p):return json.loads(Path(p).read_text())
def write(p,obj):Path(p).write_text(json.dumps(obj,sort_keys=True,indent=2)+'\n')
def freeze():
 parent.ROOT=ROOT;parent.E=E;parent.P=P;parent.P_PIN=P_PIN;parent.OWN=OWN;parent.TESTS=TESTS
 with contextlib.redirect_stdout(io.StringIO()):parent.freeze()
 top=E/'source-v1/CMakeLists.txt';text=top.read_text();prefix,last,after=text.rpartition('include(cmake/SourceTu15Status.cmake)')
 assert last and not after.strip();top.write_text(prefix+'include(cmake/SourceTu15Timer.cmake)'+after)
 m=read(E/'source-manifest-v1.json');m['top_sha256']=sha(top)
 m['scope']='Nine original timer bodies and actual TU1.5 caller; native interrupt/clock/expiry and live physics ownership remain required.'
 write(E/'source-manifest-v1.json',m);print(json.dumps(dict(files=len(m['files']),sha256=sha(E/'source-manifest-v1.json'))))
def snapshot():
 production=ROOT/'analysis/multiplayer/sandbox-source/phantom-world-tu15-20261011/build-production'
 parent.tests(E/'production-tests-v4.xml',TESTS-{'original_runtime_image_tu15'})
 names=set(read(P/'production-snapshot/manifest.json')['files'])-{'tests.xml'}
 names.update(('source_tu15_timer_startup_tests','source_tu15_timer_procedures_tests','libhalo2_source_tu15_timer.a'))
 dest=E/'production-snapshot';dest.mkdir();pins={}
 for n in sorted(names):
  pin=sha(production/n);shutil.copyfile(production/n,dest/n);assert sha(dest/n)==pin==sha(production/n);pins[n]=pin
 shutil.copyfile(E/'production-tests-v4.xml',dest/'tests.xml');pins['tests.xml']=sha(dest/'tests.xml')
 native={n:pin for n,pin in read(E/'source-manifest-v1.json')['files'].items() if n.endswith(('.hpp','.cpp','.cmake'))}
 for n,pin in native.items():assert sha(ROOT/n)==pin
 write(dest/'manifest.json',dict(schema=1,files=pins,native_source_pins=native,tests=sorted(TESTS-{'original_runtime_image_tu15'}),
  source_manifest_sha256=sha(E/'source-manifest-v1.json'),production_directory=str(production),
  world_archive_variant='Current production graph may contain unrelated unused owners; selected startup uses the pinned source world bodies.'))
 print(json.dumps(dict(files=len(pins),sha256=sha(dest/'manifest.json'))))
def validate():
 m=read(E/'source-manifest-v1.json');assert m['parent_source_manifest_sha256']==P_PIN
 for n,pin in m['files'].items():
  assert sha(E/'source-v1'/n)==pin
  if n.endswith(('.hpp','.cpp','.cmake')):assert sha(ROOT/n)==pin,n
 assert sha(E/'source-v1/CMakeLists.txt')==m['top_sha256']
 for row in m['archives'].values():assert sha(E/'source-v1'/row['file'])==row['sha256']
 g=read(E/'generated-v3.json');assert g==read(E/'generated-repeat.json')
 assert g['generated_sha256']==sha(ROOT/OWN[2])==sha(E/'runtime-repeat.cpp') and g['generator_sha256']==sha(ROOT/OWN[6])
 assert g['complete_bodies']==9 and g['instruction_count']==304 and not g['production_decoder']
 for first,repeat,tool in (('startup-original-v2','startup-original-repeat-v2',OWN[7]),('procedures-original-v1','procedures-original-repeat',OWN[8])):
  assert sha(E/first/'fixture.json')==sha(E/repeat/'fixture.json')
  assert read(E/first/'fixture.json')['tool_sha256']==sha(ROOT/tool)
 f=read(E/'startup-original-v2/fixture.json');p=read(E/'procedures-original-v1/fixture.json')
 assert len(f['cases'])==48 and len(p['cases'])==37 and not f['timer_armed'] and not f['older_image_executed']
 assert sha(f['original_path'])==f['original_sha256']=='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
 assert not f['timer_reference']['mutable_context_supplied'] and not f['live_physics_parity'] and not p['captured_scheduler_state_used']
 assert all(c['boundary']['instruction']==0x8001434a and c['boundary']['address']==0x80035c00 and c['boundary']['bytes']==1 for c in f['cases'])
 assert sum(len(c['kernel_instructions']) for c in f['cases'])==8832 and sum(len(c['kernel_stores']) for c in f['cases'])==720
 assert sum(len(v['instructions']) for c in p['cases'] for v in c['phases'])==15536
 assert sum(len(v['stores']) for c in p['cases'] for v in c['phases'])==6317
 assert p['original_producers_executed'] and p['procedure_clock_inputs'] and not p['live_timer_scheduling'] and not p['code_patches']
 covered=set(p['covered_instructions'])|{pc for c in f['cases'] for pc in c['kernel_instructions'] if f'{pc:08x}' in g['source_instructions']}
 assert len(covered)==210
 boot=E/'text-boot-v1';capture=read(boot/'kernel/report.json');v=read(boot/'validation.json')
 assert sha(boot/'kernel/kernel-text.bin')==capture['code_section']['sha256']==g['function_sha256']
 assert capture['tool_sha256']==sha(ROOT/OWN[14]) and not capture['mutable_kernel_state_captured']
 assert v['original_resources_unchanged'] and not v['guest_writes'] and not v['title_executed'] and v['kernel_report_sha256']==sha(boot/'kernel/report.json')
 builds={}
 for mode in ('release','sanitizer'):
  d=E/('build-'+mode);parent.tests(d/'tu15-tests.xml',TESTS)
  builds[mode]=dict(tests=len(TESTS),results_sha256=sha(d/'tu15-tests.xml'),binaries={n:sha(d/n) for n in ('source_tu15_timer_startup_tests','source_tu15_timer_procedures_tests')})
 production=read(E/'production-snapshot/manifest.json')
 for n,pin in production['files'].items():assert sha(E/'production-snapshot'/n)==pin
 parent.tests(E/'production-snapshot/tests.xml',TESTS-{'original_runtime_image_tu15'})
 result=dict(schema=1,parity_target='English TU1.5 only',validator_sha256=sha(__file__),source_manifest_sha256=sha(E/'source-manifest-v1.json'),
  startup_fixture_sha256=sha(E/'startup-original-v2/fixture.json'),procedure_fixture_sha256=sha(E/'procedures-original-v1/fixture.json'),
  complete_bodies=9,sites=304,sites_covered=210,sites_unverified=94,fresh_startup_prefixes=48,procedure_cases=37,
  builds=builds,production_snapshot_manifest_sha256=sha(E/'production-snapshot/manifest.json'),
  native_context_clock_and_expiry_still_required=True,heap_world_body_ownership_still_required=True,live_timer_scheduling=False,live_physics_parity=False,physical_xbox_proof=False)
 write(E/'validation.json',result);print(json.dumps(result))
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--freeze',action='store_true');ap.add_argument('--snapshot',action='store_true');a=ap.parse_args()
 freeze() if a.freeze else snapshot() if a.snapshot else validate()
