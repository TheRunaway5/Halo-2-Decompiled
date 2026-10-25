#!/usr/bin/env python3
"""Recheck the existing actual Ghost right-click boost owner after intent repair."""
import hashlib,json,os,subprocess,sys
from pathlib import Path
from build_launcher_engine import source_hashes
ROOT=Path(__file__).resolve().parents[1]
def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source'
    base=ROOT/'analysis/combat-presentation-audit-20261022';follow=base/'repair-followup-20261023'
    pins=json.loads((follow/'source-hashes.json').read_text());assert source_hashes(source)==pins
    out=base/'repair-scenery-audio/vehicle-input-compatibility';out.mkdir(exist_ok=False)
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
        '--target','vehicle_weapon_hud_world_tests','--filter','^native_mcc_ghost_right_click_boost$',
        '--jobs','2','--test-jobs','1','--retain-report','ghost-boost.json','--output',str(out)]
    with (out/'command.log').open('w') as log:code=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT).returncode
    receipt=dict(scope=__doc__,command=command,exit_code=code,production_unchanged=source_hashes(source)==pins,
        fixture_sha256=hashlib.sha256((source/'tests/vehicle_weapon_hud_world_tests.cpp').read_bytes()).hexdigest(),
        source_boost_state_or_result_supplied=False,launcher_gameplay_verified=False)
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt),flush=True);return code
if __name__=='__main__':raise SystemExit(main())
