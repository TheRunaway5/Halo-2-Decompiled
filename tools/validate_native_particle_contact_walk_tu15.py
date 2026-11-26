#!/usr/bin/env python3
"""Verify the particle contact setup and eligibility walk against pinned Effect Frame."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

import native_particle_contact_flags_replacements as flags
import native_particle_contact_walk_replacements as binding
from instrument_native_execution import instrument_translation
from specialize_native_operands import specialize


ROOT = Path(__file__).resolve().parents[1]
UNIT = ROOT / 'src/rendering/effects/source_effect_particle_frame_tu15_generated.cpp'
HEADER = ROOT / 'src/rendering/effects/native_tu15_particle_contact_walk.hpp'
TEST = ROOT / 'tests/native_tu15_particle_contact_walk_tests.cpp'
IMAGE_PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
OWNER_PIN = '27c950f5aa9835178f48e1855943f4af4943a8994fca3ca67aeb5454e53997f3'
CHILDREN = (*binding.CHILDREN, '252100', '4efa0')
MODES = ('literal', 'census', 'verify', 'collector')
MUTANTS = (
    ('wrong_return', 'port.ret(last, caller_args);', 'port.ret(last, 0u);'),
    ('particle_slot', 'element_index = 0xffffu', 'element_index = 0x7fffu'),
    ('signed_age_gate', 'if (flags.cf) return Particle::skip;', 'if (flags.sf) return Particle::skip;'),
    ('counter_width', '13u, 2, false);', '13u, 1, false);'),
    ('wrong_enumeration_return', '0x2c2e25u, 0x2c2e2au', '0x2c2e25u, 0x2c2e2bu'),
)


def sha(value):
    return hashlib.sha256(value.encode() if isinstance(value, str) else value).hexdigest()


def image(path):
    sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
    from xbe import Xbe

    reader = Xbe(path)
    if sha(reader.data) != IMAGE_PIN or sha(reader.read(0x2c2d90, 0x601)) != OWNER_PIN:
        raise ValueError('Particle contact walk source image or owner bytes drifted')
    for site, target in ((0x2c2b01,0x2c2d90), (0x2c2dad,0x178070), (0x2c2db5,0x13a6b0),
                         (0x2c2e25,0x176a40), (0x2c2e60,0x24ffe0)):
        code = reader.read(site, 5)
        if code[0] != 0xe8 or site + 5 + int.from_bytes(code[1:], 'little', signed=True) != target:
            raise ValueError(f'Particle contact walk call at {site:x} drifted')


def literal_sites(source):
    return set(re.findall(r'\bs\.instruction_literal\(0x([0-9a-f]+)u\)', source))


def function(source, name):
    return re.search(r'\b' + name + r'\(WorldState &s[^\n]*\) \{.*?\n\}', source, re.S)[0]


def prepare():
    canonical = UNIT.read_text()
    prepared, receipt = specialize(canonical)
    if ((binding.OWNER not in receipt.get('native_partial_frame_owners', {}) and
            receipt.get('native_spatial_owners', []).count(binding.OWNER) != 1)
            or not binding.admitted(canonical)
            or binding.install(canonical, prepared) != (prepared, [])):
        raise ValueError('Particle contact walk expects its exact production binding')
    owner = binding.world.body(prepared, binding.OWNER)[0]
    owner = owner.replace(flags.HOOK, '', 1)
    fallback = owner.replace(binding.SETUP_HOOK, '', 1).replace(binding.COLLISION_LABEL, '', 1)
    for start in ('particle', 'advance', 'epilogue'):
        fallback = fallback.replace(binding.walk_hook(start), '', 1)
    start, particle, collision, countdown, end = binding.extract(fallback)
    source = (fallback[start:particle], fallback[particle:collision], fallback[countdown:end])
    cuts = [literal_sites(span) for span in source]
    if [len(row) for row in cuts] != [71, 18, 15] or len(set.union(*cuts)) != 104:
        raise ValueError('Particle contact walk source spans drifted')
    sites = {'setup': cuts[0], 'walk': cuts[1] | cuts[2], 'retained': literal_sites(fallback[collision:countdown])}
    if len(literal_sites(owner)) != 401 or len(sites['retained']) != 297:
        raise ValueError('Particle contact walk did not partition all owner sites')
    native_refs = binding.references(*source)
    if not all(ref in prepared for ref in native_refs) or set.union(*[literal_sites(ref) for ref in native_refs]) != set.union(*cuts):
        raise ValueError('Particle contact walk exact replay text diverged')
    mutant = canonical.replace(binding.world.body(canonical, binding.OWNER)[0],
                               binding.world.body(canonical, binding.OWNER)[0].replace('0x107f0u', '0x107e0u', 1), 1)
    if mutant == canonical or binding.admitted(mutant):
        raise ValueError('Particle contact walk accepted changed authored masks')

    accounted, manifest, _ = instrument_translation(prepared, canonical, UNIT.relative_to(ROOT).as_posix(), ROOT)
    static = {row['symbol']: row for row in manifest['functions']
              if row['symbol'] in (binding.OWNER, binding.SETUP, binding.WALK)}
    expected = {binding.OWNER: 401, binding.SETUP: 71, binding.WALK: 33}
    if {key: row['literal_sites'] for key, row in static.items()} != expected:
        raise ValueError('Particle contact walk missing owned accounting aliases')

    helpers = 'static std::uint32_t canonical_branch(std::uint32_t p, SourcePhysicsContactImage) { return p; }\n'
    helpers += re.search(r'^std::uint32_t native_single_spill\(.*?^}', canonical, re.S | re.M)[0] + '\n'
    # The authored curve service is outside Effect Frame. Exercise 24FFE0's
    # actual continuation through it with a deterministic authored result.
    helpers += '''static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail) {
    if (target != 0x13de70u || site != 0x250004u || tail)
        throw std::runtime_error("Unexpected particle contact fixture dispatch");
    log_call(entry_hash(s,target));
    s.xmm[0] = _mm_set_ss(0.25f);
    s.pop();
    s.gp[4] += 8u;
}
'''
    children = {}
    for name in CHILDREN:
        child = binding.world.body(prepared, name)[0]
        begin = child.index('s.instruction_literal(')
        child = child[:child.index('{')+1] + '\n' + child[begin:]
        children[name] = child.replace(f'run_{name}(', f'body_{name}(', 1)
    child_inc = 'namespace children {\n' + helpers
    child_inc += ''.join(f'static void run_{name}(WorldState &);\n' for name in CHILDREN)
    child_inc += '\n'.join(children.values()) + '\n'
    for name in CHILDREN:
        child_inc += (f'static void run_{name}(WorldState &s) {{\n'
                      f'    log_call(entry_hash(s,0x{name}u));\n'
                      f'    body_{name}(s);\n'
                      f'    log_call(entry_hash(s,0x{name}u));\n'
                      '}\n')
    child_inc += 'inline constexpr auto call_local = [](WorldState &value, auto child) {\n'
    child_inc += binding.DIRECT + '};\n}\n'
    live = '\n'.join(binding.references(*source, replay=False))
    for target in binding.CHILDREN:
        live = live.replace(f'run_{target}(s);', f'children::run_{target}(s);')
    refs = (child_inc + 'namespace prepared {\n' + '\n'.join(native_refs) + '}\n'
            'namespace original {\n' + live + '}\n')
    return prepared, accounted, refs, static, sites, {name: sha(body) for name, body in children.items()}


def run(binary, count, mode):
    env = os.environ.copy()
    for flag in ('HALO2_REFERENCE_PARTICLE_CONTACT_WALK', 'HALO2_REFERENCE_PHYSICS_WORLD_LEAVES',
                 'HALO2_VERIFY_PHYSICS_WORLD_LEAVES', 'HALO2_CENSUS_NATIVE_BRIDGES'):
        env.pop(flag, None)
    return subprocess.run([str(binary), str(count), mode], cwd=binary.parent, env=env,
                          capture_output=True, text=True, timeout=90)


def results(binary, cases):
    data = {}
    for mode in MODES:
        completed = run(binary, cases, mode)
        if completed.returncode:
            raise RuntimeError(f'{binary.name} {mode}:\n{completed.stderr}')
        row = data[mode] = json.loads(completed.stdout)
        if mode == 'verify':
            counts = re.search(r'Particle contact walk verify: (\d+) setup, (\d+) traversal', completed.stderr)
            if not counts or not all(map(int, counts.groups())):
                raise ValueError('Particle contact walk skipped a verified span')
            row['verified'] = dict(zip(('setup', 'walk'), map(int, counts.groups())))
    return data


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default='c++')
    parser.add_argument('--xbe', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    image(args.xbe)
    ordinary, accounted, refs, manifest, sites, children = prepare()
    command = [args.compiler, '-std=c++20', '-O2', '-fno-fast-math', '-ffp-contract=off', '-frounding-math',
               f'-I{ROOT / "src"}', f'-I{ROOT / "include"}', f'-I{scratch}']
    for name, source in (('ordinary', ordinary), ('accounted', accounted)):
        cpp = scratch / f'{name}.cpp'
        cpp.write_text(source)
        subprocess.run([*command, '-fsyntax-only', str(cpp)], cwd=scratch, check=True)
    (scratch / 'particle_contact_walk_reference.inc').write_text(refs)
    fixture = {}
    for name, count, flags in (('release', 1600, []), ('sanitized', 400, [
            '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer'])):
        binary = scratch / name
        subprocess.run([*command, *flags, str(TEST), '-o', str(binary)], cwd=scratch, check=True)
        fixture[name] = results(binary, count)
    rejected = []
    original = HEADER.read_text()
    for name, before, after in MUTANTS:
        if original.count(before) != 1:
            raise ValueError(f'Particle contact walk mutant {name} needs one source anchor')
        override = scratch / name
        target = override / HEADER.relative_to(ROOT / 'src')
        target.parent.mkdir(parents=True)
        target.write_text(original.replace(before, after, 1))
        binary = override / 'probe'
        subprocess.run([command[0], f'-I{override}', *command[1:], str(TEST), '-o', str(binary)], cwd=scratch, check=True)
        completed = run(binary, 90, 'literal')
        if completed.returncode != 1 or 'differential mismatch' not in completed.stderr:
            raise ValueError(f'Particle contact walk accepted mutant {name}: {completed.stderr}')
        rejected.append(name)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(dict(passed=True, owned_scratch=str(scratch), retained_large_artifacts=[],
        source_hashes={str(path.relative_to(ROOT)): sha(path.read_bytes()) for path in
                       (UNIT, HEADER, TEST, Path(binding.__file__), Path(__file__))},
        image_pin=IMAGE_PIN, machine_owner=OWNER_PIN, children=children, manifest=manifest,
        production_syntax={'ordinary': sha(ordinary), 'accounted': sha(accounted)},
        instruction_sites={name: sorted(group) for name, group in sites.items()}, fixtures=fixture,
        rejected_semantics=rejected), indent=2) + '\n')
    print('TU1.5 particle contact setup, eligibility, countdown and return matched the pinned source.')


if __name__ == '__main__':
    main()
