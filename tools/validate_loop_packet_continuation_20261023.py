#!/usr/bin/env python3
"""Verify actual loop-packet PCM/receipts across native Audio continuation."""
import hashlib,json,os,subprocess,sys
from pathlib import Path
from build_launcher_engine import source_hashes
ROOT=Path(__file__).resolve().parents[1]
def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source'
    base=ROOT/'analysis/combat-presentation-audit-20261022';follow=base/'repair-followup-20261023'
    pins=json.loads((follow/'source-hashes.json').read_text());assert source_hashes(source)==pins
    rel='tests/multiplayer_audio_continuation_tests.cpp';data=(ROOT/rel).read_bytes();(source/rel).write_bytes(data)
    maps=ROOT/'assets/imported/mcc-dire-valley-menu-20261023/halo2/h2_maps_win64_dx11'
    out=base/'repair-scenery-audio/loop-packet-continuation';attempt=2
    while out.exists():out=out.with_name('loop-packet-continuation-'+str(attempt));attempt+=1
    out.mkdir()
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
        '--target','multiplayer_audio_continuation_tests',
        '--filter','^(native_audio_core_owner_preflight|native_mcc_audio_core_enrollment)$',
        '--cmake-arg=-DHALO2_MCC_CORE_AUDIO_TEST_MAP='+str(maps/'lockout.map'),
        '--jobs','4','--test-jobs','1','--output',str(out)]
    with (out/'command.log').open('w') as log:code=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT).returncode
    receipt=dict(scope=__doc__,command=command,exit_code=code,test_sha256=hashlib.sha256(data).hexdigest(),
        production_source_unchanged=source_hashes(source)==pins,launcher_gameplay_verified=False,
        complete_object_loop_scheduler_verified=False)
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt),flush=True)
    return code
if __name__=='__main__':raise SystemExit(main())
