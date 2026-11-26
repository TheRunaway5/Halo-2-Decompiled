#!/usr/bin/env python3
"""Compare both 178110 production routes against the authenticated TU1.5 source."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

from instrument_native_execution import instrument_translation
import native_particle_location_prepare_replacements as binding
from specialize_native_operands import specialize


ROOT = Path(__file__).resolve().parents[1]
UNITS = {
    'death': ROOT / 'src/gameplay/units/death_cycle_runtime_tu15_generated.cpp',
    'frame': ROOT / 'src/rendering/effects/source_effect_particle_frame_tu15_generated.cpp',
}
HEADER = ROOT / 'src/rendering/effects/native_tu15_particle_location_prepare.hpp'
TEST = ROOT / 'tests/native_tu15_particle_location_prepare_tests.cpp'
ACCOUNTING = ROOT / 'src/runtime/native_execution_accounting.cpp'
IMAGE = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
MACHINE = 'c87732269faea85baf9edfc6f0a07f93528f2073b7e289e7877f2bf65619860d'
CHILDREN = ('251930', '2520a0', '176a40')
MODES = ('literal', 'census', 'verify', 'collector')
MUTANTS = (
    ('owner_flag_mask', 'state | 0x101u', 'state | 0x100u'),
    ('location_stride', 'flags.product(gp[esi], sizeof(location::LocationDatum))',
     'flags.product(gp[esi], sizeof(location::LocationDatum) - 4u)'),
    ('frame_nan', 'world::ordered_add_ss(value, scalar(0x17819bu, gp[ebp]))',
     'world::ordered_add_ss(scalar(0x17819bu, gp[ebp]), value)'),
    ('locator_mode_byte', 'gp[esp] + 0x7cu, 1', 'gp[esp] + 0x7du, 1'),
    ('return', 'port.ret(last, argument_size);', 'port.ret(last, 0u);'),
)


def sha(value):
    return hashlib.sha256(value.encode() if isinstance(value, str) else value).hexdigest()


def image(path):
    sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
    from xbe import Xbe

    reader = Xbe(path)
    if sha(reader.data) != IMAGE or sha(reader.read(0x178110, 0x124)) != MACHINE:
        raise ValueError('Particle location preparation image or owner bytes drifted')
    for name, site in binding.CHILDREN.items():
        addr = int(site, 16)
        code = reader.read(addr, 5)
        if code[0] != 0xe8 or addr + 5 + int.from_bytes(code[1:], 'little', signed=True) != int(name, 16):
            raise ValueError(f'Particle location preparation child at {site} drifted')


def alias(source, name):
    return re.search(r'static void ' + name + r'\(WorldState &s\) \{.*?\n\}', source, re.S)[0]


def prepare():
    variants, manifest, live, published = {}, {}, {}, {}
    for kind, path in UNITS.items():
        canonical = path.read_text()
        original = binding.world.body(canonical, binding.OWNER)[0]
        marker, _ = binding.OWNERS[kind]
        wrong = binding.OWNERS['frame' if kind == 'death' else 'death'][0]
        changed = (
            canonical.replace(marker, wrong, 1),
            canonical.replace(original, original.replace('0xfffff98fu', '0xfffff99fu', 1), 1),
        )
        if binding.route(canonical) != kind or any(candidate == canonical or binding.route(candidate) for candidate in changed):
            raise ValueError(f'Particle location preparation {kind} accepted wrong unit or changed mask')
        ordinary, receipt = specialize(canonical)
        if (receipt.get('native_spatial_owners', []).count(binding.OWNER) != 1 or
                binding.install(canonical, ordinary) != (ordinary, [])):
            raise ValueError(f'Particle location preparation missing {kind} binding')
        owner = binding.world.body(ordinary, binding.OWNER)[0]
        fallback = owner.replace(binding.hook(kind), '', 1)
        replay = binding.reference(kind, fallback)
        pcs = set(binding.LITERAL.findall(owner))
        if (owner.count(binding.hook(kind)) != 1 or len(pcs) != 93 or replay not in ordinary or
                pcs != set(binding.LITERAL.findall(replay))):
            raise ValueError(f'Particle location preparation {kind} replay differs from exact source')
        published[kind] = sorted(pcs)
        live[kind] = binding.reference(kind, fallback, replay=False)
        accounted, boundaries, _ = instrument_translation(ordinary, canonical, path.relative_to(ROOT).as_posix(), ROOT)
        rows = {row['symbol']: row for row in boundaries['functions']
                if row['symbol'] == binding.OWNER or row.get('canonical_reference_owner') == binding.OWNER}
        if {name: row['literal_sites'] for name,row in rows.items()} != {binding.OWNER: 93, binding.REFERENCE: 93}:
            raise ValueError(f'Particle location preparation {kind} accounting alias drifted')
        manifest[kind] = rows
        variants[kind] = {'ordinary': ordinary, 'accounted': accounted}
    if published['death'] != published['frame']:
        raise ValueError('Particle location preparation lexical variants disagree on TU1.5 sites')
    return variants, manifest, live, published


def fixture(variants, live, variant):
    children = 'namespace children {\n'
    child_pins = {}
    for name in CHILDREN:
        child = alias(variants['death']['ordinary'], 'run_' + name)
        start = child.index('s.instruction_literal(')
        body = child[:child.index('{')+1] + '\n' + child[start:]
        child_pins[name] = sha(body)
        children += body.replace(f'run_{name}(', f'body_{name}(', 1) + '\n'
        children += (f'static void run_{name}(WorldState &s) {{ log_call(entry_hash(s,0x{name}u)); '
                     f'body_{name}(s); log_call(entry_hash(s,0x{name}u)); }}\n')
    parts = [children + '}\n']
    for kind, units in variants.items():
        text = units[variant]
        replay = alias(text, binding.REFERENCE)
        if variant == 'accounted':
            unit = re.search(r'&::halo2::execution_accounting::(unit_\w+)', replay)[1]
            parts += ['#include "halo2/native_execution_accounting.hpp"\n',
                      next(line+'\n' for line in text.splitlines()
                           if line.startswith('namespace halo2::execution_accounting { ') and f' {unit}{{' in line)]
        route = binding.direct(kind)
        parts += [f'namespace {kind}_copy {{\n',
                  ''.join(f'using children::run_{name};\n' for name in binding.CHILDREN),
                  'using children::dispatch;\n', replay, '\n', alias(text, 'run_' + binding.OWNER), '\n',
                  live[kind], '\nstatic bool attempt_native(WorldState &s) {\n',
                  f'    return {binding.NAMESPACE}::bridge(s,{binding.REFERENCE},[](auto &value,auto child) {{\n',
                  route, '        });\n}\n}\n']
    return ''.join(parts), child_pins


def run(binary, count, mode, accounting=None):
    env = os.environ.copy()
    for flag in ('HALO2_REFERENCE_PARTICLE_LOCATION_PREPARE', 'HALO2_REFERENCE_PHYSICS_WORLD_LEAVES',
                 'HALO2_VERIFY_PHYSICS_WORLD_LEAVES', 'HALO2_CENSUS_NATIVE_BRIDGES'):
        env.pop(flag, None)
    if mode == 'reference':
        env['HALO2_REFERENCE_PARTICLE_LOCATION_PREPARE'] = '1'
    if accounting:
        env['HALO2_NATIVE_EXECUTION_REPORT'] = str(accounting)
    return subprocess.run([str(binary), str(count), mode], cwd=binary.parent, env=env,
                          capture_output=True, text=True, timeout=90)


def results(binary, count, modes, accounted=False):
    outcomes = {}
    for mode in modes:
        accounting = binary.parent / f'{mode}.json' if accounted else None
        result = run(binary, count, mode, accounting)
        if result.returncode:
            raise RuntimeError(f'{binary.name} {mode} failed:\n{result.stderr}')
        report = outcomes[mode] = json.loads(result.stdout)
        if mode == 'verify':
            hits = re.search(r'Particle location prepare verify: (\d+) calls', result.stderr)
            if not hits or int(hits[1]) != 2 * count:
                raise ValueError(f'Particle location preparation missed a lexical replay: {result.stderr}')
            report['verified'] = int(hits[1])
        if not accounted:
            continue
        counters = json.loads(accounting.read_text())
        if any(counters[key] for key in ('orphan_literal_events', 'registration_failures', 'active_scopes')):
            raise ValueError('Incomplete particle location preparation accounting')
        rows = [dict(zip(counters['row_fields'], row)) for row in counters['rows']]
        owners = [row for row in rows if row['symbol'] == binding.OWNER]
        replays = [row for row in rows if row['symbol'] == binding.REFERENCE]
        if (len(owners) != 2 or any(not row['entries'] or bool(row['literal_statements']) != (mode == 'reference') for row in owners)
                or len(replays) != (2 if mode == 'verify' else 0)
                or any(not row['literal_statements'] for row in replays)):
            raise ValueError(f'Accounting did not isolate main owners and verification replays in {mode}')
        report['accounting'] = rows
    return outcomes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default='c++')
    parser.add_argument('--xbe', required=True, type=Path)
    parser.add_argument('--report', required=True, type=Path)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    image(args.xbe)
    variants, manifest, live, sites = prepare()
    command = [args.compiler, '-std=c++20', '-O2', '-fno-fast-math', '-ffp-contract=off', '-frounding-math',
               f'-I{ROOT / "src"}', f'-I{ROOT / "include"}']
    for kind, units in variants.items():
        for variant, source in units.items():
            cpp = scratch / f'{kind}-{variant}.cpp'
            cpp.write_text(source)
            subprocess.run([*command, '-fsyntax-only', str(cpp)], cwd=scratch, check=True)
    comparisons = {}
    cases = (('ordinary', 900, []), ('accounted', 600, [str(ACCOUNTING)]), ('sanitized', 300, [
        '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']))
    for variant, count, extra in cases:
        directory = scratch / variant
        directory.mkdir()
        accounted = variant == 'accounted'
        include, children = fixture(variants, live, 'accounted' if accounted else 'ordinary')
        (directory / 'particle_location_prepare_reference.inc').write_text(include)
        binary = directory / 'probe'
        subprocess.run([*command, f'-I{directory}', str(TEST), *extra, '-o', str(binary)], cwd=scratch, check=True)
        modes = ('literal', 'verify', 'reference') if accounted else MODES
        comparisons[variant] = results(binary, count, modes, accounted)
    rejected = []
    header = HEADER.read_text()
    for name, before, after in MUTANTS:
        if header.count(before) != 1:
            raise ValueError(f'Particle location prepare mutant {name} needs one source anchor')
        directory = scratch / name
        target = directory / HEADER.relative_to(ROOT / 'src')
        target.parent.mkdir(parents=True)
        target.write_text(header.replace(before, after, 1))
        binary = directory / 'probe'
        subprocess.run([command[0], f'-I{directory}', *command[1:], f'-I{scratch / "ordinary"}',
                        str(TEST), '-o', str(binary)], check=True, cwd=scratch)
        result = run(binary, 48, 'literal')
        if result.returncode != 1 or 'differential mismatch' not in result.stderr:
            raise ValueError(f'Particle location prepare accepted mutant {name}: {result.stderr}')
        rejected.append(name)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(dict(passed=True, owned_scratch=str(scratch), retained_large_artifacts=[],
        source_hashes={str(path.relative_to(ROOT)): sha(path.read_bytes()) for path in
                       (*UNITS.values(), HEADER, TEST, Path(binding.__file__), Path(__file__), ACCOUNTING)},
        image_sha256=IMAGE, machine_sha256=MACHINE, actual_child_source_pins=children,
        production_routes={kind: {name: 'local' if binding.local(kind,name) else 'dispatch'
                                  for name in binding.CHILDREN} for kind in UNITS},
        literal_sites=sites, manifest=manifest,
        production_syntax={kind: {name:sha(src) for name,src in units.items()} for kind,units in variants.items()},
        comparisons=comparisons, rejected_semantics=rejected), indent=2) + '\n')
    print('TU1.5 178110 passed both actual source routes, pinned children and accounted execution.')


if __name__ == '__main__':
    main()
