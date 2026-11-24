#pragma once
// Native TU1.5 state reset, entered at C93D0 (AI frame and physics live
// support units): a 0x7C-byte state starts over.
//
//   state_reset::reset   C93D0(EDX = state)
//
// The state is cleared (31 words, REP STOSD); its kind (+0) becomes
// 6000086, its index (+4) 0, and its handles (+6, +8, +9, +0xA, +0xC and,
// after the points, +0x58, +0x5C, +0x60) NONE. Its three points (+0x28,
// +0x34, +0x40) are the origin (the point [474D08] names), its two
// distances (+0x74, +0x78) zero and its count (+0x70) 0.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_state_reset.hpp) maps the role to the original instruction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_state_reset {
using native_object_update::at;
using native_object_update::none;
namespace globals {inline constexpr std::uint32_t origin=0x474d08u;}   // the origin point's address
inline constexpr std::uint32_t state_words=0x1fu,kind_value=0x6000086u;
namespace state {
inline constexpr std::uint32_t kind=0u,index=4u,handle_a=6u,handle_b=8u,handle_c=9u,handle_d=0xau,handle_e=0xcu,
    first_point=0x28u,second_point=0x34u,third_point=0x40u,handle_f=0x58u,handle_g=0x5cu,handle_h=0x60u,count=0x70u,
    first_distance=0x74u,second_distance=0x78u;
}

namespace state_reset {
enum class Access {clear,index_store,origin,kind_store,handle_a,handle_b,handle_c,handle_d,handle_e,point_read,point_store,
    first_distance,second_distance,handle_f,handle_g,handle_h,count};
// Frame:
//   cleared()        the clear's registers (REP STOSD done)
//   point(n)         the n-th point is copied next
template<class Memory,class Frame> void reset(Memory &memory,Frame &frame,std::uint32_t target) {
    using enum Access;
    for(std::uint32_t word=0;word<state_words;++word)memory.store32(clear,target+4u*word,0u);
    frame.cleared();
    memory.store16(index_store,at(target,state::index),0u);
    const auto origin_address=memory.load32(origin,globals::origin);
    memory.store32(kind_store,at(target,state::kind),kind_value);
    memory.store16(handle_a,at(target,state::handle_a),0xffffu);
    memory.store8(handle_b,at(target,state::handle_b),0xffu);
    memory.store8(handle_c,at(target,state::handle_c),0xffu);
    memory.store16(handle_d,at(target,state::handle_d),0xffffu);
    memory.store16(handle_e,at(target,state::handle_e),0xffffu);
    for(const auto point:{state::first_point,state::second_point,state::third_point}) {
        frame.point(point);
        for(std::uint32_t word=0;word<3u;++word)
            memory.store32(point_store,at(target,point+4u*word),memory.load32(point_read,at(origin_address,4u*word)));
    }
    memory.store32(first_distance,at(target,state::first_distance),0u);
    memory.store32(second_distance,at(target,state::second_distance),0u);
    memory.store32(handle_f,at(target,state::handle_f),none);
    memory.store32(handle_g,at(target,state::handle_g),none);
    memory.store32(handle_h,at(target,state::handle_h),none);
    memory.store16(count,at(target,state::count),0u);
}
} // namespace state_reset
} // namespace halo2::native_state_reset
