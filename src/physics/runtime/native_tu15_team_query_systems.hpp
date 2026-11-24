#pragma once
// Native TU1.5 team query, entered at 15C7F0 (physics live support unit):
// whether a team takes part under the session's variant.
//
//   team_query::takes_part   15C7F0(team byte) -> AL
//
// Only with a variant record for the session ([56DFA0] by [4F63B8] +0xC14),
// in game mode 7 (the game globals [4F3218] +0x180): the team's bit (the
// argument's low byte) must be set in the team mask ([51DF9C], a word) and
// the game globals' byte +0x22C must carry bit 5.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_team_query.hpp) maps the role to the original instruction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_team_query {
using native_object_update::at;
namespace globals {
inline constexpr std::uint32_t session=0x4f63b8u;          // the session record (+0xC14 its variant index)
inline constexpr std::uint32_t variants=0x56dfa0u;         // the variant records, by the session's index
inline constexpr std::uint32_t game_state=0x4f3218u;       // GameGlobals* (+0x180 the mode, +0x22C flags)
inline constexpr std::uint32_t team_mask=0x51df9cu;        // word*: the teams taking part
}
inline constexpr std::uint32_t wanted_mode=7u,team_flag=0x20u;

namespace team_query {
enum class Access {session,variant_index,variant,game_state,mode,team,mask_address,mask,flags};
// Frame: argument() the team's slot.
template<class Memory,class Frame> bool takes_part(Memory &memory,Frame &frame) {
    using enum Access;
    const auto session_address=memory.load32(session,globals::session);
    const auto index=memory.load32(variant_index,at(session_address,0xc14u));
    if(!memory.load32(variant,globals::variants+4u*index))return false;
    const auto state=memory.load32(game_state,globals::game_state);
    if(memory.load32(mode,at(state,0x180u))!=wanted_mode)return false;
    const auto bit=1u<<(memory.load8(team,frame.argument())&31u);
    if(!(memory.load16(mask,memory.load32(mask_address,globals::team_mask))&bit))return false;
    return (memory.load8(flags,at(state,0x22cu))&team_flag)!=0;
}
} // namespace team_query
} // namespace halo2::native_team_query
