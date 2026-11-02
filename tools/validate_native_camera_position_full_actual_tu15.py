#!/usr/bin/env python3
"""Independent CD4A0 formal entry on actual1477 BEFORE DATA, native query returns.

Object is resolved from current control0 DATA. Output/FP are declared ordinary
inputs, not capturedCD4 caller ABI. All captured banks+owned12B output and FP
are strict; parent/query ordered operands remain strict. Other pure children
retain separate whole/fault proofs; no original query outcome is supplied.
"""
import inspect
import validate_native_camera_query_binding_actual_tu15 as query
import validate_native_camera_position_tu15 as camera
OWNERS=(*query.OWNERS,0xcd4a0)
def main():
 source=inspect.getsource(query.parent.main)
 source=source.replace("p.add_argument('--stage',", "p.add_argument('--default-fp-only',action='store_true');p.add_argument('--declared-heights',action='store_true');p.add_argument('--stage',")
 source=source.replace("meta_source=inspect.getsource(base.metadata)","meta_source=inspect.getsource(base.metadata).replace(\"if ins.mnemonic!='lea':\",\"if ins.mnemonic not in('lea','prefetcht0'):\")")
 start=source.index('  fp=typed[\'floating_point\'];q=');end=source.index('\n  base.OWNERS=',start)
 source=source[:start]+'''  def word(p):
   owner=next((first,raw)for first,raw in banks.items()if first<=p and p+4<=first+len(raw));return int.from_bytes(owner[1][p-owner[0]:p-owner[0]+4],'little')
  fp=typed['floating_point'];q=dict(object=word(word(0x4f9b54)+0x14),destination=base.UI+0x2000)
  assert q['object']not in(0,0xffffffff)
  controls=[]
  if a.declared_heights:
   objects=word(0x4ecbd0);headers=word(objects+0x44);unit=word(headers+(q['object']&0xffff)*12+8);tags=word(0x4f0414);definition=word(tags+(word(unit)&0xffff)*16+8)
   for address in(definition+0x218,definition+0x21c,unit+0x2c4):
    owner=next(first for first,raw in banks.items()if first<=address and address+4<=first+len(raw));raw=bytearray(banks[owner]);before=word(address);raw[address-owner:address-owner+4]=bytes(4);banks[owner]=bytes(raw);controls.append(dict(address=address,before=before,declared=0))
  save(a.output/'declared-camera-inputs.json',dict(offline_only=True,source_position_flags_geometry_unchanged=True,controls=controls))
  rows=[dict(name='actualData_declared_camera_'+str(mode),entry=0xcd4a0,receiver=q['object'],arguments=[],request=q,before_fp_input=[(fp['x87_control_word']&~0xc00)|(mode<<10),fp['x87_status'],(fp['mxcsr']&~0x6000)|(mode<<13)])for mode in range(1 if a.default_fp_only else 4)]
'''+source[end:]
 source=source.replace("kernel=core.metadata(image);meta['sites']+=kernel['sites'];", "kernel=core.metadata(image);meta['sites']+=kernel['sites'];code=__import__('oracle_smg_firing_1_1')._Code(image,'.text');body=__import__('generate_physics_live_support_tu15').complete_body(code,0xcd4a0).code;[site.update(opaque_call=True,instruction_size=code.at(site['instruction']).size)for site in meta['sites']if site['instruction']in body and code.at(site['instruction']).mnemonic=='call'and code.at(site['instruction']).op_str!='0x16c620'];")
 source=source.replace("original=reference_original().replace", "original=reference_original().replace(\"observer=owned/'observer';subprocess.run\",\"observer=owned/'observer';(owned/'observer.cpp').write_text(OBSERVER);subprocess.run\").replace(\"str(ROOT/'tools/observe_native_array_growth_tu15.cpp')\",\"str(owned/'observer.cpp')\").replace")
 source=source.replace('env2=dict(base.__dict__,metadata=metadata)','env2=dict(base.__dict__,metadata=metadata,OBSERVER=OBSERVER)')
 source=source.replace("meta['regions']=[r for r in meta['regions']if r['address']!=base.UI]","meta['regions']=[r for r in meta['regions']if r['address']!=base.UI]+[dict(address=base.UI+0x2000,bytes=12)]")
 start=source.index('  original=original.replace(');end=source.index('\n  env2=',start)
 source=source[:start]+'''  original=original.replace(" mov ${case['receiver']},%ecx\\n int3",f" mov ${{case['receiver']}},%ecx\\n mov ${q['destination']},%eax\\n int3")
'''+source[end:]
 units=('native_tu15_camera_geometric_query.cpp','native_tu15_camera_query_leaves.cpp','native_tu15_camera_dynamic_query.cpp','native_tu15_camera_query_object_context.cpp','native_tu15_camera_dynamic_query_leaves.cpp','native_tu15_camera_collision_ray.cpp','native_tu15_camera_collision_regions.cpp','native_tu15_bsp_segment_query.cpp','native_tu15_bsp_surface_query.cpp','native_tu15_constraint_effect_location.cpp','native_tu15_camera_projection.cpp','native_tu15_camera_instanced_query.cpp','native_tu15_instanced_ray_callbacks.cpp','native_tu15_instanced_scene_ray.cpp','native_tu15_instanced_ray_lifetime.cpp','native_tu15_island_activation.cpp','native_tu15_mopp_ray_traversal.cpp','native_tu15_mopp_ray_setup.cpp',*[query.parent.Path(u).name for u in camera.UNITS])
 units=tuple(dict.fromkeys(units));start=source.index("units=('native_tu15_instanced_scene_ray.cpp'");end=source.index('\n  for name in',start);source=source[:start]+'units='+repr(units)+source[end:]
 extra={query.parent.Path(u).name:str(query.parent.ROOT/u)for u in camera.UNITS};extra['native_tu15_camera_projection.cpp']=str(query.parent.ROOT/'src/rendering/world/native_tu15_camera_projection.cpp');extra['native_tu15_camera_position.cpp']=str(query.parent.ROOT/'analysis/physics/native-camera-position-query-suffix-stage-20261102/staged/src/physics/native_tu15_camera_position.cpp')
 source=source.replace("else(ROOT/'src/physics'/name)","else(HOST_STAGE/'src/physics'/name)if name=='native_tu15_instanced_ray_callbacks.cpp'else Path(EXTRA[name])if name in EXTRA else(ROOT/'src/physics'/name)")
 needle="shutil.copyfile(ROOT/'tests/native_tu15_instanced_scene_ray_actual_tests.cpp',project/'driver.cpp')"
 source=source.replace(needle,"(project/'driver.cpp').write_text(DRIVER);shutil.copyfile(BINDING_STAGE/'include/halo2/native_tu15_camera_query_binding.hpp',project/'include/halo2/native_tu15_camera_query_binding.hpp');shutil.copyfile(HOST_STAGE/'include/halo2/native_tu15_instanced_ray_host_callbacks.hpp',project/'include/halo2/native_tu15_instanced_ray_host_callbacks.hpp')")
 source=source.replace('full_geometric_query_or_live_qualified=False','full_camera_false_query_suffix_qualified=True,live_qualified=False')
 source=source.replace('profiles=4,results=results','profiles=len(rows),declared_camera_input_controls=controls,actual_before_data_unmodified=not a.declared_heights,qualified_control_word_domain="27F only"if a.default_fp_only else"all4roundings",results=results')
 env=dict(query.parent.__dict__,__file__=__file__,__doc__=__doc__,OWNERS=OWNERS,DRIVER=driver(),EXTRA=extra,OBSERVER=observer_source(),HOST_STAGE=query.parent.ROOT/'analysis/physics/native-instanced-ray-host-callback-stage-20261102/staged',BINDING_STAGE=query.parent.ROOT/'analysis/physics/native-camera-query-binding-stage-20261102/staged');exec(source,env);return env['main']()
def observer_source():
 text=(query.parent.ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text()
 text=text.replace('allocator_calls=J::array();user_regs_struct','allocator_calls=J::array(),camera_queries=J::array();std::uint32_t query_return{};user_regs_struct')
 needle='const auto pc=std::uint32_t(regs.rip);if(pc==stop)break;'
 assert text.count(needle)==1
 text=text.replace(needle,'''const auto pc=std::uint32_t(regs.rip);
    if(pc==query_return){user_fpregs_struct call_fp{};request(PTRACE_GETFPREGS,child.pid,nullptr,&call_fp);camera_queries.back()["return_al"]=std::uint32_t(regs.rax)&255u;camera_queries.back()["after_fp"]=J::array({call_fp.cwd,call_fp.swd,call_fp.mxcsr});query_return=0;}
    if(pc==0x16c620u){const auto word=[&](std::uint32_t p){const auto bytes=memory(child.pid,p,4);std::uint32_t n{};for(unsigned k=0;k<4;++k)n|=std::to_integer<unsigned>(bytes[k])<<(8*k);return n;};const auto stack=std::uint32_t(regs.rsp),origin=word(stack+8u),delta=word(stack+12u);user_fpregs_struct call_fp{};request(PTRACE_GETFPREGS,child.pid,nullptr,&call_fp);query_return=word(stack);camera_queries.push_back(J{{"mask",word(stack+4u)},{"ignore_first",word(stack+16u)},{"ignore_second",word(stack+20u)},{"origin_bits",hex(memory(child.pid,origin,12))},{"displacement_bits",hex(memory(child.pid,delta,12))},{"before_fp",J::array({call_fp.cwd,call_fp.swd,call_fp.mxcsr})}});}
    if(pc==stop)break;''')
 text=text.replace('{"initial_banks",before}','{"camera_queries",camera_queries},{"initial_banks",before}')
 needle='    request(PTRACE_SINGLESTEP,child.pid);wait();check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original array instruction fault");'
 assert text.count(needle)==1
 return text.replace(needle,needle+'''
    if(site->second.value("opaque_call",false)){
      const auto target=pc+site->second.at("instruction_size").get<std::uint32_t>();
      request(PTRACE_POKEUSER,child.pid,reinterpret_cast<void*>(offsetof(user,u_debugreg)),reinterpret_cast<void*>(std::uintptr_t(target)));
      request(PTRACE_POKEUSER,child.pid,reinterpret_cast<void*>(offsetof(user,u_debugreg)+7*sizeof(unsigned long)),reinterpret_cast<void*>(1));
      request(PTRACE_CONT,child.pid);wait();user_regs_struct done{};request(PTRACE_GETREGS,child.pid,nullptr,&done);
      check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP&&done.rip==target,"Original genuinecamera child did notreturn");
      request(PTRACE_POKEUSER,child.pid,reinterpret_cast<void*>(offsetof(user,u_debugreg)+7*sizeof(unsigned long)),nullptr);
    }
''')
def driver():
 source=query.driver().replace('#include "halo2/native_tu15_camera_query_binding.hpp"','#include "halo2/native_tu15_camera_query_binding.hpp"\n#include "halo2/native_tu15_camera_position.hpp"')
 source=source.replace('bool log{};','bool log{},inside_child{};').replace('if(log&&','if(log&&!inside_child&&').replace('observer=[&](const auto&e){','observer=[&](const auto&e){if(m.inside_child)return;').replace('private_data+0x10000u','private_data+0x20000u').replace('std::vector<std::byte>(0x10000,','std::vector<std::byte>(0x20000,')
 source=source.replace('profiles==4','profiles==fixture.at("cases").size()')
 start=source.index('NativeTu15CameraQueryBindingActivity bound;');end=source.index('  check(result==',start)
 source=source[:start]+'''NativeTu15CameraQueryBindingActivity bound;auto&activity=bound.instanced.scene;auto&core=bound.instanced.traversal;NativeTu15CameraPositionStats camera;NativeTu15CameraPositionServices services;
  services.native_child=[&](auto owner,bool entering){if(entering&&owner==0xbad60u&&observer)throw std::runtime_error("Camera component requires BAD60 semantic observer owner");m.inside_child=entering;};
  unsigned segment_index{};
  services.segment=[&](const NativeTu15CameraSegment&r,auto&current){const auto&expected=c.at("camera_queries").at(segment_index++);const auto old_log=m.log;m.log=false;std::array<std::byte,12>origin{},delta{};m.read(r.origin,origin);m.read(r.displacement,delta);m.log=old_log;check(expected.at("mask")==r.mask&&expected.at("ignore_first")==r.ignore_first&&expected.at("ignore_second")==r.ignore_second&&expected.at("origin_bits")==hex(origin)&&expected.at("displacement_bits")==hex(delta)&&expected.at("before_fp")==J::array({current.control_word,current.status,current.mxcsr}),"Genuine camera query input DATA/FP differs at "+std::to_string(segment_index));const bool result=native_tu15_query_bound_camera_scene(m,{r.mask,r.origin,r.displacement,r.ignore_first,r.ignore_second,r.result},{private_data+0x10000u},current,0,observer,&bound);check(result==bool(expected.at("return_al").get<unsigned>())&&expected.at("after_fp")==J::array({current.control_word,current.status,current.mxcsr}),"Genuine camera query result/FP differs");return result;};
  const auto q=c.at("request");native_tu15_compute_unit_camera_position(m,q["object"],q["destination"],{private_data,{private_data+0x200u}},fp,0,services,observer,&camera);
  check(camera.calls==1&&camera.completed==1&&camera.segment_calls==2&&bound.attempts==2&&bound.completed==2&&bound.refusals==0&&bound.unknown_owners()==0&&bound.resource_refusals==0,"Fullnativecamera/query activity differs");
'''+source[end:]
 needle='check(result==bool(c.at("return_word").get<unsigned>()&255u),"Actual native instanced return differs");';assert source.count(needle)==1;source=source.replace(needle,'')
 return source
if __name__=='__main__':raise SystemExit(main())
