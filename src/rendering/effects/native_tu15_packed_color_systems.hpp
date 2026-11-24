#pragma once
// Native TU1.5 packed colour, entered at 18BBC0 (effect particle frame
// unit): a colour becomes one 32-bit word of three 11-bit channels.
//
//   packed_color::pack   18BBC0(EAX = colour) -> EAX
//
// Each channel (blue +8, green +4, red +0) is clamped to [46A0DC, 46A0D0]
// (COMISS; a NaN keeps its value), offset from the low end and scaled (blue
// by 46A338, green and red by 46A33C), and rounded to an integer on the x87
// under the live control word (FLD m32, FISTP m32). The result is
// (blue << 22) | (green << 11) | red, as the original's shifts and ORs give
// it.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_packed_color.hpp) maps the role to the original instruction
// and rounds on the logical x87.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_packed_color {
using native_object_update::at;
namespace sse=native_aim_clusters::sse;
namespace globals {
inline constexpr std::uint32_t low=0x46a0dcu,high=0x46a0d0u,blue_scale=0x46a338u,other_scale=0x46a33cu;   // floats
}
inline constexpr std::uint32_t channel_shift=11u;
enum class Channel : unsigned {blue,green,red};
constexpr std::uint32_t channel_offset(Channel c){return c==Channel::blue?8u:c==Channel::green?4u:0u;}

namespace packed_color {
enum class Local {spill,blue,green,red};
enum class Access {low,high,first,second,third,blue_scale,spill_store,spill_x87,rounded_store,other_scale,rounded_read};
// Frame:
//   local(Local)                 a local's address now
//   channel(c)                   the channel read next
//   clamped(c, value, kind)      the channel's clamp result (its register)
//   rounded(value)               FLD m32; FISTP m32 on the logical x87
//   scaled(c, value, scale)      the scaled channel (its register)
enum class Clamp {to_low,to_high,kept};
template<class Memory,class Frame> std::uint32_t pack(Memory &memory,Frame &frame,std::uint32_t colour) {
    using enum Access;
    const auto local=[&](Local l){return frame.local(l);};
    const auto low_value=memory.load32(low,globals::low);
    const auto high_value=memory.load32(high,globals::high);
    std::uint32_t clamped[3]{};
    for(const auto c:{Channel::blue,Channel::green,Channel::red}) {
        frame.channel(c);
        const auto address=at(colour,channel_offset(c));
        auto &value=clamped[unsigned(c)];
        if(!sse::not_above(low_value,memory.load32(first,address))){value=low_value;frame.clamped(c,value,Clamp::to_low);continue;}
        const auto again=memory.load32(second,address);
        if(!sse::not_above(again,high_value)){value=high_value;frame.clamped(c,value,Clamp::to_high);continue;}
        value=c==Channel::red?again:memory.load32(third,address);
        frame.clamped(c,value,Clamp::kept);
    }
    // Scaled from the low end, rounded on the x87.
    const auto round=[&](Channel c,std::uint32_t scaled,Local slot) {
        memory.store32(spill_store,local(Local::spill),scaled);
        memory.store32(rounded_store,local(slot),frame.rounded(memory.load32(spill_x87,local(Local::spill))));
        (void)c;
    };
    const auto blue_offset=sse::subtract(clamped[0],low_value);
    const auto blue=sse::multiply(blue_offset,memory.load32(blue_scale,globals::blue_scale));
    frame.scaled(Channel::blue,blue,0u);
    round(Channel::blue,blue,Local::blue);
    const auto scale=memory.load32(other_scale,globals::other_scale);
    const auto green=sse::multiply(sse::subtract(clamped[1],low_value),scale);
    frame.scaled(Channel::green,green,scale);
    round(Channel::green,green,Local::green);
    const auto red=sse::multiply(sse::subtract(clamped[2],low_value),scale);
    frame.scaled(Channel::red,red,scale);
    round(Channel::red,red,Local::red);
    const auto b=memory.load32(rounded_read,local(Local::blue));
    const auto g=memory.load32(rounded_read,local(Local::green));
    const auto packed=(b<<channel_shift)|g;
    return (packed<<channel_shift)|memory.load32(rounded_read,local(Local::red));
}
} // namespace packed_color
} // namespace halo2::native_packed_color
