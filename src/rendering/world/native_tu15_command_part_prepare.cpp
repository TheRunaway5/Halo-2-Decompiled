#include "halo2/native_tu15_command_part_prepare.hpp"
#include "halo2/detail/native_tu15_arithmetic_environment.hpp"
#include <bit>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <emmintrin.h>
#endif

namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory& m,std::uint32_t at,unsigned n=4) {
    std::array<std::byte,4> b{};m.read(at,std::span(b).first(n));std::uint32_t v{};
    for(unsigned i=0;i<n;++i)v|=std::to_integer<unsigned>(b[i])<<(8*i);return v;
}
std::int32_t half(std::uint32_t v){return std::bit_cast<std::int16_t>(std::uint16_t(v));}
std::uint32_t plane(const SourcePhysicsContactMemory& m,std::uint32_t index,std::uint32_t part,bool instance) {
    const auto table=read(m,0x52dfd0u);
    if(!table||read(m,table+0x20u)!=4096u||read(m,table+0x24u)!=2048u||read(m,table+0x28u)!=16u||
       read(m,table+0x2cu)!=0x1e8a0u||read(m,table+0x30u)!=0x1e8d0u)
        throw std::invalid_argument("Native45CD0 requires its populated original transparent-plane table");
    const auto key=(index&65535u)|((part*2u+unsigned(instance))<<16);
    std::uint32_t hash{};
    constexpr std::array<unsigned,4> weights{17,43,53,59};
    for(unsigned i=0;i<4;++i)hash+=std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(key>>(8*i)))))*weights[i];
    auto node=read(m,table+0x3cu+4u*(hash%4096u));
    for(unsigned count=0;node;++count) {
        if(count>=2048u)throw std::invalid_argument("Native45CD0 transparent-plane chain exceeds its original capacity");
        if(read(m,node+4u)==hash&&read(m,read(m,node))==key)return read(m,node)+4u;
        node=read(m,node+8u);
    }
    return 0;
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
using Scalar=__m128;
Scalar scalar(std::uint32_t v){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(v)));}
std::uint32_t bits(Scalar v){return std::bit_cast<std::uint32_t>(_mm_cvtsi128_si32(_mm_castps_si128(v)));}
Scalar mul(Scalar a,Scalar b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar add(Scalar a,Scalar b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool positive(Scalar a) {
    unsigned char above;const auto zero=scalar(0);
    asm volatile("comiss %2,%1\n\tseta %0":"=qm"(above):"x"(a),"x"(zero):"cc","memory");return above;
}
#endif
}
NativeTu15PartPacket45cd0 native_tu15_command_part_prepare_45cd0(const SourcePhysicsContactMemory& m,
    std::uint32_t unit,std::uint32_t part,std::uint32_t matrix,const std::array<std::byte,44>& initial,
    NativeTu15PhysicsFrameFp& fp) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    NativeTu15PartPacket45cd0 out;out.bytes=initial;
    const auto store=[&](std::uint32_t pc,unsigned offset,std::uint32_t value,unsigned n=4) {
        out.stores.push_back({pc,offset,value,n});
        for(unsigned i=0;i<n;++i)out.bytes[offset+i]=std::byte(value>>(8*i));
    };
    const auto at=0x4c6958u+32u*std::uint32_t(half(unit));
    const auto model=read(m,at+4u),packed=read(m,at+12u),section=(packed>>9)&511u;
    std::uint32_t geometry{},definition{},bsp{};
    if(model!=0xffffffffu) {
        definition=read(m,read(m,0x4f0414u)+16u*(model&65535u)+8u);
        geometry=read(m,read(m,definition+0x28u)+92u*section+0x34u);
    }else {
        bsp=read(m,0x4ecc18u);
        if(read(m,at)&0x1000u) {
            const auto instance=read(m,bsp+0x144u)+88u*((packed>>18)&2047u);
            geometry=read(m,read(m,bsp+0x13cu)+200u*std::uint32_t(half(read(m,instance+0x34u,2)))+0x50u);
        }else geometry=read(m,read(m,bsp+0xa0u)+176u*section+0x50u);
    }
    if(!geometry)return out;
    // SSE-only ownership preserves the original x87 control word, including
    // reserved bit6 that FLDENV/FNSTCW canonicalize on the host.
    const auto original_control=fp.control_word;auto arithmetic=fp;
    {
        detail::NativeTu15ArithmeticEnvironment environment(arithmetic);
        const auto p=read(m,geometry+4u)+72u*std::uint32_t(half(part));
        store(0x45d8cu,0x10,0);
        const auto lod=read(m,p+0x2cu);store(0x45daeu,0x28,positive(scalar(lod))?lod:read(m,0x46a0d0u));
        const auto material=read(m,(model==0xffffffffu?bsp+0xa8u:definition+0x64u))+
            32u*std::uint32_t(half(read(m,p+4u,2)));
        bool visible=true;
        if(model==0xffffffffu) {
            const auto index=read(m,material+0x1cu,1);
            if(index!=255u) {
                const auto instance=(packed>>18)&2047u,selector=std::uint32_t(half(read(m,0x474c24u,2)));
                const auto bank=read(m,0x4f9b50u)+(instance==2047u?selector*32u+1u:selector*1024u+0x201u+instance);
                visible=(read(m,bank+4u*(index>>5))&(1u<<(index&31u)))!=0;
            }
        }
        store(0x45e2du,0,read(m,material+12u));store(0x45e2fu,0x14,0,1);
        if(visible&&model!=0xffffffffu&&section!=255u) {
            const auto palette_offset=read(m,at+8u);
            if(palette_offset&0x80000000u)throw std::invalid_argument("Native45CD0 has no original model palette");
            const auto palette=0x49433cu+(palette_offset&0xfffffffu);
            auto node=half(read(m,read(m,definition+0x28u)+92u*section+0x2cu,2));
            const bool rigid_none=node==-1;
            if(rigid_none)node=0;
            else if(read(m,definition+4u,1)&4u)node=half(read(m,palette+4u*std::uint32_t(half(matrix))+4u,2));
            const auto pose=palette+0x44u+48u*std::uint32_t(node);
            for(unsigned i=0;i<3;++i)store((rigid_none?0x45ecau:0x45eb6u)+6u*i,4u+4u*i,read(m,pose+12u+16u*i));
            const auto bias=read(m,at+0x1bu,1)&15u;
            if(bias) {
                auto value=scalar(0);const auto integer=int(bias)-5;
                asm volatile("cvtsi2ss %1,%0":"+x"(value):"r"(integer):"memory");
                store(0x45ef4u,0x10,bits(mul(value,scalar(read(m,0x46a180u)))));
            }
            out.accepted=true;
        }else if(visible&&model==0xffffffffu) {
            const bool instance=(read(m,at)&0x1000u)!=0;
            const auto inst=instance?read(m,bsp+0x144u)+88u*((packed>>18)&2047u):0u;
            const auto found=plane(m,instance?read(m,inst+0x34u,2):section,part,instance);
            store(instance?0x45f4bu:0x4606bu,0x14,unsigned(found!=0),1);
            std::array<std::uint32_t,4> plane_words{};
            if(found)for(unsigned i=0;i<4;++i) {
                plane_words[i]=read(m,found+4u*i);
                store(instance?std::array{0x45f55u,0x45f5au,0x45f60u,0x45f66u}[i]:
                    std::array{0x46075u,0x4607au,0x46080u,0x46086u}[i],0x18u+4u*i,plane_words[i]);
            }
            for(unsigned i=0;i<3;++i)store(instance?std::array{0x45f73u,0x45f78u,0x45f7eu}[i]:
                std::array{0x46091u,0x46096u,0x4609eu}[i],4u+4u*i,read(m,(instance?inst+0x3cu:p+0x10u)+4u*i));
            if(instance&&found) {
                const auto x=scalar(plane_words[0]),y=scalar(plane_words[1]),z=scalar(plane_words[2]);
                Scalar transformed[3];
                for(unsigned i=0;i<3;++i) {
                    const auto a=mul(scalar(read(m,inst+0x1cu+4u*i)),z);
                    const auto b=mul(scalar(read(m,inst+0x10u+4u*i)),y);
                    const auto c=mul(scalar(read(m,inst+4u+4u*i)),x);
                    transformed[i]=add(add(a,b),c);
                    store(std::array{0x45fb9u,0x45fe5u,0x4600du}[i],0x18u+4u*i,bits(transformed[i]));
                }
                auto d=mul(scalar(plane_words[3]),scalar(read(m,inst)));
                d=add(d,mul(scalar(read(m,inst+0x30u)),transformed[2]));
                d=add(d,mul(scalar(read(m,inst+0x2cu)),transformed[1]));
                d=add(d,mul(scalar(read(m,inst+0x28u)),transformed[0]));
                store(0x46046u,0x24,bits(d));
            }
            out.accepted=true;
        }
    }
    arithmetic.control_word=original_control;fp=arithmetic;return out;
#else
    (void)m;(void)unit;(void)part;(void)matrix;(void)initial;(void)fp;
    throw std::runtime_error("Exact native45CD0 arithmetic backend unavailable");
#endif
}
}
