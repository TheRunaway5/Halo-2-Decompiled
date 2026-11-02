#!/usr/bin/env python3
"""Positive native18B160/1755B0 proof on an authenticated original view closure."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from validate_native_contact_agent_dispatch_tu15 import sha,save
from build_native_tu15_camera_reference import build_camera_reference
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
 p.add_argument('--compilers',nargs='+',default=['g++','clang++']);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False)
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 units={n:ROOT/f'src/physics/native_tu15_constraint_effect_{n}.cpp'for n in ['selector','location']}
 files=[*units.values(),*[ROOT/f'include/halo2/native_tu15_constraint_effect_{n}.hpp'for n in ['selector','location']],
  ROOT/'include/halo2/native_tu15_constraint_runtime_update.hpp',ROOT/'tools/observe_native_constraint_effect_proximity_tu15.cpp',
  ROOT/'tools/observe_native_activation_agents_tu15.cpp',ROOT/'tools/build_native_tu15_camera_reference.py',ROOT/'tools/screen_native_constraint_effect_camera_reference_tu15.py',Path(__file__)]
 pins={str(f.relative_to(ROOT)):sha(f)for f in files}
 try:
  from capstone import CS_AC_READ,CS_AC_WRITE
  from capstone.x86 import X86_OP_MEM
  assert sha(NEW_DEFAULT)==NEW_PIN;code=_Code(Xbe(NEW_DEFAULT),'.text');metadata={};counts={};hashes={}
  for owner in (0x18b160,0x1755b0):
   body=complete_body(code,owner);counts[hex(owner)]=len(body.code);listing=[]
   for pc in sorted(body.code):
    instruction=code.at(pc);listing.append(f'{pc:x} {instruction.bytes.hex()} {instruction.mnemonic} {instruction.op_str}\n');operands=[]
    if instruction.mnemonic!='lea':
     for op in instruction.operands:
      if op.type!=X86_OP_MEM or op.mem.segment:continue
      operands.append(dict(base=instruction.reg_name(op.mem.base)or'',index=instruction.reg_name(op.mem.index)or'',scale=op.mem.scale,
       displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(op.access&CS_AC_READ),write=bool(op.access&CS_AC_WRITE)))
    metadata[str(pc)]=dict(bytes=instruction.bytes.hex(),size=instruction.size,memory=operands)
   hashes[hex(owner)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
  instructions=scratch/'instructions.json';save(instructions,metadata);reference=build_camera_reference(scratch/'reference')
  save(a.output/'reference-read-closure.json',reference)
  prefix=(ROOT/'tools/observe_native_activation_agents_tu15.cpp').read_text().split('int main(int argc,char**argv)')[0]
  driver=scratch/'driver.cpp';driver.write_text('#include "halo2/native_tu15_constraint_effect_selector.hpp"\n'+prefix+(ROOT/'tools/observe_native_constraint_effect_proximity_tu15.cpp').read_text())
  project=scratch/'project';project.mkdir()
  for name,file in units.items():shutil.copyfile(file,project/(name+'.cpp'))
  source=units['selector'].read_text();needle='if(greater(square,distance))return true;';assert source.count(needle)==1
  (project/'wrong_distance.cpp').write_text(source.replace(needle,'if(greater(distance,square))return true;'))
  (project/'negative.py').write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nj=json.load(open(sys.argv[-1]));assert r.returncode!=0 and not j.get("error") and any(not x["passed"]for x in j["cases"]),(r.returncode,j)\nprint("Original1755B0 rejects reversed native distance comparison")\n')
  cmake=f'''cmake_minimum_required(VERSION 3.20)
project(NativeConstraintEffectProximity LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
add_library(driver OBJECT "{driver}")
target_include_directories(driver PRIVATE "{ROOT}/include")
target_compile_features(driver PRIVATE cxx_std_20)
target_link_libraries(driver PRIVATE nlohmann_json::nlohmann_json)
file(MAKE_DIRECTORY "${{CMAKE_BINARY_DIR}}/evidence")
'''
  retained=[]
  for variant in ['native','wrong_distance']:
   source='selector.cpp'if variant=='native'else'wrong_distance.cpp';name='proximity_'+variant;out='evidence/'+variant+'.json';retained.append(out)
   cmake+=f'add_executable({name} $<TARGET_OBJECTS:driver> {source} location.cpp)\ntarget_include_directories({name} PRIVATE "{ROOT}/include")\ntarget_compile_features({name} PRIVATE cxx_std_20)\ntarget_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
   command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '+(''if variant=='native'else f'"{sys.executable}" "{project}/negative.py" ')+f'$<TARGET_FILE:{name}> "{reference["elf"]}" "{scratch}/reference/metadata.json" "{instructions}" "${{CMAKE_BINARY_DIR}}/{out}"'
   cmake+=f'add_test(NAME {name} COMMAND {command})\nset_tests_properties({name} PROPERTIES TIMEOUT 240)\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in a.compilers:
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),
    '--target','proximity_native','--target','proximity_wrong_distance','--filter','^proximity_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1',
    *sum((['--retain-report',path]for path in retained),[])]
   result=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=result.returncode,command=command))
  unchanged=all(sha(ROOT/f)==h for f,h in pins.items())
  report.update(passed=all(r['exit_code']==0 for r in results)and unchanged,current_sources_unchanged=unchanged,compiler_results=results,
   source_hashes=pins,body_sites=counts,listing_sha256=hashes,original_sha256=NEW_PIN,reference=reference,
   current_MCC_live_input=False,captured_caller_abi=False,manufactured_view_registration=False,supplied_original_outcomes=False,
   remaining=['Current native MCC source-view backing','Camera-qualified full selector positive input','Unmasked faults and additional view/alias domains'])
 finally:
  report['temporary_allocated_bytes_before_cleanup']=sum(f.stat().st_blocks*512 for f in scratch.rglob('*')if f.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
