#pragma once
// Native TU1.5 channel advance, entered at 1CE950 (physics live support
// unit; TU1.1 1CA090): an animation channel set to a new frame, firing the
// events it passes.
//
//   channel_advance::advance   1CE950(channel, frame, variant, owner, context) -> RET 14
//
// The channel's graph definition comes from 1CEE60 (ECX the channel), its
// animation, unless the index (+6) is NONE, from 1E3150 (EDX the graph's
// definition through the tag instances at 4F0414; the channel's +4 pushed).
// The channel's events (+0x14, +0x16) are cleared and its flags (+0x11) keep
// only 0xF1; the frame (at least 0) is the step, the animation's frame count
// (int16 +0x14) less the epsilon at 46A0EC the last frame. 1CF700 then fires
// the events from the channel's frame to the step (the definition, the
// animation, the options +0x12, the old frame, the step and the context
// pushed; EBX the owner), and its answer joins the events. While the channel
// still plays the animation it started with (+4 and +6 unchanged; a changed
// +4 ends the advance), a step at or past the last frame fires the events to
// the end (FLT_MAX), then:
//   - a channel that loops (option bit 1) switches animations first when it
//     asks to (option bit 5) and its animation has no loop frame (int16
//     +0x2E 0): 1D3660 (ESI the variant's slot; the channel's +4 pushed)
//     names the next animation, which the channel takes (and a rate of 1,
//     46A0D0, under option bit 3), and B8780 (ECX the channel) answers it;
//     the step then wraps to the loop frame (step - last + loop frame) and
//     the events from the loop frame to it are fired; it is marked looped
//     (flag bit 2).
//   - a channel that does not loop is marked ended (flag bit 3) and stops at
//     the last frame.
// Finally, while the animation is the same, 1CE810 (EAX the channel) places
// the step.
//
// The single-precision arithmetic is the original's scalar SSE, in its
// order, on the live MXCSR. This is the game logic only. The game's memory
// is the retained TU1.5 bank, reached through a Memory object with typed
// loads and stores; every access names its role (Access), including the
// original's frame (the definition, the old frame, the animation it started
// with) and the argument slots it reuses (the step in the channel's, the
// last frame in the frame's); the boundary adapter
// (native_tu15_channel_advance.hpp) maps the role to the original
// instruction and owns the ABI of the children. The values the original
// keeps in callee-saved registers across them are a Kept, refreshed from the
// registers each returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "gameplay/units/native_tu15_record_fade_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_channel_advance {
using native_object_update::at;
using native_object_update::none;
using native_object_update::widen16;
namespace sse=native_aim_clusters::sse;
namespace scalar=native_record_fade::scalar;
namespace globals {
inline constexpr std::uint32_t tag_instances=0x4f0414u;
inline constexpr std::uint32_t epsilon=0x46a0ecu,one=0x46a0d0u;   // floats
}
inline constexpr std::uint32_t tag_size=16u,tag_data=8u,no_limit=0x7f7fffffu;   // FLT_MAX
namespace channel {inline constexpr std::uint32_t graph=0u,animation=4u,index=6u,flags=0x11u,options=0x12u,events=0x14u,
    events_high=0x16u,rate=0x18u,frame=0x1cu;}
namespace animation {inline constexpr std::uint32_t frames=0x14u,loop_frame=0x2eu;}
namespace flag {inline constexpr std::uint32_t kept=0xf1u,looped=4u,ended=8u;}
namespace option {inline constexpr std::uint32_t loops=2u,switches=0x20u,resets_rate=8u;}
namespace argument {inline constexpr std::uint32_t channel=0u,frame=1u,variant=2u,owner=3u,context=4u;}
// The frame's slots, by offset from the return address.
namespace local {inline constexpr std::int32_t definition=-4,old_frame=-8,started=-0xc,started_index=-0xa;}

namespace channel_advance {
enum class Access {channel_argument,started_load,started_store,index_test,definition_store,graph_tag,instances,graph_definition,
    current_frame,frame_argument,events_clear,events_high_clear,frame_count,old_frame_store,epsilon,last_store,step_store,step_zero,
    flags_load,context_load,step_load,options_load,flags_store,old_frame_load,owner_load,playing_animation,saved_index,playing_index,
    events_load,events_store,step_again,last_again,context_again,old_frame_again,options_again,definition_load,options_word,
    loop_frame,next_animation,variant_argument,switched,rate_option,animation_store,one,rate_store,started_again,loop_frame_again,
    step_wrap,last_wrap,context_wrap,owner_wrap,options_wrap,wrapped_store,loop_store,wrapped_load,loop_load,definition_wrap,
    flags_looped,flags_looped_store,flags_ended,flags_ended_store,frame_count_end,epsilon_end,step_end_store,step_final};
enum class Outcome {replaced,stepped,looped,ended,changed};
// The original's EBX (the definition, then the owner), EBP (the channel), ESI (the animation) and EDI (the animation
// it started with: its +4 and +6) across the children.
struct Kept {std::uint32_t held,channel,animation,started;};
// Whether the channel still plays the animation it started with.
enum class Playing {replaced,reindexed,same};
// Frame:
//   argument(k), local(offset)                     the argument slots and the frame's slots now
//   definition(kept) -> EAX                         1CEE60(ECX = channel), refreshes kept
//   animation_of(graph, kept) -> EAX                1E3150(EDX = graph; the started +4 pushed), refreshes kept
//   index_tested(index), clamped(requested, negative), flags_kept(flags)   the registers and flags between
//   pushing(value), pushing_fire(from, options, animation, definition)     a 1CF700 call's arguments, in the original's order
//   fire(kept) -> AX                                1CF700 (EBX the owner), refreshes kept
//   checkpoint(n), animation_compared(kept, now), index_compared(saved, now), joined(events)   the checks of the animation
//   step_compared(before_end), options_tested(options), loop_tested(loop), rate_tested(resets)
//   next_variant(current, kept) -> EAX              1D3660(ESI = the variant's slot; current pushed), refreshes kept
//   answer(kept) -> EAX                             B8780(ECX = channel), refreshes kept
//   wrapped(difference, loop_from, wrapped), ended(last)   the wrap's and the end's SSE registers
//   place(step, kept)                               1CE810(EAX = channel; step), refreshes kept
template<class Memory,class Frame> Outcome advance(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    kept.channel=memory.load32(channel_argument,frame.argument(argument::channel));
    kept.started=memory.load32(started_load,at(kept.channel,channel::animation));
    memory.store32(started_store,frame.local(local::started),kept.started);
    kept.held=frame.definition(kept);
    const auto definition_value=kept.held;
    kept.animation=0u;
    const auto index=memory.load16(index_test,at(kept.channel,channel::index));
    frame.index_tested(index);
    memory.store32(definition_store,frame.local(local::definition),definition_value);
    if(index!=0xffffu) {
        const auto graph_tag_value=memory.load32(graph_tag,at(kept.channel,channel::graph));
        const auto instances_address=memory.load32(instances,globals::tag_instances);
        const auto graph=memory.load32(graph_definition,tag_size*(graph_tag_value&0xffffu)+instances_address+tag_data);
        kept.animation=frame.animation_of(graph,kept);
    }
    // The step (the requested frame, at least 0) and the last frame, in the slots of the first two arguments.
    const auto frame_now=memory.load32(current_frame,at(kept.channel,channel::frame));
    const auto requested=memory.load32(frame_argument,frame.argument(argument::frame));
    memory.store16(events_clear,at(kept.channel,channel::events),0u);
    memory.store16(events_high_clear,at(kept.channel,channel::events_high),0u);
    const auto count=widen16(memory.load16(frame_count,at(kept.animation,animation::frames)));
    memory.store32(old_frame_store,frame.local(local::old_frame),frame_now);
    const auto converted=scalar::from_integer(count);
    const auto last=sse::subtract(converted,memory.load32(Access::epsilon,globals::epsilon));
    frame.last_frame(converted,last);
    memory.store32(last_store,frame.argument(argument::frame),last);
    const bool negative=!sse::not_above(0u,requested);
    frame.clamped(requested,negative);
    memory.store32(step_store,frame.argument(argument::channel),requested);
    if(negative)memory.store32(step_zero,frame.argument(argument::channel),0u);
    // The events from the old frame to the step.
    const auto flags_value=memory.load8(flags_load,at(kept.channel,channel::flags));
    const auto context_value=memory.load32(context_load,frame.argument(argument::context));
    const auto step=memory.load32(step_load,frame.argument(argument::channel));
    frame.pushing(context_value);
    const auto options_value=memory.load16(options_load,at(kept.channel,channel::options));
    frame.flags_kept(flags_value);
    frame.pushing(step);
    memory.store8(flags_store,at(kept.channel,channel::flags),flags_value&flag::kept);
    const auto old_frame=memory.load32(old_frame_load,frame.local(local::old_frame));
    frame.pushing_fire(old_frame,options_value,kept.animation,kept.held);   // EBX: the definition, as 1E3150 left it
    kept.held=memory.load32(owner_load,frame.argument(argument::owner));
    auto events_now=frame.fire(kept);
    // The checks of the animation the channel started with: its +4 (a change ends everything) and +6.
    const auto playing=[&](unsigned n)->Playing {
        frame.checkpoint(n);
        const auto now=memory.load16(playing_animation,at(kept.channel,channel::animation));
        frame.animation_compared(kept,now);
        if((kept.started&0xffffu)!=now)return Playing::replaced;
        const auto saved=memory.load16(saved_index,frame.local(local::started_index));
        const auto index_now=memory.load16(playing_index,at(kept.channel,channel::index));
        frame.index_compared(saved,index_now);
        return saved==index_now?Playing::same:Playing::reindexed;
    };
    const auto join=[&](std::uint32_t answer) {
        const auto value=memory.load16(events_load,at(kept.channel,channel::events))|(answer&0xffffu);
        frame.joined(value);
        memory.store16(events_store,at(kept.channel,channel::events),value);
    };
    if(playing(0u)!=Playing::same)return Outcome::replaced;
    join(events_now);
    auto outcome=Outcome::stepped;
    const auto step_value=memory.load32(step_again,frame.argument(argument::channel));
    const bool before_end=sse::below(step_value,memory.load32(last_again,frame.argument(argument::frame)));
    frame.step_compared(step_value,before_end);
    if(!before_end) {
        // The events to the end.
        const auto context_end=memory.load32(context_again,frame.argument(argument::context));
        const auto old_frame_end=memory.load32(old_frame_again,frame.local(local::old_frame));
        const auto options_end=memory.load16(options_again,at(kept.channel,channel::options));
        frame.pushing(context_end);
        const auto definition_end=memory.load32(definition_load,frame.local(local::definition));
        frame.pushing(no_limit);
        frame.pushing_fire(old_frame_end,options_end,kept.animation,definition_end);
        events_now=frame.fire(kept);
        const auto state=playing(1u);
        if(state==Playing::replaced)return Outcome::replaced;
        if(state==Playing::same) {
            join(events_now);
            const auto options_now=memory.load16(options_word,at(kept.channel,channel::options));
            frame.options_tested(options_now);
            if(options_now&option::loops) {
                if(options_now&option::switches) {
                    const auto loop=memory.load16(loop_frame,at(kept.animation,animation::loop_frame));
                    frame.loop_tested(loop);
                    if(!loop) {
                        // The next animation of the variant, and its answer.
                        const auto current=memory.load32(next_animation,at(kept.channel,channel::animation));
                        memory.load32(variant_argument,frame.argument(argument::variant));
                        const auto slot=frame.next_variant(current,kept);
                        kept.started=memory.load32(switched,slot);
                        const bool resets=memory.load8(rate_option,at(kept.channel,channel::options))&option::resets_rate;
                        frame.rate_tested(resets);
                        memory.store32(animation_store,at(kept.channel,channel::animation),kept.started);
                        if(resets)memory.store32(rate_store,at(kept.channel,channel::rate),memory.load32(Access::one,globals::one));
                        memory.store32(started_again,frame.local(local::started),kept.started);
                        kept.animation=frame.answer(kept);
                    }
                }
                // The step wrapped to the loop frame, and the events from the loop frame to it.
                const auto loop_frame_value=widen16(memory.load16(loop_frame_again,at(kept.animation,animation::loop_frame)));
                const auto step_end=memory.load32(step_wrap,frame.argument(argument::channel));
                const auto difference=sse::subtract(step_end,memory.load32(last_wrap,frame.argument(argument::frame)));
                const auto context_loop=memory.load32(context_wrap,frame.argument(argument::context));
                kept.held=memory.load32(owner_wrap,frame.argument(argument::owner));
                const auto loop_from=scalar::from_integer(loop_frame_value);
                frame.pushing(context_loop);
                const auto options_loop=memory.load16(options_wrap,at(kept.channel,channel::options));
                const auto wrapped_step=sse::add(difference,loop_from);
                frame.wrapped(difference,loop_from,wrapped_step);
                memory.store32(wrapped_store,frame.argument(argument::channel),wrapped_step);
                memory.store32(loop_store,frame.local(local::old_frame),loop_from);
                const auto to=memory.load32(wrapped_load,frame.argument(argument::channel));
                const auto from=memory.load32(loop_load,frame.local(local::old_frame));
                frame.pushing(to);
                const auto definition_loop=memory.load32(definition_wrap,frame.local(local::definition));
                frame.pushing_fire(from,options_loop,kept.animation,definition_loop);
                events_now=frame.fire(kept);
                const auto looped_state=playing(2u);
                if(looped_state==Playing::replaced)return Outcome::replaced;
                if(looped_state==Playing::same) {
                    join(events_now);
                    const auto looped_flags=memory.load8(flags_looped,at(kept.channel,channel::flags))|flag::looped;
                    frame.marked(looped_flags);
                    memory.store8(flags_looped_store,at(kept.channel,channel::flags),looped_flags);
                }
                outcome=Outcome::looped;
            } else {
                // The channel stops at the last frame.
                const auto ended_flags=memory.load8(flags_ended,at(kept.channel,channel::flags))|flag::ended;
                frame.marked(ended_flags);
                memory.store8(flags_ended_store,at(kept.channel,channel::flags),ended_flags);
                const auto count_end=widen16(memory.load16(frame_count_end,at(kept.animation,animation::frames)));
                const auto converted_end=scalar::from_integer(count_end);
                const auto last_end=sse::subtract(converted_end,memory.load32(epsilon_end,globals::epsilon));
                frame.ended(converted_end,last_end);
                memory.store32(step_end_store,frame.argument(argument::channel),last_end);
                outcome=Outcome::ended;
            }
        }
    }
    if(playing(3u)!=Playing::same)return Outcome::changed;
    frame.place(memory.load32(step_final,frame.argument(argument::channel)),kept);
    return outcome;
}
} // namespace channel_advance
} // namespace halo2::native_channel_advance
