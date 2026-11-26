#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/physics_solver_internal.hpp"

namespace halo2::native_particle_draw_distance_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::flag;
using native_small_queries_detail::run_owner;
namespace scalar = physics_step_detail;

enum Branch : unsigned { bypass_distance, accept_distance, reject_distance, branch_count };
inline std::atomic<std::uint64_t> &coverage() {
    static std::atomic<std::uint64_t> value{};
    return value;
}
struct Axis {
    std::uint32_t position, origin, difference, stack_slot;
};
inline constexpr std::array<Axis, 3> observer_axes{{
    {0x177650u, 0x177655u, 0x17765du, 0x38u},
    {0x177663u, 0x177668u, 0x177670u, 0x3cu},
    {0x177676u, 0x17767bu, 0x177683u, 0x40u},
}};

template<class State, class Check, class Spill>
struct Frame : Port<State> {
    Check &check;
    Spill &spill;

    Frame(State &state, std::uint32_t *cursor, Trace<State> *trace, Check &query, Spill &store_float)
        : Port<State>(state, cursor, trace, {}), check(query), spill(store_float) {}

    __m128 load_scalar(std::uint32_t pc, std::uint32_t address) {
        return _mm_set_ss(scalar::f(this->read(pc, address, 4)));
    }

    long double load_extended(std::uint32_t pc, std::uint32_t address) {
        return scalar::ext(scalar::f(this->read(pc, address, 4)));
    }

    void observer_offset() {
        auto &gp = this->gp;
        auto &delta = this->s.xmm[0];
        for (unsigned axis = 0; axis < observer_axes.size(); ++axis) {
            const auto &site = observer_axes[axis];
            delta = load_scalar(site.position, gp[esi] + 0x10u + 4u * axis);
            delta = _mm_sub_ss(delta, load_scalar(site.origin, 0x4c65c0u + 4u * axis));
            this->store(site.difference, gp[esp] + site.stack_slot, 4, scalar::u(_mm_cvtss_f32(delta)));
        }
    }

    // Keep the ordered stack reads for the squared axes. Provider faults can
    // observe the first component on the source FP stack before the next one.
    void squared_axis(std::uint32_t load, std::uint32_t multiply, std::uint32_t stack_slot) {
        auto &fp = this->s.fp;
        const auto address = this->gp[esp] + stack_slot;
        fp.push(load_extended(load, address));
        fp.status &= ~0x200u;
        fp.st(0) = scalar::mul(fp.st(0), load_extended(multiply, address));
        fp.st(1) = scalar::add(fp.st(1), fp.st(0));
        fp.pop();
    }

    bool observer_distance_passes() {
        auto &gp = this->gp;
        auto &fp = this->s.fp;
        auto &xmm = this->s.xmm;
        fp.push(load_extended(0x17768bu, gp[esp] + 0x40u));
        fp.status &= ~0x200u;
        gp[eax] = this->read(0x17768fu, gp[esp] + 0x14u, 4);
        fp.st(0) = scalar::mul(fp.st(0), load_extended(0x177693u, gp[esp] + 0x40u));
        squared_axis(0x177697u, 0x17769bu, 0x3cu);
        squared_axis(0x1776a1u, 0x1776a5u, 0x38u);
        fp.st(0) = physics_solver_detail::root(fp.st(0));
        fp.st(0) = scalar::mul(fp.st(0), load_extended(0x1776adu, 0x4c65ecu));
        const auto result = spill(fp.st(0), fp.status);
        this->store(0x1776b3u, gp[esp] + 0x1cu, 4, result);
        fp.pop();

        xmm[1] = load_scalar(0x1776b7u, gp[esp] + 0x1cu);
        this->call(0x1776bdu, 0x1776c2u, 0x252ec0u, [&](State &state) {
            try {
                check(state);
            } catch (...) {
                gp = state.gp;
                this->flags = state.flags;
                throw;
            }
        });
        this->flags.scalar_compare(xmm[0], load_scalar(0x1776c2u, 0x46a0e8u), false);
        return !this->flags.cf && !this->flags.zf;
    }

    void evaluate() {
        auto &gp = this->gp;
        this->test8(gp[eax]);
        observer_offset();
        if (!this->flags.zf) {
            this->reached(bypass_distance);
            set_low_byte(gp[ebx], 1u);
        } else if (observer_distance_passes()) {
            this->reached(accept_distance);
            set_low_byte(gp[ebx], 1u);
        } else {
            this->reached(reject_distance);
            set_low_byte(gp[ebx], this->flags.logic(0, 1));
        }
        gp[edx] = this->read(0x1776d1u, 0x4c6854u, 4);
        this->store(0x1776d7u, gp[esi] + 0x30u, 4, gp[edx]);
        this->publish(0x1776d7u);
    }

    void run() {
        try {
            evaluate();
        } catch (...) {
            this->s.gp = this->gp;
            this->s.flags = this->flags;
            throw;
        }
    }
};

template<class State, class Reference, class Check, class Spill>
bool bridge(State &state, Reference reference, Check check, Spill spill) {
    static const bool reference_mode = flag("HALO2_REFERENCE_PARTICLE_DRAW_DISTANCE");
    static const bool verify_mode = flag("HALO2_VERIFY_PARTICLE_DRAW_DISTANCE");
    static VerifySummary summary{"Particle draw distance"};
    return run_owner(state, 0x17764eu, 0x1776d7u, reference_mode, verify_mode, coverage(), summary, reference,
        [&](State &value, std::uint32_t *cursor, Trace<State> *trace) {
            return Frame<State, Check, Spill>{value, cursor, trace, check, spill};
        });
}
} // namespace halo2::native_particle_draw_distance_detail
