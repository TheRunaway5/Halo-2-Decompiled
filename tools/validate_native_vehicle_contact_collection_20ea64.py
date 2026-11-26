#!/usr/bin/env python3
"""Check the original wheel contact collection phases and the production binding."""

import argparse
from collections import Counter
import json
import os
from pathlib import Path
import re
import sys

from instrument_native_execution import instrument_translation
import native_vehicle_contact_collection_20ea64_replacements as binding
from specialize_native_operands import specialize
from validate_native_vehicle_wheel_offset_20e414 import COMPOSITION, PROVIDER, ROOT, SOURCE, run


ADAPTER = 'src/physics/vehicles/native_tu15_vehicle_contact_collection_20ea64.hpp'
FIXTURE = 'tests/native_tu15_vehicle_contact_collection_20ea64_tests.cpp'
SITE_LISTS = ('arguments_sites', 'wheel_sites', 'contact_sites')
DELTA = 0x4900


def hardware_compare(binary, count, sites, scratch):
    from oracle_sandbox_first_person_hardware_tu15 import execute

    fixtures = json.loads(run([binary, str(count), 'hardware'], scratch))

    def abi(fields):
        result = dict(fields)
        result['xmm'] = [''.join(f'{word:08x}' for word in reversed(lane)) for lane in result.pop('xmm_words')]
        return result

    def expected_end(case_id):
        route = case_id // 8 % 4
        mode = case_id % 8
        if route == 0:
            return 0x20ea78
        if route == 1 and mode == 0:
            return 0x20f918
        if route in (1, 2):
            return 0x20f8f4 if mode in (1, 7) else 0x20ebd2
        if mode <= 1:
            return 0x20f6c1
        return 0x20ee92 if mode & 4 else 0x20ecf7

    cases = []
    for row in fixtures:
        finish = expected_end(row['id'])
        if row['exit_entry'] != finish:
            raise ValueError(f'Unexpected native contact collection exit in hardware case {row["id"]}')
        cases.append(dict(id=row['id'], entry=row['entry'], exit_entry=finish, return_address=row['return_address'],
                          entry_abi=abi(row['entry_abi']), initial_banks=row['initial_banks'], exit={}))
    port = lambda pc: int(pc, 16) + DELTA
    paired = dict(address_map={}, entries={f'{port(pc):x}': port(pc) for pc in binding.ENTRIES},
                  instruction_map={f'{port(pc):x}': port(pc) for pc in sites})
    with (scratch / 'original-i386.log').open('w+') as log:
        stdout = os.dup(1)
        try:
            sys.stdout.flush()
            os.dup2(log.fileno(), 1)
            result = execute(dict(image='1.5', cases=cases), paired, scratch / 'original-i386')
        except Exception:
            log.seek(0)
            print(log.read()[-4000:], file=sys.stderr)
            raise
        finally:
            os.dup2(stdout, 1)
            os.close(stdout)
    coverage, routes = set(), Counter()
    for original, native in zip(result['cases'], fixtures, strict=True):
        expected = abi(native['exit'])
        actual = dict(original['exit'], fs_base=expected['fs_base'])
        actual['eflags'] &= 0xcd5
        expected['eflags'] &= 0xcd5
        if any(actual[key] != value for key, value in expected.items()):
            fields = {key: (actual[key], value) for key, value in expected.items() if actual[key] != value}
            raise ValueError(f'Hardware case {original["id"]} ABI differs: {fields}')
        if original['writes'] != native['native_writes'] or original['final_banks'] != native['final_banks']:
            raise ValueError(f'Hardware case {original["id"]} stores or complete memory differ')
        coverage.update(original['instructions'])
        routes[f'{original["entry"]:x}->{original["exit_entry"]:x}'] += 1
    if count >= 32 and coverage != {port(pc) for pc in sites}:
        raise ValueError('Original hardware fixtures omitted a contact collection site')
    return dict(passed=True, cases=count, continuations=dict(routes), observed_static_sites=len(coverage),
        ordered_stores=sum(len(case['writes']) for case in result['cases']),
        final_memory_bytes=sum(len(bank['hex'])//2 for case in result['cases'] for bank in case['final_banks']),
        original_sha256=result['original_sha256'], original_instruction_patches=result['original_instruction_patches'],
        original_instruction_steps=result['single_stepped_original_instructions'], original_backend=result['oracle_backend'],
        xbox_hardware_proof=False)


def exports(canonical, prepared, name):
    text = []
    for index in range(len(binding.SPANS)):
        internal = '    if (entry==0x20a190u) goto L20a190;\n' if index == 1 else '    (void)entry;\n'
        source = prepared(canonical, index)
        exits = binding.EXITS[index]
        suffix = f'    return 0x{exits[0]}u;\n' + ''.join(f'L{pc}:\n    return 0x{pc}u;\n' for pc in exits[1:])
        text.append(f'static std::uint32_t {name}_{index}(MotionState &s,std::uint32_t entry) {{\n{internal}{source}{suffix}}}\n')
    text.append(f'static std::uint32_t {name}(MotionState &s,std::uint32_t entry) {{\n'
                f'    if (entry==0x20a164u) return {name}_0(s,entry);\n'
                f'    if (entry==0x20a2d7u) return {name}_2(s,entry);\n'
                f'    return {name}_1(s,entry);\n}}\n')
    return ''.join(text)


def qualify(args, scratch):
    canonical = (ROOT / SOURCE).read_text()
    alias, metadata = specialize(canonical)
    if metadata.get('native_spatial_owners', []).count('20ea64') != 1 or binding.install(canonical, alias) != (alias, ['20ea64']):
        raise ValueError('Vehicle 20EA64 production alias binding missing or changed')
    accounted, manifest, _ = instrument_translation(alias, canonical, SOURCE, ROOT)
    owner, = (row for row in manifest['functions'] if row['symbol'] == '209a50')
    if owner['literal_sites'] != 1452 or owner['dynamic_instruction_forwarders']:
        raise ValueError('Vehicle 209A50 production accounting changed')
    adapter = (ROOT / ADAPTER).read_text()
    source = (ROOT / COMPOSITION).read_text().split('vehicle_sites={{\n', 1)[1].split('}};', 1)[0]
    mapping = re.findall(r'\{0x([0-9a-f]+)u,0x([0-9a-f]+)u\}', source)
    sites = []
    for index, marker in enumerate(SITE_LISTS):
        original = re.findall(r'instruction_literal\(0x([0-9a-f]+)u\)', binding.reference(canonical, index))
        admitted = re.findall(r'0x([0-9a-f]+)u', adapter.split(f'{marker}[]{{', 1)[1].split('};', 1)[0])
        expected = [(pc, f'{int(pc, 16)+DELTA:x}') for pc in original]
        start = mapping.index(expected[0])
        if original != admitted or mapping[start:start+len(original)] != expected:
            raise ValueError(f'Vehicle 20EA64 phase {index} actual-site relocation mismatch')
        sites.extend(original)
    for exit in {pc for phase in binding.EXITS for pc in phase}:
        if (exit, f'{int(exit, 16)+DELTA:x}') not in mapping:
            raise ValueError(f'Vehicle 20EA64 exit {exit} relocation mismatch')

    (scratch / 'vehicle_contact_collection_original.inc').write_text(
        exports(canonical, binding.reference, 'retained') + exports(canonical, binding.bridge_stage, 'selected'))
    flags = ['-std=c++20', '-O2', '-g0', '-fno-fast-math', '-ffp-contract=off', '-frounding-math',
             *(f'-I{path}' for path in (ROOT / 'include', ROOT / 'src', ROOT / 'tests', scratch))]
    if args.full_syntax:
        for name, input_source in (('normal', alias), ('accounted', accounted)):
            path = scratch / f'{name}.cpp'
            path.write_text(input_source)
            run([args.compiler, *flags, '-fsyntax-only', path], scratch)
    profiles = {}
    hardware = None
    for name, count in (('release', args.cases), ('sanitized', args.sanitized_cases)):
        if not count and not (name == 'release' and args.hardware_cases):
            continue
        sanitizer = ['-O1', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-pie', '-no-pie']
        binary = scratch / f'contact-collection-{name}'
        run([args.compiler, *flags, *(sanitizer if name == 'sanitized' else []), ROOT / FIXTURE, '-o', binary], scratch)
        if count:
            profiles[name] = json.loads(run([binary, str(count), 'native'], scratch))
        if name == 'release' and args.hardware_cases:
            hardware = hardware_compare(binary, args.hardware_cases, sites, scratch)
    print(json.dumps(dict(passed=True, compiler=run([args.compiler, '--version'], scratch).splitlines()[0],
        source_sha256=binding.digest(canonical), alias_sha256=binding.digest(alias),
        native_header_sha256=binding.digest(adapter), provider_sha256=binding.digest((ROOT / PROVIDER).read_text()),
        full_production_syntax=args.full_syntax, mapped_sites=len(sites), profiles=profiles,
        hardware=hardware, scratch=str(scratch)), indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default='c++')
    parser.add_argument('--cases', type=int, default=128)
    parser.add_argument('--sanitized-cases', type=int, default=32)
    parser.add_argument('--hardware-cases', type=int, default=0)
    parser.add_argument('--full-syntax', action='store_true')
    args = parser.parse_args()
    scratch = os.getenv('HALO2_TEST_SCRATCH')
    if not scratch:
        sys.exit('Run through tools/run_test.py')
    qualify(args, Path(scratch))


if __name__ == '__main__':
    main()
