#pragma once
#include "rendering/models/native_tu15_prt_basis.hpp"

namespace halo2::prt_scaled_basis_detail {
using namespace prt_projection_detail;
struct Caller {
    std::uint32_t normal, order, stack, saved_bx, saved_bp, saved_si, saved_di;
};
struct ScaledBasis {
    prt_basis_detail::Basis basis;
    __m128 scale{}, factor{}, accumulated{};
    std::uint32_t last_output{}, processed{}, remaining{};
};
// Complete148B20: evaluate the real-SH basis once, then scale the requested
// RGB outputs. Four-coefficient blocks and scalar tails preserve source
// operand grouping, caller-slot rereads and ordered overlapping stores.
inline ScaledBasis evaluate_scaled(SourcePhysicsContactMemory& memory, const Caller& caller,
    const SourcePhysicsContactRecordObserver& observe) {
    const auto frame = caller.stack - 0x50u;
    const auto get = [&](std::uint32_t offset) { return read(memory, frame + offset); };
    store(memory, caller.stack - 0x44u, caller.saved_bx, 4, 0x148b23u, observe);
    store(memory, caller.stack - 0x48u, caller.saved_bp, 4, 0x148b24u, observe);
    const auto red = get(0x60);
    store(memory, caller.stack - 0x4cu, caller.saved_si, 4, 0x148b29u, observe);
    store(memory, caller.stack - 0x50u, caller.saved_di, 4, 0x148b2au, observe);
    store(memory, frame - 4u, 0x148b34u, 4, 0x148b2fu, observe);
    ScaledBasis result;
    result.basis = prt_basis_detail::evaluate(memory, caller.normal, frame + 0x10u,
        caller.order, frame - 4u, observe);
    (void)read(memory, frame - 4u);
    result.last_output = frame + 0x10u;
    const auto divisor = scalar(read(memory, caller.order <= 2u ? 0x46a13cu : 0x46a1b0u));
    result.factor = scalar(read(memory, 0x46a5a8u));
    result.remaining = caller.order * caller.order;
    result.factor = _mm_div_ss(result.factor, divisor);
    result.scale = multiply(result.factor, scalar(get(0x54)), false);
    result.accumulated = result.basis.third;
    for (unsigned channel = 0; channel < 3; ++channel) {
        auto destination = channel == 0 ? red : get(0x60 + channel * 4);
        if (channel && !destination) continue;
        result.processed = 0;
        if (channel == 1) result.scale = multiply(result.factor, scalar(get(0x58)), false);
        if (channel == 2) result.factor = multiply(result.factor, scalar(get(0x5c)), false);
        const auto scale = channel == 2 ? result.factor : result.scale;
        const auto publish = [&](std::uint32_t source, std::uint32_t target, std::uint32_t pc) {
            const auto value = multiply(scalar(read(memory, source)), scale, false);
            if (channel == 2) result.scale = value;
            else result.accumulated = value;
            store(memory, target, bits(value), 4, pc, observe);
        };
        if (std::int32_t(result.remaining) >= 4) {
            const auto target_base = destination;
            result.last_output = destination + 8u;
            constexpr std::array<std::array<std::uint32_t, 4>, 3> sites{{
                {0x148b8au, 0x148b99u, 0x148ba7u, 0x148bb4u},
                {0x148c2bu, 0x148c3au, 0x148c48u, 0x148c55u},
                {0x148ccau, 0x148cd9u, 0x148ce7u, 0x148cf4u}}};
            do {
                for (unsigned word = 0; word < 4; ++word)
                    publish(frame + 0x10u + (result.processed + word) * 4u,
                        target_base + (result.processed + word) * 4u, sites[channel][word]);
                result.processed += 4u;
                result.last_output += 16u;
            } while (result.processed < result.remaining - 3u);
            if (channel < 2) destination = get(0x60 + channel * 4);
        }
        if (result.processed < result.remaining) {
            result.last_output = destination + result.processed * 4u;
            const auto source_offset = frame + 0x10u - destination;
            auto tail = result.remaining - result.processed;
            do {
                constexpr std::array sites{0x148be9u, 0x148c89u, 0x148d1cu};
                publish(result.last_output + source_offset, result.last_output, sites[channel]);
                result.last_output += 4u;
            } while (--tail);
            if (channel == 2) result.remaining = 0;
        }
    }
    return result;
}
template<class State> void bridge(State& state) {
    const Caller caller{state.gp[1], state.gp[2], state.gp[4], state.gp[3], state.gp[5], state.gp[6], state.gp[7]};
    const auto result = evaluate_scaled(state.memory, caller, state.observe);
    state.gp[0] = (result.last_output & 0xffffff00u) | 1u;
    state.gp[1] = result.processed;
    state.gp[2] = result.remaining;
    state.gp[7] = read(state.memory, caller.stack - 0x50u);
    state.gp[6] = read(state.memory, caller.stack - 0x4cu);
    state.gp[5] = read(state.memory, caller.stack - 0x48u);
    state.gp[3] = read(state.memory, caller.stack - 0x44u);
    state.xmm[0] = result.scale;
    state.xmm[1] = result.factor;
    state.xmm[2] = result.accumulated;
    state.xmm[3] = result.basis.z;
    state.xmm[4] = result.basis.x;
    state.xmm[5] = result.basis.y;
    if (result.basis.quadratic) state.xmm[6] = result.basis.sixth;
    state.xmm[7] = result.basis.half;
    state.fp.status = (state.fp.status & ~0x200u) | (result.basis.final_status & 0x200u);
    state.flags.plus(caller.stack - 0x40u, 0x40u, 4);
    (void)read(state.memory, caller.stack);
    state.gp[4] = caller.stack + 28u;
}
}
