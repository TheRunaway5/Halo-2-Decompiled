#pragma once

#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include "rendering/effects/native_tu15_contrail_vertex_emission.hpp"

namespace halo2::native_contrail_orientation_setup_detail {

using namespace physics_world_runtime_detail;

enum class Next { retained, dispatch, skip };

inline Next prepare(WorldState &s) {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_CONTRAIL_ORIENTATION_SETUP");
        return value && *value;
    }();
    if (reference || s.image != SourcePhysicsContactImage::TitleUpdate15 || s.services.instruction || s.observe ||
        !native_contrail_vertex_emission_detail::ScopedDrawServices::identifies(s))
        return Next::retained;

    auto &gp = s.gp;
    auto &x = s.xmm;
    const auto load_scalar = [&](std::uint32_t address) { return _mm_set_ss(f(r(s.memory, address, 4))); };
    const auto store_parameter = [&](std::uint32_t pc, std::uint32_t offset, unsigned lane) {
        w(s.memory, s.observe, pc, gp[7] + offset, 4, u(_mm_cvtss_f32(x[lane])));
    };
    x[1] = native_tu15_leaves::ordered_mul_ss(x[1], load_scalar(0x46a0ccu));
    x[0] = load_scalar(gp[4] + 0x60u);
    gp[0] = r(s.memory, gp[4] + 0x2cu, 4);
    store_parameter(0x43092u, 0xffffffe4u, 0);
    x[4] = x[1];
    x[1] = load_scalar(gp[4] + 0x90u);
    store_parameter(0x430a3u, 0xffffffe8u, 1);
    store_parameter(0x430a8u, 0xfffffffcu, 0);
    x[0] = load_scalar(gp[4] + 0xacu);
    store_parameter(0x430b6u, 0u, 0);
    const auto mode = static_cast<std::uint16_t>(r(s.memory, gp[0] + 0x18u, 2));
    gp[0] = static_cast<std::uint32_t>(static_cast<std::int32_t>(std::bit_cast<std::int16_t>(mode)));
    s.flags.minus(gp[0], 4u, 4);
    return s.flags.cf || s.flags.zf ? Next::dispatch : Next::skip;
}

} // namespace halo2::native_contrail_orientation_setup_detail
