#pragma once
// Native TU1.5 player mark counters, entered at 165860 (multiplayer engines
// unit): for each local player, how long each other player has been marked.
//
//   player_marks::update   165860()
//
// The counters are 16-bit, sixteen per local player slot, at 4F6408. For
// each local player slot with a player (the handles at [4F54F0]+0xC, in
// slot order), every player of the players array (4F54F4, walked by the
// filtered iterator 1A8E50 through an iterator on the stack) other than the
// slot's own, whose word +0x28 is NONE, is counted when the marks apply
// (the bytes 4D61D8 and 48377C, and 53780) and the player's bit is set in
// the slot player's marks (the words at 535DA0 and 535DE0 for its index,
// when 535CD8 enables them): its counter goes up by one. The counters of
// the players not counted this time go back to zero. Finally the counters
// of the slots that had no player are cleared.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_player_marks.hpp) maps the role to the original instruction
// and owns the ABI of the children (1A8E50, 53780). The values the original
// keeps in callee-saved registers across them are a Kept, refreshed from
// the registers each child returned.
#include "physics/runtime/native_tu15_filtered_iterator_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_player_marks {
using native_object_update::at;
using native_object_update::none;
using native_filtered_iterator::Iterator;

namespace globals {
inline constexpr std::uint32_t players=0x4f54f0u;          // +0xC: the four local player handles
inline constexpr std::uint32_t player_array=0x4f54f4u;     // DataArray* of the players
inline constexpr std::uint32_t marks_apply=0x4d61d8u,marks_ready=0x48377cu;   // bytes
inline constexpr std::uint32_t marks_enabled=0x535cd8u;    // byte
inline constexpr std::uint32_t first_marks=0x535da0u,second_marks=0x535de0u;  // uint32[player index]
inline constexpr std::uint32_t counters=0x4f6408u;         // uint16[4][16]
}
inline constexpr std::uint32_t local_handles=0xcu,slot_count=4u,counters_per_slot=16u,player_word=0x28u,
    no_word=0xffffu;

namespace player_marks {
// The locals of the 0x18-byte frame: the slots processed, the players
// counted for the current slot, and the iterator.
enum class Local {processed,counted,iterator};
enum class Access {
    players,processed_init,first_probe,slot_player,player_array,iterator_index,iterator_handle,counted_init,
    iterator_array,other_handle,other_record,other_word,marks_apply,marks_ready,marks_enabled,first_marks,
    second_marks,counter_read,counter_store,counted_read,counted_store,players_again,counted_final,counter_reset,
    processed_read,processed_store,next_probe,processed_final,counter_clear
};
// The values the original keeps in callee-saved registers.
// EDI holds the player globals between the walks and the slot player's
// index within one; ESI the other player's index within a walk.
struct Kept {
    std::uint32_t globals;       // EDI: the player globals
    std::uint32_t own_index;     // EDI: the slot player's index
    std::uint32_t slot;          // EBP: the local player slot
    std::uint32_t player;        // EBX: the slot's player handle
    std::uint32_t other_index;   // ESI: the other player's index
};

// Frame:
//   local(Local)            a local's address now
//   first_player(kept)      1A8E50(ESI the iterator) as the walk starts;
//   next_player(kept)       and for each next player; returns AL,
//                           refreshes kept
//   marks_allowed(kept)     53780 (ESI the other player's index, EDI the
//                           slot player's); returns AL, refreshes kept
//   marks_tested(enabled)   the mark bit's test (EAX, ECX, EDX)
//   next_search(start)      the next slot search starts (EAX, ECX, EDX)
//   found(slot)             a slot search found a player (EDX)
template<class Memory,class Frame> void update(Memory &memory,Frame &frame) {
    using enum Access;
    const auto local=[&](Local l){return frame.local(l);};
    const auto iterator=[&](std::size_t field){return at(local(Local::iterator),field);};
    Kept kept{};
    kept.globals=memory.load32(players,globals::players);
    memory.store32(processed_init,local(Local::processed),0u);
    // The first local slot with a player.
    std::uint32_t slot=none;
    for(std::uint32_t k=0;k<slot_count;++k)
        if(memory.load32(first_probe,at(kept.globals,local_handles+4u*k))!=none){slot=k;frame.found(k);break;}
    kept.slot=slot;
    while(kept.slot!=none) {
        kept.player=memory.load32(slot_player,at(kept.globals,local_handles+4u*kept.slot));
        if(kept.player!=none) {
            // Walk the players.
            const auto array=memory.load32(player_array,globals::player_array);
            memory.store32(iterator_index,iterator(offsetof(Iterator,index)),none);
            memory.store32(iterator_handle,iterator(offsetof(Iterator,handle)),none);
            memory.store32(counted_init,local(Local::counted),0u);
            memory.store32(iterator_array,iterator(offsetof(Iterator,array)),array);
            if(frame.first_player(kept)&0xffu) {
                do {
                    const auto other=memory.load32(other_handle,iterator(offsetof(Iterator,handle)));
                    if(other==kept.player)continue;
                    const auto record=memory.load32(other_record,iterator(offsetof(Iterator,record)));
                    if(memory.load16(other_word,at(record,player_word))!=no_word)continue;
                    if(!memory.load8(marks_apply,globals::marks_apply))continue;
                    kept.other_index=other&0xffffu;kept.own_index=kept.player&0xffffu;
                    if(!memory.load8(marks_ready,globals::marks_ready))continue;
                    if(!(frame.marks_allowed(kept)&0xffu))continue;
                    const bool enabled=memory.load8(marks_enabled,globals::marks_enabled)!=0;
                    const auto first=enabled?memory.load32(first_marks,globals::first_marks+4u*kept.own_index):0u;
                    const auto second=enabled?memory.load32(second_marks,globals::second_marks+4u*kept.own_index):0u;
                    const auto bit=1u<<(kept.other_index&31u);
                    frame.marks_tested(enabled);
                    if(!((first|second)&bit))continue;
                    const auto counter=globals::counters+2u*(kept.slot*counters_per_slot+kept.other_index);
                    memory.store16(counter_store,counter,memory.load16(counter_read,counter)+1u);
                    memory.store32(counted_store,local(Local::counted),memory.load32(counted_read,local(Local::counted))|bit);
                } while(frame.next_player(kept)&0xffu);
                kept.globals=memory.load32(players_again,globals::players);
            }
            // The players not counted start again.
            const auto counted=memory.load16(counted_final,local(Local::counted));
            for(std::uint32_t k=0;k<counters_per_slot;++k)
                if(!(counted&(1u<<k)))memory.store16(counter_reset,globals::counters+2u*(kept.slot*counters_per_slot+k),0u);
            memory.store32(processed_store,local(Local::processed),
                           memory.load32(processed_read,local(Local::processed))|(1u<<(kept.slot&31u)));
        }
        // The next local slot with a player.
        const auto start=kept.slot==none?0u:kept.slot+1u;
        slot=none;
        frame.next_search(start);
        for(auto k=start;std::int32_t(k)<std::int32_t(slot_count);++k)
            if(memory.load32(next_probe,at(kept.globals,local_handles+4u*k))!=none){slot=k;frame.found(k);break;}
        kept.slot=slot;
    }
    // The counters of the slots without a player are cleared.
    const auto processed=memory.load16(processed_final,local(Local::processed));
    for(std::uint32_t k=0;k<slot_count;++k)
        if(!(processed&(1u<<k)))
            for(std::uint32_t word=0;word<counters_per_slot/2u;++word)
                memory.store32(counter_clear,globals::counters+k*2u*counters_per_slot+4u*word,0u);
}
} // namespace player_marks
} // namespace halo2::native_player_marks
