#pragma once
// Native TU1.5 seat mode, entered at 158340 (physics live support, campaign
// camera and map world support units): how a unit sits.
//
//   seat_mode::of   158340(EBX = unit; mode*) -> AX, RET 4
//
// The unit's seat record (BCDF0, kind 3) names its vehicle (+0x14). Without
// a record or a vehicle the mode is cleared and the answer is zero. With a
// biped or vehicle (type byte +0xAA 0 or 1), the seat (the vehicle
// definition's 0xB0-byte seats +0x1CC, by the record's int16 +0x1FC) whose
// flags carry bit 6 makes the mode 1 when C9550 accepts the unit, or 3 when
// the unit's name (111CD0) is 400004A, the answer then one. Otherwise the
// mode is 2 and the answer whether the seat's flags carry bit 4.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_seat_mode.hpp) maps the role to the original instruction and
// owns the ABI of the children; the unit, the record, the vehicle and the
// name, which the original keeps in EBX, EDI, ESI and EBP across them, are a
// Kept, refreshed from the registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_seat_mode {
using native_object_update::at;
using native_object_update::none;
using native_object_update::widen16;
namespace globals {
inline constexpr std::uint32_t objects=0x4ecbd0u;   // DataArray* of 12-byte rows (+8 the object)
inline constexpr std::uint32_t tags=0x4f0414u;      // 16-byte tag instances (+8 the definition)
}
inline constexpr std::uint32_t datum_data=0x44u,row_size=12u,row_object=8u,tag_size=16u,tag_definition=8u;
inline constexpr std::uint32_t record_kind=3u,seated_types=3u,seat_size=0xb0u,passenger_name=0x400004au;
namespace record {inline constexpr std::uint32_t vehicle=0x14u,seat=0x1fcu;}
namespace object {inline constexpr std::uint32_t type=0xaau;}
namespace definition {inline constexpr std::uint32_t seats=0x1ccu;}
namespace seat {inline constexpr std::uint32_t gunner=1u<<4,driver=1u<<6;}
inline constexpr std::uint32_t driving=1u,passenger=3u,other=2u;

namespace seat_mode {
enum class Access {local_clear,mode_slot,mode_clear,vehicle,objects,object_data,object,type,tag,tags,definition,seat_index,
    seats,seat_flags,local_gunner,mode_slot_again,local,mode_store};
enum class Outcome {unseated,driving,passenger,other};
struct Kept {std::uint32_t unit,record,vehicle,name;};   // EBX, EDI, ESI, EBP
// Frame:
//   argument(), local()          the mode's slot and the frame's word now
//   record(kept) -> EAX          BCDF0(EAX the unit; kind 3), refreshes kept
//   name(kept) -> EAX            111CD0(EAX the unit), refreshes kept
//   accepts(kept) -> AL          C9550(ECX the unit), refreshes kept
template<class Memory,class Frame> Outcome of(Memory &memory,Frame &frame,Kept &kept,std::uint32_t &answer) {
    using enum Access;
    memory.store32(local_clear,frame.local(),0u);
    kept.record=frame.record(kept);
    // The vehicle's register still holds its zero: the cleared mode and the unseated answer.
    memory.store16(mode_clear,memory.load32(mode_slot,frame.argument()),kept.vehicle);
    answer=kept.vehicle&0xffffu;
    if(!kept.record)return Outcome::unseated;
    const auto vehicle_handle=memory.load32(vehicle,at(kept.record,record::vehicle));
    if(vehicle_handle==none)return Outcome::unseated;
    const auto data=memory.load32(object_data,at(memory.load32(objects,globals::objects),datum_data));
    kept.vehicle=memory.load32(object,data+row_size*(vehicle_handle&0xffffu)+row_object);
    kept.name=frame.name(kept);
    auto outcome=Outcome::other;
    if((1u<<(memory.load8(type,at(kept.vehicle,object::type))&31u))&seated_types) {
        const auto tag_handle=memory.load32(tag,at(kept.vehicle,0u));
        const auto definition_address=memory.load32(definition,memory.load32(tags,globals::tags)+tag_size*(tag_handle&0xffffu)+tag_definition);
        const auto index=widen16(memory.load16(seat_index,at(kept.record,record::seat)));
        const auto flags=memory.load32(seat_flags,memory.load32(seats,at(definition_address,definition::seats))+seat_size*index);
        if(flags&seat::gunner)memory.store32(local_gunner,frame.local(),1u);
        if(flags&seat::driver) {
            if(frame.accepts(kept)&0xffu)outcome=Outcome::driving;
            else if(kept.name==passenger_name)outcome=Outcome::passenger;
        }
    }
    const auto mode_address=memory.load32(mode_slot_again,frame.argument());
    if(outcome==Outcome::other) {
        answer=memory.load16(local,frame.local());
        memory.store16(mode_store,mode_address,other);
        return outcome;
    }
    memory.store16(mode_store,mode_address,outcome==Outcome::driving?driving:passenger);
    answer=1u;
    return outcome;
}
} // namespace seat_mode
} // namespace halo2::native_seat_mode
