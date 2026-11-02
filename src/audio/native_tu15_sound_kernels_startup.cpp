// Native kernels: sound system startup (TU1.5 sound manager).
#include "audio/native_tu15_sound_kernel.hpp"

namespace halo2::native_tu15_sound {
namespace {
std::uint32_t low8(std::uint32_t reg,std::uint32_t value){return (reg&~0xffu)|(value&0xffu);}

std::uint64_t word64(const M& m,std::uint32_t at){
    std::array<std::byte,8> data;m.read(at,data);return std::bit_cast<std::uint64_t>(data);
}
void private64(M& m,std::uint32_t at,std::uint64_t value){
    const auto data=std::bit_cast<std::array<std::byte,8>>(value);m.write(at,data);
}
long double load_single(std::uint32_t bits){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    long double result;asm volatile("flds %1\n\tfstpt %0":"=m"(result):"m"(bits):"st");return result;
#else
    (void)bits;throw FormatError("Native stream format x87 load backend unavailable");
#endif
}
std::uint64_t integer64(long double value,Floating& floating){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    std::uint64_t result;std::uint16_t status;
    asm volatile("fldt %2\n\tfistpq %0\n\tfnstsw %1":"=m"(result),"=m"(status):"m"(value):"st");
    floating.a.x87_status=std::uint16_t((floating.a.x87_status&~0x200u)|(status&0x200u));return result;
#else
    (void)value;(void)floating;throw FormatError("Native stream format x87 integer backend unavailable");
#endif
}
long double load_integer64(std::uint64_t bits){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    long double result;asm volatile("fildq %1\n\tfstpt %0":"=m"(result):"m"(bits):"st");return result;
#else
    (void)bits;throw FormatError("Native stream format x87 integer load unavailable");
#endif
}
void subtract_pop(Floating& floating){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    auto& a=floating.a;
    if(a.x87_depth<2)throw FormatError("Native stream format x87 subtraction lacks operands");
    const auto left=a.x87[1],right=a.x87[0];long double result;std::uint16_t status;
    // DE E9 is FSUBP ST(1),ST(0); avoid assembler-dialect reversal.
    asm volatile("fldt %2\n\tfldt %3\n\t.byte 0xde,0xe9\n\tfnstsw %1\n\tfstpt %0"
        :"=m"(result),"=m"(status):"m"(left),"m"(right):"st","st(1)");
    a.x87[1]=result;a.x87_status=std::uint16_t((a.x87_status&~0x200u)|(status&0x200u));floating.pop();
#else
    (void)floating;throw FormatError("Native stream format x87 subtraction unavailable");
#endif
}

// The complete32CC0C conversion reached only inside224E20. This private
// composition retains its aligned frame and actual temporary rereads; it is
// not a global conversion registry entry for unrelated callers. x87 operands
// use the caller's controls and existing logical-stack/headroom contract.
void format_integer(M& m,Abi& a,Floating& floating){
    auto& g=a.gp;
    const auto push=[&](std::uint32_t pc,std::uint32_t value){g[4]-=4u;store(m,{},pc,g[4],value);};
    const auto pop=[&](){const auto value=u32(m,g[4]);g[4]+=4u;return value;};
    const auto single=[&](std::uint32_t pc,std::uint32_t at,bool remove){
        const auto value=std::bit_cast<std::uint32_t>(floating.store_single(a.x87[0]));store(m,{},pc,at,value);
        if(remove)floating.pop();
    };
    push(0x32cc0cu,g[5]);g[5]=g[4];g[4]=(g[4]-0x20u)&~0xfu;
    floating.push(a.x87[0]);single(0x32cc17u,g[4]+0x18u,false);
    const auto integer=integer64(a.x87[0],floating);private64(m,g[4]+0x10u,integer);floating.pop();
    floating.push(load_integer64(word64(m,g[4]+0x10u)));
    g[2]=u32(m,g[4]+0x18u);g[0]=u32(m,g[4]+0x10u);
    bool subtract=g[0]!=0;
    if(!subtract){g[2]=u32(m,g[4]+0x14u);subtract=(g[2]&0x7fffffffu)!=0;}
    if(subtract){
        subtract_pop(floating);const bool negative=std::bit_cast<std::int32_t>(g[2])<0;
        single(negative?0x32cc35u:0x32cc53u,g[4],true);g[1]=u32(m,g[4]);
        if(negative)g[1]^=0x80000000u;
        const auto sum=std::uint64_t(g[1])+0x7fffffffu;g[1]=std::uint32_t(sum);const auto carry=std::uint32_t(sum>>32u);
        if(negative){const auto next=std::uint64_t(g[0])+carry;g[0]=std::uint32_t(next);g[2]=u32(m,g[4]+0x14u)+std::uint32_t(next>>32u);}
        else {const auto borrow=std::uint32_t(g[0]<carry);g[0]-=carry;g[2]=u32(m,g[4]+0x14u)-borrow;}
    }else {single(0x32cc77u,g[4]+0x18u,true);single(0x32cc7bu,g[4]+0x18u,true);}
    g[4]=g[5];g[5]=pop();pop_return(m,a);
}

//224E20: format and codec selection. PCM intentionally writes only18 of
//20 format bytes. ADPCM executes the real x87 conversion/frame composition.
void stream_format(M& m,Abi& a,FP fp,const O& o,const Services&){
    auto& g=a.gp;g[4]-=4u;store(m,{},0x224e20u,g[4],g[1]);
    if(g[0]==2u){
        store16(m,o,0x224e88u,g[6],1);store16(m,o,0x224e8du,g[6]+2u,2);
        store(m,o,0x224e93u,g[6]+4u,44100);store(m,o,0x224e9au,g[6]+8u,176400);
        store16(m,o,0x224ea1u,g[6]+12u,4);store16(m,o,0x224ea7u,g[6]+14u,16);
        store16(m,o,0x224eadu,g[6]+16u,0);store(m,o,0x224eb3u,g[7],0x48b8e8u);
    }else{
        const bool stereo=g[0]==1u;set_xmm(a,0,_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(u32(m,0x46a11cu)))));
        store16(m,o,0x224e31u,g[6],0x69);store16(m,o,0x224e36u,g[6]+14u,4);
        if(!stereo)set_xmm(a,0,_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(u32(m,0x46a0d0u)))));
        store(m,{},0x224e46u,g[4],a.xmm[0][0]);
        Floating floating(a,fp);floating.push(load_single(u32(m,g[4])));
        g[4]-=4u;store(m,{},0x224e4eu,g[4],0x224e53u);format_integer(m,a,floating);
        store16(m,o,0x224e53u,g[6]+2u,g[0]);store(m,o,0x224e57u,g[6]+4u,44100);
        store16(m,o,0x224e5eu,g[6]+16u,2);store16(m,o,0x224e64u,g[6]+18u,64);
        g[0]=g[0]*9u;g[0]<<=2u;store16(m,o,0x224e70u,g[6]+12u,g[0]);g[0]&=0xffffu;g[0]*=689u;
        store(m,o,0x224e7du,g[6]+8u,g[0]);store(m,o,0x224e80u,g[7],0x48b908u);
    }
    g[1]=u32(m,g[4]);g[4]+=4u;pop_return(m,a);
}

//225EF0: original descriptor/format caller, including its reused index/output
//slot and the two unwritten PCM format bytes in the caller-owned frame.
void construct_channel(M& m,Abi& a,FP,const O& o,const Services& services){
    auto& g=a.gp;
    const auto push=[&](std::uint32_t pc,std::uint32_t value){g[4]-=4u;store(m,{},pc,g[4],value);};
    const auto pop=[&](){const auto value=u32(m,g[4]);g[4]+=4u;return value;};
    g[4]-=0x30u;g[1]=u32(m,0x52d674u);push(0x225ef9u,g[5]);g[5]=u32(m,g[4]+0x38u);
    g[0]=g[5]*0x34u;g[2]=g[0]+g[1]+0xcu;g[0]=0;
    store(m,{},0x225f09u,g[4]+0x1cu,g[0]);store(m,{},0x225f0du,g[4]+0x20u,g[0]);
    store(m,{},0x225f11u,g[4]+0x24u,g[0]);store(m,{},0x225f15u,g[4]+0x28u,g[0]);
    push(0x225f19u,g[6]);store(m,{},0x225f1au,g[4]+0x30u,g[0]);push(0x225f1eu,g[7]);
    store(m,{},0x225f1fu,g[4]+0x38u,g[0]);g[7]=g[4]+0x40u;g[6]=g[4]+0x10u;g[0]=g[3];
    store(m,{},0x225f2du,g[4]+0xcu,g[2]);store(m,{},0x225f31u,g[4]+0x40u,0);
    call_out(m,a,services,o,0x225f39u,0x224e20u,0x225f3eu,{},true);
    g[0]=u32(m,g[4]+0xcu);g[1]=g[6];store(m,{},0x225f44u,g[4]+0x2cu,g[1]);g[1]=u32(m,g[4]+0x40u);
    push(0x225f4cu,g[3]);g[7]=g[4]+0x28u;store(m,{},0x225f51u,g[4]+0x28u,0);
    store(m,{},0x225f59u,g[4]+0x2cu,2);store(m,{},0x225f61u,g[4]+0x34u,0x227140u);
    store(m,{},0x225f69u,g[4]+0x38u,g[5]);call_out(m,a,services,o,0x225f6du,0x2b8370u,0x225f72u,{},true);
    g[7]=pop();g[6]=pop();g[5]=pop();g[4]+=0x30u;pop_return(m,a,4);
}

//2B7F40: the literal PCM codec Initialize method. ECX is the codec;
//the stream pointer is the sole stack argument. Stock ADPCM uses2E4D60.
void pcm_initialize(M& m,Abi& a,FP,const O& o,const Services&){
    a.gp[0]=argument(m,a,0);store(m,o,0x2b7f44u,a.gp[1]+4u,a.gp[0]);pop_return(m,a,4);
}

//2B8370: construct a stream from the caller's actual descriptor/category.
//Creation3875C2 and Pause3858F8 require their owning platform services;
//this CPU owner supplies neither a device nor a creation/pause result.
void construct_stream(M& m,Abi& a,FP,const O& o,const Services& services){
    auto& g=a.gp;
    const auto push=[&](std::uint32_t pc,std::uint32_t value){g[4]-=4u;store(m,{},pc,g[4],value);};
    const auto pop=[&](){const auto value=u32(m,g[4]);g[4]+=4u;return value;};
    const auto call=[&](std::uint32_t pc,std::uint32_t target,std::uint32_t next){
        call_out(m,a,services,o,pc,target,next,{},true);
    };
    push(0x2b8370u,g[3]);push(0x2b8371u,g[5]);push(0x2b8372u,g[6]);g[6]=g[0];
    store(m,o,0x2b8375u,g[6]+0x30u,g[1]);g[0]=u32(m,g[1]);
    push(0x2b837au,g[6]);call(0x2b837bu,u32(m,g[0]),0x2b837du);
    g[2]=u32(m,g[7]);g[1]=u32(m,0x52d674u);push(0x2b8385u,0);
    g[2]|=0x20000000u;g[5]=g[6]+0x24u;push(0x2b8390u,g[5]);
    store(m,o,0x2b8391u,g[7],g[2]);g[0]=u32(m,g[1]+0x2ab0u);
    push(0x2b8399u,g[7]);push(0x2b839au,g[0]);g[3]=low8(g[3],0);
    call(0x2b839du,0x3875c2u,0x2b83a2u);
    if(std::bit_cast<std::int32_t>(g[0])>=0)g[3]=low8(g[3],1);
    g[2]=u32(m,g[5]);push(0x2b83abu,0);push(0x2b83adu,g[2]);
    // Pause is unconditional, even when creation failed and the output slot
    // still contains its caller-owned preimage.
    call(0x2b83aeu,0x3858f8u,0x2b83b3u);
    g[0]=(g[0]&0xffff0000u)|u16(m,g[4]+0x10u);
    g[0]=(g[0]&0xffffu)==2u?1u:std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(g[0]))));
    g[1]=low8(g[1],u8(m,g[6]+1u));g[0]=low8(g[0],g[0]<<3u);
    g[0]=low8(g[0],g[0]^g[1]);g[0]=low8(g[0],g[0]&0x38u);g[1]=low8(g[1],g[1]^g[0]);
    store8(m,o,0x2b83d4u,g[6]+2u,u8(m,g[6]+2u)&0xc0u);
    store8(m,o,0x2b83d8u,g[6]+1u,g[1]);store(m,o,0x2b83dbu,g[6]+0x28u,0xffffffffu);
    g[0]=0;g[6]+=0x1cu;store(m,o,0x2b83e7u,g[6],g[0]);store(m,o,0x2b83e9u,g[6]+4u,g[0]);
    g[6]=pop();g[5]=pop();g[0]=low8(g[0],g[3]);g[3]=pop();pop_return(m,a,4);
}
}
std::span<const Kernel> startup_kernels(){
    static constexpr Kernel kernels[]{{0x224e20u,stream_format},{0x225ef0u,construct_channel,{},true},
        {0x2b7f40u,pcm_initialize},{0x2b8370u,construct_stream,{},true}};
    return kernels;
}
} // namespace halo2::native_tu15_sound
