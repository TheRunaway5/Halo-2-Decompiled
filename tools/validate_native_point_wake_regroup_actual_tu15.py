#!/usr/bin/env python3
"""Native point→wake→regroup on genuine1470 DATA; declared leaf ABIs only."""
import argparse,json,os,shutil,subprocess,sys
from pathlib import Path
from build_native_tu15_contact_point_before_reference import PointBeforeInput,build_constraint_reference,sha,save
from census_native_point_regroup_before_tu15 import ARCHIVE,PIN
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--source-root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);stage=a.stage.resolve()/'staged';source_root=a.source_root.resolve();scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],live_invocation_claimed=False,source_physics_fields_overridden=False)
 try:
  source=PointBeforeInput(ARCHIVE,PIN,scratch/'before');reference=build_constraint_reference(source,scratch/'reference');reference['entry']='Actual1470 typed322F70 plus declared standalone316D40 then3187C0; no captured caller/GP or fabricated physics rows';project=scratch/'project';project.mkdir();shutil.copytree(source_root/'include',project/'include');save(a.output/'reference.json',reference)
  model=json.loads((ROOT/'analysis/physics/native-world-island-regroup-model-20261101/report.json').read_text());assert model['passed'];files=[f for f in model['compiled_cpp_closure']if f!='tests/native_tu15_world_residuals_tests.cpp'];pins={}
  for relative in files:
   path=stage/relative if(stage/relative).is_file()else source_root/relative;target=project/relative;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(path,target);pins[relative]=sha(path)
  for path in(source_root/'src/physics').rglob('*'):
   if path.suffix in('.hpp','.inc'):target=project/path.relative_to(source_root);target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(path,target);pins[str(path.relative_to(source_root))]=sha(path)
  for path in(stage/'include').rglob('*.hpp'):shutil.copyfile(path,project/'include'/path.relative_to(stage/'include'));pins[str(path.relative_to(stage))]=sha(path)
  prefix=(ROOT/'tools/observe_native_activation_agents_tu15.cpp').read_text().split('int main(int argc,char**argv)')[0];prefix=prefix.replace('if(retained(p,b.size()))ledger.push_back','if(false&&retained(p,b.size()))ledger.push_back');assert prefix.count('if(false&&retained(p,b.size()))')==2
  driver=ROOT/'tools/observe_native_point_wake_regroup_actual_tu15.cpp';includes=''.join('#include "halo2/'+h+'"\n'for h in('native_tu15_contact_point_binding.hpp','native_tu15_object_activation_residuals.hpp','native_tu15_island_regroup_binding.hpp'));(project/'driver.cpp').write_text(includes+prefix+driver.read_text());pins[str(driver.relative_to(ROOT))]=sha(driver);pins[str(Path(__file__).relative_to(ROOT))]=sha(Path(__file__))
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(nativeWakeRegroup LANGUAGES CXX)\ninclude(CTest)\nfind_package(Threads REQUIRED)\nadd_executable(native_wake_regroup driver.cpp '+' '.join('"'+str(project/f)+'"'for f in files)+')\ntarget_include_directories(native_wake_regroup PRIVATE "'+str(project/'include')+'")\ntarget_compile_features(native_wake_regroup PRIVATE cxx_std_20)\ntarget_compile_options(native_wake_regroup PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\ntarget_link_libraries(native_wake_regroup PRIVATE Threads::Threads)\nadd_test(NAME native_wake_regroup COMMAND "'+sys.executable+'" "'+str(ROOT/'tools/run_test.py')+'" -- $<TARGET_FILE:native_wake_regroup> "'+reference['elf']+'" "'+str(source.path/'capture-native-before.json')+'" "${CMAKE_BINARY_DIR}/native.json")\nset_tests_properties(native_wake_regroup PROPERTIES TIMEOUT 180)\n';(project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in a.compilers:
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','native_wake_regroup','--filter','^native_wake_regroup$','--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--retain-report','native.json'];r=subprocess.run(command,check=False);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  report.update(passed=all(r['exit_code']==0 for r in results),compiler_results=results,source_hashes=pins,reference=reference,scope='Full unchanged actual1470 source banks after each physical original/native phase. Point actualformals, then standalone declared wake/regroup formals on source-computed DATA. No original GP/SIMD/after supplied or rows/flags/poses manufactured. No actualcaller/live invocation claim.')
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
