#pragma once
// Native TU1.5 slot kinds, entered at 106230 (physics live support unit):
// whether any of an object's slots is of an active kind.
//
//   slot_kinds::any   106230(EAX = object handle) -> AL
//
// The object's tag (+0, through the tag instances at 4F0414) counts its
// slots (+0x2C8); the object's slots are 12-byte records from +0x20C whose
// first byte is the kind. Kinds 3, 7, 5 and 6 are active; kind 2 is active
// when the tag's word +0x294 is not 0. Every slot is looked at; the result
// is whether one was active.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_slot_kinds.hpp) maps the role to the original instruction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_slot_kinds {
using native_object_update::at;
using native_object_update::object_of;
namespace globals {inline constexpr std::uint32_t tag_instances=0x4f0414u;}
inline constexpr std::uint32_t tag_address=8u,instance_shift=4u,slot_count=0x2c8u,slots=0x20cu,slot_size=0xcu,
    conditional_flag=0x294u;
inline constexpr std::uint32_t conditional_kind=2u;
constexpr bool always_active(std::uint32_t kind){return kind==3u || kind==7u || kind==5u || kind==6u;}

namespace slot_kinds {
enum class Access {object_table,object_datums,object_address,object_tag,instances,tag,count,kind,conditional};
// Frame:
//   slot(kind, active)   one slot's comparisons ran
template<class Memory,class Frame> bool any(Memory &memory,Frame &frame,std::uint32_t handle) {
    using enum Access;
    const auto object=object_of(memory,handle,object_table,object_datums,object_address);
    const auto tag_handle=memory.load32(object_tag,object);
    const auto instances_address=memory.load32(instances,globals::tag_instances);
    const auto tag_data=memory.load32(tag,at(((tag_handle&0xffffu)<<instance_shift)+instances_address,tag_address));
    const auto count_value=memory.load32(count,at(tag_data,slot_count));
    bool active=false;
    if(std::int32_t(count_value)<=0)return active;
    for(std::uint32_t k=0;k<count_value;++k) {
        const auto kind_value=memory.load8(kind,at(object,slots+k*slot_size));
        if(always_active(kind_value) ||
           (kind_value==conditional_kind && memory.load16(conditional,at(tag_data,conditional_flag))!=0))active=true;
        frame.slot(kind_value,active);
    }
    return active;
}
} // namespace slot_kinds
} // namespace halo2::native_slot_kinds
