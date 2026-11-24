#pragma once
// Native TU1.5 finite checks, the parts of the placement check (175300),
// physics live support unit:
//
//   finite_vector::finite   B16C0(EAX = vector) -> EAX   each component finite
//   scalar_range::within    175280, 175240, 1752C0(XMM0 = x) -> AL
//                           x finite and within the check's range
//
// A value is finite when its exponent bits are not all set. The ranges:
//   175280  [46A0E8] .. [55D6F0]
//   175240  [55D6E8] .. [55D6EC]
//   1752C0  [46A0E8] .. [44EAAC]
// An unordered comparison fails the check. Each value passes through the
// stack slot the entry PUSH ECX makes, as the original reads its bits.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapters
// (native_tu15_finite_vector.hpp, native_tu15_scalar_range.hpp) map the role
// to the original instruction.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include <cstdint>

namespace halo2::native_finite_checks {
using native_object_update::at;
namespace sse=native_aim_clusters::sse;
inline constexpr std::uint32_t exponent_bits=0x7f800000u;
constexpr bool finite(std::uint32_t bits){return (bits&exponent_bits)!=exponent_bits;}

namespace finite_vector {
enum class Access {component,spill,reload};
// Frame: slot() (the stack slot), component(k) before each component.
template<class Memory,class Frame> bool finite(Memory &memory,Frame &frame,std::uint32_t vector) {
    using enum Access;
    for(std::uint32_t k=0;k<3u;++k) {
        frame.component(k);
        memory.store32(spill,frame.slot(),memory.load32(component,at(vector,4u*k)));
        if(!native_finite_checks::finite(memory.load32(reload,frame.slot())))return false;
    }
    return true;
}
} // namespace finite_vector

namespace scalar_range {
enum class Access {spill,reload,low,high};
struct Range {std::uint32_t low,high;};   // the bounds' addresses
inline constexpr Range ranges[]{{0x46a0e8u,0x55d6f0u},{0x55d6e8u,0x55d6ecu},{0x46a0e8u,0x44eaacu}};
enum class Outcome {not_finite,below,above,within};
// Frame: slot() (the stack slot), compared(a,b) (COMISS a,b: the flags).
template<class Memory,class Frame> Outcome within(Memory &memory,Frame &frame,std::uint32_t x,const Range &range) {
    using enum Access;
    memory.store32(spill,frame.slot(),x);
    if(!finite(memory.load32(reload,frame.slot())))return Outcome::not_finite;
    const auto low_value=memory.load32(low,range.low);
    frame.compared(x,low_value);
    if(sse::below(x,low_value))return Outcome::below;
    const auto high_value=memory.load32(high,range.high);
    frame.compared(high_value,x);
    if(sse::below(high_value,x))return Outcome::above;
    return Outcome::within;
}
} // namespace scalar_range
} // namespace halo2::native_finite_checks
