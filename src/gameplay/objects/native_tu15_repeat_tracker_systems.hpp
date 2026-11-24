#pragma once
// Native TU1.5 repeat tracker, entered at 1F0F90 (physics live support
// unit): a tracker of one (kind, handle) pair counting its repeats.
//
//   repeat_tracker::track   1F0F90(EAX = tracker*, ECX = source*)
//
// The source holds a kind word (+0x28), a level byte (+0x2A), a handle
// (+0x2C) and flags (+0x1C). The tracker holds a kind (+0), a handle (+4),
// a count (+8), a second word (+0xA) and a level (+0xC). A source whose
// kind (signed) or handle differs from the tracker's restarts it: the
// tracker takes the source's kind, handle and level, with both words zero.
// Then a tracker of kind 1, 3, 4, 5 or 6 counts one repeat when the
// source's flag bit 3 is set.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_repeat_tracker.hpp) maps the role to the original
// instruction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_repeat_tracker {
using native_object_update::at;
using native_object_update::widen16;
namespace source {inline constexpr std::uint32_t flags=0x1cu,kind=0x28u,level=0x2au,handle=0x2cu;}
namespace tracker {inline constexpr std::uint32_t kind=0u,handle=4u,count=8u,second=0xau,level=0xcu;}
inline constexpr std::uint32_t counted_kinds[]{1u,3u,4u,5u,6u};
inline constexpr std::uint32_t repeat_flag=1u<<3;

namespace repeat_tracker {
enum class Access {source_kind,tracker_kind,source_handle,tracker_handle,kind_store,handle_read,handle_store,level,
    level_store,count_clear,second_clear,kind_check,source_flags,count,count_store};
enum class Outcome {uncounted,unflagged,counted};
// Frame: compared(kind, wanted) each kind tested.
template<class Memory,class Frame> Outcome track(Memory &memory,Frame &frame,std::uint32_t tracked,std::uint32_t from) {
    using enum Access;
    const auto kind=memory.load16(source_kind,at(from,source::kind))&0xffffu;
    const auto current=memory.load16(tracker_kind,at(tracked,tracker::kind))&0xffffu;
    const bool same=widen16(kind)==current &&
        memory.load32(source_handle,at(from,source::handle))==memory.load32(tracker_handle,at(tracked,tracker::handle));
    if(!same) {
        memory.store16(kind_store,at(tracked,tracker::kind),kind);
        memory.store32(handle_store,at(tracked,tracker::handle),memory.load32(handle_read,at(from,source::handle)));
        memory.store16(level_store,at(tracked,tracker::level),memory.load8(Access::level,at(from,source::level))&0xffu);
        memory.store16(count_clear,at(tracked,tracker::count),0u);
        memory.store16(second_clear,at(tracked,tracker::second),0u);
    }
    const auto now=memory.load16(kind_check,at(tracked,tracker::kind))&0xffffu;
    bool counted=false;
    for(const auto wanted:counted_kinds) {
        frame.compared(now,wanted);
        if(now==wanted){counted=true;break;}
    }
    if(!counted)return Outcome::uncounted;
    if(!(memory.load8(source_flags,at(from,source::flags))&repeat_flag))return Outcome::unflagged;
    memory.store16(count_store,at(tracked,tracker::count),memory.load16(count,at(tracked,tracker::count))+1u);
    return Outcome::counted;
}
} // namespace repeat_tracker
} // namespace halo2::native_repeat_tracker
