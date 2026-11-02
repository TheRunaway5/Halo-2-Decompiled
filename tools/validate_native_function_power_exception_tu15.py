#!/usr/bin/env python3
"""Typed native infinite-power classification and kind2/8 CRT error policy."""
import argparse,os,shutil,subprocess,sys
from pathlib import Path
from build_native_tu15_constraint_before_reference import build_constraint_reference,sha,save
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 names=['include/halo2/native_tu15_function_power_special.hpp','src/core/native_tu15_function_power_special.cpp','include/halo2/native_tu15_function_power_error.hpp','src/core/native_tu15_function_power_error.cpp','include/halo2/source_power_special_tu15.hpp','src/physics/source_power_special_tu15.cpp','include/halo2/source_math_error_tu15.hpp','src/physics/source_math_error_tu15.cpp','tools/observe_native_function_power_exception_tu15.cpp','tools/observe_native_activation_agents_tu15.cpp',str(Path(__file__).relative_to(ROOT))];pins={n:sha(ROOT/n)for n in names}
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT)
  assert image.read(0x37cf1c,3)==bytes.fromhex('dd7108')and image.read(0x37cf28,3)==bytes.fromhex('dd6108')
  class Declared:pass
  source=Declared();source.path=scratch/'declared';source.path.mkdir();source.regions=[];source.receipt=dict(declared_function_math_inputs=True,original_sha256=NEW_PIN,captured_runtime_inputs=False,source_outcomes_supplied=False)
  banks=[(0x1000000,bytes(0x1000))]+[(s.va,image.section_bytes(s)+bytes(max(0,s.vsize-s.rsize)))for s in image.sections if s.name!='.text']
  for n,(address,raw)in enumerate(banks):name=f'input-{n}.bin';(source.path/name).write_bytes(raw);source.regions.append(dict(address=address,size=len(raw),before_file=name))
  reference=build_constraint_reference(source,scratch/'reference');reference['entry']='Declaredordinary37D948 RET0(double,double,output) and3334F0 RET0(functionCode,record,CWptr), no capturedcaller';save(a.output/'reference.json',reference);save(scratch/'fixture.json',dict(regions=source.regions))
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');prefix=(ROOT/'tools/observe_native_activation_agents_tu15.cpp').read_text().split('int main(int argc,char**argv)')[0]
  (project/'driver.cpp').write_text('#include <bit>\n#include <xmmintrin.h>\n#include "halo2/native_tu15_function_power_special.hpp"\n#include "halo2/native_tu15_function_power_error.hpp"\n'+prefix+(ROOT/'tools/observe_native_function_power_exception_tu15.cpp').read_text())
  special=(ROOT/'src/core/native_tu15_function_power_special.cpp').read_text();error=(ROOT/'src/core/native_tu15_function_power_error.cpp').read_text();(project/'special.cpp').write_text(special);(project/'error.cpp').write_text(error)
  (project/'wrong_special.cpp').write_text(special.replace('absolute_base<one?infinity:0u','absolute_base>one?infinity:0u'))
  (project/'wrong_error.cpp').write_text(error.replace('store_error_number(34)','store_error_number(33)'))
  shutil.copyfile(ROOT/'src/physics/source_power_special_tu15.cpp',project/'positive.cpp');shutil.copyfile(ROOT/'src/physics/source_math_error_tu15.cpp',project/'policy.cpp')
  (project/'negative.py').write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True);j=json.load(open(sys.argv[-1]));assert r.returncode!=0 and not j.get("error")and not j["passed"],(r.returncode,j)\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(nativeFunctionException LANGUAGES CXX)\ninclude(CTest)\nadd_library(driver OBJECT driver.cpp)\ntarget_include_directories(driver PRIVATE "'+str(project/'include')+'")\ntarget_compile_features(driver PRIVATE cxx_std_20)\n'
  for variant,s,e in(('native','special.cpp','error.cpp'),('wrong_special','wrong_special.cpp','error.cpp'),('wrong_error','special.cpp','wrong_error.cpp')):
   cmake+=f'add_executable(exception_{variant} $<TARGET_OBJECTS:driver> {s} {e} positive.cpp policy.cpp)\ntarget_include_directories(exception_{variant} PRIVATE "{project}/include")\ntarget_compile_features(exception_{variant} PRIVATE cxx_std_20)\ntarget_compile_options(exception_{variant} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n';negative=''if variant=='native'else f'"{sys.executable}" "{project}/negative.py" ';cmake+=f'add_test(NAME exception_{variant} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {negative}$<TARGET_FILE:exception_{variant}> "{reference["elf"]}" "{scratch}/fixture.json" "${{CMAKE_BINARY_DIR}}/{variant}.json")\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','exception_native','--target','exception_wrong_special','--target','exception_wrong_error','--filter','^exception_','--jobs','1','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--retain-report','native.json','--retain-report','wrong_special.json','--retain-report','wrong_error.json'];r=subprocess.run(command,check=False);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  report.update(passed=all(r['exit_code']==0 for r in results)and all(sha(ROOT/f)==h for f,h in pins.items()),source_hashes=pins,compiler_results=results,reference=reference,
   scope='Source37D948 caller-discarded-FP pureinfinite result/event closure andwhole3334F0 kind2/8 nativeerrorpolicy/errno/FP closure. Allinputs declared, no capturedoutcomes supplied. Existing positiveclassification anddomain/range nativepolicy reused. Extended13DE70 composition remains separate/frozen.')
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
