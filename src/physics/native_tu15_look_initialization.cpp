#include "halo2/native_tu15_look_initialization.hpp"
#include <cstring>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
std::uint32_t word(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned width=4){
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(b[n])<<(8u*n);return value;
}
void publish(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,
    std::uint32_t instruction,std::uint32_t p,std::uint32_t value,unsigned width=4){
    std::array<std::byte,4>b{};for(unsigned n=0;n<width;++n)b[n]=std::byte(value>>(8u*n));
    SourcePhysicsContactRecordStore event{instruction,p,{},{}};
    if(o){event.before.resize(width);m.read(p,event.before);event.written.assign(b.begin(),b.begin()+width);}
    m.write(p,std::span(b).first(width));if(o)o(event);
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
    alignas(16)std::array<std::byte,108>host_{};NativeTu15PhysicsFrameFp&fp_;unsigned previous_;
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp),previous_(_mm_getcsr()){
        if(depth>4||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||
            (fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))
            throw std::runtime_error("Native look initialization requires its numerical-fault owner");
        asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::array<std::byte,28>env{};std::memcpy(env.data(),host_.data(),28);const std::uint16_t empty=0xffffu;
        std::memcpy(env.data(),&fp.control_word,2);std::memcpy(env.data()+4,&fp.status,2);std::memcpy(env.data()+8,&empty,2);
        asm volatile("fldenv %0"::"m"(env):"memory");_mm_setcsr(fp.mxcsr);
    }
    ~Environment(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));
        fp_.mxcsr=_mm_getcsr();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(previous_);}
};
void load(std::uint32_t value){asm volatile("flds %0"::"m"(value):"memory","st");}
void publish_angle(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,
    std::uint32_t instruction,std::uint32_t p){
    // A failed DATA write must leave the native x87 value/status at the
    // preceding phase. This snapshot is generated from this kernel's live
    // arithmetic, never from a captured source control frame or result.
    alignas(16)std::array<std::byte,108>pending{};
    asm volatile("fnsave %0;frstor %0":"=m"(pending)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    std::uint32_t value;asm volatile("fstps %0":"=m"(value)::"memory","st");
    std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(value>>(8u*n));
    SourcePhysicsContactRecordStore event{instruction,p,{},{}};
    if(o){event.before.resize(4);m.read(p,event.before);event.written.assign(b.begin(),b.end());}
    try{m.write(p,b);}catch(...){asm volatile("frstor %0"::"m"(pending):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");throw;}
    if(o)o(event);
}
__m128 scalar(std::uint32_t value){__m128 v{};std::memcpy(&v,&value,4);return v;}
bool negative(std::uint32_t value){const auto zero=_mm_setzero_ps(),v=scalar(value);unsigned char result;
    asm volatile("comiss %2,%1;seta %0":"=qm"(result):"x"(zero),"x"(v):"cc","memory");return result;
}
std::uint32_t add(std::uint32_t first,std::uint32_t second){auto a=scalar(first);const auto b=scalar(second);
    asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");std::uint32_t value;std::memcpy(&value,&a,4);return value;
}
#endif
}
void native_tu15_initialize_look_controller(SourcePhysicsContactMemory&m,std::uint32_t index,
    std::uint32_t unit,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15LookInitializationStats*stats){
    if(stats)++stats->calls;
    try{
        const auto done=[&]{if(stats)++stats->completed;};
        if(index==0xffffffffu){if(stats)++stats->unchanged;done();return;}
        const auto inputs=word(m,0x4f9b54u),record=inputs+index*0x94u+0x14u;
        if(word(m,inputs,1)&&word(m,record)==unit){if(stats)++stats->unchanged;done();return;}
        const auto first_default=word(m,0x46a58cu);
        for(unsigned n=0;n<0x25u;++n)publish(m,observe,0x18a0feu,record+n*4u,0);
        if(stats)++stats->cleared_records;
        publish(m,observe,0x18a100u,record+0x84u,first_default);
        const auto second_default=word(m,0x46a588u);
        publish(m,observe,0x18a110u,record+0x89u,0,1);publish(m,observe,0x18a116u,record+0x88u,0,1);
        publish(m,observe,0x18a11cu,record+0x30u,0,1);publish(m,observe,0x18a11fu,record+0x80u,second_default);
        publish(m,observe,0x18a12au,record+0xcu,0,2);publish(m,observe,0x18a12eu,record+0xeu,0,2);
        publish(m,observe,0x18a132u,record,unit);publish(m,observe,0x18a134u,record+0x28u,0xffffu,2);
        publish(m,observe,0x18a138u,record+0x2au,0xffu,1);publish(m,observe,0x18a13bu,record+0x2bu,0xffu,1);
        publish(m,observe,0x18a13eu,record+0x2cu,0xffffu,2);publish(m,observe,0x18a142u,record+0x2eu,0xffffu,2);
        publish(m,observe,0x18a146u,record+0x5cu,0,2);publish(m,observe,0x18a14au,record+0x60u,0);
        publish(m,observe,0x18a14fu,record+0x64u,0);publish(m,observe,0x18a154u,record+0x44u,0xffffffffu);
        publish(m,observe,0x18a157u,record+0x48u,0xffffffffu);publish(m,observe,0x18a15au,record+0x4cu,0xffffffffu);
        publish(m,observe,0x18a166u,record+0x6cu,0);publish(m,observe,0x18a168u,record+0x70u,0);
        publish(m,observe,0x18a16bu,record+0x74u,0);publish(m,observe,0x18a16eu,record+0x6cu,0);
        publish(m,observe,0x18a170u,record+0x70u,0);publish(m,observe,0x18a173u,record+0x70u,0xffffffffu);
        publish(m,observe,0x18a176u,record+0x74u,0xffffffffu);
        if(unit==0xffffffffu){done();return;}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        const auto objects=word(m,0x4ecbd0u),headers=word(m,objects+0x44u),body=word(m,headers+(unit&0xffffu)*12u+8u);
        Environment environment(fp,depth);load(word(m,body+0x154u));load(word(m,body+0x150u));
        asm volatile("fpatan":::"memory","st","st(1)");publish_angle(m,observe,0x18a1a4u,record+0x10u);
        load(word(m,body+0x154u));load(word(m,body+0x150u));load(word(m,body+0x158u));
        asm volatile("fld %%st(2);fmulp %%st,%%st(3);fld %%st(1);fmul %%st(2),%%st;faddp %%st,%%st(3);"
            "fxch %%st(2);fsqrt;fxch %%st(2);fxch %%st(1);fxch %%st(2);fpatan;fstp %%st(1)"
            :::"memory","st","st(1)","st(2)","st(3)");
        publish_angle(m,observe,0x18a1d1u,record+0x14u);
        if(negative(word(m,record+0x10u))){const auto yaw=word(m,record+0x10u),turn=word(m,0x44923cu);publish(m,observe,0x18a1e7u,record+0x10u,add(yaw,turn));}
        publish(m,observe,0x18a1f2u,record+0x28u,word(m,body+0x214u));
        const auto first=std::uint32_t(std::int32_t(std::int8_t(word(m,body+0x23du,1))));
        publish(m,observe,0x18a1fdu,record+0x2cu,first,2);
        const auto second=std::uint32_t(std::int32_t(std::int8_t(word(m,body+0x241u,1))));
        publish(m,observe,0x18a209u,record+0x2eu,second,2);if(stats)++stats->unit_directions;done();
#else
        (void)fp;(void)depth;throw std::runtime_error("Exact native look initialization backend unavailable");
#endif
    }catch(...){if(stats)++stats->refusals;throw;}
}
}
