#pragma once
#include "rendering/models/native_tu15_prt_rotation_polynomial.hpp"

namespace halo2::prt_rotation_detail {
template<class State> void bridge(State& state) {
    const auto stack = state.gp[4];
    const Caller caller{state.gp[0], state.gp[1], state.gp[6], stack,
        state.gp[3], state.gp[5], state.gp[7], std::uint16_t(state.fp.status & 0x200u)};
    Frame frame{state.memory, state.observe};
    const auto result = rotate_polynomial(frame, caller);
    // Native continuation publication is separate from the numerical body.
    state.gp[7] = frame.word(stack - 0x30cu);
    state.gp[5] = frame.word(stack - 0x308u);
    state.gp[0] = result.output;
    state.gp[3] = frame.word(stack - 0x304u);
    state.gp[1] = 0;
    state.gp[2] = result.source;
    state.gp[6] = result.output;
    const auto publish = [](float value) { return prt_projection_detail::scalar(std::bit_cast<std::uint32_t>(value)); };
    state.xmm[0] = publish(result.first); state.xmm[1] = publish(result.second);
    state.xmm[2] = publish(result.third); state.xmm[3] = publish(result.fourth);
    state.xmm[4] = publish(result.fifth); state.xmm[5] = publish(result.sixth);
    state.xmm[6] = publish(result.seventh); state.xmm[7] = publish(result.eighth);
    state.fp.status = (state.fp.status & ~0x200u) | (result.x87_c1 & 0x200u);
    state.flags.plus(stack - 0x300u, 0x300u, 4);
    (void)frame.word(stack);
    state.gp[4] = stack + 8u;
}
}
