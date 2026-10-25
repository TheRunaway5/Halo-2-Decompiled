#!/usr/bin/env python3
"""Qualify the missing canonical lsnd root projection against real MCC caches."""
import hashlib,json,os,subprocess,sys
from pathlib import Path
from build_launcher_engine import source_hashes
ROOT=Path(__file__).resolve().parents[1]
def sha(data):return hashlib.sha256(data).hexdigest()
def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source'
    base=ROOT/'analysis/combat-presentation-audit-20261022';follow=base/'repair-followup-20261023'
    pins=json.loads((follow/'source-hashes.json').read_text());assert source_hashes(source)==pins;changes={}
    def save(rel,data):
        path=source/rel;before=path.read_bytes();
        if before==data:return
        path.write_bytes(data);changes[rel]=dict(before=sha(before),after=sha(data))
    rel='include/halo2/mcc_sound_tag_layout.hpp';before=(source/rel).read_text();root=(ROOT/rel).read_text()
    anchor='std::array<std::byte,0x24> project_mcc_sound_globals(std::span<const std::byte>);'
    a=root.index('\n// lsnd prepends',root.index(anchor));b=root.index('\nenum class MccSoundLayoutNeed',a)
    assert before.count(anchor)==1
    if 'project_mcc_looping_sound_root' not in before:save(rel,before.replace(anchor,anchor+root[a:b]).encode())
    rel='src/audio/mcc_sound_tag_layout.cpp';before=(source/rel).read_text();root=(ROOT/rel).read_text()
    a=root.index('std::array<std::byte,0x2c> project_mcc_looping_sound_root(');b=root.index('\nbool MccSoundDefinitionLayout::',a)
    anchor='bool MccSoundDefinitionLayout::original_adpcm_admission() const {'
    assert before.count(anchor)==1
    if 'project_mcc_looping_sound_root' not in before:save(rel,before.replace(anchor,root[a:b]+'\n'+anchor).encode())
    rel='src/runtime/mcc_runtime_delivery.cpp';before=(source/rel).read_text();root=(ROOT/rel).read_text()
    a=root.index('    // Original object/audio callers read lsnd+0 flags,');b=root.index('    // The source binder follows matg+C0/C4',a)
    anchor='    // The source binder follows matg+C0/C4'
    assert before.count(anchor)==1
    if 'project_mcc_looping_sound_root' not in before:save(rel,before.replace(anchor,root[a:b]+anchor).encode())
    for rel in ('tests/mcc_runtime_delivery_tests.cpp','tests/mcc_sound_tag_layout_tests.cpp'):
        save(rel,(ROOT/rel).read_bytes())
    updated=source_hashes(source)
    assert {p for p in updated if updated[p]!=pins.get(p)}=={p for p in changes if p in updated}
    (follow/'source-hashes.json').write_text(json.dumps(updated,indent=2,sort_keys=True)+'\n')
    path=follow/'diagnostic-admission.json';admission=json.loads(path.read_text());admission['build_completed']=False
    admission['invalidated_by']=__doc__;path.write_text(json.dumps(admission,indent=2)+'\n')
    out=base/'repair-scenery-audio/loop-root-layout-validation';attempt=2
    while out.exists():out=out.with_name('loop-root-layout-validation-'+str(attempt));attempt+=1
    out.mkdir()
    maps=ROOT/'assets/imported/mcc-dire-valley-menu-20261023/halo2/h2_maps_win64_dx11'
    cmake=source/'CMakeLists.txt';saved=cmake.read_bytes()
    cmake.write_bytes(saved+('''
add_custom_target(loop_root_layout_validation DEPENDS mcc_sound_tag_layout_tests
    mcc_runtime_delivery_tests mcc_sound_runtime_delivery_tests mcc_sound_pipeline_coherence_tests)
add_test(NAME actual_loop_root_layout_coag COMMAND mcc_runtime_delivery_tests "'''+str(maps/'coagulation.map')+'''")
add_test(NAME actual_loop_root_sound_layout COMMAND mcc_sound_tag_layout_tests "'''+str(maps/'coagulation.map')+'''")
''').encode())
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
        '--target','loop_root_layout_validation','--filter','^(actual_loop_root_.*|mcc_runtime_delivery_contract|mcc_sound_tag_layouts|mcc_sound_runtime_delivery_contract|mcc_sound_pipeline_coherence_mcc_only)$',
        '--cmake-arg=-DHALO2_MCC_SOUND_PIPELINE_MAP='+str(maps/'lockout.map'),
        '--jobs','6','--test-jobs','1','--output',str(out)]
    try:
        with (out/'command.log').open('w') as log:code=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT).returncode
    finally:cmake.write_bytes(saved)
    receipt=dict(scope=__doc__,changes=changes,command=command,exit_code=code,
        source_voice_or_readiness_results_supplied=False,launcher_gameplay_verified=False,
        frozen_source_unchanged=source_hashes(source)==updated)
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt),flush=True)
    return code
if __name__=='__main__':raise SystemExit(main())
