#include "halo2/native_tu15_convex_overlap_query.hpp"
#include "halo2/native_tu15_inverse_geometry.hpp"
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

#endif
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
