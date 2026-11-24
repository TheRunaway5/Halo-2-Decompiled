#pragma once
// Native TU1.5 active entry, entered at 105950 (physics live support unit):
// whether an object's entry, or any of them, is in an active state.
//
//   active_entry::any   105950(EAX = object handle, EBX = entry or NONE) -> AL
//
// The object's tag (its definition) counts its entries (+0x2C0); the object
// holds each entry's state as a signed word at +0x224 (16 bytes apiece). An
// entry is active in states 1 to 6. With EBX NONE every entry is looked at,
// otherwise only the entry EBX names.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_active_entry.hpp) maps the role to the original instruction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_active_entry {
using native_object_update::at;
using native_object_update::none;
using native_object_update::signed_value;
using native_object_update::widen16;
namespace globals {
inline constexpr std::uint32_t object_headers=0x4ecbd0u;   // DataArray* of 12-byte object headers
inline constexpr std::uint32_t tag_instances=0x4f0414u;    // 16-byte tag instances (+8 the tag's address)
}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u,tag_instance_shift=4u,
    instance_address=8u;
inline constexpr std::uint32_t entry_count=0x2c0u,entry_states=0x224u,entry_shift=4u,last_active=6u;

namespace active_entry {
enum class Access {headers,header_datums,instances,object_address,object_tag,definition,count,state};
// Frame: looked_at(index) at each entry looked at, stepped(index) past each entry.
template<class Memory,class Frame> bool any(Memory &memory,Frame &frame,std::uint32_t handle,std::uint32_t entry) {
    using enum Access;
    const auto table=memory.load32(headers,globals::object_headers);
    const auto datums=memory.load32(header_datums,at(table,datum_data));
    const auto instance_table=memory.load32(instances,globals::tag_instances);
    const auto address=memory.load32(object_address,datums+header_size*(handle&0xffffu)+header_address);
    const auto tag_handle=memory.load32(object_tag,address);
    const auto tag=memory.load32(definition,((tag_handle&0xffffu)<<tag_instance_shift)+instance_table+instance_address);
    const auto count_value=signed_value(memory.load32(count,at(tag,entry_count)));
    for(std::int32_t index=0;index<count_value;++index) {
        if(entry==none || entry==std::uint32_t(index)) {
            frame.looked_at(std::uint32_t(index));
            const auto offset=(widen16(std::uint32_t(index)&0xffffu)<<entry_shift)+entry_states;
            const auto state_value=signed_value(widen16(memory.load16(state,address+offset)));
            if(state_value>0 && state_value<=std::int32_t(last_active))return true;
        }
        frame.stepped(std::uint32_t(index));
    }
    return false;
}
} // namespace active_entry
} // namespace halo2::native_active_entry
