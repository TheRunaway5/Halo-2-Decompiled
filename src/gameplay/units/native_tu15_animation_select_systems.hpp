#pragma once
// Native TU1.5 animation select, entered at 1E5E40 (physics live support, AI
// frame and AI actor units): an entry of a graph found by three keys, its
// items refreshed.
//
//   animation_select::select   1E5E40(BL = forced; graph, a, b, c, flag) -> RET 14
//
// Unless neither BL nor the flag's low byte is set, 1E5810 finds the
// graph's entry for a (in the graph's +0x34, 20-byte entries), that one's
// for b (in its +4, 20 bytes) and that one's for c (in its +4, 0x34 bytes);
// a missing one ends the select. With BL set, each item the found entry
// lists in +0x28 (its count +0x24) that is in use (byte +0x10 of the
// graph's 20-byte items at +0xB0) goes to 126200 with CL 1; with the flag
// set, those listed in +0x30 (count +0x2C) with CL 0. The counts are read
// again after each item.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_animation_select.hpp) maps the role to the original
// instruction and owns the ABI of the children; the graph, the entry, the
// list index and BL the original keeps in EBP, ESI, EDI and BL across them
// are a Kept, refreshed from the registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_animation_select {
using native_object_update::at;
using native_object_update::signed_value;
namespace graph {inline constexpr std::uint32_t entries=0x34u,items=0xb0u,item_size=20u,item_in_use=0x10u;}
inline constexpr std::uint32_t link=4u,key_stride=0x14u,last_stride=0x34u;
struct List {std::uint32_t count,items,refresh;};
inline constexpr List lists[]{{0x24u,0x28u,1u},{0x2cu,0x30u,0u}};

namespace animation_select {
enum class Access {graph,flag,first_key,second_key,third_key,count,list,index,items,in_use,count_again,off_flag};
enum class Outcome {skipped,unmatched,found};
struct Kept {std::uint32_t graph,entry,index,forced;};   // EBP, ESI, EDI, BL
// Frame: argument_slot(k); lookup(kept, k, table, key, stride) 1E5810,
// refreshes kept, returns EAX; saved(k) the pushes of ESI and EDI;
// listing(which) before a list; refresh(kept, item, which) 126200,
// refreshes kept; stepped(kept, count) after each item.
template<class Memory,class Frame> Outcome select(Memory &memory,Frame &frame,std::uint32_t forced) {
    using enum Access;
    Kept kept{memory.load32(Access::graph,frame.argument_slot(0)),0u,0u,forced&0xffu};
    if(!kept.forced && !(memory.load8(flag,frame.argument_slot(4))&0xffu))return Outcome::skipped;
    auto found=frame.lookup(kept,0u,at(kept.graph,graph::entries),memory.load32(first_key,frame.argument_slot(1)),key_stride);
    if(!found)return Outcome::unmatched;
    found=frame.lookup(kept,1u,found+link,memory.load32(second_key,frame.argument_slot(2)),key_stride);
    if(!found)return Outcome::unmatched;
    const auto third=memory.load32(third_key,frame.argument_slot(3));
    frame.saved(0u);
    kept.entry=frame.lookup(kept,2u,found+link,third,last_stride);
    if(!kept.entry)return Outcome::unmatched;
    frame.saved(1u);
    for(std::uint32_t which=0;which<2u;++which) {
        if(which==0 ? !kept.forced : !(memory.load8(off_flag,frame.argument_slot(4))&0xffu))continue;
        const auto &l=lists[which];
        frame.listing(which);
        auto count_value=memory.load32(count,at(kept.entry,l.count));
        if(signed_value(count_value)<=0)continue;
        kept.index=0u;
        do {
            const auto index_value=memory.load32(index,memory.load32(list,at(kept.entry,l.items))+4u*kept.index);
            const auto item=memory.load32(items,at(kept.graph,graph::items))+graph::item_size*index_value;
            if(memory.load8(in_use,at(item,graph::item_in_use))&0xffu)frame.refresh(kept,item,which);
            count_value=memory.load32(count_again,at(kept.entry,l.count));
            ++kept.index;
            frame.stepped(kept,count_value);
        } while(signed_value(kept.index)<signed_value(count_value));
    }
    return Outcome::found;
}
} // namespace animation_select
} // namespace halo2::native_animation_select
