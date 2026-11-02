#!/usr/bin/env python3
"""Actual16C620 native parent+BSP to genuine24DC90 typed entry, not full return."""
import inspect
import validate_native_bsp_segment_query_actual_tu15 as parent

def main():
 source=inspect.getsource(parent.main)
 source=source.replace('base.OWNERS=OWNERS','base.OWNERS=(0x16c620,0x16b940,*OWNERS)')
 source=source.replace("observer=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text()", "observer=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text().replace('if(pc==stop)break;','if(pc==stop||pc==0x24dc90u)break;').replace('check(declared_fault||std::uint32_t(regs.rip)==stop,','check(declared_fault||std::uint32_t(regs.rip)==stop||std::uint32_t(regs.rip)==0x24dc90u,').replace('{\"return_word\",std::uint32_t(regs.rax)}','{\"return_word\",std::uint32_t(regs.rax)},{\"actual_stop\",std::uint32_t(regs.rip)},{\"boundary_gp\",gp(regs)},{\"boundary_stack_hex\",hex(memory(child.pid,std::uint32_t(regs.rsp),64))}')")
 start=source.index('  source=source.replace(" mov ${case[\'receiver\']}');end=source.index('\n  fp=',start);source=source[:start]+source[end:]
 source=source.replace('context=base.SP-40;banks[context]=bytes(40)','context=base.SP-0x490;banks[context]=bytes(40);bsp_output=base.SP-0x41c;banks[bsp_output]=bytes(0x41c)')
 source=source.replace("entry=0x1e68e0,receiver=q['flags'],arguments=[q['limit'],q['bitmap'],q['origin'],q['displacement']]","entry=0x16c620,receiver=0,arguments=[typed['mask'],typed['origin'],typed['displacement'],typed['ignore_first'],typed['ignore_second'],typed['result']]")
 source=source.replace("assert all(row['after_esp']==base.SP+20 for row in rows);","assert all(row['actual_stop']==0x24dc90 for row in rows);")
 source=source.replace("units=('native_tu15_bsp_segment_query.cpp','native_tu15_bsp_surface_query.cpp')","units=('native_tu15_camera_geometric_query.cpp','native_tu15_bsp_segment_query.cpp','native_tu15_bsp_surface_query.cpp','native_tu15_camera_query_leaves.cpp','native_tu15_camera_query_object_context.cpp','native_tu15_constraint_effect_location.cpp')")
 source=source.replace('#include "halo2/native_tu15_bsp_segment_query.hpp"','#include "halo2/native_tu15_camera_geometric_query.hpp"')
 source=source.replace('NativeTu15BspSegmentQueryStats activity;','NativeTu15CameraGeometricQueryStats activity;')
 start=source.index("  invoke='      try{result=");end=source.index('\n  driver=driver[:begin]',start)
 source=source[:start]+'''  invoke='''+repr('''      NativeTu15CameraGeometricQueryServices services;
      services.instanced=[&](const NativeTu15CameraInstancedQuery&query,auto&current)->bool{const auto gp=c.at("boundary_gp");const auto stack=unhex(c.at("boundary_stack_hex"));const auto word=[&](unsigned at){std::uint32_t n{};for(unsigned k=0;k<4;++k)n|=std::to_integer<unsigned>(stack[at+k])<<(8*k);return n;};check(query.cluster==gp[0]&&query.origin==gp[6]&&query.displacement==gp[7]&&query.mask==word(4)&&query.bsp_filter==word(8)&&query.result==word(12),"Actual instanced query DATA differs");check(J::array({current.control_word,current.status,current.mxcsr})==c.at("after_fp"),"Actual instanced FP differs");throw Required{};};
      try{const auto args=c.at("arguments");result=native_tu15_query_camera_geometry(memory,{args[0],args[1],args[2],args[3],args[4],args[5]},SCRATCH,fp,0,services,observe,&activity);
''')+'''.replace('SCRATCH','{'+str(bsp_output)+'u,'+str(context)+'u,'+str(base.SP-0x438)+'u,'+str(base.SP-0x43c)+'u}')
'''+source[end:]
 source=source.replace('activity.calls==1&&activity.completed==1&&bool(c.at("return_word").get<unsigned>()&255u)==result','activity.calls==1&&activity.completed==0&&activity.instanced_calls==1&&activity.unbound_services==0')
 source=source.replace("  project=scratch/'project';", "  for row in rows:row['required_owner']=True\n  save(fixture/'original.json',dict(image_sha256=NEW_PIN,oracle='unchanged-tu15-physical-array-and-real-pool-v1',cases=rows))\n  project=scratch/'project';")
 source=source.replace("ROOT/'src/physics/native_tu15_bsp_segment_query.cpp',ROOT/'include/halo2/native_tu15_bsp_segment_query.hpp'","ROOT/'src/physics/native_tu15_camera_geometric_query.cpp',ROOT/'include/halo2/native_tu15_camera_geometric_query.hpp'")
 env=dict(parent.__dict__,__doc__=__doc__,__file__=__file__);exec(source,env);return env['main']()
if __name__=='__main__':raise SystemExit(main())
