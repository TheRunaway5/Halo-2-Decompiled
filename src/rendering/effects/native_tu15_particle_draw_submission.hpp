#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"

namespace halo2::native_particle_draw_submission_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::flag;
using native_small_queries_detail::run_owner;

enum class Stage { transform, submit };
enum Branch : unsigned { transformed, discarded, submitted, branch_count };
inline std::atomic<std::uint64_t> &coverage() {
    static std::atomic<std::uint64_t> value{};
    return value;
}
struct Argument {
    std::uint32_t instruction;
    std::uint32_t value;
};
inline constexpr std::array<Argument, 7> submit_arguments{{
    {0x17778cu, 0x176c20u}, {0x177791u, 0x176b50u}, {0x177796u, 0u}, {0x177798u, 0xffffffffu},
    {0x17779au, 0xffffffffu}, {0x17779cu, 5u}, {0x17779eu, 0u},
}};

template<class State, class Dispatch>
struct Frame : Port<State> {
    Stage stage;
    Dispatch &dispatch;

    Frame(State &state, std::uint32_t *cursor, Trace<State> *trace, Stage operation, Dispatch &route)
        : Port<State>(state, cursor, trace, {}), stage(operation), dispatch(route) {}

    void routed(std::uint32_t pc, std::uint32_t returned, std::uint32_t target) {
        this->call(pc, returned, target, [&](State &state) {
            try {
                dispatch(state, target, pc);
            } catch (...) {
                this->gp = state.gp;
                this->flags = state.flags;
                throw;
            }
        });
    }

    void transform() {
        auto &gp = this->gp;
        gp[ecx] = gp[esp] + 0x24u;
        this->push(0x17775du, gp[ecx]);
        this->push(0x17775eu, 0x176b30u);
        gp[edx] = gp[esp] + 0x34u;
        gp[ecx] = 8u;
        routed(0x17776cu, 0x177771u, 0x1e4c0u);
        gp[esi] = this->read(0x177771u, gp[esp] + 0x28u, 4);
        this->reached(transformed);
    }

    void submit() {
        auto &gp = this->gp;
        auto &flags = this->flags;
        flags.logic(gp[edi] & 0xffff7fffu, 4);
        if (flags.zf) {
            this->reached(discarded);
            this->publish(0x17777bu);
            return;
        }
        gp[ecx] = gp[ebp];
        routed(0x17777fu, 0x177784u, 0x178070u);
        gp[edx] = this->read(0x177784u, gp[esp] + 0x78u, 4);
        gp[edi] = this->read(0x177788u, gp[eax] + 4u, 4);
        this->push(0x17778bu, gp[edx]);
        for (const auto &argument : submit_arguments) this->push(argument.instruction, argument.value);
        gp[eax] = flags.logic(0u, 4);
        routed(0x1777a2u, 0x1777a7u, 0x40e20u);
        this->s.xmm[7] = _mm_set_ss(physics_step_detail::f(this->read(0x1777a7u, gp[esp] + 0x18u, 4)));
        this->reached(submitted);
        this->publish(0x1777adu);
    }

    void run() {
        try {
            if (stage == Stage::transform) transform();
            submit();
        } catch (...) {
            this->s.gp = this->gp;
            this->s.flags = this->flags;
            throw;
        }
    }
};

template<class State, class Reference, class Dispatch>
bool bridge(State &state, Stage stage, Reference reference, Dispatch dispatch) {
    static const bool reference_mode = flag("HALO2_REFERENCE_PARTICLE_DRAW_SUBMISSION");
    static const bool verify_mode = flag("HALO2_VERIFY_PARTICLE_DRAW_SUBMISSION");
    static VerifySummary summary{"Particle draw submission"};
    const auto start = stage == Stage::transform ? 0x177759u : 0x177775u;
    return run_owner(state, start, 0x1777adu, reference_mode, verify_mode, coverage(), summary, reference,
        [&](State &value, std::uint32_t *cursor, Trace<State> *trace) {
            return Frame<State, Dispatch>{value, cursor, trace, stage, dispatch};
        });
}
} // namespace halo2::native_particle_draw_submission_detail
