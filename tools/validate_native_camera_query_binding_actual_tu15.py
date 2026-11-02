#!/usr/bin/env python3
"""Same-production native16C620 service builder on genuine typed BEFORE query DATA.

Selected actual no-leaf MOPP lifetime remains bounded. All captured banks and
FP/order are strict; source-private arithmetic owners are independent DATA.
"""
import inspect
import validate_native_instanced_scene_ray_actual_tu15 as parent
OWNERS=(*parent.OWNERS,0x16c620,0x16b940,0x16e360,0xba970,0xba9e0,0x120fa0,0x16c370,0x16b870,0x1f7e00,0x1f8040,0x1f8200,0x1f7e90,0x1f7f60,0x144090,0xdb030,0x16b9a0,0x1e68e0,0x1e6980,0x1e6cb0,0x2a0230,0x1e6e80,0x14cdb0,0x11e600,0x16bb40)
def main():
 source=inspect.getsource(parent.main)
 source=source.replace("meta_source=inspect.getsource(base.metadata)","meta_source=inspect.getsource(base.metadata).replace(\"if ins.mnemonic!='lea':\",\"if ins.mnemonic not in('lea','prefetcht0'):\")")
 source=source.replace("q=dict(cluster=cluster,mask=typed['mask'],bsp_filter=(typed['mask']>>23)&63,origin=typed['origin'],displacement=typed['displacement'],result=typed['result'])","q=dict(mask=typed['mask'],origin=typed['origin'],displacement=typed['displacement'],ignore_first=typed['ignore_first'],ignore_second=typed['ignore_second'],result=typed['result'])")
 source=source.replace("entry=0x24dc90,receiver=0,arguments=[q['mask'],q['bsp_filter'],q['result']]","entry=0x16c620,receiver=0,arguments=[q['mask'],q['origin'],q['displacement'],q['ignore_first'],q['ignore_second'],q['result']]")
 start=source.index('  original=original.replace(');end=source.index('\n  env2=',start);source=source[:start]+source[end:]
 source=source.replace("units=('native_tu15_instanced_scene_ray.cpp'", "units=('native_tu15_camera_geometric_query.cpp','native_tu15_camera_query_leaves.cpp','native_tu15_camera_dynamic_query.cpp','native_tu15_camera_query_object_context.cpp','native_tu15_camera_dynamic_query_leaves.cpp','native_tu15_camera_collision_ray.cpp','native_tu15_camera_collision_regions.cpp','native_tu15_bsp_segment_query.cpp','native_tu15_bsp_surface_query.cpp','native_tu15_constraint_effect_location.cpp','native_tu15_camera_projection.cpp','native_tu15_camera_instanced_query.cpp','native_tu15_instanced_ray_callbacks.cpp','native_tu15_instanced_scene_ray.cpp'")
 source=source.replace("else(ROOT/'src/physics'/name)","else(ROOT/'src/rendering/world'/name)if name=='native_tu15_camera_projection.cpp'else(ROOT/'src/physics'/name)")
 source=source.replace("else(ROOT/'src/physics'/name)","else(HOST_STAGE/'src/physics'/name)if name=='native_tu15_instanced_ray_callbacks.cpp'else(ROOT/'src/physics'/name)")
 source=source.replace("shutil.copyfile(ROOT/'tests/native_tu15_instanced_scene_ray_actual_tests.cpp',project/'driver.cpp')","(project/'driver.cpp').write_text(DRIVER);shutil.copyfile(BINDING_STAGE/'include/halo2/native_tu15_camera_query_binding.hpp',project/'include/halo2/native_tu15_camera_query_binding.hpp');shutil.copyfile(HOST_STAGE/'include/halo2/native_tu15_instanced_ray_host_callbacks.hpp',project/'include/halo2/native_tu15_instanced_ray_host_callbacks.hpp');shutil.copyfile(ROOT/'include/halo2/native_tu15_instanced_scene_ray_binding.hpp',project/'include/halo2/native_tu15_instanced_scene_ray_binding.hpp')")

 source=source.replace("shutil.copyfile(ROOT/'tests/native_tu15_instanced_scene_ray_actual_tests.cpp',project/'driver.cpp')","(project/'driver.cpp').write_text(DRIVER)")
 source=source.replace('full_geometric_query_or_live_qualified=False','full_geometric_query_return_qualified=True,live_qualified=False')
 env=dict(parent.__dict__,__file__=__file__,__doc__=__doc__,OWNERS=OWNERS,DRIVER=driver(),HOST_STAGE=parent.ROOT/'analysis/physics/native-instanced-ray-host-callback-stage-20261102/staged',BINDING_STAGE=parent.ROOT/'analysis/physics/native-camera-query-binding-stage-20261102/staged');exec(source,env);return env['main']()
def driver():
 import validate_native_camera_geometric_query_full_actual_tu15 as whole
 source=whole.driver().replace('#include "halo2/native_tu15_instanced_scene_ray.hpp"','#include "halo2/native_tu15_instanced_scene_ray.hpp"\n#include "halo2/native_tu15_camera_query_binding.hpp"')
 start=source.index('NativeTu15InstancedSceneRayStats activity;');end=source.index('  check(result==',start)
 source=source[:start]+'''NativeTu15CameraQueryBindingActivity bound;auto&activity=bound.instanced.scene;auto&core=bound.instanced.traversal;
  const auto q=c.at("request");const bool result=native_tu15_query_bound_camera_scene(m,{q["mask"],q["origin"],q["displacement"],q["ignore_first"],q["ignore_second"],q["result"]},{private_data},fp,0,observer,&bound);
  check(bound.attempts==1&&bound.completed==1&&bound.refusals==0&&bound.unknown_owners()==0&&bound.resource_refusals==0&&bound.query.completed==1&&activity.completed>0&&activity.setup.unprovided_reads==0&&activity.setup.unprovided_writes==0&&core.completed>0&&core.leaves==0&&core.unbound_services==0,"Sameproduction native query activity differs");
  for(unsigned bad=0;bad<3;++bad){NativeTu15CameraQueryBindingActivity rejected;auto old=fp;const auto old_reads=m.reads;const auto old_stores=stores;bool failed{};try{(void)native_tu15_query_bound_camera_scene(m,{q["mask"],q["origin"],q["displacement"],q["ignore_first"],q["ignore_second"],q["result"]},{bad==0?private_data+1u:bad==1?0xfffffff0u:private_data},fp,bad==2?1u:0u,observer,&rejected);}catch(const std::runtime_error&){failed=true;}check(failed&&rejected.attempts==1&&rejected.refusals==1&&rejected.resource_refusals==1&&rejected.completed==0&&rejected.query.calls==0&&m.reads==old_reads&&stores==old_stores&&fp.control_word==old.control_word&&fp.status==old.status&&fp.mxcsr==old.mxcsr,"Query resource refusal changed DATA/FP or stats");}

'''+source[end:]
 return source
if __name__=='__main__':raise SystemExit(main())
