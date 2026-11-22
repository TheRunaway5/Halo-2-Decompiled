// Native world placement owners (track "placement"). Each owner replicates
// its TU1.5 routine over the live world memory: stores keep the original
// order, widths and values; floating-point steps run the original SSE/x87
// operation under the live control words. Oracle cases are in
// tools/native_object_factory_cases_placement.py.
#include "halo2/native_object_factory_placement_tu15.hpp"
#include "halo2/native_object_factory_havok_tu15.hpp"
#include "campaign/scripting/native_campaign_script_fp_tu15.hpp"
#include "halo2/native_object_factory_core_tu15.hpp"
#include "halo2/native_object_factory_nodebuild_tu15.hpp"
#include "halo2/native_tu15_activation_services.hpp"
#include "halo2/native_tu15_component_state.hpp"
#include "halo2/native_tu15_data_array_allocate.hpp"
#include "halo2/native_tu15_data_array_delete.hpp"
#include "halo2/native_tu15_dirty_island_objects.hpp"
#include "halo2/native_tu15_object_delete_protection.hpp"
#include "halo2/native_tu15_object_detachment.hpp"
#include "halo2/native_tu15_object_spatial_packet.hpp"
#include "native_object_factory_tu15_internal.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <vector>

namespace halo2::native_object_factory_tu15 {
using namespace detail;
namespace {
namespace fp=native_campaign_script_fp;
constexpr std::uint32_t structure_bsp_index=0x474c24u; // int16 current structure BSP
constexpr std::uint32_t collision_bsp=0x4ecc0cu;       // current collision BSP
constexpr std::uint32_t collision_bsp_sphere=0x4ecc10u; // collision BSP of 1E60C0 queries
constexpr std::uint32_t structure_bsp=0x4ecc18u;       // current structure BSP

float mul(float a,float b,std::uint32_t mxcsr){return fp::mulss(a,b,mxcsr);}
float add(float a,float b,std::uint32_t mxcsr){return fp::addss(a,b,mxcsr);}
float sub(float a,float b,std::uint32_t mxcsr){return fp::subss(a,b,mxcsr);}
float cmp(float value,std::uint32_t mxcsr){return fp::sse_compared(bits(value),mxcsr);}
float rf(const M& m,std::uint32_t at){return f32(r32(m,at));}
// COMISS a,b: CF clear (a >= b, ordered) and the strict a > b.
bool above_or_equal(float a,float b,std::uint32_t mxcsr){return cmp(a,mxcsr)>=cmp(b,mxcsr);}
bool above(float a,float b,std::uint32_t mxcsr){return cmp(a,mxcsr)>cmp(b,mxcsr);}
// Signed 24-bit child reference at node byte `offset` (node + 3*side + 1, SAR 8).
std::uint32_t child(const M& m,std::uint32_t node,unsigned side){return std::uint32_t(s32(r32(m,node+side*3u+1u))>>8);}
// d = ((p.z*c.z + p.y*c.y) + p.x*c.x) - p.w with the original operand order.
float plane_distance(const M& m,std::uint32_t plane,std::uint32_t c,std::uint32_t mxcsr,bool center_first) {
  float z,y,x;
  if(center_first) {
    z=mul(rf(m,c+8u),rf(m,plane+8u),mxcsr);y=mul(rf(m,c+4u),rf(m,plane+4u),mxcsr);x=mul(rf(m,plane),rf(m,c),mxcsr);
  } else {
    z=mul(rf(m,plane+8u),rf(m,c+8u),mxcsr);y=mul(rf(m,plane+4u),rf(m,c+4u),mxcsr);x=mul(rf(m,plane),rf(m,c),mxcsr);
  }
  return sub(add(add(z,y,mxcsr),x,mxcsr),rf(m,plane+0xcu),mxcsr);
}
// The cluster of leaf `leaf` of the current structure BSP (sign-extended).
std::uint32_t leaf_cluster(const M& m,std::uint32_t leaf){return sx16(r16(m,r32(m,r32(m,structure_bsp)+0x30u)+leaf*8u));}

// 1E60C0/1E6150 leaf collection: the collision leaves a sphere touches, in
// the original preorder (back child first on a straddle). C1540 consumes only
// the first leaf and whether any leaf was recorded; the surface, edge and
// vertex lists 1E64D0/1E6570 build into C1540's stack buffer are dead there.
struct SphereLeaves {
  const M& m;std::uint32_t bsp,center;float radius;std::uint32_t mxcsr;
  std::uint32_t count=0,first=none,depth=0;
  void visit(std::uint32_t reference) {
    if(reference&0x800000u) {
      if(reference==none)return;
      if(count<0x100u){if(!count)first=reference&0x7fffffu;++count;}
      return;
    }
    const auto node=r32(m,bsp+4u)+reference*8u;
    const auto plane=sx16(r16(m,node))*16u+r32(m,bsp+0xcu);
    const auto d=plane_distance(m,plane,center,mxcsr,true);
    const bool inside_front=above(radius,d,mxcsr);
    if(!above(d,sub(0.f,radius,mxcsr),mxcsr)){visit(child(m,node,0));return;}
    if(!inside_front){visit(child(m,node,1));return;}
    // Straddle: the plane index is stacked while each side is visited. The
    // stack has 128 slots; a deeper push runs into the walk's projection
    // state (see WalkRecord, 16D740's walk): a plane marked for the first
    // child at slot 128 sets its stop byte, the plain plane for the second
    // child clears it, and a set stop byte ends the walk once a leaf is
    // recorded. A push past slot 130 overwrites 1E60C0's return address.
    if(depth>=131u)
      refuse("1E6150","a collision plane stack deeper than 131 entries (the original overwrites 1E60C0's return address)");
    const auto slot=depth++;
    if(slot==128u)stop=true;
    visit(child(m,node,0));
    if(stop && count)return;
    if(slot==128u)stop=false;
    visit(child(m,node,1));
    if(stop && count)return;
    --depth;
  }
  bool stop=false;
};
}

std::uint32_t bsp3d_leaf_from_point(const M& m,std::uint32_t bsp,std::uint32_t node,std::uint32_t point,std::uint32_t mxcsr) {
  const auto nodes=r32(m,bsp+4u),planes=r32(m,bsp+0xcu);
  auto reference=node;
  do {
    const auto at=nodes+(reference&0x7fffffu)*8u;
    const auto plane=sx16(r16(m,at))*16u+planes;
    const auto d=plane_distance(m,plane,point,mxcsr,false);
    reference=child(m,at,above_or_equal(d,0.f,mxcsr)?1u:0u);
  } while(!(reference&0x800000u));
  return reference==none?none:reference&0x7fffffu;
}

Location location_of_point(const M& m,std::uint32_t point,std::uint32_t mxcsr) {
  const auto index=r16(m,structure_bsp_index);
  if(index==0xffffu)return {none,0xffffu,index};
  const auto leaf=bsp3d_leaf_from_point(m,r32(m,collision_bsp),0,point,mxcsr);
  return {leaf,std::uint16_t(leaf!=none?leaf_cluster(m,leaf):none),index};
}

void location_from_point(M& m,std::uint32_t out,std::uint32_t point,std::uint32_t mxcsr) {
  const auto index=r16(m,structure_bsp_index);
  if(index==0xffffu){w16(m,out+4u,index);w16(m,out+6u,index);w32(m,out,none);return;}
  const auto leaf=bsp3d_leaf_from_point(m,r32(m,collision_bsp),0,point,mxcsr);
  w32(m,out,leaf);
  const auto cluster=leaf!=none?leaf_cluster(m,leaf):none;
  w16(m,out+6u,index);w16(m,out+4u,std::uint16_t(cluster));
}

void location_from_leaf(M& m,std::uint32_t out,std::uint32_t leaf) {
  w32(m,out,leaf);
  if(leaf==none){const auto index=r16(m,structure_bsp_index);w16(m,out+4u,0xffffu);w16(m,out+6u,index);return;}
  w16(m,out+4u,std::uint16_t(leaf_cluster(m,leaf)));w16(m,out+6u,r16(m,structure_bsp_index));
}

bool object_get_location(M& m,std::uint32_t object,std::uint32_t out,const PlacementFp& fpu) {
  const auto body=object_body(m,object);
  location_from_point(m,out,body+0x40u,fpu.mxcsr);
  if(r16(m,out+4u)==0xffffu) {
    const auto radius=r32(m,body+0x4cu);
    SphereLeaves leaves{m,r32(m,collision_bsp_sphere),body+0x40u,f32(radius),fpu.mxcsr};
    leaves.visit(0);
    if(leaves.count)location_from_leaf(m,out,leaves.first);
    else location_from_point(m,out,body+0x64u,fpu.mxcsr);
  }
  return r16(m,out+4u)!=0xffffu;
}

void owned_lights(M&,std::uint32_t owner,bool detach,bool reconnect,PlacementFp&);
void object_disconnect_from_map(M& m,std::uint32_t object,PlacementFp& fpu) {
  const auto header=object_row(m,object),body=r32(m,header+8u);
  NativeTu15ObjectDetachmentServices services;
  services.detach_attachment=[&](std::uint32_t light,PlacementFp&){native_tu15_detach_light_attachment(m,light);};
  services.detach_children=[&m](std::uint32_t owner,std::uint32_t detach,std::uint32_t reconnect,PlacementFp& f) {
    owned_lights(m,owner,(detach&0xffu)!=0,(reconnect&0xffu)!=0,f);
  };
  native_tu15_detach_object_tree(m,{object,1,0,0,0},fpu,services);
  const auto partition=(r32(m,body+4u)>>9)&1u?0x4eabb0u:0x4eaba4u;
  native_tu15_remove_spatial_chain(m,partition,object,body+0x60u);
  w32(m,body+4u,r32(m,body+4u)&0xfffffeffu);
  w8(m,header+2u,r8(m,header+2u)&0xbfu);
}

void object_mark_for_update(M& m,std::uint32_t object){native_tu15_wake_object_ancestors(m,object);}

void object_defer_physics(M& m,std::uint32_t object,PlacementFp& fpu) {
  NativeTu15ComponentStateServices services;
  // 1D97F0 is track havok's (no original stack model: its fixed-body pair
  // collector keeps a private address).
  services.activate=[&m](std::uint32_t component,PlacementFp& f,unsigned){havok_component_add(m,component,f);};
  native_tu15_defer_component_object(m,object,fpu,0,services);
}
}

// ---- Cluster portal flood (1D2F10 and its leaves) ----
namespace halo2::native_object_factory_tu15 {
namespace {
constexpr std::uint32_t flood_stamp=0x4f3ce4u,flood_active=0x4f3ce1u,flood_marks=0x4f3ce8u;
// BEGIN TU1.5 .rdata literals (checked against the XBE by the oracle cases)
// 0x44AC8C: projection rows (axis*2 + positive) of (u, v, w) int16.
constexpr std::array<std::array<std::uint16_t,3>,6> projection_rows{{{2,1,0},{1,2,0},{0,2,1},{2,0,1},{1,0,2},{0,1,2}}};
constexpr std::uint32_t zero_bits=0x00000000u;   // 0x46A0E8
constexpr std::uint32_t world_lower=0xc7000000u; // 0x46AA04
constexpr std::uint32_t world_upper=0x47000000u; // 0x46A530
constexpr std::uint32_t normalize_epsilon=0x38d1b717u; // 0x46A0EC
constexpr std::uint32_t one_bits=0x3f800000u;    // 0x46A0D0
constexpr std::uint32_t float_max=0x7f7fffffu;    // 0x45695C (FLT_MAX)
constexpr std::uint32_t sphere_margin=0x3d800000u; // 0x46A4B0 (1/16: 16D2A0's query margin)
constexpr std::uint32_t surface_distance=0x3c23d70au; // 0x46A18C (0.01)
constexpr std::uint32_t facing_limit=0xb8d1b717u;  // 0x46A30C (-1e-4)
// END TU1.5 .rdata literals

#if HALO2_NATIVE_SCRIPT_X86_FP
// 14CEC8..14CEF0: x87 plane distance under the live control word; FST to
// `dist`, then FCOMIP radius against |d|. True when radius > |d| (ordered).
bool x87_portal_plane(const std::array<float,4>& p,const std::array<float,3>& c,float radius,
    std::uint16_t control,float& dist) {
  const auto mode=fp::x87_control(control);std::uint16_t saved{};std::uint8_t accepted{};
  asm volatile("fnstcw %[saved]\n\tfldcw %[mode]\n\t"
      "flds %[py]\n\tfmuls %[cy]\n\tflds %[pz]\n\tfmuls %[cz]\n\t.byte 0xde,0xc1\n\t"
      "flds %[cx]\n\tfmuls %[px]\n\t.byte 0xde,0xc1\n\tfsubs %[pw]\n\tfsts %[dist]\n\tfabs\n\t"
      "flds %[r]\n\tfcomip %%st(1), %%st\n\tfstp %%st(0)\n\tseta %[ok]\n\tfldcw %[saved]"
      :[dist]"=m"(dist),[ok]"=q"(accepted),[saved]"+m"(saved)
      :[mode]"m"(mode),[py]"m"(p[1]),[cy]"m"(c[1]),[pz]"m"(p[2]),[cz]"m"(c[2]),[cx]"m"(c[0]),
       [px]"m"(p[0]),[pw]"m"(p[3]),[r]"m"(radius)
      :"st","st(1)","cc");
  return accepted!=0;
}
// 14D026..14D04F: FLD r, FMUL r, FLD d, FMUL d, FSUBP st(1),st, FSQRT, FSTP dword.
float x87_portal_disk(float radius,float dist,std::uint16_t control) {
  const auto mode=fp::x87_control(control);std::uint16_t saved{};float disk{};
  asm volatile("fnstcw %[saved]\n\tfldcw %[mode]\n\t"
      "flds %[r]\n\tfmuls %[r]\n\tflds %[d]\n\tfmuls %[d]\n\t.byte 0xde,0xe9\n\tfsqrt\n\tfstps %[disk]\n\t"
      "fldcw %[saved]"
      :[disk]"=m"(disk),[saved]"+m"(saved):[mode]"m"(mode),[r]"m"(radius),[d]"m"(dist):"st","st(1)");
  return disk;
}
// FLD dword; FABS; FSTP dword (a signalling NaN loads quiet).
float x87_abs(float value) {
  float out{};asm volatile("flds %[v]\n\tfabs\n\tfstps %[o]":[o]"=m"(out):[v]"m"(value):"st");return out;
}
#else
#error "Native placement owners need the x87 (i386/x86-64) floating-point unit"
#endif

// 243450: the circle (pt, r) lies inside every edge of the projected polygon.
bool circle_inside_polygon(const std::vector<std::array<float,2>>& vt,std::array<float,2> pt,float r,
    std::int16_t count,std::uint32_t mxcsr) {
  const auto r2=mul(r,r,mxcsr);
  if(count<=0)return true;
  for(std::int16_t i=0;i<count;++i) {
    const auto px=sub(pt[0],vt[std::size_t(i)][0],mxcsr),py=sub(pt[1],vt[std::size_t(i)][1],mxcsr);
    const auto j=i+1>=count?0:i+1;
    const auto ex=sub(vt[std::size_t(j)][0],vt[std::size_t(i)][0],mxcsr);
    const auto ey=sub(vt[std::size_t(j)][1],vt[std::size_t(i)][1],mxcsr);
    const auto len2=add(mul(ex,ex,mxcsr),mul(ey,ey,mxcsr),mxcsr);
    if(cmp(len2,mxcsr)==0.f)continue;             // UCOMISS: an ordered equal skips
    const auto cross=sub(mul(px,ey,mxcsr),mul(py,ex,mxcsr),mxcsr);
    if(!above(cross,0.f,mxcsr))continue;
    if(above(mul(cross,cross,mxcsr),mul(len2,r2,mxcsr),mxcsr))return false;
  }
  return true;
}
}

std::uint16_t dominant_axis(const M& m,std::uint32_t vector,std::uint32_t mxcsr) {
  const auto x=x87_abs(rf(m,vector)),y=x87_abs(rf(m,vector+4u)),z=x87_abs(rf(m,vector+8u));
  if(z>=y && above_or_equal(z,x,mxcsr))return 2;  // FCOMIP (exact), then COMISS
  return above_or_equal(y,x,mxcsr)?1:0;
}

bool circle_inside_portal(const M& m,std::uint32_t vertices,std::uint32_t point,float radius,std::uint16_t count,
    std::uint32_t mxcsr) {
  std::vector<std::array<float,2>> vt;
  for(std::int32_t i=0;i<std::int16_t(count);++i)vt.push_back({rf(m,vertices+std::uint32_t(i)*8u),rf(m,vertices+std::uint32_t(i)*8u+4u)});
  return circle_inside_polygon(vt,{rf(m,point),rf(m,point+4u)},radius,std::int16_t(count),mxcsr);
}

bool portal_intersects_sphere(const M& m,std::uint16_t portal_index,std::uint32_t center,std::uint32_t sbsp,
    float radius,const PlacementFp& fpu) {
  const auto mxcsr=fpu.mxcsr;
  const auto portal=r32(m,sbsp+0x60u)+sx16(portal_index)*0x24u;
  const auto offset=r32(m,portal+4u)<<4;
  const auto plane=r32(m,r32(m,sbsp+0x18u)+0xcu)+offset;
  const std::array<float,4> p{rf(m,plane),rf(m,plane+4u),rf(m,plane+8u),rf(m,plane+0xcu)};
  const std::array<float,3> c{rf(m,center),rf(m,center+4u),rf(m,center+8u)};
  float dist{};
  if(!x87_portal_plane(p,c,radius,fpu.control_word,dist))return false;
  const auto dz=sub(rf(m,portal+0x10u),c[2],mxcsr),dx=sub(rf(m,portal+8u),c[0],mxcsr),dy=sub(rf(m,portal+0xcu),c[1],mxcsr);
  const auto rr=add(rf(m,portal+0x14u),radius,mxcsr);
  const auto d2=add(add(mul(dz,dz,mxcsr),mul(dx,dx,mxcsr),mxcsr),mul(dy,dy,mxcsr),mxcsr);
  if(!above(mul(rr,rr,mxcsr),d2,mxcsr))return false;
  const auto plane2=r32(m,r32(m,collision_bsp)+0xcu)+offset;
  const auto axis=sx16(dominant_axis(m,plane2,mxcsr));
  const auto positive=above(rf(m,plane2+axis*4u),0.f,mxcsr)?1u:0u;
  const auto negated=sub(0.f,dist,mxcsr);
  std::array<float,3> projected{};
  for(unsigned k=0;k<3;++k)projected[k]=add(mul(rf(m,plane2+4u*k),negated,mxcsr),c[k],mxcsr);
  if(axis>2u)refuse("14CEA0","a dominant axis outside 0..2");
  const auto& projection=projection_rows[axis*2u+positive];
  const std::uint32_t u=projection[0],v=projection[1];
  const std::array<float,2> pt{projected[u],projected[v]};
  const auto count=s32(r32(m,portal+0x1cu));
  std::vector<std::array<float,2>> vt;
  if(count>0) {
    if(count>0x80)refuse("14CEA0","a portal with more than 128 vertices");
    const auto vertices=r32(m,portal+0x20u);
    for(std::int32_t i=0;i<count;++i)vt.push_back({rf(m,vertices+std::uint32_t(i)*12u+u*4u),rf(m,vertices+std::uint32_t(i)*12u+v*4u)});
  }
  const auto disk=x87_portal_disk(radius,dist,fpu.control_word);
  return circle_inside_polygon(vt,pt,disk,std::int16_t(r16(m,portal+0x1cu)),mxcsr);
}

std::uint32_t cluster_portal_flood(M& m,std::uint16_t cluster,std::uint32_t center,float radius,std::uint32_t maximum,
    const ClusterSink& out,std::uint32_t slot,const PlacementFp& fpu) {
  const auto sbsp=r32(m,structure_bsp);
  const auto record=r32(m,sbsp+0xa0u)+sx16(cluster)*0xb0u;
  auto remaining=maximum-1u;
  if(std::int16_t(maximum)>0)out(slot++,cluster);
  const auto stamp=r32(m,flood_stamp),mark=flood_marks+sx16(cluster)*4u;
  if(r32(m,mark)!=stamp)w32(m,mark,stamp);
  if(s32(r32(m,record+0x8cu))<=0)return 1;
  std::uint32_t count=1;
  for(std::int16_t i=0;s32(sx16(std::uint16_t(i)))<s32(r32(m,record+0x8cu));++i) {
    const auto index=r16(m,r32(m,record+0x90u)+sx16(std::uint16_t(i))*2u);
    const auto portal=r32(m,sbsp+0x60u)+sx16(index)*0x24u;
    const auto a=r16(m,portal);
    const auto next=a==cluster?r16(m,portal+2u):a;
    if(r32(m,flood_marks+sx16(next)*4u)==r32(m,flood_stamp))continue;
    if(!portal_intersects_sphere(m,index,center,sbsp,radius,fpu))continue;
    const auto n=cluster_portal_flood(m,next,center,radius,remaining,out,slot,fpu)&0xffffu;
    slot+=sx16(std::uint16_t(n));count+=n;remaining-=n;
  }
  return count&0xffffu;
}

std::uint32_t clusters_from_bits(const M& m,const std::function<bool(std::uint32_t)>& bit,std::uint16_t maximum,
    const ClusterSink& out,std::uint16_t& total) {
  std::uint32_t stored=0;total=0;
  const auto sbsp=r32(m,structure_bsp);
  for(std::uint32_t i=0;s32(i)<s32(r32(m,sbsp+0x9cu));++i) {
    if(!bit(i))continue;
    if(std::int16_t(stored)<std::int16_t(maximum)){out(stored,std::uint16_t(i));++stored;}
    ++total;
  }
  return stored;
}

void cluster_partition_reconnect(M& m,const ClusterSelection& selection,const ClusterLinks& links,const PlacementFp& fpu) {
  std::array<std::uint16_t,0x100> clusters{};
  const ClusterSink sink=[&](std::uint32_t slot,std::uint16_t value) {
    if(slot>=clusters.size())refuse("1D2F10","a cluster list beyond 256 entries");
    clusters[slot]=value;
  };
  std::uint16_t stored{},total{};
  if(selection.forced || selection.all) {
    const auto bits=selection.forced;
    stored=std::uint16_t(clusters_from_bits(m,[&](std::uint32_t i) {
      return selection.all || (r32(m,bits+std::uint32_t(s32(i)>>5)*4u)>>(i&31u)&1u);
    },0x100,sink,total));
  } else {
    const auto cluster=r16(m,links.location+4u);
    std::uint32_t count=0;
    if(cluster!=0xffffu) {
      if(above(f32(links.radius),f32(zero_bits),fpu.mxcsr)) {
        w32(m,flood_stamp,r32(m,flood_stamp)+1u);
        w8(m,flood_active,1);
        count=cluster_portal_flood(m,cluster,links.center,f32(links.radius),0x100,sink,0,fpu);
        w8(m,flood_active,0);
      } else {count=1;clusters[0]=cluster;}
    }
    total=std::uint16_t(count);
    stored=std::int16_t(count)>0x100?0x100:std::uint16_t(count);
  }
  if(links.overflow)w8(m,links.overflow,std::int16_t(total)>std::int16_t(stored));
  if(std::int16_t(stored)<=0)return;
  for(std::uint32_t n=0;n<stored;++n) {
    const auto k=sx16(clusters[n]);
    const auto refs=r32(m,links.partition+8u);
    if(const auto r=native_tu15_data_array_allocate(m,refs);r!=none) {
      const auto row=r32(m,refs+0x44u)+(r&0xffffu)*r32(m,refs+0x24u);
      w32(m,row+4u,k);w32(m,row+8u,r32(m,links.owner_head));w32(m,links.owner_head,r);
    }
    const auto head=r32(m,links.partition)+k*4u;
    const auto entries=r32(m,links.partition+4u);
    if(const auto e=native_tu15_data_array_allocate(m,entries);e!=none) {
      const auto row=(e&0xffffu)*r32(m,entries+0x24u)+r32(m,entries+0x44u);
      w32(m,row+4u,links.owner);w32(m,row+8u,r32(m,head));w32(m,head,e);
      if(!links.payload.empty()) {
        const auto size=links.payload_size;
        for(std::uint32_t d=0;d<size>>2;++d) {
          std::uint32_t word{};for(unsigned b=0;b<4;++b)word|=std::uint32_t(links.payload[d*4u+b])<<(8u*b);
          w32(m,row+0xcu+d*4u,word);
        }
        for(std::uint32_t b=0;b<(size&3u);++b)w8(m,row+0xcu+(size&~3u)+b,std::uint8_t(links.payload[(size&~3u)+b]));
      }
    }
  }
}

void light_reconnect(M&,std::uint32_t light,PlacementFp&);
// C3750 (owner datum, detach, reconnect; RET C): every live light of the
// light pool (0x4ECBDC: +24 stride, +38 count, +44 rows, +48 live bits)
// whose owner (+58) is `owner`, in index order: detached (C5250's steps,
// `native_tu15_detach_light_attachment`) and then reconnected (C4D30). The
// light count it returns is unused by C0F60.
void owned_lights(M& m,std::uint32_t owner,bool detach,bool reconnect,PlacementFp& fpu) {
  const auto pool=r32(m,0x4ecbdcu);
  for(std::uint32_t index=0xffffffffu;;) {
    auto n=index+1u;
    if(std::int32_t(n)<0)return;
    const auto count=r32(m,pool+0x38u);
    if(std::int32_t(n)>=std::int32_t(count))return;
    const auto bits=r32(m,pool+0x48u);
    while(!((r32(m,bits+(std::uint32_t(std::int32_t(n)>>5))*4u)>>(n&31u))&1u))
      if(std::int32_t(++n)>=std::int32_t(count))return;
    const auto row=r32(m,pool+0x24u)*n+r32(m,pool+0x44u);
    const auto datum=(std::uint32_t(std::int32_t(std::int16_t(r16(m,row))))<<16)|n;
    index=n;
    if(r32(m,row+0x58u)!=owner)continue;
    if(detach)native_tu15_detach_light_attachment(m,datum);
    if(reconnect)light_reconnect(m,datum,fpu);
  }
}
// ---- Activation, physics creation and BA630 ----
namespace {
constexpr std::uint32_t object_globals=0x4eabc4u;   // word +4: active objects with body flag bit 14
constexpr std::uint32_t game_options=0x4f3218u;
constexpr std::uint32_t physics_components=0x52be5cu; // 160-byte component rows at +44
bool type_bit(const M& m,std::uint32_t header,std::uint32_t mask){return ((1u<<(r8(m,header+3u)&31u))&mask)!=0;}
std::uint32_t component_flags(const M& m,std::uint32_t component) {
  return r32(m,r32(m,r32(m,physics_components)+0x44u)+(component&0xffffu)*160u+4u);
}
// 1CC310: the havok track's owner, the one place its Havok blockers
// (1D9510, 1D73D0, 1D97F0) are bound; 1D8470 is native there.
void component_set_flags(M& m,std::uint32_t object,std::uint32_t flags,PlacementFp& fpu) {
  havok_component_state_flags(m,object,flags,fpu);
}
// 118120: machine "in active clusters" type callback (definition +48).
bool machine_in_clusters(M& m,std::uint32_t object,std::uint32_t bits,bool& result) {
  const auto body=object_body(m,object);
  const auto count=r16(m,body+0x1dcu);
  if(std::int16_t(count)<=0)return false;
  auto index=s32(sx16(r16(m,body+0x1dau)));
  const auto end=s32(sx16(count))+index;
  const auto sbsp=r32(m,structure_bsp);
  // 118163's JGE (first >= end) cannot fire: end = first + count, count > 0.
  const auto portals=r32(m,r32(m,sbsp+0x220u)+0xcu),rows=r32(m,sbsp+0x60u);
  const auto set=[&](std::uint32_t cluster) {
    const auto c=s32(sx16(std::uint16_t(cluster)));
    return (r32(m,bits+std::uint32_t(c>>5)*4u)&(1u<<(std::uint32_t(c)&31u)))!=0;
  };
  for(;index<end;++index) {
    const auto portal=rows+sx16(r16(m,portals+(std::uint32_t(index)&0xffffu)*2u))*0x24u;
    if(set(r16(m,portal))||set(r16(m,portal+2u))){result=true;return true;}
  }
  return true;
}
// 10DDE0: the object type chain's +48 callbacks (literal 0x474B90 table of
// the core track) until one answers.
bool type_in_clusters(M& m,std::uint32_t object,std::uint32_t bits,bool& result) {
  const auto type=sx8(r8(m,object_body(m,object)+0xaau));
  if(type>12u)refuse("10DDE0","an object type outside 0..12");
  const auto chain=object_type_chain(type);
  const auto callbacks=object_type_callbacks(type,0x48u);
  bool answered=false;
  for(std::size_t level=0;level<chain.size() && chain[level];++level) {
    if(const auto callback=callbacks[level]) {
      if(callback!=0x118120u)refuse("10DDE0","object type callback "+std::to_string(callback));
      answered=machine_in_clusters(m,object,bits,result);
    }
    if(answered)break;
  }
  return answered;
}
// 1CC520: body +C0 bit 0 (word OR) and the counter at [52BE44].
void object_count_physics(M& m,std::uint32_t object) {
  const auto body=object_body(m,object);
  if(r8(m,body+0xc0u)&1u)return;
  w16(m,body+0xc0u,std::uint16_t(r16(m,body+0xc0u)|1u));
  const auto counter=r32(m,0x52be44u);w32(m,counter,r32(m,counter)+1u);
}
// BCFB0: the root ancestor (over +14), or NONE.
std::uint32_t root_object(const M& m,std::uint32_t object) {
  if(object==none)return none;
  std::uint32_t root;
  do{root=object;object=r32(m,object_body(m,object)+0x14u);}while(object!=none);
  return root;
}
}

bool object_in_active_clusters(M& m,std::uint32_t object,std::uint32_t bits) {
  const auto header=object_row(m,object);
  bool result=false;
  if((r32(m,r32(m,header+8u)+4u)>>1)&1u)return true;
  if(type_bit(m,header,0x80u) && type_in_clusters(m,object,bits,result))return result;
  const auto cluster=r16(m,header+4u);
  if(cluster==0xffffu)return result;
  const auto c=s32(sx16(cluster));
  return (r32(m,bits+std::uint32_t(c>>5)*4u)&(1u<<(std::uint32_t(c)&31u)))!=0;
}

void object_activate(M& m,std::uint32_t object,PlacementFp& fpu) {
  const auto header=object_row(m,object);
  const auto flags=r8(m,header+2u);
  if(flags&1u)return;
  if(r16(m,header+4u)==0xffffu)return;
  const auto body=r32(m,header+8u);
  if(r32(m,body+0x14u)!=none)return;
  w8(m,header+2u,flags|1u);
  if(type_bit(m,header,0x1883u)) {   // 1CC3F0
    const auto component=r32(m,object_body(m,object)+0xb4u);
    if(component!=none && !((component_flags(m,component)>>17)&1u))component_set_flags(m,object,0,fpu);
  }
  if((r32(m,body+4u)>>14)&1u){const auto g=r32(m,object_globals);w16(m,g+4u,std::uint16_t(r16(m,g+4u)+1u));}
}

void object_deactivate(M& m,std::uint32_t object,PlacementFp& fpu) {
  const auto header=object_row(m,object);
  const auto flags=r8(m,header+2u);
  if(!(flags&1u))return;
  const auto body=r32(m,header+8u);
  w8(m,header+2u,flags&0xfau);
  if(type_bit(m,header,0x1883u)) {   // 1CC440
    const auto row=object_row(m,object),current=r32(m,row+8u);
    if(type_bit(m,row,1u) && r8(m,current+0x34bu)==1u && r8(m,current+0x3dcu)==3u)
      refuse("E3E20","biped deactivation while seated");
    const auto component=r32(m,current+0xb4u);
    if(component!=none && ((component_flags(m,component)>>17)&1u))component_set_flags(m,object,0,fpu);
  }
  if((r32(m,body+4u)>>14)&1u){const auto g=r32(m,object_globals);w16(m,g+4u,std::uint16_t(r16(m,g+4u)-1u));}
}

bool object_delete_protected(const M& m,std::uint32_t object) {
  return native_tu15_object_delete_protected(m,object);
}

void object_create_physics(M& m,std::uint32_t object,PlacementFp& fpu) {
  const auto rows=r32(m,r32(m,object_pool)+0x44u);
  const auto header=rows+(object&0xffffu)*12u;
  if(!type_bit(m,header,0x1883u))return;
  if(r8(m,header+2u)&0x10u)return;
  const auto root=rows+(root_object(m,object)&0xffffu)*12u;
  const auto body=r32(m,header+8u);
  w8(m,body+0xc1u,r8(m,body+0xc1u)&0xefu);
  if(r16(m,root+4u)==0xffffu)return;
  if(r8(m,0x48b870u))refuse("1CC190","Havok component creation (1D7360, 1D73D0, 1D97F0)");
  object_count_physics(m,object);
  (void)fpu;
}

void process_dirty_islands(M& m,PlacementFp& fpu) {
  NativeTu15DirtyIslandObjectServices services;
  services.set_component_flags=[&](std::uint32_t datum,std::uint32_t flags,PlacementFp& f,unsigned){component_set_flags(m,datum,flags,f);};
  services.collect_memory=[&](PlacementFp& f,unsigned){physics_memory_reserve(m,f.mxcsr);};
  services.destroy_component=[](std::uint32_t,PlacementFp&,unsigned){refuse("1D7CA0","Havok component destruction");};
  services.free_component=[&](std::uint32_t pool,std::uint32_t datum,PlacementFp&,unsigned){native_tu15_data_array_delete(m,pool,datum);};
  services.delete_object=[](std::uint32_t,PlacementFp&,unsigned){refuse("BA570","object deletion");};
  services.remove_component=[&m](std::uint32_t component,PlacementFp& f,unsigned){havok_component_remove(m,component,f);};
  services.defer_object=[&](std::uint32_t datum,PlacementFp& f,unsigned){object_defer_physics(m,datum,f);};
  native_tu15_process_dirty_island_objects(m,fpu,0,services);
}

void object_create_physics_tree(M& m,std::uint32_t root,std::uint32_t object,PlacementFp& fpu) {
  const auto body=object_body(m,object);
  auto child=r32(m,body+0x10u);
  if(r32(m,body+0xb4u)==none) {
    physics_memory_reserve(m,fpu.mxcsr);
    object_create_physics(m,object,fpu);
    process_dirty_islands(m,fpu);
    physics_memory_reserve(m,fpu.mxcsr);
  }
  while(child!=none) {
    const auto current=object_body(m,child);
    if(child==root)break;
    object_create_physics_tree(m,root,child,fpu);
    child=r32(m,current+0xcu);
  }
}

void object_reconnect_to_map(M& m,std::uint32_t object,std::uint32_t location,PlacementFp& fpu) {
  const auto header=object_row(m,object),body=r32(m,header+8u);
  const bool unclustered=r16(m,header+4u)==0xffffu;
  std::uint32_t flags;
  bool clustered;
  if(location) {
    clustered=r16(m,location+4u)!=0xffffu;
    if(clustered) {
      w32(m,body+0x28u,r32(m,location));
      const auto second=r32(m,location+4u);
      w32(m,body+0x2cu,second);w16(m,header+4u,std::uint16_t(second));
    }
  } else {
    auto local=location_of_point(m,body+0x40u,fpu.mxcsr);
    if(local.cluster==0xffffu)local=location_of_point(m,body+0x64u,fpu.mxcsr);
    clustered=local.cluster!=0xffffu;
    if(clustered) {
      w32(m,body+0x28u,local.leaf);
      const auto second=std::uint32_t(local.cluster)|std::uint32_t(local.bsp)<<16;
      w32(m,body+0x2cu,second);w16(m,header+4u,local.cluster);
    }
  }
  flags=clustered?r32(m,body+4u)&~0x40000u:r32(m,body+4u)|0x40000u;
  w32(m,body+4u,flags);
  ClusterSelection selection;
  if((flags>>21)&1u) {
    const auto dwords=s32(r32(m,r32(m,structure_bsp)+0x9cu)+0x1fu)>>5;
    if(dwords>16)refuse("BA630","an all-cluster vector beyond its 512-cluster buffer");
    selection.all=true;
  }
  const auto packet=native_tu15_build_object_spatial_packet(m,object);
  ClusterLinks links;
  links.partition=(r32(m,body+4u)>>9)&1u?0x4eabb0u:0x4eaba4u;
  links.owner=object;links.owner_head=body+0x60u;links.center=body+0x40u;links.radius=r32(m,body+0x4cu);
  links.location=body+0x28u;links.payload_size=0x14u;links.payload=packet.bytes;
  cluster_partition_reconnect(m,selection,links,fpu);
  w32(m,body+4u,r32(m,body+4u)|0x100u);
  w8(m,header+2u,r8(m,header+2u)|0x40u);
  NativeTu15ObjectDetachmentServices services;
  services.delete_attachment=[&](std::uint32_t light,PlacementFp& f){light_reconnect(m,light,f);};
  services.detach_children=[&m](std::uint32_t owner,std::uint32_t detach,std::uint32_t reconnect,PlacementFp& f) {
    owned_lights(m,owner,(detach&0xffu)!=0,(reconnect&0xffu)!=0,f);
  };
  native_tu15_detach_object_tree(m,{object,0,1,0,0},fpu,services);
  if(object_in_active_clusters(m,object,r32(m,game_options)+0x11b8u))object_activate(m,object,fpu);
  else if(((r32(m,body+4u)>>17)&1u) && !object_delete_protected(m,object))refuse("BA570","object deletion");
  else object_deactivate(m,object,fpu);
  if(unclustered && r16(m,header+4u)!=0xffffu) {
    physics_memory_reserve(m,fpu.mxcsr);
    object_create_physics_tree(m,object,object,fpu);
    process_dirty_islands(m,fpu);
    physics_memory_reserve(m,fpu.mxcsr);
  }
}
}

// ---- Relocation driver (B95D0 / B9460) ----
namespace halo2::native_object_factory_tu15 {
namespace {
std::uint32_t component_word(const M& m,const PlacementVector& v,unsigned k) {
  return v.local?(*v.local)[k]:r32(m,v.address+4u*k);
}
void store_component(M& m,const PlacementVector& v,unsigned k,std::uint32_t value) {
  if(v.local)(*v.local)[k]=value;else w32(m,v.address+4u*k,value);
}
// 30BD0: normalize in place; returns the XMM0 magnitude bits (0 when the
// x87 length is below the epsilon). The x87 length runs under the live
// control word with the original's instruction bytes (30BD1..30BF1).
std::uint32_t normalize(M& m,const PlacementVector& v,PlacementFp& fpu) {
  const auto z=component_word(m,v,2),y=component_word(m,v,1),x=component_word(m,v,0);
  const auto mode=fp::x87_control(fpu.control_word);const auto epsilon=normalize_epsilon;
  std::uint16_t saved{};std::uint32_t magnitude{};std::uint8_t too_small{};
  asm volatile("fnstcw %[saved]\n\tfldcw %[mode]\n\tflds %[z]\n\tflds %[y]\n\tflds %[x]\n\t"
      ".byte 0xd9,0xc0,0xd8,0xc9,0xd9,0xc2,0xd8,0xcb,0xde,0xc1,0xd9,0xc3,0xd8,0xcc,0xde,0xc1,0xd9,0xfa,"
      "0xdd,0xdb,0xdd,0xd8,0xdd,0xd8\n\t"
      "fsts %[magnitude]\n\tfabs\n\tflds %[epsilon]\n\tfcomip %%st(1), %%st\n\tfstp %%st(0)\n\tseta %[small]\n\t"
      "fldcw %[saved]"
      :[magnitude]"=m"(magnitude),[small]"=q"(too_small),[saved]"+m"(saved)
      :[mode]"m"(mode),[z]"m"(z),[y]"m"(y),[x]"m"(x),[epsilon]"m"(epsilon)
      :"st","st(1)","st(2)","st(3)","st(4)","cc");
  if(too_small)return 0;
  const auto mxcsr=fpu.mxcsr;
  const auto reciprocal=fp::divss(f32(one_bits),f32(magnitude),mxcsr);
  store_component(m,v,0,bits(mul(f32(component_word(m,v,0)),reciprocal,mxcsr)));
  const auto ny=mul(reciprocal,f32(component_word(m,v,1)),mxcsr);
  const auto nz=mul(reciprocal,f32(component_word(m,v,2)),mxcsr);
  store_component(m,v,1,bits(ny));store_component(m,v,2,bits(nz));
  return magnitude;
}
}

void simulation_entity_dirty(M& m,std::uint32_t entity,std::uint32_t mask) {
  const auto owner=r32(m,0x4dc04cu);
  const auto mode=s32(r32(m,owner+8u));
  if(mode!=4 && mode!=5)return;
  if(mode==3 || mode==5)return;
  const auto state=r32(m,owner+4u);
  const auto index=entity&0x3ffu;
  if(!(r8(m,r32(m,state+0x20a4u)+index*8u+0x44u)&4u))return;
  const auto at=state+0x2098u+index*0x24u+0x14u+0xcu;
  w32(m,at,r32(m,at)|mask);
}

void object_update_physics_position(M& m,std::uint32_t object,bool position_absent,bool force) {
  const auto component=r32(m,object_body(m,object)+0xb4u);
  if(component==none)return;
  const auto flags=component_flags(m,component);
  if(((flags>>1)&1u) && position_absent)return;
  if((flags&0x100u) || (flags&0x80u)) {
    if(!force || (flags&0x100u))return;
  }
  refuse("1DC270","Havok body transform update");
}

void biped_relocation_filter(M& m,std::uint32_t object,const PlacementVector& forward,const PlacementVector& up,PlacementFp& fpu) {
  const auto body=object_body(m,object);
  if(r32(m,body+0x14u)!=none)return;
  if((r32(m,tag_data(m,r32(m,body))+0x264u)>>3)&1u)return;
  if(r8(m,body+0x3dcu)==3u)return;
  const auto mxcsr=fpu.mxcsr;
  if(up.present()) {
    const auto global=r32(m,0x474d10u);
    if(!(cmp(f32(component_word(m,up,2)),mxcsr)==cmp(rf(m,global+8u),mxcsr)))   // UCOMISS: an ordered equal keeps it
      for(unsigned k=0;k<3;++k)store_component(m,up,k,r32(m,global+4u*k));
  }
  if(!forward.present())return;
  if(cmp(f32(component_word(m,forward,2)),mxcsr)==0.f)return;
  store_component(m,forward,2,0);
  const auto magnitude=normalize(m,forward,fpu);
  if(cmp(f32(magnitude),mxcsr)==f32(zero_bits)) {
    const auto global=r32(m,0x474d08u);
    for(unsigned k=0;k<3;++k)store_component(m,forward,k,r32(m,global+4u*k));
  }
}

void object_type_relocation(M& m,std::uint32_t object,const PlacementVector& position,const PlacementVector& forward,
    const PlacementVector& up,PlacementFp& fpu) {
  (void)position;
  const auto type=sx8(r8(m,object_body(m,object)+0xaau));
  if(type>12u)refuse("10E340","an object type outside 0..12");
  const auto chain=object_type_chain(type);
  const auto callbacks=object_type_callbacks(type,0x68u);
  for(std::size_t level=0;level<chain.size() && chain[level];++level) {
    if(!callbacks[level])continue;
    if(callbacks[level]!=0xe24f0u)refuse("10E340","object type relocation callback "+std::to_string(callbacks[level]));
    biped_relocation_filter(m,object,forward,up,fpu);
  }
}

bool object_relocate(M& m,std::uint32_t object,const PlacementVector& position,const PlacementVector& forward,
    const PlacementVector& up,const RelocationOptions& options,PlacementFp& fpu,const PlacementServices& services) {
  const auto objects=r32(m,object_pool);
  const auto offset=(object&0xffffu)*12u;
  const auto body=r32(m,r32(m,objects+0x44u)+offset+8u);
  const bool connected=(r32(m,body+4u)>>8)&1u;
  bool position_ok=true;
  if(connected && !options.keep_links)object_disconnect_from_map(m,object,fpu);
  const auto entity=[&]{return r32(m,r32(m,r32(m,objects+0x44u)+offset+8u)+0xd4u);};
  if(position.present()) {
    bool inside=true;
    if(r32(m,body+0x14u)==none) {
      const auto mxcsr=fpu.mxcsr;
      const auto lower=f32(world_lower),upper=f32(world_upper);
      inside=above_or_equal(f32(component_word(m,position,0)),lower,mxcsr) &&
             above_or_equal(upper,f32(component_word(m,position,0)),mxcsr) &&
             above_or_equal(f32(component_word(m,position,1)),lower,mxcsr) &&
             above_or_equal(upper,f32(component_word(m,position,1)),mxcsr) &&
             above_or_equal(f32(component_word(m,position,2)),lower,mxcsr) &&
             above_or_equal(upper,f32(component_word(m,position,2)),mxcsr);
    }
    if(inside) {
      for(unsigned k=0;k<3;++k)w32(m,body+0x64u+4u*k,component_word(m,position,k));
      if(const auto e=entity();e!=none)simulation_entity_dirty(m,e,2);
    } else position_ok=false;
  }
  if(forward.present()) {
    for(unsigned k=0;k<3;++k)w32(m,body+0x70u+4u*k,component_word(m,forward,k));
    for(unsigned k=0;k<3;++k)w32(m,body+0x7cu+4u*k,component_word(m,up,k));
    if(const auto e=entity();e!=none)simulation_entity_dirty(m,e,4);
  }
  if(options.update_physics)object_update_physics_position(m,object,!position.present(),options.physics_force&0xffu);
  if(options.rebuild_nodes) {
    if(services.rebuild_nodes)services.rebuild_nodes(object);
    else object_rebuild_nodes(m,object,fpu);   // BF050 (track nodebuild)
  }
  if(connected && !options.keep_links)object_reconnect_to_map(m,object,options.location,fpu);
  return position_ok;
}

void object_set_position(M& m,std::uint32_t object,std::uint32_t position,std::uint32_t forward,std::uint32_t up,
    std::uint32_t location,std::uint32_t physics_force,PlacementFp& fpu,const PlacementServices& services) {
  std::array<std::uint32_t,3> local_position{},local_forward{},local_up{};
  PlacementVector p,f,u;
  if(position){for(unsigned k=0;k<3;++k)local_position[k]=r32(m,position+4u*k);p.local=&local_position;}
  if(forward){for(unsigned k=0;k<3;++k)local_forward[k]=r32(m,forward+4u*k);f.local=&local_forward;}
  if(up){for(unsigned k=0;k<3;++k)local_up[k]=r32(m,up+4u*k);u.local=&local_up;}
  object_type_relocation(m,object,p,f,u,fpu);
  RelocationOptions options;options.location=location;options.physics_force=physics_force;
  if(!object_relocate(m,object,p,f,u,options,fpu,services)) {
    if(r8(m,r32(m,game_options)+0xcu)!=4u || r32(m,object_body(m,object)+0xd4u)==none)
      refuse("BA570","deletion of an object relocated outside the world bounds");
  }
  object_mark_for_update(m,object);
}
}

// ---- Object markers (BAD60 and its leaves) ----
namespace halo2::native_object_factory_tu15 {
namespace {
constexpr std::uint32_t two_bits=0x40000000u;   // 0x46A11C
// Caller-local stack data (the original's frame buffers) at a reserved
// address range, so native owners that take addresses can fill it.
class ScratchMemory final:public SourcePhysicsContactMemory {
public:
  static constexpr std::uint32_t base=0xfffd0000u,size=0x1000u;
  explicit ScratchMemory(M& world):world_(world){}
  void read(std::uint32_t address,std::span<std::byte> output)const override {
    if(address-base<size){for(std::size_t n=0;n<output.size();++n)output[n]=bytes_.at(address-base+n);return;}
    world_.read(address,output);
  }
  void write(std::uint32_t address,std::span<const std::byte> input)override {
    if(address-base<size){for(std::size_t n=0;n<input.size();++n)bytes_.at(address-base+n)=input[n];return;}
    world_.write(address,input);
  }
private:
  M& world_;std::array<std::byte,size> bytes_{};
};
}

std::uint32_t object_attachment_marker(const M& m,std::uint32_t object,std::uint16_t attachment) {
  const auto definition=tag_data(m,r32(m,object_body(m,object)));
  if(std::int16_t(attachment)<0)return none;
  const auto index=sx16(attachment);
  if(s32(index)>=s32(r32(m,definition+0x94u)))return none;
  return r32(m,r32(m,definition+0x98u)+index*24u+8u);
}

void object_root_location(M& m,std::uint32_t object,std::uint32_t out) {
  auto root=none;
  for(auto current=object;current!=none;current=r32(m,object_body(m,current)+0x14u))root=current;
  const auto body=object_body(m,root);
  w32(m,out,r32(m,body+0x28u));w32(m,out+4u,r32(m,body+0x2cu));
}

std::uint32_t object_marker_owner(const M& m,std::uint32_t object) {
  for(;;) {
    const auto body=object_body(m,object);
    const auto parent=r32(m,body+0x14u);
    if(parent!=none && (r8(m,tag_data(m,r32(m,body))+2u)>>4)&1u){object=parent;continue;}
    // FC510 (ECX object): an item (types 2..4, +4 bit 0) in parent mode
    // (+12C bit 0 without bit 1) carries its markers on +154.
    if((r8(m,body+4u)&1u) && ((1u<<(r8(m,body+0xaau)&31u))&0x1cu)) {
      const auto mode=r8(m,object_body(m,object)+0x12cu);
      if((mode&1u) && !(mode&2u))return r32(m,body+0x154u);
    }
    return object;
  }
}

std::uint32_t model_marker_index(const M& m,std::uint32_t model,std::uint32_t name) {
  if(model==none || name==none || name==0)return none;
  const auto data=tag_data(m,model);
  const auto count=s32(r32(m,data+0x58u));
  if(count<=0)return none;
  const auto rows=r32(m,data+0x5cu);
  for(std::int32_t n=0;n<count;++n)if(r32(m,rows+std::uint32_t(n)*12u)==name)return std::uint32_t(n);
  return none;
}

void quaternion_rotation(M& m,std::uint32_t quaternion,std::uint32_t out,std::uint32_t mxcsr) {
  const auto at=[&](unsigned k){return rf(m,quaternion+4u*k);};
  const auto yy=mul(at(1),at(1),mxcsr),xx=mul(at(0),at(0),mxcsr),zz=mul(at(2),at(2),mxcsr);
  const auto norm=add(add(add(xx,yy,mxcsr),zz,mxcsr),mul(at(3),at(3),mxcsr),mxcsr);
  const auto s=above(norm,f32(normalize_epsilon),mxcsr)?fp::divss(f32(two_bits),norm,mxcsr):0.f;
  const auto xs=mul(at(0),s,mxcsr),ys=mul(at(1),s,mxcsr),zs=mul(s,at(2),mxcsr);
  const auto wz=mul(zs,at(3),mxcsr),wx=mul(xs,at(3),mxcsr);
  const auto xxs=mul(at(0),xs,mxcsr),xys=mul(at(0),ys,mxcsr),xzs=mul(at(0),zs,mxcsr);
  const auto yys=mul(at(1),ys,mxcsr),wy=mul(ys,at(3),mxcsr),yzs=mul(at(1),zs,mxcsr);
  const auto zzs=mul(zs,at(2),mxcsr);
  const auto one=f32(one_bits);
  const auto zy=add(zzs,yys,mxcsr),yx=add(yys,xxs,mxcsr);
  w32(m,out,bits(sub(one,zy,mxcsr)));
  w32(m,out+0xcu,bits(sub(xys,wz,mxcsr)));
  w32(m,out+4u,bits(add(xys,wz,mxcsr)));
  const auto zx=add(zzs,xxs,mxcsr);
  w32(m,out+0x18u,bits(add(xzs,wy,mxcsr)));
  w32(m,out+0x10u,bits(sub(one,zx,mxcsr)));
  w32(m,out+0x1cu,bits(sub(yzs,wx,mxcsr)));
  w32(m,out+8u,bits(sub(xzs,wy,mxcsr)));
  w32(m,out+0x14u,bits(add(yzs,wx,mxcsr)));
  w32(m,out+0x20u,bits(sub(one,yx,mxcsr)));
}

std::uint32_t model_marker_matrices(M& m,std::uint32_t index,std::uint32_t model,std::uint32_t regions,
    std::uint32_t node_map,std::uint32_t nodes,bool mirrored,std::uint32_t out,std::uint32_t maximum,std::uint32_t mxcsr) {
  std::uint32_t found=0;
  if(index==none)return 0;
  const auto group=r32(m,tag_data(m,model)+0x5cu)+index*12u;
  if(s32(r32(m,group+4u))<=0)return 0;
  auto entry=out;
  for(std::int32_t n=0;n<s32(r32(m,group+4u));++n) {
    const auto marker=r32(m,group+8u)+std::uint32_t(n)*0x24u;
    if(regions) {
      const auto region=r8(m,marker);
      if(region!=0xffu && r8(m,regions+sx8(region))!=r8(m,marker+1u))continue;
    }
    if(s32(found)>=s32(maximum))break;
    const auto current=entry;entry+=0x70u;++found;
    auto node=std::uint32_t(r8(m,marker+2u));
    if(node_map)node=r32(m,node_map+node*4u);  // the first-person node map (dwords; the low word is stored)
    w16(m,current,std::uint16_t(node));
    const auto matrix=current+4u;
    quaternion_rotation(m,marker+0x10u,matrix+4u,mxcsr);
    w32(m,matrix+0x28u,0);w32(m,matrix+0x2cu,0);w32(m,matrix+0x30u,0);
    w32(m,matrix,one_bits);
    for(unsigned k=0;k<3;++k)w32(m,matrix+0x28u+4u*k,r32(m,marker+4u+4u*k));
    matrix_product(m,nodes+sx16(r16(m,current))*0x34u,matrix,current+0x38u,mxcsr);
    w32(m,current+0x6cu,r32(m,marker+0x20u));
    if(mirrored)for(unsigned k=0;k<3;++k)w32(m,current+0x48u+4u*k,bits(sub(0.f,rf(m,current+0x48u+4u*k),mxcsr)));
  }
  return found;
}

std::uint32_t object_marker_matrices(M& m,std::uint32_t object,std::uint32_t name,std::uint16_t maximum,
    bool owner_given,std::uint32_t out,std::uint32_t mxcsr) {
  const auto owner=owner_given?object:object_marker_owner(m,object);
  if(owner!=none) {
    const auto body=object_body(m,owner);
    const auto model=r32(m,tag_data(m,r32(m,body))+0x38u);
    if(model!=none) {
      const auto regions=sx16(r16(m,body+0x11au))+body;
      const auto nodes=sx16(r16(m,body+0x116u))+body;
      const bool mirrored=(r32(m,body+4u)>>10)&1u;
      const auto render=r32(m,tag_data(m,model)+4u);
      const auto index=model_marker_index(m,render,name);
      const auto found=model_marker_matrices(m,index,render,regions,0,nodes,mirrored,out,sx16(maximum),mxcsr);
      if(found&0xffffu)return found&0xffffu;
    }
  }
  const auto body=object_body(m,object);
  w16(m,out,0);
  w32(m,out+4u,one_bits);w32(m,out+8u,one_bits);
  for(unsigned k=0;k<3;++k)w32(m,out+0xcu+4u*k,0);
  w32(m,out+0x18u,one_bits);
  for(unsigned k=0;k<3;++k)w32(m,out+0x1cu+4u*k,0);
  w32(m,out+0x28u,one_bits);
  for(unsigned k=0;k<3;++k)w32(m,out+0x2cu+4u*k,0);
  const auto nodes=sx16(r16(m,object_body(m,object)+0x116u))+object_body(m,object);
  for(unsigned k=0;k<13;++k)w32(m,out+0x38u+4u*k,r32(m,nodes+4u*k));
  w32(m,out+0x6cu,0);
  if((r32(m,body+4u)>>10)&1u)for(unsigned k=0;k<3;++k)w32(m,out+0x48u+4u*k,bits(sub(0.f,rf(m,out+0x48u+4u*k),mxcsr)));
  return name?0u:1u;
}
}

// ---- Light reconnection (C4D30) ----
namespace halo2::native_object_factory_tu15 {
namespace {
constexpr std::uint32_t light_pool=0x4ecbdcu,light_partition=0x4ecbe0u;
constexpr std::uint32_t global_zero_vector2d=0x474cd4u;  // [0x474CD4]: the global zero 2-D vector
constexpr std::uint32_t half_bits=0x3f000000u;      // 0x46A0CC
constexpr std::uint32_t minus_one_bits=0xbf800000u; // 0x46A0DC
constexpr std::uint32_t frustum_minimum=0x322bcc76u; // 0x46A610
std::uint32_t light_row(const M& m,std::uint32_t light){return r32(m,r32(m,light_pool)+0x44u)+(light&0xffffu)*0x110u;}
void copy3(M& m,std::uint32_t to,std::uint32_t from){for(unsigned k=0;k<3;++k)w32(m,to+4u*k,r32(m,from+4u*k));}
// 145070 (EAX point, EDX out, stack: matrix; RET 4): out = matrix * point
// (the point scaled first unless the scale is exactly 1; every point lane
// read before the first store).
void transform_point(M& m,std::uint32_t point,std::uint32_t out,std::uint32_t matrix,std::uint32_t mxcsr) {
  const auto scale=rf(m,matrix);
  auto x=rf(m,point),y=rf(m,point+4u),z=rf(m,point+8u);
  if(cmp(scale,mxcsr)!=cmp(1.f,mxcsr)) {  // UCOMISS scale, 1.0; JNP: an ordered equal keeps the point
    x=mul(scale,x,mxcsr);y=mul(rf(m,matrix),y,mxcsr);z=mul(rf(m,matrix),z,mxcsr);
  }
  for(std::uint32_t k=0;k<3u;++k) {
    auto v=add(mul(rf(m,matrix+0x1cu+4u*k),z,mxcsr),mul(rf(m,matrix+0x10u+4u*k),y,mxcsr),mxcsr);
    v=add(add(v,mul(rf(m,matrix+4u+4u*k),x,mxcsr),mxcsr),rf(m,matrix+0x28u+4u*k),mxcsr);
    w32(m,out+4u*k,bits(v));
  }
}
// ---- First-person markers (1692E0 and 16918F) ----
// First-person view state [0x4F6498]: four users of 0x20CC bytes (+8 the
// model variant, two weapon slots of 0x1010 at +C: +0 flags, +4 weapon,
// +F4/+F8, node maps +FC/+1FC, node matrices +308; the view matrix +2060);
// the player rows [0x4F54F4] (+28 local user) and the director rows
// 0x4F5514 (0x140 each: +4 blend, +C mode function).
constexpr std::uint32_t first_person=0x4f6498u,player_pool=0x4f54f4u,directors=0x4f5514u;
// BCDF0: the body of a live object whose type bit is in `mask`, else 0.
std::uint32_t verified_body(const M& m,std::uint32_t object,std::uint32_t mask) {
  if(object==none)return 0;
  const auto pool=r32(m,0x4ecbd0u),index=object&0xffffu;
  if(s32(index)>=s32(r32(m,pool+0x38u)))return 0;
  const auto row=r32(m,pool+0x24u)*index+r32(m,pool+0x44u);
  const auto salt=r16(m,row);
  if(!salt || sx16(salt)!=std::uint32_t(s32(object)>>16))return 0;
  return (mask>>(r8(m,row+3u)&31u))&1u?r32(m,row+8u):0u;
}
// 158290 (EAX user): the director's camera kind: 0 first person (mode
// 245400 with a zero +4), 1 (245EA0), 2 (16F6A0, or 246380 while
// [0x52D6A0]), else 3.
std::uint32_t director_kind(const M& m,std::uint32_t user,std::uint32_t mxcsr) {
  const auto row=directors+user*0x140u;
  const auto mode=r32(m,row+0xcu);
  if(mode==0x245400u)return cmp(rf(m,row+4u),mxcsr)==cmp(0.f,mxcsr)?0u:3u;  // UCOMISS + JP: equal and ordered
  if(mode==0x245ea0u)return 1;
  if(mode==0x16f6a0u || (mode==0x246380u && r8(m,0x52d6a0u)))return 2;
  return 3;
}
// 16918F (stack: weapon, name, out, maximum; RET 10): the marker matrices of
// a weapon held in a first-person slot of a first-person user, through
// the slot's node map and matrices and the user's view matrix; 0 when the
// weapon is not held in first person. Name 0xA00000B6 takes the user's
// hands model ([0x4ECC1C] +13C rows of 0xBC, +4).
std::uint16_t first_person_marker_matrices(M& m,std::uint32_t weapon,std::uint32_t name,std::uint32_t out,
    std::uint16_t maximum,std::uint32_t mxcsr) {
  std::uint32_t count=0;
  const auto body=verified_body(m,weapon,4u);
  if(!body)return 0;
  std::uint32_t user=none;
  {  // 169112: the last user holding the weapon in either slot
    const auto base=r32(m,first_person);
    for(std::uint32_t n=0;n<4u;++n)
      for(std::uint32_t slot=0;slot<2u;++slot)
        if(r32(m,base+0x10u+n*0x20ccu+slot*0x1010u)==weapon){user=n;break;}
  }
  if(user==none || director_kind(m,user,mxcsr))return 0;
  std::uint32_t slot=none;  // 16945E
  for(std::uint32_t n=0;n<2u;++n)
    if(r32(m,r32(m,first_person)+user*0x20ccu+0x10u+n*0x1010u)==weapon){slot=n;break;}
  if(slot==none)return 0;
  const auto view=r32(m,first_person)+user*0x20ccu;
  const auto record=view+slot*0x1010u+0xcu;
  const auto markers=[&](std::uint32_t model,std::uint32_t node_map) {
    const auto index=model_marker_index(m,model,name);
    count=model_marker_matrices(m,index,model,0,node_map,record+0x308u,false,out,sx16(maximum),mxcsr);
  };
  if(name==0xa0000b6u) {
    const auto model=r32(m,r32(m,view+8u)*0xbcu+r32(m,r32(m,0x4ecc1cu)+0x13cu)+4u);
    if((r8(m,record)&4u) && model!=none)markers(model,record+0x1fcu);
  } else {
    // 11E060: the weapon definition's first-person model entry (+2A8 count,
    // +2AC rows of 0x10) for variant +8 (odd variants take the second).
    const auto definition=tag_data(m,r32(m,body));
    const auto variant=r32(m,view+8u);
    const std::uint32_t wanted=variant<=3u?(variant&1u):0u;
    const auto entries=r32(m,definition+0x2a8u);
    std::uint32_t entry=0;
    if(s32(wanted)<s32(entries))entry=r32(m,definition+0x2acu)+wanted*16u;
    else if(s32(entries)>0)entry=r32(m,definition+0x2acu);
    if(entry && (r8(m,record)&2u)) {
      const auto model=r32(m,entry+4u);
      if(model!=none && r32(m,entry+0xcu)!=none)markers(model,record+0xfcu);
    }
  }
  const auto found=sx16(std::uint16_t(count));
  for(std::uint32_t n=0;s32(n)<s32(found);++n)
    matrix_product(m,view+0x2060u,out+0x38u+n*0x70u,out+0x38u+n*0x70u,mxcsr);
  return std::uint16_t(count);
}
// 1692E0 (ECX object; stack: name, position, forward, up; RET 10): for an
// object whose root is a unit of a local player in first person, the first
// marker of the user's slot-0 weapon (16918F) into position (+60), forward
// (+3C) and up (+54) of the entry; false otherwise.
bool first_person_marker(M& m,std::uint32_t object,std::uint32_t name,std::uint32_t position,std::uint32_t forward,
    std::uint32_t up,std::uint32_t mxcsr) {
  const auto root=root_object(m,object);
  const auto player_of=[&](std::uint32_t o){const auto b=verified_body(m,o,3u);return b?r32(m,b+0x13cu):none;};  // 1509B0
  if(player_of(root)==none)return false;
  const auto player=player_of(root);
  const auto user=sx16(r16(m,r32(m,r32(m,player_pool)+0x44u)+(player&0xffffu)*0x21cu+0x28u));
  if(user==none)return false;
  const auto slot=r32(m,first_person)+user*0x20ccu+0xcu;
  if(!(r8(m,slot)&1u))return false;
  ScratchMemory local(m);
  constexpr auto entry=ScratchMemory::base+0xf00u;
  if(!first_person_marker_matrices(local,r32(m,slot+4u),name,entry,1,mxcsr))return false;
  copy3(local,position,entry+0x60u);
  copy3(local,forward,entry+0x3cu);
  copy3(local,up,entry+0x54u);
  return true;
}
// Scratch layout for the frustum path (the original's stack frames).
constexpr std::uint32_t shape_bounds=ScratchMemory::base,shape_sphere=ScratchMemory::base+0x100u,
  shape_corners=ScratchMemory::base+0x200u,corner_far=ScratchMemory::base+0x300u,corner_near=ScratchMemory::base+0x340u,
  local_axis=ScratchMemory::base+0x380u;

// x87 FLD angle; FMUL 0.5; FPTAN; FSTP st0; FST tangent; FMUL aspect; FSTP scaled.
std::pair<float,float> x87_half_tangent(float angle,float aspect,std::uint16_t control) {
  const auto mode=fp::x87_control(control);const float half=f32(half_bits);
  std::uint16_t saved{};float tangent{},scaled{};
  asm volatile("fnstcw %[saved]\n\tfldcw %[mode]\n\tflds %[a]\n\tfmuls %[h]\n\tfptan\n\tfstp %%st(0)\n\t"
      "fsts %[t]\n\tfmuls %[s]\n\tfstps %[u]\n\tfldcw %[saved]"
      :[t]"=m"(tangent),[u]"=m"(scaled),[saved]"+m"(saved):[mode]"m"(mode),[a]"m"(angle),[h]"m"(half),[s]"m"(aspect)
      :"st","st(1)");
  return {tangent,scaled};
}
// x87 length of three floats (z, y, x squared and summed in that order),
// FSQRT, then FMUL scale; FSTP dword.
float x87_scaled_length(float x,float y,float z,float scale,std::uint16_t control) {
  const auto mode=fp::x87_control(control);std::uint16_t saved{};float out{};
  asm volatile("fnstcw %[saved]\n\tfldcw %[mode]\n\tflds %[z]\n\tfmuls %[z]\n\tflds %[y]\n\tfmuls %[y]\n\t.byte 0xde,0xc1\n\t"
      "flds %[x]\n\tfmuls %[x]\n\t.byte 0xde,0xc1\n\tfsqrt\n\tfmuls %[s]\n\tfstps %[o]\n\tfldcw %[saved]"
      :[o]"=m"(out),[saved]"+m"(saved):[mode]"m"(mode),[x]"m"(x),[y]"m"(y),[z]"m"(z),[s]"m"(scale):"st","st(1)");
  return out;
}
// 1E75C6..1E75EF: distance from a center to a point (x87, original bytes).
float x87_distance(const std::array<float,3>& c,const std::array<float,3>& p,std::uint16_t control) {
  const auto mode=fp::x87_control(control);std::uint16_t saved{};float out{};
  asm volatile("fnstcw %[saved]\n\tfldcw %[mode]\n\tflds %[cx]\n\tfsubs %[px]\n\tflds %[cy]\n\tfsubs %[py]\n\t"
      "flds %[cz]\n\tfsubs %[pz]\n\t"
      ".byte 0xd9,0xc0,0xd8,0xc9,0xd9,0xc2,0xd8,0xcb,0xde,0xc1,0xd9,0xc3,0xd8,0xcc,0xde,0xc1,0xd9,0xfa,"
      "0xdd,0xdb,0xdd,0xd8,0xdd,0xd8\n\tfstps %[o]\n\tfldcw %[saved]"
      :[o]"=m"(out),[saved]"+m"(saved):[mode]"m"(mode),[cx]"m"(c[0]),[cy]"m"(c[1]),[cz]"m"(c[2]),
       [px]"m"(p[0]),[py]"m"(p[1]),[pz]"m"(p[2]):"st","st(1)","st(2)","st(3)","st(4)");
  return out;
}
// 1E753F..1E75BD: FLD d2; FSQRT; FMUL 0.5; FSTP dword.
float x87_half_root(float value,std::uint16_t control) {
  const auto mode=fp::x87_control(control);const float half=f32(half_bits);std::uint16_t saved{};float out{};
  asm volatile("fnstcw %[saved]\n\tfldcw %[mode]\n\tflds %[v]\n\tfsqrt\n\tfmuls %[h]\n\tfstps %[o]\n\tfldcw %[saved]"
      :[o]"=m"(out),[saved]"+m"(saved):[mode]"m"(mode),[v]"m"(value),[h]"m"(half):"st");
  return out;
}

float ld(const M& s,std::uint32_t at){return f32(r32(s,at));}
void st(M& s,std::uint32_t at,float value){w32(s,at,bits(value));}
// C3A20 (with C3960, or C39B0 for a scenario light whose +3E bit 0 is set):
// the light's shape record into `out` (0x7C bytes).
std::int16_t light_shape_record(M& s,std::uint32_t light,std::uint32_t out,std::uint32_t mxcsr) {
  const auto row=light_row(s,light);
  const auto definition=tag_data(s,r32(s,row+4u));
  auto entry=none;
  if(const auto scenario=r32(s,row+8u);scenario!=none) {
    entry=scenario*0x6cu+r32(s,r32(s,0x4ecc20u)+0xecu);
    if(!(r8(s,entry+0x3eu)&1u))entry=none;
  }
  for(unsigned k=0;k<0x1fu;++k)w32(s,out+4u*k,0);
  std::uint32_t type{};
  if(entry!=none) {
    // C39B0 (ESI entry, EBX definition): the entry's +3C type; a sphere's
    // +68 size twice, the second scaled by definition +1C / +18 unless +1C
    // is an ordered zero; otherwise the entry's +58..+68.
    type=sx16(r16(s,entry+0x3cu));
    w32(s,out,type);
    if(type&0xffffu)for(unsigned k=0;k<5;++k)w32(s,out+4u+4u*k,r32(s,entry+0x58u+4u*k));
    else {
      const auto size=r32(s,entry+0x68u);
      w32(s,out+4u,size);
      w32(s,out+8u,size);
      if(const auto aspect=ld(s,definition+0x1cu);fp::sse_compared(bits(aspect),mxcsr)!=0.f)
        w32(s,out+8u,bits(mul(fp::divss(aspect,ld(s,definition+0x18u),mxcsr),f32(size),mxcsr)));
    }
  } else {
    type=sx16(r16(s,definition+4u));
    w32(s,out,type);
    if(!(type&0xffffu)){w32(s,out+4u,r32(s,definition+0x18u));w32(s,out+8u,r32(s,definition+0x1cu));}
    else for(unsigned k=0;k<5;++k)w32(s,out+4u+4u*k,r32(s,definition+0x20u+4u*k));
  }
  if(type==1u)w32(s,out+0xcu,0);
  else if(type==3u)w32(s,out+4u,0);
  return std::int16_t(type);
}

// C3AB0: the frustum frame of the shape record `shape` from the light frame
// at `frame` (light +84) and the light definition.
void light_frustum(M& s,std::uint32_t frame,std::uint32_t shape,std::uint32_t definition,const PlacementFp& fpu) {
  const auto mxcsr=fpu.mxcsr;
  if(r16(s,shape)==0)refuse("C3AB0","sphere shape frustum");   // census-unexecuted (C48E0 handles spheres)
  const auto e=shape+0x38u,dir=e+4u;
  st(s,dir,sub(ld(s,frame+0xcu),ld(s,frame),mxcsr));
  st(s,dir+4u,sub(ld(s,frame+0x10u),ld(s,frame+4u),mxcsr));
  st(s,dir+8u,sub(ld(s,frame+0x14u),ld(s,frame+8u),mxcsr));
  const auto dot=add(add(mul(ld(s,frame+0x30u),ld(s,dir+8u),mxcsr),mul(ld(s,frame+0x2cu),ld(s,dir+4u),mxcsr),mxcsr),
      mul(ld(s,frame+0x28u),ld(s,dir),mxcsr),mxcsr);
  st(s,e+0x10u,dot);
  if(f32(normalize_epsilon)>std::fabs(x87_abs(dot)) || above(0.f,dot,mxcsr)) {
    // C3B75: a target on or behind the light's forward plane. The frame's
    // forward axis stands in for the direction, at unit distance.
    copy3(s,dir,frame+0x28u);
    st(s,e+0x10u,f32(one_bits));
  }
  const auto reciprocal=fp::divss(f32(one_bits),ld(s,e+0x10u),mxcsr);
  for(unsigned k=0;k<3;++k)st(s,dir+4u*k,mul(ld(s,dir+4u*k),reciprocal,mxcsr));
  auto aspect=ld(s,shape+8u);
  st(s,e+0x1cu,aspect);
  if(const auto bitmap=r32(s,definition+0x7cu);bitmap!=none) {
    const auto data=r32(s,tag_data(s,bitmap)+0x48u);
    const auto ratio=fp::divss(fp::cvtsi2ss(s32(sx16(r16(s,data+6u))),mxcsr),fp::cvtsi2ss(s32(sx16(r16(s,data+4u))),mxcsr),mxcsr);
    st(s,e+0x1cu,mul(ratio,aspect,mxcsr));
  }
  if(r32(s,shape)==1u) {
    // C3C0B: an orthogonal frustum has no spread: its two slopes come from
    // the global zero 2-D vector ([0x474CD4]).
    const auto zero=r32(s,global_zero_vector2d);
    w32(s,e+0x14u,r32(s,zero));
    w32(s,e+0x18u,r32(s,zero+4u));
  } else {
    // C3C1E: the half-angle's tangent, and the tangent scaled by the aspect.
    const auto [tangent,scaled]=x87_half_tangent(ld(s,shape+0xcu),ld(s,e+0x1cu),fpu.control_word);
    st(s,e+0x14u,tangent);st(s,e+0x18u,scaled);
  }
  w32(s,e+0x20u,r32(s,shape+4u));
  st(s,e+0x24u,mul(ld(s,e+0x1cu),ld(s,shape+4u),mxcsr));
  st(s,e+0x2cu,mul(ld(s,shape+0x14u),ld(s,frame+0x40u),mxcsr));
  st(s,e+0x28u,mul(ld(s,shape+0x10u),ld(s,frame+0x40u),mxcsr));
  const auto depth=ld(s,e+0x2cu);
  st(s,e+0x30u,add(mul(ld(s,e+0x14u),depth,mxcsr),ld(s,e+0x20u),mxcsr));
  st(s,e+0x34u,add(mul(ld(s,e+0x18u),depth,mxcsr),ld(s,e+0x24u),mxcsr));
  const auto far=ld(s,e+0x2cu);
  for(unsigned k=0;k<3;++k)st(s,e+0x38u+4u*k,add(mul(ld(s,dir+4u*k),far,mxcsr),ld(s,frame+4u*k),mxcsr));
  // C3CCE: the frame's left axis (forward x up) into +1C.
  const auto a=sub(mul(ld(s,frame+0x38u),ld(s,frame+0x28u),mxcsr),mul(ld(s,frame+0x2cu),ld(s,frame+0x34u),mxcsr),mxcsr);
  const auto b=sub(mul(ld(s,frame+0x30u),ld(s,frame+0x34u),mxcsr),mul(ld(s,frame+0x3cu),ld(s,frame+0x28u),mxcsr),mxcsr);
  const auto c=sub(mul(ld(s,frame+0x2cu),ld(s,frame+0x3cu),mxcsr),mul(ld(s,frame+0x30u),ld(s,frame+0x38u),mxcsr),mxcsr);
  st(s,shape+0x20u,b);st(s,shape+0x24u,a);st(s,shape+0x1cu,c);
  w32(s,shape+0x28u,0);
  copy3(s,shape+0x2cu,frame);
  if(r32(s,shape)==2u && above(ld(s,e+0x14u),f32(normalize_epsilon),mxcsr)) {
    const auto offset=mul(fp::divss(ld(s,shape+4u),ld(s,e+0x14u),mxcsr),f32(half_bits),mxcsr);
    st(s,shape+0x28u,offset);
    const auto back=sub(0.f,offset,mxcsr);
    for(unsigned k=0;k<3;++k)st(s,shape+0x2cu+4u*k,add(mul(ld(s,dir+4u*k),back,mxcsr),ld(s,frame+4u*k),mxcsr));
  }
  w8(s,shape+0x18u,1);
}

// C3820: the light's frustum record is usable (C3A20 + C3AB0 into `shape`).
bool light_frustum_record(M& s,std::uint32_t light,std::uint32_t shape,bool require_view,const PlacementFp& fpu) {
  const auto mxcsr=fpu.mxcsr;
  const auto row=light_row(s,light);
  const auto definition=tag_data(s,r32(s,row+4u));
  bool visible=true;
  if(!((r32(s,definition)>>8)&1u) && require_view) {
    const auto user=r32(s,0x4f63b8u);
    if(r32(s,0x56dfa0u+r32(s,user+0xc14u)*4u))visible=!((r32(s,user)>>1)&1u);
  }
  if(!r8(s,r32(s,0x51dae8u)+8u) || !visible || !(r8(s,row+2u)&2u))return false;
  light_shape_record(s,light,shape,mxcsr);
  light_frustum(s,row+0x84u,shape,definition,fpu);
  if(r16(s,shape)==0)return above(ld(s,shape+0x44u),f32(normalize_epsilon),mxcsr);
  if(!above(ld(s,shape+0x64u),f32(normalize_epsilon),mxcsr))return false;
  const auto spread=add(mul(ld(s,shape+0x68u),ld(s,shape+0x68u),mxcsr),mul(ld(s,shape+0x6cu),ld(s,shape+0x6cu),mxcsr),mxcsr);
  return above(spread,f32(frustum_minimum),mxcsr);
}

// C55D0: the frustum's eight corners (near then far quad) into `out`.
bool light_frustum_corners(M& s,std::uint32_t light,std::uint32_t out,PlacementFp& fpu) {
  const auto mxcsr=fpu.mxcsr;
  const auto frame=light_row(s,light)+0x84u;
  const auto S=shape_corners;
  if(!light_frustum_record(s,light,S,true,fpu))return false;
  const auto width=add(mul(mul(ld(s,S+0x4cu),ld(s,S+0x14u),mxcsr),f32(two_bits),mxcsr),ld(s,S+0x58u),mxcsr);
  const auto height=mul(ld(s,S+0x54u),width,mxcsr);
  std::array<std::uint32_t,3> axis{r32(s,S+0x3cu),r32(s,S+0x40u),r32(s,S+0x44u)};
  PlacementVector v;v.local=&axis;normalize(s,v,fpu);
  std::array<float,3> near{},far{};
  for(unsigned k=0;k<3;++k) {
    near[k]=add(mul(f32(axis[k]),0.f,mxcsr),ld(s,frame+4u*k),mxcsr);
    far[k]=add(mul(f32(axis[k]),ld(s,S+0x14u),mxcsr),ld(s,frame+4u*k),mxcsr);
  }
  constexpr std::array<unsigned,4> signs{0,1,3,2};
  for(unsigned n=0;n<4;++n) {
    const auto sx=mul((signs[n]&1u)?f32(one_bits):f32(minus_one_bits),f32(half_bits),mxcsr);
    const auto sy=mul((signs[n]&2u)?f32(one_bits):f32(minus_one_bits),f32(half_bits),mxcsr);
    const auto a=corner_far+n*12u,b=corner_near+n*12u;
    for(unsigned k=0;k<3;++k)st(s,a+4u*k,far[k]);
    const auto nx=mul(sx,ld(s,S+0x58u),mxcsr),fx=mul(sx,width,mxcsr);
    for(unsigned k=0;k<3;++k)st(s,b+4u*k,near[k]);
    st(s,b,add(mul(ld(s,S+0x1cu),nx,mxcsr),near[0],mxcsr));
    st(s,b+4u,add(mul(ld(s,S+0x20u),nx,mxcsr),ld(s,b+4u),mxcsr));
    st(s,b+8u,add(mul(ld(s,S+0x24u),nx,mxcsr),ld(s,b+8u),mxcsr));
    st(s,a,add(mul(fx,ld(s,S+0x1cu),mxcsr),ld(s,a),mxcsr));
    st(s,a+4u,add(mul(ld(s,S+0x20u),fx,mxcsr),ld(s,a+4u),mxcsr));
    st(s,a+8u,add(mul(ld(s,S+0x24u),fx,mxcsr),ld(s,a+8u),mxcsr));
    const auto ny=mul(sy,ld(s,S+0x5cu),mxcsr),fy=mul(sy,height,mxcsr);
    for(unsigned k=0;k<3;++k)st(s,b+4u*k,add(mul(ld(s,frame+0x34u+4u*k),ny,mxcsr),ld(s,b+4u*k),mxcsr));
    for(unsigned k=0;k<3;++k)st(s,a+4u*k,add(mul(ld(s,frame+0x34u+4u*k),fy,mxcsr),ld(s,a+4u*k),mxcsr));
  }
  for(unsigned k=0;k<12;++k)w32(s,out+4u*k,r32(s,corner_near+4u*k));
  for(unsigned k=0;k<12;++k)w32(s,out+0x30u+4u*k,r32(s,corner_far+4u*k));
  return true;
}
}

// 1E7330: the sphere through the farthest pair of `count` points, grown to
// hold every point; center and radius stored to memory.
void points_bounding_sphere(M& s,std::uint32_t points,std::uint32_t count,std::uint32_t center,std::uint32_t radius,
    const PlacementFp& fpu) {
  const auto mxcsr=fpu.mxcsr;
  const auto n=s32(count);
  if(n<=1) {
    w32(s,radius,0);
    if(n==0)copy3(s,center,r32(s,0x474ce8u));
    else copy3(s,center,points);
    return;
  }
  float best=0.f;std::uint32_t first=0,second=0;
  const auto p=[&](std::uint32_t i,unsigned k){return ld(s,points+i*12u+4u*k);};
  for(std::int32_t i=0;i<n;++i)
    for(std::int32_t j=i+1;j<n;++j) {
      const auto dx=sub(p(i,0),p(j,0),mxcsr),dy=sub(p(i,1),p(j,1),mxcsr),dz=sub(p(i,2),p(j,2),mxcsr);
      const auto d2=add(add(mul(dz,dz,mxcsr),mul(dx,dx,mxcsr),mxcsr),mul(dy,dy,mxcsr),mxcsr);
      if(above(d2,best,mxcsr)){best=d2;second=std::uint32_t(j);first=std::uint32_t(i);}
    }
  const auto half=f32(half_bits);
  const auto cx=mul(add(p(first,0),p(second,0),mxcsr),half,mxcsr);
  const auto cy=mul(add(p(second,1),p(first,1),mxcsr),half,mxcsr);
  st(s,center,cx);st(s,center+4u,cy);
  st(s,center+8u,mul(add(p(second,2),p(first,2),mxcsr),half,mxcsr));
  const auto root=x87_half_root(best,fpu.control_word);
  float grown=0.f;
  for(std::int32_t i=0;i<n;++i) {
    const std::array<float,3> c{ld(s,center),ld(s,center+4u),ld(s,center+8u)};
    const auto d=x87_distance(c,{p(std::uint32_t(i),0),p(std::uint32_t(i),1),p(std::uint32_t(i),2)},fpu.control_word);
    if(d>root && above(d,grown,mxcsr))grown=d;   // FCOMIP (exact), then COMISS
  }
  st(s,radius,above(grown,0.f,mxcsr)?grown:root);
}

void light_update_sphere(M& m,std::uint32_t light,PlacementFp& fpu) {
  ScratchMemory s(m);
  const auto row=light_row(m,light);
  if(light_shape_record(s,light,shape_sphere,fpu.mxcsr)==0) {
    copy3(m,row+0x28u,row+0x18u);
    w32(m,row+0x34u,r32(m,row+0x24u));
    return;
  }
  if(light_frustum_corners(s,light,shape_sphere,fpu))
    points_bounding_sphere(s,shape_sphere,8,row+0x28u,row+0x34u,fpu);
  else {
    w32(m,row+0x34u,0);
    copy3(m,row+0x28u,r32(m,0x474ce8u));
  }
}

void light_update_bounds(M& m,std::uint32_t light,PlacementFp& fpu) {
  const auto mxcsr=fpu.mxcsr;
  ScratchMemory s(m);
  const auto row=light_row(m,light);
  const auto definition=tag_data(m,r32(m,row+4u));
  const auto B=shape_bounds;
  if(light_shape_record(s,light,B,mxcsr)==0) {
    // C4932: the record's sizes (+4, +8: the definition's, or C39B0's).
    float size=ld(s,B+4u);
    if(!(r8(m,definition)&2u)) {
      const auto other=ld(s,B+8u);
      size=above(size,other,mxcsr)?size:other;
    }
    auto radius=mul(rf(m,definition+0xcu),size,mxcsr);
    if(!above(radius,rf(m,definition+0x98u),mxcsr))radius=rf(m,definition+0x98u);
    const auto extra=rf(m,row+0x9cu);
    if(above(extra,f32(zero_bits),mxcsr)) {
      copy3(m,row+0x18u,row+0xa0u);
      w32(m,row+0x24u,bits(add(extra,radius,mxcsr)));
    } else {
      copy3(m,row+0x18u,row+0x84u);
      w32(m,row+0x24u,bits(radius));
    }
    copy3(m,row+0x38u,row+0x18u);
    copy3(m,row+0x28u,row+0x18u);
    w32(m,row+0x34u,r32(m,row+0x24u));
    light_update_sphere(m,light,fpu);
    return;
  }
  // C4A00: the frustum's far-plane extent around its axis.
  light_frustum(s,row+0x84u,B,definition,fpu);
  const auto width=add(mul(mul(ld(s,B+0x4cu),ld(s,B+0x14u),mxcsr),f32(two_bits),mxcsr),ld(s,B+0x58u),mxcsr);
  const auto height=mul(ld(s,B+0x54u),width,mxcsr);
  const auto dz=ld(s,B+0x44u),dy=ld(s,B+0x40u),dx=ld(s,B+0x3cu);
  const auto side=add(add(mul(dz,ld(s,B+0x24u),mxcsr),mul(dy,ld(s,B+0x20u),mxcsr),mxcsr),mul(dx,ld(s,B+0x1cu),mxcsr),mxcsr);
  const auto horizontal=above_or_equal(side,0.f,mxcsr)?f32(one_bits):f32(minus_one_bits);
  const auto up=add(add(mul(rf(m,row+0xc0u),dz,mxcsr),mul(rf(m,row+0xbcu),dy,mxcsr),mxcsr),mul(rf(m,row+0xb8u),dx,mxcsr),mxcsr);
  const auto vertical=above_or_equal(up,0.f,mxcsr)?f32(one_bits):f32(minus_one_bits);
  std::array<std::uint32_t,3> axis{bits(dx),bits(dy),bits(dz)};
  PlacementVector v;v.local=&axis;normalize(m,v,fpu);
  const auto far=ld(s,B+0x14u);
  const auto origin=r32(m,0x474ce8u);
  const auto px=add(mul(f32(axis[0]),far,mxcsr),rf(m,origin),mxcsr);
  const auto py=add(mul(f32(axis[1]),far,mxcsr),rf(m,origin+4u),mxcsr);
  const auto pz=add(mul(f32(axis[2]),far,mxcsr),rf(m,origin+8u),mxcsr);
  const auto hs=mul(mul(horizontal,width,mxcsr),f32(half_bits),mxcsr);
  const auto x1=add(mul(hs,ld(s,B+0x1cu),mxcsr),px,mxcsr);
  const auto y1=add(mul(ld(s,B+0x20u),hs,mxcsr),py,mxcsr);
  const auto z1=add(mul(ld(s,B+0x24u),hs,mxcsr),pz,mxcsr);
  const auto vs=mul(mul(vertical,height,mxcsr),f32(half_bits),mxcsr);
  const auto qx=add(mul(rf(m,row+0xb8u),vs,mxcsr),x1,mxcsr);
  const auto qy=add(mul(rf(m,row+0xbcu),vs,mxcsr),y1,mxcsr);
  const auto qz=add(mul(rf(m,row+0xc0u),vs,mxcsr),z1,mxcsr);
  auto radius=x87_scaled_length(qx,qy,qz,rf(m,definition+0xcu),fpu.control_word);
  if(!(radius>rf(m,definition+0x98u)))radius=rf(m,definition+0x98u);   // FCOMIP (exact)
  w32(m,row+0x24u,bits(radius));
  if(above(rf(m,row+0x9cu),f32(zero_bits),mxcsr))copy3(m,row+0x18u,row+0xa0u);
  else copy3(m,row+0x18u,row+0x84u);
  copy3(m,row+0x38u,row+0x18u);
  light_update_sphere(m,light,fpu);
}

namespace {
// C2873..C28C5: the x87 offsets target - position (FSTP dword each; the z
// term squares the extended z by its stored float), the FSQRT length (FST
// dword) and 1e-4 > |length| (FCOMIP of the extended length).
struct FrameRay {std::array<float,4> d;bool near;};
FrameRay x87_frame_ray(const M& m,std::uint32_t target,std::uint32_t position,std::uint16_t control) {
  const std::array<float,3> t{rf(m,target),rf(m,target+4u),rf(m,target+8u)};
  const std::array<float,3> p{rf(m,position),rf(m,position+4u),rf(m,position+8u)};
  const auto mode=fp::x87_control(control);std::uint16_t saved{};FrameRay out{};std::uint8_t near{};
  const auto limit=f32(normalize_epsilon);
  asm volatile("fnstcw %[saved]\n\tfldcw %[mode]\n\t"
      "flds 0(%[t])\n\tfsubs 0(%[p])\n\tfstps 0(%[o])\n\t"
      "flds 4(%[t])\n\tfsubs 4(%[p])\n\tfstps 4(%[o])\n\t"
      "flds 8(%[t])\n\tfsubs 8(%[p])\n\tfsts 8(%[o])\n\tfmuls 8(%[o])\n\t"
      "flds 4(%[o])\n\tfmuls 4(%[o])\n\tfaddp %%st,%%st(1)\n\t"
      "flds 0(%[o])\n\tfmuls 0(%[o])\n\tfaddp %%st,%%st(1)\n\t"
      "fsqrt\n\tfsts 12(%[o])\n\tfabs\n\tflds %[limit]\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\t"
      "seta %[near]\n\tfldcw %[saved]"
      :[saved]"+m"(saved),[near]"=q"(near):[mode]"m"(mode),[t]"r"(t.data()),[p]"r"(p.data()),[o]"r"(out.d.data()),
       [limit]"m"(limit),"m"(t),"m"(p):"st","st(1)","memory","cc");
  out.near=near;
  return out;
}
}

bool light_frame_point(M& m,std::uint32_t position,std::uint32_t target,std::uint32_t point,std::uint32_t distance,
    const PlacementFp& fpu) {
  const auto mxcsr=fpu.mxcsr;
  const auto ray=x87_frame_ray(m,target,position,fpu.control_word);
  if(ray.near)return false;
  const auto length=ray.d[3];
  const auto inverse=fp::divss(1.f,length,mxcsr);
  const std::array<float,3> direction{mul(inverse,ray.d[0],mxcsr),mul(ray.d[1],inverse,mxcsr),mul(ray.d[2],inverse,mxcsr)};
  if(!above(length,0.f,mxcsr))return false;
  const auto index=r16(m,structure_bsp_index);
  float t=0.f,step=f32(0x3a83126fu);                      // 0x46A180 (0.001)
  ScratchMemory local(m);
  const auto at=ScratchMemory::base;
  while(above(length,t,mxcsr)) {
    std::array<float,3> p{};
    for(unsigned k=0;k<3;++k)p[k]=add(mul(direction[k],t,mxcsr),rf(m,position+4u*k),mxcsr);
    if(index!=0xffffu) {
      for(unsigned k=0;k<3;++k)w32(local,at+4u*k,bits(p[k]));
      const auto leaf=bsp3d_leaf_from_point(local,r32(m,collision_bsp),0,at,mxcsr);
      if(leaf!=none && r16(m,r32(m,r32(m,structure_bsp)+0x30u)+leaf*8u)!=0xffffu) {
        w32(m,distance,bits(t));
        for(unsigned k=0;k<3;++k)w32(m,point+4u*k,bits(p[k]));
        return true;
      }
    }
    const auto next=add(step,t,mxcsr);
    step=mul(step,f32(two_bits),mxcsr);
    t=next;
    if(above(step,f32(0x3dcccccdu),mxcsr))step=f32(0x3dcccccdu);   // 0x46A178 (0.1)
  }
  return false;
}

// ---- Collision sphere fit (16D740) ----
// 16D740 fits a sphere into the structure's collision geometry. It gathers
// the collision features near the sphere (16D2A0: the BSP walk 1E60C0 and
// the feature builder 24EDB0), then, for every surface feature the sphere
// reaches, casts rays through the feature set (24FCC0) to find the free span
// around the centre on that surface's side. The spans' midpoints and half
// lengths are averaged into the fitted sphere.
//
// Everything between the inputs and the four output stores lives on the
// original's stack (0xB96C bytes), so the native keeps it in host structs.
// The arithmetic keeps the original's operation order: SSE steps under the
// live MXCSR, x87 steps as the original instruction sequences under the live
// control word.
namespace {
using Vec3=std::array<float,3>;
using Point2=std::array<float,2>;

constexpr std::uint32_t breakable_surfaces=0x4f9b50u;  // [0x4F9B50]: 32 bytes of bits per structure BSP
constexpr std::uint32_t global_origin=0x474ce8u;       // [0x474CE8]: the global zero vector
constexpr std::size_t feature_capacity=0x100u;         // every contact and feature list holds 256
constexpr std::size_t polygon_capacity=8u;             // 24EB60's polygon buffer holds 8 vertices

Vec3 load3(const M& m,std::uint32_t at){return {rf(m,at),rf(m,at+4u),rf(m,at+8u)};}
// COMISS a, b under MXCSR.DAZ: a >= b (ordered).
bool at_least(float a,float b,std::uint32_t mxcsr){return above_or_equal(a,b,mxcsr);}

// ---- x87 sequences (each runs under the live control word) ----
// FLD a; FADD b; FSTP dword.
float x87_add(float a,float b,std::uint16_t control) {
  const auto mode=fp::x87_control(control);std::uint16_t saved{};float out{};
  asm volatile("fnstcw %[saved]\n\tfldcw %[mode]\n\tflds %[a]\n\tfadds %[b]\n\tfstps %[o]\n\tfldcw %[saved]"
      :[o]"=m"(out),[saved]"+m"(saved):[mode]"m"(mode),[a]"m"(a),[b]"m"(b):"st");
  return out;
}
// FLD a; FSUB b; FSTP dword.
float x87_sub(float a,float b,std::uint16_t control) {
  const auto mode=fp::x87_control(control);std::uint16_t saved{};float out{};
  asm volatile("fnstcw %[saved]\n\tfldcw %[mode]\n\tflds %[a]\n\tfsubs %[b]\n\tfstps %[o]\n\tfldcw %[saved]"
      :[o]"=m"(out),[saved]"+m"(saved):[mode]"m"(mode),[a]"m"(a),[b]"m"(b):"st");
  return out;
}
// The length of a vector summed (x*x + z*z) + y*y in extended precision:
// FSQRT, FST dword, and whether the epsilon (1e-4) exceeds the extended
// |length| (FABS, FCOMIP).
struct X87Length {float length;bool below_epsilon;};
X87Length x87_length_xzy(const Vec3& v,std::uint16_t control) {
  const auto mode=fp::x87_control(control);const auto epsilon=f32(normalize_epsilon);
  std::uint16_t saved{};float out{};std::uint8_t small{};
  asm volatile("fnstcw %[saved]\n\tfldcw %[mode]\n\t"
      "flds %[x]\n\tfmuls %[x]\n\tflds %[z]\n\tfmuls %[z]\n\tfaddp %%st,%%st(1)\n\t"
      "flds %[y]\n\tfmuls %[y]\n\tfaddp %%st,%%st(1)\n\tfsqrt\n\tfsts %[o]\n\tfabs\n\t"
      "flds %[e]\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %[small]\n\tfldcw %[saved]"
      :[o]"=m"(out),[small]"=q"(small),[saved]"+m"(saved)
      :[mode]"m"(mode),[x]"m"(v[0]),[y]"m"(v[1]),[z]"m"(v[2]),[e]"m"(epsilon):"st","st(1)","cc");
  return {out,small!=0};
}
// 30BD1..30BF1: the length (x*x + y*y) + z*z of the original's FLD z, y, x
// stack, FSQRT, FST dword, and the extended |length| against 1e-4.
X87Length x87_length_xyz(const Vec3& v,std::uint16_t control) {
  const auto mode=fp::x87_control(control);const auto epsilon=f32(normalize_epsilon);
  std::uint16_t saved{};float out{};std::uint8_t small{};
  asm volatile("fnstcw %[saved]\n\tfldcw %[mode]\n\tflds %[z]\n\tflds %[y]\n\tflds %[x]\n\t"
      ".byte 0xd9,0xc0,0xd8,0xc9,0xd9,0xc2,0xd8,0xcb,0xde,0xc1,0xd9,0xc3,0xd8,0xcc,0xde,0xc1,0xd9,0xfa,"
      "0xdd,0xdb,0xdd,0xd8,0xdd,0xd8\n\t"
      "fsts %[o]\n\tfabs\n\tflds %[e]\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %[small]\n\tfldcw %[saved]"
      :[o]"=m"(out),[small]"=q"(small),[saved]"+m"(saved)
      :[mode]"m"(mode),[x]"m"(v[0]),[y]"m"(v[1]),[z]"m"(v[2]),[e]"m"(epsilon)
      :"st","st(1)","st(2)","st(3)","st(4)","cc");
  return {out,small!=0};
}
// FLD v; FSQRT; FSTP dword.
float x87_sqrt(float v,std::uint16_t control) {
  const auto mode=fp::x87_control(control);std::uint16_t saved{};float out{};
  asm volatile("fnstcw %[saved]\n\tfldcw %[mode]\n\tflds %[v]\n\tfsqrt\n\tfstps %[o]\n\tfldcw %[saved]"
      :[o]"=m"(out),[saved]"+m"(saved):[mode]"m"(mode),[v]"m"(v):"st");
  return out;
}
// 24F596..24F5AC: the ray's entry b - sqrt(disc) (FSQRT, FSUBR, FST dword)
// and whether a >= the extended entry (FCOMIP; NaN fails).
struct X87Entry {float entry;bool within;};
X87Entry x87_ray_entry(float b,float disc,float a,std::uint16_t control) {
  const auto mode=fp::x87_control(control);std::uint16_t saved{};float out{};std::uint8_t within{};
  asm volatile("fnstcw %[saved]\n\tfldcw %[mode]\n\tflds %[d]\n\tfsqrt\n\tfsubrs %[b]\n\tfsts %[o]\n\t"
      "flds %[a]\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tsetae %[w]\n\tsetnp %%al\n\tandb %%al,%[w]\n\tfldcw %[saved]"
      :[o]"=m"(out),[w]"=q"(within),[saved]"+m"(saved):[mode]"m"(mode),[d]"m"(disc),[b]"m"(b),[a]"m"(a)
      :"st","st(1)","cc","eax");
  return {out,within!=0};
}
// 24F9C3..24FA34: the plane normal against the ray, ((n.z*V.z) + (n.y*V.y))
// + V.x*n.x in extended precision, FST dword, and whether 1e-4 exceeds the
// extended |dot|.
X87Length x87_plane_dot(const Vec3& n,const Vec3& v,std::uint16_t control) {
  const auto mode=fp::x87_control(control);const auto epsilon=f32(normalize_epsilon);
  std::uint16_t saved{};float out{};std::uint8_t small{};
  asm volatile("fnstcw %[saved]\n\tfldcw %[mode]\n\t"
      "flds %[nz]\n\tfmuls %[vz]\n\tflds %[ny]\n\tfmuls %[vy]\n\tfaddp %%st,%%st(1)\n\t"
      "flds %[vx]\n\tfmuls %[nx]\n\tfaddp %%st,%%st(1)\n\tfsts %[o]\n\tfabs\n\t"
      "flds %[e]\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %[small]\n\tfldcw %[saved]"
      :[o]"=m"(out),[small]"=q"(small),[saved]"+m"(saved)
      :[mode]"m"(mode),[nx]"m"(n[0]),[ny]"m"(n[1]),[nz]"m"(n[2]),[vx]"m"(v[0]),[vy]"m"(v[1]),[vz]"m"(v[2]),
       [e]"m"(epsilon):"st","st(1)","cc");
  return {out,small!=0};
}
// FLD dword; FABS; FCOMIP against 1e-4: the epsilon exceeds |value| (exact;
// x87 compares see denormals, NaN fails).
bool below_epsilon(float value){return f32(normalize_epsilon)>std::fabs(value);}
// The same against 0.01 (16DD16: a centre this close to the plane is on it).
bool on_plane(float distance){return f32(surface_distance)>std::fabs(distance);}

// 123210 on a value: the dominant axis of `v` (the x87 |z| >= |y| test,
// then COMISS against |x|).
std::uint16_t dominant_axis_of(const Vec3& v,std::uint32_t mxcsr) {
  const auto x=x87_abs(v[0]),y=x87_abs(v[1]),z=x87_abs(v[2]);
  if(z>=y && at_least(z,x,mxcsr))return 2;
  return at_least(y,x,mxcsr)?1:0;
}
// The (u, v) coordinates a plane with dominant `axis` projects onto, by the
// sign the projection keeps (0x44AC8C rows).
std::array<std::uint16_t,2> projection(std::uint16_t axis,bool positive) {
  const auto& row=projection_rows[std::size_t(axis)*2u+(positive?1u:0u)];
  return {row[0],row[1]};
}

// 30BD0 on a value: normalize in place; returns the length (0 when it is
// below the epsilon, and the vector is then left as it was).
float normalize3(Vec3& v,const PlacementFp& fpu) {
  const auto [length,small]=x87_length_xyz(v,fpu.control_word);
  if(small)return 0.f;
  const auto mxcsr=fpu.mxcsr;
  const auto inverse=fp::divss(f32(one_bits),length,mxcsr);
  v[0]=mul(v[0],inverse,mxcsr);
  v[1]=mul(inverse,v[1],mxcsr);
  v[2]=mul(inverse,v[2],mxcsr);
  return length;
}

// ---- The collision BSP ----
// The structure's collision BSP ([0x4ECC10]) as the sphere fit reads it:
//   +4  3-D nodes, 8 bytes: int16 plane, two signed 24-bit children at +1/+4
//       (bit 23 marks a leaf; NONE is no leaf)
//   +C  planes, 16 bytes: normal, offset
//   +14 leaves, 4 bytes: +1 BSP2D reference count, +2 the first reference
//   +1C BSP2D references, 4 bytes: int16 plane (bit 15: the plane's back
//       side), int16 BSP2D node
//   +24 BSP2D nodes, 16 bytes: the line a, b, c; int16 children +C/+E
//       (negative: a surface, bits 0..14)
//   +2C surfaces, 8 bytes (count +28): int16 plane (bit 15 flips it), +2
//       the first edge, +4 flags (bit 3 breakable), +5 the breakable
//       surface index
//   +34 edges, 12 bytes: vertices +0/+2, the next edge +4/+6 and the
//       surface +8/+A on each side
//   +3C vertices, 16 bytes: the point
struct CollisionBsp {
  const M& m;
  std::uint32_t at;
  std::uint32_t table(std::uint32_t offset)const{return r32(m,at+offset);}
  std::uint32_t node(std::uint32_t index)const{return table(4u)+index*8u;}
  std::int32_t node_plane(std::uint32_t index)const{return std::int16_t(r16(m,node(index)));}
  std::uint32_t node_child(std::uint32_t index,unsigned side)const{return child(m,node(index),side);}
  std::uint32_t plane(std::uint32_t index)const{return table(0xcu)+index*16u;}
  Vec3 plane_normal(std::uint32_t index)const{return load3(m,plane(index));}
  float plane_offset(std::uint32_t index)const{return rf(m,plane(index)+0xcu);}
  std::uint32_t leaf(std::uint32_t index)const{return table(0x14u)+index*4u;}
  std::uint32_t leaf_first_reference(std::uint32_t index)const{return r16(m,leaf(index)+2u);}
  std::uint32_t leaf_references(std::uint32_t index)const{return r8(m,leaf(index)+1u);}
  std::uint32_t reference(std::uint32_t index)const{return table(0x1cu)+index*4u;}
  std::uint16_t reference_plane(std::uint32_t index)const{return r16(m,reference(index));}
  std::int32_t reference_node(std::uint32_t index)const{return std::int16_t(r16(m,reference(index)+2u));}
  std::uint32_t bsp2d(std::uint32_t index)const{return table(0x24u)+index*16u;}
  std::int32_t surface_count()const{return s32(r32(m,at+0x28u));}
  std::uint32_t surface(std::uint32_t index)const{return table(0x2cu)+index*8u;}
  std::uint16_t surface_plane(std::uint32_t index)const{return r16(m,surface(index));}
  std::uint32_t surface_first_edge(std::uint32_t index)const{return r16(m,surface(index)+2u);}
  std::uint8_t surface_flags(std::uint32_t index)const{return r8(m,surface(index)+4u);}
  std::uint8_t surface_breakable(std::uint32_t index)const{return r8(m,surface(index)+5u);}
  std::uint32_t edge(std::uint32_t index)const{return table(0x34u)+index*12u;}
  std::uint32_t edge_vertex(std::uint32_t index,unsigned side)const{return r16(m,edge(index)+2u*side);}
  std::uint32_t edge_next(std::uint32_t index,unsigned side)const{return r16(m,edge(index)+4u+2u*side);}
  std::int32_t edge_surface(std::uint32_t index,unsigned side)const{return std::int16_t(r16(m,edge(index)+8u+2u*side));}
  // The side of edge `index` that bounds `surface` (+A names it: side 1).
  unsigned edge_side(std::uint32_t index,std::uint32_t surface)const{return edge_surface(index,1)==std::int32_t(surface)?1u:0u;}
  std::uint32_t vertex(std::uint32_t index)const{return table(0x3cu)+index*16u;}
  Vec3 vertex_point(std::uint32_t index)const{return load3(m,vertex(index));}
};

// Appends `value` unless the list has it (or is full), as the original's
// linear searches with their 256-entry caps.
void add_unique(std::vector<std::uint32_t>& list,std::uint32_t value) {
  if(std::find(list.begin(),list.end(),value)!=list.end() || list.size()>=feature_capacity)return;
  list.push_back(value);
}

// 120FA0 (EAX centre, ECX start, EDX direction, stack: radius): the segment
// start + [0, 1] * direction touches the sphere.
bool segment_touches_sphere(const Vec3& center,const Vec3& start,const Vec3& direction,float radius,std::uint32_t mxcsr) {
  const auto dy=sub(start[1],center[1],mxcsr),dz=sub(start[2],center[2],mxcsr),dx=sub(start[0],center[0],mxcsr);
  const auto outside=sub(add(add(mul(dz,dz,mxcsr),mul(dy,dy,mxcsr),mxcsr),mul(dx,dx,mxcsr),mxcsr),mul(radius,radius,mxcsr),mxcsr);
  if(above(0.f,outside,mxcsr))return true;  // the start lies inside
  const auto along=add(add(mul(direction[2],dz,mxcsr),mul(direction[1],dy,mxcsr),mxcsr),mul(direction[0],dx,mxcsr),mxcsr);
  if(at_least(along,0.f,mxcsr))return false;  // leaving the sphere
  const auto length2=add(add(mul(direction[2],direction[2],mxcsr),mul(direction[1],direction[1],mxcsr),mxcsr),
      mul(direction[0],direction[0],mxcsr),mxcsr);
  const auto disc=sub(mul(along,along,mxcsr),mul(length2,outside,mxcsr),mxcsr);
  if(at_least(0.f,disc,mxcsr))return false;
  const auto rest=sub(sub(0.f,length2,mxcsr),along,mxcsr);
  if(above(0.f,rest,mxcsr))return true;
  return above(disc,mul(rest,rest,mxcsr),mxcsr);
}

// ---- 1E60C0: the collision features a sphere touches ----
// The surfaces (+0), edges (+404) and vertices (+808) of the collision BSP
// within the sphere, and the leaves it reaches (+C0C), in discovery order.
struct SphereContacts {
  std::vector<std::uint32_t> surfaces,edges,vertices,leaves;
  bool any()const{return !surfaces.empty() || !edges.empty() || !vertices.empty();}
};

// 1E60C0's walk state record from its plane stack on (frame +0x18..+0x227):
// the stack depth, 128 slots of straddled node planes, then the projection
// of the current BSP2D reference: the dominant axis (word +0x21C), the
// flipped byte (+0x21E), the walk's stop byte (+0x21F) and the projected
// point (+0x220, +0x224).
//
// The original never bounds the stack. A straddle chain deeper than 128
// planes writes slots 128..130 over the projection state, and the native
// keeps that overlap: slot 128 holds the axis, flipped and stop bytes (a
// plane marked with bit 15 sets the stop byte, which ends the walk once a
// leaf is recorded), slots 129 and 130 are the projected point (a later
// projection overwrites those slots' planes). Slot 131 would be 1E60C0's
// return address: there the original faults.
struct WalkRecord {
  static constexpr std::size_t stack_slots=128u,state_slots=3u;
  std::int32_t depth=0;
  std::array<std::uint32_t,stack_slots+state_slots> slots{};  // 1E60C0 clears the stop byte only
  std::uint16_t axis()const{return std::uint16_t(slots[stack_slots]);}
  bool flipped()const{return std::uint8_t(slots[stack_slots]>>16)!=0;}
  bool stopped()const{return (slots[stack_slots]>>24)!=0;}
  Point2 point()const{return {f32(slots[stack_slots+1u]),f32(slots[stack_slots+2u])};}
  // 1E6434..1E6491: the projection's stores (the stop byte is kept).
  void project(std::uint16_t axis,bool flipped,const Point2& point) {
    auto& state=slots[stack_slots];
    state=(state&0xff000000u)|(flipped?0x10000u:0u)|axis;
    slots[stack_slots+1u]=bits(point[0]);
    slots[stack_slots+2u]=bits(point[1]);
  }
  // 1E623D / 1E627E: the plane of the straddled node at the current depth.
  void set_top(std::uint32_t plane) {
    if(std::size_t(depth)>=slots.size())
      refuse("1E6150","a collision plane stack deeper than 131 entries (the original overwrites 1E60C0's return address)");
    slots[std::size_t(depth)]=plane;
  }
  // 1E6345..1E635F: `plane` (the sign-extended reference word) is on the stack.
  bool holds(std::int32_t plane)const {
    for(std::int32_t k=0;k<depth;++k)if(std::int32_t(slots[std::size_t(k)])==plane)return true;
    return false;
  }
};

// The BSP walk (1E6150, 1E64D0, 1E6570) over its state record.
class SphereContactWalk {
public:
  SphereContactWalk(const CollisionBsp& bsp,const Vec3& center,float radius,std::uint32_t breakable,
      std::int16_t breakable_limit,std::uint32_t mxcsr)
      :bsp_(bsp),center_(center),radius_(radius),breakable_(breakable),breakable_limit_(breakable_limit),mxcsr_(mxcsr){}
  SphereContacts contacts;
  // 1E6150: a 3-D node or leaf reference.
  void visit(std::uint32_t reference) {
    if(reference&0x800000u){visit_leaf(reference);return;}
    const auto plane=std::uint32_t(bsp_.node_plane(reference));
    const auto front=bsp_.node_child(reference,0),back=bsp_.node_child(reference,1);
    const auto distance=node_distance(bsp_.plane(plane));
    const bool short_of_front=above(radius_,distance,mxcsr_);
    if(!above(distance,sub(0.f,radius_,mxcsr_),mxcsr_)){visit(front);return;}
    if(!short_of_front){visit(back);return;}
    // Straddling: each side is walked with the plane on the stack, marked
    // with bit 15 (sign-extended) while the first child is walked. A set
    // stop byte ends the walk once a leaf is recorded: the original returns
    // without popping (1E624F, 1E628A).
    record_.set_top(plane|0xffff8000u);
    ++record_.depth;
    visit(front);
    if(stopped())return;
    --record_.depth;
    record_.set_top(r16(bsp_.m,bsp_.node(reference))&0x7fffu);
    ++record_.depth;
    visit(back);
    if(stopped())return;
    --record_.depth;
  }
private:
  bool stopped()const{return record_.stopped() && !contacts.leaves.empty();}
  // The centre's distance from a node plane: ((c.z*p.z + c.y*p.y) +
  // p.x*c.x) - p.w (1E61DD..1E6204).
  float node_distance(std::uint32_t plane)const {
    const auto& m=bsp_.m;const auto& c=center_;
    const auto z=mul(c[2],rf(m,plane+8u),mxcsr_),y=mul(c[1],rf(m,plane+4u),mxcsr_),x=mul(rf(m,plane),c[0],mxcsr_);
    return sub(add(add(z,y,mxcsr_),x,mxcsr_),rf(m,plane+0xcu),mxcsr_);
  }
  void visit_leaf(std::uint32_t reference) {
    if(reference==none)return;
    const auto leaf=reference&0x7fffffu;
    if(contacts.leaves.size()<feature_capacity)contacts.leaves.push_back(leaf);
    // 1E630F: a set stop byte records the leaf only. 1E60C0 clears it, so
    // only a straddle chain deeper than 128 planes sets it (WalkRecord).
    if(record_.stopped())return;
    const auto first=bsp_.leaf_first_reference(leaf);
    for(auto index=first;index<first+bsp_.leaf_references(leaf);++index) {
      // Only the references on a plane the walk straddled, on the same side.
      const auto plane=bsp_.reference_plane(index);
      if(!record_.holds(std::int16_t(plane)))continue;
      project_onto(plane);
      visit_bsp2d(bsp_.reference_node(index));
    }
  }
  // 1E6364..1E6491: the centre projected onto the reference's plane, in the
  // plane's dominant-axis coordinates.
  void project_onto(std::uint16_t reference_plane) {
    const auto plane=std::uint32_t(reference_plane&0x7fffu);
    const auto n=bsp_.plane_normal(plane);
    const Vec3 magnitude{x87_abs(n[0]),x87_abs(n[1]),x87_abs(n[2])};
    const auto& m=bsp_.m;const auto at=bsp_.plane(plane);
    const auto distance=sub(add(add(mul(center_[2],rf(m,at+8u),mxcsr_),mul(center_[1],rf(m,at+4u),mxcsr_),mxcsr_),
        mul(center_[0],rf(m,at),mxcsr_),mxcsr_),rf(m,at+0xcu),mxcsr_);
    const auto t=sub(0.f,distance,mxcsr_);
    const Vec3 q{add(mul(t,n[0],mxcsr_),center_[0],mxcsr_),add(mul(t,n[1],mxcsr_),center_[1],mxcsr_),
        add(mul(n[2],t,mxcsr_),center_[2],mxcsr_)};
    std::uint16_t axis=2;
    if(!(magnitude[2]>=magnitude[1] && at_least(magnitude[2],magnitude[0],mxcsr_)))
      axis=at_least(magnitude[1],magnitude[0],mxcsr_)?1:0;
    const bool positive=above(n[axis],0.f,mxcsr_);
    const bool flipped=positive!=bool(reference_plane>>15);
    const auto [u,v]=projection(axis,flipped);
    record_.project(axis,flipped,{q[u],q[v]});
  }
  // 1E64D0: a BSP2D reference: lines within the radius of the projected
  // point are walked on both sides.
  void visit_bsp2d(std::int32_t reference) {
    const auto point=record_.point();
    while(!(reference&0x8000)) {
      const auto node=bsp_.bsp2d(std::uint32_t(reference)&0x7fffu);
      const auto& m=bsp_.m;
      const auto d=sub(add(mul(rf(m,node+4u),point[1],mxcsr_),mul(point[0],rf(m,node),mxcsr_),mxcsr_),rf(m,node+8u),mxcsr_);
      const bool front=at_least(radius_,d,mxcsr_),back=at_least(d,sub(0.f,radius_,mxcsr_),mxcsr_);
      if(front)visit_bsp2d(std::int16_t(r16(m,node+0xcu)));
      if(!back)return;
      reference=std::int16_t(r16(m,node+0xeu));
    }
    visit_surface(std::uint32_t(reference)&0x7fffu);
  }
  // 1E6570: a surface: its vertices and edges within the sphere, and the
  // surface itself when any is, or when the projected centre lies inside
  // its polygon. A breakable surface whose bit is clear is skipped.
  void visit_surface(std::uint32_t surface) {
    const auto flags=bsp_.surface_flags(surface),breakable=bsp_.surface_breakable(surface);
    if((flags&8u) && std::int16_t(breakable)<breakable_limit_ &&
       !((r32(bsp_.m,breakable_+(breakable>>5)*4u)>>(breakable&31u))&1u))return;
    const auto radius2=mul(radius_,radius_,mxcsr_);
    const auto first=bsp_.surface_first_edge(surface);
    bool touched=false;
    auto edge=first;
    do {  // 1E65D4: vertices
      const auto side=bsp_.edge_side(edge,surface);
      const auto vertex=bsp_.edge_vertex(edge,side);
      const auto p=bsp_.vertex_point(vertex);
      // SUBPS/MULPS over (x, 0, y, z), then (x*x + y*y) + z*z.
      const auto dx=sub(p[0],center_[0],mxcsr_),dy=sub(p[1],center_[1],mxcsr_),dz=sub(p[2],center_[2],mxcsr_);
      const auto distance2=add(add(mul(dx,dx,mxcsr_),mul(dy,dy,mxcsr_),mxcsr_),mul(dz,dz,mxcsr_),mxcsr_);
      if(at_least(radius2,distance2,mxcsr_)){add_unique(contacts.vertices,vertex);touched=true;}
      edge=bsp_.edge_next(edge,side);
    } while(edge!=first);
    do {  // 1E66B0: edges
      const auto side=bsp_.edge_side(edge,surface);
      const auto start=bsp_.vertex_point(bsp_.edge_vertex(edge,side));
      const auto end=bsp_.vertex_point(bsp_.edge_vertex(edge,side^1u));
      const Vec3 direction{sub(end[0],start[0],mxcsr_),sub(end[1],start[1],mxcsr_),sub(end[2],start[2],mxcsr_)};
      if(segment_touches_sphere(center_,start,direction,radius_,mxcsr_)){add_unique(contacts.edges,edge);touched=true;}
      edge=bsp_.edge_next(edge,side);
    } while(edge!=first);
    if(!touched) {  // 1E6794: the projected centre against each edge
      const auto [u,v]=projection(record_.axis(),record_.flipped());
      const auto point=record_.point();
      do {
        const auto side=bsp_.edge_side(edge,surface);
        const auto a=bsp_.vertex_point(bsp_.edge_vertex(edge,side)),b=bsp_.vertex_point(bsp_.edge_vertex(edge,side^1u));
        const auto au=sub(a[u],point[0],mxcsr_),av=sub(a[v],point[1],mxcsr_);
        const auto bv=sub(b[v],point[1],mxcsr_),bu=sub(b[u],point[0],mxcsr_);
        if(above(0.f,sub(mul(bv,au,mxcsr_),mul(av,bu,mxcsr_),mxcsr_),mxcsr_))return;
        edge=bsp_.edge_next(edge,side);
      } while(edge!=first);
    }
    add_unique(contacts.surfaces,surface);
  }
  const CollisionBsp& bsp_;
  Vec3 center_;
  float radius_;
  std::uint32_t breakable_;
  std::int16_t breakable_limit_;
  std::uint32_t mxcsr_;
  WalkRecord record_;
};

// ---- Instanced geometry transforms ----
// An instance's node matrix (instance +0, 0x34 bytes): +0 scale, +4
// forward, +10 left, +1C up, +28 position. A point p of the instance's
// collision BSP lies at position + scale * (forward * p.x + left * p.y +
// up * p.z) in the world.
//
// Every caller sums the products in its own order, and the order is kept
// (each SSE sum rounds; the operand order of each MULSS/ADDSS also decides
// which NaN payload survives). A point is scaled first unless the scale is
// exactly 1 (UCOMISS, LAHF, TEST AH 44, JNP: a NaN scale multiplies).
struct Transform {float scale;Vec3 forward,left,up,position;};
Transform load_transform(const M& m,std::uint32_t at) {
  return {rf(m,at),load3(m,at+4u),load3(m,at+0x10u),load3(m,at+0x1cu),load3(m,at+0x28u)};
}
bool unit_scale(float scale){return scale==f32(one_bits);}
Vec3 scaled(float scale,const Vec3& p,std::uint32_t mxcsr) {
  if(unit_scale(scale))return p;
  return {mul(scale,p[0],mxcsr),mul(scale,p[1],mxcsr),mul(scale,p[2],mxcsr)};
}

// 144090: the inverse transform: the transposed rotation, the reciprocal
// scale, and the negated position through both. A zero scale (UCOMISS:
// also a denormal under DAZ) gives the all-zero transform.
Transform inverse(const Transform& t,std::uint32_t mxcsr) {
  if(cmp(t.scale,mxcsr)==0.f)return {};
  Vec3 back{sub(0.f,t.position[0],mxcsr),sub(0.f,t.position[1],mxcsr),sub(0.f,t.position[2],mxcsr)};
  Transform out{};
  out.scale=f32(one_bits);
  if(!unit_scale(t.scale)) {
    out.scale=fp::divss(f32(one_bits),t.scale,mxcsr);
    back={mul(out.scale,back[0],mxcsr),mul(out.scale,back[1],mxcsr),mul(out.scale,back[2],mxcsr)};
  }
  out.forward={t.forward[0],t.left[0],t.up[0]};
  out.left={t.forward[1],t.left[1],t.up[1]};
  out.up={t.forward[2],t.left[2],t.up[2]};
  // 144129..1441A9: each axis of the original dotted with the scaled
  // negated position, in the order (z + x) + y, (z + x) + y, (x + z) + y.
  const auto& f=t.forward;const auto& l=t.left;const auto& u=t.up;
  out.position={
      add(add(mul(f[2],back[2],mxcsr),mul(f[0],back[0],mxcsr),mxcsr),mul(f[1],back[1],mxcsr),mxcsr),
      add(add(mul(l[2],back[2],mxcsr),mul(l[0],back[0],mxcsr),mxcsr),mul(back[1],l[1],mxcsr),mxcsr),
      add(add(mul(u[0],back[0],mxcsr),mul(u[2],back[2],mxcsr),mxcsr),mul(u[1],back[1],mxcsr),mxcsr)};
  return out;
}

// 16CFBB..16D08A: a world point in an instance's space, through its
// inverse transform. The point is scaled first (point * scale), then each
// lane sums ((forward + left) + up) + position.
Vec3 point_to_instance(const Transform& to_local,const Vec3& point,std::uint32_t mxcsr) {
  auto p=point;
  if(!unit_scale(to_local.scale))
    p={mul(point[0],to_local.scale,mxcsr),mul(point[1],to_local.scale,mxcsr),mul(point[2],to_local.scale,mxcsr)};
  Vec3 out{};
  for(std::size_t k=0;k<3u;++k)
    out[k]=add(add(add(mul(to_local.forward[k],p[0],mxcsr),mul(to_local.left[k],p[1],mxcsr),mxcsr),
        mul(to_local.up[k],p[2],mxcsr),mxcsr),to_local.position[k],mxcsr);
  return out;
}

// 24E710: a vertex into the world: ((up + left) + forward) + position in
// every lane.
Vec3 vertex_to_world(const Transform& t,const Vec3& point,std::uint32_t mxcsr) {
  const auto p=scaled(t.scale,point,mxcsr);
  Vec3 out{};
  for(std::size_t k=0;k<3u;++k)
    out[k]=add(add(add(mul(t.up[k],p[2],mxcsr),mul(t.left[k],p[1],mxcsr),mxcsr),mul(t.forward[k],p[0],mxcsr),mxcsr),
        t.position[k],mxcsr);
  return out;
}

// 24E9A2..24EB0A: an edge's direction (rotated, no position) and its start
// into the world, each lane in the original's order.
Vec3 edge_direction_to_world(const Transform& t,const Vec3& direction,std::uint32_t mxcsr) {
  const auto d=scaled(t.scale,direction,mxcsr);
  const auto& f=t.forward;const auto& l=t.left;const auto& u=t.up;
  return {add(add(mul(f[0],d[0],mxcsr),mul(l[0],d[1],mxcsr),mxcsr),mul(u[0],d[2],mxcsr),mxcsr),
          add(add(mul(u[1],d[2],mxcsr),mul(d[0],f[1],mxcsr),mxcsr),mul(l[1],d[1],mxcsr),mxcsr),
          add(add(mul(f[2],d[0],mxcsr),mul(l[2],d[1],mxcsr),mxcsr),mul(u[2],d[2],mxcsr),mxcsr)};
}
Vec3 edge_start_to_world(const Transform& t,const Vec3& start,std::uint32_t mxcsr) {
  const auto p=scaled(t.scale,start,mxcsr);
  const auto& f=t.forward;const auto& l=t.left;const auto& u=t.up;const auto& o=t.position;
  return {add(add(add(mul(f[0],p[0],mxcsr),mul(u[0],p[2],mxcsr),mxcsr),mul(l[0],p[1],mxcsr),mxcsr),o[0],mxcsr),
          add(add(add(mul(p[0],f[1],mxcsr),mul(u[1],p[2],mxcsr),mxcsr),mul(l[1],p[1],mxcsr),mxcsr),o[1],mxcsr),
          add(add(add(mul(f[2],p[0],mxcsr),mul(u[2],p[2],mxcsr),mxcsr),mul(l[2],p[1],mxcsr),mxcsr),o[2],mxcsr)};
}

// 24EBC0..24EC79: a surface polygon's vertex into the world.
Vec3 polygon_point_to_world(const Transform& t,const Vec3& point,std::uint32_t mxcsr) {
  const auto p=scaled(t.scale,point,mxcsr);
  const auto& f=t.forward;const auto& l=t.left;const auto& u=t.up;const auto& o=t.position;
  return {add(add(add(mul(u[0],p[2],mxcsr),mul(l[0],p[1],mxcsr),mxcsr),mul(p[0],f[0],mxcsr),mxcsr),o[0],mxcsr),
          add(add(add(mul(f[1],p[0],mxcsr),mul(u[1],p[2],mxcsr),mxcsr),mul(l[1],p[1],mxcsr),mxcsr),o[1],mxcsr),
          add(add(add(mul(f[2],p[0],mxcsr),mul(u[2],p[2],mxcsr),mxcsr),mul(l[2],p[1],mxcsr),mxcsr),o[2],mxcsr)};
}

// 24EC87..24ED48: a surface plane into the world: the normal rotated, and
// the offset moved by the position along the new normal plus the scaled
// offset (always scaled: no unit test here).
std::array<float,4> plane_to_world(const Transform& t,const std::array<float,4>& plane,std::uint32_t mxcsr) {
  const auto& f=t.forward;const auto& l=t.left;const auto& u=t.up;const auto& o=t.position;
  const auto x=add(add(mul(f[0],plane[0],mxcsr),mul(u[0],plane[2],mxcsr),mxcsr),mul(l[0],plane[1],mxcsr),mxcsr);
  const auto y=add(add(mul(f[1],plane[0],mxcsr),mul(u[1],plane[2],mxcsr),mxcsr),mul(l[1],plane[1],mxcsr),mxcsr);
  const auto z=add(add(mul(l[2],plane[1],mxcsr),mul(f[2],plane[0],mxcsr),mxcsr),mul(u[2],plane[2],mxcsr),mxcsr);
  const auto w=add(add(add(mul(o[2],z,mxcsr),mul(o[1],y,mxcsr),mxcsr),mul(o[0],x,mxcsr),mxcsr),
      mul(t.scale,plane[3],mxcsr),mxcsr);
  return {x,y,z,w};
}

// ---- 24EDB0: the collision features of the contacts ----
// 16D740 asks for features of zero height and zero radius (16D2A0's
// arguments 3 and 4), so no feature is extruded and every radius is 0. The
// structure's features are in world space; an instance's (16CF00) go
// through its transform.
struct FeatureVertex {Vec3 point;float radius;};                  // 24DFB0: 0x20 rows at +8
struct FeatureEdge {Vec3 start,direction;float radius;};          // 24E0F0: 0x2C rows at +2008
struct FeatureSurface {                                           // 24E580: 0x6C rows at +4C08
  std::array<float,4> plane;  // normal and offset (negated for a flipped surface)
  float radius;
  std::uint16_t axis;         // the dominant axis of the normal
  bool positive;              // the normal's sign on that axis
  std::int32_t count;
  std::array<Point2,polygon_capacity> points;  // the polygon in (u, v)
};
struct CollisionFeatures {
  std::vector<FeatureVertex> vertices;
  std::vector<FeatureEdge> edges;
  std::vector<FeatureSurface> surfaces;
};

// 24E850: an edge is a feature when it is convex (the cross product of its
// surfaces' planes along it: above -1e-4 for planes facing the same way,
// below 1e-4 for opposite ones) or both its surfaces share a plane index.
// The test runs in the BSP's own space; an instance's edge then goes into
// the world.
void add_edge_feature(CollisionFeatures& features,const CollisionBsp& bsp,std::uint32_t edge,float radius,
    const Transform* transform,std::uint32_t mxcsr) {
  const auto plane0=bsp.surface_plane(std::uint32_t(bsp.edge_surface(edge,0)));
  const auto plane1=bsp.surface_plane(std::uint32_t(bsp.edge_surface(edge,1)));
  if(plane0==plane1)return;
  auto start=bsp.vertex_point(bsp.edge_vertex(edge,0));
  const auto end=bsp.vertex_point(bsp.edge_vertex(edge,1));
  Vec3 direction{sub(end[0],start[0],mxcsr),sub(end[1],start[1],mxcsr),sub(end[2],start[2],mxcsr)};
  if((plane0&0x7fffu)!=(plane1&0x7fffu)) {
    const auto a=bsp.plane_normal(plane0&0x7fffu),b=bsp.plane_normal(plane1&0x7fffu);
    const auto cx=sub(mul(b[2],a[1],mxcsr),mul(a[2],b[1],mxcsr),mxcsr);
    const auto cy=sub(mul(a[2],b[0],mxcsr),mul(b[2],a[0],mxcsr),mxcsr);
    const auto cz=sub(mul(b[1],a[0],mxcsr),mul(b[0],a[1],mxcsr),mxcsr);
    const auto convexity=add(add(mul(cz,direction[2],mxcsr),mul(cy,direction[1],mxcsr),mxcsr),mul(cx,direction[0],mxcsr),mxcsr);
    if((plane0>>15)==(plane1>>15)) {
      if(!above(convexity,f32(facing_limit),mxcsr))return;
    } else if(!above(f32(normalize_epsilon),convexity,mxcsr))return;
  }
  if(transform) {
    direction=edge_direction_to_world(*transform,direction,mxcsr);
    start=edge_start_to_world(*transform,start,mxcsr);
  }
  if(features.edges.size()<feature_capacity)features.edges.push_back({start,direction,radius});
}

// 24EB60 (with 1E6050 and 14CE30): a surface's polygon (its edge ring) and
// plane (negated, 0 - x, for a flipped one), an instance's into the world,
// projected on the plane's dominant axis.
void add_surface_feature(CollisionFeatures& features,const CollisionBsp& bsp,std::uint32_t surface,float radius,
    const Transform* transform,std::uint32_t mxcsr) {
  std::vector<Vec3> polygon;
  const auto first=bsp.surface_first_edge(surface);
  auto edge=first;
  do {
    // 24EB60's buffer holds 8 vertices; the ninth overwrites its return
    // address, and the original faults on returning.
    if(polygon.size()==polygon_capacity)
      refuse("1E6050","a collision surface of more than 8 vertices (the original overwrites 24EB60's return address)");
    const auto side=bsp.edge_side(edge,surface);
    polygon.push_back(bsp.vertex_point(bsp.edge_vertex(edge,side)));
    edge=bsp.edge_next(edge,side);
  } while(edge!=first);
  const auto word=bsp.surface_plane(surface);
  const auto n=bsp.plane_normal(word&0x7fffu);
  std::array<float,4> plane{n[0],n[1],n[2],bsp.plane_offset(word&0x7fffu)};
  if(word&0x8000u)for(auto& value:plane)value=sub(0.f,value,mxcsr);
  if(transform) {
    for(auto& point:polygon)point=polygon_point_to_world(*transform,point,mxcsr);
    plane=plane_to_world(*transform,plane,mxcsr);
  }
  if(features.surfaces.size()>=feature_capacity)return;
  FeatureSurface out{plane,radius,0,false,std::int32_t(polygon.size()),{}};
  out.axis=dominant_axis_of({plane[0],plane[1],plane[2]},mxcsr);
  out.positive=above(plane[out.axis],0.f,mxcsr);
  const auto [u,v]=projection(out.axis,out.positive);
  for(std::size_t i=0;i<polygon.size();++i)out.points[i]={polygon[i][u],polygon[i][v]};
  features.surfaces.push_back(out);
}

// 24EDB0 (EAX transform or 0, EDI contacts, EBX features): the vertex
// (24E710), edge (24E850) and surface (24EB60) features of a walk's
// contacts, in that order.
void add_contact_features(CollisionFeatures& features,const CollisionBsp& bsp,const SphereContacts& contacts,
    const Transform* transform,std::uint32_t mxcsr) {
  constexpr float feature_radius=0.f;
  for(const auto vertex:contacts.vertices) {
    auto point=bsp.vertex_point(vertex);
    if(transform)point=vertex_to_world(*transform,point,mxcsr);
    if(features.vertices.size()<feature_capacity)features.vertices.push_back({point,feature_radius});
  }
  for(const auto edge:contacts.edges)add_edge_feature(features,bsp,edge,feature_radius,transform,mxcsr);
  for(const auto surface:contacts.surfaces)add_surface_feature(features,bsp,surface,feature_radius,transform,mxcsr);
}

// 186E30 (EAX instance): the breakable-surface bits a walk tests
// ([0x4F9B50]): for the structure (NONE) 32 bytes per structure BSP from
// +1; for an instance one byte per instance from +0x201, 1024 bytes per
// structure BSP. 16D2A0 inlines the structure's case.
std::uint32_t breakable_bits(const M& m,std::int32_t instance) {
  const auto sbsp=std::uint32_t(std::int32_t(std::int16_t(r16(m,structure_bsp_index))));
  const auto base=r32(m,breakable_surfaces);
  if(instance==-1)return base+sbsp*32u+1u;
  return base+sbsp*1024u+std::uint32_t(instance)+0x201u;
}

// The structure BSP ([0x4ECC18]) as 16D2A0 and 16CF00 read it (block
// fields: count, then rows):
//   +2C leaves, 8 bytes: +0 int16 cluster
//   +9C clusters, 0xB0 bytes: +98 the cluster's instanced geometry indices
//       (int16)
//   +C4 pathfinding data, 0x74 bytes: +40 instanced geometry references
//       (4 bytes: int16, NONE for an instance without pathfinding)
//   +138 instanced geometry definitions, 0xC8 bytes: +70 the collision BSP
//   +140 instanced geometry instances, 0x58 bytes: +0 the transform, +34
//       int16 definition, +3C bounding sphere centre, +48 its radius
struct StructureBsp {
  const M& m;
  std::uint32_t at;
  std::int32_t leaf_cluster(std::uint32_t leaf)const{return std::int16_t(r16(m,r32(m,at+0x30u)+leaf*8u));}
  std::uint32_t cluster(std::int32_t index)const{return r32(m,at+0xa0u)+std::uint32_t(index)*0xb0u;}
  std::int32_t cluster_instance_count(std::int32_t index)const{return s32(r32(m,cluster(index)+0x98u));}
  std::int32_t cluster_instance(std::int32_t index,std::int32_t k)const {
    return std::int16_t(r16(m,r32(m,cluster(index)+0x9cu)+std::uint32_t(k)*2u));
  }
  std::uint32_t instance(std::int32_t index)const{return r32(m,at+0x144u)+std::uint32_t(index)*0x58u;}
  std::uint32_t definition(std::int32_t index)const{return r32(m,at+0x13cu)+std::uint32_t(index)*0xc8u;}
  std::uint32_t instance_definition(std::uint32_t instance_row)const{return definition(std::int16_t(r16(m,instance_row+0x34u)));}
  // 16B839..16B85E: whether the pathfinding data names `instance`; true
  // without pathfinding data.
  bool instance_has_pathfinding(std::int32_t instance)const {
    if(s32(r32(m,at+0xc4u))<=0)return true;
    const auto data=r32(m,at+0xc8u);
    if(!data)return true;
    if(instance<0 || instance>=s32(r32(m,data+0x40u)))return false;
    return r16(m,r32(m,data+0x44u)+std::uint32_t(instance)*4u)!=0xffffu;
  }
};

// A visit block in .bss: a busy byte, a counter and one stamp per index. A
// query takes the next counter value and stamps each index it visits, so it
// visits an index at most once. Index NONE (-1) reads the counter itself and
// so always counts as visited (the original's bounds test).
struct VisitStamps {
  std::uint32_t busy,counter,stamps;
  // The counter store, then the busy byte.
  void begin(M& m)const{w32(m,counter,r32(m,counter)+1u);w8(m,busy,1);}
  void end(M& m)const{w8(m,busy,0);}
  // Stamps `index` unless it holds the current counter already.
  bool visit(M& m,std::int32_t index)const {
    const auto at=stamps+std::uint32_t(index)*4u,current=r32(m,counter);
    if(r32(m,at)==current)return false;
    w32(m,at,current);
    return true;
  }
};
constexpr VisitStamps cluster_visits{0x4f3ce1u,0x4f3ce4u,0x4f3ce8u};
constexpr VisitStamps instance_visits{0x4f44e8u,0x4f44ecu,0x4f44f0u};
constexpr VisitStamps object_visits{0x4eabc8u,0x4eabccu,0x4eabd0u};
// The object cluster partition: [0x4EABB0] heads (a datum per cluster) and
// [0x4EABB4] its entries (a data array: +24 row size, +44 rows; an entry's
// +4 object and +8 next entry).
constexpr std::uint32_t object_partition_heads=0x4eabb0u,object_partition_entries=0x4eabb4u;

// 16B810 (EDX definition, ESI instance; stack: flags): an instance's
// collision BSP takes part unless it has no surfaces or its last surface's
// flags have bit 4; with flags bit 18 only an instance the pathfinding data
// names does.
bool instance_collides(const StructureBsp& sbsp,const CollisionBsp& bsp,std::int32_t instance,std::uint32_t flags) {
  const auto surfaces=bsp.surface_count();
  if(surfaces<=0 || (bsp.surface_flags(std::uint32_t(surfaces-1))&0x10u))return false;
  return !(flags&0x40000u) || sbsp.instance_has_pathfinding(instance);
}

// 16CF00 (ECX instance; stack: flags, centre, radius, height, feature
// radius, features; RET 0x18): the features of one instance. Its bounding
// sphere must reach the query sphere ((r + radius)^2 >= d^2); the centre
// goes into the instance's space with the radius times the inverse scale,
// its collision BSP is walked there (1E60C0 with the breakable limit 8),
// and the contacts' features come back into the world (24EDB0).
void add_instance_features(CollisionFeatures& features,const StructureBsp& sbsp,std::int32_t instance,
    std::uint32_t flags,const Vec3& center,float radius,const PlacementFp& fpu) {
  const auto mxcsr=fpu.mxcsr;
  const auto& m=sbsp.m;
  const auto row=sbsp.instance(instance);
  const CollisionBsp bsp{m,sbsp.instance_definition(row)+0x70u};
  if(!instance_collides(sbsp,bsp,instance,flags))return;
  const auto reach=add(rf(m,row+0x48u),radius,mxcsr);
  const auto dz=sub(rf(m,row+0x44u),center[2],mxcsr),dx=sub(rf(m,row+0x3cu),center[0],mxcsr);
  const auto dy=sub(rf(m,row+0x40u),center[1],mxcsr);
  const auto distance2=add(add(mul(dz,dz,mxcsr),mul(dx,dx,mxcsr),mxcsr),mul(dy,dy,mxcsr),mxcsr);
  if(!at_least(mul(reach,reach,mxcsr),distance2,mxcsr))return;
  const auto transform=load_transform(m,row);
  const auto to_local=inverse(transform,mxcsr);
  const auto local_center=point_to_instance(to_local,center,mxcsr);
  SphereContactWalk walk(bsp,local_center,mul(to_local.scale,radius,mxcsr),breakable_bits(m,instance),8,mxcsr);
  walk.visit(0);
  if(walk.contacts.any())add_contact_features(features,bsp,walk.contacts,&transform,mxcsr);
}

// 16D36B..16D5DA: the clusters of the leaves the structure walk reached,
// each once, and in each its instanced geometry (bit 2: each instance once,
// 16CF00) and its objects (bit 3: each object once, 16D0E0). The visit
// blocks stay busy for the query.
void query_clusters(M& m,CollisionFeatures& features,const std::vector<std::uint32_t>& leaves,std::uint32_t flags,
    const Vec3& center,float radius,const PlacementFp& fpu) {
  const StructureBsp sbsp{m,r32(m,structure_bsp)};
  const bool instances=flags&collision_test_instances,objects=flags&collision_test_objects;
  cluster_visits.begin(m);
  if(instances)instance_visits.begin(m);
  if(objects) {
    object_visits.begin(m);
    // 16D39D: an object query without any of bits 4..16 asks for all of
    // them (only 16D0E0 reads them).
    if(!(flags&0x1fff0u))flags|=0x1fff0u;
  }
  for(const auto leaf:leaves) {
    const auto cluster=sbsp.leaf_cluster(leaf);
    if(!cluster_visits.visit(m,cluster))continue;
    if(instances)
      for(std::int32_t k=0;k<sbsp.cluster_instance_count(cluster);++k) {
        const auto instance=sbsp.cluster_instance(cluster,k);
        if(instance_visits.visit(m,instance))add_instance_features(features,sbsp,instance,flags,center,radius,fpu);
      }
    if(objects) {
      // The cluster's chain of entries; an entry whose object is NONE ends
      // it.
      const auto entries=r32(m,object_partition_entries);
      const auto entry=[&](std::uint32_t datum){return r32(m,entries+0x44u)+(datum&0xffffu)*r32(m,entries+0x24u);};
      auto next=r32(m,r32(m,object_partition_heads)+std::uint32_t(cluster)*4u);
      while(next!=none) {
        const auto at=entry(next);
        next=r32(m,at+8u);
        const auto object=r32(m,at+4u);
        if(object==none)break;
        if(object_visits.visit(m,std::int32_t(object&0xffffu)))
          refuse("16D0E0","an object in a collision sphere query (bit 3)");
      }
    }
  }
  if(objects)object_visits.end(m);
  if(instances)instance_visits.end(m);
  cluster_visits.end(m);
}

// 16D2A0 (stack: flags, centre, radius, height, feature radius, two
// ignored objects, features; RET 0x20): the BSP walk with the radius grown
// by 1/16 (x87), the structure's features (bit 0), then the clusters'
// instanced geometry (bit 2, when the walk reached a leaf) and objects
// (bit 3). The grown radius is the instances' query radius too.
CollisionFeatures collision_features(M& m,std::uint32_t flags,const Vec3& center,float radius,const PlacementFp& fpu) {
  CollisionFeatures features;
  if(!(flags&(collision_test_structure|collision_test_instances|collision_test_objects)))return features;
  const CollisionBsp bsp{m,r32(m,collision_bsp_sphere)};
  const auto query_radius=x87_add(radius,f32(sphere_margin),fpu.control_word);
  SphereContactWalk walk(bsp,center,query_radius,breakable_bits(m,-1),0x100,fpu.mxcsr);
  walk.visit(0);
  const auto& contacts=walk.contacts;
  if(contacts.any() && (flags&collision_test_structure))add_contact_features(features,bsp,contacts,nullptr,fpu.mxcsr);
  if((!contacts.leaves.empty() && (flags&collision_test_instances)) || (flags&collision_test_objects))
    query_clusters(m,features,contacts.leaves,flags,center,query_radius,fpu);
  return features;
}

// ---- 24FCC0: a ray through the features ----
struct FeatureHit {float t;Vec3 normal;};

// 24F4B0 (ESI vertex, ECX origin, EDX ray): the ray enters the vertex's
// sphere (t 0 when the origin is inside).
bool ray_hits_vertex(const FeatureVertex& f,const Vec3& origin,const Vec3& ray,FeatureHit& hit,const PlacementFp& fpu) {
  const auto mxcsr=fpu.mxcsr;
  const auto dx=sub(f.point[0],origin[0],mxcsr),dz=sub(f.point[2],origin[2],mxcsr),dy=sub(f.point[1],origin[1],mxcsr);
  const auto outside=sub(add(add(mul(dz,dz,mxcsr),mul(dx,dx,mxcsr),mxcsr),mul(dy,dy,mxcsr),mxcsr),
      mul(f.radius,f.radius,mxcsr),mxcsr);
  float t=0.f;
  if(!at_least(0.f,outside,mxcsr)) {
    const auto along=add(add(mul(ray[0],dx,mxcsr),mul(dz,ray[2],mxcsr),mxcsr),mul(dy,ray[1],mxcsr),mxcsr);
    if(!above(along,0.f,mxcsr))return false;
    const auto length2=add(add(mul(ray[1],ray[1],mxcsr),mul(ray[2],ray[2],mxcsr),mxcsr),mul(ray[0],ray[0],mxcsr),mxcsr);
    const auto disc=sub(mul(along,along,mxcsr),mul(length2,outside,mxcsr),mxcsr);
    if(!at_least(disc,0.f,mxcsr))return false;
    const auto [entry,within]=x87_ray_entry(along,disc,length2,fpu.control_word);
    if(!within)return false;
    t=fp::divss(entry,length2,mxcsr);
  }
  hit.t=t;
  hit.normal={sub(mul(ray[0],t,mxcsr),dx,mxcsr),sub(mul(ray[1],t,mxcsr),dy,mxcsr),sub(mul(ray[2],t,mxcsr),dz,mxcsr)};
  if(fp::sse_compared(bits(normalize3(hit.normal,fpu)),mxcsr)==0.f)hit.normal={0.f,0.f,f32(one_bits)};
  return true;
}

// 24F670 (ESI edge, ECX ray, EDX origin): the ray enters the capsule along
// the edge, within the segment (t clamped to [0, 1]).
bool ray_hits_edge(const FeatureEdge& f,const Vec3& origin,const Vec3& ray,FeatureHit& hit,const PlacementFp& fpu) {
  const auto mxcsr=fpu.mxcsr;
  const auto& e=f.direction;
  const auto ee=add(add(mul(e[0],e[0],mxcsr),mul(e[1],e[1],mxcsr),mxcsr),mul(e[2],e[2],mxcsr),mxcsr);
  const auto ev=add(add(mul(e[2],ray[2],mxcsr),mul(e[1],ray[1],mxcsr),mxcsr),mul(e[0],ray[0],mxcsr),mxcsr);
  const auto vv=add(add(mul(ray[0],ray[0],mxcsr),mul(ray[1],ray[1],mxcsr),mxcsr),mul(ray[2],ray[2],mxcsr),mxcsr);
  const auto denominator=sub(mul(vv,ee,mxcsr),mul(ev,ev,mxcsr),mxcsr);
  if(fp::sse_compared(bits(denominator),mxcsr)==0.f)return false;  // UCOMISS: parallel (NaN continues)
  const auto dz=sub(origin[2],f.start[2],mxcsr),dy=sub(origin[1],f.start[1],mxcsr),dx=sub(origin[0],f.start[0],mxcsr);
  const auto vd=add(add(mul(ray[1],dy,mxcsr),mul(ray[0],dx,mxcsr),mxcsr),mul(ray[2],dz,mxcsr),mxcsr);
  const auto ed=add(add(mul(e[2],dz,mxcsr),mul(e[1],dy,mxcsr),mxcsr),mul(e[0],dx,mxcsr),mxcsr);
  const auto b=sub(mul(ed,ev,mxcsr),mul(vd,ee,mxcsr),mxcsr);
  const auto dd=add(add(mul(dz,dz,mxcsr),mul(dy,dy,mxcsr),mxcsr),mul(dx,dx,mxcsr),mxcsr);
  const auto c=sub(mul(sub(dd,mul(f.radius,f.radius,mxcsr),mxcsr),ee,mxcsr),mul(ed,ed,mxcsr),mxcsr);
  const auto disc=sub(mul(b,b,mxcsr),mul(c,denominator,mxcsr),mxcsr);
  if(above(0.f,disc,mxcsr))return false;
  const auto root=x87_sqrt(disc,fpu.control_word);
  const auto inverse=fp::divss(f32(one_bits),denominator,mxcsr);
  auto enter=mul(sub(b,root,mxcsr),inverse,mxcsr);
  if(above(enter,f32(one_bits),mxcsr))return false;
  auto leave=mul(add(root,b,mxcsr),inverse,mxcsr);
  if(above(0.f,leave,mxcsr))return false;
  if(above(0.f,enter,mxcsr))enter=0.f;
  if(above(leave,f32(one_bits),mxcsr))leave=f32(one_bits);
  if(fp::sse_compared(bits(ev),mxcsr)==0.f) {
    // A ray square to the edge: the origin's projection must lie on it.
    if(above(0.f,ed,mxcsr) || above(ed,ee,mxcsr))return false;
  } else {
    // The span where the ray's projection lies on the segment.
    const auto per=fp::divss(f32(one_bits),ev,mxcsr);
    const auto from=sub(0.f,mul(per,ed,mxcsr),mxcsr),to=mul(sub(ee,ed,mxcsr),per,mxcsr);
    float bound;
    if(above(ev,0.f,mxcsr)) {
      if(above(from,enter,mxcsr))enter=from;
      bound=above(leave,to,mxcsr)?to:leave;
    } else {
      if(above(to,enter,mxcsr))enter=to;
      bound=above(leave,from,mxcsr)?from:leave;
    }
    if(above(enter,bound,mxcsr))return false;
  }
  hit.t=enter;
  const Vec3 q{add(mul(ray[0],enter,mxcsr),dx,mxcsr),add(mul(ray[1],enter,mxcsr),dy,mxcsr),add(mul(ray[2],enter,mxcsr),dz,mxcsr)};
  const auto s=fp::divss(add(add(mul(e[2],q[2],mxcsr),mul(e[1],q[1],mxcsr),mxcsr),mul(q[0],e[0],mxcsr),mxcsr),ee,mxcsr);
  const auto back=sub(0.f,s,mxcsr);
  hit.normal={add(mul(e[0],back,mxcsr),q[0],mxcsr),add(mul(e[1],back,mxcsr),q[1],mxcsr),add(mul(e[2],back,mxcsr),q[2],mxcsr)};
  if(fp::sse_compared(bits(normalize3(hit.normal,fpu)),mxcsr)==0.f)hit.normal={f32(one_bits),0.f,0.f};
  return true;
}

// 24F9C0 (EAX surface, ECX ray, EDX origin): the ray crosses the surface's
// slab (its plane to the radius above it) inside the polygon's edges.
bool ray_hits_surface(const FeatureSurface& f,const Vec3& origin,const Vec3& ray,FeatureHit& hit,const PlacementFp& fpu) {
  const auto mxcsr=fpu.mxcsr;
  const Vec3 n{f.plane[0],f.plane[1],f.plane[2]};
  const auto [dot,parallel]=x87_plane_dot(n,ray,fpu.control_word);
  const auto height=sub(add(add(mul(n[1],origin[1],mxcsr),mul(n[2],origin[2],mxcsr),mxcsr),mul(origin[0],n[0],mxcsr),mxcsr),
      f.plane[3],mxcsr);
  float enter=0.f,leave=f32(one_bits);
  if(!parallel) {
    const auto per=fp::divss(f32(one_bits),dot,mxcsr);
    const auto at_plane=sub(0.f,mul(per,height,mxcsr),mxcsr);
    const auto at_top=sub(0.f,mul(sub(height,f.radius,mxcsr),per,mxcsr),mxcsr);
    if(above(dot,0.f,mxcsr)) {
      if(above(at_plane,0.f,mxcsr))enter=at_plane;
      if(above(leave,at_top,mxcsr))leave=at_top;
    } else {
      if(above(at_top,0.f,mxcsr))enter=at_top;
      if(above(leave,at_plane,mxcsr))leave=at_plane;
    }
    if(above(enter,leave,mxcsr))return false;
  } else if(above(0.f,height,mxcsr) || at_least(height,f.radius,mxcsr))return false;
  // The origin and the ray projected onto the plane, in (u, v).
  const auto back=sub(0.f,height,mxcsr);
  const Vec3 base{add(mul(n[0],back,mxcsr),origin[0],mxcsr),add(mul(n[1],back,mxcsr),origin[1],mxcsr),
      add(mul(n[2],back,mxcsr),origin[2],mxcsr)};
  const auto away=sub(0.f,dot,mxcsr);
  const Vec3 slide{add(mul(away,n[0],mxcsr),ray[0],mxcsr),add(mul(n[1],away,mxcsr),ray[1],mxcsr),
      add(mul(n[2],away,mxcsr),ray[2],mxcsr)};
  const auto [u,v]=projection(f.axis,f.positive);
  const Point2 p{base[u],base[v]},d{slide[u],slide[v]};
  for(std::int32_t i=0;i<f.count;++i) {
    const auto& a=f.points[std::size_t(i)];
    const auto& b=f.points[std::size_t(i+1>=f.count?0:i+1)];
    const auto pv=sub(p[1],a[1],mxcsr),pu=sub(p[0],a[0],mxcsr);
    const auto ev=sub(b[1],a[1],mxcsr),eu=sub(b[0],a[0],mxcsr);
    const auto across=sub(mul(ev,d[0],mxcsr),mul(d[1],eu,mxcsr),mxcsr);
    const auto side=sub(mul(pv,eu,mxcsr),mul(ev,pu,mxcsr),mxcsr);
    if(below_epsilon(across)) {
      if(above(0.f,side,mxcsr))return false;  // parallel to the edge, outside it
      continue;
    }
    const auto t=fp::divss(side,across,mxcsr);
    if(above(0.f,across,mxcsr)) {
      if(above(t,enter,mxcsr))enter=t;
    } else if(above(leave,t,mxcsr))leave=t;
    if(above(enter,leave,mxcsr))return false;
  }
  hit.t=enter;
  hit.normal=n;
  return true;
}

// 24FCC0 (stack: features, origin, ray, result): the nearest feature the
// ray (origin + [0, 1] * ray) enters against its normal (dot below -1e-4):
// vertices, then edges, then surfaces. Returns the hit point (result +14),
// or nothing; 16D740 reads no other result field.
std::optional<Vec3> ray_through_features(const CollisionFeatures& features,const Vec3& origin,const Vec3& ray,
    const PlacementFp& fpu) {
  const auto mxcsr=fpu.mxcsr;
  float best=f32(float_max);bool found=false;
  const auto consider=[&](bool hit,const FeatureHit& h) {
    if(!hit || !above(best,h.t,mxcsr))return;
    const auto facing=add(add(mul(ray[1],h.normal[1],mxcsr),mul(ray[2],h.normal[2],mxcsr),mxcsr),mul(ray[0],h.normal[0],mxcsr),mxcsr);
    if(above(f32(facing_limit),facing,mxcsr)){best=h.t;found=true;}
  };
  FeatureHit h{};
  for(const auto& f:features.vertices)consider(ray_hits_vertex(f,origin,ray,h,fpu),h);
  for(const auto& f:features.edges)consider(ray_hits_edge(f,origin,ray,h,fpu),h);
  for(const auto& f:features.surfaces)consider(ray_hits_surface(f,origin,ray,h,fpu),h);
  if(!found)return std::nullopt;
  return Vec3{add(mul(ray[0],best,mxcsr),origin[0],mxcsr),add(mul(ray[1],best,mxcsr),origin[1],mxcsr),
      add(mul(ray[2],best,mxcsr),origin[2],mxcsr)};
}
// 16D740's end of a ray that hits nothing: centre + ray (z: ray + centre).
Vec3 ray_end(const Vec3& center,const Vec3& ray,std::uint32_t mxcsr) {
  return {add(center[0],ray[0],mxcsr),add(center[1],ray[1],mxcsr),add(ray[2],center[2],mxcsr)};
}

// 16D82C..16DCD9: the point of the surface's polygon nearest to the centre:
// the centre's projection when it lies inside every edge, else the nearest
// point of the edges it lies outside of. `distance` is the centre's height
// over the plane.
// `closest` is the original's one stack record (16D740's +B8): it keeps the
// previous surface's point when no edge candidate improves on FLT_MAX.
struct NearestPoint {float distance2;bool inside;float distance;};
NearestPoint nearest_point(const FeatureSurface& s,const Vec3& c,std::optional<Vec3>& closest,std::uint32_t mxcsr) {
  const Vec3 n{s.plane[0],s.plane[1],s.plane[2]};
  const auto d=sub(add(add(mul(c[0],n[0],mxcsr),mul(c[1],n[1],mxcsr),mxcsr),mul(n[2],c[2],mxcsr),mxcsr),s.plane[3],mxcsr);
  const auto back=sub(0.f,d,mxcsr);
  const Vec3 q{add(mul(back,n[0],mxcsr),c[0],mxcsr),add(mul(n[1],back,mxcsr),c[1],mxcsr),add(mul(back,n[2],mxcsr),c[2],mxcsr)};
  NearestPoint out{f32(float_max),true,d};
  const auto count=s.count>8?8:s.count;  // only eight points fit a row
  const auto [u,v]=projection(s.axis,s.positive);
  // A polygon point back on the plane: its third coordinate solved from
  // the plane (0 when the plane is nearly parallel to the axis).
  const auto unproject=[&](const Point2& p) {
    Vec3 a{};a[u]=p[0];a[v]=p[1];
    if(!below_epsilon(n[s.axis]))
      a[s.axis]=fp::divss(sub(sub(s.plane[3],mul(n[u],p[0],mxcsr),mxcsr),mul(n[v],p[1],mxcsr),mxcsr),n[s.axis],mxcsr);
    return a;
  };
  for(std::int32_t i=1;i<=count;++i) {
    const auto a=unproject(s.points[std::size_t(i-1)]),b=unproject(s.points[std::size_t(i%s.count)]);
    const Vec3 qa{sub(q[0],a[0],mxcsr),sub(q[1],a[1],mxcsr),sub(q[2],a[2],mxcsr)};
    const Vec3 e{sub(b[0],a[0],mxcsr),sub(b[1],a[1],mxcsr),sub(b[2],a[2],mxcsr)};
    const Vec3 cross{sub(mul(e[1],qa[2],mxcsr),mul(e[2],qa[1],mxcsr),mxcsr),sub(mul(e[2],qa[0],mxcsr),mul(qa[2],e[0],mxcsr),mxcsr),
        sub(mul(qa[1],e[0],mxcsr),mul(e[1],qa[0],mxcsr),mxcsr)};
    const auto side=add(add(mul(n[2],cross[2],mxcsr),mul(n[1],cross[1],mxcsr),mxcsr),mul(n[0],cross[0],mxcsr),mxcsr);
    if(above(0.f,side,mxcsr)){out.inside=false;continue;}
    // The projection's foot on the edge's line, clamped to the segment.
    const Vec3 across{sub(mul(n[2],e[1],mxcsr),mul(n[1],e[2],mxcsr),mxcsr),sub(mul(e[2],n[0],mxcsr),mul(n[2],e[0],mxcsr),mxcsr),
        sub(mul(n[1],e[0],mxcsr),mul(n[0],e[1],mxcsr),mxcsr)};
    const auto k=sub(0.f,add(add(mul(across[0],qa[0],mxcsr),mul(across[2],qa[2],mxcsr),mxcsr),mul(across[1],qa[1],mxcsr),mxcsr),mxcsr);
    const Vec3 foot{add(mul(across[0],k,mxcsr),q[0],mxcsr),add(mul(across[1],k,mxcsr),q[1],mxcsr),add(mul(across[2],k,mxcsr),q[2],mxcsr)};
    const auto along=add(add(mul(sub(foot[0],a[0],mxcsr),e[0],mxcsr),mul(sub(foot[2],a[2],mxcsr),e[2],mxcsr),mxcsr),
        mul(sub(foot[1],a[1],mxcsr),e[1],mxcsr),mxcsr);
    Vec3 candidate=foot;
    if(at_least(0.f,along,mxcsr))candidate=a;
    else if(at_least(along,add(add(mul(e[1],e[1],mxcsr),mul(e[0],e[0],mxcsr),mxcsr),mul(e[2],e[2],mxcsr),mxcsr),mxcsr))candidate=b;
    const auto dx=sub(candidate[0],q[0],mxcsr),dz=sub(candidate[2],q[2],mxcsr),dy=sub(candidate[1],q[1],mxcsr);
    const auto distance2=add(add(mul(dx,dx,mxcsr),mul(dz,dz,mxcsr),mxcsr),mul(dy,dy,mxcsr),mxcsr);
    if(above(out.distance2,distance2,mxcsr)){out.distance2=distance2;closest=candidate;}
  }
  if(out.inside){closest=q;out.distance2=0.f;}
  return out;
}
}

SphereFit collision_sphere_fit(M& m,std::uint32_t flags,const Vec3& center,float radius,const PlacementFp& fpu) {
  const auto mxcsr=fpu.mxcsr;
  if(!above(radius,0.f,mxcsr))return {center,radius,false};
  const auto features=collision_features(m,flags,center,radius,fpu);
  // Two groups of spans: those from surfaces the centre is on (or whose
  // nearest point is degenerate), and those cast toward a nearest point.
  struct Group {Vec3 sum;float smallest;std::int32_t count;};
  const auto origin=load3(m,r32(m,global_origin));
  Group on_surface{origin,radius,0},toward{origin,radius,0};
  const auto radius2=mul(radius,radius,mxcsr);
  std::optional<Vec3> closest;
  for(const auto& surface:features.surfaces) {
    const auto nearest=nearest_point(surface,center,closest,mxcsr);
    if(!above(radius2,nearest.distance2,mxcsr))continue;
    Vec3 near_end{},far_end{};bool along_normal=true;
    if(!(on_plane(nearest.distance) && nearest.inside)) {
      if(!closest)refuse("16D740","a nearest point left in the original's uninitialised stack (radius squared overflows)");
      const auto& point=*closest;
      const Vec3 offset{x87_sub(point[0],center[0],fpu.control_word),x87_sub(point[1],center[1],fpu.control_word),
          x87_sub(point[2],center[2],fpu.control_word)};
      const auto [length,small]=x87_length_xzy(offset,fpu.control_word);
      if(!small) {
        const auto inverse=fp::divss(f32(one_bits),length,mxcsr);
        const Vec3 direction{mul(inverse,offset[0],mxcsr),mul(offset[1],inverse,mxcsr),mul(offset[2],inverse,mxcsr)};
        if(above(length,f32(surface_distance),mxcsr)) {
          // The span from the centre along the direction both ways.
          along_normal=false;
          const Vec3 ray{mul(direction[0],radius,mxcsr),mul(direction[1],radius,mxcsr),mul(direction[2],radius,mxcsr)};
          near_end=ray_through_features(features,center,ray,fpu).value_or(ray_end(center,ray,mxcsr));
          const auto reverse=sub(0.f,radius,mxcsr);
          const Vec3 back{mul(reverse,direction[0],mxcsr),mul(direction[1],reverse,mxcsr),mul(direction[2],reverse,mxcsr)};
          far_end=ray_through_features(features,center,back,fpu).value_or(ray_end(center,back,mxcsr));
        }
      }
    }
    if(along_normal) {
      // The span from the centre along the surface normal.
      const Vec3 ray{mul(surface.plane[0],radius,mxcsr),mul(surface.plane[1],radius,mxcsr),mul(surface.plane[2],radius,mxcsr)};
      near_end=ray_through_features(features,center,ray,fpu).value_or(ray_end(center,ray,mxcsr));
      far_end=center;
    }
    const auto half=f32(half_bits);
    const Vec3 middle{mul(add(far_end[0],near_end[0],mxcsr),half,mxcsr),mul(add(far_end[1],near_end[1],mxcsr),half,mxcsr),
        mul(add(far_end[2],near_end[2],mxcsr),half,mxcsr)};
    const Vec3 reach{sub(middle[0],near_end[0],mxcsr),sub(middle[1],near_end[1],mxcsr),sub(middle[2],near_end[2],mxcsr)};
    const auto extent=x87_length_xzy(reach,fpu.control_word).length;
    auto& group=along_normal?on_surface:toward;
    for(unsigned k=0;k<3;++k)group.sum[k]=add(group.sum[k],middle[k],mxcsr);
    if(above(group.smallest,extent,mxcsr))group.smallest=extent;
    ++group.count;
  }
  for(const auto* group:{&on_surface,&toward}) {
    if(group->count<=0)continue;
    const auto scale=fp::divss(f32(one_bits),fp::cvtsi2ss(group->count,mxcsr),mxcsr);
    return {{mul(group->sum[0],scale,mxcsr),mul(group->sum[1],scale,mxcsr),mul(group->sum[2],scale,mxcsr)},group->smallest,true};
  }
  return {center,radius,false};
}

void light_reconnect(M& m,std::uint32_t light,PlacementFp& fpu) {
  const auto mxcsr=fpu.mxcsr;
  const auto row=light_row(m,light);
  const auto definition=tag_data(m,r32(m,row+4u));
  const auto flags=r16(m,row+2u);
  if(!(flags&2u))return;
  // C4D82: +2 bit 2 places the frame position (+84 toward the +90 target)
  // at the first point inside a cluster (+A0, distance +9C).
  if(flags&4u)light_frame_point(m,row+0x84u,row+0x90u,row+0xa0u,row+0x9cu,fpu);
  if(r16(m,row+0x54u)!=0xffffu) {
    const auto owner=r32(m,row+0x4cu);
    const auto name=object_attachment_marker(m,owner,r16(m,row+0x54u));
    // Definition +0 bit 5: the first-person marker first (1692E0); its x
    // target sum takes position-first operands.
    if((r8(m,definition)&0x20u) && first_person_marker(m,owner,name,row+0x84u,row+0xacu,row+0xb8u,mxcsr)) {
      w32(m,row+0x90u,bits(add(rf(m,row+0x84u),rf(m,row+0xacu),mxcsr)));
    } else {
      ScratchMemory local(m);
      const auto out=ScratchMemory::base;
      object_marker_matrices(local,owner,name,1,false,out,mxcsr);
      copy3(local,row+0x84u,out+0x60u);
      copy3(local,row+0xacu,out+0x3cu);
      copy3(local,row+0xb8u,out+0x54u);
      w32(m,row+0x90u,bits(add(rf(m,row+0xacu),rf(m,row+0x84u),mxcsr)));
    }
    w32(m,row+0x94u,bits(add(rf(m,row+0x88u),rf(m,row+0xb0u),mxcsr)));
    w32(m,row+0x98u,bits(add(rf(m,row+0x8cu),rf(m,row+0xb4u),mxcsr)));
  } else if(const auto parent=r32(m,row+0x58u);parent!=none) {
    // C4ECB: the light's local frame (+60 position, +6C forward, +78 up)
    // through its parent object's node matrix (+5C node of the body +116
    // block): the position by 145070, both axes by the matrix rotation, and
    // the target +90 = position + forward (position-first operands).
    const auto body=object_body(m,parent);
    const auto matrix=std::uint32_t(s32(sx16(r16(m,row+0x5cu)))*0x34)+body+sx16(r16(m,body+0x116u));
    transform_point(m,row+0x60u,row+0x84u,matrix,mxcsr);
    for(const auto [from,to]:{std::array<std::uint32_t,2>{0x6cu,0xacu},std::array<std::uint32_t,2>{0x78u,0xb8u}}) {
      const auto x=rf(m,row+from),y=rf(m,row+from+4u),z=rf(m,row+from+8u);
      // Lane 0 sums (z + y) + x, lanes 1 and 2 (x + z) + y (C4F0D..C5024).
      for(std::uint32_t k=0;k<3u;++k) {
        const auto tx=mul(rf(m,matrix+4u+4u*k),x,mxcsr),ty=mul(rf(m,matrix+0x10u+4u*k),y,mxcsr);
        const auto tz=mul(rf(m,matrix+0x1cu+4u*k),z,mxcsr);
        w32(m,row+to+4u*k,bits(k?add(add(tx,tz,mxcsr),ty,mxcsr):add(add(tz,ty,mxcsr),tx,mxcsr)));
      }
    }
    for(std::uint32_t k=0;k<3u;++k)
      w32(m,row+0x90u+4u*k,bits(add(rf(m,row+0x84u+4u*k),rf(m,row+0xacu+4u*k),mxcsr)));
  }
  light_update_bounds(m,light,fpu);
  if(!(r8(m,row+2u)&8u) && r16(m,row+0x48u)==0xffffu && (r32(m,definition)&0x8000u) &&
     above(rf(m,row+0x24u),f32(zero_bits),mxcsr)) {
    // C5088: definition +0 bit 15 fits an unplaced light's sphere (+18,
    // radius +24) into the structure's collision geometry (16D740 with
    // flags 0x0D800001); the fitted centre is the light's centre and its
    // location point (+38). The fitted radius goes to a dead local.
    const auto fit=collision_sphere_fit(m,0x0d800001u,load3(m,row+0x18u),rf(m,row+0x24u),fpu);
    for(std::uint32_t k=0;k<3u;++k)w32(m,row+0x18u+4u*k,bits(fit.center[k]));
    copy3(m,row+0x38u,row+0x18u);
  }
  location_from_point(m,row+0x44u,row+0x38u,mxcsr);
  if(r16(m,row+0x48u)==0xffffu && (!(r8(m,row+2u)&1u) || (r8(m,definition)&8u))) {
    if(const auto owner=r32(m,row+0x4cu);owner!=none)object_root_location(m,owner,row+0x44u);
    else if(const auto object=r32(m,row+0x58u);object!=none)object_root_location(m,object,row+0x44u);
    else location_from_point(m,row+0x44u,row+0x18u,mxcsr);
  }
  ClusterLinks links;
  links.partition=light_partition;links.owner=light;links.owner_head=row+0x10u;links.center=row+0x18u;
  links.radius=r32(m,row+0x24u);links.location=row+0x44u;
  cluster_partition_reconnect(m,{},links,fpu);
  w8(m,row+2u,r8(m,row+2u)|8u);
}
}
