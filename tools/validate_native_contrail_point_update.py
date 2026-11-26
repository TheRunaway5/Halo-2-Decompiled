#!/usr/bin/env python3
"""Compare the exact 17F3D0 build binding with its authenticated retained body."""

import argparse
from hashlib import sha256
import json
import os
from pathlib import Path
import re
import subprocess

from instrument_native_execution import instrument_translation, unit_declaration
import native_contrail_point_update_replacements as binding
import specialize_native_operands as loader


ROOT = Path(__file__).resolve().parents[1]
UNIT = 'src/rendering/effects/source_contrail_geometry_tu15_generated.cpp'
ADAPTER = 'src/rendering/effects/native_tu15_contrail_point_update.hpp'
TEST = 'tests/native_tu15_contrail_point_update_tests.cpp'
SWITCH = 'HALO2_REFERENCE_CONTRAIL_POINT_UPDATE'
ACCOUNTING = 'HALO2_NATIVE_EXECUTION_REPORT'


def command(argv, scratch, env=None):
    result = subprocess.run(argv, cwd=scratch, env=env, text=True, capture_output=True)
    if result.returncode:
        raise RuntimeError(f'{argv[0]} returned {result.returncode}\n{result.stdout[-1200:]}{result.stderr[-4000:]}')
    return result.stdout


def counted(report, observed):
    data = json.loads(report.read_text())
    rows = [dict(zip(data['row_fields'], row, strict=True)) for row in data['rows']]
    point = [row for row in rows if row['symbol'] == '17f3d0']
    values = {key: sum(row[key] for row in point) for key in (
        'entries', 'body_entries', 'returns_without_body', 'unwinds_without_body', 'unwinds', 'literal_statements')}
    if (values['entries'] != observed['cases'] or values['body_entries'] != observed['retained'] or
            values['returns_without_body'] + values['unwinds_without_body'] != observed['native'] or
            values['unwinds'] != observed['faults']):
        raise ValueError(f'Point accounting differs from observed entries and returns: {values}')
    return values


def qualify(args, scratch):
    canonical = (ROOT/UNIT).read_text()
    active = loader.specialize
    alias, receipt = active(canonical)
    if args.staged_binding and '17f3d0' not in receipt.get('native_spatial_owners', []):
        def staged(source):
            prepared, metadata = active(source)
            if source == canonical:
                prepared = binding.install(source, prepared)
                metadata.setdefault('native_spatial_owners', []).append('17f3d0')
            return prepared, metadata
        loader.specialize = staged
        alias, receipt = staged(canonical)
    if receipt.get('native_spatial_owners', []).count('17f3d0') != 1:
        raise ValueError('The normal contrail specialization has no exact point update owner')
    bound = binding.owner(alias)[0]
    raw = bound.replace(binding.HOOK, '', 1)
    for op in ('mul', 'add'):
        raw = raw.replace(f'halo2::native_tu15_leaves::ordered_{op}_ss(', f'_mm_{op}_ss(')
    if sha256(raw.encode()).hexdigest() != binding.PREPARED or bound.count(binding.HOOK) != 1:
        raise ValueError('The retained point update no longer matches the pre-install prepared owner')

    accounted, manifest, _ = instrument_translation(alias, canonical, UNIT, ROOT)
    owner, = [row for row in manifest['functions'] if row['symbol'] == '17f3d0']
    if owner['literal_sites'] != 229 or owner['dynamic_instruction_forwarders']:
        raise ValueError('Unexpected contrail point accounting sites')
    for name, pattern in (
        ('contrail_point_fpu.inc', r'(?m)^    void unordered_compare_and_pop\(long double other\) \{\n[\s\S]*?^    \}\n'),
        ('contrail_point_spill.inc', r'(?m)^std::uint32_t native_single_spill\([^\n]+\n[\s\S]*?^\}\n'),
    ):
        matches = re.findall(pattern, canonical)
        if len(matches) != 1:
            raise ValueError(f'The exact retained float helper changed: {name}')
        (scratch/name).write_text(matches[0])

    flags = ['-std=c++20', '-O2', '-g0', '-fno-fast-math', '-ffp-contract=off', '-frounding-math',
             *(f'-I{path}' for path in (ROOT/'include', ROOT/'src', ROOT/'tests', scratch))]
    if args.full_syntax:
        for label, source in (('normal', alias), ('accounted', accounted)):
            file = scratch/f'full-{label}.cpp'
            file.write_text(source)
            command([args.compiler, *flags, '-fsyntax-only', file], scratch)

    results = {}
    for profile, source, cases in (('ordinary', alias, args.cases), ('accounted', accounted, args.cases),
                                   ('sanitized', alias, args.sanitized_cases)):
        if cases < 1:
            continue
        extra, units = [], [ROOT/TEST]
        identity = ''
        if profile == 'accounted':
            units.append(ROOT/'src/runtime/native_execution_accounting.cpp')
            identity = '#include "halo2/native_execution_accounting.hpp"\n' + unit_declaration(UNIT, canonical, alias)
        elif profile == 'sanitized':
            extra = ['-O1', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-pie', '-no-pie']
        (scratch/'contrail_point_accounting.inc').write_text(identity)
        (scratch/'contrail_point_owners.inc').write_text(
            '#include "contrail_point_spill.inc"\n' +
            binding.ordered(raw).replace('run_17f3d0(', 'point_reference(', 1) + binding.owner(source)[0])
        program = scratch/f'point-{profile}'
        command([args.compiler, *flags, *extra, *units, '-o', program], scratch)
        results[profile] = {}
        for mode in ('native', 'reference'):
            env = {key: value for key, value in os.environ.items() if key not in (SWITCH, ACCOUNTING)}
            if mode == 'reference':
                env[SWITCH] = '1'
            report = scratch/'point-accounting.json'
            if profile == 'accounted':
                env[ACCOUNTING] = str(report)
            observed = json.loads(command([program, str(cases), mode], scratch, env))
            if profile == 'accounted':
                observed['execution'] = counted(report, observed)
            results[profile][mode] = observed
        if profile == 'accounted' and (results[profile]['native']['execution']['literal_statements'] >=
                results[profile]['reference']['execution']['literal_statements']):
            raise ValueError('The normal point update did not retire any retained body statements')

    print(json.dumps({'passed': True, 'source': UNIT, 'scratch': str(scratch), 'manifest_owner': owner, 'profiles': results,
        'compiler': command([args.compiler, '--version'], scratch).splitlines()[0],
        'full_source_syntax': args.full_syntax, 'adapter_sha256': sha256((ROOT/ADAPTER).read_bytes()).hexdigest(),
        'prepared_sha256': binding.PREPARED, 'canonical_sha256': binding.CANONICAL}, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default='g++')
    parser.add_argument('--cases', type=int, default=256)
    parser.add_argument('--sanitized-cases', type=int, default=64)
    parser.add_argument('--full-syntax', action='store_true')
    parser.add_argument('--staged-binding', action='store_true')
    args = parser.parse_args()
    owned = os.environ.get('HALO2_TEST_SCRATCH')
    if not owned or not Path(owned).is_dir():
        parser.error('Run through tools/run_test.py for owned scratch')
    qualify(args, Path(owned))


if __name__ == '__main__':
    main()
