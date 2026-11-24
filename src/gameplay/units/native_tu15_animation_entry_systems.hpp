#pragma once
// Native TU1.5 animation entry value, entered at 1E5A70 (physics live
// support and AI frame units): a value of a definition's animation entry.
//
//   animation_entry::value   1E5A70(ECX = definition; reference) -> EAX, RET 4
//
// The reference's low word, unless NONE, picks the owner: the definition
// of the tag named by entry +4 of the definition's 32-byte block (+0x50,
// its count +0x4C, signed), or 0 past the count or for a NONE tag. Its high
// word (signed), unless -1, picks an entry of the owner's 0x6C-byte block
// (+0x30), otherwise the entry at 0. An entry with bit 4 of byte +0x16 has
// 1262F0's answer for its tag (+0) and the owner's row (+0xB0, 20-byte rows,
// the entry's +0x24) plus its +0x28; any other its +0x38.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_animation_entry.hpp) maps the role to the original
// instruction and owns the ABI of 1262F0; the entry the original keeps in
// ESI across it is a Kept, refreshed from the register it returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_animation_entry {
using native_object_update::at;
using native_object_update::none;
using native_object_update::signed_value;
using native_object_update::widen16;
namespace globals {inline constexpr std::uint32_t tag_instances=0x4f0414u;}   // 16-byte tag instances (+8 the tag's address)
namespace owner {inline constexpr std::uint32_t count=0x4cu,list=0x50u,list_size=32u,list_tag=4u,entries=0x30u,rows=0xb0u;}
namespace entry {inline constexpr std::uint32_t size=0x6cu,tag=0u,flags=0x16u,row=0x24u,offset=0x28u,plain=0x38u;}
inline constexpr std::uint32_t computed_bit=1u<<4,row_size=20u;

namespace animation_entry {
enum class Access {reference,count,list,list_tag,tags,definition,reference_again,entries,flags,row,rows,tag,offset,plain};
struct Kept {std::uint32_t entry;};   // ESI
struct Result {bool computed;std::uint32_t value;};
// Frame: reference_slot() the argument's slot; resolved(owner) the owner
// picked by the low word; lookup(kept, row, tag) 1262F0, refreshes kept,
// returns EAX.
template<class Memory,class Frame> Result value(Memory &memory,Frame &frame,std::uint32_t base) {
    using enum Access;
    const auto low=memory.load32(Access::reference,frame.reference_slot())&0xffffu;
    auto chosen=base;
    if(low!=0xffffu) {
        chosen=0u;
        const auto index=widen16(low);
        if(signed_value(index)<signed_value(memory.load32(count,at(base,owner::count)))) {
            const auto item=owner::list_size*index+memory.load32(list,at(base,owner::list));
            const auto tag_handle=memory.load32(list_tag,at(item,owner::list_tag));
            if(tag_handle!=none) {
                const auto instances=memory.load32(tags,globals::tag_instances);
                chosen=memory.load32(Access::definition,16u*(tag_handle&0xffffu)+instances+8u);
            }
        }
        frame.resolved(chosen);
    }
    const auto high=std::uint32_t(signed_value(memory.load32(reference_again,frame.reference_slot()))>>16);
    Kept kept{0u};
    if(high!=none)kept.entry=entry::size*high+memory.load32(entries,at(chosen,owner::entries));
    if(!(memory.load8(Access::flags,at(kept.entry,entry::flags))&computed_bit))
        return {false,memory.load32(plain,at(kept.entry,entry::plain))};
    const auto row_index=memory.load32(row,at(kept.entry,entry::row));
    const auto row_address=memory.load32(rows,at(chosen,owner::rows))+row_size*row_index;
    const auto answer=frame.lookup(kept,row_address,memory.load32(tag,at(kept.entry,entry::tag)));
    return {true,answer+memory.load32(offset,at(kept.entry,entry::offset))};
}
} // namespace animation_entry
} // namespace halo2::native_animation_entry
