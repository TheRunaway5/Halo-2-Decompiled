#include "halo2/native_tu15_vector_checks.hpp"
#include <cstring>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
std::uint32_t word(const SourcePhysicsContactMemory&m,std::uint32_t p){
    std::array<std::byte,4>b{};m.read(p,b);std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(b[n])<<(n*8u);return value;
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
    alignas(16)std::array<std::byte,108>host_{};NativeTu15PhysicsFrameFp&fp_;unsigned prior_;
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp),prior_(_mm_getcsr()){
        if(depth>6||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||
            (fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))
            throw std::runtime_error("Native vector checks require their numerical-fault owner");
        asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::array<std::byte,28>env{};std::memcpy(env.data(),host_.data(),28);const std::uint16_t empty=0xffff;
        std::memcpy(env.data(),&fp.control_word,2);std::memcpy(env.data()+4,&fp.status,2);std::memcpy(env.data()+8,&empty,2);
        asm volatile("fldenv %0"::"m"(env):"memory");_mm_setcsr(fp.mxcsr);
    }
    ~Environment(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));fp_.mxcsr=_mm_getcsr();
        asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(prior_);}
};
using F=__m128;
F scalar(std::uint32_t value){F out{};std::memcpy(&out,&value,4);return out;}
std::uint32_t bits(F value){std::uint32_t out;std::memcpy(&out,&value,4);return out;}
F mul(F a,F b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
F add(F a,F b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
F sub(F a,F b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool within_tolerance(SourcePhysicsContactMemory&m,F difference){
    const auto value=bits(difference);if((value&0x7f800000u)==0x7f800000u)return false;
    asm volatile("flds %0;fabs"::"m"(value):"memory","st");
    const auto tolerance=word(m,0x46a180u);unsigned char result;
    asm volatile("flds %1;fcomip %%st(1),%%st;seta %0;fstp %%st(0)"
        :"=qm"(result):"m"(tolerance):"cc","memory","st","st(1)");return result;
}
bool unit_active(SourcePhysicsContactMemory&m,std::uint32_t vector){
    const auto y=scalar(word(m,vector+4u)),z=scalar(word(m,vector+8u)),x=scalar(word(m,vector));
    const auto yy=mul(y,y),xx=mul(x,x);auto norm=add(xx,yy);const auto zz=mul(z,z);norm=add(norm,zz);
    return within_tolerance(m,sub(norm,scalar(word(m,0x46a0d0u))));
}
#endif
}
bool native_tu15_vector_near_unit(SourcePhysicsContactMemory&m,std::uint32_t vector,
    NativeTu15PhysicsFrameFp&fp,unsigned depth){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    Environment environment(fp,depth);return unit_active(m,vector);
#else
    (void)m;(void)vector;(void)fp;(void)depth;throw std::runtime_error("Exact native vector check backend unavailable");
#endif
}
bool native_tu15_scalar_near(SourcePhysicsContactMemory&m,std::uint32_t first,
    std::uint32_t second,NativeTu15PhysicsFrameFp&fp,unsigned depth){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    Environment environment(fp,depth);return within_tolerance(m,sub(scalar(first),scalar(second)));
#else
    (void)m;(void)first;(void)second;(void)fp;(void)depth;throw std::runtime_error("Exact native scalar check backend unavailable");
#endif
}
bool native_tu15_vectors_orthonormal(SourcePhysicsContactMemory&m,std::uint32_t first,
    std::uint32_t second,NativeTu15PhysicsFrameFp&fp,unsigned depth){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    Environment environment(fp,depth);if(!unit_active(m,first)||!unit_active(m,second))return false;
    const auto second_z=scalar(word(m,second+8u)),first_z=scalar(word(m,first+8u));auto dot=mul(second_z,first_z);
    const auto second_y=scalar(word(m,second+4u)),first_y=scalar(word(m,first+4u));dot=add(dot,mul(second_y,first_y));
    const auto first_x=scalar(word(m,first)),second_x=scalar(word(m,second));dot=add(dot,mul(first_x,second_x));
    return within_tolerance(m,sub(dot,F{}));
#else
    (void)m;(void)first;(void)second;(void)fp;(void)depth;throw std::runtime_error("Exact native orthogonality backend unavailable");
#endif
}
}
