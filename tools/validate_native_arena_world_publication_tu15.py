#!/usr/bin/env python3
"""Private World/Arena interface and exact actual producer syntax proof."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import shlex
import subprocess

import native_arena_world_policy_prototype as prototype
import native_arena_world_producer_inputs as world

ROOT=Path(__file__).resolve().parents[1]
HEADER='src/physics/runtime/native_tu15_arena_world_publication.hpp'
DRIVER='tests/native_tu15_arena_world_publication_tests.cpp'
ARENA='src/physics/physics_game_arena_native.cpp'
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--compiler',action='append')
    parser.add_argument('--sanitizers',action='store_true')
    parser.add_argument('--require-binding',action='store_true')
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    if platform.system()!='Linux'or platform.machine().lower()not in('x86_64','amd64'):return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    names=(HEADER,DRIVER,ARENA,world.WORLD,'tools/native_arena_world_policy_prototype.py',
        'tools/validate_native_arena_world_publication_tu15.py','tools/native_arena_initialize_replacements.py',
        'tools/native_arena_world_producer_inputs.py','src/physics/runtime/native_tu15_required_owner_policy.hpp')
    pins={name:sha(ROOT/name)for name in names}
    report=dict(passed=False,private_unbound=not args.require_binding,source_before=pins,owned_scratch=str(scratch),retained_large_artifacts=[])
    try:
        report['actual_factory_authority']=world.prepare(ROOT,scratch)
        sources={world.WORLD:prototype.world_source((ROOT/world.WORLD).read_text()),ARENA:prototype.arena_source((ROOT/ARENA).read_text())}
        if args.require_binding and any(text!=(ROOT/name).read_text()for name,text in sources.items()):
            raise ValueError('Required installed World/Arena producer binding missing')
        report['actual_private_producers']={name:dict(original_sha256=pins[name],private_sha256=prototype.digest(text))for name,text in sources.items()}
        controls=[]
        for name,source in sources.items():
            current=(ROOT/name).read_text()
            remove=prototype.canonical_world if name==world.WORLD else prototype.canonical_arena
            install=prototype.world_source if name==world.WORLD else prototype.arena_source
            baseline=remove(current)
            if remove(source)!=baseline or install(source)!=source or install(remove(source))!=source:
                raise ValueError('Private producer exact removal/idempotence differs '+name)
            mutations={'removed-include':source[len(prototype.INCLUDE):],
                'duplicate-include':prototype.INCLUDE+source,
                'changed-unrelated-source':source+'\n'}
            if name==world.WORLD:
                mutations['changed-world-owner']=source.replace('*arena,bank,services(),this','*arena,bank,services(),nullptr',1)
                mutations['scope-after-call']=source.replace(prototype.WORLD_SCOPED,prototype.WORLD_CALL+'\n'+prototype.WORLD_SCOPED,1)
                mutations['duplicate-world-scope']=source.replace(prototype.WORLD_SCOPED,prototype.WORLD_SCOPED*2,1)
            else:
                mutations['changed-arena-owner']=source.replace('joined,impl_.get(),&Impl::RequiredOwner','joined,nullptr,&Impl::RequiredOwner',1)
                mutations['removed-matcher']=source.replace(prototype.MATCHER,'',1)
                mutations['duplicate-arena-borrow']=source.replace(prototype.NEW_CALL,prototype.NEW_CALL*2,1)
            for label,changed in mutations.items():
                for api in(remove,install):
                    try:api(changed)
                    except ValueError:controls.append(dict(source=name,mutation=label,api=api.__name__,rejected=True))
                    else:raise ValueError('Private producer authentication accepted '+name+' '+label+' '+api.__name__)
        report['actual_private_producer_authentication']={'exact_original_roundtrip':True,'idempotent':True,'refusals':controls}
        actual={}
        for number,(name,text)in enumerate(sources.items()):
            path=scratch/(str(number)+'.cpp');path.write_text(text);actual[name]=path
        pkg=subprocess.run(['pkg-config','--cflags','sdl2'],capture_output=True,text=True)
        if pkg.returncode:raise ValueError('Actual World SDL compiler arguments unavailable: '+pkg.stderr[-2000:])
        includes=['-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'src/physics'),'-I'+str(ROOT/'src/gameplay/weapons')]
        results=[]
        for compiler in args.compiler or('g++','clang++'):
            binary=scratch/Path(compiler).name
            command=[compiler,'-std=c++20','-O1',*includes,str(ROOT/DRIVER),'-pthread','-o',str(binary)]
            if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            built=subprocess.run(command,capture_output=True,text=True)
            if built.returncode:raise ValueError('Publication interface compile failed: '+built.stderr[-7000:])
            ran=subprocess.run([str(binary)],capture_output=True,text=True,timeout=60)
            if ran.returncode:raise ValueError('Publication interface check failed: '+(ran.stdout+ran.stderr)[-6000:])
            syntax=[]
            for name,path in actual.items():
                check=[compiler,'-std=c++20','-fsyntax-only',*includes,*shlex.split(pkg.stdout),str(path)]
                done=subprocess.run(check,capture_output=True,text=True)
                if done.returncode:raise ValueError('Actual producer syntax failed '+name+': '+done.stderr[-9000:])
                syntax.append(dict(source=name,command=check,passed=True))
            results.append(dict(compiler=compiler,command=command,result=json.loads(ran.stdout),actual_cpp_syntax=syntax))
        report.update(passed=True,compilers=results,scope='Private publication interface identity/lifetime checks and authenticated actual World nested-constructor +Arena named-issuer syntax. This does not execute the production issuer or install a default; genuine public World/child continuation evidence remains separately scoped.')
    except BaseException as error:
        report['error']=repr(error);raise
    finally:
        report['source_after']={name:sha(ROOT/name)for name in names}
        if report['source_after']!=pins:report.update(passed=False,source_drift=True)
        args.report.parent.mkdir(parents=True,exist_ok=True)
        args.report.write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
