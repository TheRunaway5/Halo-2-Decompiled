#pragma once
// Native TU1.5 timer sweep, entered at 115FF0 (physics live support unit):
// the five timers of an object's block stepped, and whether any answered.
//
//   timer_sweep::step   115FF0(EAX = object handle) -> AL
//
// The block sits at the object's signed offset +0x33E; its five 4-byte
// timers from +0x80 each go to 1E15D0 (ECX the timer), and the result is
// the OR of their answers. The answers gather in the register the original
// keeps across the calls, so the one each call returns counts.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_timer_sweep.hpp) maps the role to the original instruction
// and owns the ABI of 1E15D0; the block and the gathered answers the
// original keeps in ESI and BL across it are a Kept, refreshed from the
// registers it returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_timer_sweep {
using native_object_update::at;
using native_object_update::widen16;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u;}   // DataArray* of 12-byte object headers
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u,block_offset=0x33eu;
inline constexpr std::uint32_t timers=0x80u,timer_size=4u,timer_count=5u;

namespace timer_sweep {
enum class Access {headers,header_datums,object_address,block};
struct Kept {std::uint32_t block,answers;};   // ESI, BL
// Frame: step(kept, timer) 1E15D0, refreshes kept, returns AL; gathered(kept, k)
// the answers so far (after the k-th call).
template<class Memory,class Frame> std::uint32_t step(Memory &memory,Frame &frame,std::uint32_t handle) {
    using enum Access;
    const auto table=memory.load32(headers,globals::object_headers);
    const auto datums=memory.load32(header_datums,at(table,datum_data));
    const auto object=memory.load32(object_address,datums+header_size*(handle&0xffffu)+header_address);
    Kept kept{widen16(memory.load16(Access::block,at(object,block_offset)))+object,0u};
    for(std::uint32_t k=0;;++k) {
        const auto answer=frame.step(kept,at(kept.block,timers+timer_size*k))&0xffu;
        if(k+1==timer_count)return (answer|kept.answers)&0xffu;
        kept.answers=k==0?answer:(kept.answers|answer);
        frame.gathered(kept,k);
    }
}
} // namespace timer_sweep
} // namespace halo2::native_timer_sweep
