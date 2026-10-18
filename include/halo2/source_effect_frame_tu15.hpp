#pragma once
#include "halo2/cache.hpp"
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <span>

namespace halo2 {
// Selected effect and environment owners from original TU1.5 13A9B0. The native
// outer caller supplies13AA50's accepted interval once, after its game ticks.
// Other13A9B0 prelude owners and the render clock remain separate.
// Existing journaled World calls own all mutation and replay observations.
template<class World,class Observer>
void source_effect_frame_tu15(World &world,float seconds,const Observer &observer) {
    if(!std::isfinite(seconds) || seconds<0)
        throw FormatError("TU1.5 effect frame requires a finite nonnegative accepted interval");
    // Literal13A9B6 compares against46A0E8 (positive zero), JBE skips the
    // contrail/decal prelude. Literal13AA28 always calls179DF0, including0.
    if(seconds>0.f) {
        const std::array delta{std::bit_cast<std::uint32_t>(seconds)};
        (void)world.call_actual(0x17ead0u,delta);
        (void)world.call_actual(0x180520u,std::span<const std::uint32_t>{});
        // Literal13A9E2 pushes the accepted float;1378E0 returns with RET4.
        (void)world.call_actual(0x1378e0u,delta);
    }
    world.advance_effect_particles(seconds,observer);
}
}
