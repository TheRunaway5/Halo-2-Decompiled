#pragma once
// Native TU1.5 director value, entered at 189F20 (vehicle director unit): a
// director record's value, scaled by its vehicle's factor.
//
//   director_value::of   189F20(EAX = record index) -> XMM0
//
// A record (0x94 bytes at [4F9B54]) whose vehicle (+0x14, NONE for none)
// is present has its vehicle definition's value (+0xCC; the tag instances
// at [4F0414] by the vehicle's tag), kept twice in the frame across CAD60,
// which answers the vehicle's factor (EAX the vehicle, BX the record's word
// +0x42). A factor other than one divides the value; a quotient strictly
// between the bounds at 46A7C4 and 46A7C0 is the answer, else the kept
// value. A record without a vehicle answers the default at 55D6E4.
//
// The single-precision arithmetic is the original's scalar SSE on the live
// MXCSR. This is the game logic only. The game's memory is the retained
// TU1.5 bank, reached through a Memory object with typed loads and stores;
// every access names its role (Access), and the boundary adapter
// (native_tu15_director_value.hpp) maps the role to the original instruction
// and owns the ABI of the child.
#include "physics/runtime/native_tu15_perpendicular_systems.hpp"
#include <cstdint>

namespace halo2::native_director_value {
using native_object_update::at;
using native_object_update::none;
using native_perpendicular::unequal;
namespace sse=native_aim_clusters::sse;
namespace globals {
inline constexpr std::uint32_t records=0x4f9b54u;      // 0x94-byte director records
inline constexpr std::uint32_t fallback=0x55d6e4u;     // float: the value without a vehicle
inline constexpr std::uint32_t objects=0x4ecbd0u;      // DataArray* of 12-byte rows (+8 the object)
inline constexpr std::uint32_t tags=0x4f0414u;         // 16-byte tag instances (+8 the definition)
inline constexpr std::uint32_t one=0x46a0d0u,lower=0x46a7c4u,upper=0x46a7c0u;
}
inline constexpr std::uint32_t record_size=0x94u,datum_data=0x44u,row_size=12u,row_object=8u,tag_size=16u,tag_definition=8u;
namespace record {inline constexpr std::uint32_t vehicle=0x14u,word=0x42u;}
namespace definition {inline constexpr std::uint32_t value=0xccu;}

namespace director_value {
enum class Access {records,fallback,vehicle,word,objects,object_data,object,tag,tags,definition,value,kept_store,kept_again_store,
    one,kept,lower,upper,kept_again};
enum class Outcome {unbound,unscaled,out_of_bounds,scaled};
// Frame:
//   local(k)                     the frame's two floats now
//   factor(vehicle, word) -> XMM0   CAD60
//   divided(quotient)            the quotient's registers
//   answered(value)              XMM0 the answer
template<class Memory,class Frame> Outcome of(Memory &memory,Frame &frame,std::uint32_t index) {
    using enum Access;
    const auto record_array=memory.load32(Access::records,globals::records);
    const auto fallback_value=memory.load32(Access::fallback,globals::fallback);
    const auto record_address=record_array+record_size*index;
    const auto vehicle_handle=memory.load32(vehicle,at(record_address,record::vehicle));
    if(vehicle_handle==none){frame.answered(fallback_value);return Outcome::unbound;}
    const auto word_value=memory.load16(word,at(record_address,record::word));
    const auto data=memory.load32(object_data,at(memory.load32(objects,globals::objects),datum_data));
    const auto object_address=memory.load32(object,data+row_size*(vehicle_handle&0xffffu)+row_object);
    const auto tag_handle=memory.load32(tag,at(object_address,0u));
    const auto tag_array=memory.load32(tags,globals::tags);
    const auto definition_address=memory.load32(Access::definition,tag_array+tag_size*(tag_handle&0xffffu)+tag_definition);
    const auto base=memory.load32(Access::value,at(definition_address,definition::value));
    memory.store32(kept_store,frame.local(0),base);
    memory.store32(kept_again_store,frame.local(1),base);
    const auto factor=frame.factor(vehicle_handle,word_value);
    if(unequal(factor,memory.load32(one,globals::one))) {
        const auto quotient=sse::divide(memory.load32(kept,frame.local(0)),factor);
        frame.divided(quotient);
        if(!sse::not_above(quotient,memory.load32(lower,globals::lower)) &&
           !sse::not_above(memory.load32(upper,globals::upper),quotient)) {
            frame.answered(quotient);
            return Outcome::scaled;
        }
        frame.answered(memory.load32(kept_again,frame.local(1)));
        return Outcome::out_of_bounds;
    }
    frame.answered(memory.load32(kept_again,frame.local(1)));
    return Outcome::unscaled;
}
} // namespace director_value
} // namespace halo2::native_director_value
