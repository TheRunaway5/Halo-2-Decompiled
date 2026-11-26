#pragma once

#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include "rendering/effects/native_tu15_contrail_vertex_emission.hpp"

namespace halo2::native_contrail_planar_facing_detail {

using namespace physics_world_runtime_detail;
using native_tu15_leaves::ordered_difference_ss;
using native_tu15_leaves::ordered_mul_ss;
using native_tu15_leaves::ordered_sum_ss;

template<class Spill>
class PlanarFacing {
    WorldState &s;
    Spill spill;

    std::uint32_t word(std::uint32_t address) const { return r(s.memory, address, 4); }
    __m128 scalar(std::uint32_t address) const { return _mm_set_ss(f(word(address))); }
    __m128 local(unsigned offset) const { return scalar(s.gp[4] + offset); }
    long double extended(std::uint32_t address) const { return ext(f(word(address))); }
    long double local_extended(unsigned offset) const { return extended(s.gp[4] + offset); }

    void save(std::uint32_t pc, unsigned offset, std::uint32_t value) {
        w(s.memory, s.observe, pc, s.gp[4] + offset, 4, value);
    }
    void save_lane(std::uint32_t pc, unsigned offset, unsigned lane) {
        save(pc, offset, u(_mm_cvtss_f32(s.xmm[lane])));
    }
    void read_normal_flag() {
        s.set(0, r(s.memory, s.gp[2] + 0x10u, 1), 1);
        s.flags.logic(s.reg(0, 1), 1);
    }
    void save_and_load_square_input(std::uint32_t pc, unsigned offset, unsigned lane) {
        save_lane(pc, offset, lane);
        s.fp.push(local_extended(offset));
    }
    void square_component(unsigned offset) {
        s.fp.st(0) = mul(s.fp.st(0), local_extended(offset));
    }
    bool normalizable(std::uint32_t spill_pc) {
        auto &fp = s.fp;
        fp.st(1) = add(fp.st(1), fp.st(0));
        fp.pop();
        fp.st(0) = root(fp.st(0));
        const auto word = spill(fp.st(0), fp.status);
        save(spill_pc, 0x10u, word);
        fp.st(0) = std::fabs(fp.st(0));
        fp.push(extended(0x46a0ecu));
        s.compare_and_pop(fp.st(1));
        (void)fp.st(0);
        fp.pop();
        return s.flags.cf || s.flags.zf;
    }

    void vertical_pair() {
        auto &g = s.gp;
        auto &x = s.xmm;
        read_normal_flag();
        x[1] = scalar(g[5] + 0x20u);
        x[0] = scalar(g[5] + 0x1cu);
        x[2] = scalar(g[5] + 0x24u);
        x[3] = scalar(g[5] + 0x24u);
        g[1] = g[5] + 0x1cu;
        x[5] = x[1];
        x[2] = ordered_difference_ss(x[2], x[4]);
        x[6] = x[0];
        save_lane(0x430f4u, 0xb4u, 5);
        x[3] = ordered_sum_ss(x[3], x[4]);
    }

    void vertical_direction() {
        auto &g = s.gp;
        auto &x = s.xmm;
        auto &flags = s.flags;
        if (flags.logic(g[3], 4)) {
            x[7] = scalar(g[3] + 0x20u);
            x[4] = x[0];
            x[4] = ordered_difference_ss(x[4], scalar(g[3] + 0x1cu));
            x[7] = ordered_difference_ss(x[7], x[1]);
            return;
        }
        g[0] = word(g[5] + 0x34u);
        g[6] = word(0x4f7210u);
        g[3] = word(g[6] + 0x44u);
        x[7] = scalar(g[5] + 0x20u);
        g[0] = flags.logic(g[0] & 0xffffu, 4);
        g[0] = flags.product(g[0], 0x38u);
        x[4] = scalar(g[3] + g[0] + 0x1cu);
        x[7] = ordered_difference_ss(x[7], scalar(g[3] + g[0] + 0x20u));
        x[4] = ordered_difference_ss(x[4], scalar(g[1]));
        g[0] = flags.plus(g[0], g[3], 4);
    }

    void vertical_normal() {
        auto &x = s.xmm;
        save_and_load_square_input(0x43149u, 0x18u, 4);
        square_component(0x18u);
        save_and_load_square_input(0x43157u, 0x14u, 7);
        x[4] = _mm_setzero_ps();
        square_component(0x14u);
        save_lane(0x43168u, 0x1cu, 4);
        if (!normalizable(0x43172u)) return;

        x[4] = scalar(0x46a0d0u);
        x[4] = _mm_div_ss(x[4], local(0x10u));
        x[5] = x[4];
        x[5] = ordered_mul_ss(x[5], x[7]);
        save_lane(0x4319du, 0x14u, 5);
        x[5] = x[4];
        x[5] = ordered_mul_ss(x[5], local(0x18u));
        x[4] = ordered_mul_ss(x[4], scalar(0x46a0e8u));
        save_lane(0x431b4u, 0x18u, 5);
        x[5] = local(0xb4u);
        save_lane(0x4353cu, 0x1cu, 4);
    }

    void horizontal_direction() {
        auto &g = s.gp;
        auto &x = s.xmm;
        auto &flags = s.flags;
        if (flags.logic(g[3], 4)) {
            x[0] = scalar(g[3] + 0x20u);
            x[3] = scalar(g[5] + 0x1cu);
            x[0] = ordered_difference_ss(x[0], scalar(g[5] + 0x20u));
            x[3] = ordered_difference_ss(x[3], scalar(g[3] + 0x1cu));
        } else {
            g[0] = word(g[5] + 0x34u);
            g[1] = word(0x4f7210u);
            g[6] = word(g[1] + 0x44u);
            x[0] = scalar(g[5] + 0x20u);
            g[0] = flags.logic(g[0] & 0xffffu, 4);
            g[0] = flags.product(g[0], 0x38u);
            x[3] = scalar(g[0] + g[6] + 0x1cu);
            x[0] = ordered_difference_ss(x[0], scalar(g[0] + g[6] + 0x20u));
            x[3] = ordered_difference_ss(x[3], scalar(g[5] + 0x1cu));
            g[0] = flags.plus(g[0], g[6], 4);
        }
        g[1] = g[5] + 0x1cu;
    }

    void horizontal_length() {
        auto &x = s.xmm;
        save_and_load_square_input(0x43214u, 0x5cu, 3);
        square_component(0x5cu);
        save_and_load_square_input(0x43222u, 0x58u, 0);
        square_component(0x58u);
        if (!normalizable(0x43234u)) return;

        x[1] = x[7];
        x[1] = _mm_div_ss(x[1], local(0x10u));
        x[2] = x[1];
        x[2] = ordered_mul_ss(x[2], x[0]);
        x[1] = ordered_mul_ss(x[1], x[3]);
        x[0] = x[2];
        x[3] = x[1];
    }

    void horizontal_pair() {
        auto &g = s.gp;
        auto &x = s.xmm;
        x[1] = scalar(g[5] + 0x20u);
        x[6] = scalar(g[1]);
        read_normal_flag();
        x[2] = scalar(g[5] + 0x24u);
        x[0] = ordered_mul_ss(x[0], x[4]);
        x[5] = x[0];
        x[0] = scalar(g[1]);
        x[3] = ordered_mul_ss(x[3], x[4]);
        x[1] = ordered_difference_ss(x[1], x[3]);
        x[3] = ordered_sum_ss(x[3], scalar(g[5] + 0x20u));
        x[0] = ordered_difference_ss(x[0], x[5]);
        x[6] = ordered_sum_ss(x[6], x[5]);
        x[5] = x[3];
        x[3] = x[2];
    }

    void horizontal_normal() {
        auto &g = s.gp;
        g[0] = word(0x474d10u);
        g[6] = word(g[0]);
        save(0x432a6u, 0x14u, g[6]);
        g[6] = word(g[0] + 4u);
        g[0] = word(g[0] + 8u);
        save(0x432b0u, 0x18u, g[6]);
        save(0x432b4u, 0x1cu, g[0]);
    }

public:
    PlanarFacing(WorldState &state, Spill round) : s(state), spill(round) {}

    void vertical() {
        vertical_pair();
        if (!s.flags.zf) {
            vertical_direction();
            vertical_normal();
        }
    }

    void horizontal() {
        horizontal_direction();
        horizontal_length();
        horizontal_pair();
        if (!s.flags.zf) horizontal_normal();
    }
};

template<class Spill>
bool bridge(WorldState &s, bool vertical, Spill spill) {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_CONTRAIL_PLANAR_FACING");
        return value && *value;
    }();
    if (reference || s.image != SourcePhysicsContactImage::TitleUpdate15 || s.services.instruction || s.observe ||
            !native_contrail_vertex_emission_detail::ScopedDrawServices::identifies(s)) return false;
    PlanarFacing frame(s, spill);
    if (vertical) frame.vertical();
    else frame.horizontal();
    return true;
}

} // namespace halo2::native_contrail_planar_facing_detail
