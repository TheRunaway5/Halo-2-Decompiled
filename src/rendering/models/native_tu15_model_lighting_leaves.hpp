#pragma once
#include "rendering/models/native_tu15_prt_basis.hpp"

namespace halo2::model_lighting_leaves_detail {
using namespace prt_projection_detail;
inline long double trigonometric(long double input, bool cosine, std::uint16_t& status) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    long double result; std::uint16_t actual;
    if (cosine) asm volatile("fldt %2; fcos; fnstsw %1; fstpt %0"
        : "=m"(result), "=m"(actual) : "m"(input) : "st");
    else asm volatile("fldt %2; fsin; fnstsw %1; fstpt %0"
        : "=m"(result), "=m"(actual) : "m"(input) : "st");
    status = (status & ~0x600u) | (actual & 0x600u);
    return result;
#else
    throw std::runtime_error("Native model lighting requires x87 trigonometry");
#endif
}
inline __m128 subtract(__m128 a, __m128 b) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm("subss %1,%0" : "+x"(a) : "x"(b)); return a;
#else
    return _mm_sub_ss(a,b);
#endif
}
struct DefaultLighting { __m128 first, second; std::uint16_t status; };
// Complete2C390 defaults. Constants remain bank reads because the output can
// overlap those constants; extended sine/cosine and spills retain source order.
inline DefaultLighting initialize(SourcePhysicsContactMemory& memory, std::uint32_t output,
    std::uint32_t stack, std::uint32_t saved_si, std::uint16_t status,
    const SourcePhysicsContactRecordObserver& observe) {
    const auto get = [&](std::uint32_t at) { return read(memory,at); };
    const auto put = [&](std::uint32_t offset,std::uint32_t bits,std::uint32_t pc,unsigned width=4) {
        store(memory,output+offset,bits,width,pc,observe);
    };
    const auto spill = [&](std::uint32_t offset,long double value,std::uint32_t pc) {
        const auto rounded=prt_basis_detail::round_single(value);
        status=(status&~0x200u)|(rounded.status&0x200u);put(offset,rounded.bits,pc);
    };
    auto angle=prt_basis_detail::double_value(memory,0x46a4c8u);status&=~0x200u;
    auto first=scalar(get(0x46a4c0u));
    const auto amplitude=trigonometric(angle,true,status);
    auto second=scalar(get(0x46a4bcu));
    put(0x74u,0,0x2c3aau,1);put(4,0,0x2c3adu);put(0x77u,0,0x2c3b0u,1);put(0x76u,0,0x2c3b3u,1);
    put(0,0xffffffffu,0x2c3b9u);put(8,0xffffffffu,0x2c3bbu);
    constexpr std::array offsets{0x10u,0x48u,0x14u,0x4cu,0x18u,0x50u};
    constexpr std::array sites{0x2c3beu,0x2c3c3u,0x2c3c8u,0x2c3cdu,0x2c3d2u,0x2c3d7u};
    for(unsigned i=0;i<offsets.size();++i)put(offsets[i],bits(first),sites[i]);
    first=scalar(get(0x46a240u));
    put(0x30u,bits(second),0x2c3e4u);put(0x34u,bits(second),0x2c3e9u);put(0x38u,bits(second),0x2c3eeu);
    put(0x28u,bits(first),0x2c3f3u);put(0x2cu,bits(first),0x2c3f8u);
    first=second=_mm_setzero_ps();
    store(memory,stack-4u,saved_si,4,0x2c403u,observe);
    auto phi=prt_basis_detail::double_value(memory,0x46a0f0u);status&=~0x200u;
    const auto cosine=trigonometric(phi,true,status);
    spill(0x3cu,physics_step_detail::mul(cosine,amplitude),0x2c411u);
    phi=prt_basis_detail::double_value(memory,0x46a0f0u);status&=~0x200u;
    const auto sine=trigonometric(phi,false,status);
    spill(0x40u,physics_step_detail::mul(sine,amplitude),0x2c41eu);
    status&=~0x200u;
    angle=prt_basis_detail::double_value(memory,0x46a4c8u);status&=~0x200u;
    spill(0x44u,trigonometric(angle,false,status),0x2c42bu);
    second=subtract(second,scalar(get(output+0x3cu)));put(0x54u,bits(second),0x2c433u);
    second=subtract(first,scalar(get(output+0x40u)));put(0x58u,bits(second),0x2c440u);
    first=subtract(first,scalar(get(output+0x44u)));put(0x5cu,bits(first),0x2c44au);
    put(0x1cu,get(output+0x3cu),0x2c452u);put(0x20u,get(output+0x40u),0x2c457u);
    put(0x24u,get(output+0x44u),0x2c45du);put(0x60u,0xffffu,0x2c460u,2);
    return {first,second,status};
}
struct ObjectFlag { std::uint32_t value,pool,rows,index; };
inline ObjectFlag object_flag(const SourcePhysicsContactMemory& memory,std::uint32_t handle) {
    const auto pool=read(memory,0x4ecbd0u), rows=read(memory,pool+0x44u);
    const auto index=handle&65535u;
    const auto object=read(memory,rows+index*12u+8u);
    return {read(memory,object+0x3dcu,1),pool,rows,index};
}
struct FilteredLookup { std::uint32_t value{},ecx,edx,flag_operand{}; };
inline FilteredLookup filtered_lookup(const SourcePhysicsContactMemory& memory,std::uint32_t handle,
    std::uint32_t mask_slot,std::uint32_t incoming_ecx,std::uint32_t incoming_edx) {
    FilteredLookup out{0,incoming_ecx,incoming_edx,0};
    if(handle==0xffffffffu)return out;
    out.edx=read(memory,0x4ecbd0u);
    const auto count=read(memory,out.edx+0x38u);out.ecx=handle&65535u;
    if(std::int32_t(out.ecx)>=std::int32_t(count))return out;
    const auto stride=read(memory,out.edx+0x24u);
    const auto row=stride*out.ecx+read(memory,out.edx+0x44u);
    out.ecx=read(memory,row,2);if(!out.ecx)return out;
    out.ecx=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(out.ecx))));
    if(std::uint16_t(out.ecx)!=std::uint16_t(handle>>16u))return out;
    if(!row)return out;
    out.ecx=(out.ecx&0xffffff00u)|read(memory,row+3u,1);
    out.edx=1u<<(out.ecx&31u);
    out.flag_operand=read(memory,mask_slot)&out.edx;
    if(out.flag_operand)out.value=read(memory,row+8u);
    return out;
}
template<class State> void bridge(State& state,std::uint32_t entry) {
    const auto stack=state.gp[4];
    if(entry==0x2c390u) {
        const auto output=state.gp[0];
        const auto result=initialize(state.memory,output,stack,state.gp[6],state.fp.status,state.observe);
        state.gp[1]=output+0x1cu;state.gp[2]=0xffffffffu;
        state.gp[6]=read(state.memory,stack-4u);
        state.xmm[0]=result.first;state.xmm[1]=result.second;
        state.fp.status=(state.fp.status&~0x600u)|(result.status&0x600u);
        state.flags.logic(0xffffffffu,4);(void)read(state.memory,stack);state.gp[4]=stack+4u;
    } else if(entry==0xe7bc0u) {
        const auto result=object_flag(state.memory,state.gp[0]);
        state.gp[0]=result.value;state.gp[1]=result.pool;state.gp[2]=result.rows;
        state.flags.logic(result.index,4);(void)read(state.memory,stack);state.gp[4]=stack+4u;
    } else {
        store(state.memory,stack-4u,state.gp[6],4,0xbcdf0u,state.observe);
        store(state.memory,stack-8u,state.gp[7],4,0xbcdf1u,state.observe);
        const auto result=filtered_lookup(state.memory,state.gp[0],stack+4u,state.gp[1],state.gp[2]);
        state.gp[0]=result.value;state.gp[1]=result.ecx;state.gp[2]=result.edx;
        state.flags.logic(result.flag_operand,4);
        state.gp[7]=read(state.memory,stack-8u);state.gp[6]=read(state.memory,stack-4u);
        (void)read(state.memory,stack);state.gp[4]=stack+8u;
    }
}
}
