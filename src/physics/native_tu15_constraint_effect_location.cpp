#include "halo2/native_tu15_constraint_effect_location.hpp"
#include <bit>
#include <stdexcept>
#include <xmmintrin.h>
#include <emmintrin.h>
namespace halo2 {
namespace {
class Environment {
    NativeTu15PhysicsFrameFp&fp_;unsigned previous_;
public:
    explicit Environment(NativeTu15PhysicsFrameFp&fp):fp_(fp),previous_(_mm_getcsr()){
        if((fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))
            throw std::runtime_error("Native TU1.5 effect location requires its SIMD fault owner");
        _mm_setcsr(fp.mxcsr);
    }
    ~Environment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(previous_);}
};
std::uint32_t read(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned width=4){
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(b[n])<<(8u*n);return value;
}
void store(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,std::uint32_t pc,std::uint32_t p,std::uint32_t value,unsigned width=4){
    std::array<std::byte,4>b{};for(unsigned n=0;n<width;++n)b[n]=std::byte(value>>(8u*n));SourcePhysicsContactRecordStore event{pc,p,{},{}};
    if(o){event.before.resize(width);m.read(p,event.before);event.written.assign(b.begin(),b.begin()+width);}
    m.write(p,std::span(b).first(width));if(o)o(event);
}
__m128 scalar(std::uint32_t value){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(value)));}
__m128 mul(__m128 a,__m128 b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 add(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 sub(__m128 a,__m128 b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool below_or_unordered(__m128 a,__m128 b){unsigned char value;asm volatile("comiss %2,%1; setb %0":"=qm"(value):"x"(a),"x"(b):"cc","memory");return value;}
bool greater(__m128 a,__m128 b){unsigned char value;asm volatile("comiss %2,%1; seta %0":"=qm"(value):"x"(a),"x"(b):"cc","memory");return value;}
std::uint32_t sign16(std::uint32_t v){return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(v))));}
}
std::uint32_t native_tu15_constraint_effect_point_leaf(const SourcePhysicsContactMemory&m,
    std::uint32_t tree,std::uint32_t node,std::uint32_t position,NativeTu15PhysicsFrameFp&fp,
    NativeTu15ConstraintEffectLocationStats*stats){
    if(stats)++stats->point_leaf_calls;Environment environment(fp);
    const auto nodes=read(m,tree+4u),planes=read(m,tree+0xcu);const auto zero=_mm_setzero_ps();
    while(true){
        if(stats)++stats->bsp_nodes;const auto row=nodes+(node&0x7fffffu)*8u,plane=planes+sign16(read(m,row,2))*16u;
        auto distance=scalar(read(m,plane+8u));distance=mul(distance,scalar(read(m,position+8u)));
        auto other=scalar(read(m,plane+4u));other=mul(other,scalar(read(m,position+4u)));distance=add(distance,other);
        other=scalar(read(m,plane));other=mul(other,scalar(read(m,position)));distance=add(distance,other);
        distance=sub(distance,scalar(read(m,plane+0xcu)));const auto side=below_or_unordered(distance,zero)?0u:1u;
        const auto packed=read(m,row+side*3u+1u);node=std::uint32_t(std::bit_cast<std::int32_t>(packed)>>8u);
        if(!(node&0x800000u))continue;if(node==0xffffffffu)return node;return node&0x7fffffu;
    }
}
void native_tu15_constraint_effect_location(SourcePhysicsContactMemory&m,std::uint32_t position,
    std::uint32_t output,NativeTu15PhysicsFrameFp&fp,const SourcePhysicsContactRecordObserver&observer,
    NativeTu15ConstraintEffectLocationStats*stats){
    if(stats)++stats->location_calls;const auto bsp=read(m,0x474c24u,2);
    if(bsp==0xffffu){store(m,observer,0x11e60eu,output+4u,bsp,2);store(m,observer,0x11e612u,output+6u,bsp,2);
        store(m,observer,0x11e616u,output,0xffffffffu);if(stats)++stats->no_active_bsp;return;}
    const auto tree=read(m,0x4ecc0cu);const auto leaf=native_tu15_constraint_effect_point_leaf(m,tree,0u,position,fp,stats);
    store(m,observer,0x11e62eu,output,leaf);
    if(leaf!=0xffffffffu){const auto structure=read(m,0x4ecc18u),leaves=read(m,structure+0x30u),cluster=read(m,leaves+leaf*8u,2);
        store(m,observer,0x11e63fu,output+6u,bsp,2);store(m,observer,0x11e643u,output+4u,cluster,2);}
    else{store(m,observer,0x11e64cu,output+6u,bsp,2);store(m,observer,0x11e650u,output+4u,0xffffu,2);if(stats)++stats->outside_bsp;}
}
bool native_tu15_constraint_effect_location_key(SourcePhysicsContactMemory&m,std::uint32_t location,
    std::uint32_t position,std::uint32_t optional_key,NativeTu15PhysicsFrameFp&fp,
    const SourcePhysicsContactRecordObserver&observer,NativeTu15ConstraintEffectLocationStats*stats){
    if(stats)++stats->key_calls;const auto cluster=read(m,location+4u,2);
    if(cluster==0xffffu){if(stats)++stats->missing_cluster;return false;}
    const auto structure=read(m,0x4ecc18u),clusters=read(m,structure+0xa0u),marker=read(m,clusters+sign16(cluster)*0xb0u+0x70u,1);
    if(marker==0xffu){if(stats)++stats->missing_key;return false;}
    const auto keys=read(m,structure+0x68u),entry=keys+(marker&0x7fu)*24u,key=read(m,entry+2u,2);
    if(key==0xffffu){if(stats)++stats->missing_key;return false;}
    if(marker&0x80u){
        if(stats)++stats->halfspace_tests;Environment environment(fp);
        auto distance=scalar(read(m,entry+0xcu));distance=mul(distance,scalar(read(m,position+8u)));
        auto other=scalar(read(m,entry+8u));other=mul(other,scalar(read(m,position+4u)));distance=add(distance,other);
        other=scalar(read(m,entry+4u));other=mul(other,scalar(read(m,position)));distance=add(distance,other);
        distance=sub(distance,scalar(read(m,entry+0x10u)));if(!greater(_mm_setzero_ps(),distance))return false;
    }
    if(optional_key)store(m,observer,0x11e8d5u,optional_key,key,2);if(stats)++stats->keys_selected;return true;
}
} // namespace halo2
