#!/usr/bin/env python3
"""Verify the selected TU1.5 descriptor refresh and due-response call regions."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

from instrument_native_execution import instrument_translation
import native_damage_descriptor_refresh_replacements as descriptor
import native_damage_due_response_replacements as due
import native_damage_response_amount_replacements as amount
import native_damage_response_secondary_replacements as secondary
from specialize_native_operands import specialize


ROOT = Path(__file__).resolve().parents[1]
UNIT = ROOT / 'src/gameplay/units/source_damage_delivery_runtime_generated.cpp'
SECTIONS = {'descriptor': (descriptor, 'damage_descriptor_refresh', 'd84f4', 35),
            'due': (due, 'damage_due_response', 'd842c', 52),
            'amount': (amount, 'damage_response_amount', 'd88f9', 60),
            'secondary': (secondary, 'damage_response_secondary', 'd87ce', 70)}
CHILD_PINS = {
    'descriptor': {
        '1e83a0': 'b83e144c7caca9e1aed6e3ffb3e3983b021c285ac6170f8251a094152cc7ab3a',
        'd7cd0': '3334a7c0544aa72b6805465d8578fd263609678d097aeddc65ce80fad382704e',
        'd7e00': 'e311f5c8d1670688549238ed9f2dbdc9af6aea3a3f5d479487f07fbf97a1dbca',
    },
    'due': {
        'db570': 'aa8c513903ac2ea6e2954ecdddf615914a31c19a8823bf43bd6e7384b3b9bcad',
        'db710': 'd149ea59b920d80f3da94f975da5984b17c04f24ffb85203ade61a2b6af95fb8',
    },
    'amount': {
        '139ff0': '2cc1da45b428fd34234271228c02688a22a7ebc3421b461189231c886556c5e7',
        'a87e0': '47f609ab56066244bf8ba2cb5e62b444b44b26f8b315606694e288bdad744812',
        '1ed030': '871ec9fb07718808216aee7ebbbfe427495edaf660a7823a1a8ffd2eb60a19dc',
        '1ed0b0': 'b9d19031010c5bc023360ddbbab9edb586976b9d0b9eef531b2a16ff6efe3e11',
    },
    'secondary': {
        'a87e0': '47f609ab56066244bf8ba2cb5e62b444b44b26f8b315606694e288bdad744812',
        'd7cd0': '3334a7c0544aa72b6805465d8578fd263609678d097aeddc65ce80fad382704e',
        '1ed030': '871ec9fb07718808216aee7ebbbfe427495edaf660a7823a1a8ffd2eb60a19dc',
        '1ed0b0': 'b9d19031010c5bc023360ddbbab9edb586976b9d0b9eef531b2a16ff6efe3e11',
        '1781f0': '4b68d3a13917ab774f1ec912802529640f4008ae401a75131298b1289be215b7',
    },
}
MODES = ('literal', 'census', 'verify', 'reference')


def sha(text):
    return hashlib.sha256(text.encode()).hexdigest()


def extract_children(canonical, section):
    children = []
    internal = {'1ed030'} if section in ('amount', 'secondary') else set()
    for name, pin in CHILD_PINS[section].items():
        body = re.search(rf'^static void run_{name}\(ObservedDamageState &s\) \{{.*?^\}}(?=\n\nstatic void run_)',
                         canonical, re.M | re.S)
        if not body or sha(body[0]) != pin:
            raise ValueError(f'Damage fixture child {name} changed')
        children.append(body[0] if name in internal else
                        body[0].replace(f'static void run_{name}(', f'static void fixture_{name}(', 1))
    nested = sorted({child for body in children for child in re.findall(r'\brun_([0-9a-f]+)\(s\);', body)} - internal)
    definitions = []
    for child in nested:
        # A successful fixture cannot silently use an unqualified nested route.
        definitions.append(f'static void run_{child}(ObservedDamageState &) {{ '
                           f'throw std::logic_error("Unqualified damage child {child} reached"); }}')
    selectors = ['selected']
    if section == 'descriptor':
        definitions.append('static void dispatch(ObservedDamageState &, std::uint32_t, std::uint32_t, bool) { '
                           'throw std::logic_error("Unqualified D8D50 dispatch reached"); }')
    elif section == 'due':
        selectors.append('canonical_branch')
    else:
        for operator in ('damage_mulss', 'damage_addss'):
            helper = re.search(rf'^__m128 {operator}\([^\n]+\n.*?^\}}', canonical, re.M | re.S)
            if not helper:
                raise ValueError(f'Damage amount fixture is missing {operator}')
            definitions.append(helper[0])
    mappings = [re.search(rf'^std::uint32_t {selector}\([^\n]+$', canonical, re.M)[0] for selector in selectors]
    return '\n'.join(mappings + definitions + children) + '\n'


def run(argv, scratch, **kwargs):
    argv = [str(arg) for arg in argv]
    result = subprocess.run(argv, cwd=scratch, text=True, **kwargs)
    if result.returncode:
        raise RuntimeError(f'{argv[0]} failed with status {result.returncode}: {result.stderr or ""}')
    return result


def validate(canonical, prepared, metadata, scratch, compiler, sections, cases, sanitized_cases):
    spatial = set(metadata.get('native_spatial_owners', ()))
    partial = set(metadata.get('native_partial_frame_owners', ()))
    if 'd8330' not in spatial | partial:
        raise ValueError('The selected D8330 production owner is missing')
    owner = descriptor.owner.body(canonical)[1]
    for section in sections:
        binding, stem, pc, _ = SECTIONS[section]
        reference = binding.reference(owner)
        if (not binding.admitted(canonical) or prepared.count(reference) != 1 or pc in spatial or
                binding.install(canonical, prepared) != (prepared, [])):
            raise ValueError(f'Expected one authenticated native damage {section} binding')
        (scratch / f'{stem}_reference.inc').write_text(reference + binding.reference(owner, replay=False))
        (scratch / f'{stem}_children.inc').write_text(extract_children(canonical, section))

    relative = UNIT.relative_to(ROOT).as_posix()
    accounted, manifest, _ = instrument_translation(prepared, canonical, relative, ROOT)
    flags = ['-std=c++20', '-O2', '-g0', '-fno-fast-math', '-ffp-contract=off', '-frounding-math',
             *(f'-I{path}' for path in (ROOT / 'include', ROOT / 'src', ROOT / 'tests', scratch))]
    for name, text in (('normal', prepared), ('accounted', accounted)):
        alias = scratch / f'{name}.cpp'
        alias.write_text(text)
        run([compiler, *flags, '-fsyntax-only', alias], scratch)

    results = {}
    for section in sections:
        binding, stem, pc, sites = SECTIONS[section]
        reference = [row for row in manifest['functions'] if row['symbol'] == binding.REFERENCE]
        if (len(reference) != 1 or reference[0]['canonical_reference_owner'] != 'd73e0' or
                reference[0]['literal_sites'] != sites):
            raise ValueError(f'Wrong damage {section} reference accounting')
        suite = ROOT / f'tests/native_tu15_{stem}_tests.cpp'
        sources = [suite]
        link = []
        if section == 'secondary':
            sources.append(ROOT / 'src/gameplay/weapons/native_weapon_effect_dispatch_tu15.cpp')
            link = ['-ffunction-sections', '-fdata-sections', '-Wl,--gc-sections']
        profiles = {}
        for profile, rows in (('release', cases), ('sanitized', sanitized_cases)):
            if not rows:
                continue
            binary = scratch / f'{section}-{profile}'
            extra = ['-O1', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                     '-fno-omit-frame-pointer', '-fno-pie', '-no-pie'] if profile == 'sanitized' else []
            run([compiler, *flags, *extra, *link, *sources, '-o', binary], scratch)
            profiles[profile] = {
                mode: json.loads(run([binary, rows, mode], scratch, capture_output=True).stdout)
                for mode in MODES}
        header = ROOT / f'src/gameplay/units/native_tu15_{stem}.hpp'
        sources.extend((header, Path(binding.__file__), Path(__file__)))
        results[section] = {
            'actual_entry': pc, 'reference_sites': sites, 'genuine_extracted_children': CHILD_PINS[section],
            'source_sha256': {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
                              for path in sources}, 'results': profiles}
    return {'passed': True, 'canonical_sha256': sha(canonical), 'reference_pin': descriptor.owner.PIN,
            'normal_alias_sha256': sha(prepared), 'accounted_alias_sha256': sha(accounted), 'sections': results}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default='c++')
    parser.add_argument('--section', choices=(*SECTIONS, 'both'), default='both')
    parser.add_argument('--cases', type=int, default=6000)
    parser.add_argument('--sanitized-cases', type=int, default=900)
    parser.add_argument('--report', required=True, type=Path)
    args = parser.parse_args()
    owned = os.environ.get('HALO2_TEST_SCRATCH')
    if not owned or args.cases < 18 or (args.sanitized_cases and args.sanitized_cases < 18):
        parser.error('Use tools/run_test.py with at least 18 cases per enabled profile')
    canonical = UNIT.read_text()
    prepared, metadata = specialize(canonical)
    sections = ('descriptor', 'due') if args.section == 'both' else (args.section,)
    report = validate(canonical, prepared, metadata, Path(owned), args.compiler, sections,
                      args.cases, args.sanitized_cases)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS selected TU1.5 damage response routes')


if __name__ == '__main__':
    main()
