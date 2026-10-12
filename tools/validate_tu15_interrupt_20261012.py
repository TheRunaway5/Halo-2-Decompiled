#!/usr/bin/env python3
"""Freeze and verify original TU1.5 interrupt-lowering evidence."""
import argparse,hashlib,json,shutil
from pathlib import Path
import validate_tu15_status_startup_20261012 as parent
ROOT=Path(__file__).resolve().parents[1]
E=ROOT/'analysis/multiplayer/sandbox-source/tu15-interrupt-startup-20261012'
P=ROOT/'analysis/multiplayer/sandbox-source/tu15-timer-startup-20261012'
PIN='77deab80b735a520a47254966d2f40fe48adf51837228d3f862732420ffd6582'
OWN=('include/halo2/source_tu15_interrupt.hpp','src/source_tu15_interrupt.cpp','src/source_tu15_interrupt_runtime_generated.cpp',
 'cmake/SourceTu15Interrupt.cmake','tests/source_tu15_interrupt_tests.cpp','tools/generate_tu15_interrupt_runtime_20261012.py',
 'tools/oracle_tu15_interrupt_20261012.py','tools/capture_tu15_kernel_entry_20261012.py',
 'tools/capture_tu15_kernel_entry_boot_20261012.py','tools/validate_tu15_interrupt_20261012.py',
 'docs/PHYSICS_TU15_INTERRUPT_LOWERING_20261012.md','tests/source_tu15_interrupt_startup_tests.cpp')
TESTS=set(json.loads((P/'source-manifest-v1.json').read_text())['tests'])|{'original_tu15_interrupt_lowering','original_tu15_fresh_startup_interrupts'}
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def read(p):return json.loads(Path(p).read_text())
def write(p,v):Path(p).write_text(json.dumps(v,sort_keys=True,indent=2)+'\n')
def freeze():
 assert sha(P/'source-manifest-v1.json')==PIN
 old=read(P/'source-manifest-v1.json');target=E/'source-v1';target.mkdir();pins={}
 for n,pin in old['files'].items():
  origin=P/'source-v1'/n;assert sha(origin)==pin
  dest=target/n;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(origin,dest);pins[n]=pin
 for n in OWN:
  origin=ROOT/n;pin=sha(origin);dest=target/n;dest.parent.mkdir(parents=True,exist_ok=True)
  shutil.copyfile(origin,dest);assert sha(dest)==pin==sha(origin);pins[n]=pin
 for row in old['archives'].values():
  origin=P/'source-v1'/row['file'];assert sha(origin)==row['sha256']
  dest=target/row['file'];dest.parent.mkdir(exist_ok=True);shutil.copyfile(origin,dest)
 top=target/'CMakeLists.txt';top.write_text((P/'source-v1/CMakeLists.txt').read_text()+'\ninclude(cmake/SourceTu15Interrupt.cmake)\n')
 for n in ('assets','analysis'):(target/n).symlink_to(ROOT/n,target_is_directory=True)
 m=dict(schema=1,files=pins,archives=old['archives'],top_sha256=sha(top),tests=sorted(TESTS),
  parent_source_manifest_sha256=PIN,foundation='Pinned prior complete source snapshot; shared cache.hpp has since changed in the root checkout.',
  scope='Actual CMake modules for complete lowering bodies and procedure returns. Initial root configuration failure is historical; production targets subsequently build and pass. Live context, clocks and physics remain required.')
 write(E/'source-manifest-v1.json',m);print(json.dumps(dict(files=len(m['files']),sha256=sha(E/'source-manifest-v1.json'))))
def validate():
 m=read(E/'source-manifest-v1.json');assert m['parent_source_manifest_sha256']==PIN
 for n,pin in m['files'].items():
  assert sha(E/'source-v1'/n)==pin,n
  if n in OWN:assert sha(ROOT/n)==pin,n
 assert sha(E/'source-v1/CMakeLists.txt')==m['top_sha256']
 for row in m['archives'].values():assert sha(E/'source-v1'/row['file'])==row['sha256']
 generated=read(E/'generated-v1.json');assert generated==read(E/'generated-repeat.json')
 assert generated['generated_sha256']==sha(ROOT/OWN[2])==sha(E/'runtime-repeat.cpp')
 assert generated['generator_sha256']==sha(ROOT/OWN[5]) and generated['complete_bodies']==2 and generated['instruction_count']==67
 fixture=E/'procedures-original-v1/fixture.json';f=read(fixture)
 assert sha(fixture)==sha(E/'procedures-original-repeat/fixture.json') and f['tool_sha256']==sha(ROOT/OWN[6])
 assert len(f['cases'])==55 and not f['callee_returns_supplied'] and not f['captured_scheduler_state_used'] and not f['code_patches']
 assert sum(len(p['instructions']) for c in f['cases'] for p in c['phases'])==5298
 assert sum(len(p['stores']) for c in f['cases'] for p in c['phases'])==2021
 assert sum(len(p['ports']) for c in f['cases'] for p in c['phases'])==24
 sites={int(p,16) for p in generated['source_instructions']};covered=sites&set(f['covered_instructions']);assert len(covered)==44
 first=E/'entry-boot-v2';repeat=E/'entry-boot-repeat'
 for boot in (first,repeat):
  b=read(boot/'validation.json');r=read(boot/'kernel/report.json')
  assert b['original_resources_unchanged'] and not b['guest_writes'] and not b['title_executed'] and not b['code_patches']
  assert r['entry']==0x8005504c and r['phase']=='before kernel entry instruction' and not r['guest_memory_writes']
  # Exact executed source is retained; the current tool corrects only a comment
  # describing INIT, which is writable loader-initial data, not immutable text.
  assert r['tool_sha256']==sha(E/'capture-entry-executed-v1.py')
  assert b['boot_driver_sha256']==sha(ROOT/OWN[8])
  assert sha(boot/'kernel/report.json')==b['kernel_report_sha256']
  assert sha(boot/'kernel/kernel-init.bin')==sha(first/'kernel/kernel-init.bin')
  assert sha(boot/'kernel/entry-metadata.bin')==sha(first/'kernel/entry-metadata.bin')
  for record in r['records']:
   path=boot/'kernel'/(record['name']+'.bin')
   assert sha(path)==record['sha256']==sha(first/'kernel'/path.name) and not any(path.read_bytes())
 builds={}
 for mode in ('release','sanitizer'):
  d=E/('build-'+mode);parent.tests(d/'tu15-tests.xml',TESTS)
  builds[mode]=dict(tests=len(TESTS),results_sha256=sha(d/'tu15-tests.xml'),binary_sha256=sha(d/'source_tu15_interrupt_tests'))
 production=read(E/'production-snapshot/manifest.json')
 for n,pin in production['files'].items():assert sha(E/'production-snapshot'/n)==pin,n
 parent.tests(E/'production-snapshot/tests.xml',{'original_tu15_fresh_startup_interrupts','original_tu15_interrupt_lowering'})
 result=dict(schema=1,source_manifest_sha256=sha(E/'source-manifest-v1.json'),validator_sha256=sha(__file__),
  original_fixture_sha256=sha(fixture),complete_bodies=2,sites=67,sites_covered=44,sites_unverified=23,
  procedure_cases=55,procedure_context_inputs=True,procedure_timer_returns=9,builds=builds,
  production_tests=2,production_snapshot_manifest_sha256=sha(E/'production-snapshot/manifest.json'),
  live_context_clock_ownership_still_required=True,live_physics_parity=False,physical_xbox_proof=False)
 write(E/'validation.json',result);print(json.dumps(result))
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--freeze',action='store_true');a=ap.parse_args()
 freeze() if a.freeze else validate()
