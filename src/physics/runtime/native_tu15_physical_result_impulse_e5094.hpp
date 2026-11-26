#pragma once

#include "physics/runtime/native_tu15_physical_result_activity.hpp"

namespace halo2::native_physical_result_impulse {
using namespace native_object_tick_detail::boundary;
using native_tu15_leaves::ordered_mul_ss;
using native_world_leaves_detail::ordered_add_ss;

enum class Entry { apply, publish };

template<class State, class Route> class ImpulseFrame {
    State &s;
    Route &route;
    decltype(State::gp) &g;
    decltype(State::flags) &flags;

    void at(std::uint32_t pc) { s.instruction_literal(pc); }
    std::uint32_t stack(std::uint32_t offset) const { return g[esp] + offset; }
    std::uint32_t object(std::uint32_t offset) const { return g[ebp] + offset; }
    std::uint32_t read(std::uint32_t pc, std::uint32_t address) {
        at(pc);
        return physics_step_detail::r(s.memory, address);
    }
    __m128 scalar(std::uint32_t pc, std::uint32_t address) {
        return _mm_set_ss(physics_step_detail::f(read(pc, address)));
    }
    void store(std::uint32_t pc, std::uint32_t address, std::uint32_t value) {
        at(pc);
        physics_step_detail::w(s.memory, s.observe, pc, address, 4, value);
    }
    void store_scalar(std::uint32_t pc, std::uint32_t address, __m128 value) {
        store(pc, address, physics_step_detail::u(_mm_cvtss_f32(value)));
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

    void integrate() {
        auto &x = s.xmm;
        g[ecx] = read(0xe5094u, 0x51df54u);
        x[0] = scalar(0xe509au, g[ecx] + 4u);
        g[edx] = read(0xe509fu, 0x4ecbd0u);
        g[eax] = read(0xe50a5u, g[edx] + 0x44u);
        g[ecx] = read(0xe50a8u, stack(0x14u));
        g[edi] = read(0xe50acu, g[eax] + g[ecx] + 8u);
        g[ebx] = read(0xe50b0u, g[edi] + 4u);
        x[1] = scalar(0xe50b3u, stack(0x3cu));
        x[2] = scalar(0xe50b9u, stack(0x40u));
        at(0xe50bfu);
        x[1] = ordered_mul_ss(x[1], x[0]);
        x[1] = ordered_add_ss(x[1], scalar(0xe50c3u, object(0x64u)));
        at(0xe50c8u);
        x[2] = ordered_mul_ss(x[2], x[0]);
        x[2] = ordered_add_ss(x[2], scalar(0xe50ccu, object(0x68u)));
        at(0xe50d1u);
        g[ebx] = flags.right(g[ebx], 8u, 4, false);
        at(0xe50d4u);
        s.set(ebx, flags.logic(g[ebx] & 1u, 1), 1, 0);
        at(0xe50d7u);
        x[0] = ordered_mul_ss(x[0], x[3]);
        x[0] = ordered_add_ss(x[0], scalar(0xe50dbu, object(0x6cu)));
        store_scalar(0xe50e0u, stack(0x18u), x[1]);
        store_scalar(0xe50e6u, stack(0x1cu), x[2]);
        store_scalar(0xe50ecu, stack(0x20u), x[0]);
        if (zero(0xe50f2u)) return;
        push(0xe50f4u, g[esi]);
        call(0xe50f5u, 0xe50fau, 0xba7e0u);
        x[0] = scalar(0xe50fau, stack(0x20u));
        x[2] = scalar(0xe5100u, stack(0x1cu));
        x[1] = scalar(0xe5106u, stack(0x18u));
    }

    bool below(std::uint32_t compare, std::uint32_t branch, __m128 left, __m128 right) {
        at(compare);
        flags.scalar_compare(left, right, false);
        at(branch);
        return flags.cf;
    }

    bool in_bounds() {
        auto &x = s.xmm;
        flags.minus(read(0xe510cu, g[edi] + 0x14u), 0xffffffffu, 4);
        if (!zero(0xe5110u)) return true;
        x[4] = scalar(0xe5112u, 0x46aa04u);
        if (below(0xe511au, 0xe511du, x[1], x[4])) return false;
        x[3] = scalar(0xe511fu, 0x46a530u);
        return !(below(0xe5127u, 0xe512au, x[3], x[1]) || below(0xe512cu, 0xe512fu, x[2], x[4]) ||
                 below(0xe5131u, 0xe5134u, x[3], x[2]) || below(0xe5136u, 0xe5139u, x[0], x[4]) ||
                 below(0xe513bu, 0xe513eu, x[3], x[0]));
    }

    void update_position() {
        g[edx] = read(0xe5140u, stack(0x18u));
        g[eax] = read(0xe5144u, stack(0x1cu));
        g[ecx] = read(0xe5148u, stack(0x20u));
        at(0xe514cu);
        g[edi] = flags.plus(g[edi], 0x64u, 4);
        store(0xe514fu, g[edi], g[edx]);
        g[edx] = read(0xe5151u, 0x4ecbd0u);
        store(0xe5157u, g[edi] + 4u, g[eax]);
        store(0xe515au, g[edi] + 8u, g[ecx]);
        g[eax] = read(0xe515du, g[edx] + 0x44u);
        g[ecx] = read(0xe5160u, stack(0x14u));
        g[eax] = read(0xe5164u, g[eax] + g[ecx] + 8u);
        g[eax] = read(0xe5168u, g[eax] + 0xd4u);
        at(0xe516eu);
        flags.minus(g[eax], 0xffffffffu, 4);
        if (zero(0xe5171u)) return;
        push(0xe5173u, 2u);
        push(0xe5175u, g[eax]);
        call(0xe5176u, 0xe517bu, 0xb7e50u);
    }

    void publish_position() {
        push(0xe517bu, 0u);
        push(0xe517du, 0u);
        at(0xe517fu);
        g[eax] = g[esi];
        call(0xe5181u, 0xe5186u, 0x1cd620u);
        at(0xe5186u);
        flags.logic(g[ebx] & 0xffu, 1);
        if (!zero(0xe5188u)) {
            push(0xe518au, 0u);
            push(0xe518cu, g[esi]);
            call(0xe518du, 0xe5192u, 0xba630u);
        }
        at(0xe5192u);
        g[eax] = g[esi];
        call(0xe5194u, 0xe5199u, 0xbda50u);
        g[edx] = read(0xe5199u, 0x4ecbd0u);
        g[ebx] = read(0xe519fu, 0x474d04u);
        at(0xe51a5u);
        flags.logic(g[ebx], 4);
        g[eax] = read(0xe51a7u, g[edx] + 0x44u);
        g[ecx] = read(0xe51aau, stack(0x14u));
        g[eax] = read(0xe51aeu, g[eax] + g[ecx] + 8u);
        if (!zero(0xe51b2u)) {
            at(0xe51b4u);
            g[edx] = g[ebx];
            g[ecx] = read(0xe51b6u, g[edx]);
            at(0xe51b8u);
            g[eax] = flags.plus(g[eax], 0x88u, 4);
            store(0xe51bdu, g[eax], g[ecx]);
            g[ecx] = read(0xe51bfu, g[edx] + 4u);
            store(0xe51c2u, g[eax] + 4u, g[ecx]);
            g[edx] = read(0xe51c5u, g[edx] + 8u);
            store(0xe51c8u, g[eax] + 8u, g[edx]);
        }
        push(0xe51cbu, 0u);
    }

public:
    ImpulseFrame(State &state, Route &child) : s(state), route(child), g(s.gp), flags(s.flags) {}

    void run(Entry entry) {
        if (entry == Entry::apply) {
            integrate();
            if (in_bounds()) update_position();
            publish_position();
        }
        at(0xe51cdu);
        g[eax] = g[esi];
        push(0xe51cfu, 0u);
        call(0xe51d1u, 0xe51d6u, 0x1cd6a0u);
    }
};

template<class State, class Route> bool publish(State &s, Route route, Entry entry) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    static const bool reference = native_small_queries_detail::flag("HALO2_REFERENCE_PHYSICAL_RESULT_IMPULSE_E5094");
    if (!reference && native_physical_result_activity::admitted(s)) {
        ImpulseFrame<State, Route>{s, route}.run(entry);
        return true;
    }
#endif
    return false;
}
}
