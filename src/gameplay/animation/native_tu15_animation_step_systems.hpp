#pragma once
// Native TU1.5 animation step, entered at 1D39C0 (physics live support unit):
// an animation state's channels, its blend and its second channel stepped.
//
//   animation_step::advance   1D39C0(ESI = state; step, last, a, b, c) -> AL, RET 0x14
//
// Each of the state's first three channels (0x20 bytes apart) that is in use
// (+0 not NONE, word +6 not NONE) counts down: with an owner (+0x68 not
// NONE) not held (bit 0 of +0x6C) the countdown step (1CEB70, EAX the
// channel; the state, step and last pushed) runs; a channel without an owner
// clears the frame's done flag, which ends the walk. A blend (+0x40, word
// +0x44, +0x46) in use and flagged (+0x51 & 0xA) is reported to 1D38A0 (EAX
// the second argument; the state, 0.267, the third and fourth pushed) unless
// its record (1E3150) has bit 1 of +0x18 set, then cleared (NONE, zeros and
// 1 in +0x40 to +0x5C), and the state's first channel loses bit 0 of +0x11
// when it has bit 0 of +0x12. 1D3930 switches the model. With blending on
// (byte [47C1B4]) a second channel (+0x20) in use advances (1CE950: the
// channel, its frames (word +0x14 of its record) times the first channel's
// ratio (its +0x1C over its record's frames, clamped to 0 to 1, kept in the
// fifth argument's slot), the state, step and last). The answer is the done
// flag.
//
// The single-precision arithmetic is the original's scalar SSE, in its order,
// on the live MXCSR. This is the game logic only. The game's memory is the
// retained TU1.5 bank, reached through a Memory object with typed loads and
// stores; every access names its role (Access), including the original's
// frame (the done and report flags, in the slot of the ECX it pushes, and the
// ratio in an argument's slot); the boundary adapter
// (native_tu15_animation_step.hpp) maps the role to the original instruction
// and owns the ABI of the children. The values the original keeps in
// callee-saved registers across them are a Kept, refreshed from the
// registers each returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "gameplay/units/native_tu15_record_fade_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_animation_step {
using native_object_update::at;
using native_object_update::widen16;
namespace sse=native_aim_clusters::sse;
namespace scalar=native_record_fade::scalar;
namespace globals {inline constexpr std::uint32_t tag_instances=0x4f0414u,one=0x46a0d0u,blending=0x47c1b4u;}
inline constexpr std::uint32_t channels=3u,channel_size=0x20u,tag_size=16u,tag_data=8u,report_weight=0x3e88b439u;   // 0.267
namespace channel {inline constexpr std::uint32_t handle=0u,sub=4u,high=6u,flags=0x11u,switches=0x12u,duration=0x1cu;
    inline constexpr std::uint32_t cleared_flag=0xfeu;}
namespace state {inline constexpr std::uint32_t second=0x20u,blend=0x40u,blend_sub=0x44u,blend_high=0x46u,blend_key=0x48u,
    blend_kind=0x4cu,blend_mode=0x4du,blend_order=0x4eu,blend_on=0x50u,blend_flags=0x51u,blend_count=0x52u,blend_first=0x54u,
    blend_second=0x56u,blend_scale=0x58u,blend_weight=0x5cu,owner=0x68u,held=0x6cu;inline constexpr std::uint32_t reported=0xau;}
namespace record {inline constexpr std::uint32_t frames=0x14u,flags=0x18u;inline constexpr std::uint32_t silent_bit=1u;}
namespace argument {inline constexpr std::uint32_t step=0u,last=1u,a=2u,b=3u,ratio=4u;}
// The frame's slots, by offset from the return address: the done and report flags, in the pushed ECX.
namespace local {inline constexpr std::int32_t done=-2,report=-1;}

namespace animation_step {
enum class Access {done_store,channel_handle,channel_high,owner,held,step_last,step_first,done_load,done_and,done_check,blend_handle,
    blend_high,blend_flags,blend_again,blend_high_again,instances,blend_sub,blend_definition,record_flags,report_store,report_c,report_b,
    report_a,one,blend_store,first_handle,first_high,first_switches,first_flags,first_flags_store,blending,second_handle,second_high,
    first_high_check,first_tag,first_instances,first_sub,first_definition,first_frames,ratio_clear,first_duration,ratio_store,
    ratio_one,ratio_clamp,second_high_again,second_tag,second_instances,second_definition,second_sub,last_again,step_again,
    second_frames,ratio_load,product_store,done_final};
// The original's EBX (NONE), EBP (the channel's index), ESI (the state) and EDI (the channel) across the children.
struct Kept {std::uint32_t none,index,state,channel;};
// Frame (each hook the original instructions named; children refresh kept):
//   saving(), starting(kept), argument(k), local(offset), step_slot()
//   compared(a, b), compared16(a, b), tested8(v)        CMP/TEST flags
//   count_down(kept) [1CEB70], record_of(definition, sub, kept) -> EAX [1E3150], report(a, kept) [1D38A0],
//   switch_model(kept) [1D3930], advance(kept) [1CE950]
//   owner_found(owner), done_anded(v, al), walked(kept, done), silent(flags) -> flag, cleared(kept)
//   xmm1() -> XMM1, converted(n, value), divided(value), multiplied(value), loaded_xmm(n, value), zeroed(n)
template<class Memory,class Frame> std::uint32_t advance(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    using native_object_update::none;
    frame.saving();
    memory.store8(done_store,frame.local(local::done),1u);
    kept.index=0u;kept.channel=kept.state;kept.none=none;
    frame.starting(kept);
    // The three channels count down; one without an owner ends the walk.
    for(;;) {
        frame.compared(kept.index,channels);
        if(std::int32_t(kept.index)>=std::int32_t(channels))break;
        const auto handle=memory.load32(channel_handle,at(kept.channel,channel::handle));
        frame.compared(handle,kept.none);
        bool in_use=handle!=kept.none;
        if(in_use) {
            const auto high=memory.load16(channel_high,at(kept.channel,channel::high));
            frame.compared16(high,kept.none);
            in_use=high!=(kept.none&0xffffu);
        }
        if(in_use) {
            const auto owner_value=memory.load32(owner,at(kept.state,state::owner));
            frame.owner_found(owner_value,kept);
            std::uint32_t al=0u;
            if(owner_value!=kept.none) {
                const auto held_value=memory.load8(held,at(kept.state,state::held));
                frame.tested8(held_value&1u);
                if(!(held_value&1u)) {
                    const auto last=memory.load32(step_last,frame.argument(argument::last));
                    const auto step=memory.load32(step_first,frame.argument(argument::step));
                    frame.count_down(last,step,kept);
                }
                al=1u;
            }
            memory.store8(done_and,frame.local(local::done),frame.done_anded(memory.load8(done_load,frame.local(local::done)),al));
        }
        const auto done=memory.load8(done_check,frame.local(local::done));
        ++kept.index;kept.channel+=channel_size;
        frame.walked(kept,done);
        if(!(done&0xffu))break;
    }
    // The blend: reported unless its record is silent, then cleared.
    bool cleared=false;
    const auto blend=memory.load32(blend_handle,at(kept.state,state::blend));
    frame.compared(blend,kept.none);
    if(blend!=kept.none) {
        const auto high=memory.load16(blend_high,at(kept.state,state::blend_high));
        frame.compared16(high,kept.none);
        if(high!=(kept.none&0xffffu)) {
            const auto flags_value=memory.load8(blend_flags,at(kept.state,state::blend_flags));
            frame.tested8(flags_value&state::reported);
            if(flags_value&state::reported) {
                bool reported=true;
                const auto again=memory.load32(blend_again,at(kept.state,state::blend));
                frame.compared(again,kept.none);
                if(again!=kept.none) {
                    const auto high_again=memory.load16(blend_high_again,at(kept.state,state::blend_high));
                    frame.compared16(high_again,kept.none);
                    if(high_again!=(kept.none&0xffffu)) {
                        const auto instances_address=memory.load32(instances,globals::tag_instances);
                        const auto sub=memory.load32(blend_sub,at(kept.state,state::blend_sub));
                        const auto definition=memory.load32(blend_definition,frame.tag_index(again)+instances_address+tag_data);
                        const auto entry=frame.record_of(0u,definition,sub,kept);
                        const auto report=frame.silent(memory.load8(record_flags,at(entry,record::flags)));
                        memory.store8(report_store,frame.local(local::report),report);
                        reported=report!=0u;
                    }
                }
                if(reported) {
                    const auto c=memory.load32(report_c,frame.argument(argument::ratio));
                    const auto b=memory.load32(report_b,frame.argument(argument::b));
                    frame.pushing_report(c);
                    const auto a=memory.load32(report_a,frame.argument(argument::a));
                    frame.report(a,b,kept);
                }
                // Cleared: NONE, zeros and 1.
                frame.zeroed(1u);
                const auto one_value=memory.load32(one,globals::one);
                frame.loaded_xmm(0u,one_value);
                frame.clear_eax();
                memory.store32(blend_store,at(kept.state,state::blend),kept.none);
                memory.store16(blend_store,at(kept.state,state::blend_sub),kept.none);
                memory.store16(blend_store,at(kept.state,state::blend_high),kept.none);
                memory.store32(blend_store,at(kept.state,state::blend_weight),0u);
                memory.store8(blend_store,at(kept.state,state::blend_on),0u);
                memory.store8(blend_store,at(kept.state,state::blend_flags),0u);
                memory.store16(blend_store,at(kept.state,state::blend_count),0u);
                memory.store32(blend_store,at(kept.state,state::blend_scale),one_value);
                memory.store16(blend_store,at(kept.state,state::blend_first),0u);
                memory.store16(blend_store,at(kept.state,state::blend_second),0u);
                memory.store32(blend_store,at(kept.state,state::blend_key),kept.none);
                memory.store8(blend_store,at(kept.state,state::blend_kind),kept.none);
                memory.store8(blend_store,at(kept.state,state::blend_mode),kept.none);
                memory.store16(blend_store,at(kept.state,state::blend_order),kept.none);
                // The first channel's switch flag.
                const auto first=memory.load32(first_handle,at(kept.state,channel::handle));
                frame.compared(first,kept.none);
                if(first!=kept.none) {
                    const auto first_high_value=memory.load16(first_high,at(kept.state,channel::high));
                    frame.compared16(first_high_value,kept.none);
                    if(first_high_value!=(kept.none&0xffffu)) {
                        const auto switches=memory.load8(first_switches,at(kept.state,channel::switches));
                        frame.tested8(switches&1u);
                        if(switches&1u)memory.store8(first_flags_store,at(kept.state,channel::flags),
                            frame.anded(memory.load8(first_flags,at(kept.state,channel::flags)),channel::cleared_flag));
                    }
                }
                cleared=true;
            }
        }
    }
    if(!cleared)frame.zeroed(1u);
    frame.switch_model(kept);
    // With blending on, the second channel advances at the first's ratio.
    const auto blending_on=memory.load8(blending,globals::blending);
    frame.tested8(blending_on);
    if(blending_on) {
        const auto second=memory.load32(second_handle,at(kept.state,state::second));
        frame.compared(second,kept.none);
        kept.channel=kept.state+state::second;
        frame.second_channel(kept);
        if(second!=kept.none) {
            const auto second_high_value=memory.load16(second_high,at(kept.channel,channel::high));
            frame.compared16(second_high_value,kept.none);
            if(second_high_value!=(kept.none&0xffffu)) {
                frame.clear_eax();
                std::uint32_t entry=0u;
                const auto first_high_value=memory.load16(first_high_check,at(kept.state,channel::high));
                frame.compared16(first_high_value,kept.none);
                if(first_high_value!=(kept.none&0xffffu)) {
                    const auto tag=memory.load32(first_tag,at(kept.state,channel::handle));
                    const auto instances_address=memory.load32(first_instances,globals::tag_instances);
                    const auto sub=memory.load32(first_sub,at(kept.state,channel::sub));
                    const auto definition=memory.load32(first_definition,frame.tag_index(tag)+instances_address+tag_data);
                    entry=frame.record_of(1u,definition,sub,kept);
                    frame.zeroed(1u);
                }
                const auto frames=memory.load16(first_frames,at(entry,record::frames));
                frame.tested16(frames);
                const auto start_ratio=frame.xmm1();
                memory.store32(ratio_clear,frame.argument(argument::ratio),start_ratio);
                if(std::int16_t(frames)>0) {
                    const auto duration=memory.load32(first_duration,at(kept.state,channel::duration));
                    frame.loaded_xmm(0u,duration);
                    const auto count=scalar::from_integer(widen16(frames));
                    frame.converted(2u,count);
                    const auto ratio=sse::divide(duration,count);
                    frame.divided(ratio);
                    frame.compared_float(start_ratio,ratio);
                    memory.store32(ratio_store,frame.argument(argument::ratio),ratio);
                    if(sse::not_above(start_ratio,ratio)) {
                        const auto one_value=memory.load32(ratio_one,globals::one);
                        frame.loaded_xmm(1u,one_value);
                        frame.compared_float(ratio,one_value);
                        if(!sse::not_above(ratio,one_value))memory.store32(ratio_clamp,frame.argument(argument::ratio),one_value);
                    } else memory.store32(ratio_clamp,frame.argument(argument::ratio),start_ratio);
                }
                frame.clear_eax();
                std::uint32_t second_entry=0u;
                const auto high_again=memory.load16(second_high_again,at(kept.channel,channel::high));
                frame.compared16(high_again,kept.none);
                if(high_again!=(kept.none&0xffffu)) {
                    const auto tag=memory.load32(second_tag,at(kept.channel,channel::handle));
                    const auto instances_address=memory.load32(second_instances,globals::tag_instances);
                    const auto definition=memory.load32(second_definition,frame.tag_index(tag)+instances_address+tag_data);
                    const auto sub=memory.load32(second_sub,at(kept.channel,channel::sub));
                    second_entry=frame.record_of(2u,definition,sub,kept);
                }
                const auto last=memory.load32(last_again,frame.argument(argument::last));
                const auto step=memory.load32(step_again,frame.argument(argument::step));
                const auto second_frames_value=widen16(memory.load16(second_frames,at(second_entry,record::frames)));
                frame.pushing_advance(last,step,kept);
                const auto converted=scalar::from_integer(second_frames_value);
                frame.converted(0u,converted);
                const auto product=sse::multiply(converted,memory.load32(ratio_load,frame.argument(argument::ratio)));
                frame.multiplied(product);
                memory.store32(product_store,frame.top(),product);
                frame.advance(kept);
            }
        }
    }
    return memory.load8(done_final,frame.local(local::done));
}
} // namespace animation_step
} // namespace halo2::native_animation_step
