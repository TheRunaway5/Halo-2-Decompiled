#!/usr/bin/env python3
"""Native virtual34 features inside genuine solver and complete original root."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);p.add_argument('--context',action='append',choices=['gemini-column0','gemini-column1','elongation-crates']);p.add_argument('--producer',type=int);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);p.add_argument('--variants',nargs='+',type=int,default=[0,1,2,3]);p.add_argument('--negative-controls',action='store_true');a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN
  source=ROOT/'src/physics/native_tu15_contact_features.cpp';header=ROOT/'include/halo2/native_tu15_contact_features.hpp';fragment=ROOT/'tools/observe_native_contact_features_handoff_full_return_tu15.cpp';bridge=ROOT/'tools/observe_native_contact_producer_tu15.cpp';handoff_source=ROOT/'src/physics/native_tu15_contact_features_handoff.cpp';handoff_header=ROOT/'include/halo2/native_tu15_contact_features_handoff.hpp';files=[source,header,handoff_source,handoff_header,fragment,bridge,Path(__file__)];pins={str(p.relative_to(ROOT)):sha(p)for p in files};project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');shutil.copyfile(source,project/'native.cpp');shutil.copyfile(handoff_source,project/'handoff.cpp');(project/'observer.cpp').write_text('#include "halo2/native_tu15_contact_features_handoff.hpp"\n'+bridge.read_text().split('int main(int argc,char**argv)')[0]+fragment.read_text());contexts=[];cases=[];base=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458'
  for name in(a.context or('gemini-column0','gemini-column1','elongation-crates')):
   capture=base/('iteration-04-'+name)/'original/required-failure-inputs';original=scratch/('original-'+name);command=[sys.executable,str(ROOT/'tools/replay_live_phantom_hardware_tu15.py'),'--native-frame-semantics','--return-breakpoints','--capture',str(capture),'--output',str(original)];r=subprocess.run(command,capture_output=True,text=True,timeout=180);assert(original/'original-hardware').exists(),r.stderr[-3000:]
   for producer in([a.producer]if a.producer is not None else range(3 if name=='elongation-crates'else 2)):
    cases.extend(dict(name=f'{name}_{producer}_{variant}',producer=producer,variant=variant,elf=original/'original-hardware',capture=capture/'capture.json')for variant in a.variants)
   contexts.append(dict(name=name,capture=str(capture),capture_sha256=sha(capture/'capture.json'),reference_command=command,reference_raw_exit=r.returncode));print(name,'complete original root inputs ready',flush=True)
  variants={};text=source.read_text()
  if a.negative_controls:
   for name,needle,replacement in(('wrong_polygon_vertex','shape.identity+(id+2u)*16u','shape.identity+(id+3u)*16u'),):
    assert text.count(needle)==1;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
  (project/'negative.py').write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True);j=json.load(open(sys.argv[-1]));assert r.returncode!=0 and not j.get("error") and not j["passed"],(r.returncode,j)\nprint("Full original root rejects native feature mutation")\n')
  cmake='''cmake_minimum_required(VERSION 3.20)
project(native_contact_features_full_return LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Threads REQUIRED)
add_library(driver OBJECT observer.cpp)
target_include_directories(driver PRIVATE include)
target_compile_features(driver PRIVATE cxx_std_20)
target_compile_options(driver PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(driver PRIVATE nlohmann_json::nlohmann_json)
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/evidence")
''';retain=[]
  for mode in('native',*variants):
   cmake+=f'add_executable(features_{mode} $<TARGET_OBJECTS:driver> {mode}.cpp handoff.cpp)\ntarget_include_directories(features_{mode} PRIVATE include)\ntarget_compile_features(features_{mode} PRIVATE cxx_std_20)\ntarget_link_libraries(features_{mode} PRIVATE Threads::Threads)\ntarget_compile_options(features_{mode} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
   selected=next((c for c in cases if c['variant']==1),cases[0])
   for c in(cases if mode=='native'else[selected]):
    relative='evidence/'+mode+'_'+c['name']+'.json';retain.append(relative);negative=''if mode=='native'else f'"{sys.executable}" "{project}/negative.py" ';cmake+=f'add_test(NAME features_{mode}_{c["name"]} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {negative}$<TARGET_FILE:features_{mode}> "{c["elf"]}" "{c["capture"]}" {c["producer"]} {c["variant"]} "${{CMAKE_BINARY_DIR}}/{relative}")\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in a.compilers:
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','features_native',*sum((['--target','features_'+name]for name in variants),[]),'--filter','^features_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1',*sum((['--retain-report',p]for p in retain),[])];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  unchanged=all(sha(ROOT/p)==pin for p,pin in pins.items());report.update(passed=all(r['exit_code']==0 for r in results)and unchanged,compiler_results=results,contexts=contexts,source_hashes=pins,sources_unchanged=unchanged,original_sha256=NEW_PIN,original_cases_per_compiler=len(cases),negative_controls=list(variants),declared_variants=a.variants,supplied_original_outcomes=False,captured_after_consumed=False,original_volatile_postimages_consumed=False,production_native_only_closure=False,remaining=['runtime source helper/model and actual World integration','other original solver helpers and parent'])
 except BaseException as error:report['error']=str(error)or repr(error);report['error_type']=type(error).__name__;raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
