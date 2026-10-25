#!/usr/bin/env python3
"""Build and verify actual Random vehicle births in one authenticated native source epoch."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
from weapon_test_cache import sha

ROOT=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prepared',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    prepared=json.loads(args.prepared.read_text());source=owner/'source'
    if Path(prepared['owner'])!=owner:raise ValueError('Use the same active source/build owner')
    pins=prepared['source_hashes'];unchanged=lambda:all(sha(source/name)==pin for name,pin in pins.items())
    if not unchanged():raise ValueError('Source changed before native Random validation')
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    families=prepared.get('runtime_cases',['light','heavy','turret'])
    command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
        '--target','halo2_native','--target','vehicle_choice_runtime_tu15_tests',
        '--filter','^('+'|'.join(prepared['tests'])+')$','--jobs','2','--test-jobs','1',
        '--cmake-arg=-DFETCHCONTENT_SOURCE_DIR_HALO2_DATACHANNEL='+prepared['peer_copy'],
        '--cmake-arg=-DCMAKE_CXX_FLAGS_RELEASE=-O2 -DNDEBUG','--output',str(args.output)]
    for family in families:command+=['--retain-report','vehicle-choice-'+family+'.json']
    for target in prepared.get('extra_targets',[]):command+=['--target',target]
    command+=['--retain-report','Testing/Temporary/LastTest.log']
    (args.output/'source-binding.json').write_text(json.dumps(dict(owner=str(owner),
        preparation_sha256=sha(args.prepared),source_files=len(pins),command=command,full_vehicle_parity=False),indent=2)+'\n')
    result=subprocess.run(command,env=dict(os.environ,CARGO_BUILD_JOBS='2'),
        stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT).returncode
    report=json.loads((args.output/'report.json').read_text());cases=[]
    for family in families:
        path=args.output/'evidence'/('vehicle-choice-'+family+'.json')
        if path.exists():cases.append(json.loads(path.read_text()))
    stable=unchanged();passed=result==0 and stable and len(cases)==len(families) and all(row['passed']for row in cases)
    runtime=owner/'build/halo2_native'
    report.update(passed=passed,vehicle_choice_source_binding=dict(preparation_sha256=sha(args.prepared),
        source_unchanged_after_run=stable,cases=cases,runtime_sha256=sha(runtime)if runtime.exists()else None,
        native_XBE_execution=False,rendered_game_verified=False,launcher_installed=False,full_vehicle_parity=False))
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=passed,cases=len(cases),report=str(args.output/'report.json'))),flush=True)
    return 0 if passed else result or 1


if __name__=='__main__':raise SystemExit(main())
