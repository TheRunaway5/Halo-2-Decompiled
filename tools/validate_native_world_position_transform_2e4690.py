#!/usr/bin/env python3
"""Qualify the selected World 2E4690 binder and original provider boundary."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]
WORLD = 'src/physics/runtime/physics_world_runtime_generated.cpp'
FRAME = 'src/physics/runtime/native_tu15_geometry_leaf_frame.hpp'
ADAPTER = 'src/physics/runtime/native_tu15_world_position_transform_2e4690.hpp'
FIXTURE = 'tests/native_tu15_world_position_transform_2e4690_tests.cpp'
SWITCH = 'HALO2_REFERENCE_WORLD_POSITION_TRANSFORM_2E4690'


def run(argv, cwd, env=None):
    result = subprocess.run(argv,cwd=cwd,env=env,text=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    if result.returncode:
        raise RuntimeError(f'{argv[0]} exited {result.returncode}\n{result.stdout[-1200:]}{result.stderr[-4000:]}')
    return result.stdout


def staged(scratch, delta):
    source = scratch/'source'
    source.mkdir()
    for directory in ('src','include','tools','tests','cmake'):
        run(['cp','-al',ROOT/directory,source/directory],scratch)
    for file in (FRAME,'tools/specialize_native_operands.py','cmake/NativeOperandSpecialization.cmake',
                 'tools/validate_native_contact_cache_update_30cd00.py'):
        path = source/file
        data = path.read_bytes()
        path.unlink()
        path.write_bytes(data)
    run(['patch','--batch','--forward','-p1','-i',delta.resolve()],source)
    return source


def count(path, stats, mode):
    report = json.loads(path.read_text())
    rows = [dict(zip(report['row_fields'],row,strict=True)) for row in report['rows']]
    owners = [row for row in rows if row['symbol']=='2e0ba0']
    if (sum(row['entries'] for row in owners)!=stats['cases'] or
            sum(row['body_entries'] for row in owners)!=stats['retained'] or
            sum(row['returns_without_body']+row['unwinds_without_body'] for row in owners)!=stats['native'] or
            sum(row['unwinds'] for row in owners)!=stats['faults']):
        raise ValueError(f'World 2E4690 accounting mismatch for {mode}')
    return owners


def qualify(args, root, scratch):
    sys.path.insert(0,str(root/'tools'))
    import native_world_position_transform_2e4690_replacements as binding
    from instrument_native_execution import instrument_translation, unit_declaration
    import specialize_native_operands as loader
    from native_world_leaves_replacements import body as world_body

    canonical = (root/WORLD).read_text()
    alias, metadata = loader.specialize(canonical)
    if metadata.get('native_spatial_owners',[]).count('2e4690')!=1 or binding.install(canonical,alias)!=(alias,['2e4690']):
        raise ValueError('World 2E4690 exact prepared owner not bound')
    dispatcher = (root/'src/physics/physics_live_entries_tu15.hpp').read_text()
    live_world = (root/'src/physics/physics_live_world_tu15.cpp').read_text()
    if (dispatcher.count('{0x2e4690u,0x2e0ba0u,Module::World},')!=1 or
            live_world.count('case Module::World:source_run_physics_world_runtime_selected_observations(m,e.canonical,a,image,fp,services(),o);return;')!=1 or
            live_world.count('services.immutable_instruction_cursor.services_=&services;')!=1):
        raise ValueError('World 2E4690 selected executor ownership changed')

    if args.staging_patch:
        active_path = ROOT/'tools/specialize_native_operands.py'
        active = type(loader)('active_specializer')
        active.__file__ = str(active_path)
        exec(compile(active_path.read_text(),str(active_path),'exec'),vars(active))
        prior = active.specialize(canonical)[0]
        bound = binding.install(canonical,prior)[0]
        if bound.replace(binding.INCLUDE,'',1)!=alias.replace(binding.INCLUDE,'',1):
            raise ValueError('World binding changed other prepared owners')
        if world_body(prior,'2e0cc0')[0]!=world_body(alias,'2e0cc0')[0]:
            raise ValueError('World 2E47B0 changed')

    changed = canonical.replace('case 0x2e0ba0u:return 0x2e4690u;','case 0x2e0ba0u:return 0x2e4691u;',1)
    try:
        binding.install(changed,alias)
    except ValueError:
        pass
    else:
        raise ValueError('Wrong World 2E4690 selected route accepted')

    accounted, manifest, _ = instrument_translation(alias,canonical,WORLD,root)
    owner, = [row for row in manifest['functions'] if row['symbol']=='2e0ba0']
    if owner['literal_sites']!=26 or owner['dynamic_instruction_forwarders']:
        raise ValueError('World 2E4690 retained sites changed')
    flags = ['-std=c++20','-O2','-g0','-fno-fast-math','-ffp-contract=off','-frounding-math',
             *(f'-I{path}' for path in (root/'include',root/'src',root/'tests',scratch))]
    if args.full_syntax:
        for name, source in (('normal',alias),('accounted',accounted)):
            path = scratch/f'{name}.cpp'
            path.write_text(source)
            run([args.compiler,*flags,'-fsyntax-only',path],scratch)
            print(f'World 2E4690 production {name} alias parsed',file=sys.stderr,flush=True)

    plain_reference = binding.translated(canonical).replace('run_2e0ba0(','position_reference(',1)
    build_results = {}
    for profile, source, cases in (('release',alias,args.cases),('accounted',accounted,args.cases),
                                   ('sanitized',alias,args.sanitized_cases)):
        if not cases:
            continue
        program = scratch/f'position-{profile}'
        extra = []
        selected_source = [root/FIXTURE]
        fuse_accounting = profile=='accounted'
        if fuse_accounting:
            selected_source.append(root/'src/runtime/native_execution_accounting.cpp')
        if profile=='sanitized':
            extra = ['-O1','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-pie','-no-pie']
        identity = ('#include "halo2/native_execution_accounting.hpp"\n'+unit_declaration(WORLD,canonical,alias)
                    if fuse_accounting else '')
        (scratch/'world_position_accounting.inc').write_text(identity)
        (scratch/'world_position_owner.inc').write_text(plain_reference+'\n'+binding.BODY.search(source)[0]+'\n')
        run([args.compiler,*flags,*extra,*selected_source,'-o',program],scratch)
        results = {}
        for mode in ('native','reference'):
            report = scratch/'counts.json'
            env = {k:v for k,v in os.environ.items() if k not in (SWITCH,'HALO2_NATIVE_EXECUTION_REPORT')}
            if fuse_accounting:
                env['HALO2_NATIVE_EXECUTION_REPORT'] = str(report)
            measured = json.loads(run([program,str(cases),mode],scratch,env))
            if fuse_accounting:
                measured['counted_owner'] = count(report,measured,mode)
            results[mode] = measured
        build_results[profile] = results

    if args.physical_report:
        run([sys.executable,root/'tools/validate_native_geometry_leaf_tu15.py','--smoke','--prefixes',
             '--compiler',args.compiler,'--report',args.physical_report.resolve()],scratch)

    def digest(file):
        return hashlib.sha256((root/file).read_bytes()).hexdigest()

    print(json.dumps({'passed':True,'compiler':run([args.compiler,'--version'],scratch).splitlines()[0],
        'canonical_body_sha256':binding.PIN,'frame_sha256':digest(FRAME),'adapter_sha256':digest(ADAPTER),
        'sibling_2e47b0_prepared_sha256':hashlib.sha256(world_body(alias,'2e0cc0')[0].encode()).hexdigest(),
        'full_production_syntax':args.full_syntax,'manifest_owner':owner,'profiles':build_results,
        'physical_frame_regression':str(args.physical_report) if args.physical_report else None},indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler',default='g++')
    parser.add_argument('--cases',type=int,default=256)
    parser.add_argument('--sanitized-cases',type=int,default=128)
    parser.add_argument('--staging-patch',type=Path)
    parser.add_argument('--full-syntax',action='store_true')
    parser.add_argument('--physical-report',type=Path)
    args = parser.parse_args()
    owned = os.environ.get('HALO2_TEST_SCRATCH')
    if not owned or not Path(owned).is_dir():
        parser.error('Run through tools/run_test.py for owned scratch')
    scratch = Path(owned)
    root = staged(scratch,args.staging_patch) if args.staging_patch else ROOT
    qualify(args,root,scratch)


if __name__=='__main__':
    main()
