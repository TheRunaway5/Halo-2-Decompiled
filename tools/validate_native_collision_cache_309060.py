#!/usr/bin/env python3
"""Qualify the selected World collision-cache initializer and its entry scope."""

import argparse
import json
import os
from pathlib import Path
import re
import sys

from instrument_native_execution import instrument_translation, unit_declaration
import native_collision_cache_309060_replacements as binding
import native_collision_cache_305560_contact_replacements as contact_binding
import native_world_position_transform_2e4690_replacements as position
import specialize_native_operands as specializer
from validate_native_collision_pair_2f88d0 import run


ROOT = Path(__file__).resolve().parents[1]
WORLD = 'src/physics/runtime/physics_world_runtime_generated.cpp'
CONTACT = 'src/physics/runtime/physics_contact_runtime_generated.cpp'
HEADER = 'src/physics/runtime/native_tu15_collision_cache_309060.hpp'
FIXTURE = 'tests/native_tu15_collision_cache_309060_tests.cpp'
ENTRIES = 'src/physics/physics_live_entries_tu15.hpp'
STAGE_ANCHOR = "                        ('static void run_2f4bd0(ObservedWorldState','native_convex_pair_replacements'),\n"
STAGE_INSERT = "                        ('static void run_305560(ObservedWorldState','native_collision_cache_309060_replacements'),\n"
CHILDREN = {
    '2e0ba0': ('fe559cc5c2d5d4ab3ad9bc66d2d4f0fe9ca0783b7050be3abf68efca01a786fa',
               'fe559cc5c2d5d4ab3ad9bc66d2d4f0fe9ca0783b7050be3abf68efca01a786fa'),
    '2e0cc0': ('d540ea11965c16208289c218c22497fbaf1f2e0bb3884a43816d8efcefa03f20',
               '33069e64d4e109ea41d645671c7b821f2ecb6ba1df9df1ac73311075e66b05c9'),
}
MODES = ('literal', 'census', 'verify', 'reference')


def use_staged_specializer():
    path = Path(specializer.__file__)
    source = path.read_text()
    if source.count(STAGE_ANCHOR) != 1 or STAGE_INSERT in source:
        raise ValueError('Expected currently unbound staged World 309060 catalog')
    name = specializer.__name__
    updated = type(specializer)(name)
    updated.__file__ = str(path)
    exec(compile(source.replace(STAGE_ANCHOR, STAGE_ANCHOR + STAGE_INSERT, 1), str(path), 'exec'), vars(updated))
    sys.modules[name] = updated
    return updated


def prepare_routes(canonical, prepared, contact, contact_prepared):
    entries = (ROOT / ENTRIES).read_text()
    if entries.count('{0x309060u,0x305560u,Module::World},') != 1:
        raise ValueError('Selected World collision cache registry route changed')
    if binding.REFERENCE in contact_prepared or binding.BODY.search(contact):
        raise ValueError('Selected World collision cache also bound the Contact entry')
    if contact_binding.install(contact, contact_prepared) != (contact_prepared, ['305560']):
        raise ValueError('Separate Contact collision cache route changed')
    if binding.install(canonical, prepared) != (prepared, ['309060']):
        raise ValueError('Wrong prepared World 309060 route')
    router = re.search(r'^static void dispatch\(ObservedWorldState &s,.*$', prepared, re.M)
    if (not router or not router[0].endswith('s.required(site,target,tail);}') or
            any(f'0x889{target}00u' in router[0] for target in range(1, 7))):
        raise ValueError('Fixture virtual targets must reach the real World required default')
    children = []
    for name, pins in CHILDREN.items():
        family = re.compile(rf'^static void run_{name}\(ObservedWorldState &s\) \{{.*?^\}}', re.M | re.S)
        raw = family.search(canonical)
        active = family.search(prepared)
        child = active[0] if active else None
        if name == '2e0ba0' and position.INCLUDE in prepared:
            if position.install(canonical, prepared) != (prepared, ['2e4690']):
                raise ValueError('Unexpected World position transform binding')
            child = position.original(canonical)
        if not raw or not child or (binding.digest(raw[0]), binding.digest(child)) != pins:
            raise ValueError(f'World collision cache child {name} changed')
        children.append(child.replace(f'run_{name}(', f'fixture_{name}('))
    old = binding.original(canonical)
    changed = old.replace('0x3055edu,s.image));run_2e0cc0(s);',
                          '0x3055edu,s.image));run_2e0ba0(s);', 1)
    if changed == old:
        raise ValueError('Missing collision cache negative-test edge')
    try:
        binding.install(canonical.replace(old, changed, 1), prepared)
    except ValueError:
        pass
    else:
        raise ValueError('Collision cache binder accepted a changed direct child')
    return '\n'.join(children) + '\n'


def owner(canonical, prepared, accounted=False):
    pairs = ''.join(f'case 0x{disc}u:return 0x{tu15}u;' for disc, tu15 in binding.SELECTED.items())
    selector = ('static std::uint32_t selected(std::uint32_t pc,SourcePhysicsContactImage image) {'
                'if(image==SourcePhysicsContactImage::MapPack11)return pc;switch(pc){' + pairs +
                'default:throw std::logic_error("Unqualified collision cache operand");}}\n')
    result = binding.BODY.search(prepared)
    reference = binding.reference(canonical)
    if accounted:
        signature = f'static void {binding.REFERENCE}('
        reference = prepared[prepared.index(signature):result.start()]
    return selector + reference + binding.reference(canonical, False) + result[0] + '\n'


def validate_profiles(canonical, prepared, counted, scratch, compiler, release_cases, sanitized_cases):
    flags = ['-std=c++20', '-O2', '-g0', '-fno-fast-math', '-ffp-contract=off', '-frounding-math',
             *(f'-I{path}' for path in (ROOT / 'include', ROOT / 'src', ROOT / 'tests', scratch))]
    results = {}
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
        (scratch / 'collision_cache_owner.inc').write_text(owner(canonical, text, profile == 'accounted'))
        (scratch / 'collision_cache_accounting.inc').write_text(identity)
        binary = scratch / f'collision-cache-{profile}'
        run([compiler, *flags, *extra, *sources, '-o', binary], scratch)
        modes = {}
        for mode in MODES:
            report = scratch / 'execution.json'
            env = {key: value for key, value in os.environ.items() if key not in (
                'HALO2_REFERENCE_COLLISION_CACHE_309060', 'HALO2_VERIFY_COLLISION_CACHE_309060',
                'HALO2_CENSUS_NATIVE_BRIDGES', 'HALO2_NATIVE_EXECUTION_REPORT')}
            if profile == 'accounted':
                env['HALO2_NATIVE_EXECUTION_REPORT'] = str(report)
            result = run([binary, cases, mode], scratch, env=env, capture=True)
            data = json.loads(result.stdout)
            if result.stderr:
                data['verification'] = result.stderr.strip()
            if profile == 'accounted':
                census = json.loads(report.read_text())
                rows = [dict(zip(census['row_fields'], row, strict=True)) for row in census['rows']]
                native, = [row for row in rows if row['symbol'] == '305560' and
                           row['image'] == census['image_names'].index('tu15')]
                if (native['entries'] != data['comparisons'] + data['canonical_fallbacks'] or
                        (mode != 'reference' and (native['literal_statements'] != 82 or native['body_entries'] != 1)) or
                        (mode == 'reference' and native['literal_statements'] <= 82)):
                    raise ValueError(f'Incorrect collision cache execution accounting in {mode}')
                data['owner_accounting'] = native
            modes[mode] = data
        results[profile] = modes
    return results


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
    if (metadata.get('native_spatial_owners', []).count('309060') != 1 or
            '309060' in contact_metadata.get('native_spatial_owners', ())):
        raise ValueError('309060 must bind exactly one World route')
    (scratch / 'collision_cache_children.inc').write_text(prepare_routes(canonical, prepared, contact, contact_prepared))
    counted, manifest, _ = instrument_translation(prepared, canonical, WORLD, ROOT)
    sites = {row['symbol']: row for row in manifest['functions'] if row['symbol'] in ('305560', binding.REFERENCE)}
    if (set(sites) != {'305560', binding.REFERENCE} or
            sites[binding.REFERENCE].get('canonical_reference_owner') != '305560' or
            any(owner['literal_sites'] != 82 for owner in sites.values())):
        raise ValueError('Incorrect collision cache retained observations')
    if args.full_syntax:
        for name, source in (('normal', prepared), ('accounted', counted)):
            alias = scratch / f'{name}.cpp'
            alias.write_text(source)
            run([args.compiler, '-std=c++20', '-g0', '-fno-fast-math', '-ffp-contract=off',
                 f'-I{ROOT / "include"}', f'-I{ROOT / "src"}', '-fsyntax-only', alias], scratch)
            print(f'309060 {name} production alias parsed', file=sys.stderr)
    report = validate_profiles(canonical, prepared, counted, scratch, args.compiler, args.release_cases, args.sanitized_cases)
    print(json.dumps({
        'compiler': run([args.compiler, '--version'], scratch, capture=True).stdout.splitlines()[0],
        'full_syntax': args.full_syntax, 'world_canonical_sha256': binding.PIN,
        'world_pre_binding_sha256': binding.PREPARED,
        'contact_pre_binding_sha256': contact_binding.PREPARED,
        'header_sha256': binding.digest((ROOT / HEADER).read_text()), 'manifest_owners': sites,
        'source_virtual_calls': 6, 'source_direct_calls': 2, 'profiles': report,
    }, indent=2))


if __name__ == '__main__':
    main()
