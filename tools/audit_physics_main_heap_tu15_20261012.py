#!/usr/bin/env python3
"""Seal original correspondence/repetition and the native build evidence."""
import hashlib,json,shutil,sys
from pathlib import Path
ROOT=next(p for p in Path(__file__).resolve().parents if (p/'tools/oracle_smg_firing_1_1.py').is_file())
sys.path.insert(0,str(ROOT/'tools'))
from oracle_smg_firing_1_1 import Xbe,PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
E=ROOT/'analysis/multiplayer/sandbox-source/physics-main-heap-tu15-20261012'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def same_tree(a,b):
 def files(d):return {str(p.relative_to(d)):sha(p)for p in d.rglob('*')if p.is_file()}
 assert files(a)==files(b),(a,b)
def main():
 same_tree(E/'correspondence-v1',E/'correspondence-repeat');same_tree(E/'original-v2',E/'original-v2-repeat')
 paired=E/'correspondence-v1/complete-report.json';assert sha(paired)=='579a526dba8c9fb6e090b78c79c906487935b1d74b9c4b8ef601b22d71277de4'
 d=json.loads(paired.read_text());f=json.loads((E/'original-v2/fixture.json').read_text())
 old_path=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
 assert sha(old_path)==PIN and sha(NEW_DEFAULT)==NEW_PIN
 assert d['tool_sha256']==sha(ROOT/'tools/research_physics_main_heap_tu15_20261012.py')
 assert f['tool_sha256']==sha(ROOT/'tools/oracle_physics_main_heap_tu15_20261012.py')
 assert f['correspondence_sha256']==sha(paired) and f['original_sha256']==NEW_PIN
 old,new=Xbe(old_path),Xbe(NEW_DEFAULT)
 for row in d['instructions']:
  assert old.read(row['old'],len(bytes.fromhex(row['old_bytes']))).hex()==row['old_bytes']
  assert new.read(row['later'],len(bytes.fromhex(row['later_bytes']))).hex()==row['later_bytes']
 for table in d['vtables'].values():assert new.read(table['later'],len(bytes.fromhex(table['hex']))).hex()==table['hex']
 generated=json.loads((E/'generated-v1.json').read_text())
 assert generated==json.loads((E/'generated-repeat.json').read_text())
 assert generated['generator_sha256']==sha(ROOT/'tools/generate_physics_main_heap_tu15_20261012.py')
 assert generated['correspondence_sha256']==sha(paired)
 assert generated['generated_cpp_sha256']==sha(ROOT/'src/physics_main_heap_runtime_generated.cpp')==sha(E/'runtime-repeat.cpp')
 from validate_physics_main_heap_tu15_20261012 import main as validate
 validate()
 for mode in ('release','sanitizer'):
  assert 'Cannot find source file' in (E/f'configure-{mode}.log').read_text()
 pin=sha(__file__);copy=E/'audit-source-v1.py'
 if copy.exists():assert sha(copy)==pin
 else:shutil.copyfile(__file__,copy)
 report=dict(schema=1,audit_sha256=pin,correspondence_sha256=sha(paired),fixture_sha256=sha(E/'original-v2/fixture.json'),
  native_validation_sha256=sha(E/'validation.json'),source_manifest_sha256=sha(E/'source-manifest-v2.json'),
  generated_manifest_sha256=sha(E/'generated-v1.json'),original_sha256=NEW_PIN,
  producer_sites_reached=sum(r['later'] in f['executed_sites']for r in d['instructions']),producer_sites=22,
  original_producer_failure_path_executed=False,initial_failed_driver_preserved=True,
  documentation_sha256=sha(ROOT/'docs/PHYSICS_MAIN_HEAP_TU15_20261012.md'),live_world_startup=False,live_physics_parity=False)
 raw=json.dumps(report,sort_keys=True,indent=2)+'\n';p=E/'evidence-audit.json'
 if p.exists():assert p.read_text()==raw,'Preserve earlier audit'
 else:p.write_text(raw)
 print('PASS complete source/evidence audit '+sha(p))
if __name__=='__main__':main()
