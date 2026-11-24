#pragma once
// Native TU1.5 channel seek, entered at 1CEBD0 (scene device constructor
// unit): a channel set to a fraction of its record's frames, then advanced.
//
//   channel_seek::seek   1CEBD0(ESI = channel; fraction, a, b, c) -> RET 0x10
//
// A channel in use (word +6 not NONE) has its record (1E3150, through the
// dispatch: EDX its definition through the tag instances at 4F0414, its sub
// +4 pushed); one not in use reads the record at 0. The channel's flag byte
// (+0x10) clears; the fraction becomes a frame, the fraction times (the
// record's frames (int16 +0x14) less one, plus [46A0EC]), clamped to 0 and
// that end (COMISS, the original's order), kept in the fraction's slot; and
// 1CE950 (through the dispatch: the channel, the frame, a, b and c pushed)
// advances the channel.
//
// The single-precision arithmetic is the original's scalar SSE, in its order,
// on the live MXCSR. This is the game logic only. The game's memory is the
// retained TU1.5 bank, reached through a Memory object with typed loads and
// stores; every access names its role (Access); the boundary adapter
// (native_tu15_channel_seek.hpp) maps the role to the original instruction
// and owns the ABI of the children. The channel the original keeps in ESI
// across them is a Kept, refreshed from the registers each returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "gameplay/units/native_tu15_record_fade_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_channel_seek {
using native_object_update::at;
using native_object_update::widen16;
namespace sse=native_aim_clusters::sse;
namespace scalar=native_record_fade::scalar;
namespace globals {inline constexpr std::uint32_t tag_instances=0x4f0414u,margin=0x46a0ecu;}
inline constexpr std::uint32_t tag_size=16u,tag_data=8u,zero_float=0u;
namespace channel {inline constexpr std::uint32_t tag=0u,sub=4u,high=6u,flags=0x10u;}
namespace record {inline constexpr std::uint32_t frames=0x14u;}
namespace argument {inline constexpr std::uint32_t fraction=0u,a=1u,b=2u,c=3u;}

namespace channel_seek {
enum class Access {high,tag,instances,definition,sub,flags_clear,frames,margin,fraction,frame_zero,frame_end,frame_store,c_value,b_value,
    a_value,frame_load};
enum class Outcome {seeked,unused};
// The original's ESI (the channel) across the children.
struct Kept {std::uint32_t channel;};
// Frame: argument(k), cleared() XOR EAX,EAX; high_tested(high); record(definition, sub, kept) [1E3150];
// frame_found(frames, end, product) the CVTSI2SS/ADDSS/MULSS registers; compared(a, b) COMISS;
// pushing(c, b, a), advance(frame, kept) [1CE950].
template<class Memory,class Frame> Outcome seek(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    frame.cleared();
    const auto high_value=memory.load16(high,at(kept.channel,channel::high));
    frame.high_tested(high_value);
    std::uint32_t entry=0u;
    const bool used=high_value!=0xffffu;
    if(used) {
        const auto tag_value=memory.load32(Access::tag,at(kept.channel,channel::tag));
        const auto instances_address=memory.load32(instances,globals::tag_instances);
        const auto definition_value=memory.load32(definition,frame.tag_index(tag_value)+instances_address+tag_data);
        const auto sub_value=memory.load32(sub,at(kept.channel,channel::sub));
        entry=frame.record(definition_value,sub_value,kept);
    }
    frame.zeroed();
    memory.store8(flags_clear,at(kept.channel,channel::flags),0u);
    // The frame: the fraction of the frames (less one, plus the margin), clamped to 0 and that end.
    const auto frames_value=widen16(memory.load16(frames,at(entry,record::frames)));
    const auto last=frame.last_frame(frames_value);
    const auto converted=scalar::from_integer(last);
    frame.converted(converted);
    const auto end=sse::add(converted,memory.load32(margin,globals::margin));
    const auto product=sse::multiply(end,memory.load32(fraction,frame.argument(argument::fraction)));
    frame.computed(end,product);
    frame.compared(zero_float,product);
    if(!sse::not_above(zero_float,product))memory.store32(frame_zero,frame.argument(argument::fraction),zero_float);
    else {
        frame.compared(product,end);
        memory.store32(frame_end,frame.argument(argument::fraction),end);
        if(sse::not_above(product,end))memory.store32(frame_store,frame.argument(argument::fraction),product);
    }
    const auto c=memory.load32(Access::c_value,frame.argument(argument::c));
    const auto b=memory.load32(Access::b_value,frame.argument(argument::b));
    const auto a=memory.load32(Access::a_value,frame.argument(argument::a));
    frame.pushing(c);
    const auto frame_value=memory.load32(frame_load,frame.argument(argument::fraction));
    frame.advance(b,a,frame_value,kept);
    return used?Outcome::seeked:Outcome::unused;
}
} // namespace channel_seek
} // namespace halo2::native_channel_seek
