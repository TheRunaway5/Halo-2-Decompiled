#!/usr/bin/env python3
"""Validate the native TU1.5 wheel stages and their retained routes."""

import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import sys

import native_vehicle_contact_output_20f6e0_replacements as contact_output
import native_vehicle_contact_projection_20ee92_replacements as projection
import native_vehicle_wheel_cross_product_20e481_replacements as cross_product
import native_vehicle_wheel_curve_20e84a_replacements as curve
import native_vehicle_wheel_force_20e4f4_replacements as force
import native_vehicle_wheel_offset_20e414_replacements as offset
import native_vehicle_wheel_output_20e91f_replacements as output
import native_vehicle_wheel_response_20e6c3_replacements as response
import native_vehicle_scratch_reset_20ea32_replacements as scratch_reset
from specialize_native_operands import specialize


ROOT = Path(__file__).resolve().parents[1]
SOURCE = 'src/physics/vehicles/physics_vehicle_motion_generated.cpp'
ADAPTER = 'src/physics/vehicles/native_tu15_vehicle_wheel_offset_20e414.hpp'
CROSS_PRODUCT_ADAPTER = 'src/physics/vehicles/native_tu15_vehicle_wheel_cross_product_20e481.hpp'
FORCE_ADAPTER = 'src/physics/vehicles/native_tu15_vehicle_wheel_force_20e4f4.hpp'
RESPONSE_ADAPTER = 'src/physics/vehicles/native_tu15_vehicle_wheel_response_20e6c3.hpp'
OUTPUT_ADAPTER = 'src/physics/vehicles/native_tu15_vehicle_wheel_output_20e91f.hpp'
SCRATCH_RESET_ADAPTER = 'src/physics/vehicles/native_tu15_vehicle_scratch_reset_20ea32.hpp'
CURVE_ADAPTER = 'src/physics/vehicles/native_tu15_vehicle_wheel_curve_20e84a.hpp'
CONTACT_OUTPUT_ADAPTER = 'src/physics/vehicles/native_tu15_vehicle_contact_output_20f6e0.hpp'
CONTACT_PROJECTION_ADAPTER = 'src/physics/vehicles/native_tu15_vehicle_contact_projection_20ee92.hpp'
PROVIDER = 'src/physics/vehicles/native_tu15_vehicle_wheel_stage.hpp'
COMPOSITION = 'src/physics/physics_phantom_world_tu15_generated.cpp'
FIXTURE = 'tests/native_tu15_vehicle_wheel_offset_20e414_tests.cpp'
REFERENCE = 'HALO2_REFERENCE_VEHICLE_WHEEL_OFFSET_20E414'
CROSS_PRODUCT_REFERENCE = 'HALO2_REFERENCE_VEHICLE_WHEEL_CROSS_PRODUCT_20E481'
FORCE_REFERENCE = 'HALO2_REFERENCE_VEHICLE_WHEEL_FORCE_20E4F4'
RESPONSE_REFERENCE = 'HALO2_REFERENCE_VEHICLE_WHEEL_RESPONSE_20E6C3'
OUTPUT_REFERENCE = 'HALO2_REFERENCE_VEHICLE_WHEEL_OUTPUT_20E91F'
SCRATCH_RESET_REFERENCE = 'HALO2_REFERENCE_VEHICLE_SCRATCH_RESET_20EA32'
CURVE_REFERENCE = 'HALO2_REFERENCE_VEHICLE_WHEEL_CURVE_20E84A'
CONTACT_OUTPUT_REFERENCE = 'HALO2_REFERENCE_VEHICLE_CONTACT_OUTPUT_20F6E0'
CONTACT_PROJECTION_REFERENCE = 'HALO2_REFERENCE_VEHICLE_CONTACT_PROJECTION_20EE92'


def run(argv, scratch, env=None):
    result = subprocess.run(argv, cwd=scratch, env=env, capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(f'{argv[0]} exited {result.returncode}\n{result.stdout[-1400:]}{result.stderr[-4500:]}')
    return result.stdout


def stage(use_cross_product, use_force, use_response, use_output, use_scratch_reset, use_curve, use_contact_output,
          use_contact_projection):
    if use_contact_projection:
        return projection, '20ee92', CONTACT_PROJECTION_ADAPTER, ['-DHALO2_TEST_VEHICLE_CONTACT_PROJECTION']
    if use_contact_output:
        return contact_output, '20f6e0', CONTACT_OUTPUT_ADAPTER, ['-DHALO2_TEST_VEHICLE_CONTACT_OUTPUT']
    if use_curve:
        return curve, '20e84a', CURVE_ADAPTER, ['-DHALO2_TEST_VEHICLE_WHEEL_CURVE']
    if use_scratch_reset:
        return scratch_reset, '20ea32', SCRATCH_RESET_ADAPTER, ['-DHALO2_TEST_VEHICLE_SCRATCH_RESET']
    if use_output:
        return output, '20e91f', OUTPUT_ADAPTER, ['-DHALO2_TEST_VEHICLE_WHEEL_OUTPUT']
    if use_response:
        return response, '20e6c3', RESPONSE_ADAPTER, ['-DHALO2_TEST_VEHICLE_WHEEL_RESPONSE']
    if use_force:
        return force, '20e4f4', FORCE_ADAPTER, ['-DHALO2_TEST_VEHICLE_WHEEL_FORCE']
    if use_cross_product:
        return cross_product, '20e481', CROSS_PRODUCT_ADAPTER, ['-DHALO2_TEST_VEHICLE_WHEEL_CROSS_PRODUCT']
    return offset, '20e414', ADAPTER, []


def sources(staged, use_cross_product=False, use_force=False, use_response=False, use_output=False,
            use_scratch_reset=False, use_curve=False, use_contact_output=False, use_contact_projection=False):
    binding, entry, adapter_path, _ = stage(use_cross_product, use_force, use_response, use_output,
                                          use_scratch_reset, use_curve, use_contact_output, use_contact_projection)
    canonical = (ROOT / SOURCE).read_text()
    alias, receipt = specialize(canonical)
    if staged:
        alias, _ = binding.install(canonical, alias)
    elif receipt.get('native_spatial_owners', []).count(entry) != 1:
        raise ValueError(f'Vehicle {entry} motion alias did not bind the wheel stage')
    if binding.install(canonical, alias) != (alias, [entry]):
        raise ValueError(f'Vehicle {entry} binder is not idempotent')
    wheel_stages = (offset, cross_product, force, response, output, scratch_reset, curve, contact_output, projection)
    for preceding in wheel_stages[:wheel_stages.index(binding)]:
        if preceding.install(canonical, alias)[0] != alias:
            raise ValueError(f'Vehicle {entry} binding changed preceding stage {preceding.__name__}')

    adapter = (ROOT / adapter_path).read_text()
    composition = (ROOT / COMPOSITION).read_text().split('vehicle_sites={{\n', 1)[1].split('}};', 1)[0]
    mapping = dict(re.findall(r'\{0x([0-9a-f]+)u,0x([0-9a-f]+)u\}', composition))
    sites = re.findall(r'instruction_literal\(0x([0-9a-f]+)u\)', binding.prepared_reference(canonical))
    lists = ('pre_sites[]{', 'post_sites[]{') if use_curve else ('sites[]{',)
    admitted = [pc for name in lists for pc in re.findall(r'0x([0-9a-f]+)u', adapter.split(name, 1)[1].split('};', 1)[0])]
    exits = [binding.END]
    if use_force:
        exits.append(binding.BYPASS)
    if use_response:
        exits.extend((binding.ACCUMULATE, binding.REJECTED))
    if use_output:
        exits.append(binding.NEXT_WHEEL)
    if use_scratch_reset:
        exits.append(binding.BYPASS)
    if use_curve:
        exits.extend((binding.CALL, binding.RETURN))
    if use_contact_projection:
        exits.extend((binding.CONTINUE, binding.SKIP))
    if sites != admitted or any(int(mapping[pc], 16) != int(pc, 16) + 0x4900 for pc in [*sites, *exits]):
        raise ValueError(f'Vehicle {entry} exact Phantom composition sites changed')
    return canonical, alias


def qualify(args, scratch):
    from instrument_native_execution import instrument_translation

    selected = (args.cross_product, args.force, args.response, args.output_stage, args.scratch_reset, args.curve,
                args.contact_output, args.contact_projection)
    binding, _, adapter_path, switch = stage(*selected)
    canonical, alias = sources(args.staged, *selected)
    accounted, manifest, _ = instrument_translation(alias, canonical, SOURCE, ROOT)
    match, = [row for row in manifest['functions'] if row['symbol'] == '209a50']
    if match['literal_sites'] != 1452 or match['dynamic_instruction_forwarders']:
        raise ValueError('Vehicle 20E350 owner accounting changed')
    flags = ['-std=c++20', '-O2', '-g0', '-fno-fast-math', '-ffp-contract=off', '-frounding-math',
        *(f'-I{i}' for i in (ROOT / 'include', ROOT / 'src', ROOT / 'tests', scratch))]
    if args.full_syntax:
        for name, source in (('normal', alias), ('accounted', accounted)):
            path = scratch / f'{name}.cpp'
            path.write_text(source)
            run([args.compiler, *flags, '-fsyntax-only', path], scratch)
            print(f'Vehicle wheel production {name} alias parsed', file=sys.stderr, flush=True)

    def body(name, text):
        if args.contact_projection:
            return (f'static std::uint32_t {name}(MotionState &s) {{\n{text}\n'
                    'L20a79a:\n    return 0x20a79au;\nL20a887:\n    return 0x20a887u;\n'
                    'L20ab16:\n    return 0x20ab16u;\n}\n')
        if args.contact_output:
            return f'static std::uint32_t {name}(MotionState &s) {{\n{text}\nL20b000:\n    return 0x20b000u;\n}}\n'
        if args.curve:
            pre, post = text
            return (f'static std::uint32_t {name}_before_curve(MotionState &s) {{\n{pre}\n'
                    f'    return 0x209f8du;\n}}\nstatic std::uint32_t {name}(MotionState &s, unsigned route) {{\n'
                    f'    if (route == 0u) return {name}_before_curve(s);\n'
                    '    if (route == 2u) goto L209fe6;\n    if (route == 3u) goto L20a019;\n'
                    f'{post}\nL20a01f:\n    return 0x20a01fu;\n}}\n')
        if args.scratch_reset:
            return (f'static std::uint32_t {name}(MotionState &s) {{\n{text}\n'
                    '    return 0x20a164u;\nL20a17d:\n    return 0x20a17du;\n}\n')
        if args.output_stage:
            return (f'static std::uint32_t {name}(MotionState &s) {{\n{text}\n'
                    '    return 0x20a12cu;\nL209b14:\n    return 0x209b14u;\n}\n')
        if args.response:
            return (f'static std::uint32_t {name}(MotionState &s) {{\n{text}\n'
                    '    return 0x209f4au;\nL209fe6:\n    return 0x209fe6u;\nL20a019:\n    return 0x20a019u;\n}\n')
        if args.force:
            return (f'static std::uint32_t {name}(MotionState &s) {{\n{text}\n'
                    '    return 0x209dbeu;\nL20a01f:\n    return 0x20a01fu;\n}\n')
        return f'static void {name}(MotionState &s) {{\n{text}\n    return;\n}}\n'

    if args.curve:
        sign_extend = re.search(r'^std::uint32_t sign_extend\(std::uint32_t x,unsigned n\) \{\n.*?\n\}',
                                canonical, re.M | re.S)
        if not sign_extend or 'if(n==4)return x;' not in sign_extend[0]:
            raise ValueError('Vehicle Motion signed dword conversion changed')
        stages = (sign_extend[0] + '\n' + body('retained', binding.references(canonical)) +
                  body('selected', binding.bridge_stages(canonical)))
    else:
        stages = body('retained', binding.prepared_reference(canonical)) + body('selected', binding.bridge_stage(canonical))
    (scratch / 'vehicle_wheel_offset_owner.inc').write_text(stages)
    profiles = {}
    env = {name: value for name, value in os.environ.items()
           if name not in (REFERENCE, CROSS_PRODUCT_REFERENCE, FORCE_REFERENCE, RESPONSE_REFERENCE, OUTPUT_REFERENCE,
                           SCRATCH_RESET_REFERENCE, CURVE_REFERENCE, CONTACT_OUTPUT_REFERENCE,
                           CONTACT_PROJECTION_REFERENCE)}
    for profile, cases in (('release', args.cases), ('sanitized', args.sanitized_cases)):
        if not cases:
            continue
        sanitizer = ['-O1', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-pie', '-no-pie']
        binary = scratch / f'vehicle-wheel-{profile}'
        run([args.compiler, *flags, *switch, *(sanitizer if profile == 'sanitized' else []),
             ROOT / FIXTURE, '-o', binary], scratch)
        profiles[profile] = {mode: json.loads(run([binary, str(cases), mode], scratch, env))
                            for mode in ('native', 'reference')}
    print(json.dumps({
        'passed': True, 'staged': args.staged, 'compiler': run([args.compiler, '--version'], scratch).splitlines()[0],
        'source_sha256': binding.digest(canonical), 'alias_sha256': binding.digest(alias),
        'adapter_sha256': binding.digest((ROOT / adapter_path).read_text()),
        'provider_sha256': binding.digest((ROOT / PROVIDER).read_text()),
        'retained_stage_sha256': binding.RAW_PIN, 'prepared_stage_sha256': binding.PREPARED_PIN,
        'reference_stage_sha256': binding.digest(binding.prepared_reference(canonical)),
        'full_production_syntax': args.full_syntax, 'staged_owner': match, 'profiles': profiles, 'scratch': str(scratch),
    }, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default='g++')
    parser.add_argument('--cases', type=int, default=320)
    parser.add_argument('--sanitized-cases', type=int, default=160)
    parser.add_argument('--full-syntax', action='store_true')
    parser.add_argument('--staged', action='store_true')
    stages = parser.add_mutually_exclusive_group()
    stages.add_argument('--cross-product', action='store_true')
    stages.add_argument('--force', action='store_true')
    stages.add_argument('--response', action='store_true')
    stages.add_argument('--output-stage', action='store_true')
    stages.add_argument('--scratch-reset', action='store_true')
    stages.add_argument('--curve', action='store_true')
    stages.add_argument('--contact-output', action='store_true')
    stages.add_argument('--contact-projection', action='store_true')
    args = parser.parse_args()
    owned = os.environ.get('HALO2_TEST_SCRATCH')
    if not owned or not Path(owned).is_dir():
        parser.error('Run through tools/run_test.py for owned scratch')
    qualify(args, Path(owned))


if __name__ == '__main__':
    main()
