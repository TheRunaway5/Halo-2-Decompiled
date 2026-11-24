#pragma once
// Native TU1.5 score lead, entered at 15BE80 (multiplayer engines unit):
// whether a player's side leads the scores, and whether it shares the lead.
//
//   score_lead::lead   15BE80(player, leading*, tied*) -> RET C
//
// In a team game (a variant record for the session, [56DFA0] by [4F63B8]
// +0xC14, and bit 0 of byte +0x184 of [4F3218]) the player's team (int8
// +0xC0 of its 0x21C-byte record among the players at [4F54F4]) scores
// 15B210's answer (EDI the team); every other active team (1648E0, ECX the
// team) of the eight is compared with it. Otherwise each other player in use
// (walked by the filtered iterator 1A8E50 through an iterator on the stack)
// is compared by its score, the int16 +4 of its 0x1C-byte entry at the
// session's +0x304 (0 without a variant record). A side scoring more ends the
// comparison: the player does not lead (both outputs 0); an equal one marks
// a tie. Otherwise *leading is 1 and *tied the tie.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the original's frame (the tie, the
// team-game byte, the iterator); the boundary adapter
// (native_tu15_score_lead.hpp) maps the role to the original instruction and
// owns the ABI of the children. The values the original keeps in
// callee-saved registers across them are a Kept, refreshed from the
// registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_score_lead {
using native_object_update::at;
using native_object_update::none;
using native_object_update::signed_value;
using native_object_update::widen16;
using native_object_update::widen8;
namespace globals {inline constexpr std::uint32_t players=0x4f54f4u,session=0x4f63b8u,variants=0x56dfa0u,game=0x4f3218u;}
inline constexpr std::uint32_t datum_data=0x44u,player_size=0x21cu,score_size=0x1cu,team_count=8u;
namespace player {inline constexpr std::uint32_t team=0xc0u;}
namespace session {inline constexpr std::uint32_t variant=0xc14u,scores=0x304u;}
namespace score {inline constexpr std::uint32_t points=4u;}
namespace game {inline constexpr std::uint32_t mode_flags=0x184u;inline constexpr std::uint32_t team_game=1u;}
namespace argument {inline constexpr std::uint32_t player=0u,leading=1u,tied=2u;}
// The frame's slots, by offset from the return address.
namespace local {inline constexpr std::int32_t tied=-0x12,team_game=-0x11,iterator=-0x10,array=-0xc,handle=-8,index=-4;}

namespace score_lead {
enum class Access {player_argument,players_array,player_data,session_address,variant_index,variant_entry,tied_clear,game_address,
    mode_flags,team_game_store,player_team,player_team_again,tied_store,own_score,array_store,index_store,handle_store,handle_load,
    player_again,variant_index_again,variant_entry_again,other_score,leading_out,tied_load,tied_out,leading_store,tied_out_store};
enum class Outcome {team_lead,team_trailing,player_lead,player_trailing};
// The original's EBX (the player's score), EBP (the players array, then the team's score), ESI (the player's record, then
// the variant record) and EDI (the walker: the session, or the team being compared) across the children.
struct Kept {std::uint32_t own,held,record,walker;};
// Frame:
//   argument(k), local(offset)                    the argument slots and the frame's slots now
//   saving(...), variant_tested(record), team_tested(flag)   the saved registers and the tests between
//   team_score(kept) -> EAX, active(kept) -> AL  15B210(EDI = team) and 1648E0(ECX = team), refresh kept
//   compared(team, own_team), difference(diff)    CMP EDI,EAX; SUB EAX,EBP (SUB ECX,EBX) and TEST
//   next(kept) -> AL                              1A8E50(ESI = the iterator), refreshes kept
//   leaving(exit)                                 POP EDI, ESI, EBP before the outputs' stores
template<class Memory,class Frame> Outcome lead(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    const auto player_handle=memory.load32(player_argument,frame.argument(argument::player));
    frame.saving(0u);
    kept.held=memory.load32(players_array,globals::players);
    frame.saving(1u);
    const auto index=player_handle&0xffffu;
    kept.record=player_size*index;
    frame.saving(2u);
    kept.record+=memory.load32(player_data,at(kept.held,datum_data));
    kept.walker=memory.load32(session_address,globals::session);
    const auto variant=memory.load32(variant_index,at(kept.walker,session::variant));
    const auto variant_record=memory.load32(variant_entry,globals::variants+4u*variant);
    frame.variant_tested(variant_record);
    memory.store8(tied_clear,frame.local(local::tied),0u);
    const auto finish=[&](bool leading,bool team_path) {
        const auto leading_address=memory.load32(leading_out,frame.argument(argument::leading));
        std::uint32_t tie=0u;
        if(leading)tie=memory.load8(tied_load,frame.local(local::tied));
        const auto tied_address=memory.load32(tied_out,frame.argument(argument::tied));
        frame.leaving(leading?(team_path?0u:1u):2u,leading_address,tied_address,tie);
        memory.store8(leading_store,leading_address,leading?1u:0u);
        memory.store8(tied_out_store,tied_address,tie);
        return leading?(team_path?Outcome::team_lead:Outcome::player_lead):(team_path?Outcome::team_trailing:Outcome::player_trailing);
    };
    if(variant_record) {
        const auto game_value=memory.load32(game_address,globals::game);
        const auto team_game_value=memory.load8(mode_flags,at(game_value,game::mode_flags))&game::team_game;
        frame.team_tested(team_game_value);
        memory.store8(team_game_store,frame.local(local::team_game),team_game_value);
        if(team_game_value) {
            // The teams: the player's against each other active one.
            kept.walker=widen8(memory.load8(player_team,at(kept.record,player::team)));
            kept.held=frame.team_score(kept);
            kept.walker=0u;
            frame.teams_started(kept);
            do {
                const auto own_team=widen8(memory.load8(player_team_again,at(kept.record,player::team)));
                frame.compared(kept.walker,own_team);
                if(kept.walker!=own_team && (frame.active(kept)&0xffu)) {
                    const auto diff=frame.team_score(kept)-kept.held;
                    frame.difference(diff,kept.held);
                    if(!diff)memory.store8(tied_store,frame.local(local::tied),1u);
                    else if(signed_value(diff)>0)return finish(false,true);
                }
                ++kept.walker;
                frame.advanced(kept.walker);
            } while(signed_value(kept.walker)<signed_value(team_count));
            return finish(true,true);
        }
    }
    // The players: each other one in use against the player.
    const auto score_of=[&](Access role,std::uint32_t record_value,std::uint32_t handle) {
        std::uint32_t points=0u;
        frame.score_tested(record_value);
        if(record_value) {
            const auto scores=kept.walker+session::scores;
            frame.scores_tested(scores,handle&0xffffu);
            if(scores)points=widen16(memory.load16(role,scores+score_size*(handle&0xffffu)+score::points));
        }
        return points;
    };
    kept.own=score_of(own_score,variant_record,player_handle);
    frame.iterating(kept);
    memory.store32(array_store,frame.local(local::array),kept.held);
    memory.store32(index_store,frame.local(local::index),none);
    memory.store32(handle_store,frame.local(local::handle),none);
    if(frame.next(kept)&0xffu) {
        do {
            const auto handle=memory.load32(handle_load,frame.local(local::handle));
            const auto player_now=memory.load32(player_again,frame.argument(argument::player));
            frame.player_compared(handle,player_now);
            if(handle!=player_now) {
                const auto variant_now=memory.load32(variant_index_again,at(kept.walker,session::variant));
                kept.record=memory.load32(variant_entry_again,globals::variants+4u*variant_now);
                const auto diff=score_of(other_score,kept.record,handle)-kept.own;
                frame.difference(diff,kept.own);
                if(!diff)memory.store8(tied_store,frame.local(local::tied),1u);
                if(signed_value(diff)>0)return finish(false,false);
            }
        } while(frame.next(kept)&0xffu);
    }
    return finish(true,false);
}
} // namespace score_lead
} // namespace halo2::native_score_lead
