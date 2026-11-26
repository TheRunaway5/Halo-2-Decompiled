#pragma once

#include "physics/runtime/native_tu15_physical_result_activity.hpp"

namespace halo2::native_physical_result_classification {
using namespace native_object_tick_detail::boundary;
using physics_step_detail::r;
using physics_step_detail::w;

template<class State> void increment_duration(State &s) {
    const auto address = s.gp[ebp] + 0x3a8u;
    const auto previous = r(s.memory, address, 1);
    auto flags = s.flags;
    const auto value = flags.increment(previous, 1, false);
    if (s.observe) {
        const SourcePhysicsContactRecordObserver after = [&](const SourcePhysicsContactRecordStore &store) {
            s.flags = flags;
            s.observe(store);
        };
        w(s.memory, after, 0xe4be4u, address, 1, value);
    } else {
        w(s.memory, {}, 0xe4be4u, address, 1, value);
        s.flags = flags;
    }
}

template<class State, class Route> class ClassificationFrame {
    State &s;
    Route &route;
    decltype(State::gp) &g;
    decltype(State::flags) &flags;

    void at(std::uint32_t pc) { s.instruction_literal(pc); }
    std::uint32_t stack(std::uint32_t offset) const { return g[esp] + offset; }
    std::uint32_t object(std::uint32_t offset) const { return g[ebp] + offset; }
    std::uint32_t al() const { return s.reg(eax, 1, 0); }
    void set_al(std::uint32_t value) { s.set(eax, value, 1, 0); }
    std::uint32_t read(std::uint32_t pc, std::uint32_t address, unsigned width = 4) {
        at(pc);
        return r(s.memory, address, width);
    }
    __m128 scalar(std::uint32_t pc, std::uint32_t address) {
        return _mm_set_ss(physics_step_detail::f(read(pc, address)));
    }
    void store(std::uint32_t pc, std::uint32_t address, std::uint32_t value, unsigned width = 4) {
        at(pc);
        w(s.memory, s.observe, pc, address, width, value);
    }
    void push(std::uint32_t pc, std::uint32_t value) {
        at(pc);
        native_two_bone_solve::push_word(s, pc, value);
    }
    void call(std::uint32_t pc, std::uint32_t resume, std::uint32_t target) {
        push(pc, resume);
        route(s, pc, target);
    }
    bool zero(std::uint32_t pc) {
        at(pc);
        return flags.zf;
    }

    bool has_variant() {
        g[eax] = read(0xe4b81u, g[esi], 1);
        at(0xe4b84u);
        flags.minus(g[eax], 4u, 4);
        if (zero(0xe4b87u)) return true;
        at(0xe4b89u);
        flags.minus(g[eax], 5u, 4);
        return zero(0xe4b8cu);
    }

    void read_variant() {
        if (!has_variant()) {
            g[esi] = read(0xe4bb7u, stack(0xe4u));
            return;
        }
        set_al(read(0xe4b8eu, stack(0x84u), 1));
        at(0xe4b95u);
        flags.logic(al(), 1);
        g[esi] = read(0xe4b97u, stack(0xe4u));
        if (zero(0xe4b9eu)) return;
        push(0xe4ba0u, 0u);
        push(0xe4ba2u, 0u);
        push(0xe4ba4u, g[esi]);
        at(0xe4ba5u);
        g[edx] = flags.logic(0u, 4);
        at(0xe4ba7u);
        g[ecx] = flags.logic(0u, 4);
        at(0xe4ba9u);
        g[eax] = stack(0x94u);
        call(0xe4bb0u, 0xe4bb5u, 0xb95d0u);
        at(0xe4bb5u);
    }

    bool within_duration_limit() {
        set_al(read(0xe4bd1u, object(0x3a9u), 1));
        at(0xe4bd7u);
        flags.logic(al(), 1);
        if (zero(0xe4bd9u)) {
            at(0xe4be4u);
            increment_duration(s);
        } else {
            store(0xe4bdbu, object(0x3a8u), 0u, 1);
            at(0xe4be2u);
        }
        g[eax] = read(0xe4beau, object(0x3a8u), 1);
        g[ecx] = read(0xe4bf1u, 0x51df54u);
        s.xmm[1] = scalar(0xe4bf7u, 0x46a178u);
        at(0xe4bffu);
        s.xmm[0] = _mm_cvtsi32_ss(s.xmm[0], int(g[eax]));
        s.xmm[0] = native_tu15_leaves::ordered_mul_ss(s.xmm[0], scalar(0xe4c03u, g[ecx] + 4u));
        at(0xe4c08u);
        flags.scalar_compare(s.xmm[1], s.xmm[0], false);
        at(0xe4c0bu);
        return !flags.cf && !flags.zf;
    }

    void restore_contact() {
        g[eax] = read(0xe4c0du, stack(0x28u));
        g[edx] = read(0xe4c11u, g[eax] + 0x7cu);
        push(0xe4c14u, 0x7000101u);
        store(0xe4c19u, g[edi], g[edx]);
        push(0xe4c1bu, 0x7000101u);
        at(0xe4c20u);
        g[edx] = 0x400000cu;
        at(0xe4c25u);
        g[esi] = stack(0x2cu);
        call(0xe4c29u, 0xe4c2eu, 0x1d3c20u);
        flags.minus(read(0xe4c2eu, stack(0x26u), 2), 0xffffffffu, 2);
        if (zero(0xe4c34u)) return;
        g[eax] = read(0xe4c36u, stack(0x24u));
        push(0xe4c3au, g[eax]);
        g[eax] = read(0xe4c3bu, stack(0x2cu));
        call(0xe4c3fu, 0xe4c44u, 0x1d6490u);
    }

    void update_duration() {
        g[ebx] = read(0xe4bbeu, g[edi]);
        at(0xe4bc0u);
        flags.minus(g[ebx], 0x400000cu, 4);
        if (zero(0xe4bc6u)) {
            flags.minus(read(0xe4bc8u, object(0x34au), 1), 1u, 1);
            if (zero(0xe4bcfu)) {
                if (!within_duration_limit()) return;
                restore_contact();
            }
        }
        g[esi] = read(0xe4c44u, stack(0xe4u));
    }

public:
    ClassificationFrame(State &state, Route &child) : s(state), route(child), g(s.gp), flags(s.flags) {}

    void run() {
        read_variant();
        update_duration();
        at(0xe4c4bu);
        g[ecx] = g[ebx];
        call(0xe4c4du, 0xe4c52u, 0xdf820u);
        store(0xe4c52u, object(0x3a9u), al(), 1);
    }
};

template<class State, class Route> bool classify(State &s, Route route) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    static const bool reference = native_small_queries_detail::flag("HALO2_REFERENCE_PHYSICAL_RESULT_CLASSIFICATION_E4B81");
    if (!reference && native_physical_result_activity::admitted(s)) {
        ClassificationFrame<State, Route>{s, route}.run();
        return true;
    }
#endif
    return false;
}
}
