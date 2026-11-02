#pragma once
#include "rendering/models/native_tu15_render_ownership_transport.hpp"
#include <bit>
#include <emmintrin.h>

namespace halo2 {
// The caller already owns its installed FP environment. Borrow its registers
// directly; leave x87, MMX, unused XMM lanes and the environment in place.
template<class State> void native_tu15_render_object_ownership_bridge(State& state, std::uint32_t entry) {
    auto flags = state.flags.bits();
    NativeTu15PhysicsFrameFp fp;
    render_ownership_detail::Registers registers{state.gp, flags, &state,
        [](void* context, std::uint32_t blend) {
            static_cast<State*>(context)->xmm[0] = _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(blend)));
        }};
    try {render_ownership_detail::run_transport(state.memory, entry, registers, fp, state.observe, true);}
    catch (...) {state.flags.load(flags); throw;}
    state.flags.load(flags);
}
}
