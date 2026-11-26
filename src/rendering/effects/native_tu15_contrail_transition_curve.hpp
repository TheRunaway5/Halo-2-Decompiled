#pragma once

#include "rendering/effects/native_tu15_contrail_vertex_emission.hpp"

namespace halo2::native_contrail_transition_curve_detail {

using namespace physics_world_runtime_detail;

enum class Continuation { retained, finished, remainder };

inline bool admitted(const WorldState &state) {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_CONTRAIL_TRANSITION_CURVE");
        return value && *value;
    }();
    return !reference && native_contrail_vertex_emission_detail::ScopedDrawServices::identifies(state) &&
        !state.services.instruction && !state.observe;
}

class TransitionCurve {
    WorldState &state;

    std::uint32_t read(std::uint32_t address, unsigned width = 4) const {
        return r(state.memory, address, width);
    }

    __m128 scalar(std::uint32_t address) const { return _mm_set_ss(f(read(address))); }

    static std::uint32_t extend_halfword(std::uint32_t value) {
        return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));
    }

    void store(std::uint32_t pc, unsigned offset, std::uint32_t value) {
        w(state.memory, state.observe, pc, state.gp[4] + offset, 4, value);
    }

    void clamp_input() {
        auto &xmm = state.xmm;
        auto &flags = state.flags;
        flags.scalar_compare(xmm[1], xmm[0], false);
        if (flags.cf || flags.zf) {
            xmm[1] = scalar(0x46a0d0u);
            flags.scalar_compare(xmm[0], xmm[1], false);
        }
        if (!flags.cf && !flags.zf)
            xmm[0] = xmm[1];
    }

    void interpolate() {
        auto &gp = state.gp;
        auto &xmm = state.xmm;
        xmm[1] = scalar(0x46a0c4u);
        xmm[2] = scalar(gp[4] + 0xcu);
        gp[1] = extend_halfword(gp[0]);
        gp[2] = read(gp[1] + gp[6], 1);
        gp[0] = gp[1] + gp[6];
        gp[0] = read(gp[0] + 1u, 1);

        xmm[0] = _mm_mul_ss(_mm_cvtsi32_ss(xmm[0], si(gp[2])), xmm[1]);
        xmm[4] = _mm_sub_ss(xmm[3], xmm[2]);
        xmm[0] = _mm_mul_ss(xmm[0], xmm[4]);
        xmm[4] = _mm_mul_ss(_mm_cvtsi32_ss(xmm[4], si(gp[0])), xmm[1]);
        xmm[4] = _mm_mul_ss(xmm[4], xmm[2]);
        xmm[0] = _mm_add_ss(xmm[0], xmm[4]);
    }

    void return_to_caller() {
        state.gp[4] = state.flags.plus(state.gp[4], 0xcu, 4);
        state.pop();
    }

public:
    explicit TransitionCurve(WorldState &state) : state(state) {}

    template<class ReadWide>
    Continuation begin(ReadWide read_wide) {
        if (!admitted(state))
            return Continuation::retained;
        const auto type = state.reg(0, 2, 0);
        if (type && (state.active_mmx || state.fp.count > 6u))
            return Continuation::retained;

        auto &gp = state.gp;
        auto &xmm = state.xmm;
        auto &flags = state.flags;
        xmm[1] = _mm_setzero_ps();
        gp[4] = flags.minus(gp[4], 0xcu, 4);
        clamp_input();
        flags.logic(type, 2);
        if (flags.zf) {
            return_to_caller();
            return Continuation::finished;
        }

        xmm[0] = _mm_mul_ss(xmm[0], scalar(0x46a43cu));
        state.push(0x17f8ccu, gp[6]);
        gp[6] = extend_halfword(type);
        store(0x17f8d0u, 4, u(_mm_cvtss_f32(xmm[0])));
        gp[6] = flags.shift(gp[6], 10, 4);
        state.fp.push(ext(f(read(gp[4] + 4u))));
        gp[6] = flags.plus(gp[6], 0x4782a8u, 4);
        state.fp.push(static_cast<long double>(std::bit_cast<double>(read_wide(state.memory, 0x46a1d0u))));
        return Continuation::remainder;
    }

    template<class Spill>
    bool after_remainder(Spill spill) {
        if (!admitted(state))
            return false;
        auto &gp = state.gp;
        auto &xmm = state.xmm;
        auto &fp = state.fp;
        auto &flags = state.flags;

        xmm[0] = scalar(gp[4] + 4u);
        xmm[0] = _mm_sub_ss(xmm[0], scalar(0x46a178u));
        store(0x17f8fcu, 0xc, spill(fp.st(0), fp.status));
        fp.pop();
        store(0x17f900u, 4, u(_mm_cvtss_f32(xmm[0])));
        fp.push(ext(f(read(gp[4] + 4u))));
        store(0x17f90au, 8, integer_spill(fp.st(0), fp.status));
        fp.pop();
        gp[0] = read(gp[4] + 8u);
        flags.minus(state.reg(0, 2, 0), 0x3ffu, 2);
        xmm[3] = scalar(0x46a0d0u);
        if (flags.zf) {
            gp[0] = read(gp[6] + 0x3ffu, 1);
            xmm[0] = _mm_cvtsi32_ss(xmm[0], si(gp[0]));
            xmm[0] = _mm_mul_ss(xmm[0], scalar(0x46a0c4u));
        } else {
            interpolate();
        }

        xmm[1] = _mm_setzero_ps();
        flags.scalar_compare(xmm[1], xmm[0], false);
        gp[6] = state.pop();
        if (!flags.cf && !flags.zf) {
            xmm[0] = xmm[1];
        } else {
            flags.scalar_compare(xmm[0], xmm[3], false);
            if (!flags.cf && !flags.zf)
                xmm[0] = xmm[3];
        }
        return_to_caller();
        return true;
    }
};

} // namespace halo2::native_contrail_transition_curve_detail
