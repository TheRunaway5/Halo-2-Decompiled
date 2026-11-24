#pragma once
// Native TU1.5 unit speed scale, entered at DFD20 (physics live support
// unit): the signed scale a unit moves at.
//
//   speed_scale::of   DFD20(EAX = unit handle) -> XMM0
//
// The base is the unit's tag (+0x250, the tag instance the unit's tag
// handle names in [4F0414]); a unit with a controller (+0x12C) whose record
// ([5020C8], 0x888 bytes, +0x858) is live has the base adjusted in place by
// 2647E0. The base is then scaled by the global at [51DF54]+4 and limited
// by the unit's fraction f (+0x2C4): a unit with flag bit 23 (+0x134) moves
// forward at no more than 1 - f, any other backward at no more than f (the
// result is negative). A unit in state 6 (+0x3DC) that is active (+0x3F8)
// with bit 13 of +0x348 instead moves at 1 - f when that is below 1, and
// at 1 otherwise.
//
// The single-precision arithmetic is the original's scalar SSE, in its
// order, on the live MXCSR. This is the game logic only. The game's memory
// is the retained TU1.5 bank, reached through a Memory object with typed
// loads and stores; every access names its role (Access), and the boundary
// adapter (native_tu15_unit_speed_scale.hpp) maps the role to the original
// instruction.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include <cstdint>

namespace halo2::native_unit_speed_scale {
using native_object_update::at;
using native_object_update::none;
namespace sse=native_aim_clusters::sse;
namespace globals {
inline constexpr std::uint32_t object_headers=0x4ecbd0u;   // DataArray*: the objects
inline constexpr std::uint32_t tag_instances=0x4f0414u;    // the tag instance table (16 bytes, +8 the tag's address)
inline constexpr std::uint32_t controllers=0x5020c8u;      // DataArray*: the controller records
inline constexpr std::uint32_t speed=0x51df54u;            // the speed globals (+4 the scale)
inline constexpr std::uint32_t one=0x46a0d0u;              // float 1
}
inline constexpr std::uint32_t datums_offset=0x44u,element_size=12u,element_address=8u;
inline constexpr std::uint32_t tag_instance_size=16u,tag_instance_address=8u;
inline constexpr std::uint32_t controller_size=0x888u,controller_record=0x858u;
inline constexpr std::uint32_t speed_scale_offset=4u;
inline constexpr std::uint32_t zero_bits=0u;
namespace unit {
inline constexpr std::uint32_t tag=0u,controller=0x12cu,flags=0x134u,fraction=0x2c4u,state=0x3dcu,active=0x3f8u,
    bits=0x348u;
inline constexpr std::uint32_t forward_bit=23u,override_bit=13u,override_state=6u;
}
namespace definition {inline constexpr std::uint32_t base_scale=0x250u;}

namespace speed_scale {
enum class Local {scale};
enum class Access {
    object_table,object_datums,tag_table,object_address,object_tag,tag_address,controller,base,scale_init,
    controller_table,controller_datums,controller_check,flags,speed_globals,speed_scale,scale_read,one,
    forward_fraction,backward_compare,backward_fraction,state,active,bits,override_fraction
};
enum class Limit {forward_capped,forward,backward_capped,backward};
enum class Override {other_state,inactive,clear,ceiling,one};
struct Outcome {std::uint32_t value;Limit limit;Override override_kind;};
// Frame:
//   adjust(kept)            2647E0(&scale); refreshes the unit (ESI)
//   compared(a,b)           COMISS a,b (the flags)
//   limited(value,xmm1)     the limit's XMM0 and XMM1
//   overridden(value)       the override's XMM0
//   local(Local)
struct Kept {std::uint32_t unit;};
template<class Memory,class Frame> Outcome of(Memory &memory,Frame &frame,std::uint32_t handle) {
    using enum Access;
    const auto headers=memory.load32(object_table,globals::object_headers);
    const auto datums=memory.load32(object_datums,at(headers,datums_offset));
    const auto tags=memory.load32(tag_table,globals::tag_instances);
    Kept kept{memory.load32(object_address,datums+element_size*(handle&0xffffu)+element_address)};
    const auto tag_handle=memory.load32(object_tag,at(kept.unit,unit::tag));
    const auto definition_address=memory.load32(tag_address,tags+tag_instance_size*(tag_handle&0xffffu)+tag_instance_address);
    const auto controller_handle=memory.load32(controller,at(kept.unit,unit::controller));
    memory.store32(scale_init,frame.local(Local::scale),memory.load32(base,at(definition_address,definition::base_scale)));
    if(controller_handle!=none) {
        const auto records=memory.load32(controller_table,globals::controllers);
        const auto record_datums=memory.load32(controller_datums,at(records,datums_offset));
        if(memory.load32(controller_check,controller_size*(controller_handle&0xffffu)+controller_record+record_datums)!=none)
            frame.adjust(kept);
    }
    const auto flags_value=memory.load32(flags,at(kept.unit,unit::flags));
    const auto speed_address=memory.load32(speed_globals,globals::speed);
    auto value=memory.load32(speed_scale,at(speed_address,speed_scale_offset));
    value=sse::multiply(value,memory.load32(scale_read,frame.local(Local::scale)));
    const auto one_value=memory.load32(one,globals::one);
    Outcome outcome{};
    if((flags_value>>unit::forward_bit)&1u) {
        // Forward, at no more than 1 - f.
        const auto ceiling=sse::subtract(one_value,memory.load32(forward_fraction,at(kept.unit,unit::fraction)));
        frame.compared(value,ceiling);
        const bool capped=!sse::not_above(value,ceiling);
        if(capped)value=ceiling;
        outcome.limit=capped?Limit::forward_capped:Limit::forward;
        frame.limited(value,ceiling);
    } else {
        // Backward, at no more than f.
        const auto fraction=memory.load32(backward_compare,at(kept.unit,unit::fraction));
        frame.compared(value,fraction);
        const bool capped=!sse::not_above(value,fraction);
        const auto limit=capped?memory.load32(backward_fraction,at(kept.unit,unit::fraction)):value;
        value=sse::subtract(zero_bits,limit);
        outcome.limit=capped?Limit::backward_capped:Limit::backward;
        frame.limited(value,limit);
    }
    outcome.value=value;outcome.override_kind=Override::other_state;
    if(memory.load8(state,at(kept.unit,unit::state))!=unit::override_state)return outcome;
    outcome.override_kind=Override::inactive;
    if(memory.load8(active,at(kept.unit,unit::active))==0)return outcome;
    outcome.override_kind=Override::clear;
    if(!((memory.load16(bits,at(kept.unit,unit::bits))>>unit::override_bit)&1u))return outcome;
    // The override: 1 - f when that is below 1, else 1.
    const auto ceiling=sse::subtract(one_value,memory.load32(override_fraction,at(kept.unit,unit::fraction)));
    frame.compared(one_value,ceiling);
    const bool below_one=!sse::not_above(one_value,ceiling);
    outcome.value=below_one?ceiling:one_value;
    outcome.override_kind=below_one?Override::ceiling:Override::one;
    frame.overridden(outcome.value);
    return outcome;
}
} // namespace speed_scale
} // namespace halo2::native_unit_speed_scale
