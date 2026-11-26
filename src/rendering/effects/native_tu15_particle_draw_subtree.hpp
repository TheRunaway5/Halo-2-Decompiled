#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"

namespace halo2::native_particle_draw_subtree_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::flag;
using native_small_queries_detail::run_owner;

enum Branch : unsigned { empty, inherited_visibility, clear_visibility, another_child, branch_count };
inline std::atomic<std::uint64_t> &coverage() {
    static std::atomic<std::uint64_t> value{};
    return value;
}

template<class State, class Draw>
struct Frame : Port<State> {
    Draw &draw;

    Frame(State &state, std::uint32_t *cursor, Trace<State> *trace, Draw &descendant)
        : Port<State>(state, cursor, trace, {}), draw(descendant) {}

    void link_child() {
        auto &gp = this->gp;
        auto &flags = this->flags;
        gp[ecx] = this->read(0x1777d0u, 0x51df74u, 4);
        gp[edi] = this->read(0x1777d6u, gp[ecx] + 0x44u, 4);
        gp[esi] = flags.product(flags.logic(gp[eax] & 0xffffu, 4), 0x54u);
        gp[ecx] = 0x400u;
        gp[esi] = flags.plus(gp[esi], gp[edi], 4);
        const auto parent_flags = this->read(0x1777ebu, gp[ebp] + 0xcu, 2);
        flags.logic(parent_flags & gp[ecx], 2);
        if (flags.zf) {
            this->reached(clear_visibility);
            const auto address = gp[esi] + 0xdu;
            const auto value = this->read(0x1777f7u, address, 1);
            this->store(0x1777f7u, address, 1, flags.logic(value & 0xfbu, 1));
        } else {
            this->reached(inherited_visibility);
            const auto address = gp[esi] + 0xcu;
            const auto value = this->read(0x1777f1u, address, 2);
            this->store(0x1777f1u, address, 2, flags.logic(value | gp[ecx], 2));
        }
    }

    void draw_child() {
        auto &gp = this->gp;
        gp[edx] = this->read(0x1777fbu, gp[esp] + 0x80u, 4);
        gp[ecx] = this->read(0x177802u, gp[esp] + 0x7cu, 4);
        this->push(0x177806u, gp[edx]);
        gp[edx] = this->read(0x177807u, gp[esp] + 0x7cu, 4);
        this->push(0x17780bu, gp[ecx]);
        this->push(0x17780cu, gp[edx]);
        this->push(0x17780du, gp[eax]);
        // Direct recursion uses the same live draw owner and the original
        // caller packet. It can alter the child's link before we reload it.
        this->call(0x17780eu, 0x177813u, 0x1772c0u, [&](State &state) {
            try {
                draw(state);
            } catch (...) {
                gp = state.gp;
                this->flags = state.flags;
                throw;
            }
        });
    }

    void visit_children() {
        auto &gp = this->gp;
        gp[eax] = this->read(0x1777c4u, gp[ebp] + 0x40u, 4);
        this->compare32(gp[eax], 0xffffffffu);
        if (this->flags.zf) this->reached(empty);
        while (!this->flags.zf) {
            link_child();
            draw_child();
            gp[eax] = this->read(0x177813u, gp[esi] + 0x38u, 4);
            this->compare32(gp[eax], 0xffffffffu);
            if (!this->flags.zf) this->reached(another_child);
        }
        gp[edi] = this->pop(0x17781bu);
        gp[esi] = this->pop(0x17781cu);
        gp[ebp] = this->pop(0x17781du);
        gp[esp] = this->flags.plus(gp[esp], 0x64u, 4);
        this->ret(0x177821u, 16u);
        this->publish(0x177821u);
    }

    void run() {
        try {
            visit_children();
        } catch (...) {
            this->s.gp = this->gp;
            this->s.flags = this->flags;
            throw;
        }
    }
};

template<class State, class Reference, class Draw>
bool bridge(State &state, Reference reference, Draw draw) {
    static const bool reference_mode = flag("HALO2_REFERENCE_PARTICLE_DRAW_SUBTREE");
    static const bool verify_mode = flag("HALO2_VERIFY_PARTICLE_DRAW_SUBTREE");
    static VerifySummary summary{"Particle draw subtree"};
    return run_owner(state, 0x1777c4u, 0x177821u, reference_mode, verify_mode, coverage(), summary, reference,
        [&](State &value, std::uint32_t *cursor, Trace<State> *trace) {
            return Frame<State, Draw>{value, cursor, trace, draw};
        });
}
} // namespace halo2::native_particle_draw_subtree_detail
