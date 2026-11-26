#!/usr/bin/env python3
"""Prove the native common 252480 property setup and velocity return."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

from instrument_native_execution import instrument_translation
import native_particle_velocity_properties_replacements as binding
import native_particle_velocity_remaining_replacements as remaining
from specialize_native_operands import specialize


ROOT = Path(__file__).resolve().parents[1]
UNIT = ROOT / 'src/gameplay/units/death_cycle_runtime_tu15_generated.cpp'
HEADER = ROOT / 'src/rendering/effects/native_tu15_particle_velocity_properties.hpp'
TEST = ROOT / 'tests/native_tu15_particle_velocity_properties_tests.cpp'
IMAGE = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
MACHINE = 'dda63ebe7d46140cf7650a9aa7f10c50d22cf5c9da8441ae66cea55ba9836ab8'
TABLE = 'd4242500972525002826250072262500ee262500ac272500b0282500122a2500192b2500'
MODES = ('literal', 'census', 'verify', 'collector')
MUTANTS = (
    ('lifetime_clamp', 'if (flags.cf || flags.zf) x[0] = x[1];', 'if (flags.cf && flags.zf) x[0] = x[1];'),
    ('finish_call', '0x252d4cu, 0x252d51u', '0x252d4cu, 0x252d52u'),
    ('estimate_operand', 'std::bit_cast<float>(estimate(input))', 'std::bit_cast<float>(estimate(input ^ 0x8000u))'),
    ('velocity_lane', 'scalar(0x252d5fu, gp[ebp] + 4u)', 'scalar(0x252d5fu, gp[ebp] + 8u)'),
    ('return', 'port.ret(last, argument_size);', 'port.ret(last, 0u);'),
)


def sha(value):
    return hashlib.sha256(value.encode() if isinstance(value, str) else value).hexdigest()


def image(path):
    sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
    from xbe import Xbe

    reader = Xbe(path)
    if (sha(reader.data) != IMAGE or sha(reader.read(0x252480,0x904)) != MACHINE or
            reader.read(0x252d84,36).hex() != TABLE):
        raise ValueError('Particle velocity property owner or nine-mode table drifted')
    for site, target in ((0x250e98,0x252480), (0x25248f,0x24ffe0), (0x2524b8,0x24ffe0), (0x252d4c,0x24ffe0)):
        call = reader.read(site,5)
        if call[0] != 0xe8 or site + 5 + int.from_bytes(call[1:], 'little', signed=True) != target:
            raise ValueError(f'Particle velocity property CALL at {site:x} drifted')


def literal_sites(body):
    return set(re.findall(r'\bs\.instruction_literal\(0x([0-9a-f]+)u\)', body))


def prepare():
    canonical = UNIT.read_text()
    ordinary, receipt = specialize(canonical)
    complete = binding.OWNER in receipt.get('native_spatial_owners', [])
    partial = binding.OWNER in receipt.get('native_partial_frame_owners', {})
    if (not binding.admitted(canonical) or complete == partial or binding.install(canonical, ordinary) != (ordinary,[])):
        raise ValueError('Expected both production particle velocity property hooks')
    owner = binding.world.body(ordinary, binding.OWNER)[0]
    fallback = owner.replace(binding.PREFIX_HOOK,'',1).replace(binding.SWITCH_LABEL,'',1)
    if complete:
        if fallback.count(remaining.selection_hook()) != 1:
            raise ValueError('Complete particle velocity owner lacks its authenticated switch lookup')
        fallback = fallback.replace(remaining.selection_hook(), binding.SWITCH + ' switch(' + remaining.LOOKUP + ')', 1)
    for value in ('true','false'):
        fallback = fallback.replace(binding.suffix_hook(value),'',1)
    start, end, tail, last = binding.extract(fallback)
    prefix, suffix = fallback[start:end], fallback[tail:last]
    piece_sites = {'prefix': literal_sites(prefix), 'finish': literal_sites(suffix)}
    included = set.union(*piece_sites.values())
    original = literal_sites(owner)
    if tuple(map(len,piece_sites.values())) != (20,45) or len(included) != 65 or len(original) != 579 or not included < original:
        raise ValueError('Particle velocity property spans lost their exact disjoint site sets')
    piece_sites['retained'] = original - included
    refs = binding.prefix_reference(prefix) + binding.suffix_reference(suffix)
    if any(ref not in ordinary for ref in (binding.prefix_reference(prefix),binding.suffix_reference(suffix))):
        raise ValueError('Prepared particle velocity property replay text drifted')
    live = binding.prefix_reference(prefix,replay=False) + binding.suffix_reference(suffix,replay=False)
    curve = binding.world.body(ordinary,'24ffe0')[0]
    start = curve.index('s.instruction_literal(')
    child = curve[:curve.index('{')+1] + '\n' + curve[start:]
    spill = re.search(r'^std::uint32_t native_single_spill\(.*?^}', canonical, re.S | re.M)[0]
    fixture = ('namespace children {\n' + spill + '\n' + child +
               '\ninline constexpr auto curve_call = [](WorldState &s,auto){run_24ffe0(s);};\n}\n'
               'namespace source {\nusing children::run_24ffe0;\n' + refs + live + '}\n')
    accounted, manifest, _ = instrument_translation(ordinary, canonical, UNIT.relative_to(ROOT).as_posix(), ROOT)
    rows = {row['symbol']: row for row in manifest['functions']
            if row['symbol'] in (binding.OWNER,binding.PREFIX,binding.SUFFIX)}
    if {name:row['literal_sites'] for name,row in rows.items()} != {binding.OWNER:579,binding.PREFIX:20,binding.SUFFIX:45}:
        raise ValueError('Particle velocity property literal accounting changed')
    return ordinary, accounted, fixture, rows, piece_sites, sha(child)


def run(binary, cases, mode):
    environment = os.environ.copy()
    for flag in ('HALO2_REFERENCE_PARTICLE_VELOCITY_PROPERTIES','HALO2_REFERENCE_PHYSICS_WORLD_LEAVES',
                 'HALO2_VERIFY_PHYSICS_WORLD_LEAVES','HALO2_CENSUS_NATIVE_BRIDGES'):
        environment.pop(flag, None)
    return subprocess.run([str(binary),str(cases),mode], cwd=binary.parent, env=environment,
                          text=True, capture_output=True, timeout=90)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default='c++')
    parser.add_argument('--xbe', required=True, type=Path)
    parser.add_argument('--report', required=True, type=Path)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    image(args.xbe)
    ordinary, accounted, fixture, rows, sites, child = prepare()
    command = [args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
               f'-I{ROOT / "src"}',f'-I{ROOT / "include"}',f'-I{scratch}']
    for name, source in (('ordinary',ordinary),('accounted',accounted)):
        cpp = scratch / f'{name}.cpp'
        cpp.write_text(source)
        result = subprocess.run([*command,'-fsyntax-only',str(cpp)], cwd=scratch, text=True, capture_output=True)
        if result.returncode:
            raise RuntimeError(result.stderr)
    (scratch / 'particle_velocity_properties_reference.inc').write_text(fixture)
    comparisons = {}
    for name, count, flags in (('release',900,[]),('sanitized',300,[
            '-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer'])):
        binary = scratch / name
        subprocess.run([*command,*flags,str(TEST),'-o',str(binary)], cwd=scratch, check=True)
        comparisons[name] = {}
        for mode in MODES:
            result = run(binary,count,mode)
            if result.returncode:
                raise RuntimeError(f'{name} {mode}:\n{result.stderr}')
            row = comparisons[name][mode] = json.loads(result.stdout)
            if mode == 'verify':
                hit = re.search(r'Particle velocity properties verify: (\d+) prefixes, (\d+) suffixes', result.stderr)
                if not hit or int(hit[1]) != count // 3 or not 0 < int(hit[2]) <= (count * 2) // 3:
                    raise ValueError(f'Particle velocity properties missed replay coverage: {result.stderr}')
                row['verified'] = dict(prefixes=int(hit[1]), suffixes=int(hit[2]))
    rejected = []
    header = HEADER.read_text()
    for name, before, after in MUTANTS:
        if header.count(before) != 1:
            raise ValueError(f'Particle velocity property mutant {name} needs one source anchor')
        directory = scratch / name
        target = directory / HEADER.relative_to(ROOT / 'src')
        target.parent.mkdir(parents=True)
        target.write_text(header.replace(before,after,1))
        binary = directory / 'probe'
        subprocess.run([command[0],f'-I{directory}',*command[1:],str(TEST),'-o',str(binary)], cwd=scratch, check=True)
        result = run(binary,63,'literal')
        if result.returncode != 1 or 'differential mismatch' not in result.stderr:
            raise ValueError(f'Particle velocity property accepted mutant {name}: {result.stderr}')
        rejected.append(name)
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(dict(passed=True, owned_scratch=str(scratch), retained_large_artifacts=[],
        source_hashes={str(path.relative_to(ROOT)):sha(path.read_bytes()) for path in
                       (UNIT,HEADER,TEST,Path(binding.__file__),Path(__file__))},
        image_sha256=IMAGE, owner_machine_pin=MACHINE, jump_table=TABLE, manifest=rows,
        instruction_sites={name:sorted(pcs) for name,pcs in sites.items()},
        authored_curve_child_sha=child, syntax=dict(ordinary=sha(ordinary),accounted=sha(accounted)),
        comparisons=comparisons, rejected_semantics=rejected), indent=2) + '\n')
    print('TU1.5 particle velocity property setup and return passed exact pinned source and service replay.')


if __name__ == '__main__':
    main()
