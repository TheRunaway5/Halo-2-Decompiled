#!/usr/bin/env python3
"""Whole native contact producer against unchanged original service execution.

Enumerate the actual calls before choosing them; use all calls in the three
protected contexts. Temporary original ELFs/compilers/banks share one owner.
"""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
def sha(p):
 with Path(p).open('rb')as f:return hashlib.file_digest(f,'sha256').hexdigest()
def save(p,j):p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);a.output=a.output.resolve();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;source=ROOT/'src/physics/native_tu15_contact_producer.cpp';observer=ROOT/'tools/observe_native_contact_producer_tu15.cpp';dependencies=[ROOT/'src/physics/native_tu15_geometry.cpp',ROOT/'src/physics/native_tu15_contact_records.cpp',ROOT/'src/physics/native_tu15_contact_buffers.cpp',ROOT/'src/physics/native_tu15_array_growth.cpp'];files=[source,observer,*dependencies,ROOT/'include/halo2/native_tu15_contact_buffers.hpp',ROOT/'include/halo2/native_tu15_array_growth.hpp',ROOT/'src/physics/native_tu15_contact_unwind_internal.hpp',ROOT/'include/halo2/native_tu15_contact_producer.hpp',ROOT/'include/halo2/native_tu15_geometry.hpp',ROOT/'include/halo2/native_tu15_contact_records.hpp',ROOT/'include/halo2/native_tu15_physics_frame.hpp',ROOT/'include/halo2/physics_contact_record.hpp',Path(__file__)];pins={str(f.relative_to(ROOT)):sha(f)for f in files};probe=scratch/'enumerator';compile=[ 'g++','-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),str(observer),str(source),*map(str,dependencies),'-o',str(probe)];r=subprocess.run(compile,capture_output=True,text=True);assert r.returncode==0,r.stderr[-6000:];contexts=[];cases=[]
  base=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458'
  for name in('gemini-column0','gemini-column1','elongation-crates'):
   capture=base/('iteration-04-'+name)/'original/required-failure-inputs';replay=scratch/('original-'+name);command=[sys.executable,str(ROOT/'tools/replay_live_phantom_hardware_tu15.py'),'--native-frame-semantics','--return-breakpoints','--capture',str(capture),'--output',str(replay)];r=subprocess.run(command,capture_output=True,text=True,timeout=180);assert(replay/'original-hardware').exists(),r.stderr[-3000:];comparison=json.loads((replay/'report.json').read_text())if(replay/'report.json').exists()else{};enum=a.output/(name+'-original-enumeration.json');e=subprocess.run([str(probe),str(replay/'original-hardware'),str(capture/'capture.json'),'list',str(enum)],capture_output=True,text=True,timeout=180);assert e.returncode==0,e.stderr[-4000:];entries=json.loads(enum.read_text())['producer_entries'];assert entries
   contexts.append(dict(name=name,capture=str(capture),capture_metadata_sha256=sha(capture/'capture.json'),ordinary_reference_command=command,ordinary_reference_compare_exit=r.returncode,ordinary_reference_report_passed=comparison.get('passed'),original_elf_sha256=sha(replay/'original-hardware'),producer_calls=entries,enumeration_report_sha256=sha(enum)));cases.extend(dict(name=name+'_'+str(row['ordinal']),ordinal=row['ordinal'],executable=replay/'original-hardware',capture=capture/'capture.json')for row in entries);print(name,len(entries),'genuine original producer calls; maximum records',max(row['record_count']for row in entries),flush=True)
  project=scratch/'project';(project/'src').mkdir(parents=True);shutil.copytree(ROOT/'include',project/'include');shutil.copyfile(source,project/'src/native.cpp');shutil.copyfile(observer,project/'src/observer.cpp');shutil.copyfile(dependencies[0],project/'src/geometry.cpp');shutil.copyfile(dependencies[1],project/'src/records.cpp');shutil.copyfile(dependencies[2],project/'src/buffers.cpp');shutil.copyfile(dependencies[3],project/'src/growth.cpp');shutil.copyfile(ROOT/'src/physics/native_tu15_contact_unwind_internal.hpp',project/'src/native_tu15_contact_unwind_internal.hpp');text=source.read_text();variants={}
  needle='m.word(0x304585,record+0x20u,m.word(p+0xb0u))';assert text.count(needle)==1;variants['wrong_scalar']=text.replace(needle,'m.word(0x304585,record+0x20u,m.word(p+0xb0u)^1u)')
  needle='data+count*128u-4u,type==1u||type==2u,1)';assert text.count(needle)==1;variants['drop_filter_flag']=text.replace(needle,'data+count*128u-4u,0,1)')
  needle='if(!(compare(m.word(p))&0x4100u))';assert text.count(needle)==1;variants['wrong_minimum']=text.replace(needle,'if((compare(m.word(p))&0x4100u)==0x100u)')
  for name,value in variants.items():(project/'src'/f'{name}.cpp').write_text(value)
  (project/'negative.py').write_text('''import json,subprocess,sys
r=subprocess.run(sys.argv[1:],capture_output=True,text=True)
j=json.load(open(sys.argv[-1]));assert r.returncode!=0 and 'error'not in j and(not j.get('fp_equal',True)or not j.get('all_contact_packet_bytes_equal',True)or j.get('different_bytes',0)>0),(r.returncode,r.stdout,r.stderr)
print('Actual original producer rejects native mutation:',json.dumps({k:j[k]for k in('different_bytes','all_contact_packet_bytes_equal','fp_equal')}))
''')
  cmake='''cmake_minimum_required(VERSION 3.20)
project(NativeContactProducerTu15 LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
add_library(driver OBJECT src/observer.cpp)
add_library(dependencies OBJECT src/geometry.cpp src/records.cpp src/buffers.cpp src/growth.cpp)
foreach(target driver dependencies)
 target_include_directories(${target} PRIVATE include)
 target_compile_features(${target} PRIVATE cxx_std_20)
 target_compile_options(${target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
endforeach()
target_link_libraries(driver PRIVATE nlohmann_json::nlohmann_json)
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/evidence")
''';retain=[]
  selected=max((row for c in contexts if c['name']=='elongation-crates'for row in c['producer_calls']),key=lambda row:row['record_count']);assert any(flags[0]==1 for flags in selected['record_flags']);negative_case=next(c for c in cases if c['name']=='elongation-crates_'+str(selected['ordinal']))
  for variant in('native',*variants):
   cmake+=f'add_executable(producer_{variant} $<TARGET_OBJECTS:driver> $<TARGET_OBJECTS:dependencies> src/{variant}.cpp)\ntarget_include_directories(producer_{variant} PRIVATE include)\ntarget_compile_features(producer_{variant} PRIVATE cxx_std_20)\ntarget_compile_options(producer_{variant} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
   for c in cases if variant=='native'else[negative_case]:
    name=variant+'_'+c['name'];relative='evidence/'+name+'.json';retain.append(relative);output='${CMAKE_BINARY_DIR}/'+relative;command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '+(''if variant=='native'else f'"{sys.executable}" "{project}/negative.py" ')+f'$<TARGET_FILE:producer_{variant}> "{c["executable"]}" "{c["capture"]}" "{c["ordinal"]}" "{output}"';cmake+=f'add_test(NAME producer_{name} COMMAND {command})\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','producer_native',*sum((['--target','producer_'+name]for name in variants),[]),'--filter','^producer_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1',*sum((['--retain-report',path]for path in(*retain,'Testing/Temporary/LastTest.log')),[])];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  unchanged=all(sha(ROOT/name)==pin for name,pin in pins.items());assert unchanged,'Native source/dependency cohort changed';report.update(passed=all(row['exit_code']==0 for row in results),contexts=contexts,native_physical_cases_per_compiler=len(cases),negative_controls=list(variants),compiler_results=results,source_sha256=pins,source_snapshot_unchanged=unchanged,original_image_sha256=NEW_PIN,supplied_original_outcomes=False,instruction_patches=[],remaining_services_execute_original_cpu=True,production_native_only_closure=False,old_whole_frame_raw_gate_unchanged=True,retained_small_physical_reports_per_compiler=len(retain),remaining=['Actual production native remaining service ownership and caller binding','X87 fault resume and unmasked/overfull owner','Live native producer/multiplayer acceptance','Protected raw RED whole-frame and future/native paired histories','Xbox hardware numerical identity'])
 except BaseException as error:report['error']=str(error);raise
 finally:report['temporary_allocated_bytes_before_owner_cleanup']=sum(f.stat().st_blocks*512 for f in scratch.rglob('*')if f.is_file());save(a.output/'report.json',report)
 print(json.dumps({k:report.get(k)for k in('passed','native_physical_cases_per_compiler','negative_controls')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
