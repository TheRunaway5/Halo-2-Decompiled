#!/usr/bin/env python3
"""Compare native64 ring edits with independent original CPU fixture results."""
import hashlib,json,os,subprocess,sys
from pathlib import Path
from build_launcher_engine import source_hashes
ROOT=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source'
    base=ROOT/'analysis/combat-presentation-audit-20261022';follow=base/'repair-followup-20261023'
    pins=json.loads((follow/'source-hashes.json').read_text());assert source_hashes(source)==pins;changes={}
    for rel in ('include/halo2/source_loop_phased_output_tu15.hpp','src/audio/source_loop_phased_output_tu15.cpp',
            'tests/source_loop_voice_ring_tu15_tests.cpp'):
        path=source/rel;before=path.read_bytes() if path.exists() else None;data=(ROOT/rel).read_bytes()
        if before!=data:
            path.write_bytes(data);changes[rel]=dict(before=hashlib.sha256(before).hexdigest() if before else None,after=sha(path))
    rel='cmake/MultiplayerAudioContinuation.cmake';before=(source/rel).read_text();root=(ROOT/rel).read_text()
    if 'add_executable(source_loop_voice_ring_tu15_tests' not in before:
        a=root.index('    add_executable(source_loop_voice_ring_tu15_tests');b=root.index('    add_executable(multiplayer_audio_loop_phase_tests',a)
        anchor='    add_executable(multiplayer_audio_loop_phase_tests';assert before.count(anchor)==1
        path=source/rel;old=sha(path);path.write_text(before.replace(anchor,root[a:b]+anchor));changes[rel]=dict(before=old,after=sha(path))
    updated=source_hashes(source);assert {p for p in updated if updated[p]!=pins.get(p)}=={p for p in changes if p in updated}
    (follow/'source-hashes.json').write_text(json.dumps(updated,indent=2,sort_keys=True)+'\n')
    path=follow/'diagnostic-admission.json';admission=json.loads(path.read_text());admission['build_completed']=False
    admission['invalidated_by']=__doc__;path.write_text(json.dumps(admission,indent=2)+'\n')
    out=base/'repair-scenery-audio/loop-voice-ring';attempt=2
    while out.exists():out=out.with_name('loop-voice-ring-'+str(attempt));attempt+=1
    out.mkdir();fixture=ROOT/'tests/fixtures/loop_voice_ring_tu15.json';pin=sha(fixture)
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
        '--target','source_loop_voice_ring_tu15_tests','--filter','^original_loop_voice_ring_tu15$',
        '--jobs','4','--test-jobs','1','--output',str(out)]
    with (out/'command.log').open('w') as log:code=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT).returncode
    receipt=dict(scope=__doc__,changes=changes,command=command,exit_code=code,fixture_sha256=pin,
        fixture_unchanged=sha(fixture)==pin,production_unchanged=source_hashes(source)==updated,
        source_readiness_or_admission_results_supplied=False,live_gameplay_verified=False)
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt),flush=True);return code
if __name__=='__main__':raise SystemExit(main())
