#pragma once
// Native TU1.5 world bounds, entered at 1751B0 (physics live support unit):
// whether a point lies within the world's limits.
//
//   world_bounds::inside   1751B0(EAX = point) -> AL
//
// Each coordinate in turn (x, y, z) must be finite (its exponent not all
// ones) and neither below 44EA9C nor above 44EAA0 (COMISS; unordered fails).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_world_bounds.hpp) maps the role to the original instruction.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_world_bounds {
using native_object_update::at;
namespace sse=native_aim_clusters::sse;
namespace globals {inline constexpr std::uint32_t least=0x44ea9cu,most=0x44eaa0u;}   // floats
inline constexpr std::uint32_t exponent_bits=0x7f800000u;

namespace world_bounds {
enum class Local {slot};
enum class Access {coordinate,coordinate_store,coordinate_bits,least,most};
// Frame:
//   local(Local)            the slot's address
//   axis(axis)              the coordinate checked next
template<class Memory,class Frame> bool inside(Memory &memory,Frame &frame,std::uint32_t point) {
    using enum Access;
    std::uint32_t least_value=0,most_value=0;
    for(std::uint32_t axis=0;axis<3;++axis) {
        frame.axis(axis);
        const auto value=memory.load32(coordinate,at(point,4u*axis));
        memory.store32(coordinate_store,frame.local(Local::slot),value);
        if((memory.load32(coordinate_bits,frame.local(Local::slot))&exponent_bits)==exponent_bits)return false;
        if(axis==0)least_value=memory.load32(least,globals::least);
        if(sse::below(value,least_value))return false;
        if(axis==0)most_value=memory.load32(most,globals::most);
        if(sse::below(most_value,value))return false;
    }
    return true;
}
} // namespace world_bounds
} // namespace halo2::native_world_bounds
