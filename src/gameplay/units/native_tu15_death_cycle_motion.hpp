#pragma once

#include "gameplay/units/native_tu15_death_cycle_opening.hpp"

#include <bit>

namespace halo2::native_death_cycle_motion {

using namespace native_death_cycle_opening;

inline bool reference() {
    static const bool enabled = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_DEATH_CYCLE_MOTION");
        return value && *value;
    }();
    return enabled;
}

template<class State>
void update_field(State &s, std::uint32_t pc, std::uint32_t address, unsigned width,
                  std::uint32_t mask, std::uint32_t bits) {
    const auto previous = r(s.memory, address, width);
    auto flags = s.flags;
    const auto value = flags.logic((previous & mask) | bits, width);
    if (s.observe) {
        const SourcePhysicsContactRecordObserver committed = [&](const SourcePhysicsContactRecordStore &event) {
            s.flags = flags;
            s.observe(event);
        };
        w(s.memory, committed, pc, address, width, value);
    } else {
        w(s.memory, {}, pc, address, width, value);
        s.flags = flags;
    }
}

template<class State>
struct MotionFrame : Frame<State> {
    using Base = Frame<State>;
    using Base::s, Base::g, Base::flags, Base::site, Base::access;
    using Base::read, Base::store, Base::push, Base::field, Base::constant;

    explicit MotionFrame(State &state) : Base{state} {}

    auto stack(std::uint32_t offset) { return field(esp, offset); }
    auto status() { return [this] { return g[ebp]+g[ebx]+0x210u; }; }

    template<class Dispatch>
    bool accepted(Dispatch dispatch) {
        set_low_byte(g[eax], read(0x1037e7u, stack(0x14u), 1));
        site(0x1037ebu);
        flags.logic(low_byte(g[eax]), 1);
        site(0x1037edu);
        if (flags.zf) {
            set_low_byte(g[eax], read(0x1037efu, stack(0x15u), 1));
            site(0x1037f3u);
            flags.logic(low_byte(g[eax]), 1);
            site(0x1037f5u);
            if (flags.zf) return false;
        }
        site(0x1037f7u);
        g[eax] = flags.logic(0u, 4);
        s.set(eax, read(0x1037f9u, field(esi, 8u), 2), 2, 0);
        site(0x1037fdu);
        flags.minus(g[eax] & 0xffffu, 0xffffu, 2);
        site(0x103801u);
        if (flags.zf) return false;

        push(0x103803u, constant(0u));
        push(0x103805u, field(eax));
        g[eax] = read(0x103806u, stack(0x54u));
        push(0x10380au, field(eax));
        push(0x10380bu, constant(0x103810u));
        dispatch(s, 0x107610u, 0x10380bu);
        site(0x103810u);
        flags.logic(low_byte(g[eax]), 1);
        site(0x103812u);
        return !flags.zf;
    }

    void mark_accepted() {
        g[edx] = read(0x103814u, status());
        g[ecx] = read(0x10381bu, constant(0x4ecbd0u));
        site(0x103821u);
        g[edx] = flags.logic(g[edx] | 0x20u, 4);
        store(0x103824u, status(), field(edx));
        const auto index = std::uint16_t(read(0x10382bu, field(esi, 8u), 2));
        g[eax] = std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(index)));
        g[edx] = read(0x10382fu, field(ecx, 0x44u));
        site(0x103832u);
        g[eax] = flags.product(g[eax], 0x34u);
        g[ecx] = read(0x103835u, stack(0x24u));
        g[edx] = read(0x103839u, [&] { return g[edx]+g[ecx]+8u; });
        access(0x10383du);
        update_field(s, 0x10383du, g[edx]+g[eax]+0x1a8u, 1, 0xffu, 1u);
        site(0x103845u);
        g[eax] = g[edx]+g[eax]+0x1a4u;
        site(0x10384cu);
    }

    void mark_rejected() {
        access(0x10384eu);
        update_field(s, 0x10384eu, g[ebp]+g[ebx]+0x210u, 4, 0xffffffdfu, 0u);
    }

    template<class Apply>
    void apply_motion(Apply apply) {
        set_low_byte(g[eax], read(0x103856u, stack(0x14u), 1));
        site(0x10385au);
        flags.logic(low_byte(g[eax]), 1);
        site(0x10385cu);
        if (!flags.zf) {
            set_low_byte(g[eax], read(0x10385eu, stack(0x15u), 1));
            site(0x103862u);
            flags.logic(low_byte(g[eax]), 1);
            site(0x103864u);
            if (flags.zf) return;
        }
        s.set(ecx, read(0x10386au, field(esi, 8u), 2), 2, 0);
        g[eax] = read(0x10386eu, stack(0x4cu));
        push(0x103872u, constant(0x103877u));
        apply(s);
        site(0x103877u);
    }
};

template<class State, class Dispatch, class Apply>
bool run(State &s, Dispatch dispatch, Apply apply) {
    if (!selected(s) || reference()) return false;
    MotionFrame<State> frame{s};
    if (frame.accepted(dispatch)) frame.mark_accepted();
    else frame.mark_rejected();
    frame.apply_motion(apply);
    return true;
}

} // namespace halo2::native_death_cycle_motion
