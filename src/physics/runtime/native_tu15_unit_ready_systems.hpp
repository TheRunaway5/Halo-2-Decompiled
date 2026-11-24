#pragma once
// Native TU1.5 unit readiness, entered at E65A0 (physics live support
// unit): whether a unit is settled enough to act.
//
//   unit_ready::ready   E65A0(EAX = unit handle) -> EAX
//
// The unit must not have bit 0 of its flags +0xC1 set and must have bit 0 of
// +0x348; its count (signed byte +0x399) must have reached the game's ticks
// per second (the word +2 of 51DF54) times 46A458, rounded to an integer on
// the x87 (FLD m32, FISTP m32 under the live control word); its state
// (+0x3DC) must be 1 or 3; and it must have no parent (+0x14 NONE).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_unit_ready.hpp) maps the role to the original instruction
// and rounds on the logical x87.
#include "gameplay/units/native_tu15_record_fade_systems.hpp"
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_unit_ready {
using native_object_update::at;
using native_object_update::none;
using native_object_update::object_of;
using native_object_update::widen16;
namespace sse=native_aim_clusters::sse;
namespace scalar=native_record_fade::scalar;
namespace globals {
inline constexpr std::uint32_t game_time=0x51df54u;   // +2: ticks per second (int16)
inline constexpr std::uint32_t settle_seconds=0x46a458u;   // float
}
namespace unit {
inline constexpr std::uint32_t flags=0xc1u,ready_flags=0x348u,count=0x399u,state=0x3dcu,parent=0x14u;
}

namespace unit_ready {
enum class Local {scaled,rounded};
enum class Access {
    object_table,object_datums,object_address,flags,ready_flags,game_time,ticks,settle_seconds,scaled_store,
    scaled_x87,rounded_store,count,rounded_read,state,parent
};
// Frame:
//   local(Local)        a local's address now
//   scaled(value)       the ticks times the seconds (XMM0)
//   rounded(value)      FLD m32; FISTP m32 on the logical x87
template<class Memory,class Frame> bool ready(Memory &memory,Frame &frame,std::uint32_t handle) {
    using enum Access;
    const auto local=[&](Local l){return frame.local(l);};
    const auto unit_data=object_of(memory,handle,object_table,object_datums,object_address);
    if(memory.load8(flags,at(unit_data,unit::flags))&1u)return false;
    if(!(memory.load8(ready_flags,at(unit_data,unit::ready_flags))&1u))return false;
    const auto time=memory.load32(game_time,globals::game_time);
    const auto ticks_value=widen16(memory.load16(ticks,at(time,2u)));
    const auto ticks_float=scalar::from_integer(ticks_value);
    const auto scaled_value=sse::multiply(ticks_float,memory.load32(settle_seconds,globals::settle_seconds));
    frame.scaled(scaled_value);
    memory.store32(scaled_store,local(Local::scaled),scaled_value);
    memory.store32(rounded_store,local(Local::rounded),frame.rounded(memory.load32(scaled_x87,local(Local::scaled))));
    const auto count_value=std::int32_t(std::int8_t(memory.load8(count,at(unit_data,unit::count))));
    if(count_value<std::int32_t(memory.load32(rounded_read,local(Local::rounded))))return false;
    const auto state_value=memory.load8(state,at(unit_data,unit::state));
    if(state_value!=1u && state_value!=3u)return false;
    return memory.load32(parent,at(unit_data,unit::parent))==none;
}
} // namespace unit_ready
} // namespace halo2::native_unit_ready
