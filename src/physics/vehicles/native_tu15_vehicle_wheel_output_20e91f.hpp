#pragma once

#include "physics/runtime/native_tu15_world_leaves.hpp"
#include "physics/vehicles/native_tu15_vehicle_wheel_stage.hpp"

namespace halo2::native_vehicle_wheel_output_20e91f_detail {
using namespace native_vehicle_wheel_stage_detail;

inline constexpr std::uint32_t next_wheel = 0x209b14u;
inline constexpr std::uint32_t after_loop = 0x20a12cu;
inline constexpr std::uint32_t sites[]{
    0x20a01fu, 0x20a024u, 0x20a029u, 0x20a02eu, 0x20a033u, 0x20a038u, 0x20a03du,
    0x20a042u, 0x20a047u, 0x20a04cu, 0x20a051u, 0x20a056u, 0x20a05bu, 0x20a060u,
    0x20a065u, 0x20a06au, 0x20a06fu, 0x20a074u, 0x20a07bu, 0x20a07fu, 0x20a084u,
    0x20a089u, 0x20a08du, 0x20a091u, 0x20a096u, 0x20a09bu, 0x20a0a0u, 0x20a0a5u,
    0x20a0a9u, 0x20a0aeu, 0x20a0b3u, 0x20a0b7u, 0x20a0bbu, 0x20a0c0u, 0x20a0c5u,
    0x20a0cau, 0x20a0cfu, 0x20a0d4u, 0x20a0d9u, 0x20a0e0u, 0x20a0e5u, 0x20a0e9u,
    0x20a0edu, 0x20a0f2u, 0x20a0f7u, 0x20a0fcu, 0x20a101u, 0x20a106u, 0x20a10bu,
    0x20a10fu, 0x20a112u, 0x20a113u, 0x20a116u, 0x20a11cu, 0x20a11eu, 0x20a122u,
    0x20a126u,
};

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
using native_tu15_leaves::ordered_difference_ss;
using native_tu15_leaves::ordered_mul_ss;
using native_world_leaves_detail::ordered_add_ss;

struct ComponentSites {
    std::uint32_t load, add, store;
};

inline void accumulate_component(Provider &io, __m128 &value, ComponentSites pc,
                                 std::uint32_t left, std::uint32_t right, std::uint32_t output) {
    value = io.scalar(pc.load, left);
    value = ordered_add_ss(value, io.scalar(pc.add, right));
    io.store(pc.store, output, value);
}

inline void compute_torque(MotionState &state, Provider &io) {
    auto &x = state.xmm;
    auto &gp = state.gp;
    const auto &wheel = gp[7];

    // Form the cross product from the updated wheel force. Reload each input
    // at its original provider boundary so overlapping outputs remain valid.
    x[0] = io.scalar(0x20a04cu, wheel + 0xcu);
    x[0] = ordered_mul_ss(x[0], io.scalar(0x20a051u, wheel - 0x10u));
    x[1] = io.scalar(0x20a056u, wheel - 0xcu);
    x[1] = ordered_mul_ss(x[1], io.scalar(0x20a05bu, wheel + 8u));
    x[2] = io.scalar(0x20a060u, wheel + 0x10u);
    x[2] = ordered_mul_ss(x[2], io.scalar(0x20a065u, wheel - 0x10u));
    x[3] = io.scalar(0x20a06au, wheel + 0xcu);
    x[3] = ordered_mul_ss(x[3], io.scalar(0x20a06fu, wheel - 8u));
    gp[0] = io.word(0x20a074u, gp[4] + 0x5a4u);
    io.at(0x20a07bu);
    x[0] = ordered_difference_ss(x[0], x[1]);
    x[1] = io.scalar(0x20a07fu, wheel + 8u);
    x[1] = ordered_mul_ss(x[1], io.scalar(0x20a084u, wheel - 8u));
    gp[6] = io.word(0x20a089u, gp[4] + 0x24u);
    io.at(0x20a08du);
    x[1] = ordered_difference_ss(x[1], x[2]);
    x[2] = io.scalar(0x20a091u, wheel - 0xcu);
    x[2] = ordered_mul_ss(x[2], io.scalar(0x20a096u, wheel + 0x10u));
    io.store(0x20a09bu, wheel + 0x18u, x[1]);
    io.store(0x20a0a0u, wheel + 0x1cu, x[0]);
    io.at(0x20a0a5u);
    x[2] = ordered_difference_ss(x[2], x[3]);
    io.store(0x20a0a9u, wheel + 0x14u, x[2]);
}

inline void accumulate_outputs(MotionState &state, Provider &io) {
    auto &value = state.xmm[0];
    auto &gp = state.gp;
    const auto &output = gp[0], &wheel = gp[7];

    accumulate_component(io, value, {0x20a0aeu, 0x20a0b3u, 0x20a0b7u}, wheel + 8u, output, output);
    accumulate_component(io, value, {0x20a0bbu, 0x20a0c0u, 0x20a0c5u}, output + 4u, wheel + 0xcu, output + 4u);
    accumulate_component(io, value, {0x20a0cau, 0x20a0cfu, 0x20a0d4u}, wheel + 0x10u, output + 8u, output + 8u);

    gp[0] = io.word(0x20a0d9u, gp[4] + 0x5a8u);
    accumulate_component(io, value, {0x20a0e0u, 0x20a0e5u, 0x20a0e9u}, wheel + 0x14u, output, output);
    accumulate_component(io, value, {0x20a0edu, 0x20a0f2u, 0x20a0f7u}, wheel + 0x18u, output + 4u, output + 4u);
    accumulate_component(io, value, {0x20a0fcu, 0x20a101u, 0x20a106u}, wheel + 0x1cu, output + 8u, output + 8u);
}

inline std::uint32_t wheel_output(MotionState &state, Provider &io) {
    auto &gp = state.gp;
    auto &flags = state.flags;
    auto &value = state.xmm[0];
    const auto &stack = gp[4], &wheel = gp[7];

    accumulate_component(io, value, {0x20a01fu, 0x20a024u, 0x20a029u}, wheel + 0x40u, wheel + 8u, wheel + 8u);
    accumulate_component(io, value, {0x20a02eu, 0x20a033u, 0x20a038u}, wheel + 0x44u, wheel + 0xcu, wheel + 0xcu);
    accumulate_component(io, value, {0x20a03du, 0x20a042u, 0x20a047u}, wheel + 0x48u, wheel + 0x10u, wheel + 0x10u);
    compute_torque(state, io);
    accumulate_outputs(state, io);

    gp[0] = io.word(0x20a10bu, stack + 0x1cu);
    gp[2] = io.word(0x20a10fu, gp[5] + 0x3cu);
    io.at(0x20a112u);
    gp[0] = flags.increment(gp[0], 4, false);
    io.at(0x20a113u);
    gp[6] = flags.plus(gp[6], 0x4cu, 4);
    io.at(0x20a116u);
    gp[7] = flags.plus(gp[7], 0xa8u, 4);
    io.at(0x20a11cu);
    flags.minus(gp[0], gp[2], 4);
    io.store_word(0x20a11eu, stack + 0x1cu, gp[0]);
    io.store_word(0x20a122u, stack + 0x24u, gp[6]);
    io.at(0x20a126u);
    return flags.sf != flags.of ? next_wheel : after_loop;
}
#endif

inline std::uint32_t bridge(MotionState &state) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_VEHICLE_WHEEL_OUTPUT_20E91F");
        return value && std::string_view{value} != "0";
    }();
    if (!reference) return run_stage_with_exit(state, sites, wheel_output);
#else
    (void)state;
#endif
    return 0;
}
} // namespace halo2::native_vehicle_wheel_output_20e91f_detail
