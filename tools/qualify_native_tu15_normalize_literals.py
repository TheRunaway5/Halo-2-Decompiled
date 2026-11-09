#!/usr/bin/env python3
"""Freeze all eight literal30BD0 bodies and compare typed/native transports.

Every generated source/header, unchanged original ELF, probe and raw golden is
owned by run_test.py. Only bounded hashes and mismatch diagnostics are retained.
"""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

import validate_native_tu15_normalize_vector as reference
from native_normalize_vector_bridge_tu15 import literal_body,prefix

ROOT=Path(__file__).resolve().parents[1]


def generate_literals(project,fresh_primary=None):
    cohort=json.loads((ROOT/'analysis/renderer-native-normalizer-20261101/body-cohort.json').read_text())['bodies']
    if fresh_primary:
        cohort.append(dict(path=str(fresh_primary),literal_body_sha256=
            '91940d101dc7a0bdb88bf74512b76902b09458abb7abd5dae0f1e2998912de6c'))
    header='#pragma once\n#include "physics/vehicles/physics_vehicle_motion_internal.hpp"\n#include "halo2/unit_animation_selection_runtime.hpp"\nnamespace normalize_literals {\n'
    pins={};variants=[]
    for n,item in enumerate(cohort):
        source=(ROOT/item['path']).read_text();bound=re.search(r'static void run_30bd0\(\w+ &s\) \{.*?\n\}',source,re.S).group()
        # Once installed, strip only our exact guarded prefix; the literal
        # owner must remain byte-for-byte equal to its reviewed input.
        profile='Mp11Literal'if n==0 else'ImageSelected'if n==2 else'Tu15Literal'
        assert prefix(profile) in bound,item['path']
        body=literal_body(bound)
        digest=hashlib.sha256(body.encode()).hexdigest();assert digest==item['literal_body_sha256'],item['path']
        ns='animation'if n==0 else 'body'+str(n)
        header+='namespace '+ns+' {\n'
        header+='using namespace halo2;using namespace halo2::physics_world_runtime_detail;\n'
        if n==0:
            start=source.index('struct State : physics_contact_runtime_detail::State {')
            state=source[start:source.index('\n};',start)+3]
            header+='using namespace halo2::physics_contact_runtime_detail;\n'+state+'\n'
            pins[item['path']+'#state']=hashlib.sha256(state.encode()).hexdigest()
        if n==2:header+='using namespace halo2::physics_vehicle_motion_detail;\n'
        if 'selected(' in body:
            selected=re.search(r'std::uint32_t selected\(.*?\n',source).group()
            header+=selected;pins[item['path']+'#selected']=hashlib.sha256(selected.encode()).hexdigest()
        if 'native_single_spill(' in body:
            spill=re.search(r'std::uint32_t native_single_spill\(.*?\n\}',source,re.S).group()
            header+=spill+'\n';pins[item['path']+'#spill']=hashlib.sha256(spill.encode()).hexdigest()
        header+=body+'\n'+bound.replace('run_30bd0(','run_bound_30bd0(',1)+'\n}\n';pins[item['path']+'#body']=digest
        pins[item['path']+'#bound']=hashlib.sha256(bound.encode()).hexdigest()
        variants.append(dict(index=n,path=item['path'],profile='mp11'if n==0 else 'image'if n==2 else'tu15'))
    header+='''template<class S>void run(int index,S&s,bool bound){
if constexpr(std::is_same_v<S,animation::State>){if(index!=0)throw std::runtime_error("Wrong animation variant");if(bound)animation::run_bound_30bd0(s);else animation::run_30bd0(s);}
else if constexpr(std::is_same_v<S,halo2::physics_vehicle_motion_detail::MotionState>){if(index!=2)throw std::runtime_error("Wrong motion variant");if(bound)body2::run_bound_30bd0(s);else body2::run_30bd0(s);}
else {switch(index){'''+''.join(f'case {n}:if(bound)body{n}::run_bound_30bd0(s);else body{n}::run_30bd0(s);break;'for n in range(1,len(cohort))if n!=2)+'''default:throw std::runtime_error("Wrong world variant");}}}
}
'''
    (project/'normalize_literal_variants.hpp').write_text(header)
    return variants,pins


def ordinary_banks(before,after):
    pages={}
    def put(address,raw):
        while raw:
            base=address&~4095;n=min(len(raw),4096-(address&4095))
            pages.setdefault(base,bytearray(4096))[address-base:address-base+n]=raw[:n]
            address+=n;raw=raw[n:]
    for b in before:put(b['address'],bytes.fromhex(b['before']))
    initial=[dict(address=p,before=data.hex())for p,data in sorted(pages.items())]
    for b in after:put(b['address'],bytes.fromhex(b['after']))
    final=[dict(address=p,after=data.hex())for p,data in sorted(pages.items())]
    return initial,final


def check(fixture,native,report):
    golden=json.loads(fixture.read_text());process=subprocess.Popen([str(native)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
    count=0;admitted=0;fallbacks=0;mismatches=[];field_counts={};digest=hashlib.sha256()
    def send(row):
        process.stdin.write(json.dumps(row,separators=(',',':'))+'\n');process.stdin.flush();line=process.stdout.readline()
        if not line:raise ValueError(process.stderr.read()[-3000:])
        return json.loads(line)
    def value(result):
        return {k:result[k]for k in ('gp','xmm','eflags','fp','live_x87','banks','rejected')}
    try:
        for variant in golden['variants']:
            images=('mp11',)if variant['index']==0 else ('tu15','mp11')
            for image in images:
                # TU1.5 literal operands stay TU1.5 even in a legacy State.
                actual_image=image if variant['profile']=='image'else variant['profile']
                inputs=golden[actual_image]['rows'];proof=golden[actual_image]['proof']['cases']
                for row,original in zip(inputs,proof,strict=True):
                    r=copy.deepcopy(row);r.update(image=image,variant=variant['index'],profile=variant['profile'])
                    native=send(r);r['literal']=True;literal=send(r)
                    expected=dict(gp=original['boundary_gp'],xmm=original['boundary_xmm'],eflags=original['boundary_eflags'],
                        fp=original['after_fp'],live_x87=r['live_x87'],banks=sorted(original['banks'],key=lambda b:b['address']),rejected=False)
                    if value(native)!=expected:raise ValueError(f'Native physical mismatch: {variant["path"]} {row["name"]}')
                    # Both implementations are compared to physical authority;
                    # retain any pre-existing literal defects, never bless them
                    # by treating translated output as the oracle.
                    different=[k for k in expected if literal[k]!=expected[k]]
                    if different:
                        for key in different:field_counts[key]=field_counts.get(key,0)+1
                        if len(mismatches)<16:mismatches.append(dict(path=variant['path'],name=row['name'],image=image,fields=different,
                            native={k:native[k]for k in different if k!='banks'},literal={k:literal[k]for k in different if k!='banks'}))
                    # The non-stack source publications stay in original order.
                    writes=[[e['address'],e['written']]for e in original['writes']if not reference.STACK<=e['address']<reference.STACK+4096]
                    reads=[[e['address'],e['bytes']]for e in original['reads']if not reference.STACK<=e['address']<reference.STACK+4096]
                    for result in (native,literal):
                        if [e for e in result['writes']if not reference.STACK<=e[0]<reference.STACK+4096]!=writes:
                            raise ValueError('Vector publication order differs')
                        if [e for e in result['reads']if not reference.STACK<=e[0]<reference.STACK+4096]!=reads:
                            raise ValueError('Vector read order differs')
                    # Execute the actual installed body with committed direct
                    # pages and its native services pointer, not a wrapper
                    # around the new kernel. Legacy TU1.5-literal operands
                    # deliberately fall back under a MapPack11 State.
                    bound_row=copy.deepcopy(r);bound_row.update(bound=True,kernel=True,direct=True)
                    bound_row['banks'],bound_after=ordinary_banks(row['banks'],original['banks'])
                    bound=send(bound_row);bound_expected={**expected,'banks':bound_after}
                    if value(bound)!=bound_expected:raise ValueError(f'Installed body physical mismatch: {variant["path"]} {row["name"]}')
                    if [e for e in bound['writes']if not reference.STACK<=e[0]<reference.STACK+4096]!=writes:
                        raise ValueError('Installed body vector publication order differs')
                    wants_native=not(variant['profile']=='tu15'and image=='mp11')
                    if bound['admitted']!=int(wants_native):raise ValueError('Installed body took the wrong native/reference path')
                    if not wants_native and bound['refused'][1]!=1:raise ValueError('Legacy literal did not retain its image refusal')
                    admitted+=wants_native;fallbacks+=not wants_native
                    digest.update(json.dumps(value(native),sort_keys=True,separators=(',',':')).encode());count+=1
        process.stdin.close();assert process.wait(timeout=10)==0,process.stderr.read()[-3000:]
    finally:
        if process.poll()is None:process.terminate();process.wait(timeout=10)
    report.write_text(json.dumps(dict(passed=not field_counts,native_physical_comparisons=count,comparison_sha256=digest.hexdigest(),
        installed_body_cases=count,installed_native_admissions=admitted,installed_legacy_fallbacks=fallbacks,
        literal_mismatch_field_counts=field_counts,literal_witnesses=mismatches,all_literal_variants_equal_physical=not field_counts,
        scope='Native must match unchanged physical code in every case; literal deviations are measured diagnostics, not acceptance authority.'),indent=2)+'\n')
    if field_counts:raise ValueError('Retained literal differs from physical code: '+str(field_counts))
    print('PASS',count,'native/retained/installed physical comparisons;',admitted,'native admissions;',fallbacks,'legacy fallbacks')


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path);p.add_argument('--check',type=Path)
    p.add_argument('--native',type=Path);p.add_argument('--report',type=Path)
    p.add_argument('--fresh-primary',action='store_true');p.add_argument('--sanitizers',action='store_true');a=p.parse_args()
    if a.check:return check(a.check,a.native,a.report)
    assert a.output;a.output.mkdir(parents=True,exist_ok=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'source';project.mkdir()
    shutil.copytree(ROOT/'include',project/'include')
    headers=['src/physics/runtime/'+n for n in ['physics_world_runtime_internal.hpp','physics_contact_runtime_internal.hpp','physics_solver_internal.hpp','physics_step_internal.hpp','physics_instruction_remap_tu15.hpp','native_tu15_normalize_vector_bridge.hpp']]
    headers+=['src/physics/vehicles/physics_vehicle_motion_internal.hpp','src/physics/vehicles/physics_vehicle_runtime_internal.hpp']
    for name in headers:
        target=project/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,target)
    shutil.copyfile(ROOT/'tests/native_tu15_normalize_vector_tests.cpp',project/'probe.cpp')
    shutil.copyfile(ROOT/'src/campaign/ai/native_campaign_ai_vector_math_tu15.cpp',project/'math.cpp')
    fresh=None
    if a.fresh_primary:
        fresh=scratch/'fresh-primary.cpp'
        subprocess.run([sys.executable,'-B',str(ROOT/'tools/generate_physics_live_support_tu15.py'),
            '--output',str(fresh),'--manifest',str(scratch/'fresh-primary-generation.json')],cwd=ROOT,check=True)
    variants,pins=generate_literals(project,fresh)
    from oracle_smg_firing_1_1 import Xbe
    paths={'tu15':Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe'),
        'mp11':ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'}
    golden=dict(variants=variants)
    for image,path in paths.items():
        assert hashlib.sha256(path.read_bytes()).hexdigest()==reference.PINS[image]
        rows=reference.rows(image);proof,owners=reference.physical(Xbe(path),rows,scratch/image);golden[image]=dict(rows=rows,proof=proof,owners=owners)
    fixture=scratch/'physical.json';fixture.write_text(json.dumps(golden,separators=(',',':')))
    (project/'CMakeLists.txt').write_text(f'''cmake_minimum_required(VERSION 3.20)
project(NormalizeLiteralVariants LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
include("{ROOT}/cmake/TestStorage.cmake")
add_executable(normalize_literals probe.cpp math.cpp)
target_include_directories(normalize_literals PRIVATE include src "${{CMAKE_CURRENT_SOURCE_DIR}}")
target_compile_features(normalize_literals PRIVATE cxx_std_20)
target_compile_definitions(normalize_literals PRIVATE HALO2_NORMALIZE_LITERAL_VARIANTS=1)
target_compile_options(normalize_literals PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
if(HALO2_SANITIZERS)
    target_compile_options(normalize_literals PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(normalize_literals PRIVATE -fsanitize=address,undefined)
endif()
target_link_libraries(normalize_literals PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME normalize_literals COMMAND "{ROOT}/.tools/python/bin/python" "{Path(__file__).resolve()}" --check "{fixture}" --native $<TARGET_FILE:normalize_literals> --report "${{CMAKE_CURRENT_BINARY_DIR}}/literals.json")
''')
    results=[]
    for compiler in ('g++','clang++'):
        kind='gcc'if compiler=='g++'else'clang'
        cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/kind),
            '--target','normalize_literals','--filter','^normalize_literals$','--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,
            '--retain-report','literals.json','--retain-report','Testing/Temporary/LastTest.log']
        if a.sanitizers:cmd.append('--sanitizers')
        run=subprocess.run(cmd);results.append(dict(kind=kind,returncode=run.returncode,command=cmd))
        if run.returncode:break
    for name in headers+['tests/native_tu15_normalize_vector_tests.cpp','src/campaign/ai/native_campaign_ai_vector_math_tu15.cpp',str(Path(__file__).relative_to(ROOT))]:
        pins[name]=hashlib.sha256((ROOT/name).read_bytes()).hexdigest()
    passed=len(results)==2 and all(r['returncode']==0 for r in results)
    (a.output/'validation.json').write_text(json.dumps(dict(passed=passed,results=results,source_sha256=pins,owned_scratch=str(scratch),large_artifacts_retained=[]),indent=2)+'\n')
    return int(not passed)


if __name__=='__main__':raise SystemExit(main())
