#!/usr/bin/env python3
"""Authenticate and compare Death Cycle's 17BB10 particle location update."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

from instrument_native_execution import instrument_translation
import native_particle_location_update_replacements as binding
from specialize_native_operands import specialize


ROOT = Path(__file__).resolve().parents[1]
UNIT = ROOT / 'src/gameplay/units/death_cycle_runtime_tu15_generated.cpp'
HEADER = ROOT / 'src/rendering/effects/native_tu15_particle_location_update.hpp'
TEST = ROOT / 'tests/native_tu15_particle_location_update_tests.cpp'
IMAGE = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
MACHINE = '676fb3a1ecf3396ffa93b0feea5fcb09f7c353aaa2d05f8ba43367b12c984e46'
TABLE = 'c2bc1700cbbb170047bc1700fcbc1700'
ACTUAL_CHILDREN = ('178020', '178070', '13a6b0', '17dce0')
CONTROLLED_CHILDREN = ('178920', '17ba50', '178110')
MODES = ('literal', 'census', 'verify', 'collector')
MUTANTS = (
    ('plane_mode', 'low_byte(eax, !node_matches_plane(0u));', 'low_byte(eax, node_matches_plane(0u));'),
    ('plane_coefficient', 'gp[ebx] + 0x38u));', 'gp[ebx] + 0x34u));'),
    ('iterator_return', 'site + 0x11u, site + 0x16u', 'site + 0x11u, site + 0x15u'),
    ('interface_method', '0x17bb7eu, 0x17bb81u, gp[edx] + 0x48u',
     '0x17bb7eu, 0x17bb81u, gp[edx] + 0x44u'),
    ('location_argument', 'port.push(0x17bceau, gp[ecx]);', 'port.push(0x17bceau, gp[edx]);'),
    ('stack_return', 'port.ret(last, argument_size);', 'port.ret(last, 0u);'),
)


def sha(value):
    return hashlib.sha256(value.encode() if isinstance(value, str) else value).hexdigest()


def image(path):
    sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
    from xbe import Xbe

    reader = Xbe(path)
    if (sha(reader.data) != IMAGE or sha(reader.read(0x17bb10, 0x216)) != MACHINE or
            reader.read(0x17bd28,16).hex() != TABLE):
        raise ValueError('Particle location update image, owner, or jump table drifted')
    calls = {
        0x17bb3b: 0x178020, 0x17bb6b: 0x178070, 0x17bb73: 0x13a6b0, 0x17bb95: 0x178920,
        0x17bbab: 0x17dce0, 0x17bccd: 0x17ba50, 0x17bcf1: 0x178110, 0x17bd0d: 0x17dce0,
        0x17be91: 0x17bb10, 0x17bff4: 0x17bb10,
    }
    for site, target in calls.items():
        code = reader.read(site, 5)
        if code[0] != 0xe8 or site + 5 + int.from_bytes(code[1:], 'little', signed=True) != target:
            raise ValueError(f'Particle location update child at {site:x} drifted')
    return {f'{site:x}': f'{target:x}' for site,target in calls.items()}


def sites(body):
    return set(re.findall(r'\bs\.instruction_literal\(0x([0-9a-f]+)u\)', body))


def prepare():
    canonical = UNIT.read_text()
    owner = binding.world.body(canonical, binding.OWNER)[0]
    drift = {
        'selector': canonical.replace('case 0x17bbcbu:return 0x17bbcbu;', 'case 0x17bbcbu:return 0x17bc47u;', 1),
        'child': canonical.replace(owner, owner.replace('run_178110(s);', 'run_178070(s);', 1), 1),
    }
    if any(candidate == canonical or binding.admitted(candidate) for candidate in drift.values()):
        raise ValueError('Particle location update accepted changed source or branch mapping')
    ordinary, metadata = specialize(canonical)
    if (metadata.get('native_spatial_owners', []).count(binding.OWNER) != 1 or
            binding.install(canonical, ordinary) != (ordinary, [])):
        raise ValueError('Expected a full, idempotent particle location update binding')
    program = binding.world.body(ordinary, binding.OWNER)[0]
    prepared_fallback = program.replace(binding.HOOK, '', 1)
    replay = binding.reference(prepared_fallback)
    if program.count(binding.HOOK) != 1 or replay not in ordinary or len(sites(program)) != 167 or sites(program) != sites(replay):
        raise ValueError('Particle location update replay did not preserve exact sites and source')
    live = binding.reference(prepared_fallback, replay=False)
    live = live.replace('canonical_branch(', 'children::canonical_branch(')
    live = live.replace('dispatch(s,target,', 'children::dispatch(s,target,')
    for name in binding.CHILDREN:
        live = live.replace(f'run_{name}(s);', f'children::run_{name}(s);')

    branch = re.search(r'std::uint32_t canonical_branch\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',
                       canonical, re.S)[0]
    children = {}
    for name in ACTUAL_CHILDREN:
        child = binding.world.body(ordinary, name)[0]
        child = child[:child.index('{')+1] + '\n' + child[child.index('s.instruction_literal('):]
        children[name] = child.replace(f'run_{name}(', f'body_{name}(', 1)
    defs = 'namespace children {\n' + branch + '\n'
    defs += ''.join(f'static void run_{name}(WorldState &);\n' for name in ACTUAL_CHILDREN)
    defs += '\n'.join(children.values()) + '\n'
    for name in ACTUAL_CHILDREN:
        defs += (f'static void run_{name}(WorldState &s) {{\n'
                 f'    log_call(entry_hash(s,0x{name}u));\n'
                 f'    body_{name}(s);\n'
                 f'    log_call(entry_hash(s,0x{name}u));\n'
                 '}\n')
    defs += 'inline constexpr auto call_local = [](WorldState &value, auto child) {\n' + binding.DIRECT + '};\n}\n'
    fixture = (defs + 'namespace prepared {\nusing children::canonical_branch;\n' + replay +
               '\n}\nnamespace original {\n' + live + '\n}\n')

    accounted, manifest, _ = instrument_translation(ordinary, canonical, UNIT.relative_to(ROOT).as_posix(), ROOT)
    rows = {row['symbol']: row for row in manifest['functions']
            if row['symbol'] == binding.OWNER or row.get('canonical_reference_owner') == binding.OWNER}
    if ({key: row['literal_sites'] for key,row in rows.items()} != {binding.OWNER:167, binding.REFERENCE:167} or
            rows[binding.REFERENCE].get('canonical_reference_owner') != binding.OWNER):
        raise ValueError('Particle location update accounting alias drifted')
    return ordinary, accounted, fixture, rows, sorted(sites(program)), {name:sha(body) for name,body in children.items()}


def run(binary, count, mode):
    env = os.environ.copy()
    for flag in ('HALO2_REFERENCE_PARTICLE_LOCATION_UPDATE', 'HALO2_REFERENCE_PHYSICS_WORLD_LEAVES',
                 'HALO2_VERIFY_PHYSICS_WORLD_LEAVES', 'HALO2_CENSUS_NATIVE_BRIDGES'):
        env.pop(flag, None)
    return subprocess.run([str(binary), str(count), mode], cwd=binary.parent, env=env,
                          capture_output=True, text=True, timeout=90)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default='c++')
    parser.add_argument('--xbe', required=True, type=Path)
    parser.add_argument('--report', required=True, type=Path)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    calls = image(args.xbe)
    ordinary, accounted, fixture, rows, published, children = prepare()
    command = [args.compiler, '-std=c++20', '-O2', '-fno-fast-math', '-ffp-contract=off', '-frounding-math',
               f'-I{ROOT / "src"}', f'-I{ROOT / "include"}', f'-I{scratch}']
    for name, source in (('ordinary', ordinary), ('accounted', accounted)):
        cpp = scratch / f'{name}.cpp'
        cpp.write_text(source)
        subprocess.run([*command, '-fsyntax-only', str(cpp)], cwd=scratch, check=True)
    (scratch / 'particle_location_update_reference.inc').write_text(fixture)
    results = {}
    for variant, count, flags in (('release', 1200, []), ('sanitized', 300, [
            '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer'])):
        binary = scratch / variant
        subprocess.run([*command, *flags, str(TEST), '-o', str(binary)], cwd=scratch, check=True)
        results[variant] = {}
        for mode in MODES:
            result = run(binary, count, mode)
            if result.returncode:
                raise RuntimeError(f'{variant} {mode}:\n{result.stderr}')
            report = results[variant][mode] = json.loads(result.stdout)
            if mode == 'verify':
                hit = re.search(r'Particle location update verify: (\d+) calls', result.stderr)
                if not hit or int(hit[1]) != count:
                    raise ValueError(f'Particle location update missed a normal verification replay: {result.stderr}')
                report['verified'] = int(hit[1])
    rejected = []
    header = HEADER.read_text()
    for name, before, after in MUTANTS:
        if header.count(before) != 1:
            raise ValueError(f'Particle location update mutant {name} needs one source anchor')
        directory = scratch / name
        target = directory / HEADER.relative_to(ROOT / 'src')
        target.parent.mkdir(parents=True)
        target.write_text(header.replace(before, after, 1))
        binary = directory / 'probe'
        subprocess.run([command[0], f'-I{directory}', *command[1:], str(TEST), '-o', str(binary)], cwd=scratch, check=True)
        result = run(binary, 96, 'literal')
        if result.returncode != 1 or 'differential mismatch' not in result.stderr:
            raise ValueError(f'Particle location update accepted mutant {name}: {result.stderr}')
        rejected.append(name)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(dict(passed=True, owned_scratch=str(scratch), retained_large_artifacts=[],
        source_hashes={str(path.relative_to(ROOT)): sha(path.read_bytes()) for path in
                       (UNIT, HEADER, TEST, Path(binding.__file__), Path(__file__))},
        image_sha256=IMAGE, machine_hash=MACHINE, jump_table=TABLE, direct_targets=calls, manifest=rows,
        literal_sites=published, exact_child_source_pins=children, controlled_child_boundaries=CONTROLLED_CHILDREN,
        production_syntax=dict(ordinary=sha(ordinary), accounted=sha(accounted)),
        comparisons=results, rejected_source=['selector', 'child'], rejected_semantics=rejected), indent=2) + '\n')
    print('TU1.5 particle location update passed its complete pinned owner and child ABI comparisons.')


if __name__ == '__main__':
    main()
