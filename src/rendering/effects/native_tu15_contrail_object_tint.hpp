#pragma once

#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include "rendering/effects/native_tu15_contrail_vertex_emission.hpp"

namespace halo2::native_contrail_object_tint_detail {

using namespace physics_world_runtime_detail;

enum class Next { retained, prepare_orientation, object_color };

class ObjectTint {
    WorldState &s;

    std::uint32_t read(std::uint32_t address, unsigned width = 4) const {
        return r(s.memory, address, width);
    }

    std::uint32_t local(unsigned offset) const { return read(s.gp[4] + offset); }

    std::uint32_t signed_word(std::uint32_t address) const {
        const auto raw = static_cast<std::uint16_t>(read(address, 2));
        return static_cast<std::uint32_t>(static_cast<std::int32_t>(std::bit_cast<std::int16_t>(raw)));
    }

    void store(std::uint32_t pc, unsigned offset, std::uint32_t value) {
        w(s.memory, s.observe, pc, s.gp[4] + offset, 4, value);
    }

    bool admitted() const {
        static const bool reference = [] {
            const auto *value = std::getenv("HALO2_REFERENCE_CONTRAIL_OBJECT_TINT");
            return value && *value;
        }();
        return !reference && s.image == SourcePhysicsContactImage::TitleUpdate15 &&
            native_contrail_vertex_emission_detail::ScopedDrawServices::identifies(s) &&
            !s.services.instruction && !s.observe;
    }

    void seed_color() {
        auto &gp = s.gp;
        gp[2] = read(0x474c70u);
        gp[1] = read(gp[2]);
        store(0x42fe5u, 0x78u, gp[1]);
        gp[1] = read(gp[2] + 4u);
        gp[2] = read(gp[2] + 8u);
        store(0x42fefu, 0x7cu, gp[1]);
        store(0x42ff3u, 0x80u, gp[2]);
    }

    void resolve_color_arguments() {
        auto &gp = s.gp;
        auto &flags = s.flags;
        gp[2] = read(0x4ecbd0u);
        gp[2] = read(gp[2] + 0x44u);
        gp[1] = flags.logic(gp[0] & 0xffffu, 4);
        gp[1] *= 3u;
        gp[1] = read(gp[2] + gp[1] * 4u + 8u);
        gp[2] = read(gp[1]);
        gp[1] = read(0x4f0414u);
        gp[2] = flags.logic(gp[2] & 0xffffu, 4);
        gp[2] = flags.shift(gp[2], 4, 4);
        gp[2] = read(gp[2] + gp[1] + 8u);
        gp[1] = signed_word(gp[6] + 0xcu);
        gp[2] = read(gp[2] + 0x98u);
        gp[1] *= 3u;
        gp[7] = signed_word(gp[2] + gp[1] * 8u + 0xcu);
        gp[6] = gp[4] + 0x78u;
    }

public:
    explicit ObjectTint(WorldState &s) : s(s) {}

    Next prepare() {
        if (!admitted())
            return Next::retained;
        auto &gp = s.gp;
        gp[0] = read(gp[6] + 8u);
        s.flags.minus(gp[0], 0xffffffffu, 4);
        if (s.flags.zf)
            return Next::prepare_orientation;
        gp[1] = local(0x2cu);
        s.flags.logic(read(gp[1], 1) & 0x80u, 1);
        if (!s.flags.zf)
            return Next::prepare_orientation;
        seed_color();
        resolve_color_arguments();
        return Next::object_color;
    }

    bool apply() {
        if (!admitted())
            return false;
        auto &x = s.xmm;
        x[0] = _mm_set_ss(f(local(0x78u)));
        s.gp[7] = local(0x50u);
        s.gp[2] = local(0xc8u);
        constexpr std::array lanes{4u, 5u, 3u};
        constexpr std::array sites{0x43057u, 0x43067u, 0x4307au};
        for (unsigned channel = 0; channel < lanes.size(); ++channel) {
            if (channel != 0u)
                x[0] = _mm_set_ss(f(local(0x78u + channel * 4u)));
            x[0] = native_tu15_leaves::ordered_mul_ss(x[0], x[lanes[channel]]);
            store(sites[channel], 0x34u + channel * 4u, u(_mm_cvtss_f32(x[0])));
        }
        return true;
    }
};

} // namespace halo2::native_contrail_object_tint_detail
