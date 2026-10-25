#!/usr/bin/env python3
"""Admit the scoped native-light pool/material join into this owned cohort."""
import hashlib,json,os
from pathlib import Path
from build_launcher_engine import source_hashes
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source'
    follow=ROOT/'analysis/combat-presentation-audit-20261022/repair-followup-20261023'
    pins=json.loads((follow/'source-hashes.json').read_text());assert source_hashes(source)==pins
    before=(source/'src/pstencil/native_lights.cpp').read_text()
    expected=before.replace('"halo2/pstencil/native_lights.hpp"','"halo2/source_native_light_snapshot_tu15.hpp"').replace(
        'namespace halo2::pstencil {','namespace halo2 {').replace(
        'NativeLightSnapshot native_lights_tu15(','SourceNativeLightSnapshotTu15 source_native_light_snapshot_tu15(').replace(
        'NativeLightSnapshot out(clusters);','SourceNativeLightSnapshotTu15 out(clusters);')
    common='src/rendering/effects/source_native_light_snapshot_tu15.cpp'
    if 'SourceNativeLightSnapshotTu15 out(clusters);' in expected:
        assert expected==(ROOT/common).read_text(), 'Common snapshot extraction changed an existing reader algorithm'
    else:
        assert (source/common).read_bytes()==(ROOT/common).read_bytes()
        assert before==(ROOT/'src/pstencil/native_lights.cpp').read_text()
    paths=['include/halo2/source_native_light_snapshot_tu15.hpp','src/rendering/effects/source_native_light_snapshot_tu15.cpp',
        'include/halo2/pstencil/native_lights.hpp','src/pstencil/native_lights.cpp',
        'include/halo2/world_effects.hpp','src/rendering/effects/world_effects.cpp',
        'include/halo2/source_bsp_effect_lights.hpp','src/rendering/world/source_bsp_effect_lights.cpp',
        'src/gameplay/multiplayer/multiplayer_runtime.cpp']
    changes={}
    for name in paths:
        path=source/name;old=sha(path) if path.exists() else None;data=(ROOT/name).read_bytes()
        if path.exists() and path.read_bytes()==data:continue
        if path.exists():path.unlink()
        path.write_bytes(data);changes[name]={'before':old,'after':sha(path)}
    path=source/'CMakeLists.txt';old=path.read_text()
    line='target_sources(halo2_systems PRIVATE src/rendering/effects/source_native_light_snapshot_tu15.cpp)\n'
    assert old.count('target_sources(halo2_systems PRIVATE src/rendering/effects/world_effects.cpp ')==1
    assert line in (ROOT/'CMakeLists.txt').read_text()
    needle='    src/rendering/renderer/source_renderer_cpu_interface_tu15.cpp)\n'
    assert old.count(needle)==1
    if line not in old:
        path.write_text(old.replace(needle,needle+line))
        changes['CMakeLists.txt']={'before':hashlib.sha256(old.encode()).hexdigest(),'after':sha(path)}
    updated=source_hashes(source)
    assert {p for p in updated if updated[p]!=pins.get(p)}==set(changes)
    (follow/'source-hashes.json').write_text(json.dumps(updated,indent=2,sort_keys=True)+'\n')
    admission=follow/'diagnostic-admission.json';value=json.loads(admission.read_text())
    value.update(build_completed=False,invalidated_by=__doc__);admission.write_text(json.dumps(value,indent=2)+'\n')
    receipt=dict(scope=__doc__,changes=changes,native_reader_algorithm_preserved=True,
        source_ready_or_admission_result_supplied=False,installed_runtime_unchanged=True)
    out=ROOT/'analysis/combat-presentation-audit-20261022/repair-native-attached-light-collection/pool-join-refresh.json'
    attempt=2
    while out.exists():out=out.with_name('pool-join-refresh-'+str(attempt)+'.json');attempt+=1
    out.write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt),flush=True)
if __name__=='__main__':main()
