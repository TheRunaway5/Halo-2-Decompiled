#!/usr/bin/env python3
"""Build and run the actual PC sky material audit inside a run_test.py owner."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import time


ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--frozen-source', type=Path, required=True)
    parser.add_argument('--archive-sha256')
    parser.add_argument('--wait-for-pin', action='store_true')
    parser.add_argument('--prepared-root', type=Path, required=True)
    parser.add_argument('--report-output', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    build = args.build.resolve(strict=True)
    source = args.frozen_source.resolve(strict=True)
    archive = build/'libhalo2_cache.a'
    command = shlex.split((build/'CMakeFiles/mcc_transparent_material_tests.dir/link.txt').read_text())
    executable = scratch/'audit_pc_sky_materials'
    command = [value for value in command if not value.startswith('-Wl,--dependency-file=')]
    command = [str((build/value).resolve()) if value.endswith('.a') and not Path(value).is_absolute() else value for value in command]
    obj = scratch/'audit_pc_sky_materials.o'
    command[command.index('CMakeFiles/mcc_transparent_material_tests.dir/tests/mcc_transparent_material_tests.cpp.o')] = str(obj)
    command[command.index('-o') + 1] = str(executable)
    compile_command = [command[0], '-O3', '-DNDEBUG', '-std=c++20', '-I'+str(source/'include'), '-I'+str(source/'src'), '-c', str(ROOT/'tools/audit_pc_sky_materials.cpp'), '-o', str(obj)]
    compile_result = subprocess.run(compile_command, cwd=scratch, check=False)
    if compile_result.returncode:
        return compile_result.returncode
    if args.wait_for_pin:
        pin_path = scratch/'archive-pin.json'
        print('PC_SKY_AUDIT_OBJECT_READY '+str(pin_path), flush=True)
        deadline = time.monotonic() + 600
        while not pin_path.exists():
            if time.monotonic() >= deadline:
                raise TimeoutError('Native Cache archive read lease was not supplied')
            time.sleep(.1)
        args.archive_sha256 = json.loads(pin_path.read_text())['sha256']
    assert args.archive_sha256 and sha(archive) == args.archive_sha256
    linked = subprocess.run(command, cwd=scratch, check=False)
    assert sha(archive) == args.archive_sha256
    if linked.returncode:
        return linked.returncode
    print('PC_SKY_AUDIT_LINKED_ARCHIVE_LEASE_CAN_RELEASE', flush=True)
    output = scratch/'pc-sky-materials.json'
    result = subprocess.run([str(executable), str(ROOT), str(args.prepared_root.resolve(strict=True)), str(output)], cwd=scratch, check=False)
    raw = output.read_bytes()
    assert len(raw) <= 1024 * 1024
    report = json.loads(raw)
    report['build_evidence'] = dict(cache_archive_sha256=args.archive_sha256,
        audit_source_sha256=sha(ROOT/'tools/audit_pc_sky_materials.cpp'),
        wrapper_sha256=sha(Path(__file__)), semantic_header_sha256=sha(source/'include/halo2/sky_input_identity.hpp'),
        executable_sha256=sha(executable), compile_command=compile_command, command=command, scratch=str(scratch))
    inputs = [ROOT/'assets/map-pack/maps/dune.map', ROOT/'assets/disc/maps/03a_oldmombasa.map',
              args.prepared_root/'halo2/h2_maps_win64_dx11/needle.map', args.prepared_root/'halo2/h2_maps_win64_dx11/street_sweeper.map']
    report['input_files'] = [dict(path=str(path.resolve(strict=True)), bytes=path.stat().st_size, sha256=sha(path)) for path in inputs]
    encoded = (json.dumps(report, indent=2) + '\n').encode()
    assert len(encoded) <= 1024 * 1024
    args.report_output.parent.mkdir(parents=True, exist_ok=True)
    args.report_output.write_bytes(encoded)
    return result.returncode


if __name__ == '__main__':
    raise SystemExit(main())
