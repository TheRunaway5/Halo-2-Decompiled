#!/usr/bin/env python3
"""Check the TU1.5 contact query stage and its production alias."""

import argparse
from collections import Counter
import json
import os
from pathlib import Path
import re
import sys

import native_vehicle_contact_query_20f3a8_replacements as binding
from instrument_native_execution import instrument_translation
from specialize_native_operands import specialize
from validate_native_vehicle_wheel_offset_20e414 import COMPOSITION, FIXTURE, PROVIDER, ROOT, SOURCE, run


ADAPTER = 'src/physics/vehicles/native_tu15_vehicle_contact_query_20f3a8.hpp'
REFERENCE = 'HALO2_TEST_REFERENCE_VEHICLE_CONTACT_QUERY'


def hardware_compare(binary, count, sites, scratch, env):
    from oracle_sandbox_first_person_hardware_tu15 import execute

    fixtures = json.loads(run([binary, str(count), 'hardware'], scratch, env))

    def format_abi(abi):
        abi = dict(abi)
        abi['xmm'] = [''.join(f'{word:08x}' for word in reversed(lane)) for lane in abi.pop('xmm_words')]
        return abi

    cases = []
    for row in fixtures:
        entry = row['entry']
        expected_exit = 0x20f6e0 if row['id'] % 8 < 2 else 0x20f631
        assert row['exit_entry'] == expected_exit
        cases.append(dict(id=row['id'], entry=entry, exit_entry=expected_exit, return_address=row['return_address'],
            entry_abi=format_abi(row['entry_abi']), initial_banks=row['initial_banks'], exit={}))
    actual_sites = [int(pc, 16) + 0x4900 for pc in sites]
    paired = dict(address_map={}, entries={f'{p:x}': p for p in {row['entry'] for row in cases}},
                  instruction_map={f'{p:x}': p for p in actual_sites})
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
    totals = Counter()
    branch_coverage = set()
    for original, native in zip(result['cases'], fixtures, strict=True):
        expected = format_abi(native['exit'])
        actual = dict(original['exit'], fs_base=expected['fs_base'])
        expected['eflags'] &= 0xcd5
        actual['eflags'] &= 0xcd5
        if any(actual[key] != value for key, value in expected.items()):
            fields = {key: (actual[key], value) for key, value in expected.items() if actual[key] != value}
            raise ValueError(f'Hardware case {original["id"]} ABI differs: {fields}')
        if original['writes'] != native['native_writes'] or original['final_banks'] != native['final_banks']:
            raise ValueError(f'Hardware case {original["id"]} ordered stores or final memory differ')
        totals[f'{original["entry"]:x}->{original["exit_entry"]:x}'] += 1
        branch_coverage.update(original['instructions'])
    return dict(passed=True, cases=count, continuations=dict(totals), original_sha256=result['original_sha256'],
        original_instruction_patches=result['original_instruction_patches'],
        original_instruction_steps=result['single_stepped_original_instructions'],
        observed_static_sites=len(branch_coverage),
        ordered_stores=sum(len(case['writes']) for case in result['cases']),
        final_memory_bytes=sum(len(bank['hex'])//2 for case in result['cases'] for bank in case['final_banks']),
        original_backend=result['oracle_backend'], xbox_hardware_proof=False)


def qualify(args, scratch):
    canonical = (ROOT / SOURCE).read_text()
    alias, metadata = specialize(canonical)
    if metadata.get('native_spatial_owners', []).count('20f3a8') != 1:
        raise ValueError('Production specialization did not bind vehicle contact query')
    if binding.install(canonical, alias) != (alias, ['20f3a8']):
        raise ValueError('Vehicle contact query binder is not idempotent')
    accounted, manifest, _ = instrument_translation(alias, canonical, SOURCE, ROOT)
    owner, = (row for row in manifest['functions'] if row['symbol'] == '209a50')
    if owner['literal_sites'] != 1452 or owner['dynamic_instruction_forwarders']:
        raise ValueError('Vehicle 209A50 production accounting changed')

    stage = binding.reference(canonical)
    adapter = (ROOT / ADAPTER).read_text()
    sites = re.findall(r'instruction_literal\(0x([0-9a-f]+)u\)', stage)
    admitted = re.findall(r'0x([0-9a-f]+)u', adapter.split('sites[]{', 1)[1].split('};', 1)[0])
    source = (ROOT / COMPOSITION).read_text().split('vehicle_sites={{\n', 1)[1].split('}};', 1)[0]
    mappings = re.findall(r'\{0x([0-9a-f]+)u,0x([0-9a-f]+)u\}', source)
    expected = [(p, f'{int(p, 16) + 0x4900:x}') for p in sites]
    start = mappings.index(expected[0])
    if sites != admitted or mappings[start:start+len(sites)] != expected or any(
        (pc, f'{int(pc, 16)+0x4900:x}') not in mappings for pc in binding.EXITS
    ):
        raise ValueError('Vehicle contact query composition relocation changed')

    def body(name, text):
        entries = ''.join(f'    case 0x{pc}u: goto L{pc};\n' for pc in binding.ENTRIES)
        exits = ''.join(f'L{pc}:\n    return 0x{pc}u;\n' for pc in binding.EXITS)
        return (f'static std::uint32_t {name}(MotionState &s, std::uint32_t entry) {{\n'
                f'    switch (entry) {{\n{entries}    default: throw std::logic_error("Invalid test entry");\n    }}\n'
                f'{text}{exits}}}\n')

    bound = binding.bridge_stage(canonical)
    bridge = 'halo2::native_vehicle_contact_query_20f3a8_detail::bridge(s,contact_query_entry)'
    assert bound.count(bridge) == 1
    selected = bound.replace(bridge, f'std::getenv("{REFERENCE}") ? 0u : {bridge}')
    (scratch / 'vehicle_wheel_offset_owner.inc').write_text(body('retained', stage) + body('selected', selected))

    flags = ['-std=c++20', '-O2', '-g0', '-fno-fast-math', '-ffp-contract=off', '-frounding-math',
             *(f'-I{path}' for path in (ROOT / 'include', ROOT / 'src', ROOT / 'tests', scratch))]
    if args.full_syntax:
        for name, source in (('normal', alias), ('accounted', accounted)):
            path = scratch / f'{name}.cpp'
            path.write_text(source)
            run([args.compiler, *flags, '-fsyntax-only', path], scratch)
    profiles = {}
    hardware = None
    env = {key: value for key, value in os.environ.items() if key != REFERENCE}
    for name, count in (('release', args.cases), ('sanitized', args.sanitized_cases)):
        if not count and not (name == 'release' and args.hardware_cases):
            continue
        sanitizer = ['-O1', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-pie', '-no-pie']
        binary = scratch / f'contact-query-{name}'
        run([args.compiler, *flags, '-DHALO2_TEST_VEHICLE_CONTACT_QUERY',
             *(sanitizer if name == 'sanitized' else []), ROOT / FIXTURE, '-o', binary], scratch)
        if count:
            profiles[name] = {mode: json.loads(run([binary, str(count), mode], scratch, env))
                              for mode in ('native', 'reference')}
        if name == 'release' and args.hardware_cases:
            hardware = hardware_compare(binary, args.hardware_cases, sites, scratch, env)
    print(json.dumps(dict(passed=True, compiler=run([args.compiler, '--version'], scratch).splitlines()[0],
        source_sha256=binding.digest(canonical), alias_sha256=binding.digest(alias),
        native_header_sha256=binding.digest(adapter), provider_sha256=binding.digest((ROOT / PROVIDER).read_text()),
        raw_stage_sha256=binding.RAW_PIN, prepared_stage_sha256=binding.PREPARED_PIN,
        full_production_syntax=args.full_syntax, mapped_sites=len(sites), profiles=profiles,
        hardware=hardware, scratch=str(scratch)), indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default='c++')
    parser.add_argument('--cases', type=int, default=128)
    parser.add_argument('--sanitized-cases', type=int, default=24)
    parser.add_argument('--hardware-cases', type=int, default=0)
    parser.add_argument('--full-syntax', action='store_true')
    args = parser.parse_args()
    scratch = os.getenv('HALO2_TEST_SCRATCH')
    if not scratch:
        sys.exit('Run through tools/run_test.py')
    qualify(args, Path(scratch))


if __name__ == '__main__':
    main()
