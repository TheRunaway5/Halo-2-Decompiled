#include "halo2/native_tu15_look_direction.hpp"
#include <bit>
#include <cstring>
#include <stdexcept>
#include <xmmintrin.h>
#include <emmintrin.h>
namespace halo2 {
namespace {
class Environment {
    alignas(16)std::array<std::byte,108> host_{};
    std::array<std::byte,28> environment_{};
    NativeTu15PhysicsFrameFp&fp_;unsigned prior_;
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp),prior_(_mm_getcsr()){
        if(depth>4||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||
           (fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))
            throw std::runtime_error("Native TU1.5 look direction requires its numerical-fault owner");
        asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::memcpy(environment_.data(),host_.data(),28);
        const std::uint16_t empty=0xffffu;
        std::memcpy(environment_.data(),&fp.control_word,2);
        std::memcpy(environment_.data()+4,&fp.status,2);
        std::memcpy(environment_.data()+8,&empty,2);
        asm volatile("fldenv %0"::"m"(environment_):"memory");_mm_setcsr(fp.mxcsr);
    }
    ~Environment(){
        asm volatile("fnstcw %0":"=m"(fp_.control_word));
        asm volatile("fnstsw %0":"=a"(fp_.status));fp_.mxcsr=_mm_getcsr();
        asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        _mm_setcsr(prior_);
    }
};
std::uint32_t word(const SourcePhysicsContactMemory&m,std::uint32_t p){
    std::array<std::byte,4>b{};m.read(p,b);std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(b[n])<<(8u*n);return value;
}
void publish(SourcePhysicsContactMemory&m,std::uint32_t site,std::uint32_t p,
    std::uint32_t value,const SourcePhysicsContactRecordObserver&o){
    std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(value>>(n*8u));
    SourcePhysicsContactRecordStore event{site,p,{},{}};
    if(o){event.before.resize(4);m.read(p,event.before);event.written.assign(b.begin(),b.end());}
    m.write(p,b);if(o)o(event);
}
void load_direction_component(std::uint32_t bits){asm volatile("flds %0"::"m"(bits):"memory","st");}
std::uint32_t publish_angle(){std::uint32_t out;asm volatile("fstps %0":"=m"(out)::"memory","st");return out;}
__m128 scalar(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(bits)));}
bool negative_yaw(std::uint32_t bits){
    unsigned char negative;const auto yaw=scalar(bits),zero=_mm_setzero_ps();
    asm volatile("comiss %2,%1; seta %0":"=qm"(negative):"x"(zero),"x"(yaw):"cc","memory");return negative;
}
std::uint32_t add_turn(std::uint32_t yaw,std::uint32_t turn){
    auto value=scalar(yaw);const auto period=scalar(turn);
    asm volatile("addss %1,%0":"+x"(value):"x"(period):"memory");
    return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(value)));
}
}
void native_tu15_look_direction(SourcePhysicsContactMemory&m,std::uint32_t direction,
    std::uint32_t index,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const SourcePhysicsContactRecordObserver&observe){
    Environment environment(fp,depth);
    load_direction_component(word(m,direction+4u));
    const auto inputs=word(m,0x4f9b54u);
    load_direction_component(word(m,direction));
    asm volatile("fpatan":::"memory","st","st(1)");
    const auto input=inputs+index*0x94u+0x14u;
    publish(m,0x18a2bau,input+0x10u,publish_angle(),observe);
    load_direction_component(word(m,direction+4u));
    load_direction_component(word(m,direction));
    load_direction_component(word(m,direction+8u));
    // atan2(z, sqrt(y*y + x*x)); retain the source association and native
    // x87 precision points, including the discarded live X temporary.
    asm volatile("fld %%st(2); fmulp %%st,%%st(3); fld %%st(1); fmul %%st(2),%%st; "
        "faddp %%st,%%st(3); fxch %%st(2); fsqrt; fxch %%st(2); "
        "fxch %%st(1); fxch %%st(2); fpatan; fstp %%st(1)"
        :::"memory","st","st(1)","st(2)","st(3)");
    publish(m,0x18a2ddu,input+0x14u,publish_angle(),observe);
    if(negative_yaw(word(m,input+0x10u))){
        const auto yaw=word(m,input+0x10u),turn=word(m,0x44923cu);
        publish(m,0x18a2f3u,input+0x10u,add_turn(yaw,turn),observe);
    }
}
}
