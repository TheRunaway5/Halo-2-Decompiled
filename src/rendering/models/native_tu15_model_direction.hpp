#pragma once
#include "rendering/models/native_tu15_model_lighting_leaves.hpp"

namespace halo2::model_direction_detail {
using namespace prt_projection_detail;
enum class Order { below,equal,above,unordered };
inline Order compare_extended(long double left,long double right) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    unsigned char carry,zero,parity;
    asm volatile("fldt %4; fldt %3; fcomip %%st(1),%%st; setb %0; sete %1; setp %2; fstp %%st(0)"
        : "=qm"(carry),"=qm"(zero),"=qm"(parity):"m"(left),"m"(right):"cc","st");
    return parity?Order::unordered:carry?Order::below:zero?Order::equal:Order::above;
#else
    throw std::runtime_error("Native model direction requires x87 comparison");
#endif
}
inline bool positive_over(__m128 left,__m128 right) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    unsigned char above;
    asm volatile("comiss %2,%1; seta %0":"=qm"(above):"x"(left),"x"(right):"cc");
    return above;
#else
    return _mm_comigt_ss(left,right);
#endif
}
inline __m128 quotient(__m128 left,__m128 right) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm("divss %1,%0":"+x"(left):"x"(right));return left;
#else
    return _mm_div_ss(left,right);
#endif
}
struct Caller {
    std::uint32_t output,input,stack,saved_si,saved_di;
    __m128 previous_first,previous_fourth;
};
struct Direction {
    std::uint32_t copied_z,saved_si,saved_di;
    __m128 first,second,fourth;
    std::uint16_t condition;
    Order last_comparison;
};
// Complete11A360 direction clamp. Native typed extended/SSE calculations keep
// both authored horizontal normalization passes and the original bank rereads.
// Stack argument spills remain memory transactions because output/input views
// may overlap that argument or the saved-register words.
inline Direction clamp(SourcePhysicsContactMemory& memory,const Caller& caller,
    const SourcePhysicsContactRecordObserver& observe) {
    using physics_step_detail::mul;using physics_step_detail::add;using physics_step_detail::sub;
    using physics_solver_detail::root;using prt_basis_detail::single;
    const auto get=[&](std::uint32_t at){return read(memory,at);};
    const auto put=[&](std::uint32_t at,std::uint32_t value,std::uint32_t pc){store(memory,at,value,4,pc,observe);};
    const auto spill=[&](std::uint32_t at,long double value,std::uint32_t pc){
        const auto rounded=prt_basis_detail::round_single(value);put(at,rounded.bits,pc);return rounded.status;
    };
    const auto horizontal_length=[&] {
        const auto y=single(memory,caller.output+4u);
        const auto x=single(memory,caller.output);
        const auto yy=mul(y,y),xx=mul(x,x);
        return root(add(yy,xx));
    };
    const auto angle=single(memory,caller.stack+4u);
    const auto zero=_mm_setzero_ps();
    std::uint16_t condition{};
    const auto bound=model_lighting_leaves_detail::trigonometric(angle,true,condition);
    put(caller.stack-4u,caller.saved_si,0x11a369u);put(caller.stack-8u,caller.saved_di,0x11a36au);
    auto copied=get(caller.input);spill(caller.stack+4u,bound,0x11a371u);
    const auto z=single(memory,caller.input+8u);
    put(caller.output,copied,0x11a378u);copied=get(caller.input+4u);
    const auto absolute_z=std::fabs(z);
    const auto spilled_bound=single(memory,caller.stack+4u);
    put(caller.output+4u,copied,0x11a383u);
    const auto copied_z=get(caller.input+8u);const auto restored_di=get(caller.stack-8u);
    put(caller.output+8u,copied_z,0x11a38cu);
    const auto outside=compare_extended(absolute_z,spilled_bound)==Order::above;
    const auto restored_si=get(caller.stack-4u);
    auto first=caller.previous_first,fourth=caller.previous_fourth;
    if(outside) {
        if(positive_over(zero,scalar(get(caller.input+8u))))
            first=model_lighting_leaves_detail::subtract(zero,scalar(get(caller.stack+4u)));
        else first=scalar(get(caller.stack+4u));
        put(caller.output+8u,bits(first),0x11a3adu);
    }
    const auto absolute_x=std::fabs(single(memory,caller.output));
    auto second=scalar(get(0x46a0d0u));
    bool reset_horizontal=false;
    if(compare_extended(single(memory,0x46a18cu),absolute_x)==Order::above) {
        const auto absolute_y=std::fabs(single(memory,caller.output+4u));
        reset_horizontal=compare_extended(single(memory,0x46a18cu),absolute_y)==Order::above;
    }
    if(!reset_horizontal) {
        const auto length=horizontal_length();spill(caller.stack+4u,length,0x11a3f0u);
        if(compare_extended(single(memory,0x46a0ecu),std::fabs(length))!=Order::above) {
            fourth=scalar(get(caller.output));first=quotient(second,scalar(get(caller.stack+4u)));
            fourth=multiply(fourth,first,false);put(caller.output,bits(fourth),0x11a413u);
            fourth=scalar(get(caller.output+4u));fourth=multiply(fourth,first,false);
            put(caller.output+4u,bits(fourth),0x11a420u);
        } else put(caller.stack+4u,0,0x11a427u);
        reset_horizontal=compare_extended(single(memory,0x46a0ecu),
            std::fabs(single(memory,caller.stack+4u)))==Order::above;
    }
    if(reset_horizontal) {
        put(caller.output,bits(second),0x11a43fu);put(caller.output+4u,0,0x11a443u);
    }
    const auto length=horizontal_length();spill(caller.stack+4u,length,0x11a45du);
    const auto comparison=compare_extended(single(memory,0x46a0ecu),std::fabs(length));
    if(comparison!=Order::above) {
        second=quotient(second,scalar(get(caller.stack+4u)));
        first=scalar(get(caller.output));first=multiply(first,second,false);
        put(caller.output,bits(first),0x11a47du);
        first=scalar(get(caller.output+4u));first=multiply(first,second,false);
        put(caller.output+4u,bits(first),0x11a48au);
    }
    const auto final_z=single(memory,caller.output+8u);
    const auto zz=mul(final_z,final_z);
    const auto complement=sub(single(memory,0x46a0d0u),zz);
    const auto horizontal_scale=root(complement);
    spill(caller.output,mul(horizontal_scale,single(memory,caller.output)),0x11a4a2u);
    const auto final_status=spill(caller.output+4u,
        mul(horizontal_scale,single(memory,caller.output+4u)),0x11a4a7u);
    condition=(condition&0x400u)|(final_status&0x200u);
    return {copied_z,restored_si,restored_di,first,second,fourth,condition,comparison};
}
template<class State>void bridge(State& state) {
    const Caller caller{state.gp[0],state.gp[1],state.gp[4],state.gp[6],state.gp[7],state.xmm[0],state.xmm[3]};
    const auto result=clamp(state.memory,caller,state.observe);
    state.gp[2]=result.copied_z;state.gp[6]=result.saved_si;state.gp[7]=result.saved_di;
    state.xmm[0]=result.first;state.xmm[1]=result.second;state.xmm[2]=_mm_setzero_ps();state.xmm[3]=result.fourth;
    state.fp.status=(state.fp.status&~0x600u)|result.condition;
    state.flags.cf=result.last_comparison==Order::below||result.last_comparison==Order::unordered;
    state.flags.zf=result.last_comparison==Order::equal||result.last_comparison==Order::unordered;
    state.flags.pf=result.last_comparison==Order::unordered;
    state.flags.af=state.flags.sf=state.flags.of=false;
    (void)read(state.memory,caller.stack);state.gp[4]=caller.stack+8u;
}
}
