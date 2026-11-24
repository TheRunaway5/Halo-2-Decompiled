#pragma once
// Native TU1.5 flat axes, entered at 152D60 (physics live support unit): a
// facing's forward and left axes in the horizontal plane.
//
//   flat_axes::build   152D60(EAX = facing, EDI = fallback, ESI = axes)
//
// The facing, its z cleared, is normalized into the forward axis (30BD0,
// which answers the length). A zero length tries the fallback the same way;
// a zero length again (against the float zero at 46A0E8) takes the default
// axis ([474D08]) as it is. The left axis (axes +0xC) is the forward axis
// turned a quarter about z: (0 - y, x, 0).
//
// The single-precision arithmetic is the original's scalar SSE on the live
// MXCSR. This is the game logic only. The game's memory is the retained
// TU1.5 bank, reached through a Memory object with typed loads and stores;
// every access names its role (Access), and the boundary adapter
// (native_tu15_flat_axes.hpp) maps the role to the original instruction and
// owns the ABI of the child 30BD0.
#include "physics/runtime/native_tu15_perpendicular_systems.hpp"
#include <cstdint>

namespace halo2::native_flat_axes {
using native_object_update::at;
using native_perpendicular::unequal;
namespace sse=native_aim_clusters::sse;
namespace globals {
inline constexpr std::uint32_t zero=0x46a0e8u;           // float 0
inline constexpr std::uint32_t default_axis=0x474d08u;   // Vector*: the direction of last resort
}
namespace axes {inline constexpr std::uint32_t forward=0u,left=0xcu;}
inline constexpr std::uint32_t zero_bits=0u;

namespace flat_axes {
enum class Access {x,x_store,y,y_store,z,z_store,flatten,zero,default_axis,forward_x,forward_y,left_x,left_y,left_z};
enum class Source {facing,fallback,default_axis};   // the forward axis's
// Frame: copying(source) before each copy; normalized(source) 30BD0 on the
// forward axis, -> the length; compared(source) after each length's test.
template<class Memory,class Frame> void copy(Memory &memory,Frame &frame,Source source,std::uint32_t vector,std::uint32_t forward) {
    using enum Access;
    frame.copying(source);
    memory.store32(x_store,at(forward,0u),memory.load32(x,at(vector,0u)));
    memory.store32(y_store,at(forward,4u),memory.load32(y,at(vector,4u)));
    memory.store32(z_store,at(forward,8u),memory.load32(z,at(vector,8u)));
}
template<class Memory,class Frame> Source build(Memory &memory,Frame &frame,std::uint32_t facing,std::uint32_t fallback,std::uint32_t result) {
    using enum Access;
    const auto forward=at(result,axes::forward);
    auto source=Source::facing;
    copy(memory,frame,Source::facing,facing,forward);
    memory.store32(flatten,at(forward,8u),zero_bits);
    const bool facing_length=unequal(frame.normalized(Source::facing),zero_bits);
    frame.compared(Source::facing);
    if(!facing_length) {
        source=Source::fallback;
        copy(memory,frame,Source::fallback,fallback,forward);
        memory.store32(flatten,at(forward,8u),zero_bits);
        const auto length=frame.normalized(Source::fallback);
        const bool fallback_length=unequal(length,memory.load32(zero,globals::zero));
        frame.compared(Source::fallback);
        if(!fallback_length) {
            source=Source::default_axis;
            copy(memory,frame,Source::default_axis,memory.load32(Access::default_axis,globals::default_axis),forward);
        }
    }
    const auto left=at(result,axes::left);
    const auto forward_x_value=memory.load32(forward_x,at(forward,0u));
    memory.store32(left_x,at(left,0u),sse::subtract(zero_bits,memory.load32(forward_y,at(forward,4u))));
    memory.store32(left_y,at(left,4u),forward_x_value);
    memory.store32(left_z,at(left,8u),zero_bits);
    return source;
}
} // namespace flat_axes
} // namespace halo2::native_flat_axes
