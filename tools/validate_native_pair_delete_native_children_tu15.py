#!/usr/bin/env python3
"""Discover/qualify only genuine native320260 cleanup owners; unknowns throw."""
import inspect,os,json,sys
from pathlib import Path
import validate_native_pair_delete_tu15 as parent
def main():
 assert os.environ.get('HALO2_TEST_SCRATCH');outer=inspect.getsource(parent.main)
 needle=' text=text.replace(needle,transform+needle)'
 assert outer.count(needle)==1
 old=''' services.destroy_agent=[&](auto agent,auto owner,auto&fp){call(owner.identity,agent.identity,{},0x32034eu,fp);};
 services.agents.release_manager=[&](auto manager,auto owner,auto&fp){call(owner.identity,manager.identity,{},0x320355u,fp);};'''
 new=''' Memory child_memory(native);J native_children=J::object(),unknown_children=J::object();NativeTu15AgentCleanupServices cleanup;
 NativeTu15PairDeletionBindingActivity binding;
 const auto record_native=[&](auto owner){native_children[std::to_string(owner)]=native_children.value(std::to_string(owner),0u)+1u;};
 const auto unknown=[&](auto owner){unknown_children[std::to_string(owner)]=unknown_children.value(std::to_string(owner),0u)+1u;throw std::runtime_error("Native pair-delete child requires owner="+std::to_string(owner));};
 cleanup.cleanup_child=[&](auto,auto owner,auto&){unknown(owner.identity);};
 NativeTu15ArrayAllocatorMemory router(child_memory);NativeTu15ArrayAllocatorServices heap;
 heap.allocate_owner=[&](const auto&r,auto&)->std::uint32_t{if(r.owner!=0x233250u)unknown(r.owner);NativeTu15PoolMemory pool(child_memory,r.receiver);return native_tu15_pool_allocate(pool,r.bytes);};
 heap.release_owner=[&](const auto&r,auto&){if(r.owner!=0x233420u)unknown(r.owner);record_native(r.owner);NativeTu15PoolMemory pool(child_memory,r.receiver);if(!native_tu15_pool_release(pool,r.data,r.bytes))unknown(r.owner);};
 heap.default_heap=[&](auto&)->std::uint32_t{unknown(0x149930u);return 0;};
 NativeTu15AgentCleanupChildrenServices children;children.agents=cleanup;
 children.allocation.allocate=[&](const auto&r,auto&fp)->std::uint32_t{if(r.owner!=0x149da0u)unknown(r.owner);return native_tu15_route_array_allocation(router,r.bytes,r.memory_class,fp,heap);};
 children.allocation.release=[&](const auto&r,auto&fp){if(r.owner!=0x149e00u)unknown(r.owner);record_native(r.owner);native_tu15_route_array_release(router,r.data,r.bytes,r.memory_class,fp,heap);};
 NativeTu15ContactManagerCleanupServices manager_services;manager_services.collector.allocation=children.allocation;
 manager_services.collector.destroy_body=[&](auto,auto owner,auto,auto&){unknown(owner.identity);};
 manager_services.collector.release_base_lock=[&](auto object,auto flags,auto&){if(flags)unknown(0x2e12a0u);native_tu15_release_unregistered_base_lock(child_memory,object);};
 manager_services.update_world=[&](auto,auto,auto,auto&){unknown(0x316aa0u);};
 cleanup.dispose=[&](auto object,auto owner,auto disposing,auto&fp){
   record_native(owner.identity);
   if(owner.identity==0x3051a0u){(void)native_tu15_destroy_contact_agent(child_memory,NativeTu15AgentDestructor::owner_3051a0,{object},disposing,fp,children);return;}
   if(owner.identity==0x3232b0u){(void)native_tu15_delete_contact_manager(child_memory,object,disposing,{0x4000100u},fp,manager_services);return;}
   unknown(owner.identity);
 };
 services.destroy_agent=[&](auto agent,auto owner,auto&fp){
   actual_calls.push_back(J{{"entry",owner.identity},{"arguments",J::array()},{"this",agent.identity}});
   if(owner.identity!=0x2f3100u)unknown(owner.identity);
   record_native(owner.identity);
   native_tu15_cleanup_contact_agent(child_memory,NativeTu15AgentCleanupKind::records_2f3100,agent,fp,cleanup);
 };
 services.agents.release_manager=[&](auto manager,auto owner,auto&fp){
   actual_calls.push_back(J{{"entry",owner.identity},{"arguments",J::array()},{"this",manager.identity}});
   if(owner.identity!=0x323220u)unknown(owner.identity);
   record_native(owner.identity);
   native_tu15_dispose_contact_reference(child_memory,manager.identity,fp,cleanup);
 };'''
 includes=''.join('#include "halo2/native_tu15_'+name+'.hpp"\n'for name in('agent_cleanup','agent_cleanup_children','array_allocator','contact_manager_cleanup','base_lock','pair_deletion_binding'))
 extra='driver='+repr(includes)+'+driver;assert driver.count('+repr(old)+')==1;driver=driver.replace('+repr(old)+','+repr(new)+');'
 extra+='driver=driver.replace("native_tu15_delete_pair(memory,handler,pair,{formal},fp,services,","native_tu15_delete_bound_pair(memory,handler,pair,{formal},fp,{services.destroy_agent,services.agents.release_manager},");'
 extra+='assert driver.count("}});});\\n std::size_t compared")==1;driver=driver.replace("}});});\\n std::size_t compared","}});},&binding);check(bridge.calls.empty(),\\\"Native pair deletion executed an original child\\\");\\n std::size_t compared");'
 extra+='driver=driver.replace("{\\\"remaining_services_execute_original_cpu\\\",true}","{\\\"remaining_services_execute_original_cpu\\\",false}").replace("{\\\"actual_original_child_entry\\\",true}","{\\\"actual_original_child_entry\\\",true}");'
 extra+='driver=driver.replace("{\\\"passed\\\",different==0", "{\\\"native_child_owners\\\",native_children},{\\\"unqualified_native_children\\\",unknown_children},{\\\"native_binding_attempts\\\",binding.attempts},{\\\"native_binding_completions\\\",binding.completed},{\\\"native_binding_refusals\\\",binding.refusals},{\\\"native_binding_unqualified_owners\\\",binding.unqualified_owners},{\\\"native_old_executor_calls\\\",bridge.calls.size()},{\\\"passed\\\",different==0");'
 addition='\n  '+extra+'\n  '
 outer=outer.replace(needle,' text=text.replace(needle,transform+'+repr(addition)+'+needle)')
 anchor=" env=dict(parent.__dict__);env['main']=None;exec(text,env);return env['main']()"
 extra_main=" text=text.replace(\"add_executable(pair_create driver.cpp native.cpp)\",\"add_executable(pair_create driver.cpp native.cpp native_agent_cleanup.cpp)\");text=text.replace(\"project/'native.cpp');driver=\",\"project/'native.cpp');shutil.copyfile(ROOT/'src/physics/native_tu15_agent_cleanup.cpp',project/'native_agent_cleanup.cpp');driver=\")\n"
 # The original copy has an intervening stage-header copy; insert at the stable project setup.
 units=['agent_cleanup','agent_cleanup_children','array_allocator','array_growth','contact_manager_cleanup','activation_contact_manager','island_activation']
 files=['src/physics/native_tu15_'+name+'.cpp'for name in units]
 extra_main=" text=text.replace(\"add_executable(pair_create driver.cpp native.cpp)\",\"add_executable(pair_create driver.cpp native.cpp "+' '.join(name+'.cpp'for name in units)+")\");text=text.replace(\"project.mkdir();shutil.copytree\",\"project.mkdir();"+''.join("shutil.copyfile(ROOT/"+repr(name)+",project/"+repr(unit+'.cpp')+");"for name,unit in zip(files,units))+"shutil.copyfile(ROOT/'src/physics/native_tu15_contact_unwind_internal.hpp',project/'native_tu15_contact_unwind_internal.hpp');shutil.copytree\")\n"
 extra_main+=" text=text.replace(\"'tools/validate_native_pair_create_tu15.py'\",\"'tools/validate_native_pair_create_tu15.py',"+','.join(repr(f)for f in files)+","+repr('tools/validate_native_pair_delete_native_children_tu15.py')+","+repr('include/halo2/native_tu15_pair_deletion_binding.hpp')+"\")\n"
 extra_main+=" needle=\"environment=dict(parent.__dict__);\";text=text.replace(needle,\"prefix=prefix.replace(\\\"  args=[typed['first']\\\",\\\"  assembly.append('.section .native_data,\\\\\\\"aw\\\\\\\"\\\\\\\\n.zero 16384');sections.append('.native_data 0x04000000:{*(.native_data)}')\\\\n  args=[typed['first']\\\");\"+needle)\n"
 outer=outer.replace(anchor,extra_main+anchor)
 env=dict(parent.__dict__);env['main']=None
 try:exec(outer,env);return env['main']()
 finally:
  owner=Path(os.environ['HALO2_TEST_SCRATCH']);out=Path(sys.argv[sys.argv.index('--output')+1]);out.mkdir(parents=True,exist_ok=True);p=out/'report.json';r=json.loads(p.read_text())if p.exists()else dict(passed=False,scope='Setup failed before original/native comparison')
  r.update(owned_scratch=str(owner),temporary_allocated_bytes_before_cleanup=sum(p.stat().st_blocks*512 for p in owner.rglob('*')if p.is_file()),retained_large_artifacts=[]);p.write_text(json.dumps(r,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':raise SystemExit(main())
