#pragma once
// Native TU1.5 transition fraction, entered at 1E1620 (AI frame and physics
// live support units): how far a timed transition has come, shaped by its
// curve and kept within 0..1.
//
//   fraction::of   1E1620(EAX = transition) -> XMM0
//
// A transition is three signed bytes: the ticks elapsed (+0), its duration
// (+1) and its curve (+2). A transition without duration is at 0. Otherwise
// the fraction is elapsed / duration (a duration under 1 counts as 1),
// shaped through the original's curve table (1E16CC, by curve - 1):
//    1  smoothstep (110FC0, 3x^2 - 2x^3)
//    2  smootherstep (1E1920, 6x^5 - 15x^4 + 10x^3)
//    3  ease out, square:  1 - (x - 1)^2
//    4  ease out, cube:    1 - (1 - x)^3
// and any other curve (0 among them) is linear. The shaped value is then clamped: below 0
// it is 0, above 1 it is 1 (an unordered value is kept).
//
// The single-precision arithmetic is the original's scalar SSE, in its
// order, on the live MXCSR. This is the game logic only. The game's memory
// is the retained TU1.5 bank, reached through a Memory object with typed
// loads and stores; every access names its role (Access), and the boundary
// adapter (native_tu15_transition_fraction.hpp) maps the role to the
// original instruction.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include <bit>
#include <cstdint>

namespace halo2::native_transition_fraction {
using native_object_update::at;
namespace sse=native_aim_clusters::sse;
namespace globals {
inline constexpr std::uint32_t one=0x46a0d0u;            // float 1
inline constexpr std::uint32_t curve_table=0x1e16ccu;    // the original jump table, by curve - 1
}
namespace transition {inline constexpr std::uint32_t elapsed=0u,duration=1u,curve=2u;}   // signed bytes
inline constexpr std::uint32_t curve_cases=4u;
// The curve table's targets, as the unit's canonical_branch names them.
inline constexpr std::uint32_t smoothstep_case=0x1e1661u,smootherstep_case=0x1e1673u,
    ease_out_square_case=0x1e1685u,ease_out_cube_case=0x1e1699u;
inline constexpr std::uint32_t zero_bits=0u;
// CVTSI2SS of a signed byte: exact.
inline std::uint32_t float_of_byte(std::uint32_t byte){return std::bit_cast<std::uint32_t>(float(std::int8_t(byte)));}

namespace fraction {
enum class Access {duration,one,elapsed,curve,curve_case,one_after_curve};
// Frame:
//   timed(clamped)              the duration is converted (and replaced by 1 when clamped)
//   divided(x)                  the linear fraction
//   smoothstep(x)               110FC0 (XMM2 = x): returns XMM0's value
//   smootherstep(x)             1E1920 (XMM3 = x): returns XMM0's value
//   squared(value)              the square ease
//   cubed(rest_cubed,value)     the cube ease
//   unknown_case()              a table target the unit does not name (throws)
//   compared(a,b)               COMISS a,b (the flags the caller sees)
//   clamped_low() / clamped_high(one)
template<class Memory,class Frame> std::uint32_t of(Memory &memory,Frame &frame,std::uint32_t address) {
    using enum Access;
    const auto duration_byte=memory.load8(duration,at(address,transition::duration));
    if((duration_byte&0xffu)==0)return zero_bits;
    auto one_value=memory.load32(one,globals::one);
    const auto duration_value=float_of_byte(duration_byte);
    const bool clamped=sse::not_above(duration_value,one_value);
    frame.timed(clamped);
    const auto divisor=clamped?one_value:duration_value;
    const auto elapsed_value=float_of_byte(memory.load8(elapsed,at(address,transition::elapsed)));
    const auto index=std::uint32_t(std::int32_t(std::int8_t(memory.load8(curve,at(address,transition::curve)))))-1u;
    auto value=sse::divide(elapsed_value,divisor);
    frame.divided(value);
    if(index<curve_cases) {
        switch(memory.load32(curve_case,globals::curve_table+4u*index)) {
        case smoothstep_case:
            value=frame.smoothstep(value);
            one_value=memory.load32(one_after_curve,globals::one);
            break;
        case smootherstep_case:
            value=frame.smootherstep(value);
            one_value=memory.load32(one_after_curve,globals::one);
            break;
        case ease_out_square_case: {
            const auto from_one=sse::subtract(value,one_value);
            value=sse::subtract(one_value,sse::multiply(from_one,from_one));
            frame.squared(value);
            break;
        }
        case ease_out_cube_case: {
            const auto rest=sse::subtract(one_value,value);
            const auto rest_cubed=sse::multiply(sse::multiply(rest,rest),rest);
            value=sse::subtract(one_value,rest_cubed);
            frame.cubed(rest_cubed,value);
            break;
        }
        default:frame.unknown_case();
        }
    }
    frame.compared(zero_bits,value);
    if(!sse::not_above(zero_bits,value)){frame.clamped_low();return zero_bits;}
    frame.compared(value,one_value);
    if(!sse::not_above(value,one_value)){frame.clamped_high(one_value);return one_value;}
    return value;
}
} // namespace fraction
} // namespace halo2::native_transition_fraction
