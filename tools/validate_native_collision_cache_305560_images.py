#!/usr/bin/env python3
"""Verify both images of the alternate Contact cache and the World Map Pack route."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]
WORLD = 'src/physics/runtime/physics_world_runtime_generated.cpp'
CONTACT = 'src/physics/runtime/physics_contact_runtime_generated.cpp'
HEADER = 'src/physics/runtime/native_tu15_collision_cache_309060.hpp'
FIXTURE = 'tests/native_tu15_collision_cache_309060_tests.cpp'
WORLD_TITLE_STAGE = "                        ('static void run_305560(ObservedWorldState','native_collision_cache_309060_replacements'),\n"
WORLD_TITLE_ANCHOR = "                        ('static void run_2f4bd0(ObservedWorldState','native_convex_pair_replacements'),\n"
CHILDREN = {
    'world': {
        '2e0ba0': 'fe559cc5c2d5d4ab3ad9bc66d2d4f0fe9ca0783b7050be3abf68efca01a786fa',
        '2e0cc0': '33069e64d4e109ea41d645671c7b821f2ecb6ba1df9df1ac73311075e66b05c9',
    },
    'contact': {
        '2e0ba0': 'a16fab87e2de3085f38cf62a9efcfe3926c218aaa0a7e1e6de7b1967f5474013',
        '2e0cc0': 'c89792f0f9f15fb5c4a83fbe8010226fea2f068b67e6985d09661e115d9ede53',
    },
}
MODE_NATIVE = ('literal', 'quiet', 'verify')
MODE_RETAINED = ('unqualified', 'trace', 'service', 'observer', 'foreign', 'reference')


def digest(value):
    return hashlib.sha256(value.encode()).hexdigest()


def run(argv, scratch, *, env=None, capture=False):
    words = [str(arg) for arg in argv]
    child = subprocess.run(words, cwd=scratch, text=True, capture_output=capture, env=env)
    if child.returncode:
        raise RuntimeError(f'{words[0]} failed ({child.returncode})\n{child.stdout or ""}{child.stderr or ""}')
    return child


def assemble_overlay(scratch, patch):
    if patch is None:
        return ROOT
    overlay = scratch / 'overlay'
    for path in re.findall(r'^\+\+\+ b/([\w./-]+)$', patch.read_text(), re.M):
        src = ROOT / path
        dst = overlay / path
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(src, dst)
    run(['patch', '--batch', '--forward', '--fuzz=0', '-p1', '-i', patch.resolve()], overlay, capture=True)
    for source in (ROOT / 'tools').iterdir():
        target = overlay / 'tools' / source.name
        if not target.exists():
            target.symlink_to(source)
    catalog_path = overlay / 'tools/specialize_native_operands.py'
    catalog = catalog_path.read_text()
    if WORLD_TITLE_STAGE not in catalog:
        if catalog.count(WORLD_TITLE_ANCHOR) != 1:
            raise ValueError('Missing prior staged selected World cache binder')
        catalog_path.write_text(catalog.replace(WORLD_TITLE_ANCHOR, WORLD_TITLE_ANCHOR + WORLD_TITLE_STAGE, 1))
    return overlay


def runtime_route(overlay):
    ranges = 'src/gameplay/units/death_world_runtime_entries.hpp'
    scheduler = 'src/gameplay/units/death_world_scheduler.cpp'
    live = 'src/gameplay/multiplayer/multiplayer_object_runtime.cpp'
    entries = (ROOT / ranges).read_text()
    arrays = {name: set(re.findall(r'0x[0-9a-f]+u', elements))
              for name, elements in re.findall(r'inline constexpr std::array (\w+)\{([^}]+)\}', entries)}
    names = {'0x305560u', '0x2e14e0u', '0x2f48a0u'}
    source = (overlay / scheduler).read_text()
    if (not names <= arrays['contact'] <= arrays['world'] or
            source.index('else if (world)') >= source.index('else if (contact)') or
            source.count('physical.instruction_cursor = services.physics_instruction_cursor;') != 1):
        raise ValueError('Map Pack canonical World dispatch or its cursor handoff changed')
    caller = (overlay / live).read_text()
    sites = 'static constexpr std::array physics_service_sites{0x337a45u};'
    callback = re.search(r's\.runtime\.instruction=\[this\]\(auto p\)\{.*?^    \};', caller, re.M | re.S)
    callback_pin = '''
      if(std::this_thread::get_id()==main_thread)last_instruction=p;
      if(p==0x337a45u)put(bank,native_tick_count,std::uint32_t(clock.interrupt_time()/10000u));
    '''
    if (not callback or callback[0] != 's.runtime.instruction=[this](auto p){' + callback_pin + '};' or
            caller.count(sites) != 1 or 'physics_cursor.executor = main_thread;' not in caller or
            'physics_cursor.callback_type = &s.runtime.instruction.target_type();' not in caller):
        raise ValueError('Map Pack main-executor observation callback changed')
    return {'world_entries': len(arrays['world']), 'contact_only': len(arrays['contact'] - arrays['world']),
            'service_site': '337a45', 'cursor_provider_sha256': digest(callback[0]), 'units': [scheduler, live]}


def children(canonical, alias, part, binding):
    state = 'ObservedWorldState' if part == 'world' else 'State'
    router = re.search(rf'^static void dispatch\({state} &s,.*$', alias, re.M)
    if (not router or not router[0].endswith('s.required(site,target,tail);}') or
            any(f'0x889{k}00u' in router[0] for k in range(1, 7))):
        raise ValueError(f'Unexpected {part} cache virtual router')
    result = []
    import native_world_position_transform_2e4690_replacements as position
    for name, pin in CHILDREN[part].items():
        body = re.search(rf'^static void run_{name}\({state} &s\) \{{.*?^\}}', alias, re.M | re.S)
        child = body[0] if body else None
        if part == 'world' and name == '2e0ba0' and position.INCLUDE in alias:
            if position.install(canonical, alias) != (alias, ['2e4690']):
                raise ValueError('Unexpected World position transform binding')
            child = position.original(canonical)
        if not child or digest(child) != pin:
            raise ValueError(f'Expected real prepared {part} child {name}')
        result.append(child.replace(f'run_{name}(', f'fixture_{name}('))
    return '\n'.join(result) + '\n'


def owner(canonical, alias, binding, world_binding, accounted=False):
    pairs = ''.join(f'case 0x{disc}u:return 0x{tu15}u;' for disc, tu15 in world_binding.SELECTED.items())
    selector = ('static std::uint32_t selected(std::uint32_t pc,SourcePhysicsContactImage image) {'
                'if(image==SourcePhysicsContactImage::MapPack11)return pc;switch(pc){' + pairs +
                'default:throw std::logic_error("Unqualified collision cache operand");}}\n')
    result = binding.BODY.search(alias)
    reference = binding.reference(canonical)
    if accounted:
        signature = f'static void {binding.REFERENCE}('
        reference = alias[alias.index(signature):result.start()]
    return selector + reference + binding.reference(canonical, False) + result[0] + '\n'


def profile(binary, scratch, cases, mode, image, group, accounted):
    report = scratch / 'execution.json'
    env = {key: value for key, value in os.environ.items() if key not in (
        'HALO2_REFERENCE_COLLISION_CACHE_309060', 'HALO2_VERIFY_COLLISION_CACHE_309060',
        'HALO2_REFERENCE_COLLISION_CACHE_MAPPACK11', 'HALO2_VERIFY_COLLISION_CACHE_MAPPACK11',
        'HALO2_REFERENCE_CONTACT_CACHE_305560', 'HALO2_VERIFY_CONTACT_CACHE_305560',
        'HALO2_CENSUS_NATIVE_BRIDGES', 'HALO2_NATIVE_EXECUTION_REPORT')}
    if accounted:
        env['HALO2_NATIVE_EXECUTION_REPORT'] = str(report)
    result = run([binary, cases, mode, image], scratch, env=env, capture=True)
    data = json.loads(result.stdout)
    if result.stderr:
        data['verification'] = result.stderr.strip()
    if accounted:
        census = json.loads(report.read_text())
        records = [dict(zip(census['row_fields'], row, strict=True)) for row in census['rows']]
        row, = [entry for entry in records if entry['symbol'] == '305560' and
                entry['image'] == census['image_names'].index(image)]
        fallback = data['canonical_fallbacks']
        native = mode in MODE_NATIVE or (group == 'world-tu15' and mode == 'census')
        if (row['entries'] != data['comparisons'] + fallback or
                (native and (row['literal_statements'] != fallback * 82 or row['body_entries'] != fallback)) or
                (not native and row['literal_statements'] <= fallback * 82)):
            raise ValueError(f'Wrong collision cache accounting for {group}/{mode}: {row}')
        data['owner_accounting'] = row
    return data


def validate(canonical, alias, counted, section, fixture_children, scratch, overlay, compiler, release, sanitizer,
             binding, world_binding, unit_declaration):
    flags = ['-std=c++20', '-O2', '-g0', '-fno-fast-math', '-ffp-contract=off', '-frounding-math',
             *(f'-I{path}' for path in (overlay / 'include', overlay / 'src',
                                      ROOT / 'include', ROOT / 'src', ROOT / 'tests', scratch))]
    source = WORLD if section == 'world' else CONTACT
    targets = [(section + '-map-pack11', 'map-pack11', (*MODE_NATIVE, *MODE_RETAINED))]
    if section == 'world':
        targets.append(('world-tu15', 'tu15', ('literal', 'census', 'verify', 'reference')))
    else:
        flags.append('-DHALO2_COLLISION_CACHE_CONTACT_FIXTURE')
        targets.append(('contact-tu15', 'tu15', (*MODE_NATIVE, *MODE_RETAINED)))
    (scratch / 'collision_cache_children.inc').write_text(fixture_children)
    results = {}
    for label, text, cases in (
            ('release', alias, release), ('sanitized', alias, sanitizer), ('accounted', counted, release)):
        if not cases:
            continue
        extra = []
        sources = [overlay / FIXTURE]
        identity = ''
        measured = label == 'accounted'
        if label == 'sanitized':
            extra = ['-O1', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                     '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
        if measured:
            identity = '#include "halo2/native_execution_accounting.hpp"\n' + unit_declaration(source, canonical, alias)
            sources.append(ROOT / 'src/runtime/native_execution_accounting.cpp')
        (scratch / 'collision_cache_owner.inc').write_text(owner(canonical, text, binding, world_binding, measured))
        (scratch / 'collision_cache_accounting.inc').write_text(identity)
        binary = scratch / f'collision-cache-{section}-{label}'
        run([compiler, *flags, *extra, *sources, '-o', binary], scratch)
        rows = {}
        for group, image, modes in targets:
            rows[group] = {mode: profile(binary, scratch, cases, mode, image, group, measured) for mode in modes}
        results[label] = rows
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--staged', type=Path)
    parser.add_argument('--compiler', default='g++')
    parser.add_argument('--release-cases', type=int, default=128)
    parser.add_argument('--sanitized-cases', type=int, default=64)
    parser.add_argument('--full-syntax', action='store_true')
    args = parser.parse_args()
    owned = os.environ.get('HALO2_TEST_SCRATCH')
    if not owned or not Path(owned).is_dir():
        parser.error('Run through tools/run_test.py for an owned test scratch directory')
    if min(args.release_cases, args.sanitized_cases) < 0:
        parser.error('Case counts must be nonnegative')
    scratch = Path(owned)
    overlay = assemble_overlay(scratch, args.staged)
    sys.path.insert(0, str(overlay / 'tools'))
    import native_collision_cache_305560_contact_replacements as contact
    import native_collision_cache_309060_replacements as world
    import specialize_native_operands as specializer
    from instrument_native_execution import instrument_translation, unit_declaration
    route = runtime_route(overlay)
    syntax = [args.compiler, '-std=c++20', '-g0', '-fno-fast-math', '-ffp-contract=off',
              *(f'-I{p}' for p in (overlay / 'src', overlay / 'include', ROOT / 'src', ROOT / 'include')),
              '-fsyntax-only']
    if args.full_syntax:
        for source in route['units']:
            run([*syntax, overlay / source], scratch)
            print(f'305560 route provider {Path(source).name} parsed', file=sys.stderr)
    results = {}
    for section, source, binder, entry in (('world', WORLD, world, '309060'), ('contact', CONTACT, contact, '305560')):
        canonical = (ROOT / source).read_text()
        alias, metadata = specializer.specialize(canonical)
        if (metadata.get('native_spatial_owners', []).count(entry) != 1 or
                binder.install(canonical, alias) != (alias, [entry])):
            raise ValueError(f'Wrong cache binding for {section}')
        counted, manifest, _ = instrument_translation(alias, canonical, source, ROOT)
        sites = {row['symbol']: row for row in manifest['functions'] if row['symbol'] in ('305560', binder.REFERENCE)}
        if (set(sites) != {'305560', binder.REFERENCE} or sites[binder.REFERENCE].get('canonical_reference_owner') != '305560' or
                any(row['literal_sites'] != 82 for row in sites.values())):
            raise ValueError(f'Wrong original cache observations for {section}')
        if args.full_syntax:
            for variant, text in (('normal', alias), ('counted', counted)):
                file = scratch / f'{section}-{variant}.cpp'
                file.write_text(text)
                run([*syntax, file], scratch)
                print(f'305560 {section} {variant} production alias parsed', file=sys.stderr)
        fixtures = validate(canonical, alias, counted, section, children(canonical, alias, section, binder), scratch,
                            overlay, args.compiler, args.release_cases, args.sanitized_cases, binder, world,
                            unit_declaration)
        results[section] = {'owner_sha256': binder.PIN, 'prepared_owner_sha256': binder.PREPARED,
                            'manifest_owners': sites, 'fixtures': fixtures}
    print(json.dumps({'compiler': run([args.compiler, '--version'], scratch, capture=True).stdout.splitlines()[0],
                      'full_syntax': args.full_syntax, 'header_sha256': digest((overlay / HEADER).read_text()),
                      'runtime_route': route, 'variants': results}, indent=2))


if __name__ == '__main__':
    main()
