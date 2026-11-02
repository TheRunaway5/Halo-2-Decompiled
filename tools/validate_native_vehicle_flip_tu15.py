#!/usr/bin/env python3
"""Native F1DD0 versus physical TU1.5 with actual Warthog-world input data.

Positive controller/basis cases are declared BEFORE input variants, not a live
rollover. The first cohort stops at the real B9800 boundary without a result.
"""
import argparse,hashlib,json,os,re,shutil,subprocess,sys,tarfile
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from validate_native_contact_agent_dispatch_tu15 import sha,save
from validate_native_body_constraint_update_tu15 import ARCHIVE,ARCHIVE_SHA
from build_native_tu15_before_reference import build_before_reference
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);p.add_argument('--case',default='.*');p.add_argument('--compose-setter',action='store_true');p.add_argument('--with-live-depths',action='store_true');p.add_argument('--handoff',action='store_true');a=p.parse_args();a.compose_setter|=a.handoff;a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[]);units=['vehicle_flip','object_motion','island_activation']+(['vehicle_flip_binding','activation_agents','activation_services','contact_agent_factory','array_allocator','array_growth','object_activation_residuals','body_policy','shape_bounds','centered_shape_bounds','compound_shape_bounds','sphere_shape_bounds','geometry','activation_payload_filter','activation_contact_manager','body_constraint_update','model_constraint_lookup','model_constraint_capacity']if a.compose_setter else[])+(['vehicle_flip_handoff']if a.handoff else[]);observer=ROOT/'tools/observe_native_vehicle_flip_tu15.cpp';units+=['gameplay_policy_destruction']if 'body_policy'in units else[];files=[ROOT/f'src/physics/native_tu15_{name}.cpp'for name in units]+[ROOT/f'include/halo2/native_tu15_{name}.hpp'for name in units]+[observer,ROOT/'tools/observe_native_activation_agents_tu15.cpp',ROOT/'tools/build_native_tu15_before_reference.py',Path(__file__)];files+=([ROOT/'src/core/native_tu15_data_array_allocate.cpp',ROOT/'include/halo2/native_tu15_data_array_allocate.hpp',ROOT/'src/physics/native_tu15_contact_unwind_internal.hpp']if a.compose_setter else[]);files+=([ROOT/'src/physics/vehicles/physics_vehicle_runtime_internal.hpp',ROOT/'src/physics/runtime/physics_world_runtime_internal.hpp',ROOT/'src/physics/runtime/physics_contact_runtime_internal.hpp',ROOT/'analysis/physics/native-vehicle-flip-live-handoff-20261101/staged/src/physics/vehicles/physics_vehicle_runtime_generated.cpp',ROOT/'analysis/physics/native-vehicle-flip-live-handoff-20261101/staged/tools/generate_vehicle_runtime_20261011.py']if a.handoff else[]);pins={str(f.relative_to(ROOT)):sha(f)for f in files}
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN and sha(ARCHIVE)==ARCHIVE_SHA;capture=scratch/'actual-before';capture.mkdir()
  with tarfile.open(ARCHIVE,'r:*')as stream:
   for member in stream.getmembers():assert(capture/member.name).resolve().is_relative_to(capture.resolve())and(member.isfile()or member.isdir())
   stream.extractall(capture,filter='data')
  loader=build_before_reference(capture,scratch/'reference');code=_Code(Xbe(NEW_DEFAULT),'.text');metadata={};listing=[]
  from capstone import CS_AC_READ,CS_AC_WRITE
  from capstone.x86 import X86_OP_MEM
  for pc in sorted(complete_body(code,0xf1dd0).code):
   i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');operands=[]
   if i.mnemonic!='lea':
    for op in i.operands:
     if op.type!=X86_OP_MEM or op.mem.segment:continue
     operands.append(dict(base=i.reg_name(op.mem.base)or'',index=i.reg_name(op.mem.index)or'',scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(op.access&CS_AC_READ)or i.mnemonic=='test',write=bool(op.access&CS_AC_WRITE)))
   metadata[str(pc)]=dict(bytes=i.bytes.hex(),size=i.size,memory=operands)
  metadata[str(0xf1e96)].update(service_arguments=1,ecx_consumed=True,result_consumed=False);metadata[str(0xf2091)].update(**(dict(service_arguments=1,ecx_consumed=False,result_consumed=False)if a.compose_setter else dict(stop_before=True)));instructions=scratch/'instructions.json';save(instructions,metadata)
  prefix=(ROOT/'tools/observe_native_activation_agents_tu15.cpp').read_text().split('int main(int argc,char**argv)')[0].replace('const auto&i=metadata.at(key);','const auto&i=metadata.at(key);if(i.value("stop_before",false))break;') ;prefix=prefix.replace('calls.push_back(event);','if(std::uint32_t(entered.rip)==0xbc200u){event["linear_output"]=activation_hex(child.read(std::uint32_t(entered.rcx),12));event["angular_output"]=activation_hex(child.read(child.word(std::uint32_t(entered.rsp)+4u),12));}if(std::uint32_t(entered.rip)==0xb9800u){event["linear_input"]=activation_hex(child.read(child.word(std::uint32_t(entered.rsp)+4u),12));event["angular_input"]=activation_hex(child.read(std::uint32_t(entered.rax),12));}calls.push_back(event);');driver=scratch/'driver.cpp';
  if a.handoff:
   generated=(ROOT/'analysis/physics/native-vehicle-flip-live-handoff-20261101/staged/src/physics/vehicles/physics_vehicle_runtime_generated.cpp').read_text();start=generated.index('static bool handoff_native_vehicle_flip(VehicleState&s) {');end=generated.index('\n}',start)+2;helper=generated[start:end];prefix='#include \"physics/vehicles/physics_vehicle_runtime_internal.hpp\"\n#include \"halo2/native_tu15_vehicle_flip_handoff.hpp\"\nusing halo2::physics_vehicle_runtime_detail::VehicleState;\n'+prefix+helper+'\n';prefix=prefix.replace('!=0x1ccacau','!=0xf24ddu')
  driver.write_text('#define VEHICLE_FLIP_HANDOFF '+str(int(a.handoff))+'\n#define VEHICLE_FLIP_COMPOSED '+str(int(a.compose_setter))+'\n#include "halo2/native_tu15_vehicle_flip_binding.hpp"\n#include "halo2/native_tu15_vehicle_flip.hpp"\n'+prefix+observer.read_text());project=scratch/'project';project.mkdir()
  for name in units:shutil.copyfile(ROOT/f'src/physics/native_tu15_{name}.cpp',project/(name+'.cpp'))
  if a.compose_setter:shutil.copyfile(ROOT/'src/core/native_tu15_data_array_allocate.cpp',project/'data_array_allocate.cpp');shutil.copyfile(ROOT/'src/physics/native_tu15_contact_unwind_internal.hpp',project/'native_tu15_contact_unwind_internal.hpp')
  objects=[0xe2a10032,0xe2a60037,0xe2ab003c,0xe2ae003f];cases=[dict(name=f'{obj:x}_{branch}_{mode}'+(f'_d{depth}'if a.with_live_depths else''),object=obj,branch=branch,mode=mode,depth=depth)for obj in objects for branch in range(13)for mode in range(4)for depth in(range(6)if a.with_live_depths else[0])if(depth==0 or(branch in range(5,9)and mode==0))and re.fullmatch(a.case,f'{obj:x}_{branch}_{mode}'+(f'_d{depth}'if a.with_live_depths else''))];assert cases
  text=(project/'vehicle_flip.cpp').read_text();needle='x=mul(x,gain);';assert text.count(needle)==1;variants={'wrong_angular_axis':text.replace(needle,'x=mul(y,gain);')};negative=next((c for c in cases if c['branch']==5 and c['mode']==0 and c['depth']==0),None)
  if negative is None:variants={}
  for name,text in variants.items():(project/(name+'.cpp')).write_text(text)
  (project/'negative.py').write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True);j=json.load(open(sys.argv[-1]));assert r.returncode!=0 and not j.get("error") and(j.get("setter_vectors_equal")is False or not j.get("fp_equal",True)or j.get("different_bytes",0)),(r.returncode,j)\nprint("Actual F1DD0 rejects wrong native angular axis")\n')
  cmake=f'''cmake_minimum_required(VERSION 3.20)
project(NativeVehicleFlip LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
add_library(driver OBJECT "{driver}")
add_library(dependencies OBJECT {' '.join(name+'.cpp'for name in units if name!='vehicle_flip')}{' data_array_allocate.cpp'if a.compose_setter else''})
foreach(t driver dependencies)
 target_include_directories(${{t}} PRIVATE "{ROOT}/include" "{ROOT}/src")
 target_compile_features(${{t}} PRIVATE cxx_std_20)
 target_compile_options(${{t}} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
endforeach()
target_link_libraries(driver PRIVATE nlohmann_json::nlohmann_json)
file(MAKE_DIRECTORY "${{CMAKE_BINARY_DIR}}/evidence")
''';retained=[]
  for variant in ['native',*variants]:
   file='vehicle_flip.cpp'if variant=='native'else variant+'.cpp';cmake+=f'add_executable(flip_{variant} $<TARGET_OBJECTS:driver> $<TARGET_OBJECTS:dependencies> {file})\ntarget_include_directories(flip_{variant} PRIVATE "{ROOT}/include")\ntarget_compile_features(flip_{variant} PRIVATE cxx_std_20)\ntarget_compile_options(flip_{variant} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
   for case in cases if variant=='native'else[negative]:
    name=variant+'_'+case['name'];output='evidence/'+name+'.json';retained.append(output);command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '+(''if variant=='native'else f'"{sys.executable}" "{project}/negative.py" ')+f'$<TARGET_FILE:flip_{variant}> "{loader["elf"]}" "{capture}/capture.json" {case["object"]} {case["branch"]} {case["mode"]+case["depth"]*4} "{instructions}" "${{CMAKE_BINARY_DIR}}/{output}"';cmake+=f'add_test(NAME flip_{name} COMMAND {command})\nset_tests_properties(flip_{name} PROPERTIES TIMEOUT 60)\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in a.compilers:
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','flip_native',*sum((['--target','flip_'+name]for name in variants),[]),'--filter','^flip_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1',*sum((['--retain-report',path]for path in retained),[])];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  unchanged=all(sha(ROOT/f)==h for f,h in pins.items());report.update(passed=all(r['exit_code']==0 for r in results)and unchanged,source_hashes=pins,current_sources_unchanged=unchanged,compiler_results=results,physical_cases_per_compiler=len(cases),negative_controls=list(variants),original_sha256=NEW_PIN,source_listing_sha256=hashlib.sha256(''.join(listing).encode()).hexdigest(),authority_sites=len(listing),loader=loader,archive=dict(path=str(ARCHIVE),sha256=ARCHIVE_SHA),genuine_rollover_qualified=False,declared_controller_operand_cases=True,native_setter_composed=a.compose_setter,generated_F24D8_handoff=a.handoff,live_depths=list(range(6))if a.with_live_depths else[0],remaining=['Full direct native B9800 composition','Actual native caller AL/FP boundary and visible source-authored multiplayer rollover','Fault/observer/alias domains'])
 except BaseException as error:report['error']=str(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
