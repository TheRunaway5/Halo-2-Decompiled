#!/usr/bin/env python3
"""Validate the reached scenery angle owner, including its missing-owner control."""
import hashlib,json,os,subprocess,sys
from pathlib import Path
from build_launcher_engine import source_hashes
ROOT=Path(__file__).resolve().parents[1]
def sha(data):return hashlib.sha256(data).hexdigest()
def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source'
    base=ROOT/'analysis/combat-presentation-audit-20261022';follow=base/'repair-followup-20261023'
    pins=json.loads((follow/'source-hashes.json').read_text());assert source_hashes(source)==pins
    baseline='--baseline' in sys.argv;changes={}
    def save(relative,data):
        path=source/relative;before=path.read_bytes() if path.exists() else None
        if data==before:return
        path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data)
        changes[relative]=dict(before=sha(before) if before else None,after=sha(data))
    save('tests/scenery_audio_angle_tu15_tests.cpp',(ROOT/'tests/scenery_audio_angle_tu15_tests.cpp').read_bytes())
    relative='CMakeLists.txt';text=(source/relative).read_text();root=(ROOT/relative).read_text()
    a=root.index('    add_executable(scenery_audio_angle_tu15_tests ');b=root.index('    add_executable(source_object_audio_views_tu15_tests ',a)
    if '    add_executable(scenery_audio_angle_tu15_tests ' not in text:
        anchor='    add_executable(source_object_audio_views_tu15_tests ';assert text.count(anchor)==1
        save(relative,text.replace(anchor,root[a:b]+anchor).encode())
    else:assert root[a:b] in text
    if not baseline:
        for relative in ('src/audio/source_object_loop_audio_tu15_generated.cpp','src/platform/native_tu15_world_memory.cpp'):
            save(relative,(ROOT/relative).read_bytes())
        save('tests/native_tu15_world_memory_tests.cpp',(ROOT/'tests/native_tu15_world_memory_tests.cpp').read_bytes())
    updated=source_hashes(source)
    assert {p for p in updated if updated[p]!=pins.get(p)}=={p for p in changes if p in updated}
    (follow/'source-hashes.json').write_text(json.dumps(updated,indent=2,sort_keys=True)+'\n')
    path=follow/'diagnostic-admission.json';admission=json.loads(path.read_text());admission.update(build_completed=False,invalidated_by=__doc__);path.write_text(json.dumps(admission,indent=2)+'\n')
    out=base/'repair-scenery-audio'/('scenery-angle-red' if baseline else 'scenery-angle-native');out.mkdir(exist_ok=False)
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
        '--target','scenery_audio_angle_tu15_tests',
        *([] if baseline else ['--target','native_tu15_world_memory_tests']),
        '--filter','^original_scenery_audio_angle_tu15$' if baseline else '^(original_scenery_audio_angle_tu15|native_tu15_world_memory_ownership)$',
        '--jobs','3','--test-jobs','1','--output',str(out)]
    with (out/'command.log').open('w') as log:code=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT).returncode
    tests=(out/'tests.log').read_text() if (out/'tests.log').exists() else ''
    passed=bool(code and 'Reached sound-scenery18B580 owner is absent' in tests) if baseline else code==0
    receipt=dict(scope=__doc__,command=command,exit_code=code,expected_missing_owner_control=baseline,passed=passed,
        changes=changes,production_unchanged=source_hashes(source)==updated,source_result_supplied=False,
        gameplay_verified=False,fixture_sha256=sha((ROOT/'tests/fixtures/scenery_audio_angle_tu15.json').read_bytes()))
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt),flush=True)
    return 0 if passed else 1
if __name__=='__main__':raise SystemExit(main())
