#pragma once
// Native TU1.5 light gate, entered at 31500 (light portal unit): whether a
// light passes its definition's portal gate.
//
//   light_gate::passes   31500(EAX = light handle) -> AL
//
// A light (0x110-byte datums of the data array at [4ECBDC]) is inside its
// portal when it has one (index +0x54, NONE for none) and 3E9B0 (ESI its
// source +0x4C, EAX the light) says so. Its definition (the tag instances
// at [4F0414], 16 bytes, +8; by the light's tag +4) may need it inside
// (flags +0 bit 5) or outside (bit 6). A light NONE does not pass.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_light_gate.hpp) maps the role to the original instruction and
// owns the ABI of the child; the definition, which the original keeps in EDI
// across it, is refreshed from the register it returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_light_gate {
using native_object_update::at;
using native_object_update::none;
namespace globals {
inline constexpr std::uint32_t lights=0x4ecbdcu;   // DataArray* of 0x110-byte lights
inline constexpr std::uint32_t tags=0x4f0414u;     // 16-byte tag instances (+8 the definition)
}
inline constexpr std::uint32_t datum_data=0x44u,light_size=0x110u,tag_size=16u,tag_definition=8u,no_index=0xffffu;
namespace light {inline constexpr std::uint32_t tag=4u,source=0x4cu,portal=0x54u;}
namespace flag {inline constexpr std::uint32_t needs_inside=0x20u,needs_outside=0x40u;}

namespace light_gate {
enum class Access {lights,data,tags,tag,definition,portal,source,flags};
enum class Outcome {none_light,passes,blocked};
// Frame: inside(definition&, source, light) -> AL   3E9B0, refreshes the definition
template<class Memory,class Frame> Outcome passes(Memory &memory,Frame &frame,std::uint32_t handle) {
    using enum Access;
    if(handle==none)return Outcome::none_light;
    const auto light_array=memory.load32(Access::lights,globals::lights);
    const auto data_value=memory.load32(data,at(light_array,datum_data));
    const auto tag_array=memory.load32(Access::tags,globals::tags);
    const auto light_address=light_size*(handle&0xffffu)+data_value;
    const auto tag_handle=memory.load32(tag,at(light_address,light::tag));
    auto definition_address=memory.load32(definition,tag_array+tag_size*(tag_handle&0xffffu)+tag_definition);
    bool inside=false;
    if(memory.load16(portal,at(light_address,light::portal))!=no_index) {
        const auto answer=frame.inside(definition_address,memory.load32(source,at(light_address,light::source)),light_address);
        inside=answer&0xffu;
    }
    const auto flags_value=memory.load32(flags,definition_address);
    if((flags_value&flag::needs_inside) && !inside)return Outcome::blocked;
    if((flags_value&flag::needs_outside) && inside)return Outcome::blocked;
    return Outcome::passes;
}
} // namespace light_gate
} // namespace halo2::native_light_gate
