#pragma once
// Native TU1.5 animation wildcard search, entered at 2A0100 (physics live
// support, AI frame and AI actor units): the next entry a graph answers for
// three keys, each of which may fall back to the any key.
//
//   wildcard_search::next   2A0100(ESI = search, EBX = third out; first out, second out) -> EAX, RET 8
//
// A search (graph +0, keys +4/+8/+0xC, stage +0x10, the first and second
// levels' entries +0x14/+0x18) runs through eight stages; a stage's bits 4,
// 2 and 1 put the any key (0x30000D9) in place of the first, second and
// third keys. Each level writes its pass's key to its output and 1E5810
// finds it: the first in the graph's +0x34 (20-byte entries), the second
// in the first's +4 (20 bytes), the third in the second's +4 (0x34 bytes).
// A pass whose any key would repeat a key that already is the any key is
// skipped. The first two levels' entries are kept between passes and
// calls; a level without an entry moves the stage to its next choice and
// drops the entries that choice changes. The search answers the first
// third-level entry found (the stage past it), or none once the stages run
// out. The second key goes to 1E5810 as the MCC assets name it, where the
// unit's copy does so (the frame's class symbol).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_animation_wildcard.hpp) maps the role to the original
// instruction and owns the ABI of the children; the search, the third
// output, the none (zero) it compares with and the entry found, which the
// original keeps in ESI, EBX, EDI and EBP across them, are a Kept,
// refreshed from the registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_animation_wildcard {
using native_object_update::at;
namespace search {inline constexpr std::uint32_t graph=0u,keys=4u,stage=0x10u,first_entry=0x14u,second_entry=0x18u;}
namespace graph {inline constexpr std::uint32_t entries=0x34u;}
inline constexpr std::uint32_t any_key=0x30000d9u,stages=8u,link=4u,key_stride=0x14u,last_stride=0x34u;
enum class Level {first,second,third};
// The stage bit that puts the any key in place of a level's key.
constexpr std::uint32_t any_bit(Level level){return 4u>>unsigned(level);}

namespace wildcard_search {
enum class Access {stage,first_cached,first_found,second_cached,second_found,second_bits,third_bits,key,out_slot,out_store,
    lookup_slot,lookup_key,lookup_table,first_store,second_reset,second_store,advance_read,advance_store,first_clear,second_clear};
enum class Outcome {exhausted,found};
struct Kept {std::uint32_t search,third_out,none,found;};   // ESI, EBX, EDI, EBP
// Frame:
//   slot(level)                             the first or second output's argument
//   choosing(level, any)                    before a level's key
//   class_symbol(key) -> key                the second key as the unit's copy passes it
//   lookup(kept, level, table, key, stride) 1E5810 -> EAX, refreshes kept
//   advancing(level)                        before a level moves the stage
template<class Memory,class Frame> std::uint32_t output(Memory &memory,Frame &frame,const Kept &kept,Level level,Access role) {
    return level==Level::third?kept.third_out:memory.load32(role,frame.slot(level));
}
// The pass's key for a level, written to the level's output: its own key,
// or the any key; false for a pass that would repeat the any key.
template<class Memory,class Frame> bool choose(Memory &memory,Frame &frame,const Kept &kept,Level level,bool any) {
    using enum Access;
    frame.choosing(level,any);
    const auto own=memory.load32(key,at(kept.search,search::keys+4u*unsigned(level)));
    if(any && own==any_key)return false;
    memory.store32(out_store,output(memory,frame,kept,level,out_slot),any?any_key:own);
    return true;
}
// The level's entry for the key its output holds.
template<class Memory,class Frame> std::uint32_t look_up(Memory &memory,Frame &frame,Kept &kept,Level level) {
    using enum Access;
    auto key_value=memory.load32(lookup_key,output(memory,frame,kept,level,lookup_slot));
    const auto table=level==Level::first
        ?memory.load32(lookup_table,at(kept.search,search::graph))+graph::entries
        :memory.load32(lookup_table,at(kept.search,level==Level::second?search::first_entry:search::second_entry))+link;
    if(level==Level::second)key_value=frame.class_symbol(key_value);
    return frame.lookup(kept,level,table,key_value,level==Level::third?last_stride:key_stride);
}
// The stage past the level's choice: the deeper levels' choices restart, and
// the entries kept for a choice that changes are dropped.
template<class Memory,class Frame> void advance(Memory &memory,Frame &frame,const Kept &kept,Level level) {
    using enum Access;
    frame.advancing(level);
    const auto step=any_bit(level);
    const auto next=(memory.load32(advance_read,at(kept.search,search::stage))+step)&~(step-1u);
    memory.store32(advance_store,at(kept.search,search::stage),next);
    switch(level) {
    case Level::third:
        if(next&any_bit(Level::third))return;
        memory.store32(second_clear,at(kept.search,search::second_entry),kept.none);
        if(!(next&any_bit(Level::second)))memory.store32(first_clear,at(kept.search,search::first_entry),kept.none);
        return;
    case Level::second:   // its entry is none already
        if(!(next&any_bit(Level::second)))memory.store32(first_clear,at(kept.search,search::first_entry),kept.none);
        return;
    case Level::first:    // its entry is none already
        memory.store32(second_clear,at(kept.search,search::second_entry),kept.none);
        return;
    }
}
template<class Memory,class Frame> Outcome next(Memory &memory,Frame &frame,Kept &kept) {
    using enum Access;
    do {
        const auto stage_value=memory.load32(stage,at(kept.search,search::stage));
        if(stage_value>=stages)return Outcome::exhausted;
        if(memory.load32(first_cached,at(kept.search,search::first_entry))==kept.none
           && choose(memory,frame,kept,Level::first,stage_value&any_bit(Level::first))) {
            const auto entry=look_up(memory,frame,kept,Level::first);
            memory.store32(first_store,at(kept.search,search::first_entry),entry);
            memory.store32(second_reset,at(kept.search,search::second_entry),kept.none);
        }
        if(memory.load32(first_found,at(kept.search,search::first_entry))==kept.none) {
            advance(memory,frame,kept,Level::first);
            continue;
        }
        if(memory.load32(second_cached,at(kept.search,search::second_entry))==kept.none) {
            if(choose(memory,frame,kept,Level::second,memory.load8(second_bits,at(kept.search,search::stage))&any_bit(Level::second))) {
                const auto entry=look_up(memory,frame,kept,Level::second);
                memory.store32(second_store,at(kept.search,search::second_entry),entry);
            }
            if(memory.load32(second_found,at(kept.search,search::second_entry))==kept.none) {
                advance(memory,frame,kept,Level::second);
                continue;
            }
        }
        if(choose(memory,frame,kept,Level::third,memory.load8(third_bits,at(kept.search,search::stage))&any_bit(Level::third)))
            kept.found=look_up(memory,frame,kept,Level::third);
        advance(memory,frame,kept,Level::third);
    } while(kept.found==kept.none);
    return Outcome::found;
}
} // namespace wildcard_search
} // namespace halo2::native_animation_wildcard
