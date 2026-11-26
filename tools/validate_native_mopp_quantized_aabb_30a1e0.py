#!/usr/bin/env python3
"""Verify the selected World MOPP quantizer and its real 309940 handoff."""

import argparse
import json
import os
from pathlib import Path
import re
import sys

from instrument_native_execution import instrument_translation, unit_declaration
import native_mopp_quantized_aabb_30a1e0_replacements as binding
import specialize_native_operands as specializer
from validate_native_collision_pair_2f88d0 import run


ROOT = Path(__file__).resolve().parents[1]
WORLD = 'src/physics/runtime/physics_world_runtime_generated.cpp'
CONTACT = 'src/physics/runtime/physics_contact_runtime_generated.cpp'
ENTRIES = 'src/physics/physics_live_entries_tu15.hpp'
HEADER = 'src/physics/runtime/native_tu15_mopp_quantized_aabb_30a1e0.hpp'
FIXTURE = 'tests/native_tu15_mopp_quantized_aabb_30a1e0_tests.cpp'
STAGE_ANCHOR = "                        ('static void run_315c90(WorldState','native_phantom_island_replacements'),\n"
STAGE_INSERT = "                        ('static void run_3066e0(ObservedWorldState','native_mopp_quantized_aabb_30a1e0_replacements'),\n"
CONTACT_PIN = '3e71dd23fe6540f83679afdc6e8e54ee0ca2896609749254d70284de79a43ff3'
CONTACT_PREPARED = '709f13ef603925944cba4069b3aec2e8f275312f7e2323d8730a82a313946b73'
TRAVERSAL = '51ce1645edb2221aa60be6849e45db195264fd90afb0681340dc0d29161f7f96'
TRAVERSAL_BRANCHES = {'309e97': '306397', '309eef': '3063ef'}
MODES = ('literal', 'census', 'verify', 'reference')


def use_staged_specializer():
    path = Path(specializer.__file__)
    source = path.read_text()
    if source.count(STAGE_ANCHOR) != 1 or STAGE_INSERT in source:
        raise ValueError('Expected currently unbound staged World 30A1E0 catalog')
    name = specializer.__name__
    updated = type(specializer)(name)
    updated.__file__ = str(path)
    exec(compile(source.replace(STAGE_ANCHOR, STAGE_ANCHOR + STAGE_INSERT, 1), str(path), 'exec'), vars(updated))
    sys.modules[name] = updated
    return updated


def prepare_routes(canonical, prepared, contact, contact_prepared):
    if (ROOT / ENTRIES).read_text().count('{0x30a1e0u,0x3066e0u,Module::World},') != 1:
        raise ValueError('Selected World MOPP quantizer registration changed')
    alternate = re.compile(r'^static void run_3066e0\(State &s\) \{.*?^\}', re.M | re.S)
    if (binding.digest(alternate.search(contact)[0]) != CONTACT_PIN or
            binding.digest(alternate.search(contact_prepared)[0]) != CONTACT_PREPARED or
            binding.REFERENCE in contact_prepared):
        raise ValueError('Unselected MOPP quantizer Contact route changed')
    if binding.install(canonical, prepared) != (prepared, ['30a1e0']):
        raise ValueError('Wrong selected World MOPP quantizer')
    family = re.compile(r'^static void run_305e40\(ObservedWorldState &s\) \{.*?^\}', re.M | re.S)
    child = family.search(prepared)
    if (not child or binding.digest(child[0]) != TRAVERSAL or
            any(f'case 0x{branch}u:return 0x{disc}u;' not in canonical for branch, disc in TRAVERSAL_BRANCHES.items())):
        raise ValueError('MOPP quantizer fixture traversal or its stop/leaf routes changed')
    old = binding.original(canonical)
    changed = old.replace('0x30680cu,s.image));run_305e40(s);', '0x3066e0u,s.image));run_305e40(s);', 1)
    if changed == old:
        raise ValueError('Missing MOPP quantizer mutation edge')
    try:
        binding.install(canonical.replace(old, changed, 1), prepared)
    except ValueError:
        pass
    else:
        raise ValueError('MOPP quantizer binder accepted an altered return boundary')
    return child[0].replace('run_305e40(', 'fixture_305e40(') + '\n'


def selector(name, mappings):
    pairs = ''.join(f'case 0x{source}u:return 0x{target}u;' for source, target in mappings.items())
    return (f'static std::uint32_t {name}(std::uint32_t pc,SourcePhysicsContactImage image) {{'
            'if(image==SourcePhysicsContactImage::MapPack11)return pc;switch(pc){' + pairs +
            'default:throw std::logic_error("Unqualified MOPP quantizer fixture PC");}}\n')


def owner(canonical, prepared, accounted=False):
    result = binding.BODY.search(prepared)
    reference = binding.reference(canonical)
    if accounted:
        signature = f'static void {binding.REFERENCE}('
        reference = prepared[prepared.index(signature):result.start()]
    return (selector('selected', binding.SELECTED) + selector('canonical_branch', TRAVERSAL_BRANCHES) +
            reference + binding.reference(canonical, False) + result[0] + '\n')


def validate_profiles(canonical, prepared, counted, scratch, compiler, release_cases, sanitized_cases):
    flags = ['-std=c++20', '-O2', '-g0', '-fno-fast-math', '-ffp-contract=off', '-frounding-math',
             *(f'-I{path}' for path in (ROOT / 'include', ROOT / 'src', ROOT / 'tests', scratch))]
    rows = {}
    for profile, text, cases in (
            ('release', prepared, release_cases), ('sanitized', prepared, sanitized_cases),
            ('accounted', counted, release_cases)):
        if not cases:
            continue
        sources = [ROOT / FIXTURE]
        extra = []
        identity = ''
        if profile == 'sanitized':
            extra = ['-O1', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                     '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
        if profile == 'accounted':
            identity = '#include "halo2/native_execution_accounting.hpp"\n' + unit_declaration(WORLD, canonical, prepared)
            sources.append(ROOT / 'src/runtime/native_execution_accounting.cpp')
        (scratch / 'mopp_quantized_aabb_owner.inc').write_text(owner(canonical, text, profile == 'accounted'))
        (scratch / 'mopp_quantized_aabb_accounting.inc').write_text(identity)
        binary = scratch / f'mopp-quantized-aabb-{profile}'
        run([compiler, *flags, *extra, *sources, '-o', binary], scratch)
        modes = {}
        for mode in MODES:
            report = scratch / 'execution.json'
            env = {key: value for key, value in os.environ.items() if key not in (
                'HALO2_REFERENCE_MOPP_QUANTIZED_AABB_30A1E0', 'HALO2_VERIFY_MOPP_QUANTIZED_AABB_30A1E0',
                'HALO2_REFERENCE_MOPP_AABB', 'HALO2_CENSUS_NATIVE_BRIDGES', 'HALO2_NATIVE_EXECUTION_REPORT')}
            if profile == 'accounted':
                env['HALO2_NATIVE_EXECUTION_REPORT'] = str(report)
            result = run([binary, cases, mode], scratch, env=env, capture=True)
            data = json.loads(result.stdout)
            if result.stderr:
                data['verification'] = result.stderr.strip()
            if profile == 'accounted':
                census = json.loads(report.read_text())
                records = [dict(zip(census['row_fields'], row, strict=True)) for row in census['rows']]
                observed, = [item for item in records if item['symbol'] == binding.CANONICAL and
                             item['image'] == census['image_names'].index('tu15')]
                if (observed['entries'] != data['comparisons'] + data['canonical_fallbacks'] or
                        (mode != 'reference' and (observed['literal_statements'] != 100 or observed['body_entries'] != 1)) or
                        (mode == 'reference' and observed['literal_statements'] <= 100)):
                    raise ValueError(f'Wrong MOPP quantizer execution accounting in {mode}')
                data['owner_accounting'] = observed
            modes[mode] = data
        rows[profile] = modes
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default='g++')
    parser.add_argument('--release-cases', type=int, default=512)
    parser.add_argument('--sanitized-cases', type=int, default=128)
    parser.add_argument('--staged', action='store_true')
    parser.add_argument('--full-syntax', action='store_true')
    args = parser.parse_args()
    owned = os.environ.get('HALO2_TEST_SCRATCH')
    if not owned or not Path(owned).is_dir():
        parser.error('Run through tools/run_test.py for an owned test scratch directory')
    if min(args.release_cases, args.sanitized_cases) < 0:
        parser.error('Case counts must be nonnegative')
    scratch = Path(owned)
    canonical = (ROOT / WORLD).read_text()
    contact = (ROOT / CONTACT).read_text()
    loader = use_staged_specializer() if args.staged else specializer
    prepared, metadata = loader.specialize(canonical)
    contact_prepared, contact_metadata = loader.specialize(contact)
    if (metadata.get('native_spatial_owners', []).count('30a1e0') != 1 or
            '30a1e0' in contact_metadata.get('native_spatial_owners', ())):
        raise ValueError('30A1E0 must bind only the selected World route')
    child = prepare_routes(canonical, prepared, contact, contact_prepared)
    (scratch / 'mopp_quantized_aabb_child.inc').write_text(child)
    counted, manifest, _ = instrument_translation(prepared, canonical, WORLD, ROOT)
    retained = {row['symbol']: row for row in manifest['functions']
                if row['symbol'] in (binding.CANONICAL, binding.REFERENCE)}
    if (set(retained) != {binding.CANONICAL, binding.REFERENCE} or
            retained[binding.REFERENCE].get('canonical_reference_owner') != binding.CANONICAL or
            any(item['literal_sites'] != 100 for item in retained.values())):
        raise ValueError('Wrong retained MOPP quantizer observation ownership')
    if args.full_syntax:
        for name, source in (('normal', prepared), ('accounted', counted)):
            alias = scratch / f'{name}.cpp'
            alias.write_text(source)
            run([args.compiler, '-std=c++20', '-g0', '-fno-fast-math', '-ffp-contract=off',
                 f'-I{ROOT / "include"}', f'-I{ROOT / "src"}', '-fsyntax-only', alias], scratch)
            print(f'30A1E0 {name} production alias parsed', file=sys.stderr)
    profiles = validate_profiles(canonical, prepared, counted, scratch, args.compiler, args.release_cases, args.sanitized_cases)
    print(json.dumps({
        'compiler': run([args.compiler, '--version'], scratch, capture=True).stdout.splitlines()[0],
        'full_syntax': args.full_syntax, 'world_canonical_sha256': binding.PIN,
        'world_pre_binding_sha256': binding.PREPARED, 'untouched_contact_prepared_sha256': CONTACT_PREPARED,
        'header_sha256': binding.digest((ROOT / HEADER).read_text()), 'direct_traversal_sha256': TRAVERSAL,
        'manifest_owners': retained, 'source_direct_calls': 1, 'profiles': profiles,
    }, indent=2))


if __name__ == '__main__':
    main()
