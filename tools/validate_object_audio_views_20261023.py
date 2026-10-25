#!/usr/bin/env python3
"""Qualify the camera seam against complete original CPU results and scoped generation."""
import hashlib,json,os,subprocess,sys
from pathlib import Path
from build_launcher_engine import source_hashes
from object_audio_views_tu15 import bind_native_camera_inputs,SITES
ROOT=Path(__file__).resolve().parents[1]
def sha(data):return hashlib.sha256(data).hexdigest()
def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source'
    base=ROOT/'analysis/combat-presentation-audit-20261022';follow=base/'repair-followup-20261023'
    pins=json.loads((follow/'source-hashes.json').read_text());assert source_hashes(source)==pins
    module=(ROOT/'src/audio/source_object_loop_audio_tu15_generated.cpp').read_text();original=module
    original=original.replace('    const SourceObjectAudioViewsTu15* native_audio_views{};\n','')
    a=original.index('std::uint32_t audio_view_read(');b=original.index('\n}\n',a)+3
    original=original[:a]+original[b:]
    original=original.replace('    const SourcePhysicsContactRuntimeServices &services,const O &observe,\n    const SourceObjectAudioViewsTu15* native_views) {',
        '    const SourcePhysicsContactRuntimeServices &services,const O &observe) {')
    original=original.replace('    s.native_audio_views=native_views;\n','')
    for pc,operand,size in SITES:
        needle=f'audio_view_read(s,0x{pc:x}u,({operand}),{size})';assert original.count(needle)==1
        original=original.replace(needle,f'r(s.memory,({operand}),{size})')
    assert sha(original.encode())=='e0b06ae4cbb7da6040a359b7b55ad185b66ad535377b8c49182d34f659af8fb4'
    assert bind_native_camera_inputs(original)==module
    changes={}
    def save(rel,data):
        path=source/rel;before=path.read_bytes() if path.exists() else b''
        if before!=data:
            path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data)
            changes[rel]=dict(before=sha(before) if before else None,after=sha(data))
    for rel in ('tests/source_object_audio_views_tu15_tests.cpp','tests/fixtures/object_audio_views_tu15.json'):
        save(rel,(ROOT/rel).read_bytes())
    rel='CMakeLists.txt';old=(source/rel).read_text();root=(ROOT/rel).read_text()
    a=root.index('    add_executable(source_object_audio_views_tu15_tests ');b=root.index('\nendif()',a)
    anchor='        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/loop_track_transition_tu15.json")'
    assert old.count(anchor)==1
    if '    add_executable(source_object_audio_views_tu15_tests ' not in old:
        save(rel,old.replace(anchor,anchor+'\n'+root[a:b]).encode())
    else:assert root[a:b] in old
    updated=source_hashes(source)
    assert {p for p in updated if updated[p]!=pins.get(p)}=={p for p in changes if p in updated}
    (follow/'source-hashes.json').write_text(json.dumps(updated,indent=2,sort_keys=True)+'\n')
    admission_path=follow/'diagnostic-admission.json';admission=json.loads(admission_path.read_text())
    admission['build_completed']=False;admission['invalidated_by']='native camera regression registration';admission_path.write_text(json.dumps(admission,indent=2)+'\n')
    out=base/'repair-scenery-audio/native-views-validation';attempt=2
    while out.exists():out=out.with_name('native-views-validation-'+str(attempt));attempt+=1
    out.mkdir()
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
        '--target','source_object_audio_views_tu15_tests','--target','source_loop_track_transition_tu15_tests',
        '--filter','^original_(object_audio_native_views|loop_track_transition)_tu15$',
        '--jobs','4','--test-jobs','1','--output',str(out)]
    with (out/'command.log').open('w') as log:code=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT).returncode
    receipt=dict(scope=__doc__,exit_code=code,command=command,changes=changes,
        preceding_module_sha256=sha(original.encode()),module_sha256=sha(module.encode()),
        original_other_instructions_unchanged=True,scoped_regeneration_matches=True,
        source_listener_bank_or_readiness_supplied=False,native_PCM_verified=False,
        frozen_source_unchanged=source_hashes(source)==updated)
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt),flush=True)
    return code
if __name__=='__main__':raise SystemExit(main())
