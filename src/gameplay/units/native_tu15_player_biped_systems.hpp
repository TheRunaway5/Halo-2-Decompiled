#pragma once
// Native TU1.5 player biped query, entered at 152E00 (physics live support
// unit): whether a player's unit is a free-standing biped in state 1 that
// E65A0 does not answer for.
//
//   player_biped::test   152E00(EAX = player handle) -> AL
//
// The player (0x21C bytes in [4F54F4]) names its unit (+0x2C, NONE for
// none). The unit qualifies when its header's type (+3) is a biped
// (1 << type has bit 0), it has no parent (+0x14 NONE), its state (+0x3DC)
// is 1 and E65A0 (EAX the unit) answers 0.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_player_biped.hpp) maps the role to the original instruction
// and owns the ABI of E65A0.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_player_biped {
using native_object_update::ObjectData;
using native_object_update::ObjectHeader;
using native_object_update::at;
using native_object_update::none;
namespace globals {
inline constexpr std::uint32_t players=0x4f54f4u;          // DataArray* of 0x21C-byte players
inline constexpr std::uint32_t object_headers=0x4ecbd0u;   // DataArray* of 12-byte object headers
}
inline constexpr std::uint32_t datum_data=0x44u,player_size=0x21cu,player_unit=0x2cu,header_type=3u;
inline constexpr std::uint32_t unit_state=0x3dcu,wanted_state=1u,biped_types=1u<<0;

namespace player_biped {
enum class Access {players,player_datums,unit,headers,header_datums,type,object_address,parent,state};
enum class Outcome {no_unit,not_biped,parented,other_state,answered,qualifies};
// Frame: biped(type) the type tested; answer(unit) E65A0, returns AL.
template<class Memory,class Frame> Outcome test(Memory &memory,Frame &frame,std::uint32_t player) {
    using enum Access;
    const auto array=memory.load32(Access::players,globals::players);
    const auto data=memory.load32(player_datums,at(array,datum_data));
    const auto unit_handle=memory.load32(Access::unit,player_size*(player&0xffffu)+data+player_unit);
    if(unit_handle==none)return Outcome::no_unit;
    const auto table=memory.load32(headers,globals::object_headers);
    const auto header=memory.load32(header_datums,at(table,datum_data))+std::uint32_t(sizeof(ObjectHeader))*(unit_handle&0xffffu);
    const auto kind=memory.load8(Access::type,at(header,header_type))&0xffu;
    if(!((1u<<(kind&31u))&biped_types))return Outcome::not_biped;
    const auto object=memory.load32(object_address,at(header,offsetof(ObjectHeader,object)));
    if(memory.load32(parent,at(object,offsetof(ObjectData,parent)))!=none)return Outcome::parented;
    if((memory.load8(state,at(object,unit_state))&0xffu)!=wanted_state)return Outcome::other_state;
    return frame.answer(unit_handle)==0?Outcome::qualifies:Outcome::answered;
}
} // namespace player_biped
} // namespace halo2::native_player_biped
