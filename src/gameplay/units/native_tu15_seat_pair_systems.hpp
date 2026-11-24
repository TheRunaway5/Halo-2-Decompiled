#pragma once
// Native TU1.5 seat pair check, entered at D3A80 (physics live support
// unit): an object with two chosen seats whose occupants are checked.
//
//   seat_pair::check   D3A80(object handle) -> RET 4
//
// The object names two seats by byte (+0x212 and +0x213, 0xFF for none);
// with either unnamed nothing happens. A seat's occupant is the handle in
// the object's seats (+0x218 + 4 * seat, the seat as a signed byte widened
// to a word) or NONE for the seat -1. Both occupants go to 106710 (EAX the
// occupant), the second only when the first answers; unless both answer,
// E8E10 gets the object (pushed, ECX 0x13).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_seat_pair.hpp) maps the role to the original instruction and
// owns the ABI of the children; the handle and the second occupant the
// original keeps in EBP and ESI across them are a Kept, refreshed from the
// registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_seat_pair {
using native_object_update::at;
using native_object_update::none;
using native_object_update::widen16;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u;}   // DataArray* of 12-byte object headers
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u;
inline constexpr std::uint32_t first_seat_byte=0x212u,second_seat_byte=0x213u,seats=0x218u,unnamed=0xffu,notice=0x13u;

namespace seat_pair {
enum class Access {headers,handle,datums,object,first_named,second_named,datums_again,object_again,first_seat,first_occupant,
    datums_last,object_last,second_seat,second_occupant};
enum class Outcome {unnamed_seat,both_answer,first_fails,second_fails};
struct Kept {std::uint32_t handle,second;};   // EBP, ESI
constexpr std::uint32_t widen_byte_to_word(std::uint32_t v){return std::uint32_t(std::int16_t(std::int8_t(std::uint8_t(v))))&0xffffu;}
// Frame: argument() the handle's slot; answer(kept, occupant) 106710,
// returns AL; notify(kept) E8E10. Each refreshes kept.
template<class Memory,class Frame> Outcome check(Memory &memory,Frame &frame) {
    using enum Access;
    const auto table=memory.load32(headers,globals::object_headers);
    Kept kept{memory.load32(handle,frame.argument()),none};
    const auto entry=header_size*(kept.handle&0xffffu)+header_address;
    const auto object_address=memory.load32(object,memory.load32(datums,at(table,datum_data))+entry);
    if((memory.load8(first_named,at(object_address,first_seat_byte))&0xffu)==unnamed)return Outcome::unnamed_seat;
    if((memory.load8(second_named,at(object_address,second_seat_byte))&0xffu)==unnamed)return Outcome::unnamed_seat;
    // The occupant of a seat named by a byte, the object read again each time.
    const auto occupant=[&](Access datums_role,Access object_role,Access seat_role,Access occupant_role,std::uint32_t seat_offset) {
        const auto record=memory.load32(object_role,memory.load32(datums_role,at(table,datum_data))+entry);
        const auto seat=widen_byte_to_word(memory.load8(seat_role,at(record,seat_offset)));
        if(seat==0xffffu)return none;
        return memory.load32(occupant_role,record+seats+4u*std::uint32_t(std::int32_t(std::int16_t(seat))));
    };
    const auto first=occupant(datums_again,object_again,first_seat,first_occupant,first_seat_byte);
    kept.second=occupant(datums_last,object_last,second_seat,second_occupant,second_seat_byte);
    if(!frame.answer(kept,first)){frame.notify(kept);return Outcome::first_fails;}
    if(!frame.answer(kept,kept.second)){frame.notify(kept);return Outcome::second_fails;}
    return Outcome::both_answer;
}
} // namespace seat_pair
} // namespace halo2::native_seat_pair
