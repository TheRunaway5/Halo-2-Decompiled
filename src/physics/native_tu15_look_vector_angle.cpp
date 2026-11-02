#include "halo2/native_tu15_look_vector_angle.hpp"
#include <cstring>
#include <stdexcept>
#include <type_traits>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
class Memory{
 SourcePhysicsContactMemory&m_;
public:explicit Memory(SourcePhysicsContactMemory&m):m_(m){}
 std::uint32_t word(std::uint32_t p)const{std::array<std::byte,4>b{};m_.read(p,b);std::uint32_t n{};for(unsigned k=0;k<4;++k)n|=std::to_integer<unsigned>(b[k])<<(k*8u);return n;}
};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
    alignas(16)std::array<std::byte,108>host_{};std::array<std::byte,28>native_{};
    NativeTu15PhysicsFrameFp&fp_;unsigned previous_;bool active_{};
    void validate()const{if((fp_.control_word&0x3fu)!=0x3fu||(fp_.status&0x8080u)||(fp_.mxcsr&0x1f80u)!=0x1f80u||(fp_.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 look vector angle requires its unsupported FP exception owner");}
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp),previous_(_mm_getcsr()){
        if(depth>7)throw std::runtime_error("Native TU1.5 look vector angle requires its overfull x87 owner");validate();asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");std::memcpy(native_.data(),host_.data(),28);resume();
    }
    ~Environment(){if(active_)publish();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(previous_);}
    void publish(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));fp_.mxcsr=_mm_getcsr();active_=false;}
    void resume(){validate();const std::uint16_t empty=0xffff;std::memcpy(native_.data(),&fp_.control_word,2);std::memcpy(native_.data()+4,&fp_.status,2);std::memcpy(native_.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp_.mxcsr);active_=true;}
    template<class F>decltype(auto)dependency(F&&f){publish();try{if constexpr(std::is_void_v<std::invoke_result_t<F>>){f();resume();return;}else{auto result=f();resume();return result;}}catch(...){resume();throw;}}
};

using F=__m128;
F scalar(std::uint32_t n){F v{};std::memcpy(&v,&n,4);return v;}
std::uint32_t bits(F v){std::uint32_t n;std::memcpy(&n,&v,4);return n;}
F add(F a,F b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
F sub(F a,F b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
F mul(F a,F b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
F divide(F a,F b){asm volatile("divss %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool above(F a,F b){unsigned char n;asm volatile("comiss %2,%1;seta %0":"=qm"(n):"x"(a),"x"(b):"cc","memory");return n;}
bool zero(F a){unsigned char equal,ordered;F z{};asm volatile("ucomiss %3,%2;sete %0;setnp %1":"=qm"(equal),"=qm"(ordered):"x"(a),"x"(z):"cc","memory");return equal&&ordered;}
// The input is already on the native x87 stack as in11F8CF. Keep its real
// result there for the caller's FMUL/FSTP; no C++ floating-return ABI or
// emulated x87 register file transports the value.
void acos_active(Memory&m,NativeTu15LookVectorAngleStats*stats){
 std::uint64_t argument;asm volatile("fstl %0":"=m"(argument)::"memory");
 std::uint16_t caller;asm volatile("fnstcw %0":"=m"(caller));
 if(caller!=0x27fu){const std::uint16_t internal=std::uint16_t((caller&0x300u)|0x7fu);asm volatile("fldcw %0"::"m"(internal):"memory");if(stats)++stats->alternate_control_words;}
 const auto high=std::uint32_t(argument>>32u),magnitude=high&0x7ff00000u;
 if(magnitude<0x3ff00000u){asm volatile("fld1;fadd %%st(1);fld1;fsub %%st(2);fmulp;fsqrt;fxch %%st(1);fpatan":::"memory","st","st(1)","st(2)");}
 else if(magnitude==0x3ff00000u&&(argument&0xfffffffffffffull)==0){asm volatile("fstp %%st(0)":::"memory","st");if(high&0x80000000u)asm volatile("fldpi":::"memory","st");else asm volatile("fldz":::"memory","st");}
 else throw std::runtime_error("Native acos requires its genuine exceptional argument owner");
 const auto policy=m.word(0x48d4f4u);
 if(caller!=0x27fu){
  if(!policy){std::uint16_t status;asm volatile("fnstsw %0":"=a"(status));if((caller&0x20u)&&(status&0x20u)){if(stats)++stats->required_crt_policy_calls;throw std::runtime_error("Native acos requires genuine332047/3334F0 policy hook");}}
  asm volatile("fldcw %0"::"m"(caller):"memory");
 }
}
#endif
}
std::uint32_t native_tu15_look_vector_angle(SourcePhysicsContactMemory&memory,
 std::uint32_t first,std::uint32_t second,NativeTu15PhysicsFrameFp&fp,unsigned depth,
 NativeTu15LookVectorAngleStats*stats){
 if(stats)++stats->calls;
 try{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  if(depth>5)throw std::runtime_error("Native look angle requires its x87 capacity owner");
  Memory m(memory);Environment environment(fp,depth);const auto read=[&](std::uint32_t p){return scalar(m.word(p));};
  const auto by=read(second+4u),bz=read(second+8u),ax=read(first),ay=read(first+4u),az=read(first+8u),bx=read(second);
  const auto yy=mul(by,by),zz=mul(bz,bz),xx_a=mul(ax,ax),yy_a=mul(ay,ay),xx=mul(bx,bx);
  auto b_norm=add(xx,yy);const auto zz_a=mul(az,az);auto a_norm=add(xx_a,yy_a);b_norm=add(b_norm,zz);a_norm=add(a_norm,zz_a);const auto product=mul(b_norm,a_norm);
  if(zero(product)){if(stats){++stats->zero_products;++stats->completed;}return 0;}
  const auto bz_now=read(second+8u),az_now=read(first+8u);auto dot=mul(bz_now,az_now);
  const auto by_now=read(second+4u),ay_now=read(first+4u);dot=add(dot,mul(by_now,ay_now));
  const auto bx_now=read(second),ax_now=read(first);dot=add(dot,mul(bx_now,ax_now));
  auto cosine=divide(dot,product);const auto one=read(0x46a0d0u);cosine=mul(cosine,dot);cosine=mul(cosine,read(0x46a11cu));
  const auto negative_one=read(0x46a0dcu);cosine=sub(cosine,one);F argument;
  if(above(negative_one,cosine))argument=negative_one;
  else if(above(cosine,one))argument=one;
  else if(above(negative_one,cosine))argument=negative_one;
  else argument=above(cosine,one)?one:cosine;
  // Keep the source's repeated JBE/JA comparison phases. Unordered inputs
  // retain their NaN and therefore require the real exceptional acos owner.
  const auto input=bits(argument);asm volatile("flds %0"::"m"(input):"memory","st");if(stats)++stats->acos_calls;
  acos_active(m,stats);
  F z{};const bool negative_dot=above(z,dot);const auto half=m.word(0x46a0ccu);asm volatile("fmuls %0"::"m"(half):"memory","st");
  std::uint32_t result;asm volatile("fstps %0":"=m"(result)::"memory","st");
  if(negative_dot)result=bits(sub(read(0x556554u),scalar(result)));
  if(stats)++stats->completed;return result;
#else
  (void)memory;(void)first;(void)second;(void)fp;(void)depth;
  throw std::runtime_error("Exact native look vector angle backend unavailable");
#endif
 }catch(...){if(stats)++stats->refusals;throw;}
}
void native_tu15_look_acos_active(SourcePhysicsContactMemory&memory,
 NativeTu15LookVectorAngleStats*stats){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
 Memory m(memory);acos_active(m,stats);
#else
 (void)memory;(void)stats;throw std::runtime_error("Exact native active acos backend unavailable");
#endif
}
std::uint32_t native_tu15_camera_downward_angle(SourcePhysicsContactMemory&memory,
 std::uint32_t vertical,NativeTu15PhysicsFrameFp&fp,unsigned depth,
 NativeTu15LookVectorAngleStats*stats){
 if(stats)++stats->calls;
 try{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  const auto magnitude=vertical&0x7fffffffu;
  if(!(vertical&0x80000000u)||!magnitude||magnitude>=0x3f800000u||depth>5)
   throw std::runtime_error("Native downward camera angle requires its source-selected domain");
  Memory m(memory);Environment environment(fp,depth);
  asm volatile("flds %0;fchs"::"m"(vertical):"memory","st");
  if(stats)++stats->acos_calls;acos_active(m,stats);
  const auto half_pi=m.word(0x41c4ccu);std::uint32_t result;
  asm volatile("fsubrs %1;fstps %0":"=m"(result):"m"(half_pi):"memory","st");
  if(stats)++stats->completed;return result;
#else
  (void)memory;(void)vertical;(void)fp;(void)depth;
  throw std::runtime_error("Exact native downward camera angle backend unavailable");
#endif
 }catch(...){if(stats)++stats->refusals;throw;}
}
} // namespace halo2
