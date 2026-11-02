#!/usr/bin/env python3
"""Source-owned bounded13DE70 native function domain, no CRT/CPU fallback."""
import argparse,os,shutil,subprocess,sys
from pathlib import Path
from build_native_tu15_constraint_before_reference import build_constraint_reference,sha,save
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],all_eleven_kinds_qualified=False)
 files=[ROOT/'include/halo2/native_tu15_function_evaluation.hpp',ROOT/'src/core/native_tu15_function_evaluation.cpp',ROOT/'tools/observe_native_function_evaluation_tu15.cpp',ROOT/'tools/observe_native_activation_agents_tu15.cpp',Path(__file__)];pins={str(p.relative_to(ROOT)):sha(p)for p in files}
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
  class Declared:pass
  source=Declared();source.path=scratch/'declared';source.path.mkdir();source.regions=[];source.receipt=dict(declared_authored_function_inputs=True,original_sha256=NEW_PIN,captured_runtime_inputs=False,source_outcomes_supplied=False)
  for n,(address,raw)in enumerate([(0x1000000,bytes(0x1000)),(0x46a0d0,image.read(0x46a0d0,4))]):name=f'input-{n}.bin';(source.path/name).write_bytes(raw);source.regions.append(dict(address=address,size=len(raw),before_file=name))
  reference=build_constraint_reference(source,scratch/'reference');reference['entry']='Declaredpublic13DE70 EDI8Bfunctionreference/stack2floatbits, real unchangedoriginalinstructions';reference['code_authentication']['scope']='PinnedTU1.5text; explicitauthoredfunctionDATAcontrols, notcapturedoutcomes';save(a.output/'reference.json',reference);save(scratch/'fixture.json',dict(regions=source.regions));metadata={}
  from capstone import CS_AC_READ,CS_AC_WRITE
  from capstone.x86 import X86_OP_MEM
  for pc in sorted(complete_body(code,0x13de70).code):
   i=code.at(pc);operands=[]
   if i.mnemonic!='lea':
    for op in i.operands:
     if op.type!=X86_OP_MEM or op.mem.segment:continue
     operands.append(dict(base=i.reg_name(op.mem.base)or'',index=i.reg_name(op.mem.index)or'',scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(op.access&CS_AC_READ),write=bool(op.access&CS_AC_WRITE)))
   # Native authored-kind dispatch is ordinary C++ control flow. Original
   # code-address jump-table fetches are architecture, not function DATA.
   if pc==0x13dea4:operands=[]
   metadata[str(pc)]=dict(size=i.size,memory=operands)
  save(scratch/'metadata.json',metadata);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');prefix=(ROOT/'tools/observe_native_activation_agents_tu15.cpp').read_text().split('int main(int argc,char**argv)')[0];(project/'driver.cpp').write_text('#include "halo2/native_tu15_function_evaluation.hpp"\n'+prefix+(ROOT/'tools/observe_native_function_evaluation_tu15.cpp').read_text());text=(ROOT/'src/core/native_tu15_function_evaluation.cpp').read_text();(project/'native.cpp').write_text(text);needle='if(greater(_mm_setzero_ps(),value))';assert text.count(needle)==1;(project/'wrong_lower.cpp').write_text(text.replace(needle,'if(greater(value,_mm_setzero_ps()))'));(project/'negative.py').write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True);j=json.load(open(sys.argv[-1]));assert r.returncode!=0 and not j.get("error")and any(not c["passed"]for c in j["cases"]),(r.returncode,j)\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(nativeFunction LANGUAGES CXX)\ninclude(CTest)\nadd_library(driver OBJECT driver.cpp)\ntarget_include_directories(driver PRIVATE "'+str(project/'include')+'")\ntarget_compile_features(driver PRIVATE cxx_std_20)\n'
  for variant in('native','wrong_lower'):
   cmake+=f'add_executable(function_{variant} $<TARGET_OBJECTS:driver> {variant}.cpp)\ntarget_include_directories(function_{variant} PRIVATE "{project}/include")\ntarget_compile_features(function_{variant} PRIVATE cxx_std_20)\ntarget_compile_options(function_{variant} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n';negative=''if variant=='native'else f'"{sys.executable}" "{project}/negative.py" ';cmake+=f'add_test(NAME function_{variant} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {negative}$<TARGET_FILE:function_{variant}> "{reference["elf"]}" "{scratch}/fixture.json" "{scratch}/metadata.json" "${{CMAKE_BINARY_DIR}}/{variant}.json")\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','function_native','--target','function_wrong_lower','--filter','^function_','--jobs','1','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--retain-report','native.json','--retain-report','wrong_lower.json'];r=subprocess.run(command,check=False);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  report.update(passed=all(r['exit_code']==0 for r in results)and all(sha(ROOT/f)==h for f,h in pins.items()),source_hashes=pins,compiler_results=results,reference=reference,architecture_only_dispatch_fetch_excluded=dict(instruction=0x13dea4,scope='Original code-address jump table; native authored-kind branch needs no original PC/address table'),scope='Boundednative13DE70 kinds0/1/null/empty/invalidcommonclamp; othervalidkinds remainexplicitunsupported. SemanticDATAreads exact, codeaddress dispatchfetch excluded. No CPP/GP/privateframe emulator or sourceoutcomes supplied.')
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
