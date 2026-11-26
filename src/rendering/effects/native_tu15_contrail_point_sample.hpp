#pragma once

#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include "rendering/effects/native_tu15_contrail_vertex_emission.hpp"

namespace halo2::native_contrail_point_sample_detail {

using namespace physics_world_runtime_detail;

enum class Continuation { retained, adjacent, emit };

inline bool admitted(const WorldState &state) {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_CONTRAIL_POINT_SAMPLE");
        return value && *value;
    }();
    return !reference && native_contrail_vertex_emission_detail::ScopedDrawServices::identifies(state) &&
        !state.services.instruction && !state.observe;
}

class PointSample {
    WorldState &state;

    std::uint32_t word(std::uint32_t address) const { return r(state.memory, address, 4); }
    __m128 scalar(std::uint32_t address) const { return _mm_set_ss(f(word(address))); }
    static std::uint32_t extend_byte(std::uint32_t value) {
        return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(value))));
    }

    void store(std::uint32_t pc, unsigned slot, std::uint32_t value) {
        w(state.memory, state.observe, pc, state.gp[4] + slot, 4, value);
    }

    bool find_sample() {
        auto &gp = state.gp;
        auto &flags = state.flags;
        gp[1] = word(0x4f7210u);
        gp[0] = word(gp[1] + 0x44u);
        gp[5] = flags.plus((gp[5] & 0xffffu) * 0x38u, gp[0], 4);
        const auto index = r(state.memory, gp[5] + 3u, 1);
        state.set(0, index, 1, 0);
        flags.logic(index, 1);
        if (flags.sf != flags.of) {
            gp[1] = flags.logic(0, 4);
        } else {
            gp[2] = word(gp[4] + 0x2cu);
            gp[2] = word(gp[2] + 0xe8u);
            gp[1] = extend_byte(index);
            gp[2] = flags.increment(gp[2], 4, true);
            flags.minus(gp[1], gp[2], 4);
            if (!flags.zf && flags.sf == flags.of)
                gp[1] = gp[2];
            gp[2] = word(gp[4] + 0xc8u);
        }
        state.xmm[7] = scalar(0x46a0d0u);
        gp[0] = extend_byte(index);
        flags.minus(gp[1], gp[0], 4);
        return flags.zf;
    }

    void use_empty_sample() {
        auto &gp = state.gp;
        auto &xmm = state.xmm;
        xmm[1] = _mm_setzero_ps();
        gp[0] = state.flags.logic(0, 4);
        store(0x42ddeu, 0x30, gp[0]);
        store(0x42de2u, 0x34, gp[0]);
        xmm[4] = scalar(gp[4] + 0x34u);
        store(0x42decu, 0x38, gp[0]);
        xmm[5] = scalar(gp[4] + 0x38u);
        store(0x42df6u, 0x3c, gp[0]);
        xmm[3] = scalar(gp[4] + 0x3cu);
    }

    void use_authored_sample() {
        using namespace native_tu15_leaves;
        auto &gp = state.gp;
        auto &flags = state.flags;
        auto &xmm = state.xmm;
        gp[1] = word(gp[4] + 0x2cu);
        gp[2] = word(gp[1] + 0xecu);
        gp[0] = flags.shift(gp[0], 6, 4);
        gp[1] = word(gp[0] + gp[2] + 0x3cu);
        gp[0] = flags.plus(gp[0], gp[2], 4);
        flags.logic(gp[1] & 0x20u, 1);
        xmm[6] = xmm[7];
        if (!flags.zf)
            xmm[6] = scalar(gp[5] + 0xcu);

        flags.logic(gp[1] & 0x10u, 1);
        xmm[1] = scalar(gp[0] + 0x18u);
        store(0x42e2du, 0x4c, u(_mm_cvtss_f32(xmm[1])));
        if (!flags.zf) {
            xmm[0] = ordered_mul_ss(scalar(gp[5] + 0xcu), xmm[1]);
            store(0x42e3eu, 0x4c, u(_mm_cvtss_f32(xmm[0])));
            xmm[1] = xmm[0];
        }

        constexpr std::array lanes{0u, 4u, 5u, 3u};
        constexpr std::array stores{0x42e5au, 0x42e73u, 0x42e8cu, 0x42ea5u};
        for (unsigned component = 0; component < lanes.size(); ++component) {
            auto &output = xmm[lanes[component]];
            const auto offset = component * 4u;
            output = scalar(gp[0] + 0x2cu + offset);
            output = ordered_difference_ss(output, scalar(gp[0] + 0x1cu + offset));
            output = ordered_mul_ss(output, xmm[6]);
            // The authored loop reloads the start component just before ADDSS.
            output = ordered_sum_ss(output, scalar(gp[0] + 0x1cu + offset));
            store(stores[component], 0x30u + offset, u(_mm_cvtss_f32(output)));
        }
    }

public:
    explicit PointSample(WorldState &state) : state(state) {}

    bool reload_output() {
        if (!admitted(state))
            return false;
        state.gp[7] = word(state.gp[4] + 0x50u);
        return true;
    }

    Continuation prepare() {
        if (!admitted(state))
            return Continuation::retained;
        if (!find_sample()) {
            use_empty_sample();
            return Continuation::emit;
        }
        use_authored_sample();
        return Continuation::adjacent;
    }
};

} // namespace halo2::native_contrail_point_sample_detail
