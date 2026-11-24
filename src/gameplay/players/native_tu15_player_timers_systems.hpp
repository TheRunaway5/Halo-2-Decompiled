#pragma once
// Native TU1.5 player timers, entered at 154D00 (multiplayer engines unit):
// a player's two countdown words, one tick down.
//
//   player_timers::tick   154D00(EAX = player)
//
// Each of the player's two int16 timers (+0x198 of the 0x21C-byte players at
// [4F54F4]) that is positive counts down by one. When the first reaches
// zero, D3340 is told for the player's unit (+0x2C) with the factor one
// (46A0D0) in XMM0.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_player_timers.hpp) maps the role to the original instruction
// and owns the ABI of the child (D3340). The values the original keeps in
// callee-saved registers across it are a Kept, refreshed from the registers
// it returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_player_timers {
using native_object_update::at;
namespace globals {inline constexpr std::uint32_t players=0x4f54f4u,one=0x46a0d0u;}
inline constexpr std::uint32_t datum_data=0x44u,player_size=0x21cu,timer_count=2u,timer_size=2u;
namespace player {inline constexpr std::uint32_t timers=0x198u,unit=0x2cu;}

namespace player_timers {
enum class Access {one,players,player_data,timer,timer_store,players_again,player_data_again,unit};
enum class Outcome {ticked};
// The original's EBX, ESI, EBP and EDI across D3340: the timer's index and
// address, the timers left and the player's offset.
struct Kept {std::uint32_t index,timer,remaining,offset;};
// Frame:
//   expired(unit, factor, kept)   D3340(EAX the unit, XMM0 the factor), refreshes kept
//   next(kept)                    the next timer's registers
template<class Memory,class Frame> Outcome tick(Memory &memory,Frame &frame,std::uint32_t player) {
    using enum Access;
    const auto factor=memory.load32(one,globals::one);
    const auto offset=player_size*(player&0xffffu);
    const auto player_address=memory.load32(player_data,at(memory.load32(players,globals::players),datum_data))+offset;
    Kept kept{0u,at(player_address,player::timers),timer_count,offset};
    do {
        const auto value=memory.load16(timer,kept.timer);
        if(std::int16_t(value)>0) {
            const auto next=(value-1u)&0xffffu;
            memory.store16(timer_store,kept.timer,next);
            if(!next && !kept.index) {
                const auto data=memory.load32(player_data_again,at(memory.load32(players_again,globals::players),datum_data));
                frame.expired(memory.load32(unit,data+kept.offset+player::unit),factor,kept);
            }
        }
        ++kept.index;kept.timer+=timer_size;--kept.remaining;
        frame.next(kept);
    } while(kept.remaining);
    return Outcome::ticked;
}
} // namespace player_timers
} // namespace halo2::native_player_timers
