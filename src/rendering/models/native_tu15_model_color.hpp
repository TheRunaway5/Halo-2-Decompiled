#pragma once
#include "rendering/models/native_tu15_prt_basis.hpp"

namespace halo2::model_color_detail {
using namespace prt_projection_detail;
struct ColorResult { std::uint32_t value,second,third;std::uint16_t status; };
inline std::int32_t signed_word(std::uint32_t word) { return std::bit_cast<std::int32_t>(word); }
inline prt_basis_detail::RoundedSingle integer_single(long double input) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    std::int32_t value;std::uint16_t status;
    asm volatile("fldt %2; fistpl %0; fnstsw %1"
        : "=m"(value),"=m"(status):"m"(input):"st");
    return {std::bit_cast<std::uint32_t>(value),status};
#else
    throw std::runtime_error("Native model RGB packing requires x87 rounding");
#endif
}
// Complete1346C0 RGB conversion. Each channel retains the scratch publication
// and FILD reread: an output overlapping scratch can change the final channel.
inline ColorResult unpack(SourcePhysicsContactMemory& memory,std::uint32_t output,
    std::uint32_t packed,std::uint32_t stack,const SourcePhysicsContactRecordObserver& observe) {
    const auto put=[&](std::uint32_t at,std::uint32_t word,std::uint32_t pc){store(memory,at,word,4,pc,observe);};
    const auto scratch=stack-4u;
    put(scratch,packed,0x1346c0u);
    const auto red=(packed>>16u)&255u;put(scratch,red,0x1346ceu);
    auto extended=static_cast<long double>(signed_word(read(memory,scratch)));
    extended=physics_step_detail::mul(extended,prt_basis_detail::single(memory,0x46a0c4u));
    const auto green=(packed>>8u)&255u;
    auto rounded=prt_basis_detail::round_single(extended);put(output,rounded.bits,0x1346efu);
    put(scratch,green,0x1346f1u);
    extended=static_cast<long double>(signed_word(read(memory,scratch)));
    extended=physics_step_detail::mul(extended,prt_basis_detail::single(memory,0x46a0c4u));
    const auto blue=packed&255u;put(scratch,blue,0x13470du);
    rounded=prt_basis_detail::round_single(extended);put(output+4u,rounded.bits,0x134710u);
    extended=static_cast<long double>(signed_word(read(memory,scratch)));
    extended=physics_step_detail::mul(extended,prt_basis_detail::single(memory,0x46a0c4u));
    rounded=prt_basis_detail::round_single(extended);put(output+8u,rounded.bits,0x134724u);
    return {read(memory,scratch),green,blue,rounded.status};
}
// Complete1349E0 RGB packing, including source x87 integer conversion instead
// of C++ casts, byte wrapping and all observable intermediate stack stores.
inline ColorResult pack(SourcePhysicsContactMemory& memory,std::uint32_t stack,
    const SourcePhysicsContactRecordObserver& observe) {
    const auto frame=stack-8u;
    const auto put=[&](std::uint32_t word,std::uint32_t pc){store(memory,frame,word,4,pc,observe);};
    const auto factor=read(memory,0x46a148u);
    store(memory,frame+4u,factor,4,0x1349ebu,observe);put(0,0x1349f1u);
    const auto source=read(memory,stack+4u);
    auto red=prt_basis_detail::single(memory,source);
    auto green=prt_basis_detail::single(memory,source+4u);
    auto blue=prt_basis_detail::single(memory,source+8u);
    const auto scale=prt_basis_detail::single(memory,frame+4u);
    red=physics_step_detail::mul(red,scale);green=physics_step_detail::mul(green,scale);
    blue=physics_step_detail::mul(blue,scale);
    auto rounded=integer_single(blue);put(rounded.bits,0x134a0eu);
    put(read(memory,frame)&255u,0x134a11u);auto packed=read(memory,frame);
    rounded=integer_single(green);put(rounded.bits,0x134a1bu);
    put(read(memory,frame)&255u,0x134a1eu);put(read(memory,frame)<<8u,0x134a25u);
    packed|=read(memory,frame);
    rounded=integer_single(red);put(rounded.bits,0x134a2cu);
    put(read(memory,frame)&255u,0x134a2fu);put(read(memory,frame)<<16u,0x134a36u);
    packed|=read(memory,frame);put(packed,0x134a3du);
    return {read(memory,frame),packed,factor,rounded.status};
}
template<class State>void bridge(State& state,std::uint32_t entry) {
    const auto stack=state.gp[4];
    if(entry==0x1346c0u) {
        const auto result=unpack(state.memory,state.gp[0],state.gp[1],stack,state.observe);
        state.gp[1]=result.value;state.gp[2]=result.second;
        state.flags.logic(result.third,4);
        state.fp.status=(state.fp.status&~0x200u)|(result.status&0x200u);
    } else {
        const auto result=pack(state.memory,stack,state.observe);
        state.gp[0]=result.value;state.gp[2]=result.second;state.xmm[0]=scalar(result.third);
        state.flags.plus(stack-8u,8u,4);
        state.fp.status=(state.fp.status&~0x200u)|(result.status&0x200u);
    }
    (void)read(state.memory,stack);state.gp[4]=stack+4u;
}
}
