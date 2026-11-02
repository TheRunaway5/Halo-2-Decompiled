#include "halo2/native_tu15_camera_basis.hpp"
#include <cstring>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned width=4){
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(b[n])<<(8u*n);return value;
}
void store(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,
    std::uint32_t instruction,std::uint32_t p,std::uint32_t value){
    std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(value>>(8u*n));
    SourcePhysicsContactRecordStore event{instruction,p,{},{}};
    if(o){event.before.resize(4);m.read(p,event.before);event.written.assign(b.begin(),b.end());}
    m.write(p,b);if(o)o(event);
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
    alignas(16)std::array<std::byte,108>host_{};NativeTu15PhysicsFrameFp&fp_;unsigned previous_;
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp),previous_(_mm_getcsr()){
        if(depth>6||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||
            (fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))
            throw std::runtime_error("Native camera basis requires its unsupported FP exception owner");
        asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::array<std::byte,28>env{};std::memcpy(env.data(),host_.data(),28);const std::uint16_t empty=0xffffu;
        std::memcpy(env.data(),&fp.control_word,2);std::memcpy(env.data()+4,&fp.status,2);std::memcpy(env.data()+8,&empty,2);
        asm volatile("fldenv %0"::"m"(env):"memory");_mm_setcsr(fp.mxcsr);
    }
    ~Environment(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));
        fp_.mxcsr=_mm_getcsr();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(previous_);}
};
using F=__m128;
F scalar(std::uint32_t value){F result{};std::memcpy(&result,&value,4);return result;}
std::uint32_t bits(F value){std::uint32_t result;std::memcpy(&result,&value,4);return result;}
F mul(F a,F b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
F sub(F a,F b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
F divide(F a,F b){asm volatile("divss %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool ordered_zero(F value){F zero{};unsigned char equal,ordered;
    asm volatile("ucomiss %3,%2;sete %0;setnp %1":"=qm"(equal),"=qm"(ordered):"x"(value),"x"(zero):"cc","memory");return equal&&ordered;
}
#endif
}
std::uint32_t native_tu15_unit_camera_transition(const SourcePhysicsContactMemory&m,std::uint32_t object){
    const auto objects=read(m,0x4ecbd0u),headers=read(m,objects+0x44u),index=object&0xffffu;
    const auto data=read(m,headers+index*12u+8u);const auto mode=read(m,data+0x3dcu,1);
    bool transition=false;
    if(mode==6u&&read(m,data+0x3f8u,1))transition=((read(m,data+0x348u,2)>>13u)&1u)!=0;
    return ((index*3u)&0xffffff00u)|std::uint32_t(transition);
}
std::uint32_t native_tu15_camera_perpendicular(SourcePhysicsContactMemory&m,std::uint32_t direction,
    std::uint32_t destination,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15CameraBasisStats*stats){
    if(stats)++stats->calls;
    try{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        Environment environment(fp,depth);
        const auto x=read(m,direction);asm volatile("flds %0"::"m"(x):"memory","st");
        const auto y=read(m,direction+4u);std::uint32_t negative_x,length;
        // FST rounds each source DATA word while retaining the extended value
        // for its following operation, just as the original x87 owner does.
        asm volatile("fchs;fsts %0;fmuls %0;flds %2;fmuls %2;faddp;fsqrt;fsts %1;fabs"
            :"=m"(negative_x),"=m"(length):"m"(y):"memory","st","st(1)");
        const auto threshold=read(m,0x46a0ecu);unsigned char fallback;
        asm volatile("flds %1;fcomip %%st(1),%%st;seta %0;fstp %%st(0)"
            :"=qm"(fallback):"m"(threshold):"cc","memory","st","st(1)");
        F first{},second{},third{};
        if(!fallback){
            const auto normalized_length=scalar(length);const auto one=scalar(read(m,0x46a0d0u));
            const bool zero=ordered_zero(normalized_length);auto reciprocal=divide(one,normalized_length);
            first=mul(reciprocal,scalar(y));second=mul(reciprocal,scalar(negative_x));third=mul(reciprocal,F{});
            fallback=zero;
        }
        if(fallback){
            const auto basis=read(m,0x474d08u),a=read(m,basis),b=read(m,basis+4u),c=read(m,basis+8u);
            first=scalar(a);second=scalar(b);third=scalar(c);if(stats)++stats->fallbacks;
        }
        const auto current_x=scalar(read(m,direction)),z=scalar(read(m,direction+8u));
        const auto current_y=scalar(read(m,direction+4u));auto output_z=sub(mul(first,current_y),mul(current_x,second));
        const auto x_again=scalar(read(m,direction));const auto z_first=mul(z,first),z_again=scalar(read(m,direction+8u));
        const auto x_third=mul(x_again,third),y_again=scalar(read(m,direction+4u));
        const auto y_third=mul(third,y_again),z_second=mul(z_again,second);
        const auto output_y=sub(x_third,z_first),output_x=sub(z_second,y_third);
        store(m,observe,0x11fb1eu,destination,bits(output_x));store(m,observe,0x11fb22u,destination+4u,bits(output_y));
        store(m,observe,0x11fb27u,destination+8u,bits(output_z));if(stats)++stats->completed;return destination;
#else
        (void)m;(void)direction;(void)destination;(void)fp;(void)depth;(void)observe;
        throw std::runtime_error("Exact native camera basis backend unavailable");
#endif
    }catch(...){if(stats)++stats->refusals;throw;}
}
}
