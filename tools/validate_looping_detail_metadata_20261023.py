#!/usr/bin/env python3
"""Test resident detail-only ambience against the real imported and original maps."""
import json,os,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source'
    (source/'tests/mcc_looping_sound_tests.cpp').write_bytes((ROOT/'tests/mcc_looping_sound_tests.cpp').read_bytes())
    base=ROOT/'analysis/combat-presentation-audit-20261022/repair-scenery-audio'
    out=base/'detail-metadata';attempt=2
    while out.exists():out=base/f'detail-metadata-{attempt}';attempt+=1
    out.mkdir()
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
        '--target','mcc_looping_sound_tests','--filter','^(mcc|xbox)_looping_sound_layout$',
        '--cmake-arg=-DHALO2_MCC_LOOPING_SOUND_DONOR='+str(ROOT/'assets/imported/mcc-dire-valley-menu-20261023/halo2/h2_maps_win64_dx11/coagulation.map'),
        '--jobs','4','--test-jobs','1','--output',str(out)]
    with (out/'command.log').open('w') as log:code=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT).returncode
    print(json.dumps(dict(command=command,exit_code=code)),flush=True);return code
if __name__=='__main__':raise SystemExit(main())
