#!/usr/bin/env python3
"""Private live World/Arena source-owner guard and continuation interfaces.

The standalone friend issuer fixture cannot establish actual World ownership;
that claim belongs to the separately frozen, complete public-engine proof.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shlex
import subprocess

import native_arena_world_initialize_replacements as binding
import specialize_native_operands as specialization
from validate_native_arena_initialize_frame_tu15 import corpus

ROOT=Path(__file__).resolve().parents[1]
UNIT='src/physics/physics_game_arena_runtime_generated.cpp'
DRIVER='tests/native_tu15_arena_world_source_tests.cpp'
OWNER='src/physics/runtime/native_tu15_arena_world_initialize_owner.hpp'


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def prepare(scratch):
    raw=binding.remove_arena_world_initialize((ROOT/UNIT).read_text())
    bound=binding.replace_arena_world_initialize(raw)
    prepared=specialization.specialize(raw)[0]
    prepared_bound=binding.retag_specialized(bound,specialization.specialize(bound)[0])
    rows=[]
    for label,reference,candidate in(('raw',raw,bound),('prepared',prepared,prepared_bound)):
        binding.installed_entries(candidate)
        resolver=binding.one(reference,r'^std::uint32_t selected\([^\n]+','resolver')
        original=binding.BODY.search(reference)[0]
        complete=binding.BODY.search(candidate)[0]
        # This isolated bridge proof does not issue the closed callback-free
        # Borrow. Remove only that exact already-authenticated fallback prefix
        # to avoid importing/redefining the actual public Arena class here.
        projected=complete.replace(binding.legacy.PREFIX,'',1)
        for route,body in(('reference',original),('candidate',projected)):
            counted,n=re.subn(r'if\(s.services.instruction\)\{s.services.instruction\((0x[0-9a-f]+u)\);\}|s.instruction_literal\((0x[0-9a-f]+u)\);',
                lambda m:'statement();'+m[0],body)
            if n!=45 or counted.count(binding.REPEAT)!=1:
                raise ValueError('World Arena exact source counter projection drift')
            counted=counted.replace(binding.REPEAT,'statement();'+binding.REPEAT,1)
            (scratch/('arena_world_owner_'+label+'_'+route+'.inc')).write_text(resolver+'\n'+counted+'\n')
        rows.append(dict(format=label,original_body_sha256=binding.digest(original),
            complete_authenticated_body_sha256=binding.digest(complete),
            isolated_bridge_projection_sha256=binding.digest(projected),callback_free_fallback_removed_for_interface_only=True))
    return rows


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--report',type=Path,required=True)
    p.add_argument('--compiler',action='append')
    p.add_argument('--sanitizers',action='store_true')
    p.add_argument('--semantic-mutants',action='store_true')
    p.add_argument('--smoke',action='store_true')
    p.add_argument('--require-binding',action='store_true')
    a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    if platform.system()!='Linux'or platform.machine().lower()not in('x86_64','amd64'):return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    names=(UNIT,DRIVER,OWNER,'tools/validate_native_arena_world_source_tu15.py',
        'tools/native_arena_world_initialize_replacements.py','tools/native_arena_initialize_replacements.py',
        'tools/native_arena_initialize_continuation_replacements.py','tools/specialize_native_operands.py',
        'src/physics/physics_live_world_tu15.cpp','src/physics/physics_game_arena_native.cpp',
        'tools/native_arena_world_policy_prototype.py')
    pins={name:sha(ROOT/name)for name in names}
    headers={path.resolve():sha(path)for tree in('include','src','tests')for path in(ROOT/tree).rglob('*')if path.suffix in('.hpp','.h','.inc','.cpp')}
    report=dict(passed=False,private_unbound=not a.require_binding,owned_scratch=str(scratch),source_before=pins,retained_large_artifacts=[])
    try:
        if a.require_binding:
            import native_arena_world_policy_prototype as publication
            for name,install in((UNIT,binding.replace_arena_world_initialize),
                ('src/physics/physics_live_world_tu15.cpp',publication.world_source),
                ('src/physics/physics_game_arena_native.cpp',publication.arena_source)):
                source=(ROOT/name).read_text()
                if install(source)!=source:raise ValueError('Required installed World Arena source/producer binding missing '+name)
            report['installed_source_and_producer_pins_proved']=True
        report['source_profiles']=prepare(scratch)
        rows=corpus()
        chosen=rows[:2]if a.smoke else[rows[n]for n in sorted({0,1,2,15,16,len(rows)//2,len(rows)-2,len(rows)-1})if n<len(rows)]
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=chosen),separators=(',',':')))
        results=[]
        native_env=dict(os.environ)
        for name in('HALO2_REFERENCE_ARENA_INITIALIZE','HALO2_INSTRUCTION_CENSUS','HALO2_REFERENCE_INSTRUCTION_CALLBACKS','HALO2_REFERENCE_LITERAL_INSTRUCTIONS'):
            native_env.pop(name,None)
        for compiler in a.compiler or('g++','clang++'):
            binary=scratch/Path(compiler).name;dep=binary.with_suffix('.d')
            command=[compiler,'-std=c++20','-O1','-g0','-fno-fast-math','-ffp-contract=off','-frounding-math',
                '-ffunction-sections','-fdata-sections','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),
                '-I'+str(ROOT/'tests'),'-I'+str(scratch),str(ROOT/DRIVER),'-Wl,--gc-sections','-o',str(binary),'-MD','-MF',str(dep)]
            if a.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            done=subprocess.run(command,capture_output=True,text=True)
            if done.returncode:raise ValueError('World Arena source interface compile failed: '+done.stderr[-8000:])
            for word in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(word).resolve()
                if path in headers:pins[str(path.relative_to(ROOT))]=headers[path]
            done=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=120,env=native_env)
            if done.returncode:raise ValueError('World Arena source interface failed: '+(done.stdout+done.stderr)[-9000:])
            result=dict(compiler=compiler,command=command,result=json.loads(done.stdout),compiled_mutants=[])
            reference_env=dict(native_env,HALO2_REFERENCE_ARENA_INITIALIZE='1')
            positive=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=120,env=reference_env)
            if positive.returncode:raise ValueError('World Arena source positive reference failed: '+(positive.stdout+positive.stderr)[-9000:])
            result['positive_reference']=json.loads(positive.stdout)
            if result['positive_reference']['prepared_native_calls']or not result['positive_reference']['forced_reference']:
                raise ValueError('World Arena source positive reference did not execute complete fallback')
            report['compilers']=results+[result]
            if a.semantic_mutants:
                defects=(
                    ('raw-callbacks-admitted',OWNER,'if constexpr(!Prepared)return false;','(void)Prepared;','World Arena raw callbacks incorrectly admitted'),
                    ('unqualified-world-publication',OWNER,'!native_arena_world_publication_detail::Borrow::qualified(s.memory,s.services)','false','World Arena untouched entry refusal failed'),
                    ('other-image-admitted',OWNER,'s.image!=SourcePhysicsContactImage::TitleUpdate15','false','World Arena untouched entry refusal failed'),
                    ('missing-returned-policy','src/physics/runtime/native_tu15_arena_initialize_source_frame.hpp','if(!continuation(s))throw Continuation{next,&token};','(void)next;','World Arena source continuation differs'),
                )
                for label,relative,old,new,witness in defects:
                    source=(ROOT/relative).read_text()
                    if source.count(old)!=1:raise ValueError('Missing source-owner semantic seam '+label)
                    overlay=scratch/(Path(compiler).name+'-'+label);header=overlay/relative;header.parent.mkdir(parents=True);header.write_text(source.replace(old,new,1))
                    mutant=overlay/'test';changed=list(command);changed.insert(1,'-I'+str(overlay/'src'));changed[changed.index('-o')+1]=str(mutant)
                    built=subprocess.run(changed,capture_output=True,text=True)
                    if built.returncode:raise ValueError('World Arena source mutant did not compile '+label+': '+built.stderr[-5000:])
                    rejected=subprocess.run([str(mutant),str(fixture)],capture_output=True,text=True,timeout=120,env=native_env)
                    diagnostic=(rejected.stdout+rejected.stderr)[-4000:]
                    if rejected.returncode==0 or witness not in diagnostic:
                        raise ValueError('World Arena source mutant survived/unrecognized '+label+': '+diagnostic)
                    result['compiled_mutants'].append(dict(name=label,rejected=True,exit_code=rejected.returncode,diagnostic=diagnostic))
            results.append(result)
        report.update(passed=True,compilers=results,declared_original_cases=len(chosen),
            scope='Controller/admission interface: exact raw/prepared source bodies, untouched entry fallback, actual retired-I/O publication and genuine reservation-child entry stops. Optional binding requirement authenticates complete installed source/producer files. Standalone friend issuers are test-only; complete public World construction/provider and children are qualified separately. No independent runtime, raw architectural fault or all-native descendant claim.')
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:sha(ROOT/name)for name in pins}
        if report['source_after']!=pins:report.update(passed=False,source_drift=True)
        a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
