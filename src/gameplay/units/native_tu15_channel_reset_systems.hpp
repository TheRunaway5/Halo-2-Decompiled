#pragma once
// Native TU1.5 channel reset, entered at 1D4E50 (physics live support and AI
// actor units): three 32-byte animation channels emptied.
//
//   channel_reset::reset   1D4E50(EAX = channels*)
//
// Each channel gets NONE in its handles (+0, +8) and words (+4, +6, +0xE)
// and bytes (+0xC, +0xD), zero in its bytes +0x10 and +0x11 and words
// +0x12, +0x14 and +0x16, the float 0 at +0x1C and the float 1 ([46A0D0])
// at +0x18, stored in the original's order.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_channel_reset.hpp) maps the role to the original
// instruction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_channel_reset {
using native_object_update::at;
using native_object_update::none;
namespace globals {inline constexpr std::uint32_t one=0x46a0d0u;}   // float 1.0
inline constexpr std::uint32_t channel_count=3u,channel_size=0x20u,zero_float=0u;
enum class Fill {none,zero,zero_float,one};
struct Field {std::uint32_t offset,bytes;Fill fill;};
// A channel's fields, in the order the original stores them.
inline constexpr Field fields[]{
    {0x00u,4u,Fill::none},{0x04u,2u,Fill::none},{0x06u,2u,Fill::none},{0x1cu,4u,Fill::zero_float},{0x10u,1u,Fill::zero},
    {0x11u,1u,Fill::zero},{0x12u,2u,Fill::zero},{0x18u,4u,Fill::one},{0x14u,2u,Fill::zero},{0x16u,2u,Fill::zero},
    {0x08u,4u,Fill::none},{0x0cu,1u,Fill::none},{0x0du,1u,Fill::none},{0x0eu,2u,Fill::none}};
inline constexpr std::uint32_t field_count=sizeof(fields)/sizeof(fields[0]);

namespace channel_reset {
enum class Access {one,field};
// Frame: field(channel, k) before each store.
template<class Memory,class Frame> void reset(Memory &memory,Frame &frame,std::uint32_t channels) {
    const auto one_value=memory.load32(Access::one,globals::one);
    for(std::uint32_t channel=0;channel<channel_count;++channel) {
        for(std::uint32_t k=0;k<field_count;++k) {
            frame.field(channel,k);
            const auto &f=fields[k];
            const auto address=at(channels,channel_size*channel+f.offset);
            const auto value=f.fill==Fill::none?none:f.fill==Fill::one?one_value:0u;
            if(f.bytes==4)memory.store32(Access::field,address,value);
            else if(f.bytes==2)memory.store16(Access::field,address,value&0xffffu);
            else memory.store8(Access::field,address,value&0xffu);
        }
    }
}
} // namespace channel_reset
} // namespace halo2::native_channel_reset
