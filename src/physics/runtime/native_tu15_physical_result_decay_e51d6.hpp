#pragma once

#include "physics/runtime/native_tu15_physical_result_activity.hpp"

namespace halo2::native_physical_result_decay {
using namespace native_object_tick_detail::boundary;
using native_physical_result_activity::projection_difference;
using native_tu15_leaves::ordered_mul_ss;
using native_world_leaves_detail::ordered_add_ss;

enum class Entry { decay, blocked };

template<class State> class ReturnFrame {
    State &s;
    decltype(State::gp) &g;
    decltype(State::flags) &flags;

    void at(std::uint32_t pc) { s.instruction_literal(pc); }
    std::uint32_t object(std::uint32_t offset) const { return g[ebp] + offset; }
    std::uint32_t read(std::uint32_t pc, std::uint32_t address, unsigned width = 4) {
        at(pc);
        return physics_step_detail::r(s.memory, address, width);
    }
    __m128 scalar(std::uint32_t pc, std::uint32_t address) {
        return _mm_set_ss(physics_step_detail::f(read(pc, address)));
    }
    bool zero(std::uint32_t pc) {
        at(pc);
        return flags.zf;
    }
    void pop(std::uint32_t pc, unsigned reg) {
        at(pc);
        g[reg] = s.pop();
    }
    void set_result(std::uint32_t value) { s.set(eax, value, 1, 0); }

    void store_decay(std::uint32_t pc, __m128 value) {
        at(pc);
        physics_step_detail::w(s.memory, s.observe, pc, object(0x3a4u), 4,
                               physics_step_detail::u(_mm_cvtss_f32(value)));
    }

    void publish_decay() {
        store_decay(0xe529cu, s.xmm[0]);
        at(0xe52a4u);
        if (!flags.cf) store_decay(0xe52a6u, s.xmm[1]);
    }

    void increase_decay() {
        auto &x = s.xmm;
        x[0] = ordered_add_ss(x[0], scalar(0xe5291u, object(0x3a4u)));
        at(0xe5299u);
        flags.scalar_compare(x[0], x[1], false);
        publish_decay();
    }

    void move_toward_zero() {
        auto &x = s.xmm;
        x[0] = scalar(0xe524du, object(0x3a4u));
        at(0xe5255u);
        flags.scalar_compare(x[0], x[1], false);
        at(0xe5258u);
        if (!flags.cf && !flags.zf) {
            g[eax] = read(0xe525au, 0x51df54u);
            x[2] = scalar(0xe525fu, g[eax] + 4u);
            x[2] = ordered_mul_ss(x[2], scalar(0xe5264u, 0x46a10cu));
            at(0xe526cu);
            x[0] = projection_difference(x[0], x[2]);
            at(0xe5270u);
            flags.scalar_compare(x[1], x[0], false);
            at(0xe5273u);
            publish_decay();
            return;
        }
        flags.scalar_compare(x[1], scalar(0xe5275u, object(0x3a4u)), false);
        at(0xe527cu);
        if (flags.cf || flags.zf) return;
        g[ecx] = read(0xe527eu, 0x51df54u);
        x[0] = scalar(0xe5284u, g[ecx] + 4u);
        x[0] = ordered_mul_ss(x[0], scalar(0xe5289u, 0x46a10cu));
        increase_decay();
    }

    bool allow_directional_decay() {
        auto &x = s.xmm;
        x[0] = scalar(0xe51d6u, object(0x2c4u));
        at(0xe51deu);
        x[1] = _mm_setzero_ps();
        at(0xe51e1u);
        flags.scalar_compare(x[0], x[1], true);
        at(0xe51e4u);
        s.set(eax, flags.bits() | 2u, 1, 8);
        at(0xe51e5u);
        flags.logic(s.reg(eax, 1, 8) & 0x44u, 1);
        at(0xe51e8u);
        if (flags.pf) return false;
        g[eax] = read(0xe51eau, 0x4f3218u);
        flags.minus(read(0xe51efu, g[eax] + 8u), 2u, 4);
        return !zero(0xe51f3u);
    }

    bool directional_decay() {
        auto &x = s.xmm;
        g[eax] = read(0xe51f5u, object(0x148u));
        at(0xe51fbu);
        flags.logic(g[eax] & 0xffu, 1);
        at(0xe51fdu);
        if (flags.sf) {
            g[ecx] = read(0xe51ffu, 0x51df54u);
            x[1] = scalar(0xe5205u, g[ecx] + 4u);
            x[1] = ordered_mul_ss(x[1], scalar(0xe520au, 0x46a120u));
            x[0] = scalar(0xe5212u, object(0x3a4u));
            at(0xe521au);
            x[0] = projection_difference(x[0], x[1]);
            x[1] = scalar(0xe521eu, 0x46a0dcu);
            at(0xe5226u);
            flags.scalar_compare(x[1], x[0], false);
            at(0xe5229u);
            publish_decay();
            return true;
        }
        at(0xe522bu);
        flags.logic(s.reg(eax, 1, 8) & 1u, 1);
        if (zero(0xe522eu)) return false;
        g[edx] = read(0xe5230u, 0x51df54u);
        x[0] = scalar(0xe5236u, g[edx] + 4u);
        x[0] = ordered_mul_ss(x[0], scalar(0xe523bu, 0x46a120u));
        x[1] = scalar(0xe5243u, 0x46a0d0u);
        at(0xe524bu);
        increase_decay();
        return true;
    }

    void release(std::uint32_t adjust, std::uint32_t ret) {
        at(adjust);
        g[esp] = flags.plus(g[esp], 0xd0u, 4);
        at(ret);
        (void)s.pop();
        g[esp] += 12u;
    }

    void finish_tick() {
        if (!allow_directional_decay() || !directional_decay()) move_toward_zero();
        pop(0xe52aeu, edi);
        if (!s.template try_pop_registers<esi, ebp>(0xe52b0u)) {
            pop(0xe52afu, esi);
            pop(0xe52b0u, ebp);
        }
        at(0xe52b1u);
        set_result(1);
        pop(0xe52b3u, ebx);
        release(0xe52b4u, 0xe52bau);
    }

    void finish_blocked() {
        set_result(read(0xe52bdu, g[esp] + 0x13u, 1));
        if (!s.template try_pop_registers<edi, esi, ebp, ebx>(0xe52c4u)) {
            pop(0xe52c1u, edi);
            pop(0xe52c2u, esi);
            pop(0xe52c3u, ebp);
            pop(0xe52c4u, ebx);
        }
        release(0xe52c5u, 0xe52cbu);
    }

public:
    explicit ReturnFrame(State &state) : s(state), g(s.gp), flags(s.flags) {}

    void run(Entry entry) {
        if (entry == Entry::decay) finish_tick();
        else finish_blocked();
    }
};

template<class State> bool finish(State &s, Entry entry) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    static const bool reference = native_small_queries_detail::flag("HALO2_REFERENCE_PHYSICAL_RESULT_DECAY_E51D6");
    if (!reference && native_physical_result_activity::admitted(s)) {
        ReturnFrame<State>{s}.run(entry);
        return true;
    }
#endif
    return false;
}
}
