#pragma once
// Native TU1.5 model switch, entered at 1D3930 (physics live support unit):
// which of a record's two models shows, by its distance.
//
//   model_switch::update   1D3930(EAX = record)
//
// A record holds two models (a handle and an index: +0/+6 and +0x20/+0x26,
// NONE for none), each with flags (+0x12, +0x32) whose bits 7..9 hide it.
// With the switch at 47C1B4 on and the second model present, a distance
// (+0x80) at or past the threshold (46A330) shows the second model, the
// record flagging the switch (byte +0x6C bit 5), unless it lies within the
// near bound (46A234; or is NaN), which leaves both as they are. Otherwise
// the first model shows. Each present model's flags are written.
//
// The comparisons are the original's COMISS on the live MXCSR. This is the
// game logic only. The game's memory is the retained TU1.5 bank, reached
// through a Memory object with typed loads and stores; every access names
// its role (Access), and the boundary adapter (native_tu15_model_switch.hpp)
// maps the role to the original instruction.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include <cstdint>

namespace halo2::native_model_switch {
using native_object_update::at;
using native_object_update::none;
namespace sse=native_aim_clusters::sse;
namespace globals {
inline constexpr std::uint32_t enabled=0x47c1b4u;     // byte
inline constexpr std::uint32_t threshold=0x46a330u;   // float: the distance the second model shows from
inline constexpr std::uint32_t near_bound=0x46a234u;  // float
}
struct Model {std::uint32_t handle,index,flags;};
inline constexpr Model first{0u,6u,0x12u},second{0x20u,0x26u,0x32u};
namespace record {inline constexpr std::uint32_t switch_flags=0x6cu,distance=0x80u;}
inline constexpr std::uint32_t hidden=0x380u,switched=0x20u,no_index=0xffffu;

namespace model_switch {
enum class Access {first_flags,second_flags,enabled,second_handle,second_index,threshold,distance,distance_again,near_bound,
    first_handle,switch_flags,switch_store,first_index,first_store,second_handle_again,second_index_again,second_store};
enum class Outcome {kept,first_shown,second_shown};
// Frame: decided(first flags, second flags) the flags as they will be written.
template<class Memory,class Frame> bool present(Memory &memory,std::uint32_t record_address,const Model &model,Access handle,Access index) {
    return memory.load32(handle,at(record_address,model.handle))!=none && memory.load16(index,at(record_address,model.index))!=no_index;
}
template<class Memory,class Frame> Outcome update(Memory &memory,Frame &frame,std::uint32_t record_address) {
    using enum Access;
    auto first_value=memory.load16(first_flags,at(record_address,first.flags));
    auto second_value=memory.load16(second_flags,at(record_address,second.flags));
    bool far=false;
    if((memory.load8(Access::enabled,globals::enabled)&0xffu) &&
       present<Memory,Frame>(memory,record_address,second,second_handle,second_index)) {
        const auto threshold_value=memory.load32(threshold,globals::threshold);
        if(sse::not_above(threshold_value,memory.load32(distance,at(record_address,record::distance)))) {
            const auto distance_value=memory.load32(distance_again,at(record_address,record::distance));
            if(sse::not_above(distance_value,memory.load32(Access::near_bound,globals::near_bound)))return Outcome::kept;
            far=true;
        }
    }
    const auto flags_address=at(record_address,record::switch_flags);
    std::uint32_t first_handle_value;
    if(far) {
        first_handle_value=memory.load32(first_handle,at(record_address,first.handle));
        memory.store8(switch_store,flags_address,memory.load8(Access::switch_flags,flags_address)|switched);
        first_value|=hidden;second_value&=~hidden;
    } else {
        memory.store8(switch_store,flags_address,memory.load8(Access::switch_flags,flags_address)&~switched);
        first_handle_value=memory.load32(first_handle,at(record_address,first.handle));
        first_value&=~hidden;second_value|=hidden;
    }
    frame.decided(first_value,second_value);
    if(first_handle_value!=none && memory.load16(first_index,at(record_address,first.index))!=no_index)
        memory.store16(first_store,at(record_address,first.flags),first_value);
    if(present<Memory,Frame>(memory,record_address,second,second_handle_again,second_index_again))
        memory.store16(second_store,at(record_address,second.flags),second_value);
    return far?Outcome::second_shown:Outcome::first_shown;
}
} // namespace model_switch
} // namespace halo2::native_model_switch
