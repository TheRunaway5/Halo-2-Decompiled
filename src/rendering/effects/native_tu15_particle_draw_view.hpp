#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"

namespace halo2::native_particle_draw_view_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::flag;
using native_small_queries_detail::run_owner;
using native_tu15_leaves::ordered_mul_ss;

enum class Stage { prepare, first, next };
enum class Resume : std::uint32_t {
    retained = 0xffffffffu, matrix = 0x17739fu, first_view = 0x177447u,
    descendants = 0x1777c4u, skip_view = 0x1777afu, transform = 0x17749eu,
    visibility = 0x17756au,
};
enum Branch : unsigned { frame_matrix, direct_view, empty_views, view_mismatch, transformed,
                        current_visibility, next_view, branch_count };
inline std::atomic<std::uint64_t> &coverage() {
    static std::atomic<std::uint64_t> value{};
    return value;
}

template<class State>
struct Frame : Port<State> {
    Stage stage;
    Resume &continuation;
    Frame(State &state, std::uint32_t *cursor, Trace<State> *trace, Stage part, Resume &next)
        : Port<State>(state, cursor, trace, {}), stage(part), continuation(next) {}

    void finish(Branch branch, Resume next, std::uint32_t pc) {
        this->reached(branch);
        continuation = next;
        this->publish(pc);
    }

    void prepare_view() {
        auto &gp = this->gp;
        auto &size = this->s.xmm[7];
        const auto raw_size = this->read(0x177376u, gp[edi] + 0x14u, 2);
        gp[ecx] = static_cast<std::uint32_t>(static_cast<std::int16_t>(raw_size));
        gp[eax] = this->read(0x17737au, gp[ebp] + 0x30u, 4);
        size = _mm_cvtsi32_ss(size, std::bit_cast<std::int32_t>(gp[ecx]));
        set_low_byte(gp[ecx], this->read(0x177381u, gp[esp] + 0x7cu, 1));
        this->test8(gp[ecx]);
        const auto scale = this->read(0x177387u, 0x46a12cu, 4);
        size = ordered_mul_ss(size, _mm_set_ss(physics_step_detail::f(scale)));
        this->store(0x17738fu, gp[esp] + 0x20u, 4, gp[ebp]);
        this->store(0x177393u, gp[esp] + 0x14u, 4, physics_step_detail::u(_mm_cvtss_f32(size)));
        if (this->flags.zf) finish(direct_view, Resume::first_view, 0x177399u);
        else finish(frame_matrix, Resume::matrix, 0x177399u);
    }

    void bind_view() {
        auto &gp = this->gp;
        auto &flags = this->flags;
        gp[edx] = this->read(0x177460u, 0x52d71cu, 4);
        gp[esi] = this->read(0x177466u, gp[edx] + 0x44u, 4);
        set_low_byte(gp[ecx], this->read(0x177469u, gp[esp] + 0x7cu, 1));
        gp[eax] = flags.product(flags.logic(gp[eax] & 0xffffu, 4), 0x34u);
        gp[eax] = flags.plus(gp[eax], gp[esi], 4);
        gp[esi] = gp[eax];
        set_low_byte(gp[edx], this->read(0x177479u, gp[esi] + 2u, 1));
        this->test8(gp[edx]);
        set_low_byte(gp[eax], !flags.zf);
        flags.minus(low_byte(gp[ecx]), low_byte(gp[eax]), 1);
        this->store(0x177483u, gp[esp] + 0x28u, 4, gp[esi]);
        if (!flags.zf) {
            finish(view_mismatch, Resume::skip_view, 0x177487u);
            return;
        }
        set_low_byte(gp[eax], this->read(0x17748du, gp[esp] + 0x80u, 1));
        this->test8(gp[eax]);
        set_low_byte(gp[ebx], 1u);
        if (flags.zf) finish(current_visibility, Resume::visibility, 0x177498u);
        else finish(transformed, Resume::transform, 0x177498u);
    }

    void select_view() {
        auto &gp = this->gp;
        if (stage == Stage::first) {
            this->compare32(gp[eax], 0xffffffffu);
            if (this->flags.zf) {
                finish(empty_views, Resume::descendants, 0x17744au);
                return;
            }
            this->push(0x177450u, gp[ebx]);
        } else {
            this->reached(next_view);
            gp[edi] = this->read(0x177453u, gp[esp] + 0x14u, 4);
        }
        bind_view();
    }

    void run() {
        try {
            if (stage == Stage::prepare) prepare_view();
            else select_view();
        } catch (...) {
            this->s.gp = this->gp;
            this->s.flags = this->flags;
            throw;
        }
    }
};

template<class State, class Reference>
Resume bridge(State &state, Stage stage, Reference reference) {
    static const bool reference_mode = flag("HALO2_REFERENCE_PARTICLE_DRAW_VIEW");
    static const bool verify_mode = flag("HALO2_VERIFY_PARTICLE_DRAW_VIEW");
    static VerifySummary summary{"Particle draw view"};
    constexpr std::array first{0x177376u, 0x177447u, 0x177453u};
    constexpr std::array last{0x177399u, 0x177498u, 0x177498u};
    const auto index = static_cast<unsigned>(stage);
    auto continuation = Resume::retained;
    const auto verify_reference = [&](State &ref) {
        if (reference(ref) != continuation)
            throw std::runtime_error("Particle draw view continuation differed");
    };
    run_owner(state, first[index], last[index], reference_mode, verify_mode, coverage(), summary, verify_reference,
        [&](State &value, std::uint32_t *cursor, Trace<State> *trace) {
            return Frame<State>{value, cursor, trace, stage, continuation};
        });
    return continuation;
}
} // namespace halo2::native_particle_draw_view_detail
