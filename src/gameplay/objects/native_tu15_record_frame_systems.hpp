#pragma once
// Native TU1.5 record frame, entered at 1DC610 (physics live support unit):
// the frame a record is taken at, handed on.
//
//   record_frame::take   1DC610(EDI = record; argument) -> RET 4
//
// A record whose marker (signed byte +0x18) is one of its markers (+0x74)
// is taken at that marker's frame (1D8B90); any other at its object's
// (+8, BC190). 1DC680 then takes the record, the frame (the original's
// 0x34-byte local) and the argument.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_record_frame.hpp) maps the role to the original instruction
// and owns the ABI of the children; the record, which the original keeps in
// EDI across them, is refreshed from the register each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_record_frame {
using native_object_update::at;
using native_object_update::signed_value;
using native_object_update::widen8;
namespace record {inline constexpr std::uint32_t object=8u,marker=0x18u,markers=0x74u;}

namespace record_frame {
enum class Access {marker,markers,object,argument};
enum class Outcome {marked,unmarked,past_markers};
// Frame:
//   marker_frame(record, marker) -> record   1D8B90 into the frame
//   object_frame(record, object) -> record   BC190 into the frame
//   argument()                               the argument's slot now
//   take(record, argument)                   1DC680
template<class Memory,class Frame> Outcome take(Memory &memory,Frame &frame,std::uint32_t record_address) {
    using enum Access;
    const auto marker_value=signed_value(widen8(memory.load8(Access::marker,at(record_address,record::marker))));
    Outcome outcome=Outcome::unmarked;
    if(marker_value>=0) {
        outcome=marker_value<signed_value(memory.load32(Access::markers,at(record_address,record::markers)))?Outcome::marked:Outcome::past_markers;
    }
    if(outcome==Outcome::marked)record_address=frame.marker_frame(record_address,std::uint32_t(marker_value));
    else record_address=frame.object_frame(record_address,memory.load32(object,at(record_address,record::object)));
    frame.take(record_address,memory.load32(argument,frame.argument()));
    return outcome;
}
} // namespace record_frame
} // namespace halo2::native_record_frame
