#pragma once
// Native TU1.5 entry kind check, entered at 1D9F50 (physics live support
// unit): whether a list's current entry names an available object.
//
//   entry_kind::available   1D9F50(EDI = list) -> AL
//
// The list's current index is its signed byte +0x18, valid when not
// negative and below the count +0x74; the entries are 0x60 bytes from +0x70
// and name an object at +0x40. The object is available unless its interface
// (+0x3C; the virtual function at +0x18 of its table) reports kind 6, or its
// byte +0x40 is set.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_entry_kind.hpp) maps the role to the original instruction
// and owns the ABI of the virtual call.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_entry_kind {
using native_object_update::at;
using native_object_update::none;
inline constexpr std::uint32_t list_index=0x18u,list_entries=0x70u,list_count=0x74u,entry_size=0x60u,entry_object=0x40u,
    object_interface=0x3cu,interface_kind=0x18u,object_busy=0x40u;
inline constexpr std::uint32_t unavailable_kind=6u;

namespace entry_kind {
enum class Access {index,count,entries,object,interface,table,kind_entry,entries_again,object_again,busy};
enum class Exit {no_entry,unavailable,available};
// The values the original keeps in callee-saved registers.
struct Kept {std::uint32_t list;std::uint32_t entry;};   // EDI; ESI: the entry's offset
// Frame:
//   indexed(index)            the index (AX, sign-extended) is decided
//   kind(entry, kept)         the virtual kind function at that entry (ECX
//                             the interface); returns EAX, refreshes kept
template<class Memory,class Frame> Exit available(Memory &memory,Frame &frame,std::uint32_t list) {
    using enum Access;
    const auto index_byte=memory.load8(index,at(list,list_index));
    std::uint32_t entry_index=none;
    if(!(index_byte&0x80u) && std::int32_t(std::int8_t(index_byte))<std::int32_t(memory.load32(count,at(list,list_count))))
        entry_index=std::uint32_t(std::int32_t(std::int8_t(index_byte)));
    frame.indexed(entry_index);
    if(entry_index==none)return Exit::no_entry;
    const auto entries_address=memory.load32(entries,at(list,list_entries));
    Kept kept{list,entry_index*entry_size};
    const auto object_data=memory.load32(object,at(kept.entry+entries_address,entry_object));
    const auto interface_address=memory.load32(interface,at(object_data,object_interface));
    const auto table_address=memory.load32(table,interface_address);
    if(frame.kind(memory.load32(kind_entry,at(table_address,interface_kind)),kept)==unavailable_kind)return Exit::unavailable;
    const auto entries_now=memory.load32(entries_again,at(kept.list,list_entries));
    const auto object_now=memory.load32(object_again,at(kept.entry+entries_now,entry_object));
    return memory.load8(busy,at(object_now,object_busy))?Exit::unavailable:Exit::available;
}
} // namespace entry_kind
} // namespace halo2::native_entry_kind
