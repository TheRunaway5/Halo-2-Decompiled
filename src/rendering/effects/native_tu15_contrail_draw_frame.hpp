#pragma once

#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include "rendering/effects/native_tu15_contrail_vertex_emission.hpp"

namespace halo2::native_contrail_draw_frame_detail {

using namespace physics_world_runtime_detail;
using native_tu15_leaves::ordered_difference_ss;
using native_tu15_leaves::ordered_mul_ss;
using native_tu15_leaves::ordered_sum_ss;

enum class Start { retained, points, endcaps };

class DrawFrame {
    WorldState &s;

    std::uint32_t read(std::uint32_t address, unsigned width = 4) const {
        return r(s.memory, address, width);
    }
    std::uint32_t local(unsigned offset) const { return read(s.gp[4] + offset); }
    __m128 scalar(std::uint32_t address) const { return _mm_set_ss(f(read(address))); }
    std::uint32_t bits(unsigned lane) const { return u(_mm_cvtss_f32(s.xmm[lane])); }
    std::uint32_t chain() const {
        const auto value = std::uint16_t(read(s.gp[2] + 8u, 2));
        return std::uint32_t(std::bit_cast<std::int16_t>(value));
    }
    void write(std::uint32_t pc, std::uint32_t address, std::uint32_t value) {
        w(s.memory, s.observe, pc, address, 4, value);
    }
    void save(std::uint32_t pc, unsigned offset, std::uint32_t value) {
        write(pc, s.gp[4] + offset, value);
    }

    void clear_head_alpha() {
        auto &g = s.gp;
        g[0] = chain();
        s.flags.minus(read(g[6] + 0x30u + g[0] * 2u, 2), 2u, 2);
        if (!s.flags.zf && s.flags.sf == s.flags.of) {
            g[0] = read(g[7] + 0x14u);
            g[0] = s.flags.logic(g[0] & 0xffffffu, 4);
            write(0x436a6u, g[7] + 0x14u, g[0]);
            write(0x436a9u, g[7] + 0x2cu, g[0]);
        }
    }

    void clear_tail_alpha() {
        auto &g = s.gp;
        g[0] = read(g[2] + 0x14u);
        g[1] = g[0] * 3u;
        g[0] = read(g[7] + g[1] * 8u - 4u);
        g[1] = g[7] + g[1] * 8u - 4u;
        g[0] = s.flags.logic(g[0] & 0xffffffu, 4);
        write(0x436c4u, g[1], g[0]);
        g[1] = read(g[2] + 0x14u);
        g[2] = g[1] * 3u;
        write(0x436ccu, g[7] + g[2] * 8u - 0x1cu, g[0]);
    }

    void finish_endcaps() {
        auto &g = s.gp;
        g[0] = read(g[2] + 0x14u);
        g[1] = local(0x2cu);
        g[0] = s.flags.product(g[0], 0xffffffe8u);
        g[7] = s.flags.plus(g[7], g[0], 4);
        if (!s.flags.logic(read(g[1], 1) & 1u, 1))
            clear_head_alpha();
        if (!s.flags.logic(read(g[1], 1) & 2u, 1))
            clear_tail_alpha();
    }

public:
    explicit DrawFrame(WorldState &state) : s(state) {}

    Start open() {
        auto &g = s.gp;
        auto &x = s.xmm;
        auto &flags = s.flags;
        g[4] = flags.minus(g[4], 0xacu, 4);
        g[2] = local(0xb8u);
        g[1] = local(0xb0u);
        s.push(0x42cf4u, g[3]);
        g[3] = read(g[2] + 4u);
        s.set(0, read(g[3] + 2u, 2), 2);
        flags.logic(s.reg(0, 1) & 0x40u, 1);
        x[1] = scalar(g[3] + 0x1cu);
        s.push(0x42d03u, g[5]);
        s.push(0x42d04u, g[6]);
        g[6] = read(g[2]);
        x[0] = scalar(g[6] + 0x1cu);
        s.push(0x42d0cu, g[7]);
        g[7] = g[1];
        save(0x42d0fu, 0x54u, g[7]);
        save(0x42d13u, 0x70u, g[6]);
        save(0x42d17u, 0x2cu, g[3]);
        save(0x42d1bu, 0x60u, bits(0));
        if (!flags.zf) {
            x[0] = scalar(g[6] + 0x14u);
            x[0] = ordered_mul_ss(x[0], x[1]);
            x[1] = x[0];
        }

        flags.logic(s.reg(0, 1), 1);
        x[0] = ordered_difference_ss(_mm_setzero_ps(), x[1]);
        x[1] = scalar(g[6] + 0x20u);
        save(0x42d3du, 0xa4u, bits(0));
        x[0] = scalar(g[3] + 0x20u);
        save(0x42d4bu, 0x90u, bits(1));
        if (flags.sf) {
            x[2] = scalar(g[6] + 0x14u);
            x[2] = ordered_mul_ss(x[2], x[0]);
            x[0] = x[2];
        }

        g[0] = chain();
        g[5] = read(g[6] + 0x38u + g[0] * 4u);
        g[3] = flags.logic(0, 4);
        flags.minus(g[5], 0xffffffffu, 4);
        x[0] = ordered_sum_ss(x[0], x[1]);
        save(0x42d73u, 0xacu, bits(0));
        if (flags.zf) return Start::endcaps;
        g[7] = g[1] + 0x28u;
        save(0x42d85u, 0x50u, g[7]);
        return Start::points;
    }

    void close(bool drawn) {
        auto &g = s.gp;
        if (drawn) finish_endcaps();
        g[7] = s.pop();
        g[6] = s.pop();
        g[5] = s.pop();
        const auto result = drawn ? 1u : s.flags.logic(0, 1);
        s.set(0, result, 1);
        g[3] = s.pop();
        g[4] = s.flags.plus(g[4], 0xacu, 4);
        (void)s.pop();
        g[4] += 12u;
    }
};

inline bool admitted(const WorldState &s) {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_CONTRAIL_DRAW_FRAME");
        return value && *value;
    }();
    return !reference && s.image == SourcePhysicsContactImage::TitleUpdate15 && !s.services.instruction && !s.observe &&
        native_contrail_vertex_emission_detail::ScopedDrawServices::identifies(s);
}

inline Start open(WorldState &s) {
    return admitted(s) ? DrawFrame(s).open() : Start::retained;
}

inline bool close(WorldState &s, bool drawn) {
    if (!admitted(s)) return false;
    DrawFrame(s).close(drawn);
    return true;
}

} // namespace halo2::native_contrail_draw_frame_detail
