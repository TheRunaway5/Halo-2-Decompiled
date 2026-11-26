#!/usr/bin/env python3
"""Qualify the production vehicle x87 classifier against its retained body."""

import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]
SOURCE = 'src/physics/vehicles/physics_vehicle_motion_math_generated.cpp'
COMPOSITION = 'src/physics/physics_phantom_world_tu15_generated.cpp'
ADAPTER = 'src/physics/vehicles/native_tu15_vehicle_x87_dispatch_331ecb.hpp'
FIXTURE = 'tests/native_tu15_vehicle_x87_dispatch_331ecb_tests.cpp'
REFERENCE = 'HALO2_REFERENCE_VEHICLE_X87_DISPATCH_331ECB'
ACCOUNTING = 'HALO2_NATIVE_EXECUTION_REPORT'


def run(argv, cwd, env=None):
    result = subprocess.run(argv, cwd=cwd, env=env, text=True, capture_output=True)
    if result.returncode:
        raise RuntimeError(f'{argv[0]} exited {result.returncode}\n{result.stdout[-1400:]}{result.stderr[-4500:]}')
    return result.stdout


def stage(scratch, patch):
    source = scratch / 'source'
    source.mkdir()
    for directory in ('src', 'include', 'tools', 'tests', 'cmake'):
        run(['cp', '-as', ROOT / directory, source / directory], scratch)
    for file in ('tools/specialize_native_operands.py', 'cmake/NativeOperandSpecialization.cmake',
                 'cmake/PhysicsVehicleMotionMath.cmake'):
        path = source / file
        content = path.read_bytes()
        path.unlink()
        path.write_bytes(content)
    run(['patch', '--batch', '--forward', '-p1', '-i', patch.resolve()], source)
    return source


def measured_owner(path, stats, mode):
    report = json.loads(path.read_text())
    rows = [dict(zip(report['row_fields'], row, strict=True)) for row in report['rows']]
    owners = [row for row in rows if row['symbol'] == '32e35b']

    def total(field):
        return sum(row[field] for row in owners)

    native = total('returns_without_body') + total('unwinds_without_body')
    if (total('entries') != stats['cases'] or total('unwinds') != stats['faults'] or
            total('body_entries') + native != stats['cases'] or native != stats['native_entries'] or
            any(report[key] for key in ('active_scopes', 'omitted_records', 'orphan_literal_events'))):
        raise ValueError(f'Vehicle 331ECB accounting mismatch for {mode}: {owners}')
    return owners


def validate(args, root, scratch):
    sys.path.insert(0, str(root / 'tools'))
    import native_vehicle_x87_dispatch_331ecb_replacements as binding
    from specialize_native_operands import specialize
    from instrument_native_execution import instrument_translation, unit_declaration

    canonical = (root / SOURCE).read_text()
    alias, metadata = specialize(canonical)
    if (metadata.get('native_spatial_owners', []).count('331ecb') != 1 or
            binding.install(canonical, alias) != (alias, ['331ecb'])):
        raise ValueError('Vehicle 331ECB production alias did not bind the native owner')
    if (canonical.count('if(call.target==selected(0x32e35bu,image)){source_run_physics_vehicle_motion_math_runtime(') != 1 or
            canonical.count('s.push(0x3799d7u,selected(0x3799dcu,s.image));run_32e35b(s);') != 1):
        raise ValueError('Vehicle 331ECB direct or selected caller changed')
    adapter = (root / ADAPTER).read_text()
    remapping = (root / COMPOSITION).read_text().split('vehicle_sites={{\n', 1)[1].split('}};', 1)[0]
    rows = dict(re.findall(r'\{0x([0-9a-f]+)u,0x([0-9a-f]+)u\}', remapping))
    sites = re.findall(r'instruction_literal\(0x([0-9a-f]+)u\)', binding.prepared_reference(canonical))
    admitted_sites = re.findall(r'0x([0-9a-f]+)u', adapter.split('canonical_sites[]{', 1)[1].split('};', 1)[0])
    if sites != admitted_sites or any(int(rows[pc], 16) != int(pc, 16) + 0x3b70 for pc in sites):
        raise ValueError('Vehicle 331ECB Phantom composition observation remap changed')
    for key in ('case 0x32e35bu:return 0x331ecbu;', 'case 0x469bf4u:return 0x46ecb4u;'):
        changed = canonical.replace(key, '', 1)
        try:
            binding.install(changed, alias)
        except ValueError:
            pass
        else:
            raise ValueError(f'Vehicle 331ECB accepted an unauthenticated original source route: {key}')

    counted, manifest, _ = instrument_translation(alias, canonical, SOURCE, root)
    owner, = [row for row in manifest['functions'] if row['symbol'] == '32e35b']
    if owner['literal_sites'] != 45 or owner['dynamic_instruction_forwarders']:
        raise ValueError('Vehicle 331ECB retained site count changed')
    flags = ['-std=c++20', '-O2', '-g0', '-fno-fast-math', '-ffp-contract=off', '-frounding-math',
             *(f'-I{path}' for path in (root / 'include', root / 'src', root / 'tests', scratch))]
    if args.full_syntax:
        for name, source in (('normal', alias), ('accounted', counted)):
            path = scratch / f'{name}.cpp'
            path.write_text(source)
            run([args.compiler, *flags, '-fsyntax-only', path], scratch)
            print(f'Vehicle 331ECB production {name} alias parsed', file=sys.stderr, flush=True)

    retained = binding.prepared_reference(canonical).replace('run_32e35b(', 'dispatch_reference(', 1)
    profiles = {}
    for profile, source, cases in (('release', alias, args.cases), ('accounted', counted, args.cases),
                                   ('sanitized', alias, args.sanitized_cases)):
        if not cases:
            continue
        accounting = profile == 'accounted'
        identity = ('#include "halo2/native_execution_accounting.hpp"\n' +
                    unit_declaration(SOURCE, canonical, alias)) if accounting else ''
        (scratch / 'vehicle_x87_dispatch_accounting.inc').write_text(identity)
        (scratch / 'vehicle_x87_dispatch_owner.inc').write_text(retained + '\n' + binding.BODY.search(source)[0] + '\n')
        sources = [root / FIXTURE]
        if accounting:
            sources.append(root / 'src/runtime/native_execution_accounting.cpp')
        sanitizer = ['-O1', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-pie', '-no-pie']
        program = scratch / f'vehicle-x87-{profile}'
        run([args.compiler, *flags, *(sanitizer if profile == 'sanitized' else []), *sources, '-o', program], scratch)
        results = {}
        for mode in ('native', 'reference'):
            env = {key: value for key, value in os.environ.items() if key not in (REFERENCE, ACCOUNTING)}
            report = scratch / 'counts.json'
            if accounting:
                env[ACCOUNTING] = str(report)
            stats = json.loads(run([program, str(cases), mode], scratch, env))
            if accounting:
                stats['counted_owner'] = measured_owner(report, stats, mode)
            results[mode] = stats
        if accounting:
            selected = sum(row['literal_statements'] for row in results['native']['counted_owner'])
            reference = sum(row['literal_statements'] for row in results['reference']['counted_owner'])
            if selected >= reference:
                raise ValueError('Vehicle 331ECB native fixture did not remove translated work')
        profiles[profile] = results

    print(json.dumps({'passed': True, 'compiler': run([args.compiler, '--version'], scratch).splitlines()[0],
        'canonical_body_sha256': binding.PIN, 'retained_body_sha256': binding.PREPARED_PIN,
        'adapter_sha256': binding.digest(adapter),
        'canonical_sha256': binding.digest(canonical), 'prepared_sha256': binding.digest(alias),
        'manifest_owner': owner, 'profiles': profiles, 'full_production_syntax': args.full_syntax,
        'scratch': str(scratch)}, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default='g++')
    parser.add_argument('--cases', type=int, default=256)
    parser.add_argument('--sanitized-cases', type=int, default=128)
    parser.add_argument('--staging-patch', type=Path)
    parser.add_argument('--full-syntax', action='store_true')
    args = parser.parse_args()
    owned = os.environ.get('HALO2_TEST_SCRATCH')
    if not owned or not Path(owned).is_dir():
        parser.error('Run through tools/run_test.py for owned scratch')
    scratch = Path(owned)
    validate(args, stage(scratch, args.staging_patch) if args.staging_patch else ROOT, scratch)


if __name__ == '__main__':
    main()
