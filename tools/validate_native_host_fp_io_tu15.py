#!/usr/bin/env python3
"""Prove opaque host-FP callback isolation and actual BSP stack-store transport.

The CTest/run_test.py owner supplies and removes HALO2_TEST_SCRATCH. No State
software x87/MMX image is imported or published by this callback boundary.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess

ROOT = Path(__file__).resolve().parents[1]
GUARD = 'src/physics/runtime/native_tu15_host_fp_io.hpp'
BSP = 'src/physics/runtime/native_tu15_bsp_surface_2d.hpp'
INPUTS = (
    GUARD, BSP,
    'src/physics/runtime/native_tu15_bsp_leaf_surface.hpp',
    'src/physics/runtime/native_tu15_bsp_segment.hpp',
    'src/physics/runtime/native_tu15_bsp_surface_polygon.hpp',
    'src/physics/runtime/native_tu15_raw_x87_scope.hpp',
    'src/physics/runtime/physics_world_runtime_internal.hpp',
    'src/physics/runtime/physics_step_internal.hpp',
    'tests/native_tu15_host_fp_io_tests.cpp',
    'tests/native_tu15_bsp_fp_io_address_tests.cpp',
    'tools/validate_native_host_fp_io_tu15.py',
    'cmake/NativeTu15HostFpIo.cmake',
)


def hashes():
    return {name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest() for name in INPUTS}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default='g++')
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--sanitizers', action='store_true')
    parser.add_argument('--negative-controls', action='store_true')
    args = parser.parse_args()
    if platform.machine().lower() not in ('x86_64', 'amd64', 'i386', 'i686'):
        return 77
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    report = dict(passed=False, owned_scratch=str(scratch), source_before=hashes(),
                  compiler=args.compiler, address_undefined_sanitizers=args.sanitizers,
                  commands=[], retained_large_artifacts=[])

    def execute(name, source, overlay=None):
        target = scratch / (name + '.bin')
        command = [args.compiler, '-std=c++20', '-O2', '-fno-fast-math',
                   '-ffp-contract=off', '-frounding-math', '-pthread']
        if overlay is not None:
            command += ['-I' + str(overlay / 'src')]
        command += ['-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'src')]
        if args.sanitizers:
            command += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
        command += [str(ROOT / source), '-o', str(target)]
        report['commands'].append(command)
        compiled = subprocess.run(command, capture_output=True, text=True, timeout=180)
        if compiled.returncode:
            raise ValueError(name + ' compilation failed: ' + (compiled.stdout + compiled.stderr)[-6000:])
        return subprocess.run([str(target)], capture_output=True, text=True, timeout=120)

    try:
        correct = {}
        for name, source in (
                ('host_fp_io', 'tests/native_tu15_host_fp_io_tests.cpp'),
                ('bsp_fp_io_address', 'tests/native_tu15_bsp_fp_io_address_tests.cpp')):
            result = execute(name, source)
            if result.returncode:
                raise ValueError(name + ' failed: ' + (result.stdout + result.stderr)[-6000:])
            correct[name] = json.loads(result.stdout)
            if not correct[name]['passed']:
                raise ValueError(name + ' did not report a pass')
        if correct['host_fp_io']['comparisons'] != 2016:
            raise ValueError('Incomplete host-FP topology/control/throw corpus')
        if correct['bsp_fp_io_address']['retained_native_address_capture_comparisons'] != 1:
            raise ValueError('Missing real State::push address-contract comparison')
        report['observations'] = correct
        rejected = []
        if args.negative_controls:
            guard = (ROOT / GUARD).read_text()
            bsp = (ROOT / BSP).read_text()
            mutations = (
                ('missing_return_restore', GUARD, guard,
                 '    native_raw_x87_detail::restore(image);\n#else', '\n#else',
                 'tests/native_tu15_host_fp_io_tests.cpp',
                 'FP guard changed live payload/tags/control/status/MXCSR'),
                ('missing_throw_restore', GUARD, guard,
                 'catch(...){native_raw_x87_detail::restore(image);throw;}', 'catch(...){throw;}',
                 'tests/native_tu15_host_fp_io_tests.cpp',
                 'FP guard changed live payload/tags/control/status/MXCSR'),
                ('retargeted_stack_store', BSP, bsp,
                 'state.memory.write(event.address,event.written);',
                 'state.memory.write(gp[4],event.written);',
                 'tests/native_tu15_bsp_fp_io_address_tests.cpp',
                 'Stack write followed mutated SP instead of captured address'),
            )
            for name, path, text, before, after, source, diagnostic in mutations:
                if text.count(before) != 1:
                    raise ValueError('Exact mutant seam drift: ' + name)
                overlay = scratch / name
                header = overlay / path
                header.parent.mkdir(parents=True)
                header.write_text(text.replace(before, after, 1))
                result = execute(name, source, overlay)
                if result.returncode != 1 or diagnostic not in result.stderr:
                    raise ValueError('Real defect not rejected by expected witness: ' + name + ' ' +
                                     (result.stdout + result.stderr)[-2000:])
                rejected.append(dict(name=name, exit_code=result.returncode, diagnostic=diagnostic))
        report.update(passed=True, rejected_mutants=rejected,
                      state_software_fp_or_mmx_modified=False,
                      genuine_child_calls_inside_guard=False)
    except BaseException as error:
        report['error'] = str(error) or repr(error)
        raise
    finally:
        report['source_after'] = hashes()
        if report['source_after'] != report['source_before']:
            report.update(passed=False, source_drift=True)
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS host FP I/O: 2016 images, actual stack address contract, ' + str(len(rejected)) + ' mutants')
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
