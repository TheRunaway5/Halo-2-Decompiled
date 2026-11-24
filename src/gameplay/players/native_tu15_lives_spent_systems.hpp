#pragma once
// Native TU1.5 lives spent, entered at 160570 (physics live support and death
// cycle units): whether a dead player has no lives left.
//
//   lives_spent::query   160570(ECX = player) -> AL
//
// In a game with lives (+0x1B8 of [4F3218], above 0), a player (0x21C bytes,
// through the players at [4F54F4]) without a unit (+0x2C NONE) and with no
// lives left (word +0x1AC zero) has spent them: the answer is 1, else 0.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads; every access names its
// role (Access); the boundary adapter (native_tu15_lives_spent.hpp) maps the
// role to the original instruction and owns the ABI.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_lives_spent {
using native_object_update::at;
using native_object_update::none;
using native_object_update::signed_value;
namespace globals {inline constexpr std::uint32_t game=0x4f3218u,players=0x4f54f4u;}
inline constexpr std::uint32_t datum_data=0x44u,player_size=0x21cu;
namespace game {inline constexpr std::uint32_t lives=0x1b8u;}
namespace player {inline constexpr std::uint32_t unit=0x2cu,lives=0x1acu;}

namespace lives_spent {
enum class Access {game,lives_limit,players_array,players_data,unit,lives};
enum class Outcome {unlimited,alive,lives_left,spent};
// Frame: saving() PUSH ESI; limit_tested(limit) XOR AL,AL, TEST; indexing(index) AND, IMUL;
// unit_tested(unit, scaled, data) ADD ECX,ESI, CMP; lives_tested(lives) CMP word.
template<class Memory,class Frame> std::uint32_t query(Memory &memory,Frame &frame,std::uint32_t player_handle,Outcome &outcome) {
    using enum Access;
    const auto game_address=memory.load32(Access::game,globals::game);
    frame.saving();
    const auto limit=memory.load32(lives_limit,at(game_address,game::lives));
    frame.limit_tested(limit);
    if(signed_value(limit)<=0){outcome=Outcome::unlimited;return 0u;}
    const auto data=memory.load32(players_data,at(memory.load32(players_array,globals::players),datum_data));
    const auto index=player_handle&0xffffu;
    frame.indexing(index);
    const auto scaled=index*player_size;
    const auto player=scaled+data;
    const auto unit_value=memory.load32(unit,at(player,player::unit));
    frame.unit_tested(unit_value,scaled,data);
    if(unit_value!=none){outcome=Outcome::alive;return 0u;}
    const auto lives_value=memory.load16(Access::lives,at(player,player::lives));
    frame.lives_tested(lives_value);
    if(lives_value){outcome=Outcome::lives_left;return 0u;}
    outcome=Outcome::spent;
    return 1u;
}
} // namespace lives_spent
} // namespace halo2::native_lives_spent
