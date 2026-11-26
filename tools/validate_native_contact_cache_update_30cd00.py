#!/usr/bin/env python3
"""Qualify selected World 30CD00 and its two real local/spatial helpers."""

import argparse
import json
import os
from pathlib import Path
import re
import sys

from instrument_native_execution import instrument_translation, unit_declaration
import native_contact_cache_update_30cd00_replacements as binding
import native_world_position_transform_2e4690_replacements as position
import specialize_native_operands as specializer
from validate_native_collision_pair_2f88d0 import ALLOCATOR, REQUIRED, ROUTER_SOURCES, run


ROOT = Path(__file__).resolve().parents[1]
WORLD = 'src/physics/runtime/physics_world_runtime_generated.cpp'
SUPPORT = 'src/physics/physics_live_support_tu15_generated.cpp'
HEADER = 'src/physics/runtime/native_tu15_contact_cache_update_30cd00.hpp'
FIXTURE = 'tests/native_tu15_contact_cache_update_30cd00_tests.cpp'
STAGE_ANCHOR = "                        ('static void run_309450(ObservedWorldState','native_manifold_replacements'),\n"
STAGE_INSERT = "                        ('static void run_309210(ObservedWorldState','native_contact_cache_update_30cd00_replacements'),\n"
TRANSFORM_PIN = 'fe559cc5c2d5d4ab3ad9bc66d2d4f0fe9ca0783b7050be3abf68efca01a786fa'
REDUCER_PIN = '47fa03a1d1a20104ae1305967df589f9d2e056c40675021e014551b5c4296dac'
REDUCER_PREPARED = 'ac0100f8b79a402a472eb8235a6f3ec47865d79a70ba363641401a7779a21764'
MODES = ('literal','census','verify','reference')


def staged_specializer():
    path = Path(specializer.__file__)
    source = path.read_text()
    if source.count(STAGE_ANCHOR) != 1 or STAGE_INSERT in source:
        raise ValueError('Expected unbound staged World 30CD00 catalog')
    name = specializer.__name__
    updated = type(specializer)(name)
    updated.__file__ = str(path)
    exec(compile(source.replace(STAGE_ANCHOR,STAGE_ANCHOR+STAGE_INSERT,1),str(path),'exec'),vars(updated))
    sys.modules[name] = updated
    return updated


def prepare(world, alias, support, support_alias, scratch):
    if (ROOT/'src/physics/physics_live_entries_tu15.hpp').read_text().count('{0x30cd00u,0x309210u,Module::World},') != 1:
        raise ValueError('World 30CD00 live entry selection changed')
    router = re.search(r'^static void dispatch\(ObservedWorldState &s,.*$', alias, re.M)
    if (not router or not router[0].endswith('s.required(site,target,tail);}') or
            any(f'0x{target}u' in router[0] for target in ('308fb0','30caa0','889100'))):
        raise ValueError('Expected actual World 30CD00 dispatched owners through the required service')
    for image in (support,support_alias):
        if (image.count('case 0x30caa0u:run_30caa0(s);break;') != 1 or
                image.count('case 0x30caa0u:run_30caa0(s);return;') != 1):
            raise ValueError('Missing real physics live-support reducer route')
    support_router = re.search(r'^static void dispatch\(WorldState &s,.*$', support_alias, re.M)
    if (not support_router or not support_router[0].endswith('default:' + ALLOCATOR + REQUIRED + '}}') or
            'case 0x889200u:' in support_router[0]):
        raise ValueError('Real contact-cache reducer fixture router changed')
    transform = position.BODY.search(alias)
    if not transform or position.install(world,alias)!=(alias,['2e4690']):
        raise ValueError('Expected the qualified World 2E4690 transform owner')
    translated_transform = position.translated(world)
    reducer = re.compile(r'^static void run_30caa0\(WorldState &s\) \{.*?^\}(?=\n\nstatic void run_30d1a0\()',re.M|re.S)
    canonical_reduce, actual_reduce = reducer.search(support)[0], reducer.search(support_alias)[0]
    if (binding.digest(position.original(world)) != TRANSFORM_PIN or
            binding.digest(canonical_reduce) != REDUCER_PIN or binding.digest(actual_reduce) != REDUCER_PREPARED):
        raise ValueError('World 30CD00 genuine transform or reducer changed')
    if binding.install(world,alias) != (alias,['30cd00']):
        raise ValueError('Wrong prepared World 30CD00 owner')
    base = binding.original(world)
    changed = base.replace('0x309428u,s.image));dispatch', '0x3093d2u,s.image));dispatch',1)
    if changed == base:
        raise ValueError('Expected reduce-call mutation site')
    try:
        binding.install(world.replace(base,changed,1),alias)
    except ValueError:
        pass
    else:
        raise ValueError('World 30CD00 binder accepted a changed reducer return address')
    (scratch/'contact_cache_update_transform.inc').write_text(translated_transform.replace('run_2e0ba0(','fixture_2e0ba0(')+'\n')
    (scratch/'contact_cache_update_reduce.inc').write_text(actual_reduce+'\n')
    return binding.digest(actual_reduce)


def owner(canonical, alias, accounted):
    pairs = ''.join(f'case 0x{disc}u:return 0x{tu15}u;' for disc,tu15 in binding.SELECTED.items())
    selector = ('static std::uint32_t selected(std::uint32_t pc,SourcePhysicsContactImage image) {'
                'if(image==SourcePhysicsContactImage::MapPack11)return pc;switch(pc){' + pairs +
                'default:throw std::logic_error("Unqualified contact-cache operand");}}\n')
    body = binding.BODY.search(alias)
    reference = binding.reference(canonical)
    if accounted:
        signature = f'static void {binding.REFERENCE}('
        reference = alias[alias.index(signature):body.start()]
    return selector+reference+binding.reference(canonical,False)+body[0]+'\n'


def profiles(world, alias, counted, scratch, args):
    flags = ['-std=c++20','-O2','-g0','-fno-fast-math','-ffp-contract=off','-frounding-math',
             *(f'-I{path}' for path in (ROOT/'include',ROOT/'src',ROOT/'tests',scratch))]
    all_modes = {}
    for profile,text,cases in (
            ('release',alias,args.release_cases),('sanitized',alias,args.sanitized_cases),
            ('accounted',counted,args.release_cases)):
        if not cases:
            continue
        extra = []
        sources = [ROOT/path for path in (FIXTURE,*ROUTER_SOURCES)]
        identity = ''
        accounted = profile=='accounted'
        if profile=='sanitized':
            extra = ['-O1','-fsanitize=address,undefined','-fno-sanitize-recover=all',
                     '-fno-omit-frame-pointer','-fno-pie','-no-pie']
        if accounted:
            identity = '#include "halo2/native_execution_accounting.hpp"\n'+unit_declaration(WORLD,world,alias)
            sources.append(ROOT/'src/runtime/native_execution_accounting.cpp')
        (scratch/'contact_cache_update_owner.inc').write_text(owner(world,text,accounted))
        (scratch/'contact_cache_update_accounting.inc').write_text(identity)
        binary = scratch/f'contact-cache-{profile}'
        run([args.compiler,*flags,*extra,*sources,'-o',binary],scratch)
        results = {}
        for mode in MODES:
            report = scratch/'execution.json'
            env = {key:value for key,value in os.environ.items() if key not in (
                'HALO2_REFERENCE_CONTACT_CACHE_UPDATE_30CD00','HALO2_VERIFY_CONTACT_CACHE_UPDATE_30CD00',
                'HALO2_CENSUS_NATIVE_BRIDGES','HALO2_NATIVE_EXECUTION_REPORT')}
            if accounted:
                env['HALO2_NATIVE_EXECUTION_REPORT'] = str(report)
            result = run([binary,cases,mode],scratch,env=env,capture=True)
            row = json.loads(result.stdout)
            if result.stderr:
                row['verification'] = result.stderr.strip()
            if accounted:
                census = json.loads(report.read_text())
                records = [dict(zip(census['row_fields'],item,strict=True)) for item in census['rows']]
                body, = [entry for entry in records if entry['symbol']=='309210' and
                         entry['image']==census['image_names'].index('tu15')]
                fallback = row['canonical_fallbacks']
                if (body['entries']!=row['comparisons']+fallback or
                        (mode!='reference' and (body['literal_statements']!=row['canonical_fallback_statements'] or
                                                body['body_entries']!=fallback)) or
                        (mode=='reference' and body['literal_statements']<=row['canonical_fallback_statements'])):
                    raise ValueError(f'Wrong World 30CD00 accounting in {mode}')
                row['owner_accounting'] = body
            results[mode] = row
        all_modes[profile] = results
    return all_modes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler',default='g++')
    parser.add_argument('--release-cases',type=int,default=512)
    parser.add_argument('--sanitized-cases',type=int,default=128)
    parser.add_argument('--staged',action='store_true')
    parser.add_argument('--full-syntax',action='store_true')
    args = parser.parse_args()
    owned = os.environ.get('HALO2_TEST_SCRATCH')
    if not owned or not Path(owned).is_dir():
        parser.error('Run through tools/run_test.py for an owned test scratch directory')
    if min(args.release_cases,args.sanitized_cases)<0:
        parser.error('Case counts must be nonnegative')
    scratch = Path(owned)
    world, support = (ROOT/WORLD).read_text(), (ROOT/SUPPORT).read_text()
    loader = staged_specializer() if args.staged else specializer
    alias, metadata = loader.specialize(world)
    support_alias, _ = loader.specialize(support)
    if metadata.get('native_spatial_owners',[]).count('30cd00')!=1:
        raise ValueError('Missing selected World native cache update')
    reducer = prepare(world,alias,support,support_alias,scratch)
    counted, manifest, _ = instrument_translation(alias,world,WORLD,ROOT)
    retained = {entry['symbol']:entry for entry in manifest['functions'] if entry['symbol'] in ('309210',binding.REFERENCE)}
    if (set(retained)!={'309210',binding.REFERENCE} or
            retained[binding.REFERENCE].get('canonical_reference_owner')!='309210' or
            any(entry['literal_sites']!=187 for entry in retained.values())):
        raise ValueError('Wrong World 30CD00 retained observation ownership')
    if args.full_syntax:
        for role,text in (('normal',alias),('counted',counted)):
            unit = scratch/f'{role}.cpp'
            unit.write_text(text)
            run([args.compiler,'-std=c++20','-g0','-fno-fast-math','-ffp-contract=off',
                 f'-I{ROOT/"include"}',f'-I{ROOT/"src"}','-fsyntax-only',unit],scratch)
            print(f'30CD00 {role} World production alias parsed',file=sys.stderr)
    result = profiles(world,alias,counted,scratch,args)
    print(json.dumps({
        'compiler':run([args.compiler,'--version'],scratch,capture=True).stdout.splitlines()[0],
        'full_syntax':args.full_syntax,'world_pin':binding.PIN,'prepared_pin':binding.PREPARED,
        'header_sha256':binding.digest((ROOT/HEADER).read_text()),
        'genuine_transform_sha256':TRANSFORM_PIN,'genuine_reducer_sha256':reducer,
        'manifest_owners':retained,'profiles':result
    },indent=2))


if __name__ == '__main__':
    main()
