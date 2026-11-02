#!/usr/bin/env python3
"""Qualify native22D0A0 on genuine existing instance lists, not invented rows."""
import argparse,hashlib,json,os,re,shutil,subprocess,sys,tarfile
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from validate_native_contact_agent_dispatch_tu15 import sha,save
from build_native_tu15_before_reference import build_before_reference
from validate_native_body_constraint_update_tu15 import ARCHIVE,ARCHIVE_SHA

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
 p.add_argument('--compilers',nargs='+',default=['g++','clang++'])
 p.add_argument('--case',default=r'(gemini-column[01]_[01]|elongation-crates_[12]|actual-warthog_[1-4])_[0-3]')
 a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False)
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
 report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 units={name:ROOT/path for name,path in {
  'removal':'src/physics/native_tu15_model_constraint_removal.cpp',
  'release':'src/physics/native_tu15_constraint_reference_release.cpp',
  'slots':'src/physics/native_tu15_constraint_body_slots.cpp',
  'cache':'src/physics/native_tu15_constraint_scalar_cache.cpp',
  'delete':'src/core/native_tu15_data_array_delete.cpp'}.items()}
 observer=ROOT/'tools/observe_native_model_constraint_removal_tu15.cpp'
 files=[*units.values(),observer,Path(__file__),ROOT/'tools/observe_native_activation_agents_tu15.cpp',
  ROOT/'tools/build_native_tu15_before_reference.py',*[ROOT/'include/halo2'/f for f in
  ('native_tu15_model_constraint_removal.hpp','native_tu15_constraint_reference_release.hpp',
   'native_tu15_constraint_body_slots.hpp','native_tu15_constraint_scalar_cache.hpp',
   'native_tu15_data_array_delete.hpp','native_tu15_physics_frame.hpp')]]
 pins={str(f.relative_to(ROOT)):sha(f)for f in files};save(a.output/'report.json',report)
 try:
  from capstone import CS_AC_READ,CS_AC_WRITE
  from capstone.x86 import X86_OP_MEM
  assert sha(NEW_DEFAULT)==NEW_PIN;code=_Code(Xbe(NEW_DEFAULT),'.text');metadata={};counts={};hashes={}
  for owner in (0x22d0a0,0x22e1b0,0x22e390,0xde8c0,0x22fe30,0x16e8a0):
   body=complete_body(code,owner);counts[hex(owner)]=len(body.code);listing=[]
   for pc in sorted(body.code):
    i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');operands=[]
    if i.mnemonic!='lea':
     for op in i.operands:
      if op.type!=X86_OP_MEM or op.mem.segment:continue
      operands.append(dict(base=i.reg_name(op.mem.base)or'',index=i.reg_name(op.mem.index)or'',scale=op.mem.scale,
       displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(op.access&CS_AC_READ)or i.mnemonic=='test',write=bool(op.access&CS_AC_WRITE)))
    metadata[str(pc)]=dict(bytes=i.bytes.hex(),size=i.size,memory=operands)
   hashes[hex(owner)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
  # Unknown lifetime services are observed at their real call boundary. A
  # native refusal may qualify that prefix; it never supplies a child outcome.
  for pc in (0x22d13f,0x22d155,0x22d233,0x22e1fc):metadata[str(pc)]['stop_before']=True
  instruction_file=scratch/'instructions.json';save(instruction_file,metadata)
  prefix=(ROOT/'tools/observe_native_activation_agents_tu15.cpp').read_text().split('int main(int argc,char**argv)')[0]
  prefix=prefix.replace('const auto&i=metadata.at(key);','const auto&i=metadata.at(key);if(i.value("stop_before",false))break;')
  driver=scratch/'driver.cpp';driver.write_text('#include "halo2/native_tu15_model_constraint_removal.hpp"\n'+prefix+observer.read_text())
  contexts=[];cases=[];base=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458'
  for name in ('gemini-column0','gemini-column1','elongation-crates','actual-warthog'):
   selections=range(5)if name=='actual-warthog'else range(3)if name=='elongation-crates'else (0,1)
   selected=[(selection,mode)for selection in selections for mode in range(4)if re.fullmatch(a.case,f'{name}_{selection}_{mode}')]
   if not selected:continue
   archive=None
   if name=='actual-warthog':
    assert sha(ARCHIVE)==ARCHIVE_SHA;capture=scratch/'actual-warthog-inputs';capture.mkdir()
    with tarfile.open(ARCHIVE,'r:*')as stream:
     for member in stream.getmembers():
      target=(capture/member.name).resolve();assert target==capture or target.is_relative_to(capture);assert member.isfile()or member.isdir()
     stream.extractall(capture,filter='data')
    archive=dict(path=str(ARCHIVE),sha256=ARCHIVE_SHA)
   else:capture=base/('iteration-04-'+name)/'original/required-failure-inputs'
   loader=build_before_reference(capture,scratch/('reference-'+name));contexts.append(dict(name=name,capture_metadata_sha256=sha(capture/'capture.json'),archive=archive,loader=loader))
   cases.extend(dict(name=f'{name}_{selection}_{mode}',elf=loader['elf'],capture=str(capture/'capture.json'),selection=selection,mode=mode)for selection,mode in selected)
  assert cases;project=scratch/'project';project.mkdir()
  for name,path in units.items():shutil.copyfile(path,project/(name+'.cpp'))
  text=units['removal'].read_text();needle='read(m,list_row+2u,2)-1u';assert text.count(needle)==1
  (project/'wrong_list_count.cpp').write_text(text.replace(needle,'read(m,list_row+2u,2)-2u'))
  (project/'negative.py').write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nj=json.load(open(sys.argv[-1]));assert r.returncode!=0 and not j.get("error") and any(not c["passed"]for c in j["cases"]),(r.returncode,j)\nprint("Original22D0A0 rejects incorrect native list removal count")\n')
  cmake=f'''cmake_minimum_required(VERSION 3.20)
project(NativeModelConstraintRemoval LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
add_library(driver OBJECT "{driver}")
target_include_directories(driver PRIVATE "{ROOT}/include")
target_compile_features(driver PRIVATE cxx_std_20)
target_link_libraries(driver PRIVATE nlohmann_json::nlohmann_json)
file(MAKE_DIRECTORY "${{CMAKE_BINARY_DIR}}/evidence")
''';retained=[]
  for variant in ('native','wrong_list_count'):
   first='removal.cpp'if variant=='native'else'wrong_list_count.cpp'
   cmake+=f'add_executable(removal_{variant} $<TARGET_OBJECTS:driver> {first} release.cpp slots.cpp cache.cpp delete.cpp)\ntarget_include_directories(removal_{variant} PRIVATE "{ROOT}/include")\ntarget_compile_features(removal_{variant} PRIVATE cxx_std_20)\ntarget_compile_options(removal_{variant} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
   for case in cases if variant=='native'else cases[:1]:
    name=variant+'_'+case['name'];out='evidence/'+name+'.json';retained.append(out)
    command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '+(''if variant=='native'else f'"{sys.executable}" "{project}/negative.py" ')+f'$<TARGET_FILE:removal_{variant}> "{case["elf"]}" "{case["capture"]}" {case["selection"]} {case["mode"]} "{instruction_file}" "${{CMAKE_BINARY_DIR}}/{out}"'
    cmake+=f'add_test(NAME removal_{name} COMMAND {command})\nset_tests_properties(removal_{name} PROPERTIES TIMEOUT 60)\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in a.compilers:
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','removal_native','--target','removal_wrong_list_count','--filter','^removal_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1',*sum((['--retain-report',path]for path in retained),[])]
   result=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=result.returncode,command=command))
  unchanged=all(sha(ROOT/f)==v for f,v in pins.items())
  report.update(passed=all(r['exit_code']==0 for r in results)and unchanged,current_sources_unchanged=unchanged,
   contexts=contexts,compiler_results=results,physical_island_cases_per_compiler=len(cases),negative_controls=['wrong_list_count'],
   source_hashes=pins,body_sites=counts,listing_sha256=hashes,original_sha256=NEW_PIN,captured_after_consumed=False,
   instruction_patches=[],native_parent_falls_back=False,manufactured_constraints=False,
   remaining=['Actual17A0F0 and22FEA0 lifetime callbacks if reached','Positive authored constraint creation','Fault/alias/mutable-observer domains','Production integration and visible multiplayer'])
 except BaseException as error:report['error']=str(error);raise
 finally:
  report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file())
  save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
