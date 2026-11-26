#pragma once

#include "physics/runtime/native_tu15_physical_result_activity.hpp"

namespace halo2::native_physical_result_notify {
using namespace native_object_tick_detail::boundary;

template<class State, class Route> class NotificationFrame {
    State &s;
    Route &route;
    decltype(State::gp) &g;
    decltype(State::flags) &flags;

    void at(std::uint32_t pc) { s.instruction_literal(pc); }
    std::uint32_t object(std::uint32_t offset) const { return g[ebp] + offset; }
    std::uint32_t read(std::uint32_t pc, std::uint32_t address, unsigned width = 4) {
        at(pc);
        return physics_step_detail::r(s.memory, address, width);
    }
    void call(std::uint32_t pc, std::uint32_t resume, std::uint32_t target) {
        at(pc);
        native_two_bone_solve::push_word(s, pc, resume);
        route(s, pc, target);
    }
    bool zero(std::uint32_t pc) {
        at(pc);
        return flags.zf;
    }

    bool entered_contact() {
        const auto offset = std::bit_cast<std::int16_t>(std::uint16_t(read(0xe4d85u, object(0x346u), 2)));
        g[eax] = std::uint32_t(std::int32_t(offset));
        g[ecx] = read(0xe4d8cu, object(0x2cu) + g[eax]);
        at(0xe4d90u);
        g[eax] = flags.plus(g[eax], g[ebp], 4);
        at(0xe4d92u);
        flags.minus(g[ecx], g[edi], 4);
        if (zero(0xe4d94u)) return true;
        s.set(edx, read(0xe4d96u, g[eax] + 0x32u, 1), 1, 0);
        flags.minus(s.reg(edx, 1, 0), read(0xe4d99u, g[eax] + 0x33u, 1), 1);
        at(0xe4d9cu);
        return !flags.zf && flags.sf == flags.of;
    }

    void notify_contact() {
        at(0xe4d9eu);
        g[eax] = g[esi];
        call(0xe4da0u, 0xe4da5u, 0xe3310u);
        at(0xe4da5u);
        flags.logic(s.reg(eax, 1, 0), 1);
        if (zero(0xe4da7u)) return;
        at(0xe4da9u);
        g[ebx] = g[esi];
        call(0xe4dabu, 0xe4db0u, 0xe31c0u);
        at(0xe4db0u);
    }

public:
    NotificationFrame(State &state, Route &child) : s(state), route(child), g(s.gp), flags(s.flags) {}

    void run() {
        flags.logic(read(0xe4d6du, g[esp] + 0x30u, 1) & 4u, 1);
        if (zero(0xe4d72u)) return;
        g[eax] = read(0xe4d74u, object(0x3dcu), 1);
        at(0xe4d7bu);
        g[eax] = flags.minus(g[eax], 4u, 4);
        if (zero(0xe4d7eu)) {
            at(0xe4db2u);
            g[eax] = g[esi];
            call(0xe4db4u, 0xe4db9u, 0xe94e0u);
        } else {
            at(0xe4d80u);
            g[eax] = flags.minus(g[eax], 2u, 4);
            if (zero(0xe4d83u)) {
                if (!entered_contact()) return;
                notify_contact();
            }
        }
        at(0xe4db9u);
        g[eax] = g[esi];
        call(0xe4dbbu, 0xe4dc0u, 0xe7e80u);
    }
};

template<class State, class Route> bool notify(State &s, Route route) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    static const bool reference = native_small_queries_detail::flag("HALO2_REFERENCE_PHYSICAL_RESULT_NOTIFY_E4D6D");
    if (!reference && native_physical_result_activity::admitted(s)) {
        NotificationFrame<State, Route>{s, route}.run();
        return true;
    }
#endif
    return false;
}
}
