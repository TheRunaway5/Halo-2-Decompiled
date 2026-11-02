#!/usr/bin/env python3
"""All three genuine304 query invocations through one frozen native closure."""
import argparse,json,os,shutil,subprocess,sys
from pathlib import Path
from native_contact_process_before_input_tu15 import NativeContactProcessBeforeInput,sha
from build_native_tu15_constraint_before_reference import build_constraint_reference,save
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);p.add_argument('--mutants',action='store_true');a=p.parse_args();a.stage=a.stage.resolve();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();manifest=a.stage/'source-pins.json';stage=json.loads(manifest.read_text());tools=['tools/observe_native_contact_query_actual_tu15.cpp','tools/observe_native_contact_candidate_entry_tu15.cpp','tools/validate_native_contact_query_simplex_all_tu15.py','tools/native_contact_process_before_input_tu15.py'];pins={r:sha(ROOT/r)for r in tools};report={'passed':False,'owned_scratch':str(scratch),'stage_manifest_sha256':sha(manifest),'retained_large_artifacts':[],'original_outcomes_supplied':False,'native_original_executor_calls':0,'tools_sha256':pins}
 try:
  for name,row in stage['files'].items():assert sha(a.stage/'staged'/name)==row['sha256'],name
  archive=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-reorientation-joint-original-v1479-20261102/iteration-01-readonly-process-304350-driving-vehicle-camera/required-native-contact-process-304350-before/actual-native-before.tar.zst'
  before=NativeContactProcessBeforeInput(archive,'5ef6aba031fc184d390fcc65ad6da055e16a279212232da4252a36e0b69d5c9c',scratch/'before',0x304350);reference=build_constraint_reference(before,scratch/'reference')
  for ordinal in range(3):save(scratch/f'input{ordinal}.json',dict(before.input,declared_query_ordinal=ordinal))
  project=scratch/'project';project.mkdir();shutil.copytree(a.stage/'staged/include',project/'include')
  for f in (a.stage/'staged/src/physics').iterdir():shutil.copyfile(f,project/f.name)
  prefix=(ROOT/tools[1]).read_text().split('int main(int argc,char**argv)')[0];(project/'driver.cpp').write_text(prefix+(ROOT/tools[0]).read_text());kernel='native_tu15_convex_simplex_solver.cpp';variants={}
  if a.mutants:
   original=(project/kernel).read_text();edits={'wrong_second_direction':('xor_bits(add(add(tx,ty),tz),sign)','add(add(tx,ty),tz)'),'wrong_normalization':('0x3f000000u','0x3f800000u'),'wrong_second_transform':('add(add(x_term,y_term),zt));\n              m.word(0x30f451u','subtract(add(x_term,y_term),zt));\n              m.word(0x30f451u')}
   for name,(old,new)in edits.items():assert original.count(old)>=1,name;(project/(name+'.cpp')).write_text(original.replace(old,new));variants[name]=name+'.cpp'
   (project/'negative.py').write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nd=json.load(open(sys.argv[-1]))\nassert r.returncode!=0 and d["passed"]is False,(r.returncode,r.stdout,r.stderr)\nassert not d.get("error"),d\nassert d.get("cases") and any(not c["passed"]for c in d["cases"]),d\nprint("Numeric mutant rejected by actual query:",sys.argv[-1])\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(actualContactQueryAll LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\n';common=[u for u in stage['units']if u!=kernel];cmake+='add_library(common OBJECT driver.cpp '+' '.join(common)+')\ntarget_include_directories(common PRIVATE include)\ntarget_compile_features(common PRIVATE cxx_std_20)\ntarget_compile_options(common PRIVATE -fno-fast-math -ffp-contract=off -frounding-math -ffunction-sections -fdata-sections)\ntarget_link_libraries(common PRIVATE nlohmann_json::nlohmann_json)\n'
  for name,unit in {'query':kernel,**variants}.items():cmake+='add_executable('+name+' $<TARGET_OBJECTS:common> '+unit+')\ntarget_include_directories('+name+' PRIVATE include)\ntarget_compile_features('+name+' PRIVATE cxx_std_20)\ntarget_compile_options('+name+' PRIVATE -fno-fast-math -ffp-contract=off -frounding-math -ffunction-sections -fdata-sections)\ntarget_link_options('+name+' PRIVATE -Wl,--gc-sections)\ntarget_link_libraries('+name+' PRIVATE nlohmann_json::nlohmann_json)\n'
  cmake+='file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/evidence")\n';retained=[]
  for ordinal in range(3):
   for mode in range(4):
    name=f'evidence/query{ordinal}-mode{mode}.json';retained.append(name);cmake+=f'add_test(NAME query_{ordinal}_{mode} COMMAND query "{reference["elf"]}" "{scratch}/input{ordinal}.json" "{before.path}/capture-native-before.json" {mode} "${{CMAKE_BINARY_DIR}}/{name}")\n'
   for name in variants:
    path=f'evidence/query{ordinal}-{name}.json';retained.append(path);cmake+=f'add_test(NAME query_{ordinal}_mutant_{name} COMMAND "{sys.executable}" "{project}/negative.py" $<TARGET_FILE:{name}> "{reference["elf"]}" "{scratch}/input{ordinal}.json" "{before.path}/capture-native-before.json" 0 "${{CMAKE_BINARY_DIR}}/{path}")\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for cc in a.compilers:
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/cc),'--target','query',*sum((['--target',n]for n in variants),[]),'--filter','^query_','--cmake-arg=-DCMAKE_CXX_COMPILER='+cc,'--jobs','2','--test-jobs','1',*sum((['--retain-report',n]for n in retained),[])];r=subprocess.run(command);results.append({'compiler':cc,'exit_code':r.returncode,'command':command})
  report['results']=results;assert all(r['exit_code']==0 for r in results),'Actual query matrix build or comparison failed; retain diagnostic'
  for cc in a.compilers:
   for ordinal in range(3):
    for mode in range(4):
     q=a.output/cc/'evidence/evidence'/f'query{ordinal}-mode{mode}.json';row=json.loads(q.read_text());assert row['passed'] and not row.get('error')and len(row['cases'])==2
  assert all(sha(ROOT/r)==h for r,h in pins.items());assert all(sha(a.stage/'staged'/n)==r['sha256']for n,r in stage['files'].items())
  report.update(passed=True,source_inputs_unchanged=True,compilers=a.compilers,genuine_query_ordinals=[0,1,2],fp_modes=[0,1,2,3],poisons=[165,90],profiles_per_compiler=24,mutants_per_query_per_compiler=list(variants),actual_input=before.receipt,generated_observer_sha256=sha(project/'driver.cpp'),scope='Three separately genuine304 query BEFORE boundaries, same native2E4FE0/2F8630 and extended shared solver. All captured banks, defined output/valid/result and FP strict. No Source outcomes, private GP/control bytes or native executor fallback. Ordered read/store/raw private/World/live domains remain separate.')
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(q.stat().st_blocks*512 for q in scratch.rglob('*')if q.is_file());save(a.output/'report.json',report)
 return 0
if __name__=='__main__':raise SystemExit(main())
