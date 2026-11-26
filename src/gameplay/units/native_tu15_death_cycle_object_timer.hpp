#pragma once

#include "gameplay/units/native_tu15_death_cycle_expiration.hpp"
#include "gameplay/units/native_tu15_death_cycle_motion.hpp"

namespace halo2::native_death_cycle_object_timer {

using namespace native_death_cycle_opening;
using native_death_cycle_expiration::subtract;
using native_death_cycle_motion::update_field;
using native_tu15_leaves::ordered_mul_ss;
using native_world_leaves_detail::ordered_add_ss;

inline bool reference() {
    static const bool enabled = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_DEATH_CYCLE_OBJECT_TIMER");
        return value && *value;
    }();
    return enabled;
}

template<class State>
void decrement_counter(State &s) {
    constexpr std::uint32_t site = 0x1032b5u;
    const auto address = s.gp[ebp]+0x19eu;
    const auto value = r(s.memory, address, 2);
    auto flags = s.flags;
    const auto next = flags.increment(value, 2, true);
    if (s.observe) {
        const SourcePhysicsContactRecordObserver committed = [&](const auto &event) {
            s.flags = flags;
            s.observe(event);
        };
        w(s.memory, committed, site, address, 2, next);
    } else {
        w(s.memory, {}, site, address, 2, next);
        s.flags = flags;
    }
}

template<class State>
struct ObjectTimerFrame : Frame<State> {
    using Base = Frame<State>;
    using Base::s, Base::g, Base::flags, Base::site, Base::access;
    using Base::read, Base::store, Base::push, Base::field, Base::constant;
    decltype(State::xmm) &x = s.xmm;

    explicit ObjectTimerFrame(State &state) : Base{state} {}

    auto object(std::uint32_t offset) { return field(ebp, offset); }
    auto stack(std::uint32_t offset) { return field(esp, offset); }
    auto magnitude() { return [this] { return u(_mm_cvtss_f32(x[0])); }; }
    template<class Address>
    __m128 scalar(std::uint32_t pc, Address address) {
        return _mm_set_ss(f(read(pc, address)));
    }
    bool above() const { return !flags.cf && !flags.zf; }
    bool positive() const { return !flags.zf && flags.sf == flags.of; }

    void mark_pending(std::uint32_t pc) {
        store(pc, stack(0x13u), field(ebx), 1);
    }

    void reload_tag(std::uint32_t pc) {
        g[ecx] = read(pc, stack(0x2cu));
    }

    void mark_direction_notified() {
        access(0x1033b8u);
        update_field(s, 0x1033b8u, g[ebp]+0x16cu, 1, 0xffu, 0x10u);
    }

    void advance_counter() {
        set_low_byte(g[edx], read(0x1032a7u, object(0x16cu), 1));
        site(0x1032adu);
        set_low_byte(g[edx], flags.right(low_byte(g[edx]), 5u, 1, false));
        site(0x1032b0u);
        flags.logic(g[edx] & 1u, 1);
        site(0x1032b3u);
        if (!flags.zf) return;

        access(0x1032b5u);
        decrement_counter(s);
        const auto counter = read(0x1032bcu, object(0x19eu), 2);
        flags.minus(counter, 0u, 2);
        site(0x1032c4u);
        if (!positive()) {
            access(0x1032c6u);
            update_field(s, 0x1032c6u, g[ebp]+0x16cu, 1, 0xffu, 0x20u);
        }
        site(0x1032cdu);
        set_low_byte(g[ebx], 1u);
        mark_pending(0x1032cfu);
    }

    template<class Dispatch>
    void apply_parent_notification(Dispatch dispatch) {
        reload_tag(0x1032d3u);
        g[eax] = read(0x1032d7u, field(ecx, 0x12cu));
        site(0x1032ddu);
        g[eax] = flags.right(g[eax], 10u, 4, false);
        site(0x1032e0u);
        flags.logic(g[eax] & 1u, 1);
        site(0x1032e2u);
        if (flags.zf) return;

        const auto parent = read(0x1032e4u, object(0x14u));
        flags.minus(parent, 0xffffffffu, 4);
        site(0x1032e8u);
        if (!flags.zf) return;
        g[edx] = read(0x1032eau, object(4u));
        site(0x1032edu);
        g[edx] = flags.right(g[edx], 8u, 4, false);
        site(0x1032f0u);
        flags.logic(g[edx] & 1u, 1);
        site(0x1032f3u);
        if (flags.zf) return;
        g[edx] = read(0x1032f5u, stack(0x4cu));
        push(0x1032f9u, constant(0x1032feu));
        dispatch(s, 0xfc8d0u, 0x1032f9u);
        reload_tag(0x1032feu);
    }

    template<class Dispatch>
    void apply_object_notification(Dispatch dispatch) {
        set_low_byte(g[eax], read(0x103302u, object(0x16eu), 1));
        site(0x103308u);
        set_low_byte(g[eax], flags.right(low_byte(g[eax]), 7u, 1, false));
        site(0x10330bu);
        flags.logic(g[eax] & 1u, 1);
        site(0x10330du);
        if (flags.zf) return;
        g[ecx] = read(0x10330fu, stack(0x4cu));
        push(0x103313u, constant(0x103318u));
        dispatch(s, 0x10a910u, 0x103313u);
        reload_tag(0x103318u);
    }

    void initialize_target() {
        const auto mode = read(0x10331cu, object(0x16eu), 1);
        flags.logic(mode & 1u, 1);
        site(0x103323u);
        if (flags.zf) return;

        x[1] = scalar(0x103325u, object(0x190u));
        site(0x10332du);
        x[0] = _mm_setzero_ps();
        site(0x103330u);
        flags.scalar_compare(x[1], x[0], true);
        site(0x103333u);
        s.set(eax, flags.bits() | 2u, 1, 8);
        access(0x103334u);
        update_field(s, 0x103334u, g[ebp]+0x16cu, 1, 0xefu, 0u);
        site(0x10333bu);
        flags.logic((g[eax] >> 8u) & 0x44u, 1);
        site(0x10333eu);
        if (!flags.pf) x[0] = scalar(0x103340u, constant(0x46a0d0u));
        store(0x103348u, object(0x190u), magnitude());
    }

    bool select_direction() {
        x[0] = scalar(0x103350u, object(0x18cu));
        const auto target = scalar(0x103358u, object(0x190u));
        flags.scalar_compare(x[0], target, true);
        site(0x10335fu);
        s.set(eax, flags.bits() | 2u, 1, 8);
        site(0x103360u);
        flags.logic((g[eax] >> 8u) & 0x44u, 1);
        site(0x103363u);
        if (!flags.pf) return false;

        x[0] = scalar(0x103369u, object(0x190u));
        const auto current = scalar(0x103371u, object(0x18cu));
        flags.scalar_compare(x[0], current, false);
        site(0x103378u);
        if (above()) {
            site(0x10337au);
            set_low_byte(g[ebx], 1u);
            site(0x10337cu);
        } else {
            site(0x10337eu);
            set_low_byte(g[ebx], flags.logic(0u, 1));
        }
        return true;
    }

    template<class Dispatch>
    void notify_direction(Dispatch dispatch) {
        set_low_byte(g[edx], read(0x103380u, object(0x16cu), 1));
        site(0x103386u);
        set_low_byte(g[edx], flags.right(low_byte(g[edx]), 4u, 1, false));
        site(0x103389u);
        flags.logic(g[edx] & 1u, 1);
        site(0x10338cu);
        if (!flags.zf) return;

        site(0x10338eu);
        flags.logic(low_byte(g[ebx]), 1);
        site(0x103390u);
        if (!flags.zf) {
            g[eax] = read(0x103392u, field(ecx, 0x248u));
            site(0x103398u);
        } else {
            g[eax] = read(0x10339au, field(ecx, 0x250u));
        }
        site(0x1033a0u);
        flags.minus(g[eax], 0xffffffffu, 4);
        site(0x1033a3u);
        if (!flags.zf) {
            g[ecx] = read(0x1033a5u, stack(0x4cu));
            push(0x1033a9u, constant(0u));
            push(0x1033abu, constant(0u));
            push(0x1033adu, constant(0xffffffffu));
            push(0x1033afu, constant(0x1033b4u));
            dispatch(s, 0x108a70u, 0x1033afu);
            reload_tag(0x1033b4u);
        }
        mark_direction_notified();
    }

    void update_magnitude() {
        site(0x1033bfu);
        flags.logic(low_byte(g[ebx]), 1);
        site(0x1033c1u);
        bool clamp;
        if (!flags.zf) {
            x[0] = scalar(0x1033c3u, field(ecx, 0x2d8u));
            g[eax] = read(0x1033cbu, constant(0x51df54u));
            const auto step = scalar(0x1033d0u, field(eax, 4u));
            x[0] = ordered_mul_ss(x[0], step);
            const auto current = scalar(0x1033d5u, object(0x18cu));
            x[0] = ordered_add_ss(x[0], current);
            const auto target = scalar(0x1033ddu, object(0x190u));
            flags.scalar_compare(x[0], target, false);
            store(0x1033e4u, object(0x18cu), magnitude());
            site(0x1033ecu);
            clamp = above();
            if (clamp) site(0x1033eeu);
        } else {
            x[1] = scalar(0x1033f0u, field(ecx, 0x2dcu));
            g[edx] = read(0x1033f8u, constant(0x51df54u));
            const auto step = scalar(0x1033feu, field(edx, 4u));
            x[1] = ordered_mul_ss(x[1], step);
            x[0] = scalar(0x103403u, object(0x18cu));
            site(0x10340bu);
            x[0] = subtract(x[0], x[1]);
            const auto target = scalar(0x10340fu, object(0x190u));
            flags.scalar_compare(x[0], target, false);
            store(0x103416u, object(0x18cu), magnitude());
            site(0x10341eu);
            clamp = !above();
        }
        if (clamp) x[0] = scalar(0x103420u, object(0x190u));
        site(0x103428u);
        set_low_byte(g[ebx], 1u);
        store(0x10342au, object(0x18cu), magnitude());
        mark_pending(0x103432u);
    }
};

template<class State, class Dispatch>
bool run(State &s, Dispatch dispatch) {
    if (!selected(s) || reference()) return false;
    ObjectTimerFrame<State> frame{s};
    frame.advance_counter();
    frame.apply_parent_notification(dispatch);
    frame.apply_object_notification(dispatch);
    frame.initialize_target();
    if (frame.select_direction()) {
        frame.notify_direction(dispatch);
        frame.update_magnitude();
    }
    return true;
}

} // namespace halo2::native_death_cycle_object_timer
