#pragma once
#include "rendering/models/native_tu15_model_color.hpp"
#include "rendering/models/native_tu15_model_lighting_leaves.hpp"

namespace halo2::model_color_step_detail {
using namespace prt_projection_detail;
inline bool above(__m128 left,__m128 right) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    unsigned char value;
    asm volatile("comiss %2,%1; seta %0":"=qm"(value):"x"(left),"x"(right):"cc");
    return value;
#else
    return _mm_comigt_ss(left,right);
#endif
}
struct ColorStep {
    std::uint32_t value,packed_value,target_blue,factor;
    __m128 difference,current;
    std::uint16_t status;
};
// Complete3E4D0 packed-RGB step transaction. The cap retains its original
// COMISS unordered rules and whole-lane moves; its two decode calls and encode
// call are typed native kernels with the original return/scratch publications.
inline ColorStep step(SourcePhysicsContactMemory& memory,std::uint32_t current,
    std::uint32_t stack,__m128 cap,const SourcePhysicsContactRecordObserver& observe) {
    const auto get=[&](std::uint32_t at){return read(memory,at);};
    const auto put=[&](std::uint32_t at,std::uint32_t word,std::uint32_t pc){store(memory,at,word,4,pc,observe);};
    const auto first_color=get(current),frame=stack-0x18u;
    put(frame-4u,0x3e4ddu,0x3e4d8u);
    (void)model_color_detail::unpack(memory,frame,first_color,frame-4u,observe);
    const auto target=get(frame+0x1cu);const auto target_color=get(target);
    put(frame-4u,0x3e4ecu,0x3e4e7u);
    const auto decoded=model_color_detail::unpack(memory,frame+12u,target_color,frame-4u,observe);
    auto delta=scalar(get(frame+12u));auto previous=scalar(get(frame));
    delta=model_lighting_leaves_detail::subtract(delta,previous);
    const auto negative=model_lighting_leaves_detail::subtract(_mm_setzero_ps(),cap);
    if(above(negative,delta))delta=negative;
    else if(above(delta,cap))delta=cap;
    delta=add(delta,previous,false);previous=scalar(get(frame+4u));
    put(frame,bits(delta),0x3e51eu);
    delta=scalar(get(frame+16u));delta=model_lighting_leaves_detail::subtract(delta,previous);
    if(above(negative,delta))delta=negative;
    else if(above(delta,cap))delta=cap;
    delta=add(delta,previous,false);previous=scalar(get(frame+8u));
    put(frame+4u,bits(delta),0x3e549u);
    delta=scalar(get(frame+20u));delta=model_lighting_leaves_detail::subtract(delta,previous);
    auto adjusted=negative;
    if(!above(adjusted,delta)) {
        const auto capped=above(delta,cap);adjusted=cap;
        if(!capped)adjusted=delta;
    }
    adjusted=add(adjusted,previous,false);
    put(frame-4u,frame,0x3e570u);put(frame+8u,bits(adjusted),0x3e571u);
    put(frame-8u,0x3e57cu,0x3e577u);
    const auto encoded=model_color_detail::pack(memory,frame-8u,observe);
    put(current,encoded.value,0x3e57cu);
    return {encoded.value,encoded.second,decoded.value,encoded.third,delta,previous,encoded.status};
}
template<class State>void bridge(State& state) {
    const auto stack=state.gp[4];
    const auto result=step(state.memory,state.gp[6],stack,state.xmm[2],state.observe);
    state.gp[0]=result.value;state.gp[2]=result.packed_value;state.gp[1]=result.target_blue;
    state.xmm[0]=scalar(result.factor);state.xmm[1]=result.difference;state.xmm[3]=result.current;
    state.fp.status=(state.fp.status&~0x200u)|(result.status&0x200u);
    state.flags.plus(stack-0x1cu,0x1cu,4);
    (void)read(state.memory,stack);state.gp[4]=stack+8u;
}
}
