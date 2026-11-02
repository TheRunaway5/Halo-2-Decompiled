#!/usr/bin/env python3
"""Whole authored13DE70 kinds0..8 with independent real curve children."""
import argparse,os,shutil,subprocess,sys
from pathlib import Path
from build_native_tu15_constraint_before_reference import build_constraint_reference,sha,save
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],all_eleven_kinds_qualified=False)
 names=['include/halo2/native_tu15_function_evaluation_extended.hpp','src/core/native_tu15_function_evaluation_extended.cpp','include/halo2/native_tu15_function_curves.hpp','src/core/native_tu15_function_curves.cpp','include/halo2/native_tu15_function_control_point.hpp','src/core/native_tu15_function_control_point.cpp','tools/observe_native_function_evaluation_extended_tu15.cpp','tools/observe_native_activation_agents_tu15.cpp',str(Path(__file__).relative_to(ROOT))];pins={n:sha(ROOT/n)for n in names}
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
  class Declared:pass
  source=Declared();source.path=scratch/'declared';source.path.mkdir();source.regions=[];source.receipt=dict(declared_authored_function_inputs=True,original_sha256=NEW_PIN,captured_runtime_inputs=False,source_outcomes_supplied=False)
  banks=[(0x1000000,bytes(0x1000))]+[(s.va,image.section_bytes(s)+bytes(max(0,s.vsize-s.rsize)))for s in image.sections if s.name!='.text']
  for n,(address,raw)in enumerate(banks):name=f'input-{n}.bin';(source.path/name).write_bytes(raw);source.regions.append(dict(address=address,size=len(raw),before_file=name))
  reference=build_constraint_reference(source,scratch/'reference');reference['entry']='Declaredpublic13DE70 EDI8Breference/RET8twofloatbits; genuineoriginalcurve/CRTchildren';save(a.output/'reference.json',reference);save(scratch/'fixture.json',dict(regions=source.regions,parent_sites=sorted(complete_body(code,0x13de70).code)));metadata={}
  from capstone import CS_AC_READ,CS_AC_WRITE
  from capstone.x86 import X86_OP_MEM
  native_owners=(0x13de70,0x17f8a0,0x17f790,0x13e9a0,0x37cde8,0x37ce0c,0x332084,0x332099,0x3320c9,0x33216d,0x332122,0x37cfd1)
  for owner in native_owners:
   for pc in sorted(complete_body(code,owner).code):
    i=code.at(pc);operands=[]
    if i.mnemonic!='lea':
     for op in i.operands:
      if op.type!=X86_OP_MEM or op.mem.segment:continue
      operands.append(dict(base=i.reg_name(op.mem.base)or'',index=i.reg_name(op.mem.index)or'',scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(op.access&CS_AC_READ),write=bool(op.access&CS_AC_WRITE)))
    if pc==0x13dea4:operands=[]
    row=dict(size=i.size,memory=operands)
    if i.mnemonic=='call'and i.operands[0].type==2 and i.operands[0].imm not in native_owners:row.update(service_arguments=0,ecx_consumed=False,result_consumed=False)
    metadata[str(pc)]=row
  save(scratch/'metadata.json',metadata);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
  prefix=(ROOT/'tools/observe_native_activation_agents_tu15.cpp').read_text().split('int main(int argc,char**argv)')[0]
  coverage=prefix.replace('J activation_trace(','std::set<std::uint32_t> function_sites;\nJ activation_trace(').replace('const auto key=std::to_string(pc);','function_sites.insert(pc);const auto key=std::to_string(pc);')
  (project/'driver.cpp').write_text('#include <set>\n#include "halo2/native_tu15_function_evaluation_extended.hpp"\n'+coverage+(ROOT/'tools/observe_native_function_evaluation_extended_tu15.cpp').read_text())
  bounded=(ROOT/'tools/observe_native_function_evaluation_tu15.cpp').read_text().replace('NativeTu15FunctionEvaluationStats','NativeTu15FunctionEvaluationExtendedStats').replace('native_tu15_evaluate_authored_function(','native_tu15_evaluate_authored_function_extended(')
  (project/'bounded.cpp').write_text('#include "halo2/native_tu15_function_evaluation_extended.hpp"\n'+prefix+bounded)
  text=(ROOT/'src/core/native_tu15_function_evaluation_extended.cpp').read_text();(project/'native.cpp').write_text(text)
  needle='result=add(upper,lower);';assert text.count(needle)==3;(project/'wrong_blend.cpp').write_text(text.replace(needle,'result=sub(upper,lower);'))
  shutil.copyfile(ROOT/'src/core/native_tu15_function_curves.cpp',project/'curves.cpp');shutil.copyfile(ROOT/'src/core/native_tu15_function_control_point.cpp',project/'control.cpp')
  (project/'negative.py').write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True);j=json.load(open(sys.argv[-1]));assert r.returncode!=0 and not j.get("error")and any(not c["passed"]for c in j["cases"]),(r.returncode,j)\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(nativeExtendedFunction LANGUAGES CXX)\ninclude(CTest)\nadd_library(driver OBJECT driver.cpp)\ntarget_include_directories(driver PRIVATE "'+str(project/'include')+'")\ntarget_compile_features(driver PRIVATE cxx_std_20)\n'
  for variant in('native','wrong_blend'):
   cmake+=f'add_executable(function_{variant} $<TARGET_OBJECTS:driver> {variant}.cpp curves.cpp control.cpp)\ntarget_include_directories(function_{variant} PRIVATE "{project}/include")\ntarget_compile_features(function_{variant} PRIVATE cxx_std_20)\ntarget_compile_options(function_{variant} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n';negative=''if variant=='native'else f'"{sys.executable}" "{project}/negative.py" ';cmake+=f'add_test(NAME function_{variant} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {negative}$<TARGET_FILE:function_{variant}> "{reference["elf"]}" "{scratch}/fixture.json" "{scratch}/metadata.json" "${{CMAKE_BINARY_DIR}}/{variant}.json")\nset_tests_properties(function_{variant} PROPERTIES TIMEOUT 180)\n'
  cmake+=f'add_executable(function_bounded bounded.cpp native.cpp curves.cpp control.cpp)\ntarget_include_directories(function_bounded PRIVATE "{project}/include")\ntarget_compile_features(function_bounded PRIVATE cxx_std_20)\ntarget_compile_options(function_bounded PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\nadd_test(NAME function_bounded COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- $<TARGET_FILE:function_bounded> "{reference["elf"]}" "{scratch}/fixture.json" "{scratch}/metadata.json" "${{CMAKE_BINARY_DIR}}/bounded.json")\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','function_native','--target','function_wrong_blend','--target','function_bounded','--filter','^function_','--jobs','1','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--retain-report','native.json','--retain-report','wrong_blend.json','--retain-report','bounded.json'];r=subprocess.run(command,check=False);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  report.update(passed=all(r['exit_code']==0 for r in results)and all(sha(ROOT/f)==h for f,h in pins.items()),source_hashes=pins,compiler_results=results,reference=reference,scope='Whole13DE70 all11kinds finite-domain plus504boundededgeprofiles; genuinecurve/powchildren. SemanticDATAreads/result/fullDATA/FP/RET/live80. ExceptionalCRT/publication/overflow/underflow/domain/source-stackfault profiles remainexplicitunsupported.')
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
