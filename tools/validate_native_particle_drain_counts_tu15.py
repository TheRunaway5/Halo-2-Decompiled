#!/usr/bin/env python3
"""Check TU1.5 Effect Frame's two particle count functions and their call boundary."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

import native_particle_drain_counts_replacements as binding
from instrument_native_execution import instrument_translation
from specialize_native_operands import specialize


ROOT = Path(__file__).resolve().parents[1]
UNIT = ROOT / 'src/rendering/effects/source_effect_particle_frame_tu15_generated.cpp'
HEADER = ROOT / 'src/rendering/effects/native_tu15_particle_drain_counts.hpp'
TEST = ROOT / 'tests/native_tu15_particle_drain_counts_tests.cpp'
ACCOUNTING = ROOT / 'src/runtime/native_execution_accounting.cpp'
IMAGE_PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
MACHINE = {
    '178240': (0x178282, 'fdaced6b1d901c044f9d1d42df0930cbcc8b20b4d8407e40fac1b6be9c7d9493'),
    '252060': (0x25209c, '4cebdee4e0098b2f4e91bd95c2eb00f4e1b572cd79b9c1c4eb4d26b7f593bb94'),
}
FLAGS = ['-std=c++20', '-O2', '-fno-fast-math', '-ffp-contract=off', '-frounding-math']
MODES = ('literal', 'census', 'verify', 'collector')
MUTANTS = (
    ('location_handle', 'gp[esi] & 0xffffu', 'gp[esi] & 0x7fffu'),
    ('count_width', 'particle_count), 2)', 'particle_count), 4)'),
    ('emitter_link', 'offsetof(location::EmitterDatum, next_emitter)',
     'offsetof(location::EmitterDatum, first_particle)'),
    ('child_fault_state', '                gp = port.s.gp;\n                flags = port.s.flags;\n', ''),
    ('location_return', 'port.ret(last_locations, 0u);', 'port.ret(last_locations, 4u);'),
    ('emitter_return', 'port.ret(last_emitters, 0u);', 'port.ret(last_emitters, 4u);'),
)


def sha(value):
    return hashlib.sha256(value.encode() if isinstance(value, str) else value).hexdigest()


def authenticate_image(path):
    sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
    from xbe import Xbe

    image = Xbe(path)
    if sha(image.data) != IMAGE_PIN:
        raise ValueError('Particle drain counts TU1.5 image drifted')
    for name, (end, pin) in MACHINE.items():
        start = int(name, 16)
        if sha(image.read(start, end - start)) != pin:
            raise ValueError(f'TU1.5 particle count {name} bytes drifted')
    for call, expected in ((0x177ddd, 0x178240), (0x17826d, 0x252060)):
        code = image.read(call, 5)
        if code[0] != 0xe8 or call + 5 + int.from_bytes(code[1:], 'little', signed=True) != expected:
            raise ValueError(f'TU1.5 particle drain direct call at {call:x} drifted')


def prepare():
    canonical = UNIT.read_text()
    if not binding.admitted(canonical):
        raise ValueError('Particle drain count source pins drifted')
    prepared, receipt = specialize(canonical)
    if any(receipt.get('native_spatial_owners', []).count(name) != 1 for name in binding.OWNERS):
        raise ValueError('Expected two complete Effect Frame particle count bindings')
    if binding.install(canonical, prepared) != (prepared, []):
        raise ValueError('Particle count binding must be idempotent')
    static = {}
    expected_origin = {}
    originals = {}
    stripped = prepared
    for name, (_, operation, expected) in binding.OWNERS.items():
        body = binding.world.body(prepared, name)[0]
        reference_name = binding.reference_name(name)
        reference, = re.findall(r'static void ' + reference_name + r'\(WorldState &s\) \{.*?\n\}', prepared, re.S)
        hook, = re.findall(r'\n    if \(' + binding.NAMESPACE + '::bridge_' + operation + r'[^\n]+', body)
        originals[name] = body.replace(hook, '', 1)
        sites = set(binding.LITERAL.findall(body))
        if len(sites) != expected or set(binding.LITERAL.findall(reference)) != sites:
            raise ValueError(f'Particle drain count {name} replay sites drifted')
        if name == '178240' and (body.count(binding.CALL) != 1 or reference.count(binding.REPLAY) != 1
                                or hook.count('run_252060(value);') != 1):
            raise ValueError('Particle drain count no longer invokes its actual emitter child')
        expected_origin.update({name: (None, expected), reference_name: (name, expected)})
        static[name] = dict(sites=sorted(sites), original_source_sha=sha(originals[name]), replay_sha=sha(reference))
        stripped = stripped.replace(reference + '\n\n', '', 1).replace(hook, '', 1)
    # A subsequent binder can prepend its own include above the drain header.
    # Compare the exact reinstall at the position this binder itself inserts.
    expected = binding.INCLUDE + prepared.replace(binding.INCLUDE, '', 1)
    if (prepared.count(binding.INCLUDE) != 1 or
            binding.install(canonical, stripped.replace(binding.INCLUDE, '', 1)) != (expected, list(binding.OWNERS))):
        raise ValueError('Prepared particle drain count bindings did not reproduce from their exact source')

    bad_canonical = {
        'wrong_unit': canonical.replace(binding.SOURCE_OWNER, 'void detached_effect_frame(', 1),
        'child_call': canonical.replace(binding.world.body(canonical, '178240')[0],
            binding.world.body(canonical, '178240')[0].replace(binding.CALL, 'run_251c80(s);', 1), 1),
        'unsigned_width': canonical.replace(binding.world.body(canonical, '252060')[0],
            binding.world.body(canonical, '252060')[0].replace('s.gp[2]*1u),2)', 's.gp[2]*1u),4)', 1), 1),
    }
    for name, candidate in bad_canonical.items():
        if candidate == canonical or binding.admitted(candidate):
            raise ValueError(f'Particle drain count source binding accepted {name}')

    accounted, manifest, _ = instrument_translation(prepared, canonical, UNIT.relative_to(ROOT).as_posix(), ROOT)
    rows = {row['symbol']: row for row in manifest['functions'] if row['symbol'] in expected_origin}
    if {symbol: (row.get('canonical_reference_owner'), row['literal_sites'])
        for symbol, row in rows.items()} != expected_origin:
        raise ValueError('Particle drain count accounted static manifest drifted')
    return prepared, accounted, originals, static, rows, list(bad_canonical)


def fixture_source(source, originals, accounted):
    names = [name for owner in binding.OWNERS for name in (binding.reference_name(owner), 'run_' + owner)]
    bodies = [re.search(r'static void ' + name + r'\(WorldState &s\) \{.*?\n\}', source, re.S)[0] for name in names]
    identity = ''
    if accounted:
        unit = re.search(r'&::halo2::execution_accounting::(unit_\w+)', bodies[0])[1]
        identity = '#include "halo2/native_execution_accounting.hpp"\n' + next(
            line + '\n' for line in source.splitlines()
            if line.startswith('namespace halo2::execution_accounting { ') and f' {unit}{{' in line)
    forward = 'static void run_252060(WorldState &);\n'
    return (identity + 'namespace prepared {\n' + forward + '\n'.join(bodies) + '\n}\n'
            'namespace original {\n' + forward + '\n'.join(originals.values()) + '\n}\n')


def run(binary, count, mode, candidate_report=None):
    env = os.environ.copy()
    for flag in ('HALO2_REFERENCE_PARTICLE_DRAIN_COUNTS', 'HALO2_REFERENCE_PARTICLE_SYSTEM_UPDATE',
                 'HALO2_REFERENCE_PHYSICS_WORLD_LEAVES', 'HALO2_VERIFY_PHYSICS_WORLD_LEAVES',
                 'HALO2_CENSUS_NATIVE_BRIDGES'):
        env.pop(flag, None)
    if candidate_report:
        env['HALO2_NATIVE_EXECUTION_REPORT'] = str(candidate_report)
    if mode == 'reference':
        env['HALO2_REFERENCE_PARTICLE_DRAIN_COUNTS'] = '1'
    return subprocess.run([str(binary), str(count), mode], cwd=binary.parent, env=env,
                          capture_output=True, text=True, timeout=90)


def check_run(binary, count, mode, candidate_report=None):
    result = run(binary, count, mode, candidate_report)
    if result.returncode:
        raise RuntimeError(f'{binary.name} {mode} failed:\n{result.stderr}')
    fixture = json.loads(result.stdout)
    if mode == 'verify':
        counts = re.search(r'Particle drain counts verify: (\d+) locations, (\d+) emitters', result.stderr)
        if not counts or not all(map(int, counts.groups())):
            raise ValueError('Particle drain count verify missed one of the two owners')
        fixture['native_verifications'] = dict(zip(binding.OWNERS, map(int, counts.groups())))
    return fixture


def check_accounting(candidate_report, mode):
    data = json.loads(candidate_report.read_text())
    if any(data[key] for key in ('orphan_literal_events', 'registration_failures', 'active_scopes')):
        raise ValueError('Incomplete particle count accounting')
    relevant = set(binding.OWNERS) | {binding.reference_name(name) for name in binding.OWNERS}
    rows = {row['symbol']: row for row in (dict(zip(data['row_fields'], item)) for item in data['rows'])
            if row['symbol'] in relevant}
    for name in binding.OWNERS:
        row = rows[name]
        ref = rows.get(binding.reference_name(name))
        if row['entries'] == 0 or bool(row['literal_statements']) != (mode == 'reference'):
            raise ValueError(f'Unexpected particle count translations for {name} in {mode} mode')
        if bool(ref and ref['literal_statements']) != (mode == 'verify'):
            raise ValueError(f'Unexpected particle count replay translations for {name} in {mode} mode')
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default='c++')
    parser.add_argument('--xbe', required=True, type=Path)
    parser.add_argument('--report', required=True, type=Path)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    authenticate_image(args.xbe)
    prepared, accounted, originals, static, manifest, source_rejections = prepare()
    command = [args.compiler, *FLAGS, f'-I{ROOT / "include"}', f'-I{ROOT / "src"}']
    syntax = {}
    fixtures = {}
    for variant, source in (('ordinary', prepared), ('accounted', accounted)):
        directory = scratch / variant
        directory.mkdir()
        cpp = directory / UNIT.name
        cpp.write_text(source)
        subprocess.run([*command, '-fsyntax-only', str(cpp)], cwd=scratch, check=True)
        syntax[variant] = sha(source)
        (directory / 'particle_drain_counts_reference.inc').write_text(fixture_source(source, originals, variant == 'accounted'))
        binary = directory / 'probe'
        extra = [str(ACCOUNTING)] if variant == 'accounted' else []
        subprocess.run([*command, f'-I{directory}', str(TEST), *extra, '-o', str(binary)], cwd=scratch, check=True)
        fixture = fixtures[variant] = {}
        for mode in (*MODES, 'reference'):
            report = directory / f'{mode}.json' if variant == 'accounted' else None
            fixture[mode] = check_run(binary, 1600, mode, report)
            if report:
                fixture[mode]['accounting'] = check_accounting(report, mode)
    directory = scratch / 'ordinary'
    binary = directory / 'sanitized'
    sanitize = ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
    subprocess.run([*command, *sanitize, f'-I{directory}', str(TEST), '-o', str(binary)], cwd=scratch, check=True)
    fixtures['sanitized'] = {mode: check_run(binary, 400, mode) for mode in MODES}

    rejected = []
    header = HEADER.read_text()
    for name, before, after in MUTANTS:
        if header.count(before) != 1:
            raise ValueError(f'Particle count mutant {name} must have a unique anchor')
        override = scratch / name
        target = override / HEADER.relative_to(ROOT / 'src')
        target.parent.mkdir(parents=True)
        target.write_text(header.replace(before, after, 1))
        binary = override / 'probe'
        subprocess.run([command[0], f'-I{override}', *command[1:], f'-I{directory}', str(TEST), '-o', str(binary)],
                       cwd=scratch, check=True)
        attempt = run(binary, 54, 'literal')
        if attempt.returncode != 1 or 'differential mismatch' not in attempt.stderr:
            raise ValueError(f'Particle count differential accepted {name}: {attempt.stderr}')
        rejected.append(name)

    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(dict(passed=True, owned_scratch=str(scratch), retained_large_artifacts=[],
        source_hashes={str(path.relative_to(ROOT)): sha(path.read_bytes()) for path in
                       (UNIT, HEADER, TEST, ACCOUNTING, Path(binding.__file__), Path(__file__))},
        image_sha256=IMAGE_PIN, machine=MACHINE, static=static, manifest=manifest, production_syntax=syntax,
        fixtures=fixtures, rejected_source=source_rejections, rejected_semantics=rejected), indent=2) + '\n')
    print('TU1.5 particle drain counts passed ordinary and accounted source, live child and injected fault comparisons.')


if __name__ == '__main__':
    main()
