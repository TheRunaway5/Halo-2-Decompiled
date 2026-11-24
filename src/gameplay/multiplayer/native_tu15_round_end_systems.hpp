#pragma once
// Native TU1.5 round end, entered at 15B740 (multiplayer engines unit):
// whether a round is down to its last side, and who won it.
//
//   round_end::check   15B740(EDI = winner*; teams_down*) -> AL, RET 4
//
// Without a variant record for the session ([56DFA0] by [4F63B8] +0xC14) it
// answers 0, and 0 to teams_down. Otherwise it counts the players (15B4D0,
// a word) and the teams (15B620: the last living player, how many teams have
// living players, how many have players). With more than one player the
// round is down when fewer than two teams are living, and the teams are down
// when fewer than two have players; with one or none, when none are. When the
// round is down to one living team whose last living player is known, the
// winner is that player; or, when the variant's option says so (161530),
// that player's team (int8 +0xC0 of its 0x21C-byte datum among the players
// at [4F54F4]), made NONE when the teams are not down, in game modes 1 and 9
// ([4F3218] +0x180) with the option +0x240 set, for the session's own team
// (+0x2F0). teams_down (when not NULL) takes the teams' answer.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the original's frame (the answer, the
// last living player and the two counts 15B620 fills); the boundary adapter
// (native_tu15_round_end.hpp) maps the role to the original instruction and
// owns the ABI of the children. The values the original keeps in
// callee-saved registers across them are a Kept, refreshed from the
// registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_round_end {
using native_object_update::at;
using native_object_update::none;
using native_object_update::signed_value;
using native_object_update::widen16;
using native_object_update::widen8;
namespace globals {inline constexpr std::uint32_t session=0x4f63b8u,variants=0x56dfa0u,players=0x4f54f4u,game=0x4f3218u;}
inline constexpr std::uint32_t datum_data=0x44u,player_size=0x21cu;
namespace session {inline constexpr std::uint32_t variant=0xc14u,team=0x2f0u;}
namespace player {inline constexpr std::uint32_t team=0xc0u;}
namespace game {inline constexpr std::uint32_t mode=0x180u,team_option=0x240u;}
inline constexpr std::uint32_t mode_a=1u,mode_b=9u,many_players=1u,sides=2u;
// The frame's locals, by offset from the return address.
namespace local {inline constexpr std::int32_t last=-4,teams=-8,living=-0xc,answer=-0xd;}

namespace round_end {
enum class Access {session_address,variant_index,variant_entry,answer_clear,last_init,teams_load,living_load,answer_store,answer_load,
    last_load,players_header,player_datums,player_team,winner_team,game_address,game_mode,team_option,session_team,winner_none,
    winner_player,out_argument,out_store,answer_final};
enum class Outcome {no_variant,not_down,not_single,no_last,player_wins,team_down,other_mode,no_option,other_team,team_none};
// The original's BL (the teams' answer), EBP (the session), ESI (the players' count, then the last living player) and EDI
// (the winner's output) across the children.
struct Kept {std::uint32_t teams_down,session,held,winner;};
// Frame:
//   argument(), local(offset)              the argument's slot and the frame's locals now
//   cleared(kept), variant_tested(record)  XOR BL,BL; TEST ECX,ECX
//   saving(), restoring()                  ESI around the counts
//   players(kept) -> AX                    15B4D0(), refreshes kept
//   count(last*, living*, teams*, kept)    15B620(the three outputs pushed), refreshes kept
//   answered(players, living, answer), teams_tested(players, teams, kept)   the counts' compares and SETcc
//   answer_tested(answer, living), last_tested(last)                        the answer's tests
//   by_team(kept) -> AL                    161530(), refreshes kept
//   team_read(kept), mode_tested(mode), option_tested(option), team_compared(session_team, team), out_tested(out)
template<class Memory,class Frame> Outcome check(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    kept.session=memory.load32(session_address,globals::session);
    const auto variant=memory.load32(variant_index,at(kept.session,session::variant));
    const auto variant_record=memory.load32(variant_entry,globals::variants+4u*variant);
    kept.teams_down=0u;
    frame.cleared(kept);
    frame.variant_tested(variant_record);
    memory.store8(answer_clear,frame.local(local::answer),0u);
    auto outcome=Outcome::no_variant;
    if(variant_record) {
        frame.saving();
        memory.store32(last_init,frame.local(local::last),none);
        kept.held=widen16(frame.players(kept)&0xffffu);
        frame.count(frame.local(local::last),frame.local(local::living),frame.local(local::teams),kept);
        const auto teams=memory.load32(teams_load,frame.local(local::teams));
        const auto living=memory.load32(living_load,frame.local(local::living));
        const bool many=signed_value(kept.held)>signed_value(many_players);
        const auto answer=(many?signed_value(living)<signed_value(sides):signed_value(living)<=0)?1u:0u;
        frame.answered(kept.held,living,answer);
        memory.store8(answer_store,frame.local(local::answer),answer);
        kept.teams_down=(many?signed_value(teams)<signed_value(sides):signed_value(teams)<=0)?1u:0u;
        frame.teams_tested(kept.held,teams,kept);
        const auto answer_now=memory.load8(answer_load,frame.local(local::answer));
        frame.answer_tested(answer_now,living);
        if(!answer_now)outcome=Outcome::not_down;
        else if(living!=1u)outcome=Outcome::not_single;
        else {
            kept.held=memory.load32(last_load,frame.local(local::last));
            frame.last_tested(kept.held);
            if(kept.held==none)outcome=Outcome::no_last;
            else if(!(frame.by_team(kept)&0xffu)) {
                memory.store32(winner_player,kept.winner,kept.held);
                outcome=Outcome::player_wins;
            } else {
                const auto datums=memory.load32(player_datums,at(memory.load32(players_header,globals::players),datum_data));
                frame.team_read(kept);
                const auto team=widen8(memory.load8(player_team,player_size*(kept.held&0xffffu)+datums+player::team));
                memory.store32(winner_team,kept.winner,team);
                if(kept.teams_down&0xffu)outcome=Outcome::team_down;
                else {
                    const auto game_value=memory.load32(game_address,globals::game);
                    const auto mode=memory.load32(game_mode,at(game_value,game::mode));
                    frame.mode_tested(mode);
                    if(mode!=mode_a && mode!=mode_b)outcome=Outcome::other_mode;
                    else if(memory.load32(team_option,at(game_value,game::team_option))!=1u)outcome=Outcome::no_option;
                    else if(memory.load32(session_team,at(kept.session,session::team))!=team)outcome=Outcome::other_team;
                    else {
                        memory.store32(winner_none,kept.winner,none);
                        outcome=Outcome::team_none;
                    }
                }
            }
        }
        frame.restoring();
    }
    const auto out=memory.load32(out_argument,frame.argument());
    frame.out_tested(out);
    if(out)memory.store8(out_store,out,kept.teams_down);
    memory.load8(answer_final,frame.local(local::answer));
    return outcome;
}
} // namespace round_end
} // namespace halo2::native_round_end
