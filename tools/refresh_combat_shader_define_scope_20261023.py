#!/usr/bin/env python3
"""Scope the installed shader path to its two existing renderer source readers."""
import hashlib,json,os
from pathlib import Path
from build_launcher_engine import source_hashes
ROOT=Path(__file__).resolve().parents[1]
def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source'
    follow=ROOT/'analysis/combat-presentation-audit-20261022/repair-followup-20261023'
    pins=json.loads((follow/'source-hashes.json').read_text());assert source_hashes(source)==pins
    users={p.relative_to(ROOT).as_posix() for name in ('src','include') for p in (ROOT/name).rglob('*')
        if p.is_file() and b'HALO2_SHADER_DIR' in p.read_bytes()}
    assert users=={'src/rendering/renderer/renderer_shader_directory.cpp','src/ui/menu_world_native_draw.cpp'}
    path=source/'CMakeLists.txt';before=path.read_text();root=(ROOT/'CMakeLists.txt').read_text()
    old='target_compile_definitions(halo2_renderer PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")'
    a=root.index('# Only these two owners read the fallback path.');b=root.index('target_compile_options(halo2_renderer',a)
    assert before.count(old)==1;after=before.replace(old+'\n',root[a:b]);path.write_text(after)
    updated=source_hashes(source);assert {p for p in updated if updated[p]!=pins.get(p)}=={'CMakeLists.txt'}
    (follow/'source-hashes.json').write_text(json.dumps(updated,indent=2,sort_keys=True)+'\n')
    path=follow/'diagnostic-admission.json';value=json.loads(path.read_text());value.update(build_completed=False,invalidated_by=__doc__);path.write_text(json.dumps(value,indent=2)+'\n')
    receipt=dict(scope=__doc__,unchanged_path_value='${HALO2_RUNTIME_SHADER_DIR}',actual_source_readers=sorted(users),
        before_sha256=hashlib.sha256(before.encode()).hexdigest(),after_sha256=hashlib.sha256(after.encode()).hexdigest(),
        production_unchanged=source_hashes(source)==updated,launcher_build_or_gameplay_verified=False)
    (follow/'shader-define-scope.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt),flush=True)
if __name__=='__main__':main()
