#include "halo2/native_tu15_camera_collision_ray.hpp"
#include "halo2/native_tu15_camera_collision_regions.hpp"
#include "halo2/native_tu15_camera_projection.hpp"
#include "halo2/native_tu15_bsp_segment_query.hpp"
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
bool different_or_unordered(F a,F b){unsigned char different,unordered;asm volatile("ucomiss %3,%2;setne %0;setp %1":"=qm"(different),"=qm"(unordered):"x"(a),"x"(b):"cc","memory");return different||unordered;}
#endif
}
bool native_tu15_query_camera_collision_ray(SourcePhysicsContactMemory&memory,
 const NativeTu15CameraCollisionRay&q,NativeTu15CameraCollisionRayScratch scratch,
 NativeTu15PhysicsFrameFp&fp,unsigned depth,const SourcePhysicsContactRecordObserver&observe,
 NativeTu15CameraCollisionRayStats*stats){
 if(stats)++stats->calls;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
 Memory m(memory,observe);const auto maximum=m.word(0x45695cu);m.word(0x1f8220u,q.result+8u,maximum);
 const auto model=m.word(q.context+4u),state=m.word(q.context+12u),collision=m.word(q.context+8u);
 native_tu15_begin_camera_collision_regions(memory,collision,state,scratch.iterator,model,observe);
 bool hit{};auto more=native_tu15_next_camera_collision_region(memory,scratch.iterator,observe);
 while(more){
  if(stats)++stats->regions;
  const auto encoded=m.word(scratch.iterator+20u),matrices=m.word(q.context+16u),node=encoded&255u,frame=matrices+node*52u;
  if(stats)++stats->inverse_calls;native_tu15_camera_inverse_frame(memory,frame,scratch.inverse,fp,observe);
  Environment environment(fp,depth);
  const auto inverse=scratch.inverse;const auto scale=scalar(m.word(inverse));const bool scale_origin=different_or_unordered(scale,scalar(m.word(0x46a0d0u)));
  auto ox=scalar(m.word(q.origin)),oy=scalar(m.word(q.origin+4u)),oz=scalar(m.word(q.origin+8u));
  if(scale_origin){ox=mul(scale,ox);oy=mul(scale,oy);oz=mul(scale,oz);}
  const auto a10=scalar(m.word(inverse+16u));auto a4=scalar(m.word(inverse+4u));auto a14=scalar(m.word(inverse+20u));
  a4=mul(a4,ox);const bool scale_displacement=different_or_unordered(scale,scalar(m.word(0x46a0d0u)));
  auto x=mul(a10,oy);x=add(x,a4);auto other=mul(oz,scalar(m.word(inverse+28u)));x=add(x,other);x=add(x,scalar(m.word(inverse+40u)));a14=mul(a14,oy);m.word(0x1f8315u,scratch.local_origin,bits(x));
  const auto a20=scalar(m.word(inverse+32u));auto y=mul(a20,oz);y=add(y,a14);other=scalar(m.word(inverse+8u));other=mul(other,ox);y=add(y,other);y=add(y,scalar(m.word(inverse+44u)));m.word(0x1f8340u,scratch.local_origin+4u,bits(y));
  const auto a24=scalar(m.word(inverse+36u));auto z=mul(a24,oz);auto middle=scalar(m.word(inverse+24u));middle=mul(middle,oy);auto first=scalar(m.word(inverse+12u));z=add(z,middle);
  auto dz=scalar(m.word(q.displacement+8u));first=mul(first,ox);auto dx=scalar(m.word(q.displacement));z=add(z,first);auto dy=scalar(m.word(q.displacement+4u));z=add(z,scalar(m.word(inverse+48u)));m.word(0x1f8387u,scratch.local_origin+8u,bits(z));
  if(scale_displacement){dx=mul(dx,scale);dy=mul(dy,scale);dz=mul(dz,scale);}
  auto delta_x_first=scalar(m.word(inverse+4u));const auto descriptor=m.word(scratch.iterator+24u);auto delta_x=mul(a10,dy);delta_x_first=mul(delta_x_first,dx);delta_x=add(delta_x,delta_x_first);other=mul(dz,scalar(m.word(inverse+28u)));delta_x=add(delta_x,other);m.word(0x1f83c8u,scratch.local_displacement,bits(delta_x));
  auto delta_y=scalar(m.word(inverse+20u));delta_y=mul(delta_y,dy);auto last_y=mul(a20,dz);last_y=add(last_y,delta_y);other=scalar(m.word(inverse+8u));other=mul(other,dx);last_y=add(last_y,other);
  auto delta_z=scalar(m.word(inverse+24u));delta_z=mul(delta_z,dy);auto last_z=mul(a24,dz);last_z=add(last_z,delta_z);other=scalar(m.word(inverse+12u));other=mul(other,dx);last_z=add(last_z,other);
  const auto fraction=scalar(m.word(q.result+8u)),zero=F{};m.word(0x1f8414u,scratch.local_displacement+4u,bits(last_y));const bool negative=compare(zero,fraction).above();
  m.word(0x1f8420u,scratch.bsp_context,q.bsp_flags);m.word(0x1f8424u,scratch.bsp_context+4u,descriptor);
  m.word(0x1f8430u,scratch.local_displacement+8u,bits(last_z));m.word(0x1f8436u,scratch.bsp_context+8u,0,2);m.word(0x1f843bu,scratch.bsp_context+12u,0);
  m.word(0x1f843fu,scratch.bsp_context+16u,scratch.local_origin);m.word(0x1f8443u,scratch.bsp_context+20u,scratch.local_displacement);m.word(0x1f8447u,scratch.bsp_context+24u,q.result+8u);
  const auto initial=negative?zero:fraction;const auto negative_again=compare(zero,fraction).above();m.word(0x1f8459u,q.result+8u,bits(initial));m.word(0x1f845du,q.result+32u,0);
  m.word(0x1f8460u,scratch.bsp_context+28u,0xffffffffu);m.word(0x1f8467u,scratch.bsp_context+32u,0,1);m.word(0x1f846eu,scratch.bsp_context+36u,0xffffffffu);
  std::uint32_t limit{};if(!negative_again){const auto one=m.word(0x46a0d0u);limit=compare(fraction,scalar(one)).above()?one:bits(fraction);}
  if(stats)++stats->bsp_calls;
  const auto found=environment.dependency([&]{return native_tu15_traverse_bsp_segment(memory,scratch.bsp_context,0,0,limit,fp,depth,observe);});
  if(found){const auto region=m.word(scratch.iterator+23u,1);m.word(0x1f84b6u,q.result,encoded);m.word(0x1f84b9u,q.result+4u,node,2);m.word(0x1f84bdu,q.result+6u,region,2);hit=true;if(stats)++stats->hits;}
  more=native_tu15_next_camera_collision_region(memory,scratch.iterator,observe);
 }
 if(stats)++stats->completed;return hit;
#else
 (void)memory;(void)q;(void)scratch;(void)fp;(void)depth;(void)observe;throw std::runtime_error("Native camera collision ray backend unavailable");
#endif
}
} // namespace halo2
