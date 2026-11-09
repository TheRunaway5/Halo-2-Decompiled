#include "halo2/native_tu15_command_emit.hpp"
#include "halo2/detail/native_tu15_arithmetic_environment.hpp"
#include "halo2/native_tu15_shader_lod.hpp"
#include <algorithm>
#include <bit>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <emmintrin.h>
#endif

namespace halo2 {
void NativeTu15CommandEmitStores::append(NativeTu15CommandEmitStore store) {
    if(size_==rows_.size())throw std::logic_error("Native41030 exceeded its source store count");
    rows_[size_++]=store;
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace {
constexpr std::uint32_t tags_address=0x4f0414u,none=0xffffffffu;
std::int32_t signed_word(std::uint32_t v){return std::bit_cast<std::int32_t>(v);}
std::uint32_t signed_half(std::uint32_t v){return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(v))));}

// Reads after a staged write see exactly the bytes the original would see.
// Check destination extents before publishing any of this entry's stores.
class EmitMemory final:public SourcePhysicsContactMemory {
    const SourcePhysicsContactMemory& base_;
    NativeTu15CommandEmitStores& staged_;
    std::uint64_t lowest_=0x100000000ull,highest_{};
public:
    EmitMemory(const SourcePhysicsContactMemory& base,NativeTu15CommandEmitStores& staged):base_(base),staged_(staged){}
    void read(std::uint32_t at,std::span<std::byte> out)const override {
        base_.read(at,out);
        const auto end=std::uint64_t(at)+out.size();
        if(at>=highest_||end<=lowest_)return;
        for(const auto& s:staged_.rows()) {
            const auto lo=std::max<std::uint64_t>(s.address,at),hi=std::min(std::uint64_t(s.address)+s.width,end);
            for(auto i=lo;i<hi;++i)out[i-at]=std::byte(s.value>>(8*(i-s.address)));
        }
    }
    void write(std::uint32_t,std::span<const std::byte>)override {
        throw std::logic_error("Native41030 stores require a source instruction address");
    }
    std::uint32_t word(std::uint32_t at,unsigned width=4)const {
        std::array<std::byte,4> raw{};read(at,std::span(raw).first(width));std::uint32_t value{};
        for(unsigned i=0;i<width;++i)value|=std::to_integer<unsigned>(raw[i])<<(8*i);
        return value;
    }
    void put(std::uint32_t pc,std::uint32_t at,std::uint32_t value,unsigned width=4) {
        std::array<std::byte,4> raw{};
        if(std::uint64_t(at)+width>0x100000000ull)throw std::out_of_range("Native41030 store wraps address space");
        base_.read(at,std::span(raw).first(width));staged_.append({pc,at,value,width});
        lowest_=std::min<std::uint64_t>(lowest_,at);highest_=std::max(highest_,std::uint64_t(at)+width);
    }
};

std::uint32_t tag_definition(const EmitMemory& m,std::uint32_t tags,std::uint32_t datum) {
    return m.word(tags+16u*(datum&0xffffu)+8u);
}
std::uint32_t lod_block(const EmitMemory& m,std::uint32_t shader) {
    const auto tags=m.word(tags_address);
    const auto group=m.word(tags+16u*signed_half(shader));
    if(group==0x5052544du||group==0x70727433u)
        throw std::invalid_argument("Native41030 dynamic shader requires its original service owner");
    const auto definition=tag_definition(m,tags,shader);
    return m.word(tag_definition(m,tags,m.word(m.word(definition+0x24u)))+0x5cu);
}
std::uint32_t layer(const EmitMemory& m,std::uint32_t block,std::uint32_t index) {
    // The original can read array-8 with a zero count. Until the underflow
    // storage has a verified owner, leave it to the original service.
    if(signed_word(m.word(block))<=0)throw std::invalid_argument("Native41030 shader has no admitted LOD rows");
    return m.word(m.word(block+4u)+10u*index+2u);
}
std::uint32_t part_row(const EmitMemory& m,std::uint32_t unit,std::uint32_t part,std::uint32_t tags) {
    const auto model=m.word(unit+4u);const auto packed=m.word(unit+0xcu);std::uint32_t geometry;
    if(model!=none)geometry=m.word(m.word(tag_definition(m,tags,model)+0x28u)+0x5cu*((packed>>9)&511u)+0x34u);
    else {
        const auto bsp=m.word(0x4ecc18u);
        if(m.word(unit)&0x1000u) {
            const auto instance=m.word(bsp+0x144u)+0x58u*((packed>>18)&2047u);
            geometry=m.word(m.word(bsp+0x13cu)+0xc8u*signed_half(m.word(instance+0x34u,2))+0x50u);
        } else geometry=m.word(m.word(bsp+0xa0u)+0xb0u*((packed>>9)&511u)+0x50u);
    }
    return m.word(geometry+4u)+72u*part;
}
using Scalar=__m128;
Scalar scalar(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(signed_word(bits)));}
std::uint32_t bits(Scalar value){return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(value)));}
Scalar mul(Scalar a,Scalar b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar sub(Scalar a,Scalar b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar add(Scalar a,Scalar b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
std::uint32_t metric_bits(const EmitMemory& m,std::uint32_t coverage,std::uint32_t packet,NativeTu15PhysicsFrameFp& fp) {
    detail::NativeTu15ArithmeticEnvironment environment(fp);
    auto metric=_mm_setzero_ps();asm volatile("cvtsi2ss %1,%0":"+x"(metric):"r"(coverage):"memory");
    metric=mul(metric,scalar(m.word(packet+0x28u)));
    metric=mul(metric,scalar(m.word(0x473644u)));
    return bits(mul(metric,scalar(m.word(0x46a120u))));
}

void emit_bank(EmitMemory& m,int category,std::uint32_t shader,std::uint32_t definition,
               std::uint32_t metric,std::uint32_t token,const NativeTu15CommandEmitArgs& a,NativeTu15PhysicsFrameFp& fp) {
    if(category<0)return; // 40E24 rejects -1 before calling 1BF30.
    const auto lod=native_tu15_shader_lod_1bf30(m,m.word(definition+0x3cu,2),shader,metric,fp);
    if(!lod.accepted)return;
    const auto header=0x4ce268u+24u*std::uint32_t(category);
    const auto count=m.word(header+4u);
    if(signed_word(count)>=signed_word(m.word(header)))return;
    const auto layers=(layer(m,lod_block(m,shader),std::uint32_t(lod.index))&a.mask)|a.add;
    m.put(0x3ba38,header+4u,count+1u);
    const auto record=m.word(header+0x14u)+24u*count;
    m.put(0x3ba47,record,0x4cbe0u);
    m.put(0x3ba4d,record+4u,0x4d0a0u);
    m.put(0x3ba54,record+0xcu,token);
    m.put(0x3ba5b,record+8u,shader);
    m.put(0x3ba62,record+0x14u,0,1);
    m.put(0x3ba6b,record+0x15u,std::uint8_t(lod.index),1);
    m.put(0x3ba70,record+0x10u,layers);
    m.put(0x3ba73,header+8u,m.word(header+8u)|layers);
}

void emit_transparent(EmitMemory& m,std::uint32_t shader,std::uint32_t definition,std::uint32_t token,
                      std::uint32_t packet,NativeTu15PhysicsFrameFp& fp) {
    const auto count=m.word(0x4c2aa0u);
    if(signed_word(count)>=0x400) {
        if(m.word(0x4c2ab1u,1))m.put(0x1e2e2,0x4c2ab1u,0,1);
        return;
    }
    const auto record=m.word(0x4c2aa8u)+0x70u*count;
    m.put(0x1e2ce,record,0);
    m.put(0x1e2d0,0x4c2aa0u,count+1u);
    m.put(0x1e2d6,record+0xcu,0,1);
    if(!record)return;
    {
        detail::NativeTu15ArithmeticEnvironment environment(fp);
        const auto dx=sub(scalar(m.word(packet+4u)),scalar(m.word(0x491e38u)));
        const auto dy=sub(scalar(m.word(packet+8u)),scalar(m.word(0x491e3cu)));
        const auto dz=sub(scalar(m.word(packet+0xcu)),scalar(m.word(0x491e40u)));
        m.put(0x413b7,record+0x24u,shader);
        m.put(0x413ba,record+0x28u,token);
        const bool dl=m.word(definition+0x16u,1)&1u,cl=m.word(definition+0x16u,1)&2u;
        m.put(0x413e6,record,dl?(cl?0u:1u):(cl?2u:3u));
        m.put(0x413e8,record+8u,0);
        m.put(0x413ef,record+0x20u,0x41010u);
        const auto y=mul(dy,scalar(m.word(0x491e48u)));
        const auto z=mul(dz,scalar(m.word(0x491e4cu)));
        const auto zy=add(z,y);
        const auto x=mul(dx,scalar(m.word(0x491e44u)));
        const auto dot=add(zy,x);
        const auto distance=sub(_mm_setzero_ps(),dot);
        m.put(0x41428,record+4u,bits(sub(distance,scalar(m.word(packet+0x10u)))));
    }
    m.put(0x41433,record+0x64u,m.word(packet+4u));
    m.put(0x41438,record+0x68u,m.word(packet+8u));
    m.put(0x4143e,record+0x6cu,m.word(packet+0xcu));
    const auto plane=m.word(packet+0x14u,1);m.put(0x41446,record+0xcu,plane,1);
    if(plane) {
        m.put(0x41453,record+0x10u,m.word(packet+0x18u));
        m.put(0x41458,record+0x14u,m.word(packet+0x1cu));
        m.put(0x4145e,record+0x18u,m.word(packet+0x20u));
        m.put(0x41464,record+0x1cu,m.word(packet+0x24u));
    }
}
}
#endif

NativeTu15CommandEmitStores native_tu15_command_emit_41030(const SourcePhysicsContactMemory& memory,
    const NativeTu15CommandEmitArgs& a,NativeTu15PhysicsFrameFp& fp) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    NativeTu15CommandEmitStores staged;
    EmitMemory m(memory,staged);auto arithmetic=fp;
    const auto run=[&] {
        const auto tags=m.word(tags_address);
        const auto unit=0x4c6958u+32u*signed_half(a.unit);
        const auto part=part_row(m,unit,a.part,tags);
        const auto coverage=a.coverage&255u;
        const auto token=(((coverage<<16u)|a.unit)<<8u)|a.part;
        const auto type=m.word(part,2);
        const auto metric=metric_bits(m,coverage,a.packet,arithmetic);
        if(type==0||type==5)return;
        const auto base=m.word(a.packet);
        if(base==none)return;
        const auto shader=a.shader==none?base:a.shader;
        const auto definition=tag_definition(m,tags,shader);
        const auto lod=native_tu15_shader_lod_1bf30(m,m.word(definition+0x3cu,2),shader,metric,arithmetic);
        const auto block=lod_block(m,shader);
        const auto layers=layer(m,block,std::uint32_t(lod.index));
        if(a.shader!=none) {
            const auto base_layers=layer(m,lod_block(m,base),std::uint32_t(lod.index));
            if((base_layers&0x8080u)&&!(base_layers&0xau)&&(layer(m,block,0)&0x200000u))return;
        }
        const bool grouped=(a.grouped&255u)!=0;
        if((layers&0xffff7fffu)||grouped) {
            const auto flags=m.word(unit);
            const bool cat1=flags&2u,cat2=(flags&4u)&&a.mode!=3u;
            if(cat1&&cat2&&a.mode!=0)emit_bank(m,2,shader,definition,metric,token,a,arithmetic);
            const auto category=a.mode==0?0:cat1?1:cat2?2:-1;
            emit_bank(m,category,shader,definition,metric,token,a,arithmetic);
        }
        if((layers&0x8000u)&&!grouped&&a.mode==0)emit_transparent(m,shader,definition,token,a.packet,arithmetic);
    };
    run();arithmetic.control_word=fp.control_word;fp=arithmetic;return staged;
#else
    (void)memory;(void)a;(void)fp;
    throw std::invalid_argument("Exact native41030 arithmetic backend unavailable");
#endif
}
}
