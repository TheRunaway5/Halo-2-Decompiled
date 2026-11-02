#include "halo2/native_tu15_camera_geometric_query.hpp"
#include "halo2/native_tu15_bsp_segment_query.hpp"
#include "halo2/native_tu15_camera_query_leaves.hpp"
#include "halo2/native_tu15_camera_query_object_context.hpp"
#include "halo2/native_tu15_constraint_effect_location.hpp"
#include <bit>
#include <cstring>
#include <functional>
#include <stdexcept>
#include <type_traits>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&m_;const SourcePhysicsContactRecordObserver&o_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):m_(m),o_(o){}
    std::uint32_t word(std::uint32_t p,unsigned width=4)const{std::array<std::byte,4>b{};m_.read(p,std::span(b).first(width));std::uint32_t n{};for(unsigned k=0;k<width;++k)n|=std::to_integer<unsigned>(b[k])<<(8*k);return n;}
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t n,unsigned width=4){std::array<std::byte,4>b{};for(unsigned k=0;k<width;++k)b[k]=std::byte(n>>(8*k));SourcePhysicsContactRecordStore e{pc,p,{},{}};if(o_){e.before.resize(width);m_.read(p,e.before);e.written.assign(b.begin(),b.begin()+width);}m_.write(p,std::span(b).first(width));if(o_)o_(e);}
};
std::uint32_t sign16(std::uint32_t n){return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(n))));}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
    alignas(16)std::array<std::byte,108>host_{};std::array<std::byte,28>native_{};NativeTu15PhysicsFrameFp&fp_;unsigned prior_;bool active_{};
    void validate(){if((fp_.control_word&0x3fu)!=0x3fu||(fp_.status&0x8080u)||(fp_.mxcsr&0x1f80u)!=0x1f80u||(fp_.mxcsr&0xffff0000u))throw std::runtime_error("Native BSP segment requires its numerical-fault owner");}
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp),prior_(_mm_getcsr()){if(depth>5)throw std::runtime_error("Native BSP segment requires x87 capacity owner");validate();asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");std::memcpy(native_.data(),host_.data(),28);resume();}
    ~Environment(){if(active_)publish();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(prior_);}
    void publish(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));fp_.mxcsr=_mm_getcsr();active_=false;}
    void resume(){validate();const std::uint16_t empty=0xffff;std::memcpy(native_.data(),&fp_.control_word,2);std::memcpy(native_.data()+4,&fp_.status,2);std::memcpy(native_.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp_.mxcsr);active_=true;}
    template<class F>auto dependency(F&&f){publish();try{auto value=f();resume();return value;}catch(...){resume();throw;}}
};
using F=__m128;
F scalar(std::uint32_t n){F v{};std::memcpy(&v,&n,4);return v;}
std::uint32_t bits(F v){std::uint32_t n;std::memcpy(&n,&v,4);return n;}
F mul(F a,F b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
F add(F a,F b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
F sub(F a,F b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
struct Compare{bool below,equal;bool above()const{return!below&&!equal;}};
Compare compare(F a,F b){unsigned char below,equal;asm volatile("comiss %3,%2;setb %0;sete %1":"=qm"(below),"=qm"(equal):"x"(a),"x"(b):"cc","memory");return{below!=0,equal!=0};}
F divide(F a,F b){asm volatile("divss %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool ordered_zero(F a){const auto zero=F{};unsigned char equal,ordered;asm volatile("ucomiss %3,%2;sete %0;setnp %1":"=qm"(equal),"=qm"(ordered):"x"(a),"x"(zero):"cc","memory");return equal&&ordered;}
std::uint32_t absolute(std::uint32_t value){std::uint32_t out;asm volatile("flds %1;fabs;fstps %0":"=m"(out):"m"(value):"memory","st");return out;}
bool absolute_slope_exceeds(std::uint32_t distance,std::uint32_t slope){unsigned char above;asm volatile("flds %1;fabs;flds %2;fcomip %%st(1),%%st;seta %0;fstp %%st(0)":"=qm"(above):"m"(distance),"m"(slope):"memory","cc","st","st(1)");return above!=0;}
std::uint32_t crossing(std::uint32_t distance,std::uint32_t slope){std::uint32_t out;asm volatile("flds %1;fdivs %2;fchs;fstps %0":"=m"(out):"m"(distance),"m"(slope):"memory","st");return out;}
#endif
}
bool native_tu15_query_camera_geometry(SourcePhysicsContactMemory&memory,
 const NativeTu15CameraGeometricQuery&q,NativeTu15CameraGeometricQueryScratch scratch,
 NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15CameraGeometricQueryServices&services,
 const SourcePhysicsContactRecordObserver&observe,NativeTu15CameraGeometricQueryStats*stats){
 if(stats)++stats->calls;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
 Memory m(memory,observe);const auto bsp_global=m.word(0x4ecc18u);auto mask=q.mask;
 if(!(mask&0x1800000u))mask|=0x1800000u;
 const auto one=m.word(0x46a0d0u);const auto slot=std::uint16_t(m.word(0x474c24u,2));
 m.word(0x16c66du,q.result+4u,one);m.word(0x16c672u,q.result,0);
 m.word(0x16c679u,q.result+20u,0xffffffffu);m.word(0x16c67cu,q.result+24u,0xffffu,2);m.word(0x16c680u,q.result+26u,slot,2);
 m.word(0x16c684u,q.result+28u,0xffffffffu);m.word(0x16c687u,q.result+32u,0xffffu,2);m.word(0x16c68bu,q.result+34u,slot,2);
 m.word(0x16c691u,q.result+56u,0xffffffffu);m.word(0x16c694u,q.result+60u,0xffffffffu);m.word(0x16c697u,q.result+64u,0xffffffffu);
 const auto bsp_flags=native_tu15_camera_bsp_query_flags(mask),epsilon=m.word(0x45695cu),bitmap_root=m.word(0x4f9b50u);
 const auto bitmap=bitmap_root+sign16(slot)*32u+1u,descriptor=m.word(0x4ecc10u);bool hit{};
 const auto child=[&](std::uint32_t owner,auto&&body)->decltype(auto){
  if(services.native_child)services.native_child(owner,true);bool exited{};
  try{auto value=body();exited=true;if(services.native_child)services.native_child(owner,false);return value;}catch(...){if(!exited&&services.native_child)services.native_child(owner,false);throw;}
 };
 if(stats)++stats->bsp_calls;
 const auto bsp_hit=child(0x1e68e0u,[&]{return native_tu15_query_bsp_segment(memory,{bsp_flags,descriptor,scratch.bsp_output,epsilon,256,bitmap,q.origin,q.displacement},scratch.bsp_context,fp,depth,observe);});
 Environment environment(fp,depth);const auto zero=F{};
 if(bsp_hit&&(mask&1u)){
  if(stats)++stats->bsp_hits;
  const auto partition=std::uint16_t(m.word(scratch.bsp_output+22u,2));m.word(0x16c6fau,q.result,1);
  std::uint32_t material;
  if(partition!=0xffffu){const auto table=m.word(bsp_global+16u);material=table+sign16(partition)*20u+8u;}else material=0x48a0b0u;
  const auto material_id=m.word(material,2),fraction=m.word(scratch.bsp_output);
  m.word(0x16c727u,q.result+36u,material_id,2);const auto flags=m.word(scratch.bsp_output+16u),plane=m.word(scratch.bsp_output+4u);m.word(0x16c735u,q.result+56u,0xffffffffu);m.word(0x16c738u,q.result+60u,0xffffffffu);m.word(0x16c73bu,q.result+64u,0xffffffffu);m.word(0x16c73eu,q.result+4u,fraction);
  if(flags&0x8000u){for(unsigned lane=0;lane<4;++lane){constexpr std::array pc{0x16c74cu,0x16c759u,0x16c766u,0x16c773u};const auto value=sub(zero,scalar(m.word(plane+lane*4u)));m.word(pc[lane],q.result+40u+lane*4u,bits(value));}}
  else{for(unsigned lane=0;lane<4;++lane){constexpr std::array pc{0x16c77fu,0x16c784u,0x16c78au,0x16c790u};m.word(pc[lane],q.result+40u+lane*4u,m.word(plane+lane*4u));}}
  const auto leaf=m.word(scratch.bsp_output+8u),surface=m.word(scratch.bsp_output+12u);m.word(0x16c79bu,q.result+76u,leaf);const auto material_word=m.word(scratch.bsp_output+16u);m.word(0x16c7a2u,q.result+80u,surface);const auto flags_byte=m.word(scratch.bsp_output+20u,1);m.word(0x16c7a9u,q.result+84u,material_word);const auto brush=m.word(scratch.bsp_output+21u,1);m.word(0x16c7b0u,q.result+88u,flags_byte,1);m.word(0x16c7b3u,q.result+89u,brush,1);m.word(0x16c7b6u,q.result+90u,partition,2);hit=true;
 }
 const auto leaf_count=m.word(scratch.bsp_output+24u);
 if(std::int32_t(leaf_count)>0){
  const auto first_leaf=m.word(scratch.bsp_output+28u);m.word(0x16c7d7u,q.result+20u,first_leaf);
  auto first_cluster=0xffffffffu;if(first_leaf!=0xffffffffu){const auto current_bsp=m.word(0x4ecc18u),leaf_rows=m.word(current_bsp+48u);first_cluster=sign16(m.word(leaf_rows+first_leaf*8u,2));}
  m.word(0x16c7eeu,q.result+24u,first_cluster,2);
  const auto last_leaf=m.word(scratch.bsp_output+24u+leaf_count*4u);m.word(0x16c7f9u,q.result+26u,slot,2);m.word(0x16c7fdu,q.result+28u,last_leaf);
  auto last_cluster=0xffffffffu;if(last_leaf!=0xffffffffu){const auto current_bsp=m.word(0x4ecc18u),leaf_rows=m.word(current_bsp+48u);last_cluster=sign16(m.word(leaf_rows+last_leaf*8u,2));}
  m.word(0x16c817u,q.result+32u,last_cluster,2);m.word(0x16c81bu,q.result+34u,slot,2);
  if((mask&14u)&&!(hit&&(mask&0x10000000u))){
   auto epoch=m.word(0x4f3ce4u)+1u;m.word(0x16c843u,0x4f3ce4u,epoch);m.word(0x16c849u,0x4f3ce1u,1,1);
   if(mask&4u){const auto serial=m.word(0x4f44ecu);m.word(0x16c852u,0x4f44ecu,serial+1u);m.word(0x16c858u,0x4f44e8u,1,1);}
   if(mask&8u){const auto serial=m.word(0x4eabccu)+1u;m.word(0x16c871u,0x4eabccu,serial);m.word(0x16c877u,0x4eabc8u,1,1);if(!(mask&0x1fff0u))mask|=0x1fff0u;}
   auto current_leaf_count=leaf_count;for(std::uint32_t index=0;std::int32_t(index)<std::int32_t(current_leaf_count);++index){
    if(index)epoch=m.word(0x4f3ce4u);
    const auto leaf=m.word(scratch.bsp_output+28u+index*4u);auto cluster=0xffffffffu;
    if(leaf!=0xffffffffu){const auto bsp=m.word(0x4ecc18u),rows=m.word(bsp+48u);cluster=sign16(m.word(rows+leaf*8u,2));}
    cluster=sign16(cluster);if(m.word(0x4f3ce8u+cluster*4u)==epoch){current_leaf_count=m.word(scratch.bsp_output+24u);continue;}m.word(0x16c8d8u,0x4f3ce8u+cluster*4u,epoch);
    const auto cluster_table=m.word(bsp_global+0xa0u),cluster_row=cluster_table+cluster*0xb0u;(void)cluster_row;if(stats)++stats->cluster_visits;
    if(mask&2u){
     if(stats)++stats->bridge_calls;const auto bridge_byte=m.word(cluster_row+0x70u,1);
     if(bridge_byte!=0xffu&&(bridge_byte&0x80u)){
      const auto bridges=m.word(bsp_global+0x68u),bridge_index=bridge_byte&0x7fu,bridge=bridges+bridge_index*24u;
      if(m.word(bridge+2u,2)!=0xffffu){
       auto distance=scalar(m.word(bridge+12u));distance=mul(distance,scalar(m.word(q.origin+8u)));auto y=scalar(m.word(bridge+8u));y=mul(y,scalar(m.word(q.origin+4u)));
       auto middle=scalar(m.word(bridge+8u));middle=mul(middle,scalar(m.word(q.displacement+4u)));const auto plane=bridge+4u;distance=add(distance,y);
       auto x=scalar(m.word(q.origin));x=mul(x,scalar(m.word(plane)));distance=add(distance,x);
       auto slope=scalar(m.word(plane+8u));slope=mul(slope,scalar(m.word(q.displacement+8u)));distance=sub(distance,scalar(m.word(plane+12u)));const bool distance_positive=compare(distance,zero).above();slope=add(slope,middle);
       auto first=scalar(m.word(q.displacement));first=mul(first,scalar(m.word(plane)));slope=add(slope,first);const bool slope_positive=compare(slope,zero).above();
       if(distance_positive!=slope_positive){
        const auto absolute_slope=absolute(bits(slope));
        if(absolute_slope_exceeds(bits(distance),absolute_slope)&&!compare(scalar(absolute_slope),scalar(m.word(0x46a0ecu))).below){
         const auto candidate=sub(zero,divide(distance,slope));
         if(compare(scalar(m.word(q.result+4u)),candidate).above()){
          const bool reverse=compare(zero,distance).above();m.word(0x16ca12u,q.result,2);m.word(0x16ca19u,q.result+4u,bits(candidate));const auto material=m.word(bridge+2u,2);m.word(0x16ca22u,q.result+36u,material,2);
          for(unsigned lane=0;lane<4;++lane){constexpr std::array pc{0x16ca2du,0x16ca32u,0x16ca38u,0x16ca3eu};m.word(pc[lane],q.result+40u+lane*4u,m.word(plane+lane*4u));}
          if(reverse){for(unsigned lane=0;lane<4;++lane){constexpr std::array pc{0x16ca4au,0x16ca56u,0x16ca63u,0x16ca70u};const auto value=sub(zero,scalar(m.word(q.result+40u+lane*4u)));m.word(pc[lane],q.result+40u+lane*4u,bits(value));}}
          m.word(0x16ca7fu,q.result+56u,bridge_index);m.word(0x16ca82u,q.result+60u,0xffffffffu);m.word(0x16ca85u,q.result+64u,0xffffffffu);hit=true;
         }
        }
       }
      }
     }
     if(hit&&(mask&0x10000000u))break;
    }
    if(mask&4u){if(stats)++stats->instanced_calls;if(!services.instanced){if(stats)++stats->unbound_services;throw std::runtime_error("Native camera query requires24DC90 instanced query");}const auto found=environment.dependency([&]{return services.instanced({cluster,mask,bsp_flags,q.origin,q.displacement,q.result},fp);});if(found)hit=true;if(hit&&(mask&0x10000000u))break;}
    if(mask&8u){m.word(0x16cb03u,scratch.payload,0);const auto object_filter=native_tu15_camera_object_query_flags(mask);
     auto datum=environment.dependency([&]{return child(0xba970u,[&]{return native_tu15_begin_spatial_object_query(memory,std::uint16_t(cluster),scratch.iterator,scratch.payload,observe);});});
     while(datum!=0xffffffffu){const auto serial=m.word(0x4eabccu),identity=datum&0xffffu;
      if(m.word(0x4eabd0u+identity*4u)!=serial){m.word(0x16cb5eu,0x4eabd0u+identity*4u,serial);const auto payload=m.word(scratch.payload),kind=m.word(payload,1);const auto kind_mask=(mask>>4u)&0x1fffu;
       if(kind_mask&(1u<<(kind&31u))){const auto excluded=m.word(payload+2u,2);if(!(excluded&object_filter)){const auto radius=m.word(payload+16u);const auto near=environment.dependency([&]{return child(0x120fa0u,[&]{return native_tu15_segment_near_sphere(memory,payload+4u,q.origin,q.displacement,radius,fp);});});
        if(near&&datum!=q.ignore_first&&datum!=q.ignore_second){if(stats)++stats->dynamic_calls;if(!services.dynamic){if(stats)++stats->unbound_services;throw std::runtime_error("Native camera query requires16C370 dynamic query");}const auto found=environment.dependency([&]{return services.dynamic({datum,1,mask,bsp_flags,q.origin,q.displacement,q.ignore_first,q.ignore_second,q.result},fp);});if(found)hit=true;}
       }}
      }
      if(hit&&(mask&0x10000000u))break;
      datum=environment.dependency([&]{return child(0xba9e0u,[&]{return native_tu15_next_spatial_object_query(memory,scratch.iterator,scratch.payload,observe);});});
     }
     if(hit&&(mask&0x10000000u))break;
    }
    current_leaf_count=m.word(scratch.bsp_output+24u);
   }
   if(mask&8u)m.word(0x16cc64u,0x4eabc8u,0,1);if(mask&4u)m.word(0x16cc70u,0x4f44e8u,0,1);m.word(0x16cc77u,0x4f3ce1u,0,1);
  }
 }
 auto fraction=scalar(m.word(q.result+4u));auto position=scalar(m.word(q.displacement));position=mul(position,fraction);position=add(position,scalar(m.word(q.origin)));m.word(0x16cca0u,q.result+8u,bits(position));
 position=mul(fraction,scalar(m.word(q.displacement+4u)));position=add(position,scalar(m.word(q.origin+4u)));m.word(0x16ccb1u,q.result+12u,bits(position));fraction=mul(fraction,scalar(m.word(q.displacement+8u)));fraction=add(fraction,scalar(m.word(q.origin+8u)));m.word(0x16ccc6u,q.result+16u,bits(fraction));
 if(hit&&(mask&0x20000000u)){
  if(stats)++stats->clearance_calls;std::uint32_t geometry{};
  if(m.word(q.result)!=1u)geometry=child(0x16bb40u,[&]{return native_tu15_camera_query_collision_geometry(memory,q.result);});
  const auto last_leaf=m.word(q.result+28u),position=q.result+8u;
  const auto point_leaf=[&](std::uint32_t tree){return environment.dependency([&]{return child(0x14cdb0u,[&]{return native_tu15_constraint_effect_point_leaf(memory,tree,0,position,fp);});});};
  bool repair{};
  if(last_leaf!=0xffffffffu){const auto tree=m.word(0x4ecc0cu);repair=point_leaf(tree)!=last_leaf;}
  if(!repair&&geometry){const auto geometry_leaf=m.word(q.result+76u);if(geometry_leaf!=0xffffffffu)repair=point_leaf(geometry)!=geometry_leaf;}
  if(repair){
   const auto nx=scalar(m.word(q.result+40u)),offset=scalar(m.word(0x46a38cu));auto component=mul(nx,offset);component=add(component,scalar(m.word(position)));m.word(0x16cd60u,position,bits(component));
   component=scalar(m.word(q.result+44u));component=mul(component,offset);component=add(component,scalar(m.word(position+4u)));m.word(0x16cd72u,position+4u,bits(component));
   component=scalar(m.word(q.result+48u));component=mul(component,offset);component=add(component,scalar(m.word(position+8u)));m.word(0x16cd87u,position+8u,bits(component));
   const auto location=[&]{environment.dependency([&]{child(0x11e600u,[&]{native_tu15_constraint_effect_location(memory,position,q.result+28u,fp,observe);return false;});return false;});};
   location();
   if(m.word(q.result+28u)==0xffffffffu||(geometry&&point_leaf(geometry)==0xffffffffu)){
    auto dot=scalar(m.word(q.result+48u));dot=mul(dot,scalar(m.word(q.displacement+8u)));auto y=scalar(m.word(q.result+44u));y=mul(y,scalar(m.word(q.displacement+4u)));dot=add(dot,y);auto x=scalar(m.word(q.displacement));x=mul(x,scalar(m.word(q.result+40u)));dot=add(dot,x);
    const bool is_zero=ordered_zero(dot);std::uint32_t decrement;
    if(!is_zero){const auto raw=bits(dot);asm volatile("flds %0;fabs"::"m"(raw):"memory","st");const auto numerator=m.word(0x46a38cu);asm volatile("fdivrs %1;fstps %0":"=m"(decrement):"m"(numerator):"memory","st");}
    else decrement=m.word(0x46a388u);
    do{
     auto value=sub(scalar(m.word(q.result+4u)),scalar(decrement));if(!compare(value,zero).above())value=zero;m.word(0x16ce28u,q.result+4u,bits(value));
     auto p=scalar(m.word(q.displacement));p=mul(p,value);p=add(p,scalar(m.word(q.origin)));m.word(0x16ce39u,position,bits(p));
     p=mul(value,scalar(m.word(q.displacement+4u)));p=add(p,scalar(m.word(q.origin+4u)));m.word(0x16ce4au,position+4u,bits(p));
     value=mul(value,scalar(m.word(q.displacement+8u)));value=add(value,scalar(m.word(q.origin+8u)));m.word(0x16ce5bu,position+8u,bits(value));
     location();if(!compare(zero,scalar(m.word(q.result+4u))).below)break;
    }while(m.word(q.result+28u)==0xffffffffu||(geometry&&point_leaf(geometry)==0xffffffffu));
   }
  }
 }
 if(stats)++stats->completed;return hit;
#else
 (void)memory;(void)q;(void)scratch;(void)fp;(void)depth;(void)services;(void)observe;throw std::runtime_error("Native camera geometric backend unavailable");
#endif
}
} // namespace halo2
