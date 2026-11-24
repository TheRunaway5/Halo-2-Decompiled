#pragma once
// Native TU1.5 lens broadcast, entered at 20700 (lens flare unit): a lens
// handle handed to each view, or kept for later.
//
//   lens_broadcast::send   20700(EAX = handle)
//
// A handle (not NONE) is kept ([532094]) while the views are deferred (byte
// 52DFD8 set and no views yet, [4918E8] zero); otherwise 20E30 takes each
// view index below the view count ([531FBC], read again after each).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_lens_broadcast.hpp) maps the role to the original instruction
// and owns the ABI of the child; the index, which the original keeps in ESI
// across it, is refreshed from the register it returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_lens_broadcast {
using native_object_update::none;
using native_object_update::signed_value;
namespace globals {
inline constexpr std::uint32_t deferring=0x52dfd8u;   // byte
inline constexpr std::uint32_t views=0x4918e8u;       // nonzero once there are views
inline constexpr std::uint32_t kept=0x532094u;        // the handle kept for later
inline constexpr std::uint32_t count=0x531fbcu;       // the view count
}
namespace lens_broadcast {
enum class Access {deferring,views,kept_store,count,count_again};
enum class Outcome {nothing,kept,sent};
// Frame: each(index) -> index   20E30(EAX = index)
template<class Memory,class Frame> Outcome send(Memory &memory,Frame &frame,std::uint32_t handle) {
    using enum Access;
    if(handle==none)return Outcome::nothing;
    if((memory.load8(deferring,globals::deferring)&0xffu) && !memory.load32(views,globals::views)) {
        memory.store32(kept_store,globals::kept,handle);
        return Outcome::kept;
    }
    auto views_value=memory.load32(count,globals::count);
    if(signed_value(views_value)>0) {
        std::uint32_t index=0u;
        do {
            index=frame.each(index);
            views_value=memory.load32(count_again,globals::count);
            ++index;
        } while(signed_value(index)<signed_value(views_value));
    }
    return Outcome::sent;
}
} // namespace lens_broadcast
} // namespace halo2::native_lens_broadcast
