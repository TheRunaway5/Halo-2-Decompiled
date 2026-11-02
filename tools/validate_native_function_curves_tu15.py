#!/usr/bin/env python3
"""Native authoredcurve helpers with physical original CRT authority."""
import argparse,os,shutil,subprocess,sys
from pathlib import Path
from build_native_tu15_constraint_before_reference import build_constraint_reference,sha,save
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],nonfinite_CRT_qualified=False)
 files=[ROOT/'include/halo2/native_tu15_function_curves.hpp',ROOT/'src/core/native_tu15_function_curves.cpp',ROOT/'tools/observe_native_function_curves_tu15.cpp',ROOT/'tools/observe_native_activation_agents_tu15.cpp',Path(__file__)];pins={str(p.relative_to(ROOT)):sha(p)for p in files}
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
  class Declared:pass
  source=Declared();source.path=scratch/'declared';source.path.mkdir();source.regions=[];source.receipt=dict(declared_authored_curve_inputs=True,original_sha256=NEW_PIN,captured_runtime_inputs=False,source_outcomes_supplied=False)
  for n,section in enumerate(s for s in image.sections if s.name!='.text'):
   raw=image.section_bytes(section);raw+=bytes(max(0,section.vsize-len(raw)));name=f'input-{n}.bin';(source.path/name).write_bytes(raw);source.regions.append(dict(address=section.va,size=len(raw),before_file=name,source_virtual_size=section.vsize,source_raw_size=section.rsize,zero_fill_declared=True))
  reference=build_constraint_reference(source,scratch/'reference');reference['entry']='Declaredpublic17F8A0 AXtype/XMM0bits or17F790 AXtype/stackfloat, originalreadonlytables/defaultmathdata';reference['code_authentication']['scope']='PinnedTU1.5text plus declaredoriginalasset/table/defaultmathDATA, no capturedoutcome';save(a.output/'reference.json',reference);save(scratch/'fixture.json',dict(regions=source.regions));metadata={}
  from capstone import CS_AC_READ,CS_AC_WRITE
  from capstone.x86 import X86_OP_MEM
  for owner in(0x17f8a0,0x17f790):
   for pc in sorted(complete_body(code,owner).code):
    i=code.at(pc);operands=[]
    if i.mnemonic!='lea':
     for op in i.operands:
      if op.type!=X86_OP_MEM or op.mem.segment:continue
      operands.append(dict(base=i.reg_name(op.mem.base)or'',index=i.reg_name(op.mem.index)or'',scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(op.access&CS_AC_READ),write=bool(op.access&CS_AC_WRITE)))
    row=dict(size=i.size,memory=operands)
    if i.mnemonic=='call':row.update(service_arguments=0,ecx_consumed=False,result_consumed=False)
    metadata[str(pc)]=row
  save(scratch/'metadata.json',metadata);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');prefix=(ROOT/'tools/observe_native_activation_agents_tu15.cpp').read_text().split('int main(int argc,char**argv)')[0];(project/'driver.cpp').write_text('#include "halo2/native_tu15_function_curves.hpp"\n'+prefix+(ROOT/'tools/observe_native_function_curves_tu15.cpp').read_text());text=(ROOT/'src/core/native_tu15_function_curves.cpp').read_text();(project/'native.cpp').write_text(text);needle='0x4782a8u+std::uint32_t(type)*1024u';assert text.count(needle)==1;(project/'wrong_table.cpp').write_text(text.replace(needle,'0x4782a8u+std::uint32_t(type)*1024u+1u'));(project/'negative.py').write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True);j=json.load(open(sys.argv[-1]));assert r.returncode!=0 and not j.get("error")and any(not c["passed"]for c in j["cases"]),(r.returncode,j)\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(nativeCurves LANGUAGES CXX)\ninclude(CTest)\nadd_library(driver OBJECT driver.cpp)\ntarget_include_directories(driver PRIVATE "'+str(project/'include')+'")\ntarget_compile_features(driver PRIVATE cxx_std_20)\n'
  for variant in('native','wrong_table'):
   cmake+=f'add_executable(curve_{variant} $<TARGET_OBJECTS:driver> {variant}.cpp)\ntarget_include_directories(curve_{variant} PRIVATE "{project}/include")\ntarget_compile_features(curve_{variant} PRIVATE cxx_std_20)\ntarget_compile_options(curve_{variant} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n';negative=''if variant=='native'else f'"{sys.executable}" "{project}/negative.py" ';cmake+=f'add_test(NAME curve_{variant} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {negative}$<TARGET_FILE:curve_{variant}> "{reference["elf"]}" "{scratch}/fixture.json" "{scratch}/metadata.json" "${{CMAKE_BINARY_DIR}}/{variant}.json")\nset_tests_properties(curve_{variant} PROPERTIES TIMEOUT 180)\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','curve_native','--target','curve_wrong_table','--filter','^curve_','--jobs','1','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--retain-report','native.json','--retain-report','wrong_table.json'];r=subprocess.run(command,check=False);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  report.update(passed=all(r['exit_code']==0 for r in results)and all(sha(ROOT/f)==h for f,h in pins.items()),source_hashes=pins,compiler_results=results,reference=reference,scope='Native17F8A0/17F790 finite-domain curve kernels andhardwareFPREM, realoriginalCRTchild. Types/assets/tableDATA sourceauthentic; nonfinite/error/fault/live domains separate.')
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
