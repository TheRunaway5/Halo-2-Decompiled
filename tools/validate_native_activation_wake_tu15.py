#!/usr/bin/env python3
"""Compose the native TU1.5 wake family on genuine sleeping-island inputs.

The same semantic282870/316200 parent is used inside native315CB0/316D40/
3165D0/322660. Remaining virtual services execute authentic original bodies.
No original outcome, private GP/SSE image or instruction patch is provided.
"""
import argparse,hashlib,json,os,re,shutil,subprocess,sys
from pathlib import Path
import validate_native_activation_agents_tu15 as agents
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from validate_native_contact_agent_dispatch_tu15 import sha,save

def replace_once(text,old,new):
 assert text.count(old)==1,old;return text.replace(old,new)

def driver_source():
 text=(ROOT/'tools/observe_native_activation_agents_tu15.cpp').read_text()
 text='#include "halo2/native_tu15_island_activation.hpp"\n'+text
 text=replace_once(text,'const auto world=original.word(0x52be48u),islands=original.word(world+8u);check(ordinal<original.word(world+12u),"Activation actual island ordinal unavailable");','const auto world=original.word(0x52be48u),islands=original.word(world+0x14u);check(ordinal<original.word(world+0x18u),"Activation actual sleeping island ordinal unavailable");')
 text=replace_once(text,'check(count>0&&count<=16,"Actual activation contact cohort unavailable");','check(count<=16,"Actual activation contact cohort unavailable");const auto body=original.word(original.word(island+0x3cu));')
 text=replace_once(text,'root_entry.rip=0x282870;root_entry.rsp=sp;','root_entry.rip=0x315cb0;root_entry.rcx=body;root_entry.rsp=sp;')
 text=replace_once(text,'ActivationBridge bridge{native,sp-40u,sp-32u,initial_fp};','ActivationBridge bridge{native,sp-100u,sp-92u,initial_fp};')
 old='NativeTu15PhysicsFrameFp fp{before.cwd,before.swd,before.mxcsr};NativeTu15ActivationAgentStats stats;native_tu15_recreate_island_agents(memory,{island},{sp+4u,sp-21u},fp,services,{},&stats);'
 new='''NativeTu15PhysicsFrameFp fp{before.cwd,before.swd,before.mxcsr};NativeTu15ActivationAgentStats stats;NativeTu15IslandActivationStats wake_stats;NativeTu15IslandActivationServices wake;ActivationBridge notifications{native,sp-52u,sp-52u,initial_fp};
 wake.grow_array=[&](auto,auto,auto&){throw std::runtime_error("Actual wake cohort requires additional active-array growth ownership");};
 const auto publish_notifications=[&]{for(const auto&call:notifications.calls)bridge.calls.push_back(call);notifications.calls.clear();};
 wake.island_activated=[&](auto listener,auto owner,auto current,auto&state){check(owner.identity==0x282870u,"Actual wake cohort requires an additional world activation listener");native_tu15_recreate_island_agents(memory,current,{sp-56u,sp-81u},state,services,{},&stats);};
 wake.body_activated=[&](auto listener,auto owner,auto current,auto&state){notifications.call(owner.identity,listener.identity,{current.identity},0x3226f5u,state);publish_notifications();};
 wake.destroy_body=[&](auto,auto,auto,auto&){throw std::runtime_error("Actual wake cohort requires additional deferred-body destruction ownership");};
 native_tu15_activate_physics_body(memory,{body},fp,wake,{},&wake_stats);
 report.update(J{{"native_wake_body_calls",wake_stats.body_calls},{"native_wake_island_calls",wake_stats.island_calls},{"native_wake_island_notifications",wake_stats.island_notifications},{"native_wake_body_notifications",wake_stats.body_notifications},{"native_wake_deferred_removals",wake_stats.deferred_removals}});'''
 text=replace_once(text,old,new)
 text=replace_once(text,'if(call.at("entry")!=0x316200u)residual_calls.push_back(call);','if(call.at("entry")!=0x316200u&&call.at("entry")!=0x2e4d60u)residual_calls.push_back(call);')
 return text

def metadata():
 from capstone import CS_AC_READ,CS_AC_WRITE
 from capstone.x86 import X86_OP_MEM
 rows,counts,hashes=agents.metadata();code=_Code(Xbe(NEW_DEFAULT),'.text')
 for entry in(0x315cb0,0x316d40,0x3165d0,0x322660,0x283450,0x2e4d60):
  body=complete_body(code,entry);counts[hex(entry)]=len(body.code);listing=[]
  for pc in sorted(body.code):
   i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');operands=[]
   if i.mnemonic!='lea':
    for op in i.operands:
     if op.type!=X86_OP_MEM:continue
     assert not op.mem.segment
     operands.append(dict(base=i.reg_name(op.mem.base)or'',index=i.reg_name(op.mem.index)or'',scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(op.access&CS_AC_READ),write=bool(op.access&CS_AC_WRITE)))
   row=dict(bytes=i.bytes.hex(),size=i.size,memory=operands)
   if pc==0x3226f2:row.update(service_arguments=1,ecx_consumed=True,result_consumed=False)
   rows[str(pc)]=row
  hashes[hex(entry)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
 return rows,counts,hashes

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);p.add_argument('--case',default='.*');a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 files=[ROOT/'src/physics/native_tu15_activation_agents.cpp',ROOT/'include/halo2/native_tu15_activation_agents.hpp',ROOT/'src/physics/native_tu15_island_activation.cpp',ROOT/'include/halo2/native_tu15_island_activation.hpp',ROOT/'tools/observe_native_activation_agents_tu15.cpp',ROOT/'tools/validate_native_activation_agents_tu15.py',Path(__file__)];pins={str(f.relative_to(ROOT)):sha(f)for f in files};save(a.output/'report.json',report)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;driver=scratch/'driver.cpp';driver.write_text(driver_source());instructions,counts,hashes=metadata();instruction_file=scratch/'instructions.json';save(instruction_file,instructions);contexts=[];cases=[]
  base=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458'
  for name in('gemini-column0','gemini-column1'):
   if not any(re.fullmatch(a.case,f'{name}_{island}_{variation}')for island in range(13)for variation in(0,1,2,3)):continue
   capture=base/('iteration-04-'+name)/'original/required-failure-inputs';replay=scratch/('original-'+name);command=[sys.executable,str(ROOT/'tools/replay_live_phantom_hardware_tu15.py'),'--native-frame-semantics','--return-breakpoints','--capture',str(capture),'--output',str(replay)];r=subprocess.run(command,capture_output=True,text=True,timeout=180);assert(replay/'original-hardware').exists(),r.stderr[-2000:];contexts.append(dict(name=name,capture=str(capture),capture_sha256=sha(capture/'capture.json'),reference_command=command,ordinary_full_frame_reference_exit=r.returncode))
   for island in range(13):
    for variation in(0,1,2,3):
     if re.fullmatch(a.case,f'{name}_{island}_{variation}'):cases.append(dict(name=f'{name}_{island}_{variation}',elf=str(replay/'original-hardware'),capture=str(capture/'capture.json'),island=island,variation=variation))
  project=scratch/'project';project.mkdir();shutil.copyfile(files[0],project/'agents.cpp');shutil.copyfile(files[2],project/'native.cpp');text=files[2].read_text();variants={}
  for name,needle,replacement in(('wrong_active_index','const auto new_index=m.word(world.identity+0xcu)-1u;','const auto new_index=m.word(world.identity+0xcu);'),('wrong_active_flag','island.identity+0x29u,1,1','island.identity+0x29u,0,1')):
   assert text.count(needle)==1;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
  if not cases:raise RuntimeError('No actual sleeping-island case selected')
  (project/'negative.py').write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nj=json.load(open(sys.argv[-1]));assert r.returncode!=0 and not j.get("error") and (j.get("different_bytes",0)>0 or not j.get("ordered_retained_operands_equal",True)),(r.returncode,j)\nprint("Actual original wake rejects mutation")\n')
  cmake=f'''cmake_minimum_required(VERSION 3.20)
project(NativeActivationWake LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
add_library(driver OBJECT "{driver}")
add_library(agents OBJECT agents.cpp)
foreach(target driver agents)
 target_include_directories(${{target}} PRIVATE "{ROOT}/include")
 target_compile_features(${{target}} PRIVATE cxx_std_20)
 target_compile_options(${{target}} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
endforeach()
target_link_libraries(driver PRIVATE nlohmann_json::nlohmann_json)
file(MAKE_DIRECTORY "${{CMAKE_BINARY_DIR}}/evidence")
''';retained=[]
  for variant in('native',*variants):
   cmake+=f'add_executable(wake_{variant} $<TARGET_OBJECTS:driver> $<TARGET_OBJECTS:agents> {variant}.cpp)\ntarget_include_directories(wake_{variant} PRIVATE "{ROOT}/include")\ntarget_compile_features(wake_{variant} PRIVATE cxx_std_20)\n'
   for c in cases if variant=='native'else cases[:1]:
    name=variant+'_'+c['name'];output='evidence/'+name+'.json';retained.append(output);command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '+(''if variant=='native'else f'"{sys.executable}" "{project}/negative.py" ')+f'$<TARGET_FILE:wake_{variant}> "{c["elf"]}" "{c["capture"]}" {c["island"]} {c["variation"]} "{instruction_file}" "${{CMAKE_BINARY_DIR}}/{output}"';cmake+=f'add_test(NAME wake_{name} COMMAND {command})\nset_tests_properties(wake_{name} PROPERTIES TIMEOUT 45)\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in a.compilers:
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','wake_native',*sum((['--target','wake_'+name]for name in variants),[]),'--filter','^wake_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1',*sum((['--retain-report',path]for path in retained),[])];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  unchanged=all(sha(ROOT/f)==v for f,v in pins.items());report.update(passed=all(r['exit_code']==0 for r in results)and unchanged,compiler_results=results,contexts=contexts,physical_cases_per_compiler=len(cases),body_sites=counts,listing_sha256=hashes,negative_controls=list(variants),source_hashes=pins,current_sources_unchanged=unchanged,image_sha256=NEW_PIN,supplied_original_outcomes=False,instruction_patches=[],native_parent_receives_guest_register_state=False,production_native_only_closure=False,remaining=['native remaining factory/filter/policy/motion/discrete/body-listener/destructor service owners','active-array growth and deferred-reference destructive composition','fault/mutable-callback/alias and unmasked platform qualification','actual visible multiplayer activation/wake integration'])
 except BaseException as error:report['error']=str(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
