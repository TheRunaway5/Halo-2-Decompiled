#pragma once

#include "physics/runtime/native_tu15_world_leaves.hpp"
#include "physics/vehicles/native_tu15_vehicle_wheel_stage.hpp"

namespace halo2::native_vehicle_wheel_curve_20e84a_detail {
using namespace native_vehicle_wheel_stage_detail;

inline constexpr std::uint32_t call_site = 0x209f8du;
inline constexpr std::uint32_t output = 0x20a01fu;
inline constexpr std::uint32_t pre_sites[]{
    0x209f4au, 0x209f4eu, 0x209f4fu, 0x209f50u, 0x209f53u, 0x209f57u,
    0x209f5bu, 0x209f60u, 0x209f66u, 0x209f6cu, 0x209f6fu, 0x209f72u,
    0x209f78u, 0x209f7cu, 0x209f82u, 0x209f84u, 0x209f88u, 0x209f8cu,
};
inline constexpr std::uint32_t post_sites[]{
    0x209f92u, 0x209f9au, 0x209fa2u, 0x209fa7u, 0x209fafu, 0x209fb8u,
    0x209fc1u, 0x209fcau, 0x209fceu, 0x209fd4u, 0x209fdau, 0x209fe0u,
    0x209fe6u, 0x209feau, 0x209fefu, 0x209ff4u, 0x209ff8u, 0x209ffdu,
    0x20a002u, 0x20a006u, 0x20a00bu, 0x20a010u, 0x20a014u, 0x20a019u,
};
inline constexpr std::size_t accumulation_offset = 12;
inline constexpr std::size_t reload_offset = 23;

inline void store_curve_argument_reference(MotionState &state) {
    spill_x87_float_and_pop(state, [&](std::uint32_t result) {
        w(state.memory, state.observe, 0x209f84u, state.gp[4] + 0x14u, 4, result);
    });
}

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
using native_tu15_leaves::ordered_difference_ss;
using native_tu15_leaves::ordered_mul_ss;
using native_world_leaves_detail::ordered_add_ss;

inline void prepare_curve_argument(MotionState &state, Provider &io) {
    auto &gp = state.gp;
    auto &fp = state.fp;
    const auto &stack = gp[4];

    gp[0] = io.word(0x209f4au, stack + 0x1cu);
    io.at(0x209f4eu);
    gp[0] = state.flags.increment(gp[0], 4, false);
    io.at(0x209f4fu);
    gp[2] = si(gp[0]) < 0 ? 0xffffffffu : 0u;
    state.quotient(io.word(0x209f50u, gp[5] + 0x3cu), true);
    io.store_word(0x209f53u, stack + 0x14u, gp[0]);

    fp.push(static_cast<long double>(si(io.word(0x209f57u, stack + 0x14u))));
    io.at(0x209f5bu);
    gp[0] = 8;
    fp.st(0) = mul(fp.st(0), ext(f(io.word(0x209f60u, 0x46ab30u))));
    fp.st(0) = add(fp.st(0), ext(f(io.word(0x209f66u, 0x46a194u))));
    fp.st(0) = mul(fp.st(0), static_cast<long double>(si(io.word(0x209f6cu, gp[1] + 8u))));
    fp.st(0) = mul(fp.st(0), ext(f(io.word(0x209f6fu, gp[1] + 4u))));
    fp.st(0) = mul(fp.st(0), ext(f(io.word(0x209f72u, 0x456918u))));
    fp.push(static_cast<long double>(si(io.word(0x209f78u, stack + 0x1cu))));
    fp.st(0) = mul(fp.st(0), ext(f(io.word(0x209f7cu, 0x46a908u))));
    io.at(0x209f82u);
    fp.st(1) = add(fp.st(1), fp.st(0));
    fp.pop();

    // Publish the store before rounding and the push before changing ESP.
    // Faults preserve the pending x87 value or the decremented stack pointer.
    io.at(0x209f84u);
    spill_x87_float_and_pop(state, [&](std::uint32_t result) {
        io.store_word(0x209f84u, stack + 0x14u, result);
    });
    gp[2] = io.word(0x209f88u, stack + 0x14u);
    io.at(0x209f8cu);
    gp[4] -= 4;
    io.store_word(0x209f8cu, gp[4], gp[2]);
}

inline void adjust_curve_response(MotionState &state, Provider &io) {
    auto &x = state.xmm;
    const auto &stack = state.gp[4], &wheel = state.gp[7];
    x[0] = ordered_mul_ss(x[0], io.scalar(0x209f92u, 0x46a234u));
    x[0] = ordered_add_ss(x[0], io.scalar(0x209f9au, 0x46a330u));
    x[0] = ordered_mul_ss(x[0], io.scalar(0x209fa2u, wheel + 0x50u));
    x[1] = io.scalar(0x209fa7u, 0x46a0d0u);
    x[2] = io.scalar(0x209fafu, stack + 0x104u);
    x[3] = io.scalar(0x209fb8u, stack + 0x108u);
    x[4] = io.scalar(0x209fc1u, stack + 0x10cu);
    io.at(0x209fcau);
    x[1] = ordered_difference_ss(x[1], x[0]);
    x[1] = ordered_mul_ss(x[1], io.scalar(0x209fceu, stack + 0x34u));
    io.store(0x209fd4u, stack + 0x34u, x[1]);
    x[1] = io.scalar(0x209fdau, stack + 0x2cu);
    x[0] = io.scalar(0x209fe0u, stack + 0x34u);
}

inline void accumulate_response(MotionState &state, Provider &io) {
    auto &x = state.xmm;
    const auto &wheel = state.gp[7];

    struct ForceComponent {
        std::uint32_t scale, add, store;
    };
    constexpr ForceComponent components[]{
        {0x209fe6u, 0x209feau, 0x209fefu},
        {0x209ff4u, 0x209ff8u, 0x209ffdu},
        {0x20a002u, 0x20a006u, 0x20a00bu},
    };
    for (unsigned axis = 0; axis < std::size(components); ++axis) {
        const auto [scale, add, store] = components[axis];
        auto &value = x[2u + axis];
        const auto address = wheel + 0x40u + axis * 4u;
        io.at(scale);
        value = ordered_mul_ss(value, x[0]);
        value = ordered_add_ss(value, io.scalar(add, address));
        io.store(store, address, value);
    }
    io.store_word(0x20a010u, wheel + 0x30u, 1, 1);
    io.store(0x20a014u, wheel + 0x54u, x[1]);
}
#endif

inline bool use_reference() {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_VEHICLE_WHEEL_CURVE_20E84A");
        return value && std::string_view{value} != "0";
    }();
    return reference;
}

inline std::uint32_t before_curve(MotionState &state) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if (!use_reference()) return run_stage(state, pre_sites, call_site, prepare_curve_argument);
#else
    (void)state;
#endif
    return 0;
}

template<std::size_t Offset = 0>
std::uint32_t after_curve(MotionState &state) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if (!use_reference()) {
        return run_stage(state, std::span{post_sites}.subspan<Offset>(), output, [](MotionState &live, Provider &io) {
            if constexpr (Offset == 0) adjust_curve_response(live, io);
            if constexpr (Offset < reload_offset) accumulate_response(live, io);
            live.gp[1] = io.word(0x20a019u, 0x474d1cu);
        });
    }
#else
    (void)state;
#endif
    return 0;
}
} // namespace halo2::native_vehicle_wheel_curve_20e84a_detail
