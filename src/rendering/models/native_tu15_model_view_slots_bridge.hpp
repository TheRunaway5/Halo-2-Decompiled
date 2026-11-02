#pragma once
#include "halo2/native_tu15_model_view_slots.hpp"
#include <stdexcept>

namespace halo2 {
// Continue the remaining22830 BSP owner at2286E. The source caller has
// already saved its nonvolatile registers. Public native rendering calls
// have the ordinary forward-string ABI; reject an incompatible DF explicitly.
template<class State> void native_tu15_model_view_slots_bridge(State& state) {
    if (state.other_flags & 1024u)
        throw std::invalid_argument("Native TU1.5 model slots require the forward-string caller ABI");
    native_tu15_reset_model_view_slots(state.memory, state.observe);
    state.gp[0] = 0;
    state.gp[1] = 0x4c5b74u;
    state.gp[2] = 0x4c5b94u;
    state.gp[6] = 0xffffffffu;
    state.gp[7] = 0x4c5b74u;
    state.flags.minus(0x4c5b94u, 0x4c5b94u, 4);
}
}
