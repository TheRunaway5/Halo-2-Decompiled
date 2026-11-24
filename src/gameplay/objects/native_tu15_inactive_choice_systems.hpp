#pragma once
// Native TU1.5 inactive choice query, entered at 106040 (physics live support
// unit): whether an object without an active entry has a first choice.
//
//   inactive_choice::chosen   106040(EAX = object handle) -> AL
//
// The choice starts NONE. When 105950 (the object's active entry) answers 0,
// the choice becomes the first index, 0, if the object's definition (its
// tag, +0, through the tag instances) has a nonzero count at +0x1FE, and
// stays NONE otherwise. The result is whether the choice's low word is not
// NONE; the choice is the register the original keeps across 105950, so the
// one it returns counts when it answers.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_inactive_choice.hpp) maps the role to the original
// instruction and owns the ABI of 105950; the definition and the choice the
// original keeps in EDI and ESI across it are a Kept, refreshed from the
// registers it returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_inactive_choice {
using native_object_update::at;
using native_object_update::none;
using native_object_update::widen16;
namespace globals {
inline constexpr std::uint32_t object_headers=0x4ecbd0u;   // DataArray* of 12-byte object headers
inline constexpr std::uint32_t tag_instances=0x4f0414u;    // 16-byte tag instances (+8 the tag's address)
}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u,tag_size=16u,tag_address=8u;
inline constexpr std::uint32_t choice_count=0x1feu,first_choice=0u;

namespace inactive_choice {
enum class Access {headers,header_datums,object_address,tag,tags,definition,count};
struct Kept {std::uint32_t definition,choice;};   // EDI, ESI
// Frame: active(kept, handle) 105950, refreshes kept, returns AL;
// chose(kept, count) the choice made.
template<class Memory,class Frame> bool chosen(Memory &memory,Frame &frame,std::uint32_t handle) {
    using enum Access;
    const auto table=memory.load32(headers,globals::object_headers);
    const auto datums=memory.load32(header_datums,at(table,datum_data));
    const auto object=memory.load32(object_address,datums+header_size*(handle&0xffffu)+header_address);
    const auto tag_handle=memory.load32(Access::tag,object);
    const auto instances=memory.load32(tags,globals::tag_instances);
    Kept kept{memory.load32(Access::definition,tag_size*(tag_handle&0xffffu)+instances+tag_address),none};
    if(frame.active(kept,handle)==0) {
        const auto count_value=widen16(memory.load16(count,at(kept.definition,choice_count)));
        kept.choice=count_value!=0?first_choice:none;
        frame.chose(kept,count_value);
    }
    return (kept.choice&0xffffu)!=0xffffu;
}
} // namespace inactive_choice
} // namespace halo2::native_inactive_choice
