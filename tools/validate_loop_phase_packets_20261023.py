#!/usr/bin/env python3
"""Qualify actual single-stream Banshee phase PCM, separately from its controller."""
import hashlib,json,os,subprocess,sys
from pathlib import Path
from build_launcher_engine import source_hashes
ROOT=Path(__file__).resolve().parents[1]
def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source'
    base=ROOT/'analysis/combat-presentation-audit-20261022';follow=base/'repair-followup-20261023'
    pins=json.loads((follow/'source-hashes.json').read_text());assert source_hashes(source)==pins;changes={}
    def save(rel,data):
        path=source/rel;before=path.read_bytes() if path.exists() else b''
        if before==data:return
        path.write_bytes(data);changes[rel]=dict(before=hashlib.sha256(before).hexdigest() if before else None,
            after=hashlib.sha256(data).hexdigest())
    for rel in ('src/audio/multiplayer_audio.cpp','include/halo2/multiplayer_audio.hpp',
            'tests/multiplayer_audio_loop_phase_tests.cpp'):save(rel,(ROOT/rel).read_bytes())
    rel='cmake/MultiplayerAudioContinuation.cmake';before=(source/rel).read_text();root=(ROOT/rel).read_text()
    if 'add_executable(multiplayer_audio_loop_phase_tests' not in before:
        a=root.index('    add_executable(multiplayer_audio_loop_phase_tests');b=root.index('    add_executable(multiplayer_audio_continuation_tests',a)
        anchor='    add_executable(multiplayer_audio_continuation_tests';assert before.count(anchor)==1
        save(rel,before.replace(anchor,root[a:b]+anchor).encode())
    updated=source_hashes(source);assert {p for p in updated if updated[p]!=pins.get(p)}=={p for p in changes if p in updated}
    (follow/'source-hashes.json').write_text(json.dumps(updated,indent=2,sort_keys=True)+'\n')
    path=follow/'diagnostic-admission.json';admission=json.loads(path.read_text());admission['build_completed']=False
    admission['invalidated_by']=__doc__;path.write_text(json.dumps(admission,indent=2)+'\n')
    out=base/'repair-scenery-audio/loop-phase-packets';attempt=2
    while out.exists():out=out.with_name('loop-phase-packets-'+str(attempt));attempt+=1
    out.mkdir();maps=ROOT/'assets/imported/mcc-dire-valley-menu-20261023/halo2/h2_maps_win64_dx11'
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
        '--target','multiplayer_audio_loop_phase_tests','--filter','^native_mcc_loop_phase_packets$',
        '--cmake-arg=-DHALO2_MCC_LOOP_PHASE_MAP='+str(maps/'coagulation.map'),
        '--jobs','4','--test-jobs','1','--output',str(out)]
    with (out/'command.log').open('w') as log:code=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT).returncode
    receipt=dict(scope=__doc__,changes=changes,command=command,exit_code=code,
        production_unchanged=source_hashes(source)==updated,full_native_loop_controller_verified=False,
        launcher_gameplay_verified=False)
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt),flush=True);return code
if __name__=='__main__':raise SystemExit(main())
