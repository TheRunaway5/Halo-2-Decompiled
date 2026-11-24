#pragma once
// Native TU1.5 event window, entered at 1CEC60 (physics live support and AI
// actor units; TU1.1 1CA3A0): the events an animation channel's next step
// can reach, as a mask.
//
//   event_window::mark   1CEC60(ESI = channel)
//
// The channel's upcoming events word (+0x16) is cleared. A channel playing an
// animation (index +6 not NONE; the animation from 1E3150, EDX the graph's
// definition through the tag instances at 4F0414, the channel's +4 pushed)
// takes the window between its frame (+0x1C) and a limit: the animation's
// frame count (int16 +0x14) plus 1 (46A0D0) when its rate (+0x18) is above
// 0, else 0. Each of the animation's events (a count +0x4C, 4-byte events
// at +0x50: the bit +0, the frame int16 +2) whose frame lies in the window
// sets its bit in the word: the window includes its far end and excludes
// the channel's frame when the frame is the window's start (UCOMISS equal),
// and the other way round otherwise. The original walks the events four at a
// time while four remain, then one at a time; its loads follow that order.
//
// The single-precision arithmetic and compares are the original's scalar
// SSE, in its order, on the live MXCSR. This is the game logic only. The
// game's memory is the retained TU1.5 bank, reached through a Memory object
// with typed loads and stores; every access names its role (Access), and
// the boundary adapter (native_tu15_event_window.hpp) maps the role to the
// original instruction and owns the ABI of the child. The channel, the
// animation and the event index the original keeps in ESI, EDI and EBX
// across it are a Kept, refreshed from the registers it returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "gameplay/units/native_tu15_record_fade_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_event_window {
using native_object_update::at;
using native_object_update::signed_value;
using native_object_update::widen16;
namespace sse=native_aim_clusters::sse;
namespace scalar=native_record_fade::scalar;
namespace globals {inline constexpr std::uint32_t tag_instances=0x4f0414u,one=0x46a0d0u;}
inline constexpr std::uint32_t tag_size=16u,tag_data=8u,event_size=4u,group=4u;
namespace channel {inline constexpr std::uint32_t graph=0u,animation=4u,index=6u,upcoming=0x16u,rate=0x18u,frame=0x1cu;}
namespace animation {inline constexpr std::uint32_t frames=0x14u,event_count=0x4cu,events=0x50u;}
namespace event {inline constexpr std::uint32_t bit=0u,frame=2u;}

namespace event_window {
enum class Access {index,upcoming_clear,graph_tag,instances,graph_definition,animation_word,rate,window_frame,frame_count,one,event_count,
    count_store,events,event_frame,channel_frame,event_bit,upcoming_load,upcoming_store,count_load};
enum class Outcome {not_playing,marked};
// The original's ESI (the channel), EDI (the animation) and EBX (the event index) across 1E3150.
struct Kept {std::uint32_t channel,animation,index;};
// Frame:
//   cleared(kept), saving_edi(), saving_ebp(), restoring(walked)   XOR EBX,EBX; the saved registers
//   playing_tested(index)                        CMP word [ESI+6],-1
//   animation_of(graph, word, kept) -> EAX       1E3150(EDX = graph; the channel's +4 pushed), refreshes kept
//   window(forward, frame, limit, start, end)    the window's XORPS, COMISS, CVTSI2SS/ADDSS and min/max in XMM0..2
//   counted(count), grouped(bound), remaining(index, bound)   CMP EBP,4; ADD EBP,-3 (the groups' bound); CMP EBX,EBP
//   slot(n)                                      the event's place in its group (0..3), or 4 for the rest
//   event_tested(event, frame, at_frame, from_start, inside)   the event's address and frame in the original's registers and flags
//   bit_set(bit, word)                           MOV EAX (EDX),1; SHL; OR word [ESI+16]
//   advanced(index, bound, grouped)              ADD EBX,4 or INC EBX; CMP EBX,EBP
template<class Memory,class Frame> Outcome mark(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    kept.index=0u;
    frame.cleared(kept);
    const auto index_value=memory.load16(Access::index,at(kept.channel,channel::index));
    frame.playing_tested(index_value);
    memory.store16(upcoming_clear,at(kept.channel,channel::upcoming),0u);
    if(index_value==0xffffu)return Outcome::not_playing;
    const auto tag_value=memory.load32(graph_tag,at(kept.channel,channel::graph));
    const auto instances_address=memory.load32(instances,globals::tag_instances);
    const auto graph=memory.load32(graph_definition,tag_size*(tag_value&0xffffu)+instances_address+tag_data);
    const auto word=memory.load32(animation_word,at(kept.channel,channel::animation));
    frame.saving_edi();
    kept.animation=frame.animation_of(graph,word,kept);
    // The window: from the smaller of the frame and the limit to the larger.
    const auto rate_value=memory.load32(rate,at(kept.channel,channel::rate));
    const bool forward=!sse::not_above(rate_value,0u);
    const auto frame_value=memory.load32(window_frame,at(kept.channel,channel::frame));
    std::uint32_t limit=0u;
    if(forward) {
        const auto converted=scalar::from_integer(widen16(memory.load16(frame_count,at(kept.animation,animation::frames))));
        limit=sse::add(converted,memory.load32(one,globals::one));
    }
    const auto start=sse::not_above(frame_value,limit)?frame_value:limit;
    const auto end=sse::not_above(frame_value,limit)?limit:frame_value;
    frame.window(forward,frame_value,limit,start,end);
    frame.saving_ebp();
    const auto count=memory.load32(event_count,at(kept.animation,animation::event_count));
    frame.counted(count);
    memory.store32(count_store,frame.local(),count);
    // One event: its frame against the window, its bit set when inside.
    const auto visit=[&](unsigned slot,std::uint32_t k) {
        frame.slot(slot);
        const auto base=memory.load32(events,at(kept.animation,animation::events));
        const auto address=base+event_size*k;
        // UCOMISS XMM0,[ESI+1C] reads the channel's frame; the first three of a group read the event's frame before it.
        std::uint32_t event_frame_value=0u;
        if(slot<3u)event_frame_value=widen16(memory.load16(event_frame,at(address,event::frame)));
        const bool from_start=scalar::ordered_equal(start,memory.load32(channel_frame,at(kept.channel,channel::frame)));
        if(slot>=3u)event_frame_value=widen16(memory.load16(event_frame,at(address,event::frame)));
        const auto at_frame=scalar::from_integer(event_frame_value);
        const bool inside=from_start
            ? !sse::not_above(at_frame,start) && !sse::below(end,at_frame)
            : !sse::below(at_frame,start) && !sse::not_above(end,at_frame);
        frame.event_tested(address,event_frame_value,at_frame,from_start,inside);
        if(inside) {
            const auto bit_value=memory.load8(event_bit,at(address,event::bit));
            const auto upcoming=memory.load16(upcoming_load,at(kept.channel,channel::upcoming))|((1u<<(bit_value&31u))&0xffffu);
            frame.bit_set(bit_value,upcoming);
            memory.store16(upcoming_store,at(kept.channel,channel::upcoming),upcoming);
        }
    };
    auto bound=count;
    if(!(signed_value(count)<signed_value(group))) {
        bound=count-(group-1u);
        frame.grouped(bound);
        do {
            for(unsigned slot=0;slot<group;++slot)visit(slot,kept.index+slot);
            kept.index+=group;
            frame.advanced(kept.index,bound,true);
        } while(signed_value(kept.index)<signed_value(bound));
        bound=memory.load32(count_load,frame.local());
    }
    frame.remaining(kept.index,bound);
    while(signed_value(kept.index)<signed_value(bound)) {
        visit(4u,kept.index);
        ++kept.index;
        frame.advanced(kept.index,bound,false);
    }
    frame.restoring();
    return Outcome::marked;
}
} // namespace event_window
} // namespace halo2::native_event_window
