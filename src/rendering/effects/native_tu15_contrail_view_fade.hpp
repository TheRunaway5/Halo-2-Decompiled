#pragma once

#include "rendering/effects/native_tu15_contrail_vertex_emission.hpp"

namespace halo2::native_contrail_view_fade_detail {

using namespace physics_world_runtime_detail;

enum class Continuation { retained, finished, curve };

inline bool admitted(const WorldState &state) {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_CONTRAIL_VIEW_FADE");
        return value && *value;
    }();
    return !reference && native_contrail_vertex_emission_detail::ScopedDrawServices::identifies(state) &&
        !state.services.instruction && !state.observe;
}

class ViewFade {
    WorldState &state;

    float single(std::uint32_t address) const { return f(r(state.memory, address, 4)); }

    void load_view_delta() {
        auto &fp = state.fp;
        for (unsigned axis = 0; axis < 3; ++axis) {
            const auto offset = axis * 4u;
            fp.push(ext(single(0x4c65c0u + offset)));
            fp.st(0) = sub(fp.st(0), ext(single(state.gp[1] + offset)));
        }
    }

    void project_view_delta() {
        auto &fp = state.fp;
        fp.push(fp.st(0));
        fp.st(0) = mul(fp.st(0), ext(single(state.gp[0] + 8u)));
        fp.push(fp.st(2));
        fp.st(0) = mul(fp.st(0), ext(single(state.gp[0] + 4u)));
        fp.st(1) = add(fp.st(1), fp.st(0));
        fp.pop();
        fp.push(fp.st(3));
        fp.st(0) = mul(fp.st(0), ext(single(state.gp[0])));
    }

    void normalize_projection() {
        auto &fp = state.fp;
        const auto projection = add(fp.st(1), fp.st(0));
        const auto z_squared = mul(fp.st(2), fp.st(2));
        const auto y_squared = mul(fp.st(3), fp.st(3));
        const auto zy_squared = add(z_squared, y_squared);
        const auto x_squared = mul(fp.st(4), fp.st(4));
        const auto magnitude = root(add(zy_squared, x_squared));
        fp.st(4) = std::fabs(divide(projection, magnitude));

        // The expression leaves one value above the incoming stack. The
        // retained temporary FLD instructions clear C1 before the spill.
        for (unsigned lane = 0; lane < 4; ++lane)
            fp.pop();
        fp.status &= ~0x200u;
    }

    void restore_stack() {
        state.gp[1] = state.pop();
        state.pop();
        state.gp[4] += 4u;
    }

    void finish() {
        state.flags.minus(state.reg(6, 2, 0), 2, 2);
        if (state.flags.zf) {
            state.xmm[1] = _mm_sub_ss(state.xmm[1], state.xmm[0]);
            state.xmm[0] = state.xmm[1];
        }
        restore_stack();
    }

public:
    explicit ViewFade(WorldState &state) : state(state) {}

    template<class Spill>
    Continuation run(Spill spill) {
        if (!admitted(state))
            return Continuation::retained;

        const auto mode = state.reg(6, 2, 0);
        if (mode) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
            if (state.active_mmx || state.fp.count > 2u)
                return Continuation::retained;
            std::uint16_t control;
            asm volatile("fnstcw %0" : "=m"(control));
            if ((control & 0x3fu) != 0x3fu)
                return Continuation::retained;
#else
            return Continuation::retained;
#endif
        }

        state.push(0x42c40u, state.gp[1]);
        state.flags.logic(mode, 2);
        state.xmm[1] = _mm_set_ss(single(0x46a0d0u));
        state.xmm[0] = state.xmm[1];
        if (!mode) {
            restore_stack();
            return Continuation::finished;
        }

        load_view_delta();
        project_view_delta();
        state.gp[0] = r(state.memory, state.gp[4] + 8u, 4);
        state.flags.logic(r(state.memory, state.gp[0], 1) & 0x40u, 1);
        normalize_projection();

        auto &fp = state.fp;
        const auto saved_projection = spill(fp.st(0), fp.status);
        w(state.memory, state.observe, 0x42ca4u, state.gp[4], 4, saved_projection);
        fp.pop();
        state.xmm[0] = _mm_set_ss(single(state.gp[4]));
        if (!state.flags.zf) {
            state.gp[0] = 2;
            return Continuation::curve;
        }
        finish();
        return Continuation::finished;
    }

    bool after_curve() {
        if (!admitted(state))
            return false;
        state.xmm[1] = _mm_set_ss(single(0x46a0d0u));
        finish();
        return true;
    }
};

} // namespace halo2::native_contrail_view_fade_detail
