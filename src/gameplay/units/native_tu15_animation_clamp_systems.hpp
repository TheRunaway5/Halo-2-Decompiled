#pragma once
// Native TU1.5 animation frame clamp, entered at 1CE810 (AI actor and
// physics live support units): an animation state's frame is placed.
//
//   animation_clamp::place   1CE810(EAX = state; frame)
//
// The state names its graph (+0, through the tag instances at 4F0414) and
// animation (+4); unless the animation index (+6) is NONE, 1E3150 gives the
// animation, whose frame count is the int16 +0x14. The state's frame (+0x1C)
// becomes the given frame, at least 0 and at most the frame count less
// 46A0EC (COMISS). A playing state (bit 0 of +0x12) that is not held (bits
// 0 and 3 of +0x11) learns whether its next frame (the frame plus the
// seconds per tick ([51DF54]+4) times its rate (+0x18) times 46A20C)
// reaches the end: bit 1 of +0x11. 1CEC60 then updates the state.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_animation_clamp.hpp) maps the role to the original
// instruction and owns the ABI of the children (1E3150, 1CEC60). The state
// the original keeps in ESI is a Kept, refreshed from the registers each
// child returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "gameplay/units/native_tu15_record_fade_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_animation_clamp {
using native_object_update::at;
using native_object_update::widen16;
namespace sse=native_aim_clusters::sse;
namespace scalar=native_record_fade::scalar;
namespace globals {
inline constexpr std::uint32_t tag_instances=0x4f0414u;
inline constexpr std::uint32_t game_time=0x51df54u;   // +4: seconds per tick
inline constexpr std::uint32_t frame_margin=0x46a0ecu,rate_scale=0x46a20cu;   // floats
}
inline constexpr std::uint32_t tag_address=8u,instance_shift=4u,frame_count=0x14u;
namespace state {
inline constexpr std::uint32_t graph=0u,animation=4u,animation_index=6u,held=0x11u,playing=0x12u,rate=0x18u,frame=0x1cu;
inline constexpr std::uint32_t held_bits=0x09u,reaches_end=0x02u;
}

namespace animation_clamp {
enum class Local {frame};
enum class Access {
    index,graph,instances,graph_address,animation,frame_argument,margin,count,playing,frame_store,held,game_time,
    seconds,rate,next_count,rate_scale,held_store
};
struct Kept {std::uint32_t state;};   // ESI
// Frame:
//   local(Local)          the argument's address now
//   animation(kept)       1E3150(EDX the graph; the animation); returns
//                         EAX, refreshes kept
//   placed(value, last, kind)  the frame's comparisons (XMM0, XMM1)
//   advanced(next, end)   the next frame's comparison (XMM0, XMM1)
//   update(kept, marked)  1CEC60 (at the call after the end bit was set,
//                         or the other); refreshes kept
enum class Placed {below_zero,past_end,within};
template<class Memory,class Frame> void place(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    std::uint32_t animation_address=0;
    if(memory.load16(index,at(kept.state,state::animation_index))!=0xffffu) {
        const auto graph_handle=memory.load32(graph,at(kept.state,state::graph));
        const auto instances_address=memory.load32(instances,globals::tag_instances);
        (void)memory.load32(graph_address,at(((graph_handle&0xffffu)<<instance_shift)+instances_address,tag_address));
        (void)memory.load32(animation,at(kept.state,state::animation));
        animation_address=frame.animation(kept);
    }
    const auto requested=memory.load32(frame_argument,frame.local(Local::frame));
    const bool below_zero=!sse::not_above(0u,requested);
    const auto margin_value=memory.load32(margin,globals::frame_margin);
    std::uint32_t placed=0u;
    if(below_zero)frame.placed(placed,0u,Placed::below_zero);
    else {
        const auto count_value=widen16(memory.load16(count,at(animation_address,frame_count)));
        const auto last=sse::subtract(scalar::from_integer(count_value),margin_value);
        if(!sse::not_above(requested,last)){placed=last;frame.placed(placed,last,Placed::past_end);}
        else{placed=requested;frame.placed(placed,last,Placed::within);}
    }
    const bool playing_state=memory.load8(playing,at(kept.state,state::playing))&1u;
    memory.store32(frame_store,at(kept.state,state::frame),placed);
    bool marked=false;
    if(playing_state) {
        const auto held_bits=memory.load8(held,at(kept.state,state::held));
        if(!(held_bits&state::held_bits)) {
            const auto time=memory.load32(game_time,globals::game_time);
            const auto seconds_value=memory.load32(seconds,at(time,4u));
            auto next=sse::multiply(seconds_value,memory.load32(rate,at(kept.state,state::rate)));
            const auto next_count_value=widen16(memory.load16(next_count,at(animation_address,frame_count)));
            next=sse::multiply(next,memory.load32(rate_scale,globals::rate_scale));
            next=sse::add(next,placed);
            const auto end=sse::subtract(scalar::from_integer(next_count_value),margin_value);
            const bool reaches=!sse::below(next,end);
            frame.advanced(next,end);
            memory.store8(held_store,at(kept.state,state::held),reaches?(held_bits|state::reaches_end):(held_bits&~state::reaches_end&0xffu));
            marked=reaches;
        }
    }
    frame.update(kept,marked);
}
} // namespace animation_clamp
} // namespace halo2::native_animation_clamp
