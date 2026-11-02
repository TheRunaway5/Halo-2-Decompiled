#!/usr/bin/env python3
"""Compose native activation filters/types/factories/object wake with TU1.5.

Genuine original services remain only for owners explicitly outside the new
native closure. Every expected result comes from independent original work.
"""
import argparse,hashlib,json,os,re,shutil,subprocess,sys
from pathlib import Path
import validate_native_activation_agents_tu15 as agents
import validate_native_activation_wake_tu15 as wake
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from validate_native_contact_agent_dispatch_tu15 import sha,save
replace_once=wake.replace_once
INLINED=(0xa9520,0xab4b0,0xadb50,0xad1d0,0x2e8cf0,0x31dd70,0x31edf0,0xad740,0x31d520,0x303c10,0x3040d0)
OWNERS=(*INLINED,0x2e9a00,0x281530,0x2816a0,0xb9390,0x2f3620,0x149da0,0x149e00,0x233250,0x233420,0x2f0380)

def driver_source(mode):
 text=wake.driver_source()if mode=='wake'else(ROOT/'tools/observe_native_activation_agents_tu15.cpp').read_text()
 text='#include "halo2/native_tu15_activation_services.hpp"\n#include "halo2/native_tu15_contact_agent_factory.hpp"\n#include "halo2/native_tu15_array_allocator.hpp"\n'+text
 predicate='bool native_activation_service_owner(std::uint32_t owner){switch(owner){'+''.join(f'case 0x{owner:x}u:'for owner in INLINED)+'return true;default:return false;}}\n'
 text=replace_once(text,'J activation_trace(',predicate+'J activation_trace(')
 text=replace_once(text,'child.step();const auto entered=child.regs();const auto returned=','child.step();const auto entered=child.regs();if(native_activation_service_owner(std::uint32_t(entered.rip)))continue;const auto returned=')
 needle='NativeTu15ActivationAgentServices services;unsigned classifications{},motion_queries{};'
 insert='''NativeTu15ActivationAgentServices services;unsigned classifications{},motion_queries{};NativeTu15ActivationServiceStats native_services;unsigned native_factories{};NativeTu15ArrayAllocatorMemory allocator_memory(memory);NativeTu15ArrayAllocatorServices allocator_services;
 allocator_services.allocate_owner=[&](const auto&request,auto&){check(request.owner==0x233250u,"Activation factory requires an additional allocator owner");NativeTu15PoolMemory pool(memory,request.receiver);return native_tu15_pool_allocate(pool,request.bytes);};
 allocator_services.release_owner=[&](const auto&request,auto&){check(request.owner==0x233420u,"Activation factory requires an additional release owner");NativeTu15PoolMemory pool(memory,request.receiver);check(native_tu15_pool_release(pool,request.data,request.bytes),"Activation factory requires large allocation release ownership");};
 NativeTu15ArrayGrowthServices allocation;
 allocation.allocate=[&](const auto&request,auto&state){check(request.owner==0x149da0u,"Activation factory requires native149DA0 routing");return native_tu15_route_array_allocation(allocator_memory,request.bytes,request.memory_class,state,allocator_services);};
 allocation.release=[&](const auto&request,auto&state){check(request.owner==0x149e00u,"Activation factory requires native149E00 routing");native_tu15_route_array_release(allocator_memory,request.data,request.bytes,request.memory_class,state,allocator_services);};
'''
 text=replace_once(text,needle,insert)
 text=replace_once(text,'services.filter_pair=[&](auto receiver,auto owner,auto output,auto first,auto second,auto&fp){return bridge.call(', 'services.filter_pair=[&](auto receiver,auto owner,auto output,auto first,auto second,auto&fp){if(owner.identity==0x2e9a00u)return native_tu15_activation_group_filter(memory,receiver,output,first,second,{},&native_services);return bridge.call(')
 text=replace_once(text,'services.classify_shape=[&](auto shape,auto owner,auto&fp){return bridge.call(', 'services.classify_shape=[&](auto shape,auto owner,auto&fp){if(owner.identity==0xa9520u)return native_tu15_contact_child_class_23();if(owner.identity==0xab4b0u)return native_tu15_contact_child_class_24();if(const auto value=native_tu15_activation_type(owner.identity))return *value;return bridge.call(')
 text=replace_once(text,'services.create_agent=[&](auto owner,auto first,auto second,auto context,auto manager,auto&fp){return NativeTu15ContactAgent{bridge.call(', 'services.create_agent=[&](auto owner,auto first,auto second,auto context,auto manager,auto&fp){if(owner.identity==0x303c10u||owner.identity==0x3040d0u){++native_factories;return native_tu15_create_contact_agent(memory,owner.identity==0x303c10u?NativeTu15ContactAgentFactory::owner_303c10:NativeTu15ContactAgentFactory::owner_3040d0,NativeTu15ContactAgentFactoryArguments{[&]{return manager.identity;}},fp,allocation);}return NativeTu15ContactAgent{bridge.call(')
 text=replace_once(text,'services.motion_kind=[&](auto motion,auto owner,auto&fp){const auto returned=', 'services.motion_kind=[&](auto motion,auto owner,auto&fp){if(const auto value=native_tu15_activation_type(owner.identity))return *value;const auto returned=')
 text=replace_once(text,'services.body_policy_kind=[&](auto policy,auto owner,auto&fp){return bridge.call(', 'services.body_policy_kind=[&](auto policy,auto owner,auto&fp){if(const auto value=native_tu15_activation_type(owner.identity))return *value;return bridge.call(')
 old='services.test_discrete_pair=[&](auto dispatcher,auto owner,auto output,auto first,auto second,auto&fp){motion_queries=0;return bridge.call(owner.identity,dispatcher,{output,first.identity,second.identity},0x282a1au,fp);};'
 base='sp-116u'if mode=='wake'else'sp-56u'
 new=f'''ActivationBridge discrete_filter{{native,{base},{base},initial_fp}};
 services.test_discrete_pair=[&](auto dispatcher,auto owner,auto output,auto first,auto second,auto&fp){{motion_queries=0;if(owner.identity==0x281530u){{NativeTu15ActivationDiscreteServices query;query.test_filter=[&](auto filter,auto target,auto a,auto b,auto&state){{const auto value=discrete_filter.call(target.identity,filter,{{a.identity,b.identity}},0x281542u,state);for(const auto&call:discrete_filter.calls)bridge.calls.push_back(call);discrete_filter.calls.clear();return value;}};return native_tu15_activation_discrete_pair(memory,dispatcher,output,first,second,fp,query,{{}},&native_services);}}return bridge.call(owner.identity,dispatcher,{{output,first.identity,second.identity}},0x282a1au,fp);}};'''
 text=replace_once(text,old,new)
 if mode=='wake':
  text=replace_once(text,'const auto put=[&](std::uint32_t p,std::uint32_t value){original.write(p,{reinterpret_cast<const std::byte*>(&value),4});};','''const auto put=[&](std::uint32_t p,std::uint32_t value){original.write(p,{reinterpret_cast<const std::byte*>(&value),4});};
  {const auto properties=original.word(body+0x30u),property_count=original.word(body+0x34u);for(unsigned n=0;n<property_count;++n)if(original.word(properties+n*8u)==0x2001u){const auto datum=original.word(properties+n*8u+4u);if(datum!=0xffffffffu){const auto manager=original.word(0x52be5cu),component=original.word(manager+0x44u)+(datum&0xffffu)*160u,object=original.word(component+8u),pool=original.word(0x4ecbd0u),header=original.word(pool+0x44u)+(object&0xffffu)*12u+2u;const auto flags=original.read(header,1);const std::array dirty_clear{std::byte(std::to_integer<unsigned>(flags[0])&~2u)};original.write(header,dirty_clear);}break;}report["declared_object_dirty_flag_clear"]=true;}
''')
  old='wake.body_activated=[&](auto listener,auto owner,auto current,auto&state){notifications.call(owner.identity,listener.identity,{current.identity},0x3226f5u,state);publish_notifications();};'
  new='''ActivationBridge body_updates{native,sp-64u,sp-64u,initial_fp};
 wake.body_activated=[&](auto listener,auto owner,auto current,auto&state){check(owner.identity==0x2816a0u,"Activation body listener requires an additional native owner");NativeTu15ActivationBodyServices body_services;body_services.publish_body_update=[&](auto datum,auto&context){body_updates.call(0x22d460u,0,{datum},0x281700u,context);for(const auto&call:body_updates.calls)bridge.calls.push_back(call);body_updates.calls.clear();};native_tu15_body_activation_listener(memory,current,state,body_services,{},&native_services);};'''
  text=replace_once(text,old,new)
 text=replace_once(text,'//316200 is fully native;', 'report.update(J{{"native_group_filter_calls",native_services.filter_calls},{"native_discrete_calls",native_services.discrete_calls},{"native_agent_factory_calls",native_factories},{"native_body_listener_calls",native_services.body_listener_calls},{"native_object_wake_calls",native_services.object_wake_calls},{"native_object_headers_marked",native_services.object_headers_marked},{"native_body_updates",native_services.body_updates}});\n //316200 is fully native;')
 return text

def metadata(mode):
 from capstone import CS_AC_READ,CS_AC_WRITE
 from capstone.x86 import X86_OP_MEM
 rows,counts,hashes=wake.metadata()if mode=='wake'else agents.metadata();code=_Code(Xbe(NEW_DEFAULT),'.text')
 for pc in(0x2828ef,0x282a17,0x3226f2):
  if str(pc)in rows:
   for key in('service_arguments','ecx_consumed','result_consumed'):rows[str(pc)].pop(key,None)
 for entry in OWNERS:
  body=complete_body(code,entry);counts[hex(entry)]=len(body.code);listing=[]
  for pc in sorted(body.code):
   i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');operands=[]
   if i.mnemonic!='lea':
    for op in i.operands:
     if op.type!=X86_OP_MEM or op.mem.segment:continue
     operands.append(dict(base=i.reg_name(op.mem.base)or'',index=i.reg_name(op.mem.index)or'',scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(op.access&CS_AC_READ),write=bool(op.access&CS_AC_WRITE)))
   row=dict(bytes=i.bytes.hex(),size=i.size,memory=operands)
   if pc==0x28153f:row.update(service_arguments=2,ecx_consumed=True,result_consumed=True)
   if pc==0x2816fb:row.update(service_arguments=1,ecx_consumed=False,result_consumed=False)
   rows[str(pc)]=row
  hashes[hex(entry)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
 return rows,counts,hashes

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--mode',choices=['agents','wake'],default='agents');p.add_argument('--compilers',nargs='+',default=['g++','clang++']);p.add_argument('--case',default='.*');a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 units=['activation_agents','activation_services','contact_agent_factory','array_allocator']+(['island_activation']if a.mode=='wake'else[]);files=[ROOT/f'src/physics/native_tu15_{name}.cpp'for name in units]+[ROOT/f'include/halo2/native_tu15_{name}.hpp'for name in units]+[ROOT/'src/physics/native_tu15_contact_unwind_internal.hpp',ROOT/'include/halo2/native_tu15_array_growth.hpp',ROOT/'tools/observe_native_activation_agents_tu15.cpp',ROOT/'tools/validate_native_activation_agents_tu15.py',ROOT/'tools/validate_native_activation_wake_tu15.py',Path(__file__)];pins={str(f.relative_to(ROOT)):sha(f)for f in files};save(a.output/'report.json',report)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;driver=scratch/'driver.cpp';driver.write_text(driver_source(a.mode));instructions,counts,hashes=metadata(a.mode);instruction_file=scratch/'instructions.json';save(instruction_file,instructions);contexts=[];cases=[]
  base=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458'
  for name in('gemini-column0','gemini-column1')if a.mode=='wake'else('gemini-column0','gemini-column1','elongation-crates'):
   ordinals=range(13)if a.mode=='wake'else range(3 if name=='elongation-crates'else 2);variations=range(4)if a.mode=='wake'else range(16)
   if not any(re.fullmatch(a.case,f'{name}_{island}_{variation}')for island in ordinals for variation in variations):continue
   capture=base/('iteration-04-'+name)/'original/required-failure-inputs';replay=scratch/('original-'+name);command=[sys.executable,str(ROOT/'tools/replay_live_phantom_hardware_tu15.py'),'--native-frame-semantics','--return-breakpoints','--capture',str(capture),'--output',str(replay)];r=subprocess.run(command,capture_output=True,text=True,timeout=180);assert(replay/'original-hardware').exists(),r.stderr[-2000:];contexts.append(dict(name=name,capture=str(capture),capture_sha256=sha(capture/'capture.json'),reference_command=command,ordinary_full_frame_reference_exit=r.returncode))
   for island in ordinals:
    for variation in variations:
     if re.fullmatch(a.case,f'{name}_{island}_{variation}'):cases.append(dict(name=f'{name}_{island}_{variation}',elf=str(replay/'original-hardware'),capture=str(capture/'capture.json'),island=island,variation=variation))
  if not cases:raise RuntimeError('No actual activation case selected')
  project=scratch/'project';project.mkdir();
  for name in units:shutil.copyfile(ROOT/f'src/physics/native_tu15_{name}.cpp',project/(name+'.cpp'))
  shutil.copyfile(ROOT/'src/physics/native_tu15_contact_unwind_internal.hpp',project/'native_tu15_contact_unwind_internal.hpp')
  variants={};text=(ROOT/'src/physics/native_tu15_activation_services.cpp').read_text()
  for name,needle,replacement in(('wrong_filter_rejection','if(!((first_group^second_group)&0xffff0000u)&&(first_group&0xffff0000u))','if(true||(!((first_group^second_group)&0xffff0000u)&&(first_group&0xffff0000u)))'),('wrong_discrete_byte','accepted&255u,1','(accepted&255u)^1u,1'),('wrong_object_dirty_bit','m.word(header,1)|2u,1','m.word(header,1)|4u,1'),('wrong_shape_class','case 0xadb50u:return 20;','case 0xadb50u:return 19;')):
   assert text.count(needle)==1,name
   if name=='wrong_object_dirty_bit'and a.mode!='wake':continue
   variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
  negative_case=next((c for c in cases if c['variation']==1),None)if a.mode=='agents'else next((c for c in cases if c['island']==12 and c['variation']==1),None)
  if a.mode=='wake'and negative_case is None:
   negative_case=cases[0];variants={name:body for name,body in variants.items()if name=='wrong_object_dirty_bit'}
  if negative_case is None:variants={}
  (project/'negative.py').write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nj=json.load(open(sys.argv[-1]));assert r.returncode!=0 and (j.get("error")or j.get("different_bytes",0)>0 or not j.get("ordered_retained_operands_equal",True)or j.get("service_context_differences")),(r.returncode,j)\nprint("Actual original activation rejects native service mutation")\n')
  cmake=f'''cmake_minimum_required(VERSION 3.20)
project(NativeActivationServices LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
add_library(driver OBJECT "{driver}")
add_library(dependencies OBJECT {' '.join(name+'.cpp'for name in units if name!='activation_services')})
foreach(target driver dependencies)
 target_include_directories(${{target}} PRIVATE "{ROOT}/include")
 target_compile_features(${{target}} PRIVATE cxx_std_20)
 target_compile_options(${{target}} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
endforeach()
target_link_libraries(driver PRIVATE nlohmann_json::nlohmann_json)
file(MAKE_DIRECTORY "${{CMAKE_BINARY_DIR}}/evidence")
''';retained=[]
  for variant in('native',*variants):
   filename='activation_services.cpp'if variant=='native'else variant+'.cpp';cmake+=f'add_executable(services_{variant} $<TARGET_OBJECTS:driver> $<TARGET_OBJECTS:dependencies> {filename})\ntarget_include_directories(services_{variant} PRIVATE "{ROOT}/include")\ntarget_compile_features(services_{variant} PRIVATE cxx_std_20)\n'
   for c in cases if variant=='native'else[negative_case]:
    name=variant+'_'+c['name'];output='evidence/'+name+'.json';retained.append(output);command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '+(''if variant=='native'else f'"{sys.executable}" "{project}/negative.py" ')+f'$<TARGET_FILE:services_{variant}> "{c["elf"]}" "{c["capture"]}" {c["island"]} {c["variation"]} "{instruction_file}" "${{CMAKE_BINARY_DIR}}/{output}"';cmake+=f'add_test(NAME services_{name} COMMAND {command})\nset_tests_properties(services_{name} PROPERTIES TIMEOUT 45)\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in a.compilers:
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','services_native',*sum((['--target','services_'+name]for name in variants),[]),'--filter','^services_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1',*sum((['--retain-report',path]for path in retained),[])];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  unchanged=all(sha(ROOT/f)==v for f,v in pins.items());report.update(passed=all(r['exit_code']==0 for r in results)and unchanged,mode=a.mode,compiler_results=results,contexts=contexts,physical_cases_per_compiler=len(cases),body_sites=counts,listing_sha256=hashes,negative_controls=list(variants),source_hashes=pins,current_sources_unchanged=unchanged,image_sha256=NEW_PIN,supplied_original_outcomes=False,instruction_patches=[],native_parent_receives_guest_register_state=False,production_native_only_closure=False,remaining=['native remaining manager/agent factories,query predicate and body-policy lifetime/body update owners','fault/mutable-callback/alias and unmasked platform qualification','actual visible multiplayer activation/wake integration'])
 except BaseException as error:report['error']=str(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
