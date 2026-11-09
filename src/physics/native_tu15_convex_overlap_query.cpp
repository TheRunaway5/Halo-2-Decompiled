#include "halo2/native_tu15_convex_overlap_query.hpp"
#include "halo2/native_tu15_inverse_geometry.hpp"
#include "native_tu15_contact_unwind_internal.hpp"
#include <bit>
#include <cstring>
#include <stdexcept>
#include <type_traits>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
class Memory {
 SourcePhysicsContactMemory&m_;const SourcePhysicsContactRecordObserver&o_;
public:
 Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):m_(m),o_(o){}
 std::uint32_t word(std::uint32_t p)const{std::array<std::byte,4>b{};m_.read(p,b);std::uint32_t n{};for(unsigned k=0;k<4;++k)n|=std::to_integer<unsigned>(b[k])<<(k*8u);return n;}
 void word(std::uint32_t pc,std::uint32_t p,std::uint32_t n,unsigned size=4){std::array<std::byte,4>b{};for(unsigned k=0;k<size;++k)b[k]=std::byte(n>>(k*8u));const auto bytes=std::span(b).first(size);SourcePhysicsContactRecordStore e{pc,p,{},{}};if(o_){e.before.resize(size);m_.read(p,e.before);}m_.write(p,bytes);if(o_){e.written.assign(bytes.begin(),bytes.end());o_(e);}}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
 __m128 vector(std::uint32_t p)const{if(p&15u)throw std::runtime_error("Native sphere query requires aligned motion DATA");std::array<std::byte,16>b{};m_.read_vector(p,b);__m128 value;std::memcpy(&value,b.data(),16);return value;}
 void vector(std::uint32_t pc,std::uint32_t p,__m128 value){if(p&15u)throw std::runtime_error("Native sphere query requires aligned output DATA");std::array<std::byte,16>b{};std::memcpy(b.data(),&value,16);SourcePhysicsContactRecordStore e{pc,p,{},{}};if(o_){e.before.resize(16);m_.read(p,e.before);e.written.assign(b.begin(),b.end());}m_.write(p,b);if(o_)o_(e);}
 void scalar(std::uint32_t pc,std::uint32_t p,bool pop){
  SourcePhysicsContactRecordStore e{pc,p,{},{}};if(o_){e.before.resize(4);m_.read(p,e.before);}
  alignas(16)std::array<std::byte,108>before{};std::uint32_t value;
  asm volatile("fnsave %0;frstor %0":"=m"(before)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
  if(pop)asm volatile("fstps %0":"=m"(value)::"memory","st");else asm volatile("fsts %0":"=m"(value)::"memory","st");
  std::array<std::byte,4>b{};for(unsigned k=0;k<4;++k)b[k]=std::byte(value>>(k*8u));
  try{m_.write(p,b);}catch(...){asm volatile("frstor %0"::"m"(before):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");throw;}
  if(o_){e.written.assign(b.begin(),b.end());o_(e);}
 }
#endif
};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
    alignas(16)std::array<std::byte,108>host_{};std::array<std::byte,28>native_{};
    NativeTu15PhysicsFrameFp&fp_;unsigned previous_;bool active_{};
    void validate()const{if((fp_.control_word&0x3fu)!=0x3fu||(fp_.status&0x8080u)||(fp_.mxcsr&0x1f80u)!=0x1f80u||(fp_.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 convex query requires its unsupported FP exception owner");}
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp),previous_(_mm_getcsr()){
        if(depth>7)throw std::runtime_error("Native TU1.5 convex query requires its overfull x87 owner");validate();asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");std::memcpy(native_.data(),host_.data(),28);resume();
    }
    ~Environment(){if(active_)publish();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(previous_);}
    void publish(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));fp_.mxcsr=_mm_getcsr();active_=false;}
    void resume(){validate();const std::uint16_t empty=0xffff;std::memcpy(native_.data(),&fp_.control_word,2);std::memcpy(native_.data()+4,&fp_.status,2);std::memcpy(native_.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp_.mxcsr);active_=true;}
    template<class F>decltype(auto)dependency(F&&f){publish();try{if constexpr(std::is_void_v<std::invoke_result_t<F>>){f();resume();return;}else{auto result=f();resume();return result;}}catch(...){resume();throw;}}
};

__m128 query_multiply(__m128 a,__m128 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 query_add(__m128 a,__m128 b){asm volatile("addps %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 query_subtract(__m128 a,__m128 b){asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 query_add_scalar(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
std::uint32_t query_low(__m128 a){std::uint32_t n;std::memcpy(&n,&a,4);return n;}
std::uint32_t query_dot(__m128 a,__m128 b){const auto product=query_multiply(a,b);return query_low(query_add_scalar(_mm_shuffle_ps(product,product,0xaa),query_add_scalar(_mm_shuffle_ps(product,product,0x55),product)));}
// Complete actual3128C0 numerical/storage leaf. The owning query has already
// installed the current native FP environment. All retained cells are DATA;
// no source CPU frame or unused register image is constructed.
void closest_point_on_segment(Memory&m,std::uint32_t point,std::uint32_t first,
    std::uint32_t second,std::uint32_t output,std::uint32_t scratch){
 const auto a=m.vector(first),b=m.vector(second),p=m.vector(point);
 const auto direction=query_subtract(b,a),from_point=query_subtract(a,p);
 m.word(0x312904u,scratch+8u,query_dot(direction,from_point));
 const auto projection=m.word(scratch+8u);
 asm volatile("flds %0\n\tfchs"::"m"(projection):"memory","st");
 m.scalar(0x312911u,scratch+8u,false);
 const auto squared=query_multiply(direction,direction);const auto zero=m.word(0x46a0e8u);std::uint16_t relation;
 asm volatile("fcomps %1\n\tfnstsw %0":"=a"(relation):"m"(zero):"memory","st");
 const auto length=query_add_scalar(_mm_shuffle_ps(squared,squared,0xaau),query_add_scalar(_mm_shuffle_ps(squared,squared,0x55u),squared));
 m.word(0x31293du,scratch+12u,query_low(length));
 const auto parity=relation&0x4100u;
 if(parity!=0u&&parity!=0x4100u){m.vector(0x312946u,output,a);return;}
 const auto numerator=m.word(scratch+8u);
 asm volatile("flds %0"::"m"(numerator):"memory","st");
 const auto denominator=m.word(scratch+12u);
 asm volatile("fcomps %1\n\tfnstsw %0":"=a"(relation):"m"(denominator):"memory","st");
 if(!(relation&0x0100u)){m.vector(0x312967u,output,query_add(a,direction));return;}
 const auto value=m.word(scratch+8u);
 asm volatile("flds %0"::"m"(value):"memory","st");
 const auto divisor=m.word(scratch+12u);
 asm volatile("fdivs %0"::"m"(divisor):"memory","st");
 m.scalar(0x312980u,scratch+12u,true);
 const auto bits=m.word(scratch+12u);__m128 fraction{};std::memcpy(&fraction,&bits,4);fraction=_mm_shuffle_ps(fraction,fraction,0);
 m.vector(0x312997u,output,query_add(a,query_multiply(fraction,direction)));
}

#endif
}
void native_tu15_query_sphere_capsule_overlap(SourcePhysicsContactMemory&memory,
 NativeTu15PhysicsBody first,NativeTu15PhysicsBody second,
 NativeTu15ContactQueryCollector collector,NativeTu15SphereCapsuleOverlapQueryScratch scratch,
 NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15ConvexOverlapQueryServices&services,
 const SourcePhysicsContactRecordObserver&observe,NativeTu15ConvexOverlapQueryStats*stats){
 if(stats)++stats->calls;
 try{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  if(depth)throw std::runtime_error("Native sphere capsule overlap requires its qualified empty live x87 domain");
  if((scratch.identity&15u)||(scratch.closest_point&15u))throw std::runtime_error("Native sphere capsule overlap requires aligned owned DATA");
  Memory m(memory,observe);Environment environment(fp,depth);const auto p=scratch.identity;
  const auto profile=[&](std::uint32_t cursor,bool closing){
   if(cursor>=m.word(0x48c978u))return;
   m.word(closing?0x2faac2u:0x2fa9bau,cursor,closing?0x41c394u:0x41cdf4u);
   if(!services.geometry.timestamp_counter)throw std::runtime_error("Native sphere capsule overlap requires actual profiling clock");
   const auto stamp=std::uint32_t(environment.dependency(services.geometry.timestamp_counter));
   m.word(closing?0x2faacau:0x2fa9c2u,p+12u,stamp);const auto retained=m.word(p+12u);
   m.word(closing?0x2faad2u:0x2fa9cau,cursor+4u,retained);
   m.word(closing?0x2faad8u:0x2fa9d0u,0x48c974u,cursor+12u);if(stats)++stats->profile_entries;
  };
  profile(m.word(0x48c974u),false);
  const auto first_shape=m.word(first.identity),second_shape=m.word(second.identity);
  const auto first_motion=m.word(first.identity+8u),second_motion=m.word(second.identity+8u);
  m.word(0x2fa9e6u,p+12u,first_shape);
  for(unsigned n=0;n<2;++n){
   const auto vertex=m.vector(second_shape+16u+n*16u);
   const auto translation=m.vector(second_motion+0x50u),up=m.vector(second_motion+0x40u);
   const auto z_term=query_add(query_multiply(up,_mm_shuffle_ps(vertex,vertex,0xaau)),translation);
   const auto side=m.vector(second_motion+0x30u);
   const auto y_term=query_multiply(side,_mm_shuffle_ps(vertex,vertex,0x55u));
   const auto forward=m.vector(second_motion+0x20u);
   const auto x_term=query_multiply(forward,_mm_shuffle_ps(vertex,vertex,0));
   m.vector(0x2faa39u,p+0x20u+n*16u,query_add(query_add(x_term,y_term),z_term));
  }
  closest_point_on_segment(m,first_motion+0x50u,p+0x20u,p+0x30u,p+0x10u,scratch.closest_point);
  const auto radius_b=m.word(second_shape+12u);asm volatile("flds %0"::"m"(radius_b):"memory","st");
  const auto current_first=m.word(p+12u),radius_a=m.word(current_first+12u);
  asm volatile("fadds %0"::"m"(radius_a):"memory","st");
  const auto centre=m.vector(first_motion+0x50u),closest=m.vector(p+0x10u);
  const auto delta=query_subtract(closest,centre),squared=query_multiply(delta,delta);
  asm volatile("fld %%st(0)\n\tfmul %%st(1)":::"memory","st","st(1)");
  const auto length=query_add_scalar(_mm_shuffle_ps(squared,squared,0xaau),query_add_scalar(_mm_shuffle_ps(squared,squared,0x55u),squared));
  m.word(0x2faa8eu,p+12u,query_low(length));const auto distance=m.word(p+12u);std::uint16_t relation;
  asm volatile("flds %1\n\tfcompp\n\tfnstsw %0\n\tfstp %%st(0)":"=a"(relation):"m"(distance):"memory","st","st(1)");
  if(std::popcount(unsigned((relation>>8u)&5u))&1u){
   const auto table=m.word(collector.identity);const NativeTu15PhysicsVirtualOwner owner{m.word(table+4u)};
   if(!services.collect)throw std::runtime_error("Native sphere capsule overlap requires actual collector virtual+4");
   if(stats)++stats->collect_calls;environment.dependency([&]{services.collect(collector,owner,first,second,fp);});
  }
  profile(m.word(0x48c974u),true);if(stats)++stats->completed;
#else
  (void)memory;(void)first;(void)second;(void)collector;(void)scratch;(void)fp;(void)depth;(void)services;(void)observe;
  throw std::runtime_error("Exact native sphere capsule overlap FP backend unavailable");
#endif
 }catch(...){if(stats)++stats->refusals;throw;}
}
void native_tu15_query_reversed_sphere_capsule_overlap(SourcePhysicsContactMemory&memory,
 NativeTu15PhysicsBody first,NativeTu15PhysicsBody second,
 NativeTu15ContactQueryCollector collector,NativeTu15SphereCapsuleOverlapQueryScratch scratch,
 NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15ConvexOverlapQueryServices&services,
 const SourcePhysicsContactRecordObserver&observe,NativeTu15ConvexOverlapQueryStats*stats){
 if(depth)throw std::runtime_error("Native sphere capsule overlap requires its qualified empty live x87 domain");
 Memory m(memory,observe);const auto local=scratch.collector;
 m.word(0x2fab8eu,local+4u,0u,1);m.word(0x2fab92u,local,0x41ca40u);m.word(0x2fab99u,local+8u,collector.identity);
 native_contact_detail::UnwindGuard cleanup([&]{m.word(0x2f0dd0u,local,0x461580u);});cleanup.arm();
 native_tu15_query_sphere_capsule_overlap(memory,second,first,{local},scratch,fp,depth,services,observe,stats);
 cleanup.disarm();
}
void native_tu15_query_sphere_overlap(SourcePhysicsContactMemory&memory,
 NativeTu15PhysicsBody first,NativeTu15PhysicsBody second,
 NativeTu15ContactQueryCollector collector,NativeTu15ConvexOverlapQueryScratch scratch,
 NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15ConvexOverlapQueryServices&services,
 const SourcePhysicsContactRecordObserver&observe,NativeTu15ConvexOverlapQueryStats*stats){
 if(stats)++stats->calls;
 try{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  if(depth)throw std::runtime_error("Native sphere overlap requires its qualified empty live x87 domain");
  if(scratch.identity&15u)throw std::runtime_error("Native sphere overlap requires aligned owned DATA");
  Memory m(memory,observe);Environment environment(fp,depth);const auto p=scratch.identity;
  const auto profile=[&](std::uint32_t cursor,std::uint32_t limit,bool closing){
   if(cursor>=limit)return cursor;
   m.word(closing?0x2f9d42u:0x2f9cb9u,cursor,closing?0x41c394u:0x41cda0u);
   if(!services.geometry.timestamp_counter)throw std::runtime_error("Native sphere overlap requires actual profiling clock");
   const auto timestamp=std::uint32_t(environment.dependency(services.geometry.timestamp_counter));
   m.word(closing?0x2f9d4au:0x2f9cc1u,p+0xcu,timestamp);
   const auto retained=m.word(p+0xcu);
   m.word(closing?0x2f9d52u:0x2f9cc9u,cursor+4u,retained);
   m.word(closing?0x2f9d58u:0x2f9ccfu,0x48c974u,cursor+12u);
   if(stats)++stats->profile_entries;return cursor+12u;
  };
  auto cursor=m.word(0x48c974u);cursor=profile(cursor,m.word(0x48c978u),false);
  const auto first_motion=m.word(first.identity+8u);const auto a=m.vector(first_motion+0x50u);
  const auto second_motion=m.word(second.identity+8u);auto b=m.vector(second_motion+0x50u);
  const auto second_shape=m.word(second.identity);
  asm volatile("subps %1,%0\n\tmulps %0,%0":"+x"(b):"x"(a):"memory");
  auto y=_mm_shuffle_ps(b,b,0x55),z=_mm_shuffle_ps(b,b,0xaa);
  asm volatile("addss %1,%0":"+x"(y):"x"(b):"memory");
  asm volatile("addss %1,%0":"+x"(z):"x"(y):"memory");
  std::uint32_t distance;std::memcpy(&distance,&z,4);m.word(0x2f9d0bu,p+0xcu,distance);
  const auto first_shape=m.word(first.identity);const auto radius_a=m.word(first_shape+0xcu);
  asm volatile("flds %0"::"m"(radius_a):"memory","st");
  const auto radius_b=m.word(second_shape+0xcu);
  asm volatile("fadds %0\n\tfld %%st(0)\n\tfmul %%st(1)"::"m"(radius_b):"memory","st","st(1)");
  const auto squared=m.word(p+0xcu);std::uint16_t relation;
  asm volatile("flds %1\n\tfcompp\n\tfnstsw %0\n\tfstp %%st(0)":"=a"(relation):"m"(squared):"memory","st","st(1)");
  if(std::popcount(unsigned((relation>>8u)&5u))&1u){
   const auto table=m.word(collector.identity);const NativeTu15PhysicsVirtualOwner owner{m.word(table+4u)};
   if(!services.collect)throw std::runtime_error("Native sphere overlap requires actual collector virtual+4");
   if(stats)++stats->collect_calls;
   environment.dependency([&]{services.collect(collector,owner,first,second,fp);});
   cursor=m.word(0x48c974u);
  }
  profile(cursor,m.word(0x48c978u),true);if(stats)++stats->completed;
#else
  (void)memory;(void)first;(void)second;(void)collector;(void)scratch;(void)fp;(void)depth;(void)services;(void)observe;
  throw std::runtime_error("Exact native sphere overlap FP backend unavailable");
#endif
 }catch(...){if(stats)++stats->refusals;throw;}
}
void native_tu15_ignore_contact_process() noexcept {}
void native_tu15_mark_contact_query_hit(SourcePhysicsContactMemory&m,
 NativeTu15ContactQueryCollector collector,const SourcePhysicsContactRecordObserver&o){
 Memory data(m,o);data.word(0x30ba80u,collector.identity+4u,1u,1u);
}
void native_tu15_query_convex_overlap(SourcePhysicsContactMemory&memory,
 NativeTu15IslandCollisionAgent agent,NativeTu15PhysicsBody first,
 NativeTu15PhysicsBody second,NativeTu15PhysicsStepConfiguration configuration,
 NativeTu15ContactQueryCollector collector,NativeTu15ConvexOverlapQueryScratch scratch,
 NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15ConvexOverlapQueryServices&services,
 const SourcePhysicsContactRecordObserver&observe,NativeTu15ConvexOverlapQueryStats*stats){
 (void)configuration;if(stats)++stats->calls;
 try{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  if(scratch.identity&15u)throw std::runtime_error("Native convex overlap requires aligned owned DATA");
  Memory m(memory,observe);Environment environment(fp,depth);const auto p=scratch.identity;
  const auto profile=[&](std::uint32_t cursor,std::uint32_t limit,bool closing){
   if(cursor>=limit)return;
   m.word(closing?0x2f84b4u:0x2f8432u,cursor,closing?0x41c394u:0x41cd2cu);
   if(!services.geometry.timestamp_counter)throw std::runtime_error("Native convex overlap requires actual profiling clock");
   const auto timestamp=std::uint32_t(environment.dependency(services.geometry.timestamp_counter));
   const auto retained=p+(closing?0x18u:0x14u);
   m.word(closing?0x2f84bcu:0x2f843au,retained,timestamp);
   const auto stamp=m.word(retained);
   m.word(closing?0x2f84c4u:0x2f8442u,cursor+4u,stamp);
   m.word(closing?0x2f84cau:0x2f8448u,0x48c974u,cursor+12u);
   if(stats)++stats->profile_entries;
  };
  const auto limit=m.word(0x48c978u);
  m.word(0x2f8422u,p+0x18u,agent.identity);
  const auto cursor=m.word(0x48c974u);profile(cursor,limit,false);
  const auto second_motion=m.word(second.identity+8u);
  const auto second_shape=m.word(second.identity);
  const auto first_motion=m.word(first.identity+8u);
  const auto first_shape=m.word(first.identity);
  m.word(0x2f8465u,p+0x14u,second_shape);
  environment.dependency([&]{NativeTu15InverseGeometryMemory transform(memory,p+0x30u,
   first_motion+0x20u,second_motion+0x20u,observe);native_tu15_compose_inverse_affine_geometry(transform,fp);});
  const auto cache_agent=m.word(p+0x18u),current_second=m.word(p+0x14u);
  m.word(0x2f8492u,p+0x7cu,1u,1u);
  if(!services.geometry.solve_simplex)throw std::runtime_error("Native convex overlap requires actual30E930 simplex owner");
  if(stats)++stats->solver_calls;
  const auto result=environment.dependency([&]{return services.geometry.solve_simplex(p+0x70u,
   {first_shape},{current_second},p+0x30u,cache_agent+12u,p+0x20u,fp);});
  const auto final_limit=m.word(0x48c978u);
  m.word(0x2f84a5u,p+0x1cu,result);
  const auto final_cursor=m.word(0x48c974u);profile(final_cursor,final_limit,true);
  bool collect=m.word(p+0x1cu)!=0;
  if(!collect){
   const auto plane_distance=m.word(p+0x2cu);asm volatile("flds %0"::"m"(plane_distance):"memory","st");
   const auto radius_shape=m.word(p+0x14u);
   const auto first_radius=m.word(first_shape+12u);asm volatile("fsubs %0"::"m"(first_radius):"memory","st");
   const auto second_radius=m.word(radius_shape+12u);asm volatile("fsubs %0"::"m"(second_radius):"memory","st");
   const auto zero=m.word(0x46a0e8u);std::uint16_t relation;
   asm volatile("fcomps %1\n\tfnstsw %0":"=a"(relation):"m"(zero):"memory","st");
   collect=(std::popcount(unsigned((relation>>8u)&5u))&1u)!=0;
  }
  if(collect){
   const auto table=m.word(collector.identity);
   const NativeTu15PhysicsVirtualOwner owner{m.word(table+4u)};
   if(!services.collect)throw std::runtime_error("Native convex overlap requires actual collector virtual+4");
   if(stats)++stats->collect_calls;
   environment.dependency([&]{services.collect(collector,owner,first,second,fp);});
  }
  if(stats)++stats->completed;
#else
  (void)memory;(void)agent;(void)first;(void)second;(void)collector;(void)scratch;(void)fp;(void)depth;(void)services;(void)observe;
  throw std::runtime_error("Exact native convex overlap FP backend unavailable");
#endif
 }catch(...){if(stats)++stats->refusals;throw;}
}
} // namespace halo2
