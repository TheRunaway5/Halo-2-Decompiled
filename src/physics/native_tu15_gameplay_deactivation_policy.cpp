#include "halo2/native_tu15_gameplay_deactivation_policy.hpp"
#include <bit>
#include <cstring>
#include <stdexcept>
#include <xmmintrin.h>
namespace halo2 {
namespace {
std::uint32_t word(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned width=4){
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(width));std::uint32_t n{};
    for(unsigned k=0;k<width;++k)n|=std::to_integer<unsigned>(b[k])<<(8u*k);return n;
}
void byte(SourcePhysicsContactMemory&m,std::uint32_t pc,std::uint32_t p,
    std::uint32_t value,const SourcePhysicsContactRecordObserver&o){
    const std::array b{std::byte(value)};SourcePhysicsContactRecordStore s{pc,p,{},{}};
    if(o){s.before.resize(1);m.read(p,s.before);s.written.assign(b.begin(),b.end());}
    m.write(p,b);if(o)o(s);
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
    alignas(16)std::array<std::byte,108>host_{};std::array<std::byte,28>native_{};
    NativeTu15PhysicsFrameFp&fp_;unsigned previous_;
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp),previous_(_mm_getcsr()){
        if(depth>5||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||
           (fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))
            throw std::runtime_error("Native TU1.5 gameplay sleep policy requires its unsupported FP owner");
        asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::memcpy(native_.data(),host_.data(),28);const std::uint16_t empty=0xffff;
        std::memcpy(native_.data(),&fp.control_word,2);std::memcpy(native_.data()+4,&fp.status,2);std::memcpy(native_.data()+8,&empty,2);
        asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp.mxcsr);
    }
    ~Environment(){
        asm volatile("fnstsw %0":"=a"(fp_.status));asm volatile("fnstcw %0":"=m"(fp_.control_word));fp_.mxcsr=_mm_getcsr();
        asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(previous_);
    }
};
bool nonzero(std::uint32_t bits){
    const auto value=_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(bits)));
    const auto zero=_mm_setzero_ps();unsigned char different,unordered;
    asm volatile("ucomiss %3,%2\n\tsetne %0\n\tsetp %1":"=qm"(different),"=qm"(unordered):"x"(value),"x"(zero):"cc","memory");
    return different||unordered;
}
std::uint32_t object(const SourcePhysicsContactMemory&m,std::uint32_t datum){
    const auto pool=word(m,0x4ecbd0u),rows=word(m,pool+0x44u);
    return word(m,rows+(datum&0xffffu)*12u+8u);
}
bool vehicle_active(const SourcePhysicsContactMemory&m,std::uint32_t datum){
    const auto unit=object(m,datum),definition=word(m,unit),tags=word(m,0x4f0414u);
    const auto tag=word(m,tags+(definition&0xffffu)*16u+8u);
    if((word(m,tag+0x1ecu,1)&1u)&&
       (nonzero(word(m,unit+0x354u))||word(m,unit+0x38bu,1)!=0xffu))return true;
    const auto flags=word(m,tag+0x1ecu);
    if((flags&2u)&&nonzero(word(m,unit+0x35cu)))return true;
    if((flags&4u)&&nonzero(word(m,unit+0x25cu)))return true;
    if((flags&8u)&&nonzero(word(m,unit+0x260u)))return true;
    return (flags&32u)&&nonzero(word(m,unit+0x358u));
}
bool biped_active(const SourcePhysicsContactMemory&m,std::uint32_t datum){
    const auto initial=object(m,datum);
    if(word(m,initial+0x3dcu,1)!=1u)return false;
    const auto unit=object(m,datum);
    if((word(m,unit+0xc1u,1)&1u)||!(word(m,unit+0x348u,1)&1u))return false;
    const auto globals=word(m,0x51df54u);
    const auto ticks=std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(word(m,globals+2u,2))));
    __m128 seconds=_mm_setzero_ps();asm volatile("cvtsi2ss %1,%0":"+x"(seconds):"r"(ticks):"memory");
    const auto scale_bits=word(m,0x46a458u);const auto scale=_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(scale_bits)));
    asm volatile("mulss %1,%0":"+x"(seconds):"x"(scale):"memory");
    std::uint32_t rounded_bits;std::memcpy(&rounded_bits,&seconds,4);std::int32_t threshold;
    asm volatile("flds %1\n\tfistpl %0":"=m"(threshold):"m"(rounded_bits):"memory","st");
    const auto state=std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(word(m,unit+0x399u,1))));
    if(state<threshold)return false;
    const auto mode=word(m,unit+0x3dcu,1);
    return (mode==1u||mode==3u)&&word(m,unit+0x14u)==0xffffffffu;
}
bool blocked(const SourcePhysicsContactMemory&m,std::uint32_t policy,NativeTu15GameplayDeactivationPolicyStats*s){
    const auto pool=word(m,0x4ecbd0u),rows=word(m,pool+0x44u),datum=word(m,policy+0x60u);
    const auto row=rows+(datum&0xffffu)*12u,unit=word(m,row+8u);
    const auto component=word(m,unit+0xb4u),components=word(m,0x52be5cu),component_rows=word(m,components+0x44u);
    const auto slot=word(m,policy+0x64u),instances=word(m,component_rows+(component&0xffffu)*160u+0x70u);
    const bool attached=word(m,instances+slot*96u+0x44u,1)==1u;
    const auto kind=word(m,row+3u,1);
    if(kind==1u){if(attached)return true;if(s)++s->vehicle_tests;return vehicle_active(m,datum);}
    if(kind!=0u)return attached;
    if(word(m,unit+0x34bu,1)==1u)return false;
    if(attached)return true;if(s)++s->biped_tests;return biped_active(m,datum);
}
#endif
}
bool native_tu15_gameplay_deactivation_policy(SourcePhysicsContactMemory&m,
    std::uint32_t policy,NativeTu15PhysicsBody body,NativeTu15DeactivationPeriod period,
    std::uint32_t output,std::uint32_t temporary,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const SourcePhysicsContactRecordObserver&o,NativeTu15GameplayDeactivationPolicyStats*s){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(s)++s->calls;bool active;
    {Environment environment(fp,depth);active=blocked(m,policy,s);}
    const bool long_period=period==NativeTu15DeactivationPeriod::long_period;
    if(active){if(s)++s->blocked;byte(m,long_period?0x1d9b8eu:0x1d9b3eu,output,0,o);return false;}
    if(s)++s->body_policies;
    NativeTu15BodyDeactivationPolicyMemory storage(m,policy,body,temporary,o);
    native_tu15_body_deactivation_policy(storage,period,fp,depth);
    const auto value=word(m,temporary,1);byte(m,long_period?0x1d9bacu:0x1d9b5cu,output,value,o);
    return value!=0;
#else
    (void)m;(void)policy;(void)body;(void)period;(void)output;(void)temporary;(void)fp;(void)depth;(void)o;(void)s;
    throw std::runtime_error("Native TU1.5 gameplay sleep policy backend unavailable");
#endif
}
} // namespace halo2
