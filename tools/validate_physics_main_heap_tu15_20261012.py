#!/usr/bin/env python3
"""Audit exact fresh heap builds, original differential corpus and dependency closure."""
import hashlib,json,xml.etree.ElementTree as ET
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
if ROOT.name.startswith('source-v'):ROOT=ROOT.parents[4]
E=ROOT/'analysis/multiplayer/sandbox-source/physics-main-heap-tu15-20261012'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def same_tree(a,b):
 x={str(p.relative_to(a)):sha(p)for p in a.rglob('*')if p.is_file()}
 y={str(p.relative_to(b)):sha(p)for p in b.rglob('*')if p.is_file()};assert x==y,(a,b)
def main():
 m=json.loads((E/'source-manifest-v2.json').read_text());source=E/'source-v2'
 for p,h in m['files'].items():
  assert sha(source/p)==h,p
  if p!='CMakeLists.txt':assert sha(ROOT/p)==h,p
 for mode,parent in m['parents'].items():
  assert sha(E/parent['archive'])==parent['archive_sha256']
  assert sha(ROOT/parent['dependency_manifest'])==parent['dependency_manifest_sha256']
  for p,h in parent['source_inputs'].items():assert sha(ROOT/p)==h,p
 same_tree(E/'original-v2',E/'original-v2-repeat')
 assert sha(E/'original-v2/fixture.json')==m['original_fixture_sha256']
 assert sha(ROOT/'src/physics_main_heap_runtime_generated.cpp')==sha(E/'runtime-repeat.cpp')
 assert json.loads((E/'generated-v1.json').read_text())==json.loads((E/'generated-repeat.json').read_text())
 f=json.loads((E/'original-v2/fixture.json').read_text())
 assert not f['captured_heap'] and not f['source_calls_replaced'] and not f['live_world_startup']
 assert len(f['sequences'])==3 and sum(len(s['steps'])for s in f['sequences'])==1839
 assert sum(len(r['stores'])for s in f['sequences']for r in s['steps'])==24114
 builds={}
 for mode in ('release','sanitizer'):
  b=E/f'build-v2-{mode}';xml=ET.parse(b/'tests.xml');tests=xml.findall('.//testcase')
  assert {t.attrib['name']for t in tests}=={'native_physics_main_heap_tu15','native_physics_pool_startup_tu15'}
  assert all(not t.findall('failure') and not t.findall('error') and not t.findall('skipped')for t in tests)
  log=(b/'tests.log').read_text();assert '100% tests passed' in log and 'fresh TU1.5 main heap lifecycle calls' in log
  commands=json.loads((b/'compile_commands.json').read_text())
  for row in commands:
   p=Path(row['file']);assert p.is_relative_to(source) and sha(p)==m['files'][str(p.relative_to(source))]
   assert '-fno-fast-math' in row['command'] and '-ffp-contract=off' in row['command']
   if mode=='sanitizer':assert '-fsanitize=address,undefined' in row['command']
  links=(b/'CMakeFiles/physics_main_heap_tests.dir/link.txt').read_text()
  assert f'parent-archives/{mode}.a' in links
  builds[mode]=dict(binary_sha256=sha(b/'physics_main_heap_tests'),results_sha256=sha(b/'tests.xml'),
   tests_log_sha256=sha(b/'tests.log'),compile_commands_sha256=sha(b/'compile_commands.json'),tests=len(tests))
 report=dict(schema=1,source_manifest_sha256=sha(E/'source-manifest-v2.json'),original_fixture_sha256=sha(E/'original-v2/fixture.json'),
  steps=1839,ordered_writes=24114,actual_original_instructions=f['instruction_count'],executed_sites=len(f['executed_sites']),
  fresh_native_resources=3,final_retained_bytes=3231744,builds=builds,live_physics_parity=False,full_world_startup=False,parity_target='English TU1.5 only')
 path=E/'validation.json';raw=json.dumps(report,sort_keys=True,indent=2)+'\n'
 if path.exists():assert path.read_text()==raw,'Do not silently replace prior seal'
 else:path.write_text(raw)
 print('PASS fresh TU1.5 main heap native/source differential, Release and sanitizer; '+sha(path))
if __name__=='__main__':main()
