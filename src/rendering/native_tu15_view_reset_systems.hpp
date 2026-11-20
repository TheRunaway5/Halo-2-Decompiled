#pragma once
// Native TU1.5 render-view queue reset: 1373F0, pure leaf shared by
// source_shadow_frame and source_effect_particle_draw.
//
//   view_reset::reset(memory, frame)
//     Resets a RenderView (EDX) for a new visibility pass:
//       - clears count (+4) on each of the four render queues (+0x0C..+0x18);
//       - zeroes the 16-dword cluster visibility bitmask (+0x21C);
//       - zeroes the active cluster entry table (+0x25C, length = view->entry_count
//         dwords from +0xA5C);
//       - clears entry_count (+0xA5C) and active_count (+0xA5E);
//       - fills the 128-dword cluster lookup table (+0x1C) with NONE (-1).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access) and the boundary adapter
// (native_tu15_view_reset.hpp) maps the role to the original instruction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_view_reset {
using native_object_update::at;
using native_object_update::none;

// ------------------------------------------------------------- game layouts
struct RenderQueue {
    std::uint32_t capacity;         // +0x00
    std::uint16_t count;            // +0x04
};
static_assert(offsetof(RenderQueue,count)==4);

struct RenderView {
    std::uint32_t unused_00[3];     // +0x00..+0x0B
    std::uint32_t queues[4];        // +0x0C, +0x10, +0x14, +0x18
    std::uint32_t cluster_slots[0x80]; // +0x1C..+0x21B
    std::uint32_t bitmask[0x10];    // +0x21C..+0x25B
    std::uint32_t entries[0x200];   // +0x25C..+0xA5B
    std::uint16_t entry_count;      // +0xA5C
    std::uint16_t active_count;     // +0xA5E
};
static_assert(offsetof(RenderView,queues)==0x0c &&
              offsetof(RenderView,cluster_slots)==0x1c &&
              offsetof(RenderView,bitmask)==0x21c &&
              offsetof(RenderView,entries)==0x25c &&
              offsetof(RenderView,entry_count)==0xa5c &&
              offsetof(RenderView,active_count)==0xa5e);

inline constexpr std::uint32_t bitmask_dwords=0x10u;
inline constexpr std::uint32_t cluster_slot_dwords=0x80u;

// ------------------------------------------------------------ access roles
enum class Access : unsigned {
    queue0,clear_queue0,queue1,clear_queue1,queue2,clear_queue2,queue3,clear_queue3,
    entry_count,clear_entry_count,clear_active_count
};

namespace view_reset {
template<class Memory,class Frame> void reset(Memory &memory,Frame &frame) {
    using enum Access;
    const auto view=frame.entry_view();
    const auto q0=memory.load32(queue0,at(view,offsetof(RenderView,queues)));
    frame.save_prolog();
    memory.store16(clear_queue0,at(q0,offsetof(RenderQueue,count)),0u);
    const auto q1=memory.load32(queue1,at(view,offsetof(RenderView,queues)+4u));
    memory.store16(clear_queue1,at(q1,offsetof(RenderQueue,count)),0u);
    const auto q2=memory.load32(queue2,at(view,offsetof(RenderView,queues)+8u));
    memory.store16(clear_queue2,at(q2,offsetof(RenderQueue,count)),0u);
    const auto q3=memory.load32(queue3,at(view,offsetof(RenderView,queues)+12u));
    memory.store16(clear_queue3,at(q3,offsetof(RenderQueue,count)),0u);

    frame.fill_bitmask(at(view,offsetof(RenderView,bitmask)),bitmask_dwords,0u);
    const auto count=memory.load16(entry_count,at(view,offsetof(RenderView,entry_count)));
    frame.fill_entries(at(view,offsetof(RenderView,entries)),count,0u);
    memory.store16(clear_entry_count,at(view,offsetof(RenderView,entry_count)),0u);
    memory.store16(clear_active_count,at(view,offsetof(RenderView,active_count)),0u);
    frame.fill_cluster_slots(at(view,offsetof(RenderView,cluster_slots)),cluster_slot_dwords,none);
    frame.finish();
}
} // namespace view_reset
} // namespace halo2::native_view_reset
