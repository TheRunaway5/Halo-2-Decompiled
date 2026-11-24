#pragma once
// Native TU1.5 index marks, entered at 3F3E0 (effect particle frame unit):
// every index a list names is set in a bit table.
//
//   index_marks::mark   3F3E0
//
// The list lives in the state at 556E18: a 16-bit count at +0xA6C and
// 26-byte entries from +0xA6E, each starting with a signed 16-bit index.
// For each entry, bit (index & 31) of the 32-bit word index >> 5
// (arithmetic) in the table at 4D1EE0 is set. The count is read again after
// every entry; a count of zero or less marks nothing.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_index_marks.hpp) maps the role to the original instruction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_index_marks {
using native_object_update::at;
using native_object_update::widen16;

namespace globals {
inline constexpr std::uint32_t state=0x556e18u;            // the list's owner
inline constexpr std::uint32_t marks=0x4d1ee0u;            // uint32 words of bits
}
inline constexpr std::uint32_t list_count=0xa6cu,list_entries=0xa6eu,entry_size=0x1au;

namespace index_marks {
enum class Access {state,count_check,entry_index,mark_word,mark_store,count_reload};
template<class Memory> void mark(Memory &memory) {
    using enum Access;
    const auto owner=memory.load32(state,globals::state);
    if(!(std::int16_t(memory.load16(count_check,at(owner,list_count)))>0))return;
    auto entry=at(owner,list_entries);
    for(std::int32_t position=0;;) {
        const auto index=widen16(memory.load16(entry_index,entry));
        const auto word=globals::marks+std::uint32_t(std::int32_t(index)>>5)*4u;
        entry+=entry_size;
        memory.store32(mark_store,word,memory.load32(mark_word,word)|(1u<<(index&31u)));
        ++position;
        if(!(position<std::int32_t(widen16(memory.load16(count_reload,at(owner,list_count))))))return;
    }
}
} // namespace index_marks
} // namespace halo2::native_index_marks
