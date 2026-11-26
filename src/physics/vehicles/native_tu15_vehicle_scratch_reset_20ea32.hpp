#pragma once

#include "physics/vehicles/native_tu15_vehicle_wheel_stage.hpp"

namespace halo2::native_vehicle_scratch_reset_20ea32_detail {
using namespace native_vehicle_wheel_stage_detail;

inline constexpr std::uint32_t prepare_child = 0x20a164u;
inline constexpr std::uint32_t bypass_child = 0x20a17du;
inline constexpr std::uint32_t sites[]{
    0x20a132u, 0x20a139u, 0x20a13du, 0x20a141u, 0x20a146u, 0x20a14du, 0x20a152u,
    0x20a157u, 0x20a15au, 0x20a15bu, 0x20a15du, 0x20a160u, 0x20a162u,
};

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
inline std::uint32_t reset_scratch(MotionState &state, Provider &io) {
    auto &gp = state.gp;
    auto &flags = state.flags;
    const auto &stack = gp[4];

    io.store_word(0x20a132u, stack + 0x90u, gp[3]);
    io.store_word(0x20a139u, stack + 0x34u, gp[3]);
    io.store_word(0x20a13du, stack + 0x2cu, gp[3]);
    io.store_word(0x20a141u, stack + 0x1bu, 0, 1);
    io.at(0x20a146u);
    gp[0] = stack + 0x1e4u;
    io.at(0x20a14du);
    gp[1] = 16;

    // Initialize only the marker word in each scratch record. The pointer
    // and counter must be visible at each store and at any resumed branch.
    do {
        io.store_word(0x20a152u, gp[0], 0xffffu, 2);
        io.at(0x20a157u);
        gp[0] = flags.plus(gp[0], 0x3cu, 4);
        io.at(0x20a15au);
        gp[1] = flags.increment(gp[1], 4, true);
        io.at(0x20a15bu);
    } while (!flags.zf);

    gp[0] = io.word(0x20a15du, gp[5] + 0x44u);
    io.at(0x20a160u);
    flags.minus(gp[0], gp[3], 4);
    io.at(0x20a162u);
    return flags.zf || (flags.sf != flags.of) ? bypass_child : prepare_child;
}
#endif

inline std::uint32_t bridge(MotionState &state) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_VEHICLE_SCRATCH_RESET_20EA32");
        return value && std::string_view{value} != "0";
    }();
    if (!reference) return run_stage_with_exit(state, sites, reset_scratch);
#else
    (void)state;
#endif
    return 0;
}
} // namespace halo2::native_vehicle_scratch_reset_20ea32_detail
