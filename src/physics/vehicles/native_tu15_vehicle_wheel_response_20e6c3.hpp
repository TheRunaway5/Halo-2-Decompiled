#pragma once

#include "physics/runtime/native_tu15_world_leaves.hpp"
#include "physics/vehicles/native_tu15_vehicle_wheel_stage.hpp"

namespace halo2::native_vehicle_wheel_response_20e6c3_detail {
using namespace native_vehicle_wheel_stage_detail;

inline constexpr std::uint32_t x87_detour = 0x209f4au;
inline constexpr std::uint32_t accumulate = 0x209fe6u;
inline constexpr std::uint32_t rejected = 0x20a019u;
inline constexpr std::uint32_t sites[]{
    0x209dc3u, 0x209dc5u, 0x209dcbu, 0x209dd1u, 0x209ddau, 0x209de0u, 0x209de5u,
    0x209de8u, 0x209dedu, 0x209df3u, 0x209df8u, 0x209dfbu, 0x209dffu, 0x209e02u,
    0x209e04u, 0x209e07u, 0x209e09u, 0x209e0cu, 0x209e10u, 0x209e18u, 0x209e1bu,
    0x209e1du, 0x209e20u, 0x209e24u, 0x209e27u, 0x209e29u, 0x209e2cu, 0x209e2eu,
    0x209e31u, 0x209e35u, 0x209e38u, 0x209e3au, 0x209e3du, 0x209e3fu, 0x209e42u,
    0x209e44u, 0x209e47u, 0x209e49u, 0x209e4cu, 0x209e4eu, 0x209e52u, 0x209e56u,
    0x209e59u, 0x209e5bu, 0x209e5eu, 0x209e60u, 0x209e63u, 0x209e65u, 0x209e68u,
    0x209e6au, 0x209e6eu, 0x209e72u, 0x209e75u, 0x209e77u, 0x209e7au, 0x209e7du,
    0x209e7fu, 0x209e87u, 0x209e8du, 0x209e90u, 0x209e92u, 0x209e98u, 0x209ea0u,
    0x209ea4u, 0x209eaau, 0x209eacu, 0x209eb4u, 0x209ebau, 0x209ebdu, 0x209ec6u,
    0x209ecbu, 0x209ed4u, 0x209ed9u, 0x209eddu, 0x209ee4u, 0x209ee8u, 0x209eebu,
    0x209eefu, 0x209ef3u, 0x209efcu, 0x209f00u, 0x209f04u, 0x209f07u, 0x209f0cu,
    0x209f10u, 0x209f15u, 0x209f19u, 0x209f1eu, 0x209f23u, 0x209f28u, 0x209f2eu,
    0x209f32u, 0x209f37u, 0x209f3eu, 0x209f44u,
};

inline __m128 ordered_divide_ss(__m128 left, __m128 right) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("divss %1,%0" : "+x"(left) : "x"(right) : "memory");
    return left;
#else
    return _mm_div_ss(left, right);
#endif
}

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
using native_tu15_leaves::ordered_difference_ss;
using native_tu15_leaves::ordered_mul_ss;
using native_world_leaves_detail::ordered_add_ss;

inline void contact_fraction(MotionState &state, Provider &io) {
    auto &x = state.xmm;
    auto &flags = state.flags;
    const auto direct_fraction = [&] {
        flags.scalar_compare(x[0], x[2], false);
        x[5] = x[6];
        if (flags.cf) x[5] = io.scalar(0x209e7fu, 0x46a0d0u);
    };

    // The original permits interpolation only inside both authored endpoint
    // tolerances. Use its ordered compares so unordered inputs choose the same
    // side, and leave the last comparison in the carried EFLAGS.
    x[7] = io.scalar(0x209e10u, 0x46a180u);
    io.at(0x209e18u);
    flags.scalar_compare(x[7], x[3], false);
    if (!flags.cf && !flags.zf) {
        direct_fraction();
        return;
    }
    x[3] = ordered_difference_ss(x[0], x[4]);
    flags.scalar_compare(x[3], x[6], false);
    x[5] = flags.cf ? ordered_difference_ss(x[6], x[3]) : x[3];
    flags.scalar_compare(x[7], x[5], false);
    if (!flags.cf && !flags.zf) {
        direct_fraction();
        return;
    }

    flags.scalar_compare(x[4], x[0], false);
    if (!flags.cf && !flags.zf) {
        flags.scalar_compare(x[0], x[2], false);
        if (!flags.cf) {
            x[5] = x[6];
            return;
        }
        flags.scalar_compare(x[2], x[4], false);
        if (!flags.cf) {
            x[5] = io.scalar(0x209e7fu, 0x46a0d0u);
            return;
        }
        x[2] = ordered_divide_ss(ordered_difference_ss(x[2], x[0]), x[1]);
        x[5] = x[2];
    } else {
        flags.scalar_compare(x[4], x[2], false);
        if (!flags.cf) {
            x[5] = io.scalar(0x209e7fu, 0x46a0d0u);
            return;
        }
        flags.scalar_compare(x[2], x[0], false);
        if (!flags.cf) {
            x[5] = x[6];
            return;
        }
        x[0] = ordered_divide_ss(ordered_difference_ss(x[0], x[2]), x[3]);
        x[5] = x[0];
    }
}

inline void blend_contact(MotionState &state, Provider &io) {
    auto &x = state.xmm;
    auto &flags = state.flags;
    const auto &stack = state.gp[4], &parameters = state.gp[3], &wheel = state.gp[7];
    x[0] = io.scalar(0x209dcbu, stack + 0x30u);
    x[0] = ordered_mul_ss(x[0], io.scalar(0x209dd1u, stack + 0x100u));
    x[0] = ordered_difference_ss(x[0], io.scalar(0x209ddau, stack + 0x3cu));
    x[4] = io.scalar(0x209de0u, parameters + 0x18u);
    io.at(0x209de5u);
    x[6] = _mm_setzero_ps();
    x[2] = io.scalar(0x209de8u, wheel - 0x2cu);
    io.store(0x209dedu, stack + 0x40u, x[0]);

    x[0] = io.scalar(0x209df3u, parameters + 0x1cu);
    io.at(0x209df8u);
    x[1] = ordered_difference_ss(x[4], x[0]);
    flags.scalar_compare(x[1], x[6], false);
    x[3] = flags.cf ? ordered_difference_ss(x[6], x[1]) : x[1];
    contact_fraction(state, io);
    x[0] = io.scalar(0x209e87u, stack + 0x40u);
    io.at(0x209e8du);
    flags.scalar_compare(x[0], x[6], false);
    if (flags.cf || flags.zf) {
        x[0] = io.scalar(0x209eacu, 0x46a0d0u);
        io.store(0x209eb4u, stack + 0x2cu, x[0]);
        io.at(0x209ebau);
        x[1] = x[0];
    } else {
        x[0] = ordered_divide_ss(x[0], io.scalar(0x209e92u, stack + 0x38u));
        x[1] = io.scalar(0x209e98u, 0x46a0d0u);
        io.at(0x209ea0u);
        x[1] = ordered_difference_ss(x[1], x[0]);
        io.store(0x209ea4u, stack + 0x2cu, x[1]);
        io.at(0x209eaau);
    }
}

inline std::uint32_t stage_response(MotionState &state, Provider &io) {
    auto &x = state.xmm;
    auto &gp = state.gp;
    auto &flags = state.flags;
    const auto &stack = gp[4], &parameters = gp[3], &wheel = gp[7];

    io.at(0x209dc3u);
    flags.logic(state.reg(0, 1), 1);
    if (flags.zf) {
        io.at(0x209dc5u);
        return rejected;
    }
    blend_contact(state, io);

    // Project the wheel vector onto the three source axes, then apply
    // the impulse scale and the authored contact factors in retail order.
    x[3] = io.scalar(0x209ebdu, stack + 0x108u);
    x[6] = io.scalar(0x209ec6u, wheel + 4u);
    x[4] = io.scalar(0x209ecbu, stack + 0x10cu);
    gp[0] = io.word(0x209ed4u, 0x52be68u);
    x[0] = io.scalar(0x209ed9u, gp[0]);
    gp[1] = io.word(0x209eddu, stack + 0x5a0u);
    io.at(0x209ee4u);
    x[6] = ordered_mul_ss(x[6], x[4]);
    x[2] = x[3];
    x[2] = ordered_mul_ss(x[2], io.scalar(0x209eebu, wheel));
    io.at(0x209eefu);
    x[6] = ordered_add_ss(x[6], x[2]);
    x[2] = io.scalar(0x209ef3u, stack + 0x104u);
    io.at(0x209efcu);
    x[0] = ordered_mul_ss(ordered_mul_ss(x[0], x[1]), x[1]);
    x[7] = x[2];
    x[7] = ordered_mul_ss(x[7], io.scalar(0x209f07u, wheel - 4u));
    io.at(0x209f0cu);
    x[6] = ordered_add_ss(x[6], x[7]);
    x[6] = ordered_mul_ss(x[6], io.scalar(0x209f10u, parameters + 0x14u));
    io.at(0x209f15u);
    x[0] = ordered_difference_ss(x[0], x[6]);
    x[0] = ordered_mul_ss(x[0], io.scalar(0x209f19u, wheel + 0x4cu));
    x[0] = ordered_mul_ss(x[0], io.scalar(0x209f1eu, parameters + 8u));
    x[0] = ordered_mul_ss(x[0], io.scalar(0x209f23u, gp[1] + 0x6cu));
    gp[1] = io.word(0x209f28u, 0x51df54u);
    io.at(0x209f2eu);
    x[0] = ordered_mul_ss(x[0], x[5]);
    x[5] = io.scalar(0x209f32u, wheel + 0x50u);
    flags.scalar_compare(x[5], io.scalar(0x209f37u, 0x46a0e8u), false);
    io.store(0x209f3eu, stack + 0x34u, x[0]);
    io.at(0x209f44u);
    return (flags.cf || flags.zf) ? accumulate : x87_detour;
}
#endif

inline std::uint32_t bridge(MotionState &state) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_VEHICLE_WHEEL_RESPONSE_20E6C3");
        return value && *value;
    }();
    if (!reference) return run_stage_with_exit(state, sites, stage_response);
#else
    (void)state;
#endif
    return 0;
}
} // namespace halo2::native_vehicle_wheel_response_20e6c3_detail
