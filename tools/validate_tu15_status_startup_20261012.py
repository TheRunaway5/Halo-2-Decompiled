#!/usr/bin/env python3
"""Freeze and seal the original TU1.5 settings/status startup composition."""
import argparse,ast,hashlib,json,shutil
from pathlib import Path
import xml.etree.ElementTree as ET
ROOT=Path(__file__).resolve().parents[1]
E=ROOT/'analysis/multiplayer/sandbox-source/tu15-status-startup-20261012'
P=ROOT/'analysis/multiplayer/sandbox-source/tu15-settings-startup-20261012'
P_PIN='a3d3e173889e1dbebee59ca60928f613677410240c24e729324e5594e244dcbd'
OWN=('include/halo2/source_tu15_status.hpp','src/platform/source_tu15_status.cpp','src/platform/source_tu15_status_runtime_generated.cpp',
 'tests/source_tu15_status_tests.cpp','tests/source_tu15_status_tables_tests.cpp','cmake/SourceTu15Status.cmake',
 'tools/generate_tu15_status_runtime_20261012.py','tools/oracle_tu15_status_startup_20261012.py',
 'tools/oracle_tu15_status_tables_20261012.py','tools/validate_tu15_status_startup_20261012.py',
 'tools/capture_tu15_status_export_20261012.py','tools/capture_tu15_status_export_boot_20261012.py',
 'tools/capture_tu15_status_tables_20261012.py','tools/capture_tu15_status_tables_boot_20261012.py',
 'docs/PHYSICS_TU15_STATUS_STARTUP_20261012.md')
TESTS={'original_tu15_fresh_startup_records','original_tu15_fresh_startup_kernel_data',
 'original_tu15_fresh_startup_settings','original_tu15_fresh_startup_status','original_tu15_status_tables','original_runtime_image_tu15'}
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def read(p):return json.loads(Path(p).read_text())
def freeze():
 assert sha(P/'source-manifest-v1.json')==P_PIN
 parent=read(P/'source-manifest-v1.json');origins={n:P/'source-v1'/n for n in parent['files']}
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
     name=str(found.relative_to(ROOT))
     if name not in origins:origins[name]=found;pending.append(name)
 target=E/'source-v1';target.mkdir();pins={n:sha(p) for n,p in origins.items()}
 for n,p in origins.items():
  dest=target/n;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,dest);assert sha(dest)==pins[n]==sha(p)
 archives={}
 for mode,row in parent['archives'].items():
  origin=P/'source-v1'/row['file'];assert sha(origin)==row['sha256']
  dest=target/row['file'];dest.parent.mkdir(exist_ok=True);shutil.copyfile(origin,dest)
  assert sha(dest)==row['sha256'];archives[mode]=dict(file=row['file'],sha256=row['sha256'],parent=str(origin))
 (target/'CMakeLists.txt').write_text((P/'source-v1/CMakeLists.txt').read_text()+'\ninclude(cmake/SourceTu15Status.cmake)\n')
 for n in ('assets','analysis'):(target/n).symlink_to(ROOT/n,target_is_directory=True)
 result=dict(schema=1,parity_target='English TU1.5 only',files=pins,archives=archives,top_sha256=sha(target/'CMakeLists.txt'),
  parent_source_manifest_sha256=P_PIN,tests=sorted(TESTS),world_archive_reuse=True,
  scope='Complete owned modified-BIOS status routine and TU1.5 caller compiled freshly; unchanged world archives pinned. Timer/world/live owners unresolved.')
 (E/'source-manifest-v1.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
 print(json.dumps(dict(files=len(pins),manifest_sha256=sha(E/'source-manifest-v1.json'))))
def tests(path,expected):
 root=ET.parse(path).getroot();assert {c.attrib['name'] for c in root.iter('testcase')}==expected
 assert not any(list(root.iter(t)) for t in ('failure','error','skipped'))
def snapshot():
 production=ROOT/'analysis/multiplayer/sandbox-source/phantom-world-tu15-20261011/build-production'
 tests(E/'production-tests-v3.xml',TESTS-{'original_runtime_image_tu15'})
 names=set(read(P/'production-snapshot/manifest.json')['files'])-{'tests.xml'}
 names.update(('source_tu15_status_tests','source_tu15_status_tables_tests','libhalo2_source_tu15_status.a'))
 dest=E/'production-snapshot';dest.mkdir();pins={}
 for n in sorted(names):
  source=production/n;pin=sha(source);shutil.copyfile(source,dest/n);assert sha(dest/n)==pin==sha(source);pins[n]=pin
 shutil.copyfile(E/'production-tests-v3.xml',dest/'tests.xml');pins['tests.xml']=sha(dest/'tests.xml')
 native={n:pin for n,pin in read(E/'source-manifest-v1.json')['files'].items() if n.endswith(('.hpp','.cpp','.cmake'))}
 for n,pin in native.items():assert sha(ROOT/n)==pin,n
 result=dict(schema=1,files=pins,native_source_pins=native,production_directory=str(production),
  source_manifest_sha256=sha(E/'source-manifest-v1.json'),tests=sorted(TESTS-{'original_runtime_image_tu15'}),
  world_archive_variant='Current production graph may include unrelated unused pool/main-heap/physical-storage owners; selected startup uses the pinned shared world bodies.')
 (dest/'manifest.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
 print(json.dumps(dict(files=len(pins),manifest_sha256=sha(dest/'manifest.json'))))
def validate():
 manifest=read(E/'source-manifest-v1.json');assert manifest['parent_source_manifest_sha256']==P_PIN
 for n,pin in manifest['files'].items():
  assert sha(E/'source-v1'/n)==pin,n
  if n.endswith(('.hpp','.cpp','.cmake')):assert sha(ROOT/n)==pin,n
 assert sha(E/'source-v1/CMakeLists.txt')==manifest['top_sha256']
 for row in manifest['archives'].values():assert sha(E/'source-v1'/row['file'])==row['sha256']
 generated=read(E/'generated-v1.json');assert generated==read(E/'generated-repeat.json')
 assert generated['generator_sha256']==sha(ROOT/OWN[6]) and generated['generated_sha256']==sha(ROOT/OWN[2])==sha(E/'runtime-repeat.cpp')
 assert generated['instruction_count']==59 and not generated['production_decoder'] and not generated['physical_xbox_proof']
 for n,tool in (('original',OWN[7]),('table-original',OWN[8])):
  assert sha(E/(n+'-v1/fixture.json'))==sha(E/(n+'-repeat/fixture.json'))
  assert read(E/(n+'-v1/fixture.json'))['tool_sha256']==sha(ROOT/tool)
 f=read(E/'original-v1/fixture.json');assert len(f['cases'])==48 and len(f['source_instructions'])==182
 assert f['original_sha256']==sha(f['original_path'])=='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
 assert sha(f['eeprom_input']['path'])==f['eeprom_input']['sha256']
 assert f['settings_wrapper_completed'] and f['status_converter_executed']
 assert sum(len(c['instructions']) for c in f['cases'])==2760
 assert sum(len(c['stores']) for c in f['cases'])==1800
 assert sum(len(c['kernel_instructions']) for c in f['cases'])==8112
 assert sum(len(c['kernel_stores']) for c in f['cases'])==144
 assert all(c['boundary']['target']==0x80000095 and c['boundary']['width']==20 for c in f['cases'])
 assert {(c['kernel_calls'][0]['before']['gp'][0],c['kernel_calls'][0]['after']['gp'][0]) for c in f['cases']}=={(0,0),(0xc000009c,23)}
 for key in ('older_image_executed','native_expected_data_used','captured_world_input_used','timer_armed',
  'process_heap_installed','live_physics_parity','physical_xbox_proof'):assert not f[key],key
 assert not f['source_instruction_patches']
 t=read(E/'table-original-v1/fixture.json');assert len(t['cases'])==786
 assert len(t['covered_instructions'])==58 and sum(len(c['instructions']) for c in t['cases'])==366026
 assert set(map(int,t['source_instructions']))-set(t['covered_instructions'])=={0x800236bd}
 assert t['original_code_executed'] and not t['code_patches'] and not t['native_expected_data_used'] and not t['physical_xbox_proof']
 for boot in ('boot-v3','boot-repeat'):
  d=E/boot;v=read(d/'validation.json');r=read(d/'kernel/report.json')
  assert v['kernel_report_sha256']==sha(d/'kernel/report.json')
  assert v['original_resources_unchanged'] and not v['title_executed'] and not v['guest_writes'] and not v['code_patches']
  assert r['tool_sha256']==sha(ROOT/OWN[12]) and v['boot_probe_override_sha256']==sha(ROOT/OWN[13])
  for b in r['status301_tables']['records']:
   assert sha(d/'kernel'/(b['name']+'.bin'))==b['sha256']==sha(E/'boot-v3/kernel'/(b['name']+'.bin'))
 builds={}
 for mode in ('release','sanitizer'):
  d=E/('build-'+mode);tests(d/'tu15-tests.xml',TESTS)
  builds[mode]=dict(tests=len(TESTS),results_sha256=sha(d/'tu15-tests.xml'),binaries={n:sha(d/n) for n in ('source_tu15_status_tests','source_tu15_status_tables_tests')})
 production=read(E/'production-snapshot/manifest.json')
 for n,pin in production['files'].items():assert sha(E/'production-snapshot'/n)==pin
 tests(E/'production-snapshot/tests.xml',TESTS-{'original_runtime_image_tu15'})
 result=dict(schema=1,parity_target='English TU1.5 only',reference='owned modified Complex4627 kernel301; not physical Xbox proof',
  validator_sha256=sha(__file__),source_manifest_sha256=sha(E/'source-manifest-v1.json'),
  original_fixture_sha256=sha(E/'original-v1/fixture.json'),table_fixture_sha256=sha(E/'table-original-v1/fixture.json'),
  generator_manifest_sha256=sha(E/'generated-v1.json'),fresh_prefixes=48,title_instructions=2760,title_writes=1800,
  kernel_instructions=8112,kernel_writes=144,table_cases=786,table_instructions=366026,reachable_sites_covered=58,
  builds=builds,production_snapshot_manifest_sha256=sha(E/'production-snapshot/manifest.json'),
  timer_still_required=True,heap_world_body_ownership_still_required=True,live_physics_parity=False,physical_xbox_proof=False)
 (E/'validation.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n');print(json.dumps(result))
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--freeze',action='store_true');ap.add_argument('--snapshot',action='store_true');a=ap.parse_args()
 freeze() if a.freeze else snapshot() if a.snapshot else validate()
