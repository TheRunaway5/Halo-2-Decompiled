#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"

namespace halo2::native_particle_draw_matrix_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::flag;
using native_small_queries_detail::run_owner;
using native_tu15_leaves::ordered_mul_ss;
using native_world_leaves_detail::ordered_add_ss;

enum class Stage { basis, transform };
enum Branch : unsigned { prepared_basis, direct_source, aliased_source, branch_count };
inline std::atomic<std::uint64_t> &coverage() {
    static std::atomic<std::uint64_t> value{};
    return value;
}

template<class State>
struct Frame : Port<State> {
    Stage stage;
    Frame(State &state, std::uint32_t *cursor, Trace<State> *trace, Stage operation)
        : Port<State>(state, cursor, trace, {}), stage(operation) {}

    __m128 scalar(std::uint32_t pc, std::uint32_t address) {
        return _mm_set_ss(physics_step_detail::f(this->read(pc,address,4)));
    }

    void write_lane(std::uint32_t pc, std::uint32_t offset, __m128 value) {
        this->store(pc, this->gp[esp] + offset, 4, physics_step_detail::u(_mm_cvtss_f32(value)));
    }

    void build_basis() {
        auto &gp = this->gp;
        auto &lane = this->s.xmm;
        const auto stack = gp[esp];
        gp[edx] = this->read(0x17739fu, 0x4c65ccu, 4);
        gp[ecx] = this->read(0x1773a5u, 0x4c65d0u, 4);
        lane[2] = scalar(0x1773abu, 0x4c65d4u);
        lane[0] = scalar(0x1773b3u, 0x4c65e0u);
        lane[3] = scalar(0x1773bbu, 0x4c65dcu);
        lane[1] = scalar(0x1773c3u, 0x4c65d0u);
        lane[5] = ordered_mul_ss(lane[0], lane[1]);
        lane[4] = ordered_mul_ss(lane[2], lane[3]);

        this->store(0x1773d9u, stack + 0x4cu, 4, gp[edx]);
        gp[edx] = this->read(0x1773ddu, 0x4c65d4u, 4);
        this->store(0x1773e3u, stack + 0x50u, 4, gp[ecx]);
        gp[ecx] = this->read(0x1773e7u, 0x4c65d8u, 4);
        lane[4] = _mm_sub_ss(lane[4], lane[5]);
        lane[5] = scalar(0x1773f1u, 0x4c65d8u);
        write_lane(0x1773f9u, 0x58u, lane[4]);
        lane[4] = scalar(0x1773ffu, 0x4c65ccu);
        this->store(0x177407u, stack + 0x54u, 4, gp[edx]);
        gp[edx] = this->read(0x17740bu, 0x4c65dcu, 4);
        this->store(0x177411u, stack + 0x64u, 4, gp[ecx]);
        gp[ecx] = this->read(0x177415u, 0x4c65e0u, 4);

        lane[0] = ordered_mul_ss(lane[0], lane[4]);
        lane[2] = ordered_mul_ss(lane[2], lane[5]);
        lane[1] = ordered_mul_ss(lane[1], lane[5]);
        lane[3] = ordered_mul_ss(lane[3], lane[4]);
        lane[0] = _mm_sub_ss(lane[0], lane[2]);
        lane[1] = _mm_sub_ss(lane[1], lane[3]);
        write_lane(0x177433u, 0x5cu, lane[0]);
        write_lane(0x177439u, 0x60u, lane[1]);
        this->store(0x17743fu, stack + 0x68u, 4, gp[edx]);
        this->store(0x177443u, stack + 0x6cu, 4, gp[ecx]);
        this->reached(prepared_basis);
        this->publish(0x177443u);
    }

    void separate_overlapping_positions() {
        auto &gp = this->gp;
        const auto stack = gp[esp];
        gp[eax] = gp[esi] + 0x10u;
        gp[ecx] = stack + 0x2cu;
        this->compare32(gp[eax], gp[ecx]);
        if (!this->flags.zf) {
            this->reached(direct_source);
            return;
        }
        this->reached(aliased_source);
        gp[edx] = this->read(0x1774a9u, gp[eax], 4);
        gp[ecx] = this->read(0x1774abu, gp[eax] + 4u, 4);
        this->store(0x1774aeu, stack + 0x44u, 4, gp[edx]);
        gp[edx] = this->read(0x1774b2u, gp[eax] + 8u, 4);
        this->store(0x1774b5u, stack + 0x48u, 4, gp[ecx]);
        this->store(0x1774b9u, stack + 0x4cu, 4, gp[edx]);
        gp[eax] = stack + 0x44u;
    }

    void apply_basis() {
        auto &gp = this->gp;
        auto &lane = this->s.xmm;
        separate_overlapping_positions();
        const auto from = gp[eax];
        const auto stack = gp[esp];

        lane[0] = scalar(0x1774c1u, from + 4u);
        lane[0] = ordered_mul_ss(lane[0], scalar(0x1774c6u, stack + 0x5cu));
        lane[1] = scalar(0x1774ccu, from + 8u);
        lane[1] = ordered_mul_ss(lane[1], scalar(0x1774d1u, stack + 0x68u));
        lane[2] = scalar(0x1774d7u, from + 4u);
        lane[2] = ordered_mul_ss(lane[2], scalar(0x1774dcu, stack + 0x60u));
        lane[3] = scalar(0x1774e2u, from + 4u);
        lane[3] = ordered_mul_ss(lane[3], scalar(0x1774e7u, stack + 0x64u));
        lane[4] = scalar(0x1774edu, 0x4c65c0u);

        lane[0] = ordered_add_ss(lane[0], lane[1]);
        lane[1] = scalar(0x1774f9u, from);
        lane[1] = ordered_mul_ss(lane[1], scalar(0x1774fdu, stack + 0x50u));
        lane[0] = ordered_add_ss(lane[0], lane[1]);

        lane[1] = scalar(0x177507u, from);
        lane[1] = ordered_mul_ss(lane[1], scalar(0x17750bu, stack + 0x54u));
        lane[1] = ordered_add_ss(lane[1], lane[2]);
        lane[2] = scalar(0x177515u, from + 8u);
        lane[2] = ordered_mul_ss(lane[2], scalar(0x17751au, stack + 0x6cu));
        lane[1] = ordered_add_ss(lane[1], lane[2]);

        lane[2] = scalar(0x177524u, from);
        lane[2] = ordered_mul_ss(lane[2], scalar(0x177528u, stack + 0x58u));
        lane[2] = ordered_add_ss(lane[2], lane[3]);
        lane[3] = scalar(0x177532u, from + 8u);
        lane[3] = ordered_mul_ss(lane[3], scalar(0x177537u, stack + 0x70u));
        gp[eax] = this->read(0x17753du, 0x4c6854u, 4);
        lane[2] = ordered_add_ss(lane[2], lane[3]);

        lane[3] = scalar(0x177546u, 0x4c65c4u);
        lane[4] = ordered_add_ss(lane[4], lane[0]);
        lane[0] = scalar(0x177552u, 0x4c65c8u);
        lane[3] = ordered_add_ss(lane[3], lane[1]);
        lane[0] = ordered_add_ss(lane[0], lane[2]);
        this->store(0x177562u, gp[esi] + 0x30u, 4, gp[eax]);
        this->publish(0x177565u);
    }

    void run() {
        try {
            if (stage == Stage::basis) build_basis();
            else apply_basis();
        } catch (...) {
            this->s.gp = this->gp;
            this->s.flags = this->flags;
            throw;
        }
    }
};

template<class State, class Reference>
bool bridge(State &state, Stage stage, Reference reference) {
    static const bool reference_mode = flag("HALO2_REFERENCE_PARTICLE_DRAW_MATRIX");
    static const bool verify_mode = flag("HALO2_VERIFY_PARTICLE_DRAW_MATRIX");
    static VerifySummary summary{"Particle draw matrix"};
    const bool basis = stage == Stage::basis;
    return run_owner(state, basis ? 0x17739fu : 0x17749eu, basis ? 0x177443u : 0x177565u,
        reference_mode, verify_mode, coverage(), summary, reference,
        [&](State &value, std::uint32_t *cursor, Trace<State> *trace) {
            return Frame<State>{value, cursor, trace, stage};
        });
}
} // namespace halo2::native_particle_draw_matrix_detail
