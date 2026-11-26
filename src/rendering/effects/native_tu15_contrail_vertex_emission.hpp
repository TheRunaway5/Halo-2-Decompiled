#pragma once

#include "physics/runtime/physics_world_runtime_internal.hpp"

#include <array>
#include <cstdlib>

namespace halo2::native_contrail_vertex_emission_detail {

using physics_world_runtime_detail::WorldState;
using physics_world_runtime_detail::ext;
using physics_world_runtime_detail::f;
using physics_world_runtime_detail::integer_spill;
using physics_world_runtime_detail::mul;
using physics_world_runtime_detail::r;
using physics_world_runtime_detail::u;
using physics_world_runtime_detail::w;

// The presenter owns the short-lived service table and does not install an
// instruction observer while emitting geometry into its private draw bank.
class ScopedDrawServices {
    inline static thread_local const SourcePhysicsWorldRuntimeServices *active{};
    const SourcePhysicsWorldRuntimeServices *previous;

public:
    explicit ScopedDrawServices(const SourcePhysicsWorldRuntimeServices &services)
        : previous(active) { active = &services; }
    ~ScopedDrawServices() { active = previous; }
    ScopedDrawServices(const ScopedDrawServices &) = delete;
    ScopedDrawServices &operator=(const ScopedDrawServices &) = delete;

    static bool identifies(const WorldState &state) { return active == &state.services; }
};

inline bool admitted(const WorldState &state) {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_CONTRAIL_VERTEX_EMISSION");
        return value && *value;
    }();
    return !reference && ScopedDrawServices::identifies(state) &&
        !state.services.instruction && !state.observe;
}

class VertexEmission {
    WorldState &state;

    std::uint32_t read(std::uint32_t address, unsigned width = 4) const {
        return r(state.memory, address, width);
    }

    void write(std::uint32_t pc, std::uint32_t address, std::uint32_t value) {
        w(state.memory, state.observe, pc, address, 4, value);
    }

    float single(std::uint32_t address) const { return f(read(address)); }
    std::uint32_t bits(unsigned lane) const { return u(_mm_cvtss_f32(state.xmm[lane])); }

    void clamp_fade(std::uint32_t frame) {
        auto &fade = state.xmm[0];
        auto &limit = state.xmm[1];
        fade = _mm_mul_ss(fade, _mm_set_ss(single(frame + 0x30u)));
        limit = _mm_setzero_ps();
        state.flags.scalar_compare(limit, fade, false);
        write(0x43584u, frame + 0x30u, bits(0));
        if (state.flags.cf || state.flags.zf) {
            limit = _mm_set_ss(single(0x46a0d0u));
            state.flags.scalar_compare(fade, limit, false);
        }
        if (!state.flags.cf && !state.flags.zf)
            write(0x43599u, frame + 0x30u, bits(1));
    }

    void pack_color(std::uint32_t frame) {
        auto &gp = state.gp;
        auto &fp = state.fp;
        state.xmm[0] = _mm_set_ss(single(0x46a148u));
        gp[1] = frame + 0x30u;
        write(0x435abu, frame + 0x88u, gp[1]);
        write(0x435b2u, frame + 0x98u, bits(0));
        write(0x435bbu, frame + 0x10u, 0);

        gp[2] = read(frame + 0x88u);
        for (unsigned channel = 0; channel < 4; ++channel)
            fp.push(ext(single(gp[2] + channel * 4u)));
        fp.push(ext(single(frame + 0x98u)));
        for (unsigned lane = 4; lane > 0; --lane)
            fp.st(lane) = mul(fp.st(lane), fp.st(0));
        fp.pop();

        // Separate FISTP stores retain both the source spill addresses and
        // the x87 depth and rounding status if a store fails partway through.
        constexpr std::array spill_pc{0x435e4u, 0x435ebu, 0x435f2u, 0x435f9u};
        constexpr std::array spill_offset{0xa0u, 0x9cu, 0x94u, 0x84u};
        for (unsigned channel = 0; channel < 4; ++channel) {
            write(spill_pc[channel], frame + spill_offset[channel], integer_spill(fp.st(0), fp.status));
            fp.pop();
        }

        gp[2] = read(frame + spill_offset[0]);
        gp[3] = read(frame + spill_offset[1]);
        gp[1] = read(frame + spill_offset[2]);
        gp[0] = read(frame + spill_offset[3]);
        gp[3] <<= 8u;
        gp[1] <<= 16u;
        gp[0] <<= 24u;
        gp[2] = state.flags.logic(gp[2] | gp[3] | gp[1] | gp[0], 4);
        write(0x4362bu, frame + 0x10u, gp[2]);
    }

    bool append_pair(std::uint32_t frame) {
        auto &gp = state.gp;
        gp[0] = read(frame + 0x50u);
        gp[1] = read(frame + 0x10u);
        gp[2] = read(frame + 0x54u);
        state.xmm[0] = _mm_set_ss(single(frame + 0xa4u));
        state.xmm[0] = _mm_add_ss(state.xmm[0], _mm_set_ss(single(frame + 0x60u)));
        gp[6] = read(frame + 0x70u);
        write(0x4364eu, gp[0] - 0x14u, gp[1]);
        write(0x43651u, gp[0] + 4u, gp[1]);

        gp[2] = state.flags.plus(gp[2], 0x30u, 4);
        gp[3] = gp[5];
        gp[5] = read(gp[5] + 0x34u);
        gp[0] = state.flags.plus(gp[0], 0x30u, 4);
        state.flags.minus(gp[5], 0xffffffffu, 4);
        write(0x43662u, frame + 0x54u, gp[2]);
        gp[2] = read(frame + 0xc8u);
        write(0x4366du, frame + 0x50u, gp[0]);
        write(0x43671u, frame + 0x60u, bits(0));
        if (!state.flags.zf)
            return true;

        gp[7] = read(frame + 0x54u);
        return false;
    }

public:
    explicit VertexEmission(WorldState &state) : state(state) {}

    void prepare_pair() {
        auto &gp = state.gp;
        const auto frame = gp[4];
        gp[0] = read(frame + 0x54u);
        write(0x43546u, gp[0], bits(6));
        gp[0] = read(frame + 0x2cu);
        write(0x4354eu, gp[7] - 0x24u, bits(5));
        write(0x43553u, gp[7] - 0x20u, bits(3));
        write(0x43558u, gp[7] - 0x10u, bits(0));
        write(0x4355du, gp[7] - 0x0cu, bits(1));
        write(0x43562u, gp[7] - 0x08u, bits(2));
        gp[2] = read(gp[2] + 0x0cu);
        state.set(6, read(gp[2] + 0x2cu, 2), 2, 0);
        state.push(0x4356eu, gp[0]);
        gp[0] = gp[4] + 0x18u;
    }

    bool finish_pair() {
        const auto frame = state.gp[4];
        clamp_fade(frame);
        pack_color(frame);
        return append_pair(frame);
    }
};

} // namespace halo2::native_contrail_vertex_emission_detail
