#pragma once
// Native TU1.5 respawn wait, entered at 1601B0 (physics live support unit):
// whether a dead player waits for a slot among the living.
//
//   respawn_wait::decide   1601B0(EAX = player) -> AL
//
// A game with a living limit (+0x1B4 of [4F3218], above 0) and a player
// (0x21C bytes, through the players at [4F54F4]) without a unit (+0x2C NONE)
// counts, through the player iterator (1A8E50, its state on the stack), the
// other players with units: in a team game (the engine [56DFA0 + 4 * the
// index +0xC14 of [4F63B8]] present and bit 0 of the variant flags +0x184)
// only the player's teammates (byte +0xC0). With the limit reached the player
// waits. Otherwise the other dead players (teammates as before) that 160570
// does not set aside and whose order (+0x1B4) is below the player's (or equal
// with a lower index) are ahead of it; with as many ahead as slots free the
// player waits. Without a limit, or alive, it does not.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the original's frame (the iterator, the
// counts, the player's index and the team flag); the boundary adapter
// (native_tu15_respawn_wait.hpp) maps the role to the original instruction
// and owns the ABI of the children. The values the original keeps in
// callee-saved registers across them are a Kept, refreshed from the
// registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_respawn_wait {
using native_object_update::at;
using native_object_update::none;
using native_object_update::signed_value;
namespace globals {inline constexpr std::uint32_t game=0x4f3218u,players=0x4f54f4u,engine_globals=0x4f63b8u,engines=0x56dfa0u;}
inline constexpr std::uint32_t datum_data=0x44u,player_size=0x21cu;
namespace game {inline constexpr std::uint32_t limit=0x1b4u,variant_flags=0x184u;inline constexpr std::uint32_t teams=1u;}
namespace engine_globals {inline constexpr std::uint32_t index=0xc14u;}
namespace player {inline constexpr std::uint32_t unit=0x2cu,team=0xc0u,order=0x1b4u;}
// The iterator's state (1A8E50): the current player, the array, its handle and index.
namespace iterator {inline constexpr std::uint32_t current=0u,array=4u,handle=8u,index=0xcu;}
// The frame's slots, by offset from the return address.
namespace local {inline constexpr std::int32_t teams=-0x1d,ahead=-0x1c,own_index=-0x18,free=-0x14,state=-0x10;}

namespace respawn_wait {
enum class Access {game,limit,players_array,players_data,index_store,unit,array_store,iterator_index,iterator_handle,engine_globals,
    engine_index,engine,current,game_again,variant_flags,teams_store,own_team,other_team,other_unit,game_final,limit_final,
    free_store,ahead_clear,handle,other_order,own_order,own_index,ahead_load,ahead_store,free_load,ahead_final};
enum class Outcome {unlimited,alive,full,queued,admitted};
// The original's EBX (the players, then NONE), EBP (the living, then a handle), ESI (the iterator, then the current
// player) and EDI (the player's address) across the children.
struct Kept {std::uint32_t held,count,current,own;};
// Frame:
//   local(offset), saving(n)                     the frame's slots now; PUSH EBX, ESI, EDI (MOV EDI,EAX; AND), EBP
//   limit_tested(index, data, limit)             IMUL, ADD, XOR AL, TEST EDX
//   unit_tested(unit), next(pass, kept) -> AL    OR ECX,-1; CMP; 1A8E50(ESI = the iterator) refreshing kept
//   engine_tested(engine), teams_tested(teams), teams_compared(own, other), unit_compared(unit, against),
//   self_compared(current, own), counted(kept), full_tested(limit, living), freeing(limit, kept)
//   set_aside(handle, kept) -> AL                160570(ECX = handle), refreshes kept
//   orders_compared(other, own), indices_compared(own index, handle), incremented(ahead), ahead_compared(ahead, free)
template<class Memory,class Frame> std::uint32_t decide(Memory &memory,Frame &frame,std::uint32_t player_handle,Outcome &outcome) {
    using enum Access;
    Kept kept{};
    const auto game_address=memory.load32(Access::game,globals::game);
    const auto limit_value=memory.load32(limit,at(game_address,game::limit));
    frame.saving(0u);
    kept.held=memory.load32(players_array,globals::players);
    frame.saving(1u);
    const auto data=memory.load32(players_data,at(kept.held,datum_data));
    frame.saving(2u);
    const auto own_index_value=player_handle&0xffffu;
    memory.store32(index_store,frame.local(local::own_index),own_index_value);
    kept.own=own_index_value*player_size+data;
    frame.limit_tested(own_index_value,data,limit_value);
    if(signed_value(limit_value)<=0){outcome=Outcome::unlimited;return 0u;}
    const auto unit_value=memory.load32(Access::unit,at(kept.own,player::unit));
    frame.unit_tested(unit_value);
    if(unit_value!=none){outcome=Outcome::alive;return 0u;}
    frame.saving(3u);
    kept.count=0u;
    // The players in the counts: in a team game, the player's teammates.
    const auto considered=[&]() {
        const auto engine_value=memory.load32(engine,globals::engines+4u*memory.load32(engine_index,
            at(memory.load32(Access::engine_globals,globals::engine_globals),engine_globals::index)));
        frame.engine_tested(engine_value);
        const auto current_address=memory.load32(current,frame.local(local::state)+iterator::current);
        bool same_side=true;
        if(engine_value) {
            const auto teams=memory.load8(variant_flags,at(memory.load32(game_again,globals::game),game::variant_flags))&game::teams;
            frame.teams_tested(teams);
            memory.store8(teams_store,frame.local(local::teams),teams);
            if(teams) {
                const auto own=memory.load8(own_team,at(kept.own,player::team));
                const auto other=memory.load8(other_team,at(current_address,player::team));
                frame.teams_compared(own,other);
                same_side=own==other;
            }
        }
        return std::pair{same_side,current_address};
    };
    // The living.
    memory.store32(array_store,frame.local(local::state)+iterator::array,kept.held);
    memory.store32(iterator_index,frame.local(local::state)+iterator::index,none);
    memory.store32(iterator_handle,frame.local(local::state)+iterator::handle,none);
    for(auto more=frame.next(0u,kept);more&0xffu;more=frame.next(0u,kept)) {
        const auto [same_side,current_address]=considered();
        if(!same_side)continue;
        const auto other_unit=memory.load32(Access::other_unit,at(current_address,player::unit));
        frame.unit_compared(other_unit,none);
        if(other_unit==none)continue;
        frame.self_compared(current_address,kept.own);
        if(current_address==kept.own)continue;
        ++kept.count;
        frame.counted(kept);
    }
    const auto limit_now=memory.load32(limit_final,at(memory.load32(game_final,globals::game),game::limit));
    frame.full_tested(limit_now,kept.count);
    if(signed_value(limit_now)<=signed_value(kept.count)){outcome=Outcome::full;return 1u;}
    // The dead ahead of the player.
    memory.store32(array_store,frame.local(local::state)+iterator::array,kept.held);
    const auto free_value=limit_now-kept.count;
    kept.held=none;
    frame.freeing(limit_now,kept);
    memory.store32(free_store,frame.local(local::free),free_value);
    memory.store32(ahead_clear,frame.local(local::ahead),0u);
    memory.store32(iterator_index,frame.local(local::state)+iterator::index,kept.held);
    memory.store32(iterator_handle,frame.local(local::state)+iterator::handle,kept.held);
    for(auto more=frame.next(1u,kept);more&0xffu;more=frame.next(1u,kept)) {
        const auto [same_side,current_address]=considered();
        if(!same_side)continue;
        kept.current=current_address;
        const auto other_unit=memory.load32(Access::other_unit,at(kept.current,player::unit));
        frame.unit_compared(other_unit,kept.held);
        if(other_unit!=kept.held)continue;
        frame.self_compared(kept.current,kept.own);
        if(kept.current==kept.own)continue;
        kept.count=memory.load32(handle,frame.local(local::state)+iterator::handle);
        if(frame.set_aside(kept.count,kept)&0xffu)continue;
        const auto other_order=memory.load32(Access::other_order,at(kept.current,player::order));
        const auto own_order=memory.load32(Access::own_order,at(kept.own,player::order));
        frame.orders_compared(other_order,own_order);
        if(signed_value(other_order)>signed_value(own_order))continue;
        if(other_order==own_order) {
            const auto own_index_now=memory.load32(own_index,frame.local(local::own_index));
            frame.indices_compared(own_index_now,kept.count);
            if(signed_value(own_index_now)<signed_value(kept.count&0xffffu))continue;
        }
        memory.store32(ahead_store,frame.local(local::ahead),frame.incremented(memory.load32(ahead_load,frame.local(local::ahead))));
    }
    const auto free_now=memory.load32(free_load,frame.local(local::free));
    const auto ahead_now=memory.load32(ahead_final,frame.local(local::ahead));
    frame.ahead_compared(ahead_now,free_now);
    if(signed_value(ahead_now)>=signed_value(free_now)){outcome=Outcome::queued;return 1u;}
    outcome=Outcome::admitted;
    return 0u;
}
} // namespace respawn_wait
} // namespace halo2::native_respawn_wait
