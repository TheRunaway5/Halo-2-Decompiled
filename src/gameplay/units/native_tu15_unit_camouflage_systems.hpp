#pragma once
// Native TU1.5 unit camouflage, entered at C8CF0 (physics live support
// unit): a unit's camouflage and second fade, one tick on.
//
//   unit_camouflage::update   C8CF0(EAX = unit handle)
//
// A player's unit (+0x12C not NONE) in an alliance game (mode 1, the game
// globals' +8) with the session's byte 5020B6 set is camouflaged (flags
// +0x134 bit 3) when its team (int16 +0x138) is not one of the 16, or the
// alliance bits ([5020C4] +0xC4, C1E90) do not ally team 1 with it.
// The camouflage (+0x2B0) rises while camouflaged, and falls otherwise, by
// its rate (+0x2B8) times the tick's length ([51DF54] +4); reaching one or
// zero clamps it there and tells the object's attachment (+0xD4, unless
// NONE) through B7E50 (the 0x800000 notice). The second fade (+0x2B4)
// rises while flag bit 4 is set, and falls otherwise, by the tick's length
// times the fixed rate at 46A174, clamped to one and zero.
//
// The single-precision arithmetic is the original's scalar SSE on the live
// MXCSR. This is the game logic only. The game's memory is the retained
// TU1.5 bank, reached through a Memory object with typed loads and stores;
// every access names its role (Access), and the boundary adapter
// (native_tu15_unit_camouflage.hpp) maps the role to the original
// instruction and owns the ABI of the children; the object table, the
// unit's row, the unit, the tick's length and the zero and one the
// original keeps in EBP, EDI, ESI, EBX, XMM1 and XMM2 across them are a
// Kept, refreshed from the registers each returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include <cstdint>

namespace halo2::native_unit_camouflage {
using native_object_update::at;
using native_object_update::none;
using native_object_update::widen16;
using native_object_update::signed_value;
namespace sse=native_aim_clusters::sse;
namespace globals {
inline constexpr std::uint32_t objects=0x4ecbd0u;        // DataArray* of 12-byte object rows (+8 the object)
inline constexpr std::uint32_t game_state=0x4f3218u;     // GameGlobals* (+8 the game mode)
inline constexpr std::uint32_t session_flag=0x5020b6u;   // byte
inline constexpr std::uint32_t alliances=0x5020c4u;      // +0xC4: the alliance bits
inline constexpr std::uint32_t one=0x46a0d0u;            // float 1
inline constexpr std::uint32_t time=0x51df54u;           // +4: the tick's length (float)
inline constexpr std::uint32_t second_rate=0x46a174u;    // float
}
inline constexpr std::uint32_t datum_data=0x44u,row_size=12u,row_object=8u,mode=8u,alliance_mode=1u,alliance_bits=0xc4u,
    teams=16u,team_one_row=16u,tick_length=4u,attachment_notice=0x800000u,zero_bits=0u;
namespace unit {inline constexpr std::uint32_t player=0x12cu,flags=0x134u,team=0x138u,camouflage=0x2b0u,second=0x2b4u,rate=0x2b8u;}
namespace object {inline constexpr std::uint32_t attachment=0xd4u;}
namespace flag {inline constexpr std::uint32_t camouflaged=0x08u,second_rising=0x10u;}

namespace unit_camouflage {
enum class Access {objects,object_data,object,player,game_state,game_mode,session_flag,team,alliances,forced_flags,forced_store,
    flags,one,time,rate,tick,camouflage,camouflage_store,camouflage_clamp,object_data_again,object_again,attachment,
    second_flags,second_tick,second_rate,second,second_store,second_clamp};
struct Kept {std::uint32_t objects,row,unit,tick,zero,one;};   // EBP, EDI, ESI, EBX, XMM1, XMM2
// Frame:
//   allied(kept, bit, bits) -> bool   C1E90(EDX = bit; bits), refreshes kept
//   stepped(step)                     a fade's step (rate times the tick's length)
//   ticked(kept)                      the tick's length kept (ADD EBX,4)
//   notify(kept, handle, notice)      B7E50(handle, notice), refreshes kept
template<class Memory,class Frame> void update(Memory &memory,Frame &frame,std::uint32_t handle) {
    using enum Access;
    const auto objects_array=memory.load32(Access::objects,globals::objects);
    Kept kept{objects_array,row_size*(handle&0xffffu),0u,0u,0u,0u};
    kept.unit=memory.load32(object,memory.load32(object_data,at(objects_array,datum_data))+kept.row+row_object);
    // An unallied player's unit is camouflaged.
    if(memory.load32(player,at(kept.unit,unit::player))!=none) {
        const auto state=memory.load32(game_state,globals::game_state);
        if(memory.load32(game_mode,at(state,mode))==alliance_mode && (memory.load8(session_flag,globals::session_flag)&0xffu)) {
            const auto team_value=signed_value(widen16(memory.load16(team,at(kept.unit,unit::team))));
            const bool unallied=team_value<0 || team_value>=std::int32_t(teams) ||
                !frame.allied(kept,team_one_row+std::uint32_t(team_value),memory.load32(Access::alliances,globals::alliances)+alliance_bits);
            if(unallied)memory.store32(forced_store,at(kept.unit,unit::flags),memory.load32(forced_flags,at(kept.unit,unit::flags))|flag::camouflaged);
        }
    }
    // The camouflage, toward one or zero by its rate.
    const auto flags_value=memory.load32(flags,at(kept.unit,unit::flags));
    kept.zero=zero_bits;
    kept.one=memory.load32(one,globals::one);
    const auto time_value=memory.load32(Access::time,globals::time);
    const bool rising=flags_value&flag::camouflaged;
    const auto rate_value=memory.load32(rate,at(kept.unit,unit::rate));
    const auto step=sse::multiply(rate_value,memory.load32(tick,at(time_value,tick_length)));
    frame.stepped(step);
    const auto current=memory.load32(camouflage,at(kept.unit,unit::camouflage));
    kept.tick=at(time_value,tick_length);
    frame.ticked(kept);
    const auto value=rising?sse::add(step,current):sse::subtract(current,step);
    const bool clamped=rising?!sse::not_above(value,kept.one):!sse::not_above(kept.zero,value);
    memory.store32(camouflage_store,at(kept.unit,unit::camouflage),value);
    if(clamped) {
        memory.store32(camouflage_clamp,at(kept.unit,unit::camouflage),rising?kept.one:kept.zero);
        const auto object_address=memory.load32(object_again,memory.load32(object_data_again,at(kept.objects,datum_data))+kept.row+row_object);
        const auto attached=memory.load32(attachment,at(object_address,object::attachment));
        if(attached!=none)frame.notify(kept,attached,attachment_notice);
    }
    // The second fade, toward one or zero by the fixed rate.
    if(memory.load32(second_flags,at(kept.unit,unit::flags))&flag::second_rising) {
        const auto tick_value=memory.load32(second_tick,kept.tick);
        const auto second_step=sse::multiply(tick_value,memory.load32(Access::second_rate,globals::second_rate));
        frame.stepped(second_step);
        const auto second_value=sse::add(second_step,memory.load32(second,at(kept.unit,unit::second)));
        const bool full=!sse::not_above(second_value,kept.one);
        memory.store32(second_store,at(kept.unit,unit::second),second_value);
        if(full)memory.store32(second_clamp,at(kept.unit,unit::second),kept.one);
    } else {
        const auto tick_value=memory.load32(second_tick,kept.tick);
        const auto second_current=memory.load32(second,at(kept.unit,unit::second));
        const auto second_step=sse::multiply(tick_value,memory.load32(Access::second_rate,globals::second_rate));
        frame.stepped(second_step);
        const auto second_value=sse::subtract(second_current,second_step);
        const bool gone=!sse::not_above(kept.zero,second_value);
        memory.store32(second_store,at(kept.unit,unit::second),second_value);
        if(gone)memory.store32(second_clamp,at(kept.unit,unit::second),kept.zero);
    }
}
} // namespace unit_camouflage
} // namespace halo2::native_unit_camouflage
