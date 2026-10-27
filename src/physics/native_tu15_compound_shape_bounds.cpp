#include "halo2/native_tu15_compound_shape_bounds.hpp"
#include <cstring>
#include <bit>
#include <stdexcept>
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
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  __m128 vector(std::uint32_t p)const{
    if(p&15u)throw std::runtime_error("Native TU1.5 compound bounds requires actual aligned vector storage");
    alignas(16)std::array<std::byte,16>b{};m_.read(p,b);__m128 value;std::memcpy(&value,b.data(),16);return value;
  }
  void vector(std::uint32_t pc,std::uint32_t p,__m128 value){
    if(p&15u)throw std::runtime_error("Native TU1.5 compound bounds requires actual aligned vector storage");
    SourcePhysicsContactRecordStore event{pc,p,{},{}};if(o_){event.before.resize(16);m_.read(p,event.before);}
    std::array<std::byte,16>b{};std::memcpy(b.data(),&value,16);m_.write(p,b);
    if(o_){event.written.assign(b.begin(),b.end());o_(event);}
  }
#endif
};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
  alignas(16)std::array<std::byte,108>host_{};
  NativeTu15PhysicsFrameFp&fp_;unsigned host_mxcsr_;
public:
  Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth=0):fp_(fp){
    if(depth>7||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||
      (fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))
      throw std::runtime_error("Native TU1.5 compound bounds requires its unsupported FP owner");
    host_mxcsr_=_mm_getcsr();
    asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    std::array<std::byte,28>env{};std::memcpy(env.data(),host_.data(),28);const std::uint16_t empty=0xffffu;
    std::memcpy(env.data(),&fp.control_word,2);std::memcpy(env.data()+4,&fp.status,2);std::memcpy(env.data()+8,&empty,2);
    asm volatile("fldenv %0"::"m"(env):"memory");_mm_setcsr(fp.mxcsr);
  }
  ~Environment(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));fp_.mxcsr=_mm_getcsr();
    asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(host_mxcsr_);}
};
__m128 add(__m128 left,__m128 right){asm volatile("addps %1,%0":"+x"(left):"x"(right));return left;}
__m128 multiply(__m128 left,__m128 right){asm volatile("mulps %1,%0":"+x"(left):"x"(right));return left;}
__m128 minimum(__m128 left,__m128 right){asm volatile("minps %1,%0":"+x"(left):"x"(right));return left;}
__m128 maximum(__m128 left,__m128 right){asm volatile("maxps %1,%0":"+x"(left):"x"(right));return left;}
__m128 broadcast(std::uint32_t bits){float value;std::memcpy(&value,&bits,4);return _mm_set1_ps(value);}
#endif
}
void native_tu15_endpoint_shape_bounds(SourcePhysicsContactMemory&memory,NativeTu15BoundsShape shape,
 NativeTu15BoundsTransform transform,std::uint32_t margin,NativeTu15BoundsOutput output,
 NativeTu15PhysicsFrameFp&fp,unsigned depth,const SourcePhysicsContactRecordObserver&observe,
 NativeTu15CompoundBoundsStats*stats){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
 if(stats)++stats->endpoint_calls;Memory m(memory,observe);Environment environment(fp,depth);
 asm volatile("flds %0"::"m"(margin):"memory","st");const auto radius=m.word(shape.identity+12u);
 asm volatile("fadds %0"::"m"(radius):"memory","st");
 const auto x=m.vector(transform.identity),translation=m.vector(transform.identity+48u),z=m.vector(transform.identity+32u);
 std::uint32_t expanded;asm volatile("fstps %0":"=m"(expanded)::"memory","st");const auto y=m.vector(transform.identity+16u);const auto padding=broadcast(expanded);
 std::array<__m128,2>endpoints;
 for(unsigned n=0;n<2;++n){const auto point=m.vector(shape.identity+16u+n*16u);
  const auto z_term=multiply(z,_mm_shuffle_ps(point,point,0xaau));
  const auto y_term=multiply(y,_mm_shuffle_ps(point,point,0x55u));
  const auto x_term=multiply(x,_mm_shuffle_ps(point,point,0));
  const auto shifted_z=add(z_term,translation),xy=add(x_term,y_term);endpoints[n]=add(xy,shifted_z);
 }
 const auto upper=maximum(endpoints[0],endpoints[1]),lower=minimum(endpoints[0],endpoints[1]);
 m.vector(0x2e81b7u,output.identity+16u,upper);
 __m128 expanded_lower=lower;asm volatile("subps %1,%0":"+x"(expanded_lower):"x"(padding):"memory");
 m.vector(0x2e81beu,output.identity,expanded_lower);const auto current_upper=m.vector(output.identity+16u);
 m.vector(0x2e81c8u,output.identity+16u,add(current_upper,padding));
#else
 (void)memory;(void)shape;(void)transform;(void)margin;(void)output;(void)fp;(void)depth;(void)observe;(void)stats;throw std::runtime_error("Exact native TU1.5 endpoint bounds backend unavailable");
#endif
}
void native_tu15_compound_shape_bounds(SourcePhysicsContactMemory&memory,NativeTu15BoundsShape shape,
 NativeTu15BoundsTransform transform,std::uint32_t margin,NativeTu15BoundsOutput output,
 NativeTu15CompoundBoundsScratch scratch,NativeTu15PhysicsFrameFp&fp,
 const NativeTu15ShapeBoundsServices&services,const SourcePhysicsContactRecordObserver&observe,
 NativeTu15CompoundBoundsStats*stats){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
 if(stats)++stats->compound_calls;Memory m(memory,observe);
 const auto call_child=[&](NativeTu15BoundsShape child,NativeTu15BoundsOutput destination,std::uint32_t table){
  const auto owner=m.word(table+24u);if(!services.child_bounds)throw std::runtime_error("Native TU1.5 compound bounds requires actual child owner");
  if(stats)++stats->child_calls;services.child_bounds(child,owner,transform,margin,destination,fp);
 };
 const auto first_list=m.word(shape.identity+12u);const NativeTu15BoundsShape first{m.word(first_list)};const auto first_table=m.word(first.identity);
 call_child(first,output,first_table);auto count=m.word(shape.identity+16u);std::uint32_t index=1;
 if(std::bit_cast<std::int32_t>(count)<=1)return;
 do{const auto list=m.word(shape.identity+12u);const NativeTu15BoundsShape child{m.word(list+index*8u)};const auto table=m.word(child.identity);
  call_child(child,{scratch.bounds},table);
  {Environment environment(fp);const auto child_minimum=m.vector(scratch.bounds),current_minimum=m.vector(output.identity);const auto lower=minimum(current_minimum,child_minimum);
   const auto child_maximum=m.vector(scratch.bounds+16u);m.vector(0x3061c8u,output.identity,lower);
   const auto current_maximum=m.vector(output.identity+16u);m.vector(0x3061d2u,output.identity+16u,maximum(current_maximum,child_maximum));}
  count=m.word(shape.identity+16u);++index;
 }while(std::bit_cast<std::int32_t>(index)<std::bit_cast<std::int32_t>(count));
#else
 (void)memory;(void)shape;(void)transform;(void)margin;(void)output;(void)scratch;(void)fp;(void)services;(void)observe;(void)stats;throw std::runtime_error("Exact native TU1.5 compound bounds backend unavailable");
#endif
}
} // namespace halo2
