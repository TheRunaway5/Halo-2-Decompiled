#!/usr/bin/env python3
"""Test-only backed World synchronous resource policy; not controller closure."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import random
import subprocess
from native_resource_composition_inputs import freeze_resource_registry,ROOT
FILES=('tests/native_tu15_resource_composition_backend.hpp','tests/native_tu15_resource_composition_backend_tests.cpp',
       'tools/native_resource_composition_inputs.py','tools/validate_native_resource_composition_backend.py',
       'include/halo2/source_animation_cache_io_tu15.hpp','src/rendering/source_bitmap_async_read.cpp','src/platform/source_async_jobs.cpp','src/assets/cache.cpp')
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path,required=True);parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--leak-sanitizer',action='store_true');args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);registry=freeze_resource_registry(scratch);pins={name:sha(ROOT/name)for name in FILES}
    path=scratch/'actual-cache-sectors.bin';path.write_bytes(random.Random(0x1260c0).randbytes(8192));binary=scratch/'resource-backend'
    command=['c++','-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-I'+str(ROOT/'include'),'-I'+str(ROOT/'tests'),'-I'+str(scratch),str(ROOT/FILES[1]),*[str(ROOT/name)for name in('src/assets/cache.cpp','src/rendering/source_bitmap_async_read.cpp','src/platform/source_async_jobs.cpp')],'-o',str(binary)]
    if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    subprocess.run(command,check=True)
    environment=dict(os.environ)
    if args.sanitizers:
        # The sandbox cannot grant LeakSanitizer's process-inspection ptrace.
        # Address/undefined checks remain enabled; leak acceptance is separate.
        environment['ASAN_OPTIONS']=environment.get('ASAN_OPTIONS','')+(':detect_leaks=1'if args.leak_sanitizer else':detect_leaks=0')
    result=subprocess.run([str(binary),str(path)],capture_output=True,text=True,env=environment)
    if result.returncode:raise ValueError('Backed resource policy probe failed: '+(result.stdout+result.stderr)[-4000:])
    if any(sha(ROOT/name)!=pin for name,pin in pins.items()):raise ValueError('Resource backend seam sources changed')
    report=dict(passed=True,source_registry=registry,source_sha256=pins,cases=9,stdout=result.stdout.strip(),compile_command=command,
        shared_production_synchronous_io_policy=True,actual_owned_file_and_native_read_algorithm=True,retained_job_outcomes_supplied=False,
        live_world_instance_or_epoch_qualified=False,pending_worker_policy_qualified=False,resource_controller_composition_qualified=False,
        physical_cpu_abi_qualified=False,leak_detection_qualified=args.sanitizers and args.leak_sanitizer,scratch_owner=str(scratch),retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(result.stdout.strip());return 0
if __name__=='__main__':raise SystemExit(main())
