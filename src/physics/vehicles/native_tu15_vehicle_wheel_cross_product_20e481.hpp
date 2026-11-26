#pragma once

#include "physics/runtime/native_tu15_world_leaves.hpp"
#include "physics/vehicles/native_tu15_vehicle_wheel_stage.hpp"

namespace halo2::native_vehicle_wheel_cross_product_20e481_detail {
using namespace native_vehicle_wheel_stage_detail;

inline constexpr std::uint32_t continuation = 0x209bf4u;
inline constexpr std::uint32_t sites[]{
    0x209b81u, 0x209b84u, 0x209b89u, 0x209b8eu, 0x209b92u, 0x209b97u, 0x209b9bu,
    0x209b9eu, 0x209ba3u, 0x209ba8u, 0x209bacu, 0x209bb0u, 0x209bb9u, 0x209bbeu,
    0x209bc2u, 0x209bc7u, 0x209bcbu, 0x209bd0u, 0x209bd9u, 0x209bddu, 0x209be1u,
    0x209beau, 0x209befu
};

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
inline void apply_cross_product(MotionState &state, Provider &io) {
    using native_tu15_leaves::ordered_mul_ss;
    using native_world_leaves_detail::ordered_add_ss;
    auto &x = state.xmm;
    const auto &wheel = state.gp[7], &stack = state.gp[4];

    // Form the three scalar cross-product components, keeping the original
    // live XMM register values at each access. Repeated wheel loads remain
    // separate provider operations.
    x[0] = x[3];
    x[0] = ordered_mul_ss(x[0], io.scalar(0x209b84u, wheel - 0x10u));
    x[3] = ordered_mul_ss(x[3], io.scalar(0x209b89u, wheel - 8u));
    io.at(0x209b8eu);
    x[1] = ordered_mul_ss(x[1], x[4]);
    x[4] = ordered_mul_ss(x[4], io.scalar(0x209b92u, wheel - 8u));
    io.at(0x209b97u);
    x[1] = _mm_sub_ss(x[1], x[0]);
    x[0] = x[2];
    x[0] = ordered_mul_ss(x[0], io.scalar(0x209b9eu, wheel - 0x10u));
    x[2] = ordered_mul_ss(x[2], io.scalar(0x209ba3u, wheel - 0xcu));
    io.at(0x209ba8u);
    x[0] = _mm_sub_ss(x[0], x[4]);
    io.store(0x209bacu, wheel, x[0]);
    x[0] = io.scalar(0x209bb0u, stack + 0xb4u);
    io.store(0x209bb9u, wheel + 4u, x[1]);
    io.at(0x209bbeu);
    x[3] = _mm_sub_ss(x[3], x[2]);
    io.store(0x209bc2u, wheel - 4u, x[3]);

    // The first axis already has its frame component in XMM0. Its original
    // intermediate store and the other axes' original rereads are observable
    // when providers alias wheel storage with the caller's frame.
    io.at(0x209bc7u);
    x[0] = ordered_add_ss(x[0], x[3]);
    io.store(0x209bcbu, wheel - 4u, x[0]);
    struct Axis { std::uint32_t frame, point, spill; };
    static constexpr Axis axes[]{
        {0x209bd0u, 0x209bd9u, 0x209bddu},
        {0x209be1u, 0x209beau, 0x209befu},
    };
    for (std::uint32_t axis = 0; axis < std::size(axes); ++axis) {
        const auto &operation = axes[axis];
        const auto offset = 4u * axis;
        x[0] = io.scalar(operation.frame, stack + 0xb8u + offset);
        x[0] = ordered_add_ss(x[0], io.scalar(operation.point, wheel + offset));
        io.store(operation.spill, wheel + offset, x[0]);
    }
}
#endif

inline std::uint32_t bridge(MotionState &state) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_VEHICLE_WHEEL_CROSS_PRODUCT_20E481");
        return value && *value;
    }();
    if (!reference) return run_stage(state, sites, continuation, apply_cross_product);
#else
    (void)state;
#endif
    return 0;
}
} // namespace halo2::native_vehicle_wheel_cross_product_20e481_detail
