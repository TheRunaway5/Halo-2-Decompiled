#pragma once

#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include "rendering/effects/native_tu15_contrail_vertex_emission.hpp"

namespace halo2::native_contrail_adjacent_sample_detail {

using namespace physics_world_runtime_detail;
using native_tu15_leaves::ordered_difference_ss;
using native_tu15_leaves::ordered_mul_ss;
using native_tu15_leaves::ordered_sum_ss;

class AdjacentSample {
    WorldState &s;

    std::uint32_t word(std::uint32_t address) const { return r(s.memory, address, 4); }
    __m128 scalar(std::uint32_t address) const { return _mm_set_ss(f(word(address))); }
    __m128 local(unsigned slot) const { return scalar(s.gp[4] + slot); }
    void save(std::uint32_t pc, unsigned slot, unsigned lane) {
        w(s.memory, s.observe, pc, s.gp[4] + slot, 4, u(_mm_cvtss_f32(s.xmm[lane])));
    }

    void next_state() {
        auto &g = s.gp;
        auto &x = s.xmm;
        auto &flags = s.flags;
        const auto index = std::uint8_t(r(s.memory, g[5] + 3u, 1));
        g[0] = std::uint32_t(std::bit_cast<std::int8_t>(index));
        x[0] = scalar(g[5] + 4u);
        g[0] = flags.increment(g[0], 4, false);
        g[0] = flags.shift(g[0], 6, 4);
        s.set(1, r(s.memory, g[0] + g[2] + 0x3cu, 1), 1);
        x[7] = scalar(g[0] + g[2] + 0x18u);
        g[0] = flags.plus(g[0], g[2], 4);
        flags.logic(s.reg(1, 1) & 0x10u, 1);
        save(0x42ed1u, 0x74u, 0);
        if (!flags.zf) {
            x[0] = scalar(g[5] + 0xcu);
            x[0] = ordered_mul_ss(x[0], x[7]);
            x[7] = x[0];
        }
    }

    void difference(unsigned lane) {
        const auto offset = lane * 4u;
        auto &value = s.xmm[lane];
        value = scalar(s.gp[0] + 0x2cu + offset);
        value = ordered_difference_ss(value, scalar(s.gp[0] + 0x1cu + offset));
    }

    void sample_next_state() {
        auto &x = s.xmm;
        difference(0);
        x[7] = ordered_difference_ss(x[7], local(0x4cu));
        for (unsigned lane = 1; lane < 4; ++lane)
            difference(lane);
        x[7] = ordered_mul_ss(x[7], local(0x74u));
        x[7] = ordered_sum_ss(x[7], local(0x4cu));
        for (unsigned lane = 0; lane < 4; ++lane) {
            x[lane] = ordered_mul_ss(x[lane], x[6]);
            x[lane] = ordered_sum_ss(x[lane], scalar(s.gp[0] + 0x1cu + lane * 4u));
        }
    }

    void blend_states() {
        auto &x = s.xmm;
        x[6] = local(0x74u);
        save(0x42f49u, 0x8cu, 7);
        x[7] = local(0x30u);
        x[0] = ordered_difference_ss(x[0], x[7]);
        x[0] = ordered_mul_ss(x[0], x[6]);
        x[0] = ordered_sum_ss(x[0], x[7]);
        x[7] = scalar(0x46a0d0u);
        x[1] = ordered_difference_ss(x[1], x[4]);
        x[2] = ordered_difference_ss(x[2], x[5]);
        save(0x42f74u, 0x30u, 0);
        x[0] = local(0x3cu);
        x[1] = ordered_mul_ss(x[1], x[6]);
        x[3] = ordered_difference_ss(x[3], x[0]);
        x[1] = ordered_sum_ss(x[1], x[4]);
        x[2] = ordered_mul_ss(x[2], x[6]);
        x[4] = x[1];
        x[1] = local(0x8cu);
        x[2] = ordered_sum_ss(x[2], x[5]);
        x[3] = ordered_mul_ss(x[3], x[6]);
        x[5] = x[2];
        x[3] = ordered_sum_ss(x[3], x[0]);
        save(0x42fabu, 0x34u, 4);
        save(0x42fb1u, 0x38u, 5);
        save(0x42fb7u, 0x3cu, 3);
    }

public:
    explicit AdjacentSample(WorldState &state) : s(state) {}

    void run() {
        if (s.flags.logic(r(s.memory, s.gp[5] + 2u, 1) & 2u, 1)) {
            next_state();
            sample_next_state();
            blend_states();
        }
        s.gp[2] = word(s.gp[4] + 0xc8u);
    }
};

inline bool bridge(WorldState &s) {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_CONTRAIL_ADJACENT_SAMPLE");
        return value && *value;
    }();
    if (reference || s.image != SourcePhysicsContactImage::TitleUpdate15 || s.services.instruction || s.observe ||
            !native_contrail_vertex_emission_detail::ScopedDrawServices::identifies(s)) return false;
    AdjacentSample(s).run();
    return true;
}

} // namespace halo2::native_contrail_adjacent_sample_detail
