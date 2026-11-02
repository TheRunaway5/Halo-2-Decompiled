#include "halo2/native_tu15_breakable_contact_damage.hpp"
#include <bit>
#include <stdexcept>
#include <xmmintrin.h>
#include <emmintrin.h>
namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned size=4) {
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(size));std::uint32_t value{};
    for(unsigned n=0;n<size;++n)value|=std::to_integer<unsigned>(b[n])<<(n*8u);return value;
}
void store(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,
    std::uint32_t pc,std::uint32_t p,std::uint32_t value,unsigned size=4) {
    std::array<std::byte,4>b{};for(unsigned n=0;n<size;++n)b[n]=std::byte(value>>(n*8u));
    SourcePhysicsContactRecordStore event{pc,p,{},{}};
    if(o){event.before.resize(size);m.read(p,event.before);event.written.assign(b.begin(),b.begin()+size);}
    m.write(p,std::span(b).first(size));if(o)o(event);
}
__m128 scalar(std::uint32_t value) {return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(value)));}
bool above_threshold(std::uint32_t impulse,std::uint32_t threshold,NativeTu15PhysicsFrameFp&fp,unsigned depth) {
    if(depth>3 || (fp.control_word&0x3fu)!=0x3fu || (fp.status&0x8080u) ||
        (fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))
        throw std::runtime_error("Native TU1.5 breakable contact damage requires its unsupported numerical-fault owner");
    const auto host=_mm_getcsr();_mm_setcsr(fp.mxcsr);unsigned char above;
    // COMISS, rather than a C++ comparison, retains unordered and denormal
    // exception flags in the source MXCSR, including DAZ behavior.
    asm volatile("comiss %2,%1; seta %0":"=qm"(above):"x"(scalar(impulse)),"x"(scalar(threshold)):"cc","memory");
    fp.mxcsr=_mm_getcsr();_mm_setcsr(host);return above;
}
}
void native_tu15_initialize_damage_data(SourcePhysicsContactMemory&m,NativeTu15DamageData data,
    std::uint32_t definition,const SourcePhysicsContactRecordObserver&o) {
    const auto p=data.identity;
    for(unsigned n=0;n<34;++n)store(m,o,0xd8bc2u,p+n*4u,0);
    store(m,o,0xd8bc8u,p,definition);
    store(m,o,0xd8bd1u,p+0x7cu,read(m,0x48a0b0u,2),2);
    store(m,o,0xd8bdfu,p+0xcu,0xffffffffu);store(m,o,0xd8be2u,p+8u,0xffffffffu);
    store(m,o,0xd8be5u,p+0x10u,0xffffu,2);store(m,o,0xd8be9u,p+0x14u,0xffffffffu);
    store(m,o,0xd8becu,p+0x1cu,0xffffffffu);store(m,o,0xd8befu,p+0x20u,0xffffu,2);
    store(m,o,0xd8bf3u,p+0x22u,read(m,0x474c24u,2),2);store(m,o,0xd8bf7u,p+0x18u,0xffffffffu);
    const auto one=read(m,0x46a0d0u);
    store(m,o,0xd8bfau,p+0x54u,one);store(m,o,0xd8bffu,p+0x58u,one);store(m,o,0xd8c04u,p+0x5cu,one);
    store(m,o,0xd8c09u,p+0x7eu,0xffffu,2);store(m,o,0xd8c0du,p+0x84u,0,1);
}
NativeTu15BreakableContactDamageResult native_tu15_build_breakable_contact_damage(
    SourcePhysicsContactMemory&m,std::uint32_t object_datum,std::uint32_t contact_index,
    NativeTu15CollisionDamageEntry entry,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const SourcePhysicsContactRecordObserver&o,NativeTu15BreakableContactDamageStats*stats) {
    if(stats)++stats->calls;
    const auto object=read(m,read(m,0x4ecbd0u)+0x44u)+(object_datum&0xffffu)*12u;
    const auto object_data=read(m,object+8u);
    const auto component=read(m,read(m,0x52be5cu)+0x44u)+(read(m,object_data+0xb4u)&0xffffu)*0xa0u;
    const auto contacts=read(m,component+0x88u),index9=contact_index*9u,row=contacts+index9*8u;
    const auto impulse=read(m,row+0x38u);
    NativeTu15BreakableContactDamageResult result;
    result.eax=component&0xffffff00u;result.ecx=index9;result.edx=contacts;result.xmm0[0]=impulse;
    if(!above_threshold(impulse,read(m,0x46a240u),fp,depth)) {
        if(stats)++stats->below_threshold;return result;
    }
    const auto key=read(m,row+4u),low=key&0xffffu,part=(key>>16)&0x1fffu;
    std::uint32_t surface,instance,breakable;
    if((key>>29)==1u) {
        surface=low;instance=0xffffffffu;breakable=read(m,read(m,read(m,0x4ecc10u)+0x2cu)+low*8u+5u,1);
        if(stats)++stats->structure_keys;
    } else {
        const auto bsp=read(m,0x4ecc18u);
        const auto definition=std::uint32_t(std::int32_t(std::int16_t(read(m,read(m,bsp+0x144u)+low*0x58u+0x34u,2))));
        breakable=read(m,read(m,read(m,bsp+0x13cu)+definition*0xc8u+0x9cu)+part*8u+5u,1);
        surface=part;instance=low;
        if(stats){++stats->instance_keys;if((key>>29)!=5u)++stats->other_kind_keys;}
    }
    const auto definition=read(m,read(m,read(m,0x4ecc1cu)+0xbcu)+4u),p=entry.identity;
    native_tu15_initialize_damage_data(m,{p},definition,o);
    constexpr std::array<std::uint32_t,3> position_pcs{0x1e28ecu,0x1e28f1u,0x1e28f7u};
    constexpr std::array<std::uint32_t,3> origin_pcs{0x1e28ffu,0x1e2904u,0x1e290au};
    constexpr std::array<std::uint32_t,3> normal_pcs{0x1e2917u,0x1e291cu,0x1e2922u};
    for(unsigned n=0;n<3;++n)store(m,o,position_pcs[n],p+0x30u+n*4u,read(m,row+0x1cu+n*4u));
    for(unsigned n=0;n<3;++n)store(m,o,origin_pcs[n],p+0x24u+n*4u,read(m,row+0x1cu+n*4u));
    for(unsigned n=0;n<3;++n)store(m,o,normal_pcs[n],p+0x3cu+n*4u,read(m,row+0x28u+n*4u));
    const auto length=native_tu15_normalize_vector(m,p+0x3cu,read(m,0x474d08u),fp,depth,o,
        stats?&stats->normalize:nullptr,&result.normalized);
    // The return can also be zero on the normalization path (for example
    // with a zero/NaN epsilon bank). Preserve the actual branch's XMM live-outs.
    result.xmm0={length,0,0,0};
    if(result.normalized){result.xmm1[0]=read(m,p+0x44u);result.xmm2[0]=read(m,p+0x40u);}
    store(m,o,0x1e2934u,p+0x84u,2,1);store(m,o,0x1e293fu,p+0x7cu,read(m,row+0x12u,2),2);
    store(m,o,0x1e2946u,p+0x1cu,read(m,object_data+0x28u));store(m,o,0x1e2950u,p+0x20u,read(m,object_data+0x2cu));
    store(m,o,0x1e2953u,p+4u,read(m,p+4u)|0x100u);store(m,o,0x1e295eu,p+0x90u,surface);
    store(m,o,0x1e2964u,p+0x88u,instance);store(m,o,0x1e296au,p+0x8cu,breakable);
    result.built=true;result.eax=(surface&0xffffff00u)|1u;result.ecx=instance;result.edx=breakable;return result;
}
}
