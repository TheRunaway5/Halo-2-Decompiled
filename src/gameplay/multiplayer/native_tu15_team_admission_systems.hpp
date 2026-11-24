#pragma once
// Native TU1.5 team admission, entered at 161560 (multiplayer engines unit):
// whether a team is let in under the session's variant.
//
//   team_admission::admits   161560(ECX = team) -> AL
//
// Without a variant record for the session ([56DFA0] by [4F63B8] +0xC14), or
// for the team NONE, every team is let in. In game mode 7 (the game globals
// [4F3218] +0x180) a team whose bit is set in the team mask ([51DF9C], a
// word) is let in when the game globals' byte +0x22C carries bit 1. Any
// other team is let in unless the game globals' +0x1C4 is 1.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_team_admission.hpp) maps the role to the original
// instruction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_team_admission {
using native_object_update::at;
using native_object_update::none;
namespace globals {
inline constexpr std::uint32_t session=0x4f63b8u;          // the session record (+0xC14 its variant index)
inline constexpr std::uint32_t variants=0x56dfa0u;         // the variant records, by the session's index
inline constexpr std::uint32_t game_state=0x4f3218u;       // GameGlobals* (+0x180 the mode, +0x1C4, +0x22C flags)
inline constexpr std::uint32_t team_mask=0x51df9cu;        // word*: the teams taking part
}
inline constexpr std::uint32_t variant_index=0xc14u,mode=0x180u,closed=0x1c4u,flags=0x22cu;
inline constexpr std::uint32_t wanted_mode=7u,team_flag=1u<<1,closed_value=1u;

namespace team_admission {
enum class Access {session,variant_index,variant,game_state,mode,mask_address,mask,flags,closed};
enum class Outcome {no_variant,no_team,admitted,decided};
struct Result {Outcome outcome;std::uint32_t value;};
// Frame: masked(team) the team's bit (SHL).
template<class Memory,class Frame> Result admits(Memory &memory,Frame &frame,std::uint32_t team) {
    using enum Access;
    const auto session_address=memory.load32(session,globals::session);
    const auto index=memory.load32(Access::variant_index,at(session_address,native_team_admission::variant_index));
    if(!memory.load32(variant,globals::variants+4u*index))return {Outcome::no_variant,1u};
    if(team==none)return {Outcome::no_team,1u};
    const auto state=memory.load32(game_state,globals::game_state);
    if(memory.load32(Access::mode,at(state,native_team_admission::mode))==wanted_mode) {
        const auto bit=frame.masked(team);
        const auto teams=memory.load16(mask,memory.load32(mask_address,globals::team_mask))&0xffffu;
        if((teams&bit) && (memory.load8(Access::flags,at(state,native_team_admission::flags))&team_flag))
            return {Outcome::admitted,1u};
    }
    return {Outcome::decided,memory.load32(Access::closed,at(state,native_team_admission::closed))!=closed_value?1u:0u};
}
} // namespace team_admission
} // namespace halo2::native_team_admission
