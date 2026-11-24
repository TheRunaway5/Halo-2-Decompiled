#pragma once
// Native TU1.5 player queries with one child:
//
//   uncontrolled_unit::test   C6E50(ECX = unit handle) -> AL
//       (physics live support) a unit with no controlling player (+0x13C
//       NONE) answers 1; one without a parent (+0x14 NONE) answers 0; for a
//       parented unit C6DD0 decides: an answer clears the result, none
//       leaves it set
//   player_live::test         15B470(ESI = player handle) -> AL
//       (physics live support, death cycle) a player (0x21C bytes) answers
//       1 when it is active (flags word +2 bit 0), not leaving (bit 1
//       clear), its byte +0xC0 is set (not 0xFF), and it has a unit
//       (+0x2C) or, without bit 14, 160570 (ECX the player) answers 0
//
// Both keep a byte in BL across their child that is the result on some
// paths, so the BL each child returns counts.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapters
// (native_tu15_uncontrolled_unit.hpp, native_tu15_player_live.hpp) map the
// role to the original instruction and own the ABI of the child; the values
// the original keeps in callee-saved registers across it are a Kept,
// refreshed from the registers it returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_player_queries {
using native_object_update::ObjectData;
using native_object_update::at;
using native_object_update::none;
namespace globals {
inline constexpr std::uint32_t object_headers=0x4ecbd0u;   // DataArray* of 12-byte object headers
inline constexpr std::uint32_t players=0x4f54f4u;          // DataArray* of 0x21C-byte players
}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u,unit_player=0x13cu;

namespace uncontrolled_unit {
enum class Access {headers,header_datums,object_address,player,parent};
enum class Outcome {no_player,unparented,answered,unanswered};
struct Kept {std::uint32_t result;};   // BL
struct Result {Outcome outcome;std::uint32_t value;};
// Frame: ask(kept) C6DD0, refreshes kept, returns AL.
template<class Memory,class Frame> Result test(Memory &memory,Frame &frame,std::uint32_t handle) {
    using enum Access;
    const auto table=memory.load32(headers,globals::object_headers);
    const auto datums=memory.load32(header_datums,at(table,datum_data));
    const auto unit=memory.load32(object_address,datums+header_size*(handle&0xffffu)+header_address);
    Kept kept{1u};
    if(memory.load32(Access::player,at(unit,unit_player))==none)return {Outcome::no_player,kept.result};
    if(memory.load32(Access::parent,at(unit,offsetof(ObjectData,parent)))==none)return {Outcome::unparented,0u};
    if(frame.ask(kept))return {Outcome::answered,0u};
    return {Outcome::unanswered,kept.result};
}
} // namespace uncontrolled_unit

namespace player_live {
namespace player {inline constexpr std::uint32_t size=0x21cu,flags=2u,marker=0xc0u,unit=0x2cu;}
namespace player_flags {inline constexpr std::uint32_t active=1u<<0,leaving=1u<<1,barred=1u<<14;}
inline constexpr std::uint32_t unset_marker=0xffu;
enum class Access {players,player_datums,flags,marker,unit};
enum class Outcome {inactive,leaving,unmarked,has_unit,barred,unanswered,answered};
struct Kept {std::uint32_t result;};   // BL
struct Result {Outcome outcome;std::uint32_t value;};
// Frame: ask(kept, player) 160570, refreshes kept, returns AL.
template<class Memory,class Frame> Result test(Memory &memory,Frame &frame,std::uint32_t handle) {
    using enum Access;
    const auto array=memory.load32(players,globals::players);
    const auto data=memory.load32(player_datums,at(array,datum_data));
    const auto record=player::size*(handle&0xffffu)+data;
    const auto flags_value=memory.load16(Access::flags,at(record,player::flags))&0xffffu;
    Kept kept{0u};
    if(!(flags_value&player_flags::active))return {Outcome::inactive,kept.result};
    if(flags_value&player_flags::leaving)return {Outcome::leaving,kept.result};
    if((memory.load8(marker,at(record,player::marker))&0xffu)==unset_marker)return {Outcome::unmarked,kept.result};
    if(memory.load32(Access::unit,at(record,player::unit))!=none)return {Outcome::has_unit,1u};
    if(flags_value&player_flags::barred)return {Outcome::barred,kept.result};
    if(frame.ask(kept,handle)==0)return {Outcome::unanswered,1u};
    return {Outcome::answered,kept.result};
}
} // namespace player_live
} // namespace halo2::native_player_queries
