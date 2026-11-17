#!/usr/bin/env python3
"""Run both installed E0900 public APIs from an independent frozen donor."""
import argparse
import json
import os
from pathlib import Path
import re
import shutil
import signal
import subprocess
import sys
import time

from build_launcher_engine import freeze_source, source_hashes
from collect_native_production_closure import prepare_peer_source
from native_metadata import check_budget, load, retain_bytes
from validate_native_replacement_snapshot import auxiliary_source_hashes, freeze_test_code, test_code_hashes

ROOT=Path(__file__).resolve().parents[1]
STABLE=(
    'src/gameplay/objects/native_tu15_flight_property_query_source_frame.hpp',
    'src/gameplay/objects/native_tu15_flight_property_query_source_owner.hpp',
    'src/gameplay/objects/native_tu15_death_cycle_flight_query_source_owner.hpp',
    'include/halo2/native_tu15_flight_property_query_kernel.hpp',
    'src/gameplay/units/death_cycle_runtime_tu15_generated.cpp',
    'tests/native_tu15_flight_property_query_public_tests.cpp',
    'tests/helpers/model_skin_gpu_admission_reference_20261023.hpp',
    'tools/validate_native_flight_property_query_public_tu15.py',
    'src/rendering/effects/source_effect_compiled_symbols_tu15.cpp',
    'src/rendering/effects/source_object_function_symbols_tu15.inc',
)


def inventory(root):
    return {**source_hashes(root),**test_code_hashes(root),**auxiliary_source_hashes(root)}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--donor',type=Path,required=True)
    parser.add_argument('--previous-components',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--xbe',type=Path,required=True)
    a=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    donor=a.donor.resolve();out=a.output.resolve();source=scratch/'source'
    out.mkdir(parents=True,exist_ok=False)
    report=dict(passed=False,status='copying',owned_scratch=str(scratch),retained_large_artifacts=[])
    def save():(out/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    def interrupted(number,_frame):raise SystemExit(128+number)
    for number in (signal.SIGINT,signal.SIGTERM):signal.signal(number,interrupted)
    save()
    try:
        before=inventory(donor)
        freeze_source(donor,source);freeze_test_code(donor,source)
        for path in sorted(source.rglob('*')):
            if not path.is_symlink():continue
            target=path.resolve(strict=True);path.unlink()
            if target.is_relative_to(donor):
                if target.is_dir():raise ValueError('Borrowed donor directory: '+str(path))
                shutil.copy2(target,path)
            else:path.symlink_to(target,target_is_directory=target.is_dir())
        if inventory(source)!=before or inventory(donor)!=before:
            raise ValueError('Donor code changed or incomplete test copy')
        old=load(a.previous_components/'source-hashes.json')
        for name in STABLE:
            if before.get(name)!=old.get(name)or name not in old:
                raise ValueError('E0900 public input differs from prior components: '+name)
        if (source/STABLE[6]).is_symlink():
            raise ValueError('Required CMake test code must be independently owned')
        inventory_data=(json.dumps(before,indent=2,sort_keys=True)+'\n').encode()
        stored=retain_bytes(inventory_data,out/'source-hashes.json',out)
        report.update(status='building-linked-public',source=str(source),source_manifest_sha256=stored['raw_sha256'],
            preceding_component_manifest_sha256=json.loads((a.previous_components/'report.json').read_text())['source_manifest_sha256'],
            exact_prior_public_inputs={name:before[name]for name in STABLE},
            source_differences_from_prior=[n for n in sorted(before.keys()|old.keys())if before.get(n)!=old.get(n)])
        save();print('E0900_PUBLIC_SNAPSHOT_READY',stored['raw_sha256'],scratch,flush=True)
        peer,_=prepare_peer_source(ROOT/'build-perf-4k60/_seed/halo2_datachannel-src',scratch)
        cmd=[sys.executable,'-B',str(source/'tools/run_tests.py'),'--build-worker','--source',str(source),
            '--output',str(out/'cmake'),'--jobs','2','--test-jobs','2','--target','native_flight_property_query_tests',
            '--filter','^native_(flight_property|death_cycle_flight)_query_public$',
            '--cmake-arg=-DFETCHCONTENT_SOURCE_DIR_HALO2_DATACHANNEL='+str(peer),
            '--cmake-arg=-DFETCHCONTENT_FULLY_DISCONNECTED=ON',
            '--cmake-arg=-DHALO2_TEST_ASSET_SOURCE_ROOT='+str(ROOT),
            '--cmake-arg=-DHALO2_TU15_GAMEPLAY_XBE='+str(a.xbe.resolve()),
            '--retain-report','native-flight-property-query-public/report.json',
            '--retain-report','native-death-cycle-flight-query-public/report.json']
        report['command']=cmd;save()
        started=time.monotonic();process=subprocess.Popen(cmd,cwd=source)
        try:
            while True:
                try:code=process.wait(timeout=30);break
                except subprocess.TimeoutExpired:
                    print(json.dumps({'E0900_public':'running','seconds':int(time.monotonic()-started)}),flush=True)
        finally:
            if process.poll()is None:
                process.terminate()
                try:process.wait(timeout=2)
                except subprocess.TimeoutExpired:process.kill();process.wait()
        report['cmake_exit_code']=code
        if code:raise ValueError('Actual E0900 linked checks failed')
        log=(out/'cmake/tests.log').read_text()
        if not re.search(r'100% tests passed[^\n]*out of 2\b',log)or'***Skipped'in log:
            raise ValueError('Expected two complete E0900 public checks')
        if inventory(source)!=before:raise ValueError('Frozen E0900 source changed')
        report.update(status='passed',passed=True,linked_public_passed=2,source_test_aux_unchanged=True)
    except SystemExit as error:
        report.update(status='interrupted',exit_code=error.code);raise
    except Exception as error:
        report.update(status='failed',error=repr(error));raise
    finally:
        for p in out.rglob('*.log'):
            if p.stat().st_size>300000:
                with p.open('rb')as stream:stream.seek(-300000,os.SEEK_END);tail=stream.read()
                p.write_bytes(tail)
        report['retained_metadata_bytes']=check_budget(out);save()
    return 0


if __name__=='__main__':raise SystemExit(main())
