#pragma once

#include "rendering/effects/native_tu15_contrail_vertex_emission.hpp"

namespace halo2::native_contrail_bitmap_selector_detail {

using namespace physics_world_runtime_detail;

inline constexpr std::uint32_t absent = 0xffffffffu;

class BitmapSelector {
    WorldState &s;

    std::uint32_t word(std::uint32_t address, unsigned width = 4) const {
        return r(s.memory, address, width);
    }
    static std::int32_t signed_word(std::uint32_t value) {
        return std::bit_cast<std::int16_t>(std::uint16_t(value));
    }

    void divide_index(std::int32_t index, std::int32_t count) {
        // Both source IDIVs receive a sign-extended 16-bit dividend and a
        // positive divisor. The caller can observe both results before the
        // next memory operation, including a negative signed remainder.
        s.gp[0] = std::uint32_t(index / count);
        s.gp[2] = std::uint32_t(index % count);
    }

    void select_in_sequence() {
        auto &g = s.gp;
        auto &flags = s.flags;
        s.set(0, word(g[1] + 0x22u, 2), 2);
        flags.logic(s.reg(0, 2), 2);
        if (!flags.zf && flags.sf == flags.of) {
            g[6] = std::uint32_t(signed_word(g[0]));
            divide_index(signed_word(g[7]), std::int32_t(g[6]));
            g[1] = std::uint32_t(signed_word(word(g[1] + 0x20u, 2)));
            g[0] = flags.plus(g[2], g[1], 4);
        } else {
            g[0] = word(g[1] + 0x34u);
            if (!flags.logic(g[0], 4)) {
                g[0] = std::uint32_t(signed_word(g[7]));
                return;
            }
            g[0] = word(g[1] + 0x38u);
            g[2] = flags.shift(std::uint32_t(signed_word(g[7])), 5, 4);
            g[0] = std::uint32_t(signed_word(word(g[0] + g[2], 2)));
        }
        flags.minus(g[0], absent, 4);
        if (flags.zf) g[0] = std::uint32_t(signed_word(g[7]));
    }

    void select_bitmap() {
        auto &g = s.gp;
        auto &flags = s.flags;
        g[1] = word(g[6] + 0x3cu);
        flags.logic(g[1], 4);
        if (flags.zf || flags.sf != flags.of) {
            g[0] = flags.logic(0, 4);
            return;
        }
        divide_index(signed_word(word(g[4] + 8u, 2)), std::int32_t(g[1]));
        g[0] = word(g[6] + 0x40u);
        g[1] = flags.product(g[2], 0x3cu);
        g[1] = flags.plus(g[1], g[0], 4);
        select_in_sequence();
    }

public:
    explicit BitmapSelector(WorldState &state) : s(state) {}

    void run() {
        auto &g = s.gp;
        auto &flags = s.flags;
        g[0] = flags.logic(absent, 4);
        flags.minus(g[1], absent, 4);
        if (!flags.zf) {
            g[2] = word(0x4f0414u);
            g[1] = flags.logic(g[1] & 0xffffu, 4);
            g[1] = flags.shift(g[1], 4, 4);
            s.push(0x13a087u, g[6]);
            g[6] = word(g[1] + g[2] + 8u);
            if (flags.logic(g[6], 4)) select_bitmap();
            g[6] = s.pop();
        }
        (void)s.pop();
        g[4] += 4u;
    }
};

inline bool bridge(WorldState &s) {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_CONTRAIL_BITMAP_SELECTOR");
        return value && *value;
    }();
    if (reference || s.image != SourcePhysicsContactImage::TitleUpdate15 || s.services.instruction || s.observe ||
            !native_contrail_vertex_emission_detail::ScopedDrawServices::identifies(s)) return false;
    BitmapSelector(s).run();
    return true;
}

} // namespace halo2::native_contrail_bitmap_selector_detail
