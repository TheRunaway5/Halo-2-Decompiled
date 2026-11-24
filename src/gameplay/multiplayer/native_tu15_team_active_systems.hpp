#pragma once
// Native TU1.5 team active, entered at 164890 (physics live support unit):
// whether a team takes part in the game.
//
//   team_active::query   164890(ECX = team) -> AL
//
// In a team game (the engine [56DFA0 + 4 * the index +0xC14 of the engine
// globals at [4F63B8]] present and bit 0 of the variant flags +0x184 of
// [4F3218]), a team from 0 to 7 takes part when its bit is set in the engine
// globals' team mask (word +0xC). The team flag is kept in the original's
// frame (the top byte of the ECX it pushes).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); the boundary adapter (native_tu15_team_active.hpp)
// maps the role to the original instruction and owns the ABI.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_team_active {
using native_object_update::at;
using native_object_update::signed_value;
namespace globals {inline constexpr std::uint32_t game=0x4f3218u,engine_globals=0x4f63b8u,engines=0x56dfa0u;}
inline constexpr std::uint32_t team_count=8u;
namespace game {inline constexpr std::uint32_t variant_flags=0x184u;inline constexpr std::uint32_t teams=1u;}
namespace engine_globals {inline constexpr std::uint32_t team_mask=0xcu,index=0xc14u;}
// The frame's slot, by offset from the return address: the team flag, in the pushed ECX.
namespace local {inline constexpr std::int32_t teams=-1;}

namespace team_active {
enum class Access {engine_globals,engine_index,engine,game,variant_flags,teams_store,team_mask};
enum class Outcome {no_engine,no_teams,out_of_range,answered};
// Frame: saving(n) PUSH ECX, ESI (then EDI); local(offset); engine_tested(engine) XOR AL,AL, TEST;
// teams_tested(teams) AND DL,1; range_tested(team, high) TEST ECX,ECX / CMP ECX,8; masked(team, mask) the SHL, TEST, SETNZ.
template<class Memory,class Frame> std::uint32_t query(Memory &memory,Frame &frame,std::uint32_t team,Outcome &outcome) {
    using enum Access;
    frame.saving(0u);
    const auto globals_address=memory.load32(Access::engine_globals,globals::engine_globals);
    const auto index=memory.load32(engine_index,at(globals_address,engine_globals::index));
    frame.saving(1u);
    const auto engine_value=memory.load32(engine,globals::engines+4u*index);
    frame.engine_tested(engine_value);
    if(!engine_value){outcome=Outcome::no_engine;return 0u;}
    const auto teams=memory.load8(variant_flags,at(memory.load32(game,globals::game),game::variant_flags))&game::teams;
    frame.teams_tested(teams);
    memory.store8(teams_store,frame.local(local::teams),teams);
    if(!teams){outcome=Outcome::no_teams;return 0u;}
    frame.range_tested(team,false);
    if(signed_value(team)<0){outcome=Outcome::out_of_range;return 0u;}
    frame.range_tested(team,true);
    if(signed_value(team)>=signed_value(team_count)){outcome=Outcome::out_of_range;return 0u;}
    const auto mask=memory.load16(team_mask,at(globals_address,engine_globals::team_mask));
    outcome=Outcome::answered;
    return frame.masked(team,mask);
}
} // namespace team_active
} // namespace halo2::native_team_active
