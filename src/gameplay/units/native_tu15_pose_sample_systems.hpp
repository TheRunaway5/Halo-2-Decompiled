#pragma once
// Native TU1.5 pose sample, entered at 212D40 (physics live support unit):
// one frame of an animation record read into a vector and a value.
//
//   pose_sample::sample   212D40(EAX = value*, ECX = vector*, EDX = record*; frame) -> RET 4
//
// The vector starts as the zero vector ([474D04] points at it) and the
// value at 0. The record (as 1E5DF0 fills it) holds a base (+0), its
// description (+4) and a kind byte (+9). Kinds 1, 2 and 3 store frames of
// 8, 12 and 16 bytes: frame `frame` lies at the base plus the description's
// word +6, byte +1, dword +0xC and byte +0 (signed) and the frame times its
// size. Kind 1 gives the vector's x and y, kind 2 also the value, kind 3
// x, y, z and the value; other kinds leave the start.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_pose_sample.hpp) maps the role to the original instruction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_pose_sample {
using native_object_update::at;
using native_object_update::widen16;
using native_object_update::widen8;
namespace globals {inline constexpr std::uint32_t zero_vector=0x474d04u;}   // a pointer to the zero hkVector4
namespace record {inline constexpr std::uint32_t base=0u,description=4u,kind=9u;}
namespace description {inline constexpr std::uint32_t lead=0u,second=1u,start=6u,offset=0xcu;}
inline constexpr std::uint32_t zero_float=0u;

namespace pose_sample {
enum class Access {frame_word,zero_vector,start_copy,start_store,value_clear,kind,description,second,start,offset,lead,base,
    component,component_store,value,value_store};
enum class Outcome {other_kind,pair,pair_value,triple_value};
// Frame: frame_slot(); component(k) before each copied component; offset(kind,
// parts) the frame's offset parts as the original sums them.
template<class Memory,class Frame> Outcome sample(Memory &memory,Frame &frame,std::uint32_t value_out,std::uint32_t vector,
                                                 std::uint32_t record_address) {
    using enum Access;
    const auto frame_index=widen16(memory.load32(frame_word,frame.frame_slot())&0xffffu);
    const auto zero=memory.load32(zero_vector,globals::zero_vector);
    for(std::uint32_t k=0;k<3u;++k) {
        frame.component(k);
        memory.store32(start_store,at(vector,4u*k),memory.load32(start_copy,at(zero,4u*k)));
    }
    memory.store32(value_clear,value_out,zero_float);
    const auto kind_value=memory.load8(kind,at(record_address,record::kind))&0xffu;
    if(kind_value<1u || kind_value>3u)return Outcome::other_kind;
    static constexpr std::uint32_t sizes[]{8u,12u,16u};
    const auto described=memory.load32(Access::description,at(record_address,record::description));
    const auto second_value=widen8(memory.load8(second,at(described,description::second)));
    const auto start_value=widen16(memory.load16(start,at(described,description::start)));
    const auto offset_value=memory.load32(offset,at(described,description::offset));
    const auto lead_value=widen8(memory.load8(lead,at(described,description::lead)));
    const auto base_value=memory.load32(base,at(record_address,record::base));
    const auto into=start_value+second_value+sizes[kind_value-1u]*frame_index+offset_value+lead_value;
    frame.located(kind_value,start_value+second_value,into-lead_value,into,base_value);
    const auto components=kind_value==3u?3u:2u;
    for(std::uint32_t k=0;k<components;++k) {
        frame.component(k);
        memory.store32(component_store,at(vector,4u*k),memory.load32(component,into+base_value+4u*k));
    }
    if(kind_value==1u)return Outcome::pair;
    memory.store32(value_store,value_out,memory.load32(value,into+base_value+4u*components));
    return kind_value==2u?Outcome::pair_value:Outcome::triple_value;
}
} // namespace pose_sample
} // namespace halo2::native_pose_sample
