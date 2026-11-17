#!/usr/bin/env python3
"""Actual complete sphere source routes with raw/prepared default/reference controls."""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import random
import struct
import subprocess
import sys

from oracle_smg_firing_1_1 import ROOT, Xbe
from validate_native_tu15_prt_helpers import PIN
from native_mopp_ray_dense_original import physical_original_dense

REGIONS=((0x41c000,0x1000),(0x46a000,0x1000),(0x61000000,0x2000),(0x62000000,0x8000))
FILES=('include/halo2/native_tu15_sphere_ray_cast_kernel.hpp',
       'src/physics/runtime/native_tu15_sphere_ray_cast_frame.hpp',
       'src/physics/runtime/native_tu15_raw_x87_scope.hpp',
       'src/physics/runtime/native_tu15_mopp_ray_integer_frame.hpp',
       'src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp',
       'include/halo2/native_tu15_mopp_ray_traversal_kernel.hpp',
       'tests/native_tu15_mopp_ray_frame_test_memory.hpp',
       'tests/native_tu15_sphere_ray_cast_tests.cpp',
       'tools/validate_native_sphere_ray_cast_tu15.py',
       'tools/native_sphere_ray_cast_mutations.py',
       'tools/native_mopp_ray_dense_original.py',
       'tools/validate_native_sphere_ray_cast_routes_tu15.py',
       'tools/native_sphere_ray_cast_inputs.py',
       'tools/native_sphere_ray_cast_replacements.py',
       'tools/native_instanced_ray_child_replacements.py',
       'src/physics/runtime/native_tu15_sphere_ray_cast.hpp',
       'src/physics/physics_live_support_tu15_generated.cpp',
       'tests/native_tu15_sphere_ray_cast_source_tests.cpp',
       'tests/native_tu15_sphere_ray_cast_admission_tests.cpp')
FILES+=('tools/native_sphere_ray_cast_binding_mutations.py',)
FILES+=('tools/generate_physics_live_support_tu15.py',)
def hashes():return {n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest()for n in FILES}

from validate_native_sphere_ray_cast_tu15 import cases

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--xbe',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True);parser.add_argument('--compiler',action='append')
    parser.add_argument('--smoke',action='store_true');parser.add_argument('--sanitizers',action='store_true')
    parser.add_argument('--semantic-mutants',action='store_true');parser.add_argument('--require-binding',action='store_true')
    parser.add_argument('--binding-mutants',action='store_true');args=parser.parse_args()
    if args.semantic_mutants:parser.error('Source binding mutation controls are qualified separately')
    if not args.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    report=dict(passed=False,production_binding=False,source_before=hashes(),owned_scratch=str(scratch),retained_large_artifacts=[])
    try:
        assert hashlib.sha256(args.xbe.read_bytes()).hexdigest()==PIN;image=Xbe(args.xbe)
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        owners=(0x2e8dd0,);common.ENTRIES=owners;original=common.metadata;code=common._Code(image,'.text')
        def metadata(image):
            result=original(image)
            for site in result['sites']:
                op=code.at(site['instruction']).mnemonic
                if op in('pop','ret'):site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
                if op in('fst','fstp'):
                    for operand in site['operands']:operand.update(read=False,write=True)
            return result
        common.metadata=metadata;meta=metadata(image)
        io=[s['instruction']for s in meta['sites']if s['operands']or code.at(s['instruction']).mnemonic in('push','pop','ret')]
        traces=[dict(entry=pc,gp=True,xmm=True)for pc in io];rows=cases(image,args.smoke)
        observed,authority=physical_original_dense(image,rows,scratch,entries=owners,regions=REGIONS,trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
        normal_count=len(rows);prefixes=[];prefix_authority=None
        if not args.smoke:
            for index in range(12):
                row,actual=rows[index],observed['cases'][index]
                for ordinal,event in enumerate(actual['reads']):
                    p=copy.deepcopy(row);p.update(id=normal_count+len(prefixes),name=row['name']+f'_read{ordinal}',phase='before_read',native_fault_ordinal=ordinal,fault=dict(phase='before_read',ordinal=ordinal));prefixes.append(p)
                for ordinal,event in enumerate(actual['writes']):
                    for phase in('before_write','after_write'):
                        p=copy.deepcopy(row);p.update(id=normal_count+len(prefixes),name=row['name']+f'_{phase}{ordinal}',phase=phase,native_fault_ordinal=ordinal,fault=dict(phase=phase,ordinal=ordinal));prefixes.append(p)
            sub=scratch/'prefixes';sub.mkdir();stopped,prefix_authority=physical_original_dense(image,prefixes,sub,entries=owners,regions=REGIONS,trace_calls=traces,trace_extended=True)
            rows+=prefixes;observed['cases']+=stopped['cases']
        executed=set()
        for row,actual in zip(rows,observed['cases'],strict=True):
            assert actual['completed']==(row['phase']=='normal')
            if row['phase']=='normal':assert actual['stopped_at']==0x63000000 and actual['live_x87_unchanged']
            executed.update(actual['executed_sites']);row.update({k:actual[k]for k in('boundary_gp','boundary_eflags','boundary_xmm','after_fp','boundary_live_x87','writes','calls')})
            row['expected_pc']=actual['stopped_at']if row['phase']=='normal'else actual['boundary']['instruction'];row['reads']=actual['reads']
            for packet in row['calls']:packet.pop('arguments')
            after={b['address']:b['after']for b in actual['banks']}
            for bank in row['banks']:bank['after']=after[bank['address']]
        from native_sphere_ray_cast_inputs import prepare
        report['source_routes']=prepare(ROOT,scratch,args.require_binding)
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(image_sha256=PIN,io_sites=io,cases=rows)))
        project=scratch/'project';project.mkdir();cmake=f'''cmake_minimum_required(VERSION 3.20)
project(sphere_ray LANGUAGES CXX)
include(CTest)
add_executable(sphere_ray "{ROOT/'tests/native_tu15_sphere_ray_cast_source_tests.cpp'}")
target_include_directories(sphere_ray PRIVATE "{scratch}" "{ROOT/'include'}" "{ROOT/'src'}")
target_compile_features(sphere_ray PRIVATE cxx_std_20)
target_compile_options(sphere_ray PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME sphere_ray COMMAND sphere_ray "{fixture}" "${{CMAKE_CURRENT_BINARY_DIR}}/sphere.json")
add_test(NAME sphere_ray_reference COMMAND "${{CMAKE_COMMAND}}" -E env HALO2_REFERENCE_SPHERE_RAY_CAST=1 $<TARGET_FILE:sphere_ray> "{fixture}" "${{CMAKE_CURRENT_BINARY_DIR}}/sphere-reference.json")
add_test(NAME sphere_ray_unrelated COMMAND "${{CMAKE_COMMAND}}" -E env HALO2_REFERENCE_INSTANCED_RAY_CHILD=1 $<TARGET_FILE:sphere_ray> "{fixture}" "${{CMAKE_CURRENT_BINARY_DIR}}/sphere-unrelated.json")
add_executable(sphere_admission "{ROOT/'tests/native_tu15_sphere_ray_cast_admission_tests.cpp'}")
target_include_directories(sphere_admission PRIVATE "{ROOT/'include'}" "{ROOT/'src'}")
target_compile_features(sphere_admission PRIVATE cxx_std_20)
add_test(NAME sphere_admission COMMAND sphere_admission)
add_test(NAME sphere_admission_reference COMMAND "${{CMAKE_COMMAND}}" -E env HALO2_REFERENCE_SPHERE_RAY_CAST=1 $<TARGET_FILE:sphere_admission> --reference)
'''
        if args.sanitizers:
            for target in('sphere_ray','sphere_admission'):
                cmake+=f'target_compile_options({target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options({target} PRIVATE -fsanitize=address,undefined)\n'
        (project/'CMakeLists.txt').write_text(cmake);compilers=[];mutants=[]
        report.update(original_paths=len(rows),normal_paths=normal_count,prefixes=len(prefixes),executed_sites=sorted(executed),authority=authority,prefix_authority=prefix_authority)
        for compiler in args.compiler or('g++','clang++'):
            cmd=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(args.output/Path(compiler).name),'--target','sphere_ray','--target','sphere_admission','--filter','^(sphere_ray(_reference|_unrelated)?|sphere_admission(_reference)?)$','--retain-report','sphere.json','--retain-report','sphere-reference.json','--retain-report','sphere-unrelated.json','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler]
            result=subprocess.run(cmd,capture_output=True,text=True);assert result.returncode==0,(result.stdout+result.stderr)[-6000:]
            compilers.append(dict(compiler=compiler,passed=True,observations={path:json.loads((args.output/Path(compiler).name/'evidence'/path).read_text())for path in('sphere.json','sphere-reference.json','sphere-unrelated.json')}))
            if args.binding_mutants:
                from native_sphere_ray_cast_binding_mutations import qualify
                mutants.append(qualify(scratch,args.output,fixture,compiler,args.sanitizers))
        parent_sites=set(common.complete_body(code,0x2e8dd0).code)
        report.update(passed=True,compilers=compilers,semantic_mutants=mutants,parent_sites=len(parent_sites),parent_executed_sites=len(parent_sites&executed))
        if not args.smoke:assert len(parent_sites)==147 and parent_sites<=executed
    except BaseException as error:report.update(passed=False,error=str(error)or repr(error));raise
    finally:
        report['source_after']=hashes()
        if report['source_before']!=report['source_after']:report.update(passed=False,source_drift=True)
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
