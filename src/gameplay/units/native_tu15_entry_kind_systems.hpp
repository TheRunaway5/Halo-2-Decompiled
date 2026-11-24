#pragma once
// Native TU1.5 entry kind query, entered at 1062A0 (combat action unit):
// whether an object has a kind-5 entry whose state is clear.
//
//   entry_kind::any   1062A0(EAX = object handle) -> AL
//
// The object's definition (its tag, +0, through the tag instances) lists
// entries of 0x40 bytes (+0x2CC, their count +0x2C8), each with a kind word
// at +6; the object keeps one 12-byte state per entry from +0x20C, its
// first byte clear when the entry is idle. The result is whether any idle
// entry is of kind 5; every entry is looked at.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_entry_kind.hpp) maps the role to the original instruction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_entry_kind {
using native_object_update::at;
using native_object_update::signed_value;
namespace globals {
inline constexpr std::uint32_t object_headers=0x4ecbd0u;   // DataArray* of 12-byte object headers
inline constexpr std::uint32_t tag_instances=0x4f0414u;    // 16-byte tag instances (+8 the tag's address)
}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u,tag_size=16u,tag_address=8u;
namespace definition {inline constexpr std::uint32_t count=0x2c8u,entries=0x2ccu,entry_size=0x40u,kind=6u;}
inline constexpr std::uint32_t states=0x20cu,state_size=12u,wanted_kind=5u;

namespace entry_kind {
enum class Access {headers,header_datums,object_address,tag,tags,tag_definition,count,state,entries,kind};
// Frame: walking() enters the walk; stepped(state, offset, remaining) the
// step past an entry.
template<class Memory,class Frame> bool any(Memory &memory,Frame &frame,std::uint32_t handle) {
    using enum Access;
    const auto table=memory.load32(headers,globals::object_headers);
    const auto datums=memory.load32(header_datums,at(table,datum_data));
    const auto object=memory.load32(object_address,datums+header_size*(handle&0xffffu)+header_address);
    const auto tag_handle=memory.load32(Access::tag,object);
    const auto instances=memory.load32(tags,globals::tag_instances);
    const auto owner=memory.load32(tag_definition,tag_size*(tag_handle&0xffffu)+instances+tag_address);
    const auto count_value=memory.load32(Access::count,at(owner,definition::count));
    if(signed_value(count_value)<=0)return false;
    frame.walking();
    bool found=false;
    std::uint32_t state_address=object+states,offset=0;
    for(auto remaining=count_value;remaining!=0;--remaining) {
        if((memory.load8(state,state_address)&0xffu)==0) {
            const auto list=memory.load32(entries,at(owner,definition::entries));
            if((memory.load16(kind,offset+list+definition::kind)&0xffffu)==wanted_kind)found=true;
        }
        frame.stepped(state_address,offset,remaining);
        state_address+=state_size;offset+=definition::entry_size;
    }
    return found;
}
} // namespace entry_kind
} // namespace halo2::native_entry_kind
