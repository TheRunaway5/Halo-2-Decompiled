#!/usr/bin/env python3
"""Freeze and seal the original TU1.5 settings/query caller composition."""
import argparse,ast,hashlib,json,shutil
from pathlib import Path
import xml.etree.ElementTree as ET
ROOT=Path(__file__).resolve().parents[1]
E=ROOT/'analysis/multiplayer/sandbox-source/tu15-settings-startup-20261012'
P=ROOT/'analysis/multiplayer/sandbox-source/tu15-kernel-hardware-20261012'
P_PIN='cef31a2a431b06a168ee2d65b074cd785b3ca547907504510f51021d642df455'
OWN=('include/halo2/source_tu15_settings.hpp','src/platform/source_tu15_settings.cpp',
 'src/platform/source_tu15_settings_runtime_generated.cpp','tests/source_tu15_settings_tests.cpp',
 'cmake/SourceTu15Settings.cmake','tools/generate_tu15_startup_settings_20261012.py',
 'tools/compile_tu15_settings_primary_20261012.py','tools/oracle_tu15_settings_startup_20261012.py',
 'tools/validate_tu15_settings_startup_20261012.py','docs/PHYSICS_TU15_SETTINGS_STARTUP_20261012.md')
TESTS={'original_tu15_fresh_startup_records','original_tu15_fresh_startup_kernel_data',
 'original_tu15_fresh_startup_settings','original_runtime_image_tu15'}
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def read(p):return json.loads(Path(p).read_text())
def freeze():
 assert sha(P/'source-manifest-v3.json')==P_PIN
 parent=read(P/'source-manifest-v3.json');origins={n:P/'source-v3'/n for n in parent['files']}
 for n,pin in parent['files'].items():
  assert sha(origins[n])==pin
  if n.endswith(('.hpp','.cpp','.cmake')):assert sha(ROOT/n)==pin,n
 origins.update({n:ROOT/n for n in OWN});pending=list(OWN)
 while pending:
  name=pending.pop()
  if not name.endswith('.py'):continue
  for node in ast.walk(ast.parse(origins[name].read_text())):
   modules=[node.module] if isinstance(node,ast.ImportFrom) and node.module else [v.name for v in node.names] if isinstance(node,ast.Import) else []
   for module in modules:
    found=next((q for q in (ROOT/'tools'/(module.replace('.','/')+'.py'),ROOT/'tools/reference/halo2-decompiled/tools'/(module.replace('.','/')+'.py')) if q.is_file()),None)
    if found:
     relative=str(found.relative_to(ROOT))
     if relative not in origins:origins[relative]=found;pending.append(relative)
 target=E/'source-v1';target.mkdir();pins={n:sha(p) for n,p in origins.items()}
 for name,path in origins.items():
  dest=target/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(path,dest)
  assert sha(dest)==pins[name]==sha(path),name
 provenance=read(P/'build-provenance-v2.json');archives={}
 for mode in ('release','sanitizer'):
  origin=P/('build-'+mode+'-v2/libhalo2_physics_world_runtime.a')
  pin=provenance[mode]['artifacts'][origin.name];assert sha(origin)==pin
  dest=target/'reference-libraries'/(mode+'-world.a');dest.parent.mkdir(exist_ok=True);shutil.copyfile(origin,dest)
  assert sha(dest)==pin;archives[mode]=dict(file=str(dest.relative_to(target)),sha256=pin,parent=str(origin))
 top=(P/'source-v3/CMakeLists.txt').read_text()
 top=top.replace('include(cmake/PhysicsWorldRuntime.cmake)','''add_library(halo2_physics_world_runtime STATIC IMPORTED GLOBAL)
if(HALO2_SANITIZERS)
    set(world_archive sanitizer-world.a)
else()
    set(world_archive release-world.a)
endif()
set_target_properties(halo2_physics_world_runtime PROPERTIES IMPORTED_LOCATION
    ${CMAKE_CURRENT_SOURCE_DIR}/reference-libraries/${world_archive})
target_link_libraries(halo2_systems PUBLIC halo2_physics_world_runtime)''')
 top+='\ninclude(cmake/SourceTu15Settings.cmake)\n'
 (target/'CMakeLists.txt').write_text(top)
 for name in ('assets','analysis'):(target/name).symlink_to(ROOT/name,target_is_directory=True)
 result=dict(schema=1,parity_target='English TU1.5 only',files=pins,archives=archives,top_sha256=sha(target/'CMakeLists.txt'),
  parent_source_manifest_sha256=P_PIN,tests=sorted(TESTS),world_archive_reuse=True,
  scope='New settings/query/startup wiring compiled freshly; unchanged world archive with complete matching dependency pins. No mapping/timer/world/live result.')
 (E/'source-manifest-v1.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
 print(json.dumps(dict(files=len(pins),manifest_sha256=sha(E/'source-manifest-v1.json'))))
def validate():
 manifest=read(E/'source-manifest-v1.json');assert manifest['parent_source_manifest_sha256']==P_PIN
 for name,pin in manifest['files'].items():
  assert sha(E/'source-v1'/name)==pin,name
  if name.endswith(('.hpp','.cpp','.cmake')):assert sha(ROOT/name)==pin,name
 for row in manifest['archives'].values():assert sha(E/'source-v1'/row['file'])==row['sha256']
 assert sha(E/'source-v1/CMakeLists.txt')==manifest['top_sha256']
 generated=read(E/'generated-v1.json');assert generated==read(E/'generated-repeat.json')
 assert generated['generator_sha256']==sha(ROOT/OWN[5])
 assert generated['generated_cpp_sha256']==sha(ROOT/OWN[2])==sha(E/'runtime-repeat.cpp')
 fixture=read(E/'original-v2/fixture.json');fixture_pin=sha(E/'original-v2/fixture.json')
 assert fixture_pin==sha(E/'original-repeat/fixture.json')
 assert fixture['tool_sha256']==sha(ROOT/OWN[7]) and fixture['primary_query_owner']['compiler_tool_sha256']==sha(ROOT/OWN[6])
 assert fixture['original_sha256']=='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
 assert sha(fixture['original_path'])==fixture['original_sha256'] and sha(fixture['eeprom_input']['path'])==fixture['eeprom_input']['sha256']
 assert len(fixture['cases'])==48 and len(fixture['source_instructions'])==182
 assert sum(len(c['instructions']) for c in fixture['cases'])==2016 and sum(len(c['stores']) for c in fixture['cases'])==1416
 calls=[q for c in fixture['cases'] for q in c['platform_calls'] if q['target']==0x80000018]
 assert sum(len(q['stores']) for q in calls)==264 and len(calls)==48
 assert sum(q['status']==0 for q in calls)==24 and sum(q['status']==0xc000009c for q in calls)==24
 assert all(c['boundary']['target']==0x8000012d and c['boundary']['width']==8 for c in fixture['cases'])
 for key in ('older_image_executed','native_expected_data_used','captured_world_input_used','settings_wrapper_completed',
  'timer_armed','process_heap_installed','live_physics_parity','physical_xbox_proof'):assert not fixture[key],key
 assert not fixture['source_instruction_patches']
 for bank in fixture['primary_query_owner']['banks']:assert sha(E/'original-v2/query'/bank['file'])==bank['sha256']
 builds={}
 for mode in ('release','sanitizer'):
  d=E/('build-'+mode);xml=d/'tu15-tests.xml';tests=ET.parse(xml).getroot()
  assert {c.attrib['name'] for c in tests.iter('testcase')}==TESTS
  assert not any(list(tests.iter(tag)) for tag in ('failure','error','skipped'))
  builds[mode]=dict(tests=len(TESTS),results_sha256=sha(xml),binary_sha256=sha(d/'source_tu15_settings_tests'))
 production=read(E/'production-snapshot/manifest.json')
 for name,pin in production['files'].items():assert sha(E/'production-snapshot'/name)==pin
 tests=ET.parse(E/'production-snapshot/tests.xml').getroot()
 assert {c.attrib['name'] for c in tests.iter('testcase')}==TESTS-{'original_runtime_image_tu15'}
 assert not any(list(tests.iter(tag)) for tag in ('failure','error','skipped'))
 result=dict(schema=1,parity_target='English TU1.5 only',validator_sha256=sha(__file__),
  source_manifest_sha256=sha(E/'source-manifest-v1.json'),original_fixture_sha256=fixture_pin,
  generator_manifest_sha256=sha(E/'generated-v1.json'),fresh_prefixes=48,source_instructions=2016,source_writes=1416,
  query_writes=264,actual_query_successes=24,actual_checksum_failures=24,builds=builds,
  production_snapshot_manifest_sha256=sha(E/'production-snapshot/manifest.json'),
  status_conversion_still_required=True,timer_still_required=True,heap_world_body_ownership_still_required=True,live_physics_parity=False)
 (E/'validation.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n');print(json.dumps(result))
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--freeze',action='store_true');a=ap.parse_args()
 freeze() if a.freeze else validate()
