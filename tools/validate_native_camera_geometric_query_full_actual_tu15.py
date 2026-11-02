#!/usr/bin/env python3
"""Whole native16C620 current-BEFORE query with BSP, guarded MOPP and dynamic DATA.

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
 source=source.replace("units=('native_tu15_instanced_scene_ray.cpp'", "units=('native_tu15_camera_geometric_query.cpp','native_tu15_camera_query_leaves.cpp','native_tu15_camera_dynamic_query.cpp','native_tu15_camera_query_object_context.cpp','native_tu15_camera_dynamic_query_leaves.cpp','native_tu15_camera_collision_ray.cpp','native_tu15_camera_collision_regions.cpp','native_tu15_bsp_segment_query.cpp','native_tu15_bsp_surface_query.cpp','native_tu15_constraint_effect_location.cpp','native_tu15_camera_projection.cpp','native_tu15_instanced_scene_ray.cpp'")
 source=source.replace("else(ROOT/'src/physics'/name)","else(ROOT/'src/rendering/world'/name)if name=='native_tu15_camera_projection.cpp'else(ROOT/'src/physics'/name)")
 source=source.replace("shutil.copyfile(ROOT/'tests/native_tu15_instanced_scene_ray_actual_tests.cpp',project/'driver.cpp')","(project/'driver.cpp').write_text(DRIVER)")
 source=source.replace('full_geometric_query_or_live_qualified=False','full_geometric_query_return_qualified=True,live_qualified=False')
 env=dict(parent.__dict__,__file__=__file__,__doc__=__doc__,OWNERS=OWNERS,DRIVER=driver());exec(source,env);return env['main']()
def driver():
 source=(parent.ROOT/'tests/native_tu15_instanced_scene_ray_actual_tests.cpp').read_text()
 source=source.replace('#include "halo2/native_tu15_instanced_scene_ray.hpp"','#include "halo2/native_tu15_instanced_scene_ray.hpp"\n#include "halo2/native_tu15_camera_geometric_query.hpp"\n#include "halo2/native_tu15_camera_dynamic_query.hpp"')
 source=source.replace('private_data+0x1000u','private_data+0x10000u').replace('std::vector<std::byte>(0x1000,','std::vector<std::byte>(0x10000,')
 start=source.index('  const auto q=c.at("request");');end=source.index('\n  check(result==',start)
 source=source[:start]+'''  const auto q=c.at("request");NativeTu15CameraGeometricQueryStats root;NativeTu15CameraGeometricQueryServices outer;NativeTu15CameraDynamicStats dynamic;
  outer.instanced=[&](const NativeTu15CameraInstancedQuery&r,auto&current){return native_tu15_query_instanced_scene_ray(m,{r.cluster,r.mask,r.bsp_filter,r.origin,r.displacement,r.result},{private_data+0x1000u},current,0,services,observer,&activity);};
  outer.dynamic=[&](const NativeTu15CameraDynamicQuery&r,auto&current){NativeTu15CameraDynamicServices children;children.acquire_frame=[&](unsigned level){check(level<8,"Native query DATA capacity requires larger owner");const auto p=private_data+0x3000u+level*0x1000u;return NativeTu15CameraDynamicFrame{p+0x200u,p+0x220u,p+0x640u,{p,p+64u,p+52u,p+76u,p+116u}};};return native_tu15_query_dynamic_camera_object(m,r,current,0,children,observer,&dynamic);};
  const bool result=native_tu15_query_camera_geometry(m,{q["mask"],q["origin"],q["displacement"],q["ignore_first"],q["ignore_second"],q["result"]},{private_data,private_data+0x500u,private_data+0x540u,private_data+0x580u},fp,0,outer,observer,&root);
  check(root.calls==1&&root.completed==1&&root.unbound_services==0&&activity.completed>0&&activity.refusals==0&&activity.setup.unprovided_reads==0&&activity.setup.unprovided_writes==0&&core.completed>0&&core.leaves==0&&core.unbound_services==0,"Whole actual native query activity differs");
'''+source[end:]
 return source
if __name__=='__main__':raise SystemExit(main())
