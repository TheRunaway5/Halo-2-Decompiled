#pragma once
// Native TU1.5 team hostility, entered at 1E7810 (physics live support and AI
// frame units): whether two teams are hostile.
//
//   team_hostility::hostile   1E7810(CX = team, SI = other team) -> AL
//
// Either team NONE is hostile. In game mode 1 (the game globals' [4F3218]
// +8) two teams within 0..15 are hostile unless their bit (16 * team +
// other) is set in the alliance bits ([5020C4] +0xC4, through C1E90); a team
// out of range is hostile. In game mode 2 160A60 decides (the teams
// pushed). Otherwise different teams are hostile.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_team_hostility.hpp) maps the role to the original
// instruction and owns the ABI of the children.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_team_hostility {
using native_object_update::at;
using native_object_update::widen16;
namespace globals {
inline constexpr std::uint32_t game_state=0x4f3218u;    // GameGlobals* (+8 the mode)
inline constexpr std::uint32_t alliances=0x5020c4u;     // the alliance record (+0xC4 its bits)
}
inline constexpr std::uint32_t game_mode=8u,alliance_bits=0xc4u,teams=16u;
inline constexpr std::uint32_t alliance_mode=1u,decided_mode=2u;

namespace team_hostility {
enum class Access {game_state,mode,alliances};
enum class Outcome {no_team,out_of_range,allied,decided,other_mode};
struct Result {Outcome outcome;std::uint32_t value;};
// Frame: compared(team, bound) the range tests; allied(index, bits) C1E90,
// returns AL; decide(team, other) 160A60, returns AL.
template<class Memory,class Frame> Result hostile(Memory &memory,Frame &frame,std::uint32_t team,std::uint32_t other) {
    using enum Access;
    team&=0xffffu;other&=0xffffu;
    if(team==0xffffu || other==0xffffu)return {Outcome::no_team,1u};
    const auto state=memory.load32(game_state,globals::game_state);
    const auto mode_value=memory.load32(Access::mode,at(state,game_mode));
    if(mode_value==alliance_mode) {
        for(const auto value:{team,other}) {
            frame.ranged(value);
            if(std::int16_t(value)<0 || std::int16_t(value)>=std::int16_t(teams))return {Outcome::out_of_range,1u};
        }
        const auto record=memory.load32(Access::alliances,globals::alliances);
        const auto index=(widen16(team)<<4)+widen16(other);
        return {Outcome::allied,frame.allied(index,record+alliance_bits)?0u:1u};
    }
    if(mode_value==decided_mode)return {Outcome::decided,frame.decide(team,other)};
    return {Outcome::other_mode,team!=other?1u:0u};
}
} // namespace team_hostility
} // namespace halo2::native_team_hostility
