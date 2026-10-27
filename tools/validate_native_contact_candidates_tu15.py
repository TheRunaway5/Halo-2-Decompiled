#!/usr/bin/env python3
"""Original2F36F0 composition with true mutable-bank service execution.

Force only the declared offline cache/order inputs. Both sides enter from
actual2F36F0 caller packets; private work products remain disposable.
"""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
import re
from oracle_smg_firing_1_1 import ROOT
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from validate_native_contact_agent_dispatch_tu15 import sha,save
def instruction_metadata():
 from capstone import CS_AC_READ,CS_AC_WRITE
 from capstone.x86 import X86_OP_MEM
 from oracle_smg_firing_1_1 import Xbe,_Code
 from generate_physics_live_support_tu15 import complete_body
 code=_Code(Xbe(NEW_DEFAULT),'.text');result={}
 services={0x2f3766:(3,True),0x2f381e:(2,True),0x2f38b8:(0,True),0x2f39bc:(3,False),0x2f3a44:(2,True),0x2f3a72:(6,True),0x2f3a8a:(4,True),0x2f3af9:(4,False),0x2f3b32:(1,True),0x2f3b65:(3,False),0x2f3bd2:(2,True),0x2f3c03:(6,True),0x2f3c1c:(4,True),0x2f3c50:(0,True),0x2f3c73:(0,True),0x2f3c98:(2,True),0x2f3cc2:(6,True),0x2f3cda:(4,True),0x2f3d2b:(3,True),0x2f3d43:(2,True),0x2f3d89:(0,True),0x2f3dbf:(3,True),0x2f3153:(0,True),0x2f3160:(0,True),0x2f317b:(4,True),0x2e31fc:(2,True),0x2e3235:(3,True),0x2f3481:(1,True),0x2f3520:(2,True),0x2f355d:(3,True)}
 for address in(0x2f3a8a,0x2f3c1c,0x2f3cda,0x2f39bc,0x2f3b65,0x2f3af9,0x2f3b32,0x2f3d89):del services[address]
 for entry in(0x2f36f0,0x2e4550,0x30bad0,0x2f3140,0x2e31e0,0x2e2e30,0x2f33b0,0x2f3430,0x2f34d0):
  fn=complete_body(code,entry)
  for address in sorted(fn.code):
   i=code.at(address);memory=[]
   if i.mnemonic!='lea':
    for op in i.operands:
     if op.type!=X86_OP_MEM or i.reg_name(op.mem.segment)=='fs':continue
     access=CS_AC_WRITE if i.mnemonic.startswith('fstp')else op.access
     memory.append(dict(base=i.reg_name(op.mem.base)or'',index=i.reg_name(op.mem.index)or'',scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
   if i.mnemonic.startswith('rep movs'):memory.sort(key=lambda x:not x['read'])
   row=dict(bytes=i.bytes.hex(),size=i.size,memory=memory,repeat_copy=i.mnemonic.startswith('rep movs'))
   if address in services:row.update(service_arguments=services[address][0],ecx_consumed=services[address][1],result_consumed=address in(0x2f3a44,0x2f3a72,0x2f3a8a,0x2f3bd2,0x2f3c03,0x2f3c1c,0x2f3c98,0x2f3cc2,0x2f3cda,0x2f3d43,0x2f3153,0x2f3160,0x2f317b,0x2e31fc,0x2f3481))
   result[str(address)]=row
 return result

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--case',default='.*');p.add_argument('--compilers',nargs='+',default=['g++','clang++']);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 source=ROOT/'src/physics/native_tu15_contact_candidates.cpp';header=ROOT/'include/halo2/native_tu15_contact_candidates.hpp';bridge=ROOT/'tools/observe_native_contact_producer_tu15.cpp';inverse=ROOT/'src/physics/native_tu15_inverse_geometry.cpp';observer=ROOT/'tools/observe_native_contact_candidates_tu15.cpp';files=[source,header,bridge,inverse,observer,ROOT/'src/physics/native_tu15_contact_candidate_buffers.cpp',ROOT/'include/halo2/native_tu15_contact_candidate_buffers.hpp',ROOT/'include/halo2/native_tu15_contact_buffers.hpp',ROOT/'src/physics/native_tu15_contact_agent_dispatch.cpp',ROOT/'include/halo2/native_tu15_contact_agent_dispatch.hpp',ROOT/'src/physics/native_tu15_array_reserve.cpp',ROOT/'include/halo2/native_tu15_array_reserve.hpp',ROOT/'src/physics/native_tu15_array_growth.cpp',ROOT/'include/halo2/native_tu15_array_growth.hpp',ROOT/'include/halo2/native_tu15_inverse_geometry.hpp',ROOT/'src/physics/native_tu15_contact_unwind_internal.hpp',Path(__file__)];pins={str(f.relative_to(ROOT)):sha(f)for f in files};save(a.output/'report.json',report)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;prefix=bridge.read_text().split('int main(int argc,char**argv)')[0];assert prefix.count('{\"after_esp\",std::uint32_t(r.rsp)}')==1;prefix=prefix.replace('{\"after_esp\",std::uint32_t(r.rsp)}','{\"before_esp\",sp},{\"after_esp\",std::uint32_t(r.rsp)}');driver=scratch/'candidates.cpp';driver.write_text('#include "halo2/native_tu15_contact_candidates.hpp"\n#include "halo2/native_tu15_array_reserve.hpp"\n#include "halo2/native_tu15_contact_candidate_buffers.hpp"\n'+prefix+observer.read_text());metadata=scratch/'instructions.json';save(metadata,instruction_metadata())
  base=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458';cases=[];contexts=[]
  for name,count in(('gemini-column0',2),('gemini-column1',2),('elongation-crates',3)):
   if not any(re.fullmatch(a.case,f'{name}_{ordinal}_{order}')for ordinal in range(count)for order in range(6)):continue
   capture=base/('iteration-04-'+name)/'original/required-failure-inputs';replay=scratch/('original-'+name);command=[sys.executable,str(ROOT/'tools/replay_live_phantom_hardware_tu15.py'),'--native-frame-semantics','--return-breakpoints','--capture',str(capture),'--output',str(replay)];r=subprocess.run(command,text=True,capture_output=True,timeout=180);assert(replay/'original-hardware').exists(),r.stderr[-2000:];contexts.append(dict(name=name,capture=str(capture),capture_sha256=sha(capture/'capture.json'),reference_command=command,reference_exit_code=r.returncode))
   for ordinal in range(count):
    for order in(0,1,2,3,4,5):
     if not re.fullmatch(a.case,f'{name}_{ordinal}_{order}'):continue
     cases.append(dict(name=f'{name}_{ordinal}_{order}',elf=str(replay/'original-hardware'),capture=str(capture/'capture.json'),ordinal=ordinal,order=order))
  project=scratch/'project';project.mkdir();shutil.copyfile(source,project/'native.cpp');shutil.copyfile(inverse,project/'inverse.cpp');
  for name in('contact_agent_dispatch','array_reserve','array_growth','contact_candidate_buffers'):shutil.copyfile(ROOT/f'src/physics/native_tu15_{name}.cpp',project/(name+'.cpp'))
  shutil.copyfile(ROOT/'src/physics/native_tu15_contact_unwind_internal.hpp',project/'native_tu15_contact_unwind_internal.hpp')
  variants={};text=source.read_text()
  for name,needle,replacement in(
   ('wrong_relative_transform','second_motion+0x20u,first_motion+0x20u','first_motion+0x20u,second_motion+0x20u'),
   ('wrong_filter_key','parent_collection,current_key,branch,fp','parent_collection,current_key^1u,branch,fp'),
   ('clear_opaque_entry_word','m.word(accepted?0x2f3c1fu:0x2f3c36u,destination+8u,agent.identity);','m.word(0x2f3c1fu,destination+4u,0);m.word(accepted?0x2f3c1fu:0x2f3c36u,destination+8u,agent.identity);')):
   assert text.count(needle)==1,needle;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
  (project/'negative.py').write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nj=json.load(open(sys.argv[-1]));assert r.returncode!=0 and not j.get("error") and (j.get("different_bytes",0)>0 or not j.get("ordered_retained_operands_equal",True) or j.get("service_context_differences") or not j.get("fp_equal",True)),(r.returncode,j)\nprint("Actual original candidate rejects native mutation")\n')
  cmake=f'''cmake_minimum_required(VERSION 3.20)
project(NativeContactCandidates LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
add_library(driver OBJECT "{driver}")
add_library(dependencies OBJECT inverse.cpp contact_agent_dispatch.cpp array_reserve.cpp array_growth.cpp contact_candidate_buffers.cpp)
foreach(target driver dependencies)
 target_include_directories(${{target}} PRIVATE "{ROOT}/include")
 target_compile_features(${{target}} PRIVATE cxx_std_20)
 target_compile_options(${{target}} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
endforeach()
target_link_libraries(driver PRIVATE nlohmann_json::nlohmann_json)
file(MAKE_DIRECTORY "${{CMAKE_BINARY_DIR}}/evidence")
''';retain=[];negative_case=next((c for c in cases if c['order']==4),None)
  if negative_case is None:variants={}
  for variant in('native',*variants):
   cmake+=f'add_executable(candidates_{variant} $<TARGET_OBJECTS:driver> $<TARGET_OBJECTS:dependencies> {variant}.cpp)\ntarget_include_directories(candidates_{variant} PRIVATE "{ROOT}/include")\ntarget_compile_features(candidates_{variant} PRIVATE cxx_std_20)\ntarget_compile_options(candidates_{variant} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
   for case in cases if variant=='native'else[negative_case]:
    name=variant+'_'+case['name'];path='evidence/'+name+'.json';retain.append(path)
    command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '+(''if variant=='native'else f'"{sys.executable}" "{project}/negative.py" ')+f'$<TARGET_FILE:candidates_{variant}> "{case["elf"]}" "{case["capture"]}" {case["ordinal"]} {case["order"]} "{metadata}" "${{CMAKE_BINARY_DIR}}/{path}"'
    cmake+=f'add_test(NAME candidates_{name} COMMAND {command})\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in a.compilers:
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','candidates_native',*sum((['--target','candidates_'+name]for name in variants),[]),'--filter','^candidates_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1',*sum((['--retain-report',path]for path in retain),[])];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  report.update(passed=all(r['exit_code']==0 for r in results),contexts=contexts,compiler_results=results,physical_cases_per_compiler=len(cases),negative_controls=list(variants),native_composed_owners=['2F36F0','2E4550','2F3140','2E31E0','2E2E30','2F33B0','2F3430','2F34D0'],declared_entry_variations=["unchanged record headers","borrowed empty entries with zero capacity","borrowed one-entry array with retired key and capacity one"],source_hashes=pins,current_sources_unchanged=all(sha(ROOT/f)==v for f,v in pins.items()),image_sha256=NEW_PIN,before_inputs_only=True,supplied_original_outcomes=False,instruction_patches=[],production_native_only_closure=False,remaining=['fault/alias/negative controls beyond current physical call cohort','native remaining services','fault/alias/negative controls','actual visible multiplayer use','unmasked faults and Xbox numerical identity'])
 except BaseException as error:report['error']=str(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
