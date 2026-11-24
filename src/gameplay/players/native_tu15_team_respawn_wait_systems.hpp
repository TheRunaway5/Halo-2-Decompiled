#pragma once
// Native TU1.5 team respawn wait, entered at 160360 (physics live support
// unit): whether a dead player waits for the teams to even out.
//
//   team_respawn_wait::decide   160360(EAX = player) -> AL
//
// In a balanced game (bit 14 of the variant flags, +0x184 of [4F3218]) that
// is a team game (the engine [56DFA0 + 4 * the index +0xC14 of [4F63B8]]
// present and bit 0 of the variant flags), a dead player (+0x2C NONE; 0x21C
// bytes through the players at [4F54F4]) counts, through the player iterator
// (1A8E50, its state on the stack), each team's (byte +0xC0, not 0xFF)
// players: the living, and the dead that 160570 does not set aside and that
// do not themselves wait (1601B0). The smallest count among the teams that
// 164890 admits (at most 0x10, at least 2) is the share. A team over its
// share has its dead wait: the slots free are the share less the player's
// teammates that live (or would, by 160570), and the teammates ahead of the
// player (by order, +0x1B4, then index) fill them first. The answer is the
// frame's flag byte (1, though a team counted below 0 reaches it), or 0.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the original's frame (the iterator, the
// team counts, the share, the player's index and the flag); the boundary
// adapter (native_tu15_team_respawn_wait.hpp) maps the role to the original
// instruction and owns the ABI of the children. The values the original
// keeps in callee-saved registers across them are a Kept, refreshed from the
// registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_team_respawn_wait {
using native_object_update::at;
using native_object_update::none;
using native_object_update::signed_value;
using native_object_update::widen8;
namespace globals {inline constexpr std::uint32_t game=0x4f3218u,players=0x4f54f4u,engine_globals=0x4f63b8u,engines=0x56dfa0u;}
inline constexpr std::uint32_t datum_data=0x44u,player_size=0x21cu,team_count=8u,share_ceiling=0x10u,share_floor=2u,no_team=0xffu;
namespace game {inline constexpr std::uint32_t variant_flags=0x184u;inline constexpr std::uint32_t balanced_bit=14u,teams=1u;}
namespace engine_globals {inline constexpr std::uint32_t index=0xc14u;}
namespace player {inline constexpr std::uint32_t unit=0x2cu,team=0xc0u,order=0x1b4u;}
// The iterator's state (1A8E50): the current player, the array, its handle and index.
namespace iterator {inline constexpr std::uint32_t current=0u,array=4u,handle=8u,index=0xcu;}
// The frame's slots, by offset from the return address: the flag byte, the share, the living teammates, the team
// counts (one byte a team; their first dword later the teammates ahead), the player's index, the iterator's state.
namespace local {inline constexpr std::int32_t flag=-0x25,share=-0x24,living=-0x20,counts=-0x1c,ahead=-0x1c,own_index=-0x14,state=-0x10;}

namespace team_respawn_wait {
enum class Access {game,variant_flags,players_array,players_data,index_store,engine_globals,engine_index,engine,teams_flags,
    teams_store,unit,counts_clear,flag_store,counts_clear_high,array_store,iterator_index,iterator_handle,current,team,other_unit,
    handle,count_team,count_load,count_store,share_init,team_size,share_load,share_store,share_again,share_floor_store,own_team,
    own_count,share_compare,living_clear,ahead_clear,own_team_again,other_team,teammate_unit,teammate_handle,living_load,
    living_store,other_order,own_order,own_index,ahead_load,ahead_store,share_final,living_final,ahead_final,flag_load};
enum class Outcome {unbalanced,no_engine,no_teams,alive,even,full,queued,admitted};
// The original's EBX (NONE, or a handle), EBP (the players, then a teammate's unit), ESI (the iterator, the current
// player or a team) and EDI (the player's address) across the children.
struct Kept {std::uint32_t held,players,current,own;};
// Frame:
//   local(offset), saving(n)                     the frame's slots now; PUSH EBP, EDI (MOV EDI,EAX), ESI, EBX (OR EBX,-1)
//   indexing(index), balance_tested(index, data, flags), engine_tested(engine), teams_tested(teams), unit_tested(unit, kept)
//   counting(kept)                               XOR EDX,EDX before the counts' stores
//   next(pass, kept) -> AL                       1A8E50(ESI = the iterator), refreshes kept
//   team_tested(team), unit_compared(unit, against), set_aside(handle, kept) -> AL (160570, ECX = handle),
//   waits(handle, kept) -> AL (1601B0, EAX = handle), released(kept) (OR EBX,-1), bumped(count) -> count + 1
//   scanning(kept), admitted(team, kept) -> AL (164890, ECX = team), share_compared(share, count), advanced(kept)
//   floored(share), even_compared(share, count), teams_compared(own, other), self_compared(current, own),
//   living_compared(unit), orders_compared(other, own), indices_compared(own index, handle), incremented(v) -> v + 1
//   free_tested(free), ahead_compared(ahead, free)
template<class Memory,class Frame> std::uint32_t decide(Memory &memory,Frame &frame,std::uint32_t player_handle,Outcome &outcome) {
    using enum Access;
    Kept kept{};
    const auto game_address=memory.load32(Access::game,globals::game);
    const auto flags=memory.load32(variant_flags,at(game_address,game::variant_flags));
    frame.saving(0u);
    kept.players=memory.load32(players_array,globals::players);
    frame.saving(1u);
    const auto data=memory.load32(players_data,at(kept.players,datum_data));
    const auto own_index_value=player_handle&0xffffu;
    frame.indexing(own_index_value);
    memory.store32(index_store,frame.local(local::own_index),own_index_value);
    kept.own=own_index_value*player_size+data;
    frame.balance_tested(own_index_value,data,flags);
    if(!((flags>>game::balanced_bit)&1u)){outcome=Outcome::unbalanced;return 0u;}
    const auto index=memory.load32(engine_index,at(memory.load32(Access::engine_globals,globals::engine_globals),engine_globals::index));
    frame.saving(2u);
    const auto engine_value=memory.load32(engine,globals::engines+4u*index);
    frame.engine_tested(engine_value);
    if(!engine_value){outcome=Outcome::no_engine;return 0u;}
    const auto teams=memory.load8(teams_flags,at(game_address,game::variant_flags))&game::teams;
    frame.teams_tested(teams);
    memory.store8(teams_store,frame.local(local::flag),teams);
    if(!teams){outcome=Outcome::no_teams;return 0u;}
    const auto unit_value=memory.load32(unit,at(kept.own,player::unit));
    frame.saving(3u);
    kept.held=none;
    frame.unit_tested(unit_value,kept);
    if(unit_value!=kept.held){outcome=Outcome::alive;return 0u;}
    // Each team's players: the living, and the dead that neither are set aside nor wait.
    frame.counting(kept);
    memory.store32(counts_clear,frame.local(local::counts),0u);
    memory.store8(flag_store,frame.local(local::flag),1u);
    memory.store32(counts_clear_high,frame.local(local::counts)+4u,0u);
    memory.store32(array_store,frame.local(local::state)+iterator::array,kept.players);
    memory.store32(iterator_index,frame.local(local::state)+iterator::index,kept.held);
    memory.store32(iterator_handle,frame.local(local::state)+iterator::handle,kept.held);
    for(auto more=frame.next(0u,kept);more&0xffu;more=frame.next(0u,kept)) {
        kept.current=memory.load32(current,frame.local(local::state)+iterator::current);
        const auto team_value=memory.load8(team,at(kept.current,player::team));
        frame.team_tested(team_value);
        if(team_value==no_team)continue;
        const auto other_unit=memory.load32(Access::other_unit,at(kept.current,player::unit));
        frame.unit_compared(other_unit,kept.held);
        if(other_unit==kept.held) {
            kept.held=memory.load32(handle,frame.local(local::state)+iterator::handle);
            if(frame.set_aside(kept.held,kept)&0xffu){kept.held=none;frame.released(kept);continue;}
            if(frame.waits(kept.held,kept)&0xffu){kept.held=none;frame.released(kept);continue;}
            kept.held=none;
            frame.released(kept);
        }
        const auto slot=frame.local(local::counts)+widen8(memory.load8(count_team,at(kept.current,player::team)));
        memory.store8(count_store,slot,frame.bumped(memory.load8(count_load,slot)));
    }
    // The share: the smallest admitted team's count, at most 0x10 and at least 2.
    memory.store32(share_init,frame.local(local::share),share_ceiling);
    kept.current=0u;
    frame.scanning(kept);
    do {
        if(frame.admitted(kept.current,kept)&0xffu) {
            const auto count=widen8(memory.load8(team_size,frame.local(local::counts)+kept.current));
            const auto share=memory.load32(share_load,frame.local(local::share));
            frame.share_compared(share,count);
            if(signed_value(share)>signed_value(count))memory.store32(share_store,frame.local(local::share),count);
        }
        ++kept.current;
        frame.advanced(kept);
    } while(signed_value(kept.current)<signed_value(team_count));
    const auto share_now=memory.load32(share_again,frame.local(local::share));
    frame.floored(share_now);
    if(!(signed_value(share_now)>signed_value(share_floor)))memory.store32(share_floor_store,frame.local(local::share),share_floor);
    const auto own_team_value=widen8(memory.load8(own_team,at(kept.own,player::team)));
    const auto own_count_value=widen8(memory.load8(own_count,frame.local(local::counts)+own_team_value));
    const auto share_value=memory.load32(share_compare,frame.local(local::share));
    frame.even_compared(share_value,own_count_value);
    if(signed_value(share_value)>=signed_value(own_count_value)){outcome=Outcome::even;return 0u;}
    // The player's team is over its share: its living, and its dead ahead of the player.
    frame.counting(kept);
    memory.store32(living_clear,frame.local(local::living),0u);
    memory.store32(ahead_clear,frame.local(local::ahead),0u);
    memory.store32(array_store,frame.local(local::state)+iterator::array,kept.players);
    memory.store32(iterator_index,frame.local(local::state)+iterator::index,kept.held);
    memory.store32(iterator_handle,frame.local(local::state)+iterator::handle,kept.held);
    for(auto more=frame.next(1u,kept);more&0xffu;more=frame.next(1u,kept)) {
        kept.current=memory.load32(current,frame.local(local::state)+iterator::current);
        const auto own=memory.load8(own_team_again,at(kept.own,player::team));
        const auto other=memory.load8(other_team,at(kept.current,player::team));
        frame.teams_compared(own,other);
        if(own!=other)continue;
        frame.self_compared(kept.current,kept.own);
        if(kept.current==kept.own)continue;
        kept.players=memory.load32(teammate_unit,at(kept.current,player::unit));
        frame.living_compared(kept.players);
        const bool living_unit=kept.players!=none;
        kept.held=memory.load32(teammate_handle,frame.local(local::state)+iterator::handle);
        if(!living_unit) {
            if(frame.set_aside(kept.held,kept)&0xffu)continue;
            frame.living_compared(kept.players);
        }
        if(kept.players!=none) {
            memory.store32(living_store,frame.local(local::living),frame.incremented(memory.load32(living_load,frame.local(local::living))));
            continue;
        }
        const auto other_order=memory.load32(Access::other_order,at(kept.current,player::order));
        const auto own_order=memory.load32(Access::own_order,at(kept.own,player::order));
        frame.orders_compared(other_order,own_order);
        if(signed_value(other_order)>signed_value(own_order))continue;
        if(other_order==own_order) {
            const auto own_index_now=memory.load32(own_index,frame.local(local::own_index));
            frame.indices_compared(own_index_now,kept.held);
            if(signed_value(own_index_now)<signed_value(kept.held&0xffffu))continue;
        }
        memory.store32(ahead_store,frame.local(local::ahead),frame.incremented(memory.load32(ahead_load,frame.local(local::ahead))));
    }
    const auto share_final_value=memory.load32(share_final,frame.local(local::share));
    const auto free_value=frame.free_tested(share_final_value,memory.load32(living_final,frame.local(local::living)));
    if(signed_value(free_value)>0) {
        const auto ahead=memory.load32(ahead_final,frame.local(local::ahead));
        frame.ahead_compared(ahead,free_value);
        if(!(signed_value(ahead)>=signed_value(free_value))){outcome=Outcome::admitted;return 0u;}
        outcome=Outcome::queued;
    } else outcome=Outcome::full;
    return memory.load8(flag_load,frame.local(local::flag));
}
} // namespace team_respawn_wait
} // namespace halo2::native_team_respawn_wait
