#pragma once
// Native TU1.5 flagged slot, entered at D2330 (weapon HUD unit): the first
// of an object's four slots holding an object whose tag carries flag bit 3.
//
//   flagged_slot::find   D2330(ECX = object handle) -> AX
//
// The four slot handles start at +0x218; an empty slot (NONE) is passed
// over. A slot's object's tag (its definition, through the tag instances
// [4F0414]) is flagged by bit 3 of +0x12C. The result is the slot index, or
// NONE (0xFFFF) when no slot is flagged.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_flagged_slot.hpp) maps the role to the original instruction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_flagged_slot {
using native_object_update::at;
using native_object_update::none;
namespace globals {
inline constexpr std::uint32_t object_headers=0x4ecbd0u;   // DataArray* of 12-byte object headers
inline constexpr std::uint32_t tag_instances=0x4f0414u;    // 16-byte tag instances (+8 the tag's address)
}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u,tag_instance_shift=4u,
    instance_address=8u;
inline constexpr std::uint32_t slots=0x218u,slot_count=4u,tag_flags=0x12cu,flag_bit=3u;
inline constexpr std::uint32_t not_found=0xffffu;

namespace flagged_slot {
enum class Access {headers,header_datums,object_address,instances,slot,slot_object,slot_tag,slot_definition,flags};
// Frame: slot_reached(index) at each slot, flags_read(value) for a filled slot's flags.
template<class Memory,class Frame> std::uint32_t find(Memory &memory,Frame &frame,std::uint32_t handle) {
    using enum Access;
    const auto table=memory.load32(headers,globals::object_headers);
    const auto datums=memory.load32(header_datums,at(table,datum_data));
    const auto object_at=[&](Access role,std::uint32_t object_handle) {
        return memory.load32(role,datums+header_size*(object_handle&0xffffu)+header_address);
    };
    const auto address=object_at(object_address,handle);
    const auto instance_table=memory.load32(instances,globals::tag_instances);
    for(std::uint32_t index=0;index<slot_count;++index) {
        frame.slot_reached(index);
        const auto slot_handle=memory.load32(slot,at(address,slots+4u*index));
        if(slot_handle==none)continue;
        const auto tag_handle=memory.load32(slot_tag,object_at(slot_object,slot_handle));
        const auto definition=memory.load32(slot_definition,((tag_handle&0xffffu)<<tag_instance_shift)+instance_table+instance_address);
        if((memory.load32(flags,at(definition,tag_flags))>>flag_bit)&1u)return index;
    }
    return not_found;
}
} // namespace flagged_slot
} // namespace halo2::native_flagged_slot
