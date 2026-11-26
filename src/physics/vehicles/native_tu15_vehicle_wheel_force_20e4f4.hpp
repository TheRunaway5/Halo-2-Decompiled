#pragma once

#include "physics/runtime/native_tu15_world_leaves.hpp"
#include "physics/vehicles/native_tu15_vehicle_wheel_stage.hpp"

namespace halo2::native_vehicle_wheel_force_20e4f4_detail {
using namespace native_vehicle_wheel_stage_detail;

inline constexpr std::uint32_t call_site = 0x209dbeu;
inline constexpr std::uint32_t bypass = 0x20a01fu;
inline constexpr std::uint32_t sites[]{
    0x209bf4u, 0x209bf9u, 0x209c01u, 0x209c09u, 0x209c0cu, 0x209c11u, 0x209c16u,
    0x209c18u, 0x209c1bu, 0x209c1du, 0x209c20u, 0x209c22u, 0x209c25u, 0x209c2au,
    0x209c2du, 0x209c30u, 0x209c33u, 0x209c35u, 0x209c38u, 0x209c3du, 0x209c41u,
    0x209c44u, 0x209c47u, 0x209c48u, 0x209c4bu, 0x209c4fu, 0x209c52u, 0x209c56u,
    0x209c58u, 0x209c5bu, 0x209c5du, 0x209c62u, 0x209c64u, 0x209c69u, 0x209c6bu,
    0x209c6du, 0x209c70u, 0x209c74u, 0x209c78u, 0x209c7cu, 0x209c80u, 0x209c82u,
    0x209c85u, 0x209c87u, 0x209c8au, 0x209c8eu, 0x209c91u, 0x209c93u, 0x209c96u,
    0x209c98u, 0x209c9bu, 0x209c9fu, 0x209ca2u, 0x209ca4u, 0x209ca7u, 0x209caau,
    0x209cacu, 0x209cb4u, 0x209cb7u, 0x209cb9u, 0x209cbcu, 0x209cbeu, 0x209cc1u,
    0x209cc3u, 0x209ccbu, 0x209cceu, 0x209cd0u, 0x209cd3u, 0x209cd5u, 0x209cd8u,
    0x209cdau, 0x209cddu, 0x209ce2u, 0x209ce7u, 0x209ceau, 0x209cedu, 0x209cf1u,
    0x209cf5u, 0x209cf8u, 0x209cfcu, 0x209d02u, 0x209d06u, 0x209d0cu, 0x209d10u,
    0x209d13u, 0x209d17u, 0x209d20u, 0x209d25u, 0x209d29u, 0x209d32u, 0x209d37u,
    0x209d3eu, 0x209d41u, 0x209d43u, 0x209d4au, 0x209d4du, 0x209d51u, 0x209d57u,
    0x209d60u, 0x209d67u, 0x209d6du, 0x209d71u, 0x209d77u, 0x209d7au, 0x209d80u,
    0x209d86u, 0x209d89u, 0x209d8eu, 0x209d91u, 0x209d94u, 0x209d98u, 0x209d9au,
    0x209d9du, 0x209da0u, 0x209da6u, 0x209dacu, 0x209daeu, 0x209db4u, 0x209db7u,
};

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
using native_tu15_leaves::ordered_difference_ss;
using native_tu15_leaves::ordered_mul_ss;
using native_world_leaves_detail::ordered_add_ss;

inline void clamp_wheel_force(MotionState &state, Provider &io) {
    auto &x = state.xmm;
    auto &gp = state.gp;
    auto &flags = state.flags;
    const auto at_or_below = [&] { return flags.cf || flags.zf; };

    // Scale the request by the vehicle data and saturate it at the authored
    // positive/negative limits. The original loads leave two force factors.
    x[0] = io.scalar(0x209bf4u, gp[5] + 0x30u);
    x[0] = ordered_mul_ss(x[0], io.scalar(0x209bf9u, gp[0] + 0x8cu));
    x[0] = ordered_mul_ss(x[0], io.scalar(0x209c01u, 0x46a4b8u));
    io.at(0x209c09u);
    flags.scalar_compare(x[5], x[0], false);
    x[7] = io.scalar(0x209c0cu, gp[3] + 0x20u);
    x[3] = io.scalar(0x209c11u, gp[7] + 0x38u);
    io.at(0x209c16u);
    if (!at_or_below()) {
        x[0] = x[5];
    } else {
        flags.scalar_compare(x[0], x[6], false);
        if (!at_or_below()) x[0] = x[6];
    }

    // Use the authored direction and form a signed quadratic response.
    // Keep the original sign classification and flags for unordered inputs.
    x[2] = io.scalar(0x209c25u, gp[0] + 0x74u);
    io.at(0x209c2au);
    x[1] = _mm_setzero_ps();
    flags.scalar_compare(x[2], x[1], false);
    x[2] = flags.cf ? x[6] : x[5];
    x[4] = io.scalar(0x209c38u, gp[0] + 0x78u);
    io.at(0x209c3du);
    x[0] = x[2] = ordered_mul_ss(x[2], x[0]);
    flags.scalar_compare(x[0], x[1], true);
    state.set(0, flags.bits() | 2u, 1, 8);
    flags.logic(state.reg(0, 1, 8) & 0x44u, 1);
    x[4] = ordered_mul_ss(x[4], x[3]);
    x[2] = ordered_difference_ss(x[1], x[4]);
    if (flags.pf) {
        flags.scalar_compare(x[1], x[0], false);
        gp[0] = at_or_below() ? 1u : 0xffffffffu;
    } else {
        gp[0] = flags.logic(0, 4);
    }

    flags.scalar_compare(x[2], x[1], false);
    x[0] = ordered_mul_ss(x[0], x[0]);
    x[4] = _mm_cvtsi32_ss(x[4], si(gp[0]));
    x[0] = ordered_mul_ss(ordered_mul_ss(x[0], x[4]), x[3]);
    x[4] = flags.cf ? ordered_difference_ss(x[1], x[2]) : x[2];
    flags.scalar_compare(x[0], x[1], false);
    x[3] = flags.cf ? ordered_difference_ss(x[1], x[0]) : x[0];
    flags.scalar_compare(x[4], x[3], false);
    if (!at_or_below()) x[0] = x[2];

    // Remove the signed dead-zone, retaining the final comparison flags
    // even when the force reaches the limit or is rounded back to zero.
    flags.scalar_compare(x[0], x[1], false);
    if (!at_or_below()) {
        x[0] = ordered_difference_ss(x[0], io.scalar(0x209cacu, 0x46a178u));
        io.at(0x209cb4u);
        flags.scalar_compare(x[1], x[0], false);
        if (!at_or_below()) return;
        flags.scalar_compare(x[0], x[6], false);
        x[1] = at_or_below() ? x[0] : x[6];
    } else {
        x[0] = ordered_add_ss(x[0], io.scalar(0x209cc3u, 0x46a178u));
        io.at(0x209ccbu);
        flags.scalar_compare(x[5], x[0], false);
        if (!at_or_below()) {
            x[1] = x[5];
        } else {
            flags.scalar_compare(x[0], x[1], false);
            if (at_or_below()) x[1] = x[0];
        }
    }
}

inline void prepare_force_packet(MotionState &state, Provider &io) {
    auto &x = state.xmm;
    auto &gp = state.gp;
    const auto &stack = gp[4], &wheel = gp[7];

    // Combine the two force contributions into the original scalar spills
    // and the caller's three-axis packet. Keep the two factor spills separate.
    x[0] = io.scalar(0x209cddu, gp[5] + 0x2cu);
    x[2] = io.scalar(0x209ce2u, gp[3] + 0x10u);
    gp[0] = io.word(0x209ce7u, wheel - 0x18u);
    gp[2] = io.word(0x209ceau, wheel - 0x1cu);
    io.at(0x209cedu);
    x[0] = ordered_add_ss(ordered_mul_ss(x[0], x[1]), x[6]);
    x[1] = ordered_mul_ss(x[0], x[7]);
    io.store(0x209cfcu, stack + 0x3cu, x[1]);
    io.at(0x209d02u);
    x[0] = ordered_mul_ss(x[0], x[2]);
    io.store(0x209d06u, stack + 0x38u, x[0]);
    io.at(0x209d0cu);
    x[1] = x[0] = ordered_add_ss(x[0], x[1]);

    x[1] = ordered_mul_ss(x[1], io.scalar(0x209d13u, gp[1]));
    io.store(0x209d17u, stack + 0xf4u, x[1]);
    x[1] = io.scalar(0x209d20u, gp[1] + 4u);
    io.at(0x209d25u);
    x[1] = ordered_mul_ss(x[1], x[0]);
    io.store(0x209d29u, stack + 0xf8u, x[1]);

    x[1] = io.scalar(0x209d32u, gp[1] + 8u);
    io.store_word(0x209d37u, stack + 0xecu, gp[0]);
    state.set(0, io.word(0x209d3eu, wheel + 0x31u, 1), 1);
    io.at(0x209d41u);
    state.flags.logic(state.reg(0, 1), 1);
    io.store_word(0x209d43u, stack + 0xe8u, gp[2]);
    gp[2] = io.word(0x209d4au, wheel - 0x14u);
    io.at(0x209d4du);
    x[1] = ordered_mul_ss(x[1], x[0]);
    io.store(0x209d51u, stack + 0x30u, x[0]);
    io.store(0x209d57u, stack + 0xfcu, x[1]);
    io.store_word(0x209d60u, stack + 0xf0u, gp[2]);
}

inline std::uint32_t force_route(MotionState &state, Provider &io) {
    auto &gp = state.gp;
    auto &flags = state.flags;
    const auto &stack = gp[4];
    io.at(0x209d67u);
    if (flags.zf) return bypass;
    gp[0] = io.word(0x209d6du, stack + 0x50u);
    gp[0] = io.word(0x209d71u, gp[0] + 0xb4u);
    io.at(0x209d77u);
    flags.minus(gp[0], 0xffffffffu, 4);
    if (flags.zf) {
        io.at(0x209d7au);
        return bypass;
    }

    // Resolve the enabled material response through the original datum
    // array. Leave both exits at their existing instruction boundaries.
    gp[2] = io.word(0x209d80u, 0x52be5cu);
    gp[6] = io.word(0x209d86u, gp[2] + 0x44u);
    io.at(0x209d89u);
    gp[0] = flags.shift(flags.logic(gp[0] & 0xffffu, 4) * 5u, 5, 4);
    gp[2] = io.word(0x209d94u, gp[0] + gp[6] + 4u);
    io.at(0x209d98u);
    gp[0] = flags.plus(gp[0], gp[6], 4);
    gp[2] = flags.right(gp[2], 5, 4, false);
    flags.logic(state.reg(2, 1) & 1u, 1);
    if (flags.zf) {
        io.at(0x209da0u);
        return bypass;
    }
    gp[6] = io.word(0x209da6u, gp[0] + 0x9cu);
    io.at(0x209dacu);
    flags.logic(gp[6], 4);
    if (flags.zf) {
        io.at(0x209daeu);
        return bypass;
    }
    gp[2] = flags.logic(0xffffffffu, 4);
    io.at(0x209db7u);
    gp[0] = stack + 0xe8u;
    return call_site;
}
#endif

inline std::uint32_t bridge(MotionState &state) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_VEHICLE_WHEEL_FORCE_20E4F4");
        return value && *value;
    }();
    if (!reference) return run_stage_with_exit(state, sites, [](MotionState &live, Provider &io) {
        clamp_wheel_force(live, io);
        prepare_force_packet(live, io);
        return force_route(live, io);
    });
#else
    (void)state;
#endif
    return 0;
}
} // namespace halo2::native_vehicle_wheel_force_20e4f4_detail
