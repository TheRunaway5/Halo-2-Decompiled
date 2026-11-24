#pragma once
// Native TU1.5 indexed tag reference, entered at 1CEE60 (physics live support
// unit): a tag, or the tag one of its block entries names, resolved to its
// definition.
//
//   indexed_tag::resolve   1CEE60(ECX = reference*) -> EAX definition, or 0
//
// The reference holds a tag handle (+0) and a word index (+4). With the
// index NONE the result is the tag's own definition. Otherwise the index
// picks an entry of the definition's block (count +0x4C, 32-byte entries at
// +0x50); an index past the count (signed), or an entry whose tag (+4) is
// NONE, resolves to 0, and any other to that tag's definition.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_indexed_tag.hpp) maps the role to the original instruction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_indexed_tag {
using native_object_update::at;
using native_object_update::none;
using native_object_update::signed_value;
using native_object_update::widen16;
namespace globals {inline constexpr std::uint32_t tag_instances=0x4f0414u;}   // 16-byte tag instances (+8 the tag's address)
inline constexpr std::uint32_t tag_size=16u,tag_address=8u;
namespace reference {inline constexpr std::uint32_t tag=0u,index=4u;}
namespace block {inline constexpr std::uint32_t count=0x4cu,entries=0x50u,entry_size=32u,entry_tag=4u;}

namespace indexed_tag {
enum class Access {tag,index,tags,definition,count,entries,entry_tag,entry_definition};
enum class Outcome {own,past_count,no_tag,entry};
struct Result {Outcome outcome;std::uint32_t definition;};
template<class Memory> Result resolve(Memory &memory,std::uint32_t reference_address) {
    using enum Access;
    const auto tag_handle=memory.load32(Access::tag,at(reference_address,reference::tag));
    const auto index_word=memory.load16(Access::index,at(reference_address,reference::index))&0xffffu;
    const auto instances=memory.load32(tags,globals::tag_instances);
    const auto owner=memory.load32(definition,tag_size*(tag_handle&0xffffu)+instances+tag_address);
    if(index_word==0xffffu)return {Outcome::own,owner};
    const auto entry_index=widen16(index_word);
    if(signed_value(entry_index)>=signed_value(memory.load32(count,at(owner,block::count))))return {Outcome::past_count,0u};
    const auto entry=block::entry_size*entry_index+memory.load32(entries,at(owner,block::entries));
    const auto entry_handle=memory.load32(Access::entry_tag,at(entry,block::entry_tag));
    if(entry_handle==none)return {Outcome::no_tag,0u};
    return {Outcome::entry,memory.load32(entry_definition,tag_size*(entry_handle&0xffffu)+instances+tag_address)};
}
} // namespace indexed_tag
} // namespace halo2::native_indexed_tag
