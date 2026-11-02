#pragma once
#include "halo2/native_tu15_lens_query_slots.hpp"

namespace halo2 {
// Compatibility boundary for the remaining generated callers. The semantic
// kernel receives a slot and retained storage, never a CPU frame. Keep only
// the caller-visible return values here; no translated body remains locally.
template<class State> void native_tu15_lens_query_slot_bridge(State& state) {
    const auto slot = state.gp[0];
    // Preserve the genuine saved-register stack write as well as the12
    // semantic query stores, for whole-stack and ordered-store observers.
    state.push(0x20e30u, state.gp[6]);
    const auto flags = native_tu15_retire_lens_query_slot(state.memory, slot, state.observe);
    state.gp[0] = state.flags.logic(flags, 4);
    state.gp[1] = 0x52dfdcu + slot * 32;
    state.gp[2] = ~(std::uint32_t{1} << (slot & 31));
    state.gp[6] = state.pop();
    state.pop();
}
}
