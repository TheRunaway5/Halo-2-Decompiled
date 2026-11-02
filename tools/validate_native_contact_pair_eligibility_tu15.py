#!/usr/bin/env python3
"""Physical native2E4F30/3038C0 dispatch eligibility, including reread faults."""
import argparse,copy,json,os,shutil,subprocess,sys
from pathlib import Path
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from build_native_tu15_constraint_before_reference import sha,save

DISPATCH=base.UI+0x1000
RESULT=base.UI+0x1200
FILES=('include/halo2/native_tu15_contact_pair_eligibility.hpp',
       'src/physics/native_tu15_contact_pair_eligibility.cpp',
       'tools/validate_native_contact_pair_eligibility_tu15.py',
       'tools/validate_native_array_growth_tu15.py',
       'tools/observe_native_array_growth_tu15.cpp','tests/native_tu15_array_growth_tests.cpp')

def cases():
    rows=[]
    classes=((0,0),(1,2),(23,8),(8,23),(31,31),(32,0),(0xffffffff,0),
             (0,0xffffffff),(0x80000000,8),(0x40000001,0x80000000),
             (0x12345678,0x98765432),(0xffffffff,0xffffffff))
    for entry in (0x2e4f30,0x3038c0):
        for first,second in classes:
            context=(DISPATCH-(((first*32+second)*4+0x18c)&0xffffffff))&0xffffffff
            for target in (0,0x2e4eb0,0x2f83b0,0xffffffff):
                rows.append(dict(name=f'pair_{entry:x}_{first:x}_{second:x}_{target:x}',
                    entry=entry,receiver=0,arguments=[RESULT,context,first,second],
                    user_words=[(DISPATCH-base.UI,target)]))
    seed=next(r for r in rows if r['name']=='pair_2e4f30_17_8_2f83b0')
    for entry in (0x2e4f30,0x3038c0):
        for target in (0x2e4eb0,0x2f83b0,0):
            for offset in range(4):
                row=copy.deepcopy(seed);row.update(name=f'alias_{entry:x}_{target:x}_{offset}',entry=entry,
                    arguments=[DISPATCH+offset,*seed['arguments'][1:]],user_words=[(DISPATCH-base.UI,target)]);rows.append(row)
    for entry,target,occurrence,writing in ((0x2e4f30,0x2e4eb0,1,False),
        (0x2e4f30,0x2f83b0,1,False),(0x2e4f30,0x2f83b0,2,False),
        (0x3038c0,0x2f83b0,1,False),(0x3038c0,0,1,False),
        (0x2e4f30,0x2e4eb0,1,True),(0x2e4f30,0x2f83b0,1,True),
        (0x2e4f30,0,1,True),(0x3038c0,0x2f83b0,1,True),(0x3038c0,0,1,True)):
        row=copy.deepcopy(seed);row.update(name=f'fault_{entry:x}_{target:x}_{occurrence}_{writing}',
            entry=entry,user_words=[(DISPATCH-base.UI,target)],
            fault=dict(address=RESULT if writing else DISPATCH,bytes=1 if writing else 4,
                occurrence=occurrence,write=writing));rows.append(row)
    return rows

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();pins={name:sha(ROOT/name)for name in FILES}
    report=dict(passed=False,owned_scratch=str(scratch),source_hashes=pins,retained_large_artifacts=[],
        source_outcomes_supplied=False,actual_live_input_claimed=False)
    try:
        assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);base.OWNERS=(0x2e4f30,0x3038c0)
        rows=cases();fixture,authority=base.original(image,{},rows,scratch)
        assert all(r['declared_fault']for r in rows if 'fault'in r)
        save(fixture/'original.json',dict(image_sha256=NEW_PIN,
            oracle='unchanged-tu15-physical-array-and-real-pool-v1',cases=rows))
        project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
        shutil.copyfile(ROOT/FILES[1],project/'native.cpp')
        driver='#include "halo2/native_tu15_contact_pair_eligibility.hpp"\n'+(ROOT/FILES[-1]).read_text()
        begin=driver.index('      NativeTu15ArrayGrowthMemory array');end=driver.index('      const auto host_sw=',begin)
        driver=driver[:begin]+'''      NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};
'''+driver[end:]
        begin=driver.index('      try{const auto entry=');end=driver.index('      }catch(const Fault&)',begin)
        driver=driver[:begin]+'''      try{const auto args=c.at("arguments").get<std::vector<std::uint32_t>>();
        if(c.at("entry")==0x2e4f30u)result=native_tu15_test_contact_pair(memory,args[0],args[1],args[2],args[3],observe);
        else result=native_tu15_test_gsk_contact_pair(memory,args[0],args[1],args[2],args[3],observe);
'''+driver[end:]
        driver=driver.replace('NativeTu15ArrayGrowthActivity activity;','')
        marker='      check(calls==c.at("allocator_calls"),'
        assert driver.count(marker)==1
        driver=driver.replace(marker,'      if(!failed&&!required)check(result==c.at("return_word"),name+" result DATA identity differs");\n'+marker)
        (project/'driver.cpp').write_text(driver)
        (project/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.20)\nproject(contactPairEligibility LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_executable(pair driver.cpp native.cpp)\ntarget_include_directories(pair PRIVATE include)\ntarget_compile_features(pair PRIVATE cxx_std_20)\ntarget_link_libraries(pair PRIVATE nlohmann_json::nlohmann_json)\nadd_test(NAME pair_eligibility COMMAND pair "'+str(fixture/'original.json')+'")\n')
        results=[]
        def run(label,compiler):
            command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),
                '--scratch-root',str(scratch/'builds'),'--output',str(args.output/label),
                '--target','pair','--filter','^pair_eligibility$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,
                '--jobs','1','--test-jobs','1']
            result=subprocess.run(command);results.append(dict(label=label,command=command,exit_code=result.returncode))
            return result.returncode
        for compiler in ('g++','clang++'):assert run(compiler,compiler)==0,'Native pair eligibility differs'
        target=project/'native.cpp';text=target.read_text()
        mutants=(('wrong_convex','==0x2e4eb0u','==0x2e4eb1u'),
            ('wrong_gsk','==0x2f83b0u','==0x2f83b1u'),
            ('cached_fallback','return native_tu15_test_gsk_contact_pair(memory,result,dispatcher,first,second,observe);',
             'const bool eligible=initial_owner==0x2f83b0u;publish(memory,eligible?0x3038e2u:0x3038e6u,result,eligible,observe);return result;'))
        for name,before,after in mutants:
            assert text.count(before)==1;mutated=text.replace(before,after)
            if name=='cached_fallback':
                before='if(owner(memory,dispatcher,first,second)==0x2e4eb0u)'
                assert mutated.count(before)==1
                mutated=mutated.replace(before,'const auto initial_owner=owner(memory,dispatcher,first,second);if(initial_owner==0x2e4eb0u)')
            target.write_text(mutated)
            assert run('negative-'+name,'g++')!=0,'Original failed to reject '+name;target.write_text(text)
        executed={pc for row in rows for pc in row['executed_sites']};code=_Code(image,'.text')
        assert all(sha(ROOT/name)==value for name,value in pins.items()),'Pair kernel inputs changed'
        report.update(passed=True,source_inputs_unchanged=True,original_sha256=NEW_PIN,authority=authority,
            profiles=len(rows),full_returns=sum(not row['declared_fault']for row in rows),
            fault_prefixes=sum(row['declared_fault']for row in rows),alias_cases=sum(row['name'].startswith('alias_')for row in rows),
            ordered_reads=sum(len(row['reads'])for row in rows),ordered_stores=sum(len(row['writes'])for row in rows),
            body_coverage={hex(owner):dict(total=len(base.complete_body(code,owner).code),
                executed=len(set(base.complete_body(code,owner).code)&executed))for owner in base.OWNERS},
            results=results,negative_controls=[row[0]for row in mutants],
            scope='Native2E4F30→3038C0 complete DATA/state/ordered reads/stores/FP/result identity, class-index wrap, output alias and both dispatch-read fault phases. Production producer/World/live admission remain separate.')
    except BaseException as error:report['error']=str(error)or repr(error);raise
    finally:
        report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file())
        save(args.output/'report.json',report)
    return 0 if report['passed']else 1

if __name__=='__main__':raise SystemExit(main())
