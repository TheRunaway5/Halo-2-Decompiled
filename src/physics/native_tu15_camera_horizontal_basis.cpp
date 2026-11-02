#include "halo2/native_tu15_camera_horizontal_basis.hpp"
#include <cstring>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory&m,std::uint32_t p){
    std::array<std::byte,4>b{};m.read(p,b);std::uint32_t n{};for(unsigned k=0;k<4;++k)n|=std::to_integer<unsigned>(b[k])<<(8u*k);return n;
}
void store(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,
    std::uint32_t pc,std::uint32_t p,std::uint32_t n){
    std::array<std::byte,4>b{};for(unsigned k=0;k<4;++k)b[k]=std::byte(n>>(8u*k));SourcePhysicsContactRecordStore e{pc,p,{},{}};
    if(o){e.before.resize(4);m.read(p,e.before);e.written.assign(b.begin(),b.end());}m.write(p,b);if(o)o(e);
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
    alignas(16)std::array<std::byte,108>host_{};NativeTu15PhysicsFrameFp&fp_;unsigned prior_;
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp),prior_(_mm_getcsr()){
        if(depth>4||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||(fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))throw std::runtime_error("Native horizontal camera basis requires its numerical-fault owner");
        asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::array<std::byte,28>env{};std::memcpy(env.data(),host_.data(),28);const std::uint16_t empty=0xffff;
        std::memcpy(env.data(),&fp.control_word,2);std::memcpy(env.data()+4,&fp.status,2);std::memcpy(env.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(env):"memory");_mm_setcsr(fp.mxcsr);
    }
    ~Environment(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));fp_.mxcsr=_mm_getcsr();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(prior_);}
};
using F=__m128;
F scalar(std::uint32_t n){F v{};std::memcpy(&v,&n,4);return v;}
std::uint32_t bits(F v){std::uint32_t n;std::memcpy(&n,&v,4);return n;}
F sub(F a,F b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
F mul(F a,F b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
F divide(F a,F b){asm volatile("divss %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool above(F a,F b){unsigned char n;asm volatile("comiss %2,%1;seta %0":"=qm"(n):"x"(a),"x"(b):"cc","memory");return n;}
bool ordered_zero(F a){F z{};unsigned char equal,ordered;asm volatile("ucomiss %3,%2;sete %0;setnp %1":"=qm"(equal),"=qm"(ordered):"x"(a),"x"(z):"cc","memory");return equal&&ordered;}
void load(std::uint32_t n){asm volatile("flds %0"::"m"(n):"memory","st");}
struct Length {std::uint32_t bits;bool too_small;};
Length finish_length(SourcePhysicsContactMemory&m){
    std::uint32_t value;asm volatile("fld %%st(0);fmul %%st(1),%%st;fld %%st(2);fmul %%st(3),%%st;faddp;fsqrt;fstp %%st(2);fstp %%st(0);fsts %0;fabs":"=m"(value)::"memory","st","st(1)","st(2)","st(3)");
    const auto threshold=read(m,0x46a0ecu);unsigned char small;
    asm volatile("flds %1;fcomip %%st(1),%%st;seta %0;fstp %%st(0)":"=qm"(small):"m"(threshold):"cc","memory","st","st(1)");return{value,small!=0};
}
#endif
}
void native_tu15_camera_horizontal_basis(SourcePhysicsContactMemory&m,std::uint32_t direction,
    std::uint32_t fallback,std::uint32_t first,std::uint32_t perpendicular,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,const SourcePhysicsContactRecordObserver&observe){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    Environment environment(fp,depth);store(m,observe,0xe05d3u,first,read(m,direction));store(m,observe,0xe05d8u,first+4u,read(m,direction+4u));
    load(read(m,first+4u));load(read(m,first));const auto one=scalar(read(m,0x46a0d0u)),zero=F{};
    auto length=finish_length(m);bool use_fallback=length.too_small;
    if(!use_fallback){
        const auto size=scalar(length.bits),x=scalar(read(m,first));const auto reciprocal=divide(one,size),normalized_x=mul(x,reciprocal);
        const bool is_zero=ordered_zero(size);store(m,observe,0xe0623u,first,bits(normalized_x));const auto y=scalar(read(m,first+4u));
        store(m,observe,0xe0634u,first+4u,bits(mul(y,reciprocal)));use_fallback=is_zero;
    }
    if(use_fallback){
        store(m,observe,0xe0641u,first,read(m,fallback));store(m,observe,0xe0646u,first+4u,read(m,fallback+4u));
        if(above(scalar(read(m,direction+8u)),zero)){
            store(m,observe,0xe065au,first,bits(sub(zero,scalar(read(m,first)))));
            store(m,observe,0xe0666u,first+4u,bits(sub(zero,scalar(read(m,first+4u)))));
        }
        load(read(m,first+4u));load(read(m,first));length=finish_length(m);bool default_basis=length.too_small;
        if(!default_basis){
            const auto size=scalar(length.bits),x=scalar(read(m,first));const auto reciprocal=divide(one,size),normalized_x=mul(x,reciprocal);
            const bool is_zero=ordered_zero(size);store(m,observe,0xe06a5u,first,bits(normalized_x));const auto y=scalar(read(m,first+4u));
            store(m,observe,0xe06b6u,first+4u,bits(mul(y,reciprocal)));default_basis=is_zero;
        }
        if(default_basis){const auto basis=read(m,0x474cd8u);store(m,observe,0xe06c4u,first,read(m,basis));store(m,observe,0xe06c9u,first+4u,read(m,basis+4u));}
    }
    store(m,observe,0xe06d1u,perpendicular,bits(sub(zero,scalar(read(m,first+4u)))));
    store(m,observe,0xe06d7u,perpendicular+4u,read(m,first));
#else
    (void)m;(void)direction;(void)fallback;(void)first;(void)perpendicular;(void)fp;(void)depth;(void)observe;
    throw std::runtime_error("Exact native horizontal camera basis backend unavailable");
#endif
}
}
