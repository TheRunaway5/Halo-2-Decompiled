#pragma once

#include "physics/vehicles/native_tu15_vehicle_wheel_stage.hpp"

namespace halo2::native_vehicle_wheel_offset_20e414_detail {
using namespace native_vehicle_wheel_stage_detail;

inline constexpr std::uint32_t continuation = 0x209b81u;
inline constexpr std::uint32_t sites[]{
    0x209b14u, 0x209b17u, 0x209b1cu, 0x209b20u, 0x209b29u, 0x209b32u, 0x209b3bu,
    0x209b43u, 0x209b45u, 0x209b4cu, 0x209b51u, 0x209b56u, 0x209b5bu, 0x209b60u,
    0x209b65u, 0x209b6au, 0x209b6fu, 0x209b74u, 0x209b79u
};

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
inline void translate_wheel_position(MotionState &state, Provider &io) {
    auto &gp = state.gp;
    auto &xmm = state.xmm;
    gp[3] = io.word(0x209b14u, gp[5] + 0x40u);
    xmm[0] = io.scalar(0x209b17u, gp[7] - 0x1cu);
    gp[0] = io.word(0x209b1cu, gp[4] + 0x24u);
    xmm[3] = io.scalar(0x209b20u, gp[4] + 0xc4u);
    xmm[4] = io.scalar(0x209b29u, gp[4] + 0xc0u);
    xmm[2] = io.scalar(0x209b32u, gp[4] + 0xc8u);
    xmm[5] = io.scalar(0x209b3bu, 0x46a0dcu);
    io.at(0x209b43u);
    gp[3] = state.flags.plus(gp[3], gp[0], 4);
    gp[0] = io.word(0x209b45u, gp[4] + 0x5a0u);

    // Subtract the vehicle origin from the three wheel position words.
    // X is already in XMM0. Keep the subsequent source loads and stores
    // ordered so aliases, observers, and faults see the original state.
    struct Axis { std::uint32_t load, subtract, spill; };
    static constexpr Axis axes[]{
        {0, 0x209b4cu, 0x209b51u},
        {0x209b56u, 0x209b5bu, 0x209b60u},
        {0x209b65u, 0x209b6au, 0x209b6fu}
    };
    for (std::uint32_t axis = 0; axis < std::size(axes); ++axis) {
        const auto &operation = axes[axis];
        const auto offset = 4u * axis;
        if (operation.load) xmm[0] = io.scalar(operation.load, gp[7] - 0x1cu + offset);
        xmm[0] = _mm_sub_ss(xmm[0], io.scalar(operation.subtract, gp[0] + 0x60u + offset));
        io.store(operation.spill, gp[7] - 0x10u + offset, xmm[0]);
    }
    xmm[1] = io.scalar(0x209b74u, gp[7] - 0xcu);
    xmm[6] = io.scalar(0x209b79u, 0x46a0d0u);
}
#endif

inline std::uint32_t bridge(MotionState &state) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_VEHICLE_WHEEL_OFFSET_20E414");
        return value && *value;
    }();
    if (!reference) return run_stage(state, sites, continuation, translate_wheel_position);
#else
    (void)state;
#endif
    return 0;
}
} // namespace halo2::native_vehicle_wheel_offset_20e414_detail
