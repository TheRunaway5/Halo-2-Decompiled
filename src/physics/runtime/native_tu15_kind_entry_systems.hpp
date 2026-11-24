#pragma once
// Native TU1.5 kind entry, entered at 11E060 (physics live support unit):
// the 16-byte entry of an owner's list that a kind selects.
//
//   kind_entry::of   11E060(ESI = owner, EDX = kind) -> EAX
//
// Kinds 0 to 3 select an index through the original's jump table (11E09C:
// entry 0 or entry 1); any other kind selects entry 0. An index within the
// owner's count (+0x2A8) gives that entry of its list (+0x2AC); past it,
// the list's first entry when the list is not empty, and zero when it is.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_kind_entry.hpp) maps the role to the original instruction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_kind_entry {
using native_object_update::at;
using native_object_update::signed_value;
namespace globals {inline constexpr std::uint32_t kind_table=0x11e09cu;}   // the original jump table, by kind
inline constexpr std::uint32_t table_kinds=4u,entry_size=16u;
// The table's targets, as the unit's canonical_branch names them.
inline constexpr std::uint32_t first_case=0x11e070u,second_case=0x11e074u;
namespace owner {inline constexpr std::uint32_t count=0x2a8u,entries=0x2acu;}

namespace kind_entry {
enum class Access {kind_case,count,entries,first_entry};
enum class Outcome {selected,first,empty};
struct Result {std::uint32_t entry;Outcome outcome;std::uint32_t index;};
// Frame: selected(index) after the table, unknown_case() (throws).
template<class Memory,class Frame> Result of(Memory &memory,Frame &frame,std::uint32_t owner_address,std::uint32_t kind) {
    using enum Access;
    std::uint32_t index=0;
    if(kind<table_kinds) {
        switch(memory.load32(kind_case,globals::kind_table+4u*kind)) {
        case first_case:index=0;break;
        case second_case:index=1;break;
        default:frame.unknown_case();
        }
        frame.selected(index);
    }
    const auto count_value=memory.load32(count,at(owner_address,owner::count));
    if(signed_value(index)<signed_value(count_value))
        return {memory.load32(entries,at(owner_address,owner::entries))+entry_size*index,Outcome::selected,index};
    if(signed_value(count_value)>0)return {memory.load32(first_entry,at(owner_address,owner::entries)),Outcome::first,index};
    return {0u,Outcome::empty,index};
}
} // namespace kind_entry
} // namespace halo2::native_kind_entry
