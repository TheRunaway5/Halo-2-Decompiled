#pragma once
// Native TU1.5 curve value, entered at 13E670 (lens draw and physics live
// support units): a value between a curve's two ends.
//
//   curve_value::of   13E670(EAX = function; a, b) -> XMM0, RET 8
//
// A function without a curve (+4) or without a positive count (+0) is zero.
// Otherwise 13DE70 (the two arguments) answers a fraction; a curve whose
// flags byte (+1) carries any of 0xF0 takes the fraction itself. Any other
// clamps it to 0..1 (an unordered fraction stays) and answers the curve's
// start (+4) plus the fraction of the way to its end (+8), in single
// precision on the live MXCSR (SUBSS, MULSS, ADDSS).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the arguments it rereads; the boundary
// adapter (native_tu15_curve_value.hpp) maps the role to the original
// instruction and owns the ABI of the child (13DE70) and the SSE registers.
// The function the original keeps in EDI across the child is a Kept,
// refreshed from the register it returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_curve_value {
using native_object_update::at;
using native_object_update::signed_value;
namespace sse=native_aim_clusters::sse;
namespace globals {inline constexpr std::uint32_t one=0x46a0d0u;}   // float 1
namespace function {inline constexpr std::uint32_t count=0u,curve=4u;}
namespace curve {inline constexpr std::uint32_t flags=1u,start=4u,end=8u;inline constexpr std::uint32_t unclamped=0xf0u;}
inline constexpr std::uint32_t zero_bits=0u;

namespace curve_value {
enum class Access {curve_test,count,second_argument,first_argument,curve_address,curve_flags,start,end,one};
enum class Outcome {empty,unclamped,below,inside,above};
// The original's EDI across 13DE70: the function.
struct Kept {std::uint32_t function;};
// The interpolation's values.
struct Interpolation {std::uint32_t fraction,start,end,clamp,value;};
// Frame:
//   argument(k)                         an argument's slot now
//   fraction(a, b, kept) -> XMM0        13DE70(the arguments pushed), refreshes kept
//   compared(a, b)                      COMISS a,b (its flags; XMM3 zero before the first)
//   interpolated(values)                the SSE registers the interpolation leaves
template<class Memory,class Frame> Outcome of(Memory &memory,Frame &frame,std::uint32_t function_address) {
    using enum Access;
    Kept kept{function_address};
    if(!memory.load32(curve_test,at(kept.function,function::curve)) ||
       signed_value(memory.load32(count,at(kept.function,function::count)))<=0)return Outcome::empty;
    const auto b=memory.load32(second_argument,frame.argument(1u));
    const auto a=memory.load32(first_argument,frame.argument(0u));
    Interpolation v{};
    v.fraction=frame.fraction(a,b,kept);
    const auto curve_record=memory.load32(curve_address,at(kept.function,function::curve));
    if(memory.load8(curve_flags,at(curve_record,curve::flags))&curve::unclamped)return Outcome::unclamped;
    frame.compared(zero_bits,v.fraction);
    v.start=memory.load32(start,at(curve_record,curve::start));
    v.end=memory.load32(end,at(curve_record,curve::end));
    auto outcome=Outcome::inside;
    v.clamp=zero_bits;
    if(!sse::not_above(zero_bits,v.fraction)){v.fraction=zero_bits;outcome=Outcome::below;}
    else {
        v.clamp=memory.load32(one,globals::one);
        frame.compared(v.fraction,v.clamp);
        if(!sse::not_above(v.fraction,v.clamp)){v.fraction=v.clamp;outcome=Outcome::above;}
    }
    v.value=sse::add(sse::multiply(sse::subtract(v.end,v.start),v.fraction),v.start);
    frame.interpolated(v);
    return outcome;
}
} // namespace curve_value
} // namespace halo2::native_curve_value
