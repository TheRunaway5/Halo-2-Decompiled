#pragma once
// Native TU1.5 state apply, entered at C92C0 (AI frame and physics live
// support units): a 0x7C-byte state (as C93D0 starts it) is written into a
// unit.
//
//   state_apply::apply   C92C0(EAX = unit handle, ECX = state)
//
// In the original's order: the point +0x14 becomes the unit's +0x1B0; the
// words +0x20 and +0x24 its +0x1C0 and +0x1C4; the byte +4 its +0x1BC; the
// pair +6 its +0x214 when the pair's first word already equals the unit's;
// the byte +0xA its +0x23D unless the word +0xA is NONE; the byte +0xC its
// +0x241; the word +0x10 its +0x148; the points +0x40, +0x34 and +0x28 its
// +0x180, +0x15C and +0x150; the kind +0 its +0x14C; the point +0x4C its
// +0x1A4; and the nine words from +0x58 its +0x1C8 (REP MOVSD).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_state_apply.hpp) maps the role to the original instruction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_state_apply {
using native_object_update::at;
using native_object_update::object_of;
inline constexpr std::uint32_t tail_words=9u;

namespace state_apply {
enum class Access {
    object_table,object_datums,object_address,position_read,position_store,timer_a_read,timer_a_store,timer_b_read,
    timer_b_store,mode_read,mode_store,pair_word,pair_current,pair_read,pair_store,choice_check,choice_read,choice_store,
    flag_read,flag_store,link_read,link_store,goal_read,goal_store,facing_read,facing_store,origin_read,origin_store,
    kind_read,kind_store,aim_read,aim_store,tail_read,tail_store
};
// A three-word point copied from the state to the unit.
struct Point {Access read,store;std::uint32_t from,to;};
inline constexpr Point position{Access::position_read,Access::position_store,0x14u,0x1b0u};
inline constexpr Point goal{Access::goal_read,Access::goal_store,0x40u,0x180u};
inline constexpr Point facing{Access::facing_read,Access::facing_store,0x34u,0x15cu};
inline constexpr Point origin{Access::origin_read,Access::origin_store,0x28u,0x150u};
inline constexpr Point aim{Access::aim_read,Access::aim_store,0x4cu,0x1a4u};
// Frame:
//   point(p)    a point's copy starts (EDX the source, ESI the target)
//   tail()      the REP MOVSD starts
template<class Memory,class Frame> void apply(Memory &memory,Frame &frame,std::uint32_t handle,std::uint32_t state) {
    using enum Access;
    const auto unit=object_of(memory,handle,object_table,object_datums,object_address);
    const auto copy_point=[&](const Point &p) {
        frame.point(p);
        for(std::uint32_t word=0;word<3u;++word)
            memory.store32(p.store,at(unit,p.to+4u*word),memory.load32(p.read,at(state,p.from+4u*word)));
    };
    copy_point(position);
    memory.store32(timer_a_store,at(unit,0x1c0u),memory.load32(timer_a_read,at(state,0x20u)));
    memory.store32(timer_b_store,at(unit,0x1c4u),memory.load32(timer_b_read,at(state,0x24u)));
    memory.store8(mode_store,at(unit,0x1bcu),memory.load8(mode_read,at(state,4u)));
    const auto pair_first=memory.load16(pair_word,at(state,6u));
    if(pair_first==memory.load16(pair_current,at(unit,0x214u)))
        memory.store32(pair_store,at(unit,0x214u),memory.load32(pair_read,at(state,6u)));
    if(memory.load16(choice_check,at(state,0xau))!=0xffffu)
        memory.store8(choice_store,at(unit,0x23du),memory.load8(choice_read,at(state,0xau)));
    memory.store8(flag_store,at(unit,0x241u),memory.load8(flag_read,at(state,0xcu)));
    memory.store32(link_store,at(unit,0x148u),memory.load32(link_read,at(state,0x10u)));
    copy_point(goal);
    copy_point(facing);
    copy_point(origin);
    memory.store32(kind_store,at(unit,0x14cu),memory.load32(kind_read,state));
    copy_point(aim);
    frame.tail();
    for(std::uint32_t word=0;word<tail_words;++word)
        memory.store32(tail_store,at(unit,0x1c8u+4u*word),memory.load32(tail_read,at(state,0x58u+4u*word)));
}
} // namespace state_apply
} // namespace halo2::native_state_apply
