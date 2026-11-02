#include "halo2/native_tu15_camera_dynamic_query.hpp"
#include "halo2/native_tu15_camera_dynamic_query_leaves.hpp"
#include "halo2/native_tu15_camera_query_object_context.hpp"
#include "halo2/native_tu15_camera_query_leaves.hpp"
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
std::uint32_t crossing(std::uint32_t distance,std::uint32_t slope){std::uint32_t out;asm volatile("flds %1;fdivs %2;fchs;fstps %0":"=m"(out):"m"(distance),"m"(slope):"memory","st");return out;}
bool query(SourcePhysicsContactMemory&memory,const NativeTu15CameraDynamicQuery&q,
 NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15CameraDynamicServices&services,
 const SourcePhysicsContactRecordObserver&observe,NativeTu15CameraDynamicStats*stats,unsigned recursion){
 if(stats)++stats->calls;
 if(!services.acquire_frame){if(stats)++stats->unbound_data_owners;throw std::runtime_error("Native dynamic camera query requires owned frame DATA");}
 const auto frame=services.acquire_frame(recursion);Memory m(memory,observe);auto datum=q.datum;bool hit{};
 for(;;){
  if(stats)++stats->objects;
  const auto objects=m.word(0x4ecbd0u),headers=m.word(objects+0x44u),header=headers+(datum&0xffffu)*12u,object=m.word(header+8u);
  bool admitted=true;
  if(!(q.recursive&255u)){
   if(stats)++stats->admission_calls;admitted=native_tu15_admit_camera_query_object(memory,datum,header,object,q.mask,q.ignore_first,q.ignore_second);
   if(admitted){const auto radius=m.word(object+0x4cu);if(stats)++stats->sphere_calls;admitted=native_tu15_segment_near_sphere(memory,object+0x40u,q.origin,q.displacement,radius,fp);}
  }
  if(admitted){
   if(stats)++stats->context_calls;
   const auto resolved=native_tu15_resolve_camera_collision_context(memory,datum,frame.collision_context,observe);
   if(resolved){
    if(stats)++stats->ray_calls;
    const auto found=native_tu15_query_camera_collision_ray(memory,{frame.collision_context,q.bsp_filter,q.origin,q.displacement,frame.ray_result},frame.ray,fp,depth,observe);
    if(found){
     Environment environment(fp,depth);
     const auto current_fraction=scalar(m.word(q.result+4u));const auto candidate_fraction=scalar(m.word(frame.ray_result+8u));
     if(compare(current_fraction,candidate_fraction).above()){
      const auto material_index=sign16(m.word(frame.ray_result+30u,2)),collision_object=m.word(frame.collision_context);
      m.word(0x16c454u,q.result,4);
      const auto material=environment.dependency([&]{return native_tu15_camera_collision_material_to(memory,collision_object,material_index,frame.material,observe);});
      const auto material_id=m.word(material,2),key=m.word(frame.ray_result);m.word(0x16c467u,q.result+36u,material_id,2);
      const auto matrices=m.word(frame.collision_context+16u);m.word(0x16c472u,q.result+56u,0xffffffffu);m.word(0x16c475u,q.result+60u,0xffffffffu);
      const auto region=m.word(frame.ray_result+6u,2);m.word(0x16c47du,q.result+68u,region,2);
      const auto node=m.word(frame.ray_result+4u,2);m.word(0x16c486u,q.result+72u,key);m.word(0x16c48fu,q.result+70u,node,2);const auto transform=matrices+sign16(node)*52u;
      m.word(0x16c49bu,q.result+64u,datum);
      environment.dependency([&]{native_tu15_publish_camera_collision(memory,q.result,transform,frame.ray_result+8u,fp,observe);return false;});
      hit=true;if(stats)++stats->hits;if(q.mask&0x10000000u){if(stats)++stats->completed;return true;}
     }
    }
   }
   if(!(q.mask&0x20000u)){
    const auto child=m.word(object+16u);
    if(child!=0xffffffffu){
     if(stats)++stats->recursive_calls;
     const auto found=query(memory,{child,0,q.mask,q.bsp_filter,q.origin,q.displacement,q.ignore_first,q.ignore_second,q.result},fp,depth,services,observe,stats,recursion+1u);
     if(found){hit=true;if(q.mask&0x10000000u){if(stats)++stats->completed;return true;}}
    }
   }
  }
  datum=m.word(object+12u);if(datum==0xffffffffu)break;
 }
 if(stats)++stats->completed;return hit;
}
#endif
}
bool native_tu15_query_dynamic_camera_object(SourcePhysicsContactMemory&memory,
 const NativeTu15CameraDynamicQuery&q,NativeTu15PhysicsFrameFp&fp,unsigned depth,
 const NativeTu15CameraDynamicServices&services,const SourcePhysicsContactRecordObserver&observe,
 NativeTu15CameraDynamicStats*stats){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
 return query(memory,q,fp,depth,services,observe,stats,0);
#else
 (void)memory;(void)q;(void)fp;(void)depth;(void)services;(void)observe;(void)stats;throw std::runtime_error("Native dynamic camera query backend unavailable");
#endif
}
} // namespace halo2
