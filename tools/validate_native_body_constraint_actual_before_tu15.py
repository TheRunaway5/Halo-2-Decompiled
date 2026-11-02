#!/usr/bin/env python3
"""Compose native22D460 on authenticated positive frame-level BEFORE DATA.

This is a declared formal component entry, not an observed22D460 invocation.
All native children execute semantic kernels, with explicit unknown-owner
refusals. No parent backend, captured GP/SIMD or expected after value is used.
"""
import argparse,hashlib,os,shutil,subprocess,sys
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from build_native_tu15_constraint_before_reference import BeforeInput,build_constraint_reference,sha,save
from census_native_constraint_component_before_tu15 import census
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture',type=Path,required=True);p.add_argument('--archive-sha256',required=True);p.add_argument('--output',type=Path,required=True)
 p.add_argument('--compilers',nargs='+',default=['g++','clang++']);p.add_argument('--parent-source',type=Path);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False)
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 units={name:ROOT/path for name,path in{
  'parent':'src/physics/native_tu15_body_constraint_update.cpp','geometry':'src/physics/native_tu15_geometry.cpp',
  'allocate':'src/core/native_tu15_data_array_allocate.cpp','lookup':'src/physics/native_tu15_model_constraint_lookup.cpp',
  'capacity':'src/physics/native_tu15_model_constraint_capacity.cpp','creation':'src/physics/native_tu15_model_constraint_creation.cpp',
  'points':'src/physics/native_tu15_constraint_local_points.cpp','inverse':'src/rendering/world/native_tu15_camera_projection.cpp',
  'normalize':'src/physics/native_tu15_vector_normalize.cpp','motion':'src/physics/native_tu15_constraint_motion_compare.cpp',
  'release':'src/physics/native_tu15_constraint_reference_release.cpp','slots':'src/physics/native_tu15_constraint_body_slots.cpp',
  'cache':'src/physics/native_tu15_constraint_scalar_cache.cpp'}.items()}
 production_parent=units['parent']
 if a.parent_source:units['parent']=a.parent_source.resolve()
 names=['body_constraint_update','geometry','data_array_allocate','model_constraint_lookup','model_constraint_capacity','model_constraint_creation',
  'constraint_local_points','camera_projection','vector_normalize','constraint_motion_compare','constraint_reference_release','constraint_body_slots','constraint_scalar_cache','physics_frame']
 files=[production_parent,*units.values(),*[ROOT/'include/halo2'/('native_tu15_'+n+'.hpp')for n in names],Path(__file__),
  ROOT/'tools/observe_native_body_constraint_actual_before_tu15.cpp',ROOT/'tools/observe_native_activation_agents_tu15.cpp',
  ROOT/'tools/build_native_tu15_constraint_before_reference.py',ROOT/'tools/census_native_constraint_component_before_tu15.py']
 pins={str(f.relative_to(ROOT)):sha(f)for f in files}
 try:
  source=BeforeInput(a.capture,a.archive_sha256,scratch/'before');identity=census(source);save(a.output/'input-census.json',identity)
  reference=build_constraint_reference(source,scratch/'reference');save(a.output/'reference.json',reference)
  from capstone import CS_AC_READ,CS_AC_WRITE
  from capstone.x86 import X86_OP_MEM
  assert sha(NEW_DEFAULT)==NEW_PIN;code=_Code(Xbe(NEW_DEFAULT),'.text');metadata={};counts={};hashes={}
  owners=[0x22d460,0x1d9ec0,0x315c90,0x1f3460,0xde8c0,0x1d8b90,0x2e4650,0x2e44d0,0x20d8b0,
   0x1dd710,0x22def0,0x22e040,0x22e000,0x22e0d0,0x230f60,0x1dd7b0,0x22dd90,
   0x22cea0,0x16e6a0,0x122b60,0x22ede0,0x22eba0,0x144090,0x22e1b0,0x22e390,0x22fe30,0x20d940]
  for owner in owners:
   body=complete_body(code,owner);counts[hex(owner)]=len(body.code);listing=[]
   for pc in sorted(body.code):
    i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');operands=[]
    if i.mnemonic!='lea':
     for op in i.operands:
      if op.type!=X86_OP_MEM or op.mem.segment:continue
      operands.append(dict(base=i.reg_name(op.mem.base)or'',index=i.reg_name(op.mem.index)or'',scale=op.mem.scale,
       displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(op.access&CS_AC_READ)or i.mnemonic=='test',write=bool(op.access&CS_AC_WRITE)))
    if i.mnemonic in('rep stosd','rep stosb'):operands=[dict(base='edi',index='',scale=1,displacement=0,bytes=4 if i.mnemonic.endswith('stosd')else 1,read=False,write=True)]
    metadata[str(pc)]=dict(bytes=i.bytes.hex(),size=i.size,memory=operands,repeat_count=i.mnemonic in('rep stosd','rep stosb'))
   hashes[hex(owner)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
  save(scratch/'instructions.json',metadata)
  prefix=(ROOT/'tools/observe_native_activation_agents_tu15.cpp').read_text().split('int main(int argc,char**argv)')[0]
  prefix=prefix.replace('check(++steps<4096','check(++steps<32768')
  prefix=prefix.replace('for(const auto&m:i.at("memory")){const auto p=','for(const auto&m:i.at("memory")){if(i.value("repeat_count",false)&&std::uint32_t(r.rcx)==0u)continue;const auto p=')
  driver=scratch/'driver.cpp';driver.write_text('#include "halo2/native_tu15_model_constraint_creation.hpp"\n#include "halo2/native_tu15_model_constraint_lookup.hpp"\n#include "halo2/native_tu15_model_constraint_capacity.hpp"\n'+prefix+(ROOT/'tools/observe_native_body_constraint_actual_before_tu15.cpp').read_text())
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
  for name,path in units.items():shutil.copyfile(path,project/(name+'.cpp'))
  parent=units['parent'].read_text();needle='word(component+4u)&0x800u';assert parent.count(needle)==1
  (project/'wrong_flag.cpp').write_text(parent.replace(needle,'word(component+4u)&0x400u'))
  (project/'negative.py').write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nj=json.load(open(sys.argv[-1]));assert r.returncode!=0 and not j.get("error")and any(not x["ordered_operands_equal"]for x in j["cases"]),(r.returncode,j)\nprint("Original22D460 rejects wrong native positive-loop gate")\n')
  cmake=f'''cmake_minimum_required(VERSION 3.20)
project(actual_body_constraint LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json REQUIRED)
add_library(driver OBJECT "{driver}")
target_include_directories(driver PRIVATE include)
target_compile_features(driver PRIVATE cxx_std_20)
target_link_libraries(driver PRIVATE nlohmann_json::nlohmann_json)
add_library(children OBJECT {' '.join(n+'.cpp'for n in units if n!='parent')})
target_include_directories(children PRIVATE include)
target_compile_features(children PRIVATE cxx_std_20)
target_compile_options(children PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
'''
  for variant in ['native','wrong_flag']:
   cpp='parent.cpp'if variant=='native'else'wrong_flag.cpp';name='constraint_'+variant
   cmake+=f'add_executable({name} $<TARGET_OBJECTS:driver> $<TARGET_OBJECTS:children> {cpp})\ntarget_include_directories({name} PRIVATE include)\ntarget_compile_features({name} PRIVATE cxx_std_20)\ntarget_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
   command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '+(''if variant=='native'else f'"{sys.executable}" "{project}/negative.py" ')+f'$<TARGET_FILE:{name}> "{reference["elf"]}" "{source.path}/capture-native-before.json" "{scratch}/instructions.json" "${{CMAKE_BINARY_DIR}}/{variant}.json"'
   cmake+=f'add_test(NAME {name} COMMAND {command})\nset_tests_properties({name} PROPERTIES TIMEOUT 240)\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in a.compilers:
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),
    '--target','constraint_native','--target','constraint_wrong_flag','--filter','^constraint_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,
    '--jobs','1','--test-jobs','1','--retain-report','native.json','--retain-report','wrong_flag.json'];result=subprocess.run(command)
   results.append(dict(compiler=compiler,exit_code=result.returncode,command=command))
  unchanged=all(sha(ROOT/f)==value for f,value in pins.items())
  report.update(passed=all(r['exit_code']==0 for r in results)and unchanged,current_sources_unchanged=unchanged,source_hashes=pins,
   compiler_results=results,reference=reference,body_sites=counts,listing_sha256=hashes,original_sha256=NEW_PIN,
   original22D460_caller_captured=False,genuine_frame_level_component=True,declared_x87_depths=[0,1,2,3],declared_rounding_modes=[0,1,2,3],
   captured_after_consumed=False,supplied_original_outcomes=False,native_parent_falls_back=False,native_services_execute_original_CPU=False,
   remaining=['Actual22D460 invocation boundary and live positive native parent activation','Saturated capacity/removal and other model rows','Fault/alias/mutable-observer domains'])
 finally:
  report['temporary_allocated_bytes_before_cleanup']=sum(f.stat().st_blocks*512 for f in scratch.rglob('*')if f.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
