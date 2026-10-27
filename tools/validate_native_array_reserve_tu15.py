#!/usr/bin/env python3
"""Physical TU1.5 reserve-array proof with actual native allocator closure.

Reuse the authenticated array/pool hardware observer and unchanged original
inputs, not allocator outcomes. Run through run_test.py; no raw products survive.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import validate_native_array_growth_tu15 as reference

ROOT=reference.ROOT
FILES=(*reference.FILES,'include/halo2/native_tu15_array_reserve.hpp',
       'src/physics/native_tu15_array_reserve.cpp',
       'tools/validate_native_array_growth_tu15.py',__file__)

def cases():
    result=[]
    for case in reference.cases(False):
        if case['entry']!=0x2e3250:
            continue
        case=dict(case)
        stride=case['arguments'][1]
        capacity=(case['count']+3)&0xffffffff
        case['entry']=0x2e31e0
        case['arguments']=[reference.HEADER,capacity,stride]
        case['name']='reserve_'+case['name']
        case['mutations']=[dict(m,instruction=0x2e3238) if m['instruction']==0x2e32b3 else m
                           for m in case.get('mutations',[])]
        result.append(case)
    for capacity,stride in ((0,0),(1,0),(0x80000001,2),(0x40000001,4)):
        result.append(dict(name=f'reserve_capacity_wrap_{capacity:x}_{stride}',
            entry=0x2e31e0,arguments=[reference.HEADER,capacity,stride],
            receiver=0x486080,header=reference.HEADER,count=0,capacity=0x80000000))
    def argument_case(name,**extra):
        result.append(dict(name='retained_arguments_'+name,entry=0x2e31e0,
            arguments=[reference.HEADER,7,128],receiver=0x486080,
            header=reference.HEADER,count=4,capacity=0x80000004,
            retained_arguments=True,**extra))
    argument_case('complete')
    for name,offset,value in (('cached_stride',12,1),('cached_capacity',8,99),
                              ('late_header',4,reference.HEADER+0x100)):
        extra=dict(mutations=[dict(instruction=0x23328f,
            address=reference.SP+offset,written=struct.pack('<I',value).hex())])
        if name=='late_header':extra['user_words']=[(0x100,reference.DATA+0x100),(0x104,1),(0x108,0x80000001)]
        argument_case(name,**extra)
    for name,offset in (('stride_fault',12),('capacity_fault',8),('header_fault',4)):
        argument_case(name,fault=dict(address=reference.SP+offset,bytes=4,occurrence=1,write=False))
    return result

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',required=True,type=Path)
    args=parser.parse_args();args.output=args.output.resolve()
    args.output.mkdir(parents=True,exist_ok=False)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    pins={str(Path(p).relative_to(ROOT)) if Path(p).is_absolute() else p:
          reference.sha(ROOT/p) for p in FILES}
    report=dict(passed=False,source_sha256=pins,scratch_cleanup_owner=str(scratch),
                allocator_outcomes_supplied=False,retained_large_artifacts=[])
    def save():(args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    save()
    try:
        assert reference.sha(reference.NEW_DEFAULT)==reference.NEW_PIN
        image=reference.Xbe(reference.NEW_DEFAULT)
        banks,provenance=reference.reference_inputs(image)
        rows=cases()
        # The original metadata closure differs only by the authenticated root.
        reference.OWNERS=(0x2e31e0,*reference.OWNERS[1:])
        directory,authority=reference.original(image,banks,rows,scratch)
        fixture=directory/'original.json'
        fixture.write_text(json.dumps(dict(image_sha256=reference.NEW_PIN,
            oracle='unchanged-tu15-physical-array-and-real-pool-v1',cases=rows),separators=(',',':')))
        report.update(original_cases=len(rows),source_authority=authority,input_provenance=provenance,
            physical_original_instructions=sum(c['original_instructions']for c in rows),
            ordered_original_reads=sum(len(c['reads'])for c in rows),
            ordered_original_stores=sum(len(c['writes'])for c in rows),
            fault_prefixes=sum(c['declared_fault']for c in rows),
            actual_allocator_calls=sum(len(c['allocator_calls'])for c in rows),
            actual_argument_cell_cases=sum(c.get('retained_arguments',False)for c in rows),
            fixture_sha256=reference.sha(fixture),all_original_instructions_unchanged=True)
        save()
        project=scratch/'project';project.mkdir()
        shutil.copytree(ROOT/'include',project/'include')
        for path in FILES:
            relative=Path(path).relative_to(ROOT) if Path(path).is_absolute()else Path(path)
            target=project/relative;target.parent.mkdir(parents=True,exist_ok=True)
            shutil.copyfile(ROOT/relative,target)
        (project/'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.20)
project(native_array_reserve LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json REQUIRED)
add_executable(native_array_reserve_tests tests/native_tu15_array_growth_tests.cpp
  src/physics/native_tu15_array_growth.cpp src/physics/native_tu15_array_allocator.cpp
  src/physics/native_tu15_array_reserve.cpp)
target_include_directories(native_array_reserve_tests PRIVATE include)
target_compile_features(native_array_reserve_tests PRIVATE cxx_std_20)
target_compile_definitions(native_array_reserve_tests PRIVATE HALO2_NATIVE_ARRAY_RESERVE=1)
target_compile_options(native_array_reserve_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(native_array_reserve_tests PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME native_array_reserve_original_tu15 COMMAND native_array_reserve_tests "'''+str(fixture)+'''")
''')
        def validate(label,compiler='g++'):
            command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),
                '--scratch-root',str(scratch/'builds'),'--output',str(args.output/label),
                '--target','native_array_reserve_tests','--filter','^native_array_reserve_original_tu15$',
                '--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1']
            report.setdefault('commands',[]).append(command);save()
            return subprocess.run(command,check=False).returncode
        assert validate('gcc')==0,'Native reserve differs from actual original'
        assert validate('clang','clang++')==0,'Clang reserve differs from actual original'
        target=project/'src/physics/native_tu15_array_reserve.cpp'
        text=target.read_text();needle='capacity*stride';assert text.count(needle)==1
        target.write_text(text.replace(needle,'(capacity+1u)*stride'))
        assert validate('negative-capacity')!=0,'Source failed to reject reserve capacity mutation'
        log=(args.output/'negative-capacity/tests.log').read_text()
        assert 'actual allocator request order differs'in log,'Mutation rejected for unrelated reason'
        target.write_text(text)
        needle='const auto header=arguments.header();'
        assert text.count(needle)==1
        mutated=text.replace(needle,'').replace('const auto stride=arguments.stride();',
            'const auto header=arguments.header();const auto stride=arguments.stride();')
        target.write_text(mutated)
        assert validate('negative-early-header')!=0,'Source failed to reject early header capture'
        log=(args.output/'negative-early-header/tests.log').read_text()
        assert any(reason in log for reason in ('array read order differs','array store differs',
            'complete array/pool bank differs')),'Early header mutation rejected for unrelated reason'
        changed=[p for p,h in pins.items()if reference.sha(ROOT/p)!=h]
        report.update(passed=not changed,changed_during_validation=changed,
            negative_control_rejected=True,early_header_control_rejected=True,live_integration_proven=False,
            remaining_explicit_owners=['default allocator149930','fixed/secondary allocation owners','oversize CRT free'])
    except BaseException as error:
        report['error']=str(error)
        raise
    finally:save()
    print(json.dumps({k:report[k]for k in ('passed','original_cases','fault_prefixes','negative_control_rejected')}))
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
