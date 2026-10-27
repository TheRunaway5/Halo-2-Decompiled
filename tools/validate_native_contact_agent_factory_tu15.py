#!/usr/bin/env python3
"""Physical native contact factories with composed native allocator ownership."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from validate_native_contact_buffers_tu15 import sha,save

def metadata():
 from capstone import CS_AC_READ,CS_AC_WRITE
 from capstone.x86 import X86_OP_MEM
 code=_Code(Xbe(NEW_DEFAULT),'.text');rows={}
 for e in(0xa9520,0xab4b0,0x303c10,0x3040d0,0x2f3620,0x2f0380,0x37ed40,0x149da0,0x149e00,0x233250,0x233420):
  for p in sorted(complete_body(code,e).code):
   i=code.at(p);memory=[]
   if i.mnemonic!='lea':
    for op in i.operands:
     if op.type!=X86_OP_MEM or i.reg_name(op.mem.segment)=='fs':continue
     memory.append(dict(base=i.reg_name(op.mem.base)or'',index=i.reg_name(op.mem.index)or'',scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(op.access&CS_AC_READ),write=bool(op.access&CS_AC_WRITE)))
   rows[str(p)]=dict(size=i.size,bytes=i.bytes.hex(),memory=memory)
 return rows

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;source=ROOT/'src/physics/native_tu15_contact_agent_factory.cpp';header=ROOT/'include/halo2/native_tu15_contact_agent_factory.hpp';observer=ROOT/'tools/observe_native_contact_agent_factory_tu15.cpp';dependencies=[ROOT/'src/physics/native_tu15_array_allocator.cpp'];files=[source,header,observer,*dependencies,ROOT/'include/halo2/native_tu15_array_allocator.hpp',ROOT/'include/halo2/native_tu15_array_growth.hpp',ROOT/'src/physics/native_tu15_contact_unwind_internal.hpp',Path(__file__)];pins={str(p.relative_to(ROOT)):sha(p)for p in files}
  capture=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458/iteration-04-gemini-column0/original/required-failure-inputs';replay=scratch/'original';command=[sys.executable,str(ROOT/'tools/replay_live_phantom_hardware_tu15.py'),'--native-frame-semantics','--return-breakpoints','--capture',str(capture),'--output',str(replay)];r=subprocess.run(command,capture_output=True,text=True,timeout=180);assert(replay/'original-hardware').exists(),r.stderr[-2000:];report.update(ordinary_reference_command=command,ordinary_reference_raw_compare_exit=r.returncode)
  rows=[dict(name='getter23',kind='getter',owner=0xa9520),dict(name='getter24',kind='getter',owner=0xab4b0),dict(name='base_constructor',kind='constructor')]
  for offset in(4,8,12):rows.append(dict(name=f'base_constructor_actual_alignment_{offset}',kind='constructor',object_offset=offset))
  for offset,site in((8,0x2f362e),(0,0x2f3631),(0x14,0x2f364d),(0x30,0x2f3658),(0x20,0x2f365c)):rows.append(dict(name=f'constructor_prewrite_{offset:x}',kind='constructor',exception=True,stop_before=site,reject_write_offset=offset))
  for site,stop in((0x2f3631,0x2f3647),(0x2f3658,0x2f365c),(0x2f365c,0x2f3660)):rows.append(dict(name=f'constructor_observer_{site:x}',kind='constructor',exception=True,observer_after=site,stop_before=stop))
  for owner in(0x303c10,0x3040d0):
   second=owner==0x3040d0;rows.append(dict(name=f'factory_{owner:x}',kind='factory',owner=owner));rows.append(dict(name=f'factory_release_{owner:x}',kind='release',owner=owner))
   for value in(0,0xffffffff,0x80000000,0x7f800001,0x7fc12345):rows.append(dict(name=f'factory_userdata_{owner:x}_{value:x}',kind='factory',owner=owner,user_data=value))
   phases=[('allocate_before',0x149da0,False),('allocate_after',0x3040f6 if second else 0x303c36,False),('user_data',0x304102 if second else 0x303c42,False)]
   for injection,stop,action in phases:rows.append(dict(name=f'factory_{owner:x}_{injection}',kind='factory',owner=owner,exception=True,injection=injection,stop_before=stop,source_action=action))
   rows.append(dict(name=f'factory_{owner:x}_size_prewrite',kind='factory',owner=owner,exception=True,reject_write_offset=4,stop_before=0x3040f8 if second else 0x303c38))
   rows.append(dict(name=f'factory_{owner:x}_size_observer',kind='factory',owner=owner,exception=True,observer_after=0x3040f8 if second else 0x303c38,stop_before=0x3040fe if second else 0x303c3e))
   for offset,site in((8,0x2f362e),(0x30,0x2f3658)):rows.append(dict(name=f'factory_{owner:x}_armed_prewrite_{offset:x}',kind='factory',owner=owner,exception=True,reject_write_offset=offset,stop_before=site,source_action=True))
   for site,stop in((0x2f3631,0x2f3647),(0x2f365c,0x2f3660)):rows.append(dict(name=f'factory_{owner:x}_armed_observer_{site:x}',kind='factory',owner=owner,exception=True,observer_after=site,stop_before=stop,source_action=True))
   rows.append(dict(name=f'factory_{owner:x}_derived_prewrite',kind='factory',owner=owner,exception=True,reject_write_offset=0,write_occurrence=1,stop_before=0x30411a if second else 0x303c5a,source_action=True))
   rows.append(dict(name=f'factory_{owner:x}_derived_observer',kind='factory',owner=owner,exception=True,observer_after=0x30411a if second else 0x303c5a,stop_before=0x304120 if second else 0x303c60,source_action=True))
  for rounding in range(4):
   for denormal in(0,0x40,0x8000,0x8040):rows.append(dict(name=f'constructor_controls_{rounding}_{denormal:x}',kind='constructor',control_word=0x27f|(rounding<<10),status=0x400,mxcsr=0x1f80|(rounding<<13)|denormal))
  fixture=scratch/'cases.json';save(fixture,dict(cases=rows));instructions=scratch/'instructions.json';save(instructions,metadata())
  project=scratch/'project';(project/'src').mkdir(parents=True);shutil.copytree(ROOT/'include',project/'include');shutil.copyfile(source,project/'src/native.cpp');shutil.copyfile(observer,project/'src/observer.cpp');shutil.copyfile(dependencies[0],project/'src/allocator.cpp');shutil.copyfile(ROOT/'src/physics/native_tu15_contact_unwind_internal.hpp',project/'src/native_tu15_contact_unwind_internal.hpp')
  text=source.read_text();variants={}
  for name,needle,replacement,count in(
   ('wrong_class','noexcept{return 23;}','noexcept{return 22;}',1),
   ('wrong_maximum','std::byte{0xff}','std::byte{0xfe}',8),
   ('wrong_derived_vtable','0x41d01cu','0x41d03cu',1),
   ('early_cleanup_arm','const auto user_data=arguments.user_data();cleanup.arm();','cleanup.arm();const auto user_data=arguments.user_data();',1),
   ('wrong_ctor_order','m.word(0x2f362eu,p+8u,user_data);m.word(0x2f3631u,p,0x41cb58u);','m.word(0x2f3631u,p,0x41cb58u);m.word(0x2f362eu,p+8u,user_data);',1)):
   assert text.count(needle)==count,(name,text.count(needle));variants[name]=text.replace(needle,replacement,1);(project/'src'/f'{name}.cpp').write_text(variants[name])
  (project/'negative.py').write_text('import json,subprocess,sys,os\nenv=dict(os.environ,HALO2_CONTACT_AGENT_FACTORY_NEGATIVE_CONTROL="1");r=subprocess.run(sys.argv[1:],env=env,capture_output=True,text=True);j=json.load(open(sys.argv[-1]));assert r.returncode!=0 and not j.get("error") and any(not c["native_semantics_passed"]for c in j.get("cases",[])),(r.returncode,j.get("error"))\nprint("Actual original factory rejects mutation")\n')
  cmake=f'''cmake_minimum_required(VERSION 3.20)
project(NativeAgentFactory LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
add_library(driver OBJECT src/observer.cpp)
add_library(dependencies OBJECT src/allocator.cpp)
foreach(target driver dependencies)
 target_include_directories(${{target}} PRIVATE include)
 target_compile_features(${{target}} PRIVATE cxx_std_20)
 target_compile_options(${{target}} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
endforeach()
target_link_libraries(driver PRIVATE nlohmann_json::nlohmann_json)
file(MAKE_DIRECTORY "${{CMAKE_BINARY_DIR}}/evidence")
''';retained=[]
  for variant in('native',*variants):
   cmake+=f'add_executable(factory_{variant} $<TARGET_OBJECTS:driver> $<TARGET_OBJECTS:dependencies> src/{variant}.cpp)\ntarget_include_directories(factory_{variant} PRIVATE include)\ntarget_compile_features(factory_{variant} PRIVATE cxx_std_20)\n';path='evidence/'+variant+'.json';retained.append(path);command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '+(''if variant=='native'else f'"{sys.executable}" "{project}/negative.py" ')+f'$<TARGET_FILE:factory_{variant}> "{replay}/original-hardware" "{capture}/capture.json" "{fixture}" "{instructions}" "${{CMAKE_BINARY_DIR}}/{path}"';cmake+=f'add_test(NAME factory_{variant} COMMAND {command})\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in a.compilers:
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','factory_native',*sum((['--target','factory_'+name]for name in variants),[]),'--filter','^factory_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1',*sum((['--retain-report',path]for path in(*retained,'Testing/Temporary/LastTest.log')),[])];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  unchanged=all(sha(ROOT/name)==pin for name,pin in pins.items());assert unchanged;report.update(passed=all(r['exit_code']==0 for r in results),compiler_results=results,physical_cases_per_compiler=len(rows),negative_controls=list(variants),source_hashes=pins,current_sources_unchanged=unchanged,image_sha256=NEW_PIN,native_composed_owners=['0A9520','0AB4B0','303C10','3040D0','2F3620','2F0380','149DA0','149E00','233250','233420'],supplied_original_outcomes=False,instruction_patches=[],production_native_only_closure=False)
 except BaseException as error:report['error']=str(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
