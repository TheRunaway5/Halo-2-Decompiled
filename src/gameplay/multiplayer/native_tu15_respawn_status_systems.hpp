#pragma once
// Native TU1.5 respawn status, entered at 1618B0 (multiplayer engines unit):
// the status code shown for a player waiting on the game.
//
//   respawn_status::status   1618B0(player handle) -> EAX status; RET 4
//
// The current game engine (the session's index +0xC14 into [56DFA0]) first
// answers through its method +0x94 (ECX the engine; the player and an
// override byte pushed): unless it set the override, an answer other than
// NONE is the status. In a game with the decision mode (+0x1128 of
// [4F3218]) 15BE80 (the player, a decided byte and a second byte pushed)
// gives 8, or 9 with the second byte, when it decides, otherwise 0xA.
// Otherwise, for a player (0x21C bytes in [4F54F4]) without a unit (+0x2C):
// 1 without a team (+0xC0 0xFF), 4 when 160570 answers (ECX the player), 3
// with its byte +3 bit 6, 2 while its respawn time (+0x164) is not negative,
// else the engine's answer. With a unit, against a window of three seconds
// (the game time's ticks per second, +2 of [51DF54], times 3): a player whose
// lives (the signed byte +0x1A8) are below it, in a game with a penalty
// (+0x1B8), gets 0x18 for the penalty kind (+0x1AC) 1, else 0x16, or 0x17 for
// the kind 2; a session whose stamp (+0x70) is less than the window before
// the game time (+8) gives 0x12; a session mode (the signed word +0xE0)
// other than NONE and 0 and below 5 gives 0x11; an engine answer other than NONE is the status;
// otherwise 15BE80 decides: 5, or 6 with the second byte, when it decides,
// otherwise 7, each 0x14 more in a game without a late mode (+0x18C).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); the boundary adapter
// (native_tu15_respawn_status.hpp) maps the role to the original instruction,
// owns the ABI of the children and leaves through the original's epilogue
// for each status. The values the original keeps in callee-saved registers
// across the children are a Kept, refreshed from the registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_respawn_status {
using native_object_update::at;
namespace globals {inline constexpr std::uint32_t session=0x4f63b8u,engines=0x56dfa0u,game=0x4f3218u,players=0x4f54f4u,game_time=0x51df54u;}
inline constexpr std::uint32_t datum_data=0x44u,handle_mask=0xffffu,none=0xffffffffu,player_size=0x21cu,no_team=0xffu,window_seconds=3u;
namespace session {inline constexpr std::uint32_t stamp=0x70u,mode=0xe0u,engine=0xc14u;}
namespace engine {inline constexpr std::uint32_t status_method=0x94u;}
namespace game {inline constexpr std::uint32_t late_mode=0x18cu,penalty=0x1b8u,decision_mode=0x1128u;}
namespace player {inline constexpr std::uint32_t flags=3u,unit=0x2cu,team=0xc0u,respawn=0x164u,lives=0x1a8u,penalty_kind=0x1acu;
    inline constexpr std::uint32_t flag_bit=0x40u;}
namespace time {inline constexpr std::uint32_t ticks_per_second=2u,now=8u;}
// The status codes.
namespace code {inline constexpr std::uint32_t teamless=1u,respawning=2u,flagged=3u,quitting=4u,late_decided=5u,late_second=6u,
    late_undecided=7u,decided=8u,undecided=0xau,mode=0x11u,recent=0x12u,penalty=0x16u,penalty_two=0x17u,penalty_one=0x18u,unpaced=0x14u;}
inline constexpr std::int32_t mode_limit=5;
// The frame (offsets from the ESP after ECX, EBX and EDI are pushed): the
// override byte and the second byte in ECX's slot, and the player's handle
// (whose slot later holds the decided byte).
namespace local {inline constexpr std::uint32_t override_byte=0xau,second=0xbu,argument=0x10u,decided=0x10u;}
inline constexpr std::uint32_t signed_byte(std::uint32_t v){return std::uint32_t(std::int32_t(std::int8_t(std::uint8_t(v))));}
inline constexpr std::uint32_t signed_word(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(std::uint16_t(v))));}

namespace respawn_status {
enum class Access {session_load,engine_index,engine_load,player_argument,override_clear,engine_table,engine_method,override_load,
    game_load,decision_mode,early_decided,early_second,players_load,players_data,unit_load,session_again,session_mode,team_test,
    flags_test,respawn_load,time_load,time_rate,lives_load,game_again,penalty_test,penalty_kind,time_now,session_stamp,game_last,
    late_mode,late_player,late_decided,late_second};
// The original's exits, each with its own epilogue.
enum class Exit {engine_early,early_decided,early_undecided,teamless,quitting,flagged,respawning,engine,penalty_one,penalty,recent,
    mode,late_decided,late_second,late_undecided};
// The original's EDI (the player's handle), EBX (the engine's answer,
// later the late mode's absence in BL) and ESI (the player).
struct Kept {std::uint32_t handle,answer,player_address;};
struct Outcome {Exit exit;std::uint32_t status;};

// Frame: see native_tu15_respawn_status.hpp.
template<class Memory,class Frame> Outcome status(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    // The engine's answer.
    frame.reserving();
    const auto session_address=memory.load32(session_load,globals::session);
    const auto index=memory.load32(engine_index,at(session_address,session::engine));
    const auto engine_address=memory.load32(engine_load,globals::engines+4u*index);
    frame.saving(1u);
    frame.saving(2u);
    kept.handle=memory.load32(player_argument,frame.local(local::argument));
    frame.override_pushed();
    memory.store8(override_clear,frame.local(local::override_byte),0u);
    const auto table=memory.load32(engine_table,engine_address);
    frame.player_pushed(kept);
    kept.answer=frame.engine_answer(kept,memory.load32(engine_method,at(table,engine::status_method)));
    if(!frame.set(memory.load8(override_load,frame.local(local::override_byte))) && frame.answered(kept.answer))
        return {Exit::engine_early,kept.answer};
    // The decision mode.
    const auto game_address=memory.load32(game_load,globals::game);
    if(frame.set(memory.load8(decision_mode,at(game_address,game::decision_mode)))) {
        frame.decide(kept,false);
        if(frame.set(memory.load8(early_decided,frame.local(local::decided)))) {
            const auto second=memory.load8(early_second,frame.local(local::second));
            return {Exit::early_decided,code::decided+((second&0xffu)?1u:0u)};
        }
        return {Exit::early_undecided,code::undecided};
    }
    // The player.
    const auto players=memory.load32(players_load,globals::players);
    const auto data=memory.load32(players_data,at(players,datum_data));
    frame.saving(3u);
    frame.saving(4u);
    const auto offset=frame.player_offset(kept.handle);
    const auto unit=memory.load32(unit_load,offset+data+player::unit);
    kept.player_address=frame.player_address(offset,data);
    const bool unitless=!frame.answered(unit);
    const auto session_now=memory.load32(session_again,globals::session);
    const auto mode=signed_word(memory.load16(session_mode,at(session_now,session::mode)));
    if(unitless) {
        if(frame.teamless(memory.load8(team_test,at(kept.player_address,player::team))))return {Exit::teamless,code::teamless};
        if(frame.quitting(kept))return {Exit::quitting,code::quitting};
        if(frame.flagged(memory.load8(flags_test,at(kept.player_address,player::flags))))return {Exit::flagged,code::flagged};
        if(frame.negative(memory.load32(respawn_load,at(kept.player_address,player::respawn))))return {Exit::engine,kept.answer};
        return {Exit::respawning,code::respawning};
    }
    // A unit: the three-second window.
    const auto time_address=memory.load32(time_load,globals::game_time);
    const auto window=frame.window(signed_word(memory.load16(time_rate,at(time_address,time::ticks_per_second))));
    const auto lives=signed_byte(memory.load8(lives_load,at(kept.player_address,player::lives)));
    if(frame.below(lives,window)) {
        const auto game_now=memory.load32(game_again,globals::game);
        if(frame.penalized(memory.load32(penalty_test,at(game_now,game::penalty)))) {
            const auto kind=signed_word(memory.load16(penalty_kind,at(kept.player_address,player::penalty_kind)));
            if(frame.kind_one(kind))return {Exit::penalty_one,code::penalty_one};
            return {Exit::penalty,kind==2u?code::penalty_two:code::penalty};
        }
    }
    const auto now=memory.load32(time_now,at(time_address,time::now));
    const auto elapsed=frame.elapsed(now,memory.load32(session_stamp,at(session_now,session::stamp)));
    if(frame.below(elapsed,window))return {Exit::recent,code::recent};
    const bool counted=mode!=none && mode!=0u && std::int32_t(mode)<mode_limit;
    frame.mode_tested(mode);
    if(counted)return {Exit::mode,code::mode};
    if(frame.answered(kept.answer))return {Exit::engine,kept.answer};
    // 15BE80 decides.
    const auto game_last_address=memory.load32(game_last,globals::game);
    const auto late=memory.load32(late_mode,at(game_last_address,game::late_mode));
    memory.load32(late_player,frame.local(local::argument));
    frame.late_pushed(late);
    frame.decide(kept,true);
    const auto unpaced=(kept.answer&0xffu)?code::unpaced:0u;
    if(frame.set(memory.load8(late_decided,frame.local(local::decided)))) {
        if(frame.set(memory.load8(late_second,frame.local(local::second))))return {Exit::late_second,code::late_second+unpaced};
        return {Exit::late_decided,code::late_decided+unpaced};
    }
    return {Exit::late_undecided,code::late_undecided+unpaced};
}
} // namespace respawn_status
} // namespace halo2::native_respawn_status
