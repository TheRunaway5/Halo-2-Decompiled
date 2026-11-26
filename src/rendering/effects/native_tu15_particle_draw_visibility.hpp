#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"

namespace halo2::native_particle_draw_visibility_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::flag;
using native_small_queries_detail::run_owner;

enum class Resume : std::uint32_t {
    retained = 0xffffffffu, draw = 0x17764eu, copy = 0x1776dau,
};
enum Branch : unsigned {skip_by_cluster, mark_visible, skip_by_query, draw_unmarked, query_cluster, query_view, branch_count};
inline std::atomic<std::uint64_t> &coverage() {
    static std::atomic<std::uint64_t> result{};
    return result;
}

template<class State, class Dispatch>
struct Frame : Port<State> {
    Dispatch &dispatch;
    Resume &continuation;
    Frame(State &state, std::uint32_t *cursor, Trace<State> *trace, Dispatch &route, Resume &next)
        : Port<State>(state, cursor, trace, {}), dispatch(route), continuation(next) {}

    bool read_view_test(std::uint32_t load) {
        auto &gp = this->gp;
        set_low_byte(gp[ecx], this->read(load, gp[esp] + 0x84u, 1));
        this->test8(gp[ecx]);
        return !this->flags.zf;
    }

    bool visible_cluster() {
        if (read_view_test(0x177594u)) return true;

        auto &gp = this->gp;
        auto &flags = this->flags;
        gp[ecx] = flags.logic(0, 4);
        gp[ecx] = this->read(0x1775a1u, gp[edi] + 0x16u, 2);
        gp[ecx] = flags.right(gp[ecx], 9u, 4, false);
        flags.logic(gp[ecx] & 1u, 1);
        if (!flags.zf) return true;

        gp[ecx] = flags.logic(gp[edx] & 31u, 4);
        gp[edi] = flags.shift(1u, low_byte(gp[ecx]), 4);
        gp[ecx] = flags.right(gp[edx], 5u, 4, true);
        const auto visible = this->read(0x1775beu, 0x557034u + gp[ecx] * 4u, 4);
        flags.logic(visible & gp[edi], 4);
        return !flags.zf;
    }

    void routed(std::uint32_t pc, std::uint32_t returned, std::uint32_t target) {
        this->call(pc, returned, target, [&](State &state) {
            try {
                dispatch(state, target);
            } catch (...) {
                this->gp = state.gp;
                this->flags = state.flags;
                throw;
            }
        });
    }

    void determine_draw() {
        auto &gp = this->gp;
        auto &flags = this->flags;
        auto &xmm = this->s.xmm;
        gp[eax] = this->read(0x1775e4u, gp[esp] + 0x14u, 4);
        gp[ecx] = flags.logic(0, 4);
        gp[ecx] = this->read(0x1775eau, gp[eax] + 0x16u, 2);
        gp[ecx] = flags.right(gp[ecx], 9u, 4, false);
        flags.logic(gp[ecx] & 1u, 1);
        gp[ecx] = gp[esi] + 0x20u;
        if (flags.zf) {
            this->reached(query_cluster);
            const auto cell = this->read(0x1775f9u, 0x556e34u + gp[edx], 1);
            gp[eax] = static_cast<std::uint32_t>(static_cast<std::int8_t>(cell));
            gp[edi] = this->read(0x177600u, 0x556e18u, 4);
            xmm[7] = _mm_set_ss(physics_step_detail::f(this->read(0x177606u, gp[esi] + 0x2cu, 4)));
            gp[edx] = gp[esp] + 0x13u;
            this->push(0x17760fu, gp[edx]);
            this->push(0x177610u, gp[ecx]);
            this->store(0x177611u, gp[esp] + 0x1bu, 1, 0u);
            routed(0x177616u, 0x17761bu, 0x167ef0u);
            xmm[7] = _mm_set_ss(physics_step_detail::f(this->read(0x17761bu, gp[esp] + 0x18u, 4)));
            set_low_byte(gp[ebx], gp[eax]);
        } else {
            this->reached(query_view);
            gp[eax] = this->read(0x177625u, 0x556e18u, 4);
            xmm[1] = _mm_set_ss(physics_step_detail::f(this->read(0x17762au, gp[esi] + 0x2cu, 4)));
            gp[eax] = flags.plus(gp[eax], 0x94u, 4);
            routed(0x177634u, 0x177639u, 0x184690u);
            this->test32(gp[eax]);
            set_low_byte(gp[ebx], !flags.zf);
        }
    }

    void finish(Branch branch, Resume next, std::uint32_t pc) {
        this->reached(branch);
        continuation = next;
        this->publish(pc);
    }

    void update_visibility() {
        const bool visible = visible_cluster();
        this->test8(this->gp[eax]);
        if (!this->flags.zf) {
            const auto address = this->gp[ebp] + 0xdu;
            const auto byte = this->read(0x1775cfu, address, 1);
            this->store(0x1775cfu, address, 1, this->flags.logic(byte | 4u, 1));
            finish(mark_visible, Resume::draw, 0x1775d3u);
            return;
        }
        if (!visible) {
            finish(skip_by_cluster, Resume::copy, 0x1775c9u);
            return;
        }
        if (!read_view_test(0x1775d9u)) {
            determine_draw();
            this->test8(this->gp[ebx]);
            if (this->flags.zf) {
                finish(skip_by_query, Resume::copy, 0x177640u);
                return;
            }
            set_low_byte(this->gp[eax], this->read(0x177646u, this->gp[esp] + 0x12u, 1));
        }
        const auto address = this->gp[ebp] + 0xdu;
        const auto byte = this->read(0x17764au, address, 1);
        this->store(0x17764au, address, 1, this->flags.logic(byte & 0xfbu, 1));
        finish(draw_unmarked, Resume::draw, 0x17764au);
    }

    void run() {
        try {
            update_visibility();
        } catch (...) {
            this->s.gp = this->gp;
            this->s.flags = this->flags;
            throw;
        }
    }
};

template<class State, class Reference, class Dispatch>
Resume bridge(State &state, Reference reference, Dispatch dispatch) {
    static const bool reference_mode = flag("HALO2_REFERENCE_PARTICLE_DRAW_VISIBILITY");
    static const bool verify_mode = flag("HALO2_VERIFY_PARTICLE_DRAW_VISIBILITY");
    static VerifySummary summary{"Particle draw visibility"};
    auto continuation = Resume::retained;
    const auto verify_reference = [&](State &ref) {
        if (reference(ref) != continuation) throw std::runtime_error("Particle draw visibility continuation differed");
    };
    run_owner(state, 0x177594u, 0x17764au, reference_mode, verify_mode, coverage(), summary, verify_reference,
        [&](State &value, std::uint32_t *cursor, Trace<State> *trace) {
            return Frame<State, Dispatch>{value, cursor, trace, dispatch, continuation};
        });
    return continuation;
}
} // namespace halo2::native_particle_draw_visibility_detail
