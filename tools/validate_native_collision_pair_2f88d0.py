#!/usr/bin/env python3
"""Qualify the 2F88D0 TU1.5 collision pair against its authenticated routes."""

import argparse
import contextlib
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

from instrument_native_execution import instrument_translation, unit_declaration
from native_model_marker_lookup_replacements import function
import native_collision_pair_2f88d0_replacements as binding
import specialize_native_operands as specializer


ROOT = Path(__file__).resolve().parents[1]
SOURCE = 'src/physics/physics_live_support_tu15_generated.cpp'
TEST = 'tests/native_tu15_collision_pair_2f88d0_tests.cpp'
HEADER = 'src/physics/runtime/native_tu15_collision_pair_2f88d0.hpp'
ROUTER_SOURCES = ('src/physics/native_tu15_array_allocator.cpp', 'src/physics/native_tu15_main_heap_allocate.cpp',
                  'src/physics/native_tu15_main_heap_release.cpp', 'src/physics/native_tu15_physical_epoch.cpp')
CHILD = re.compile(r'^static void run_309150\(WorldState &s\) \{.*?^\}(?=\n\nstatic void run_)', re.M | re.S)
CHILD_PIN = '600e65a209150b9e20a9d1ff2bd33f156aa5c8b32fec9f831b826c19267aad39'
STAGE_ANCHOR = "                        ('void source_run_physics_live_support_tu15(','native_object_hierarchy_root_replacements'),\n"
STAGE_INSERT = "                        ('void source_run_physics_live_support_tu15(','native_collision_pair_2f88d0_replacements'),\n"
ROUTES = ('2e4550', '309060', '30e930', '30d350', '2e4760', '880000', '880010', '880020', '880030', '880040')
ROUTER = re.compile(r'^static void dispatch\(WorldState &s,([\w :,&]+)\)\{switch\(target\)\{.*default:(.*)\}\}$', re.M)
REQUIRED = 's.required(site,target,tail);return;'
ALLOCATOR = ('if(halo2::native_allocator_owner_detail::dispatch(s,target,[](auto &v,std::uint32_t owner,'
             'std::uint32_t call_site,bool is_tail){dispatch(v,owner,call_site,is_tail);}))return;')
MODES = ('literal', 'census', 'verify', 'reference')


def sha(text):
    return hashlib.sha256(text.encode()).hexdigest()


@contextlib.contextmanager
def staged_specializer(enabled):
    if not enabled:
        yield
        return
    path = Path(specializer.__file__)
    source = path.read_text()
    if source.count(STAGE_ANCHOR) != 1 or STAGE_INSERT in source:
        raise ValueError('Expected currently unbound 2F88D0 staged specializer')
    name = specializer.__name__
    staged = type(specializer)(name)
    staged.__file__ = str(path)
    exec(compile(source.replace(STAGE_ANCHOR, STAGE_ANCHOR + STAGE_INSERT, 1), str(path), 'exec'), vars(staged))
    previous = sys.modules[name]
    sys.modules[name] = staged
    try:
        yield
    finally:
        sys.modules[name] = previous


def validate_routes(canonical, prepared):
    raw = binding.original(canonical)
    expected = binding.reference(canonical)
    for source, default in ((canonical, REQUIRED), (prepared, ALLOCATOR + REQUIRED)):
        binding.check_route(source)
        router = ROUTER.search(source)
        if not router or router[2] != default:
            raise ValueError('Collision pair fixture lost the original dispatch default')
        if any(f'case 0x{target}u:' in router[0] for target in ROUTES):
            raise ValueError('Collision pair fixture expected dispatched children through the required service')
    selected = specializer.MAPPING.search(canonical)
    if not selected or 'case 0x2f88d0u:return 0x2f88d0u;' not in selected['cases']:
        raise ValueError('Collision pair runtime address changed')
    child = CHILD.search(canonical)
    compiled_child = CHILD.search(prepared)
    if (not child or not compiled_child or sha(child[0]) != CHILD_PIN or
            specializer.group_literal_stack_pops(binding.CURSOR.sub(r's.instruction_literal(\1);', child[0]))[0]
            != compiled_child[0]):
        raise ValueError('Collision pair direct 309150 child changed')
    if prepared.count(expected) != 1 or binding.install(canonical, prepared) != (prepared, ['2f88d0']):
        raise ValueError('Collision pair production binding differs')
    first = binding.FIRST_BODY.search(prepared)
    if not first or prepared.index(expected) >= first.start():
        raise ValueError('Collision pair replay must be in the shared partition prefix')
    mutations = (
        canonical.replace(raw, raw.replace('0x2f894eu);run_309150', '0x2f8950u);run_309150', 1), 1),
        canonical.replace('case 0x2f88d0u:run_2f88d0(s);break;', 'case 0x2f88d0u:s.required(0u,0u,false);break;', 1),
    )
    for changed in mutations:
        try:
            binding.install(changed, prepared)
        except ValueError:
            continue
        raise ValueError('Collision pair binder accepted a changed route')
    direct = compiled_child[0].replace('void run_309150(', 'void fixture_309150(', 1) + '\n'
    routed = f'static void dispatch(WorldState &s,{router[1]}) {{{router[2]}}}\n'
    return direct, routed


def run(argv, scratch, *, env=None, capture=False):
    argv = list(map(str, argv))
    child = subprocess.run(argv, cwd=scratch, text=True, env=env, capture_output=capture)
    if child.returncode:
        raise RuntimeError(f'{argv[0]} failed ({child.returncode})\n{child.stdout or ""}{child.stderr or ""}')
    return child


def prepared_owner(text, canonical, accounted=False):
    generated = binding.BODY.search(text)
    if not generated:
        raise ValueError('Missing generated 2F88D0 owner')
    if accounted:
        reference = function(text, binding.REFERENCE) + '\n'
    else:
        reference = binding.reference(canonical)
    return reference + binding.reference(canonical, replay=False) + generated[0] + '\n'


def run_profiles(canonical, prepared, accounted, scratch, compiler, release_cases, sanitized_cases):
    flags = ['-std=c++20', '-O2', '-g0', '-fno-fast-math', '-ffp-contract=off', '-frounding-math',
             *(f'-I{path}' for path in (ROOT / 'include', ROOT / 'src', ROOT / 'tests', scratch))]
    sources = [ROOT / path for path in (TEST, *ROUTER_SOURCES)]
    result = {}
    for profile, text, cases in (
            ('release', prepared, release_cases), ('sanitized', prepared, sanitized_cases),
            ('accounted', accounted, release_cases)):
        if not cases:
            continue
        extra = []
        additional = []
        (scratch / 'collision_pair_owner.inc').write_text(prepared_owner(text, canonical, profile == 'accounted'))
        account_header = ''
        if profile == 'accounted':
            account_header = ('#include "halo2/native_execution_accounting.hpp"\n' +
                              unit_declaration(SOURCE, canonical, prepared))
            additional = [ROOT / 'src/runtime/native_execution_accounting.cpp']
        if profile == 'sanitized':
            extra = ['-O1', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                     '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
        (scratch / 'collision_pair_accounting.inc').write_text(account_header)
        binary = scratch / f'collision-pair-{profile}'
        run([compiler, *flags, *extra, *sources, *additional, '-o', binary], scratch)
        profiles = {}
        for mode in MODES:
            report = scratch / 'execution.json'
            env = {key: value for key, value in os.environ.items() if key not in (
                'HALO2_REFERENCE_COLLISION_PAIR_2F88D0', 'HALO2_VERIFY_COLLISION_PAIR_2F88D0',
                'HALO2_CENSUS_NATIVE_BRIDGES', 'HALO2_NATIVE_EXECUTION_REPORT')}
            if profile == 'accounted':
                env['HALO2_NATIVE_EXECUTION_REPORT'] = str(report)
            passed = run([binary, cases, mode], scratch, env=env, capture=True)
            profiles[mode] = json.loads(passed.stdout)
            if passed.stderr:
                profiles[mode]['verification'] = passed.stderr.strip()
            if profile == 'accounted':
                summary = json.loads(report.read_text())
                rows = [dict(zip(summary['row_fields'], row, strict=True)) for row in summary['rows']]
                body, = [row for row in rows if row['symbol'] == '2f88d0' and
                         row['image'] == summary['image_names'].index('tu15')]
                if body['entries'] != profiles[mode]['comparisons'] or (
                        bool(body['literal_statements']) != (mode == 'reference')):
                    raise ValueError(f'Incorrect 2F88D0 execution accounting in {mode}')
                profiles[mode]['owner_accounting'] = body
        result[profile] = profiles
    return result


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
    canonical = (ROOT / SOURCE).read_text()
    before = None
    if args.staged:
        unbound, prior = specializer.specialize(canonical)
        if binding.REFERENCE in unbound or '2f88d0' in prior.get('native_spatial_owners', ()):
            raise ValueError('2F88D0 is already bound')
        before = sha(binding.BODY.search(unbound)[0])
    with staged_specializer(args.staged):
        prepared, metadata = sys.modules[specializer.__name__].specialize(canonical)
        if metadata.get('native_spatial_owners', []).count('2f88d0') != 1:
            raise ValueError('Missing sole full 2F88D0 owner in production metadata')
        child, router = validate_routes(canonical, prepared)
        accounted, manifest, _ = instrument_translation(prepared, canonical, SOURCE, ROOT)
    sites = {row['symbol']: row for row in manifest['functions'] if row['symbol'] in ('2f88d0', binding.REFERENCE)}
    if (set(sites) != {'2f88d0', binding.REFERENCE} or
            sites[binding.REFERENCE].get('canonical_reference_owner') != '2f88d0' or
            any(owner['literal_sites'] != 187 for owner in sites.values())):
        raise ValueError('Incorrect 2F88D0 fallback or replay accounting ownership')
    (scratch / 'collision_pair_child.inc').write_text(child)
    (scratch / 'collision_pair_router.inc').write_text(router)
    if args.full_syntax:
        for label, source in (('normal', prepared), ('accounted', accounted)):
            alias = scratch / f'{label}.cpp'
            alias.write_text(source)
            run([args.compiler, '-std=c++20', '-g0', '-fno-fast-math', '-ffp-contract=off',
                 f'-I{ROOT / "include"}', f'-I{ROOT / "src"}', '-fsyntax-only', alias], scratch)
            print(f'2F88D0 {label} production alias parsed', file=sys.stderr)
    report = run_profiles(canonical, prepared, accounted, scratch, args.compiler, args.release_cases, args.sanitized_cases)
    print(json.dumps({
        'canonical_owner_sha256': binding.PIN,
        'direct_child_sha256': CHILD_PIN,
        'prepared_owner_sha256': sha(binding.BODY.search(prepared)[0]),
        'pre_binding_owner_sha256': before,
        'header_sha256': sha((ROOT / HEADER).read_text()),
        'compiler': run([args.compiler, '--version'], scratch, capture=True).stdout.splitlines()[0],
        'full_syntax': args.full_syntax,
        'manifest_owners': sites,
        'source_dispatch_calls': 7,
        'source_direct_calls': 1,
        'profiles': report,
    }, indent=2))


if __name__ == '__main__':
    main()
