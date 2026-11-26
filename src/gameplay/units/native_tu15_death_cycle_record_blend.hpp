#pragma once

#include "gameplay/units/native_tu15_death_cycle_expiration.hpp"
#include "gameplay/units/native_tu15_death_cycle_motion.hpp"

namespace halo2::native_death_cycle_record_blend {

using namespace native_death_cycle_opening;
using native_death_cycle_expiration::subtract;
using native_death_cycle_motion::update_field;
using native_tu15_leaves::ordered_mul_ss;
using native_world_leaves_detail::ordered_add_ss;

inline bool reference() {
    static const bool enabled = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_DEATH_CYCLE_RECORD_BLEND");
        return value && *value;
    }();
    return enabled;
}

template<class State>
struct RecordBlendFrame : Frame<State> {
    using Base = Frame<State>;
    using Base::s, Base::g, Base::flags, Base::site, Base::access;
    using Base::read, Base::store, Base::push, Base::field, Base::constant;
    decltype(State::xmm) &x = s.xmm;

    explicit RecordBlendFrame(State &state) : Base{state} {}

    auto record(std::uint32_t offset) { return field(esi, offset); }
    auto stack(std::uint32_t offset) { return field(esp, offset); }

    template<class Address>
    __m128 scalar(std::uint32_t pc, Address address) {
        return _mm_set_ss(f(read(pc, address)));
    }

    void store_blend(std::uint32_t pc, unsigned lane) {
        store(pc, record(0x10u), [&] { return u(_mm_cvtss_f32(x[lane])); });
    }

    bool above() const { return !flags.cf && !flags.zf; }

    void mark_pending() {
        set_low_byte(g[eax], read(0x104371u, record(1u), 1));
        site(0x104374u);
        flags.logic(low_byte(g[eax]), 1);
        site(0x104376u);
        if (flags.zf) {
            flags.minus(read(0x104378u, record(2u), 2), 0u, 2);
            site(0x10437du);
            if (flags.zf || flags.sf != flags.of) return;
        }
        store(0x10437fu, stack(0x13u), constant(1u), 1);
    }

    bool rising() {
        set_low_byte(g[eax], read(0x104384u, stack(0x15u), 1));
        site(0x104388u);
        flags.logic(low_byte(g[eax]), 1);
        site(0x10438au);
        return !flags.zf;
    }

    bool increase() {
        x[0] = scalar(0x104390u, field(ebp, 0xd4u));
        g[eax] = read(0x104398u, constant(0x51df54u));
        x[0] = ordered_mul_ss(x[0], scalar(0x10439du, field(eax, 4u)));
        x[0] = ordered_add_ss(x[0], scalar(0x1043a2u, record(0x10u)));
        x[1] = scalar(0x1043a7u, constant(0x46a0d0u));
        site(0x1043afu);
        flags.scalar_compare(x[0], x[1], false);
        store_blend(0x1043b2u, 0);
        site(0x1043b7u);
        if (above()) store_blend(0x1043b9u, 1);

        x[0] = scalar(0x1043beu, field(ebp, 0x18u));
        flags.scalar_compare(x[0], scalar(0x1043c3u, constant(0x46a0e8u)), true);
        site(0x1043cau);
        s.set(eax, flags.bits() | 2u, 1, 8);
        site(0x1043cbu);
        flags.logic((g[eax] >> 8u) & 0x44u, 1);
        site(0x1043ceu);
        if (!flags.pf) return false;
        flags.logic(read(0x1043d4u, record(4u), 1) & 8u, 1);
        site(0x1043d8u);
        if (!flags.zf) return false;
        x[0] = scalar(0x1043deu, record(0x10u));
        flags.scalar_compare(x[0], scalar(0x1043e3u, field(ebp, 0x18u)), false);
        site(0x1043e7u);
        return above();
    }

    bool decrease() {
        x[0] = scalar(0x10444du, record(0x10u));
        site(0x104452u);
        x[1] = _mm_setzero_ps();
        site(0x104455u);
        flags.scalar_compare(x[0], x[1], false);
        site(0x104458u);
        if (above()) {
            x[2] = scalar(0x10445au, field(ebp, 0xd8u));
            g[eax] = read(0x104462u, constant(0x51df54u));
            x[2] = ordered_mul_ss(x[2], scalar(0x104467u, field(eax, 4u)));
            site(0x10446cu);
            x[0] = subtract(x[0], x[2]);
            site(0x104470u);
            flags.scalar_compare(x[1], x[0], false);
            store_blend(0x104473u, 0);
            site(0x104478u);
            if (above()) store_blend(0x10447au, 1);
            store(0x10447fu, stack(0x13u), constant(1u), 1);
        }
        flags.logic(read(0x104484u, record(4u), 1) & 8u, 1);
        site(0x104488u);
        if (flags.zf) return false;
        x[0] = scalar(0x10448au, field(ebp, 0x18u));
        flags.scalar_compare(x[0], scalar(0x10448fu, record(0x10u)), false);
        site(0x104493u);
        return above();
    }

    template<std::uint32_t Delta>
    void resolve_notification_datums() {
        g[ecx] = read(0x1043edu + Delta, constant(0x4ecbd0u));
        g[edx] = read(0x1043f3u + Delta, field(ecx, 0x44u));
        g[eax] = read(0x1043f6u + Delta, stack(0x24u));
        g[ecx] = read(0x1043fau + Delta, [&] { return g[eax] + g[edx] + 8u; });
        g[edi] = read(0x1043feu + Delta, constant(0x4f0414u));
        set_low_byte(g[ebx], read(0x104404u + Delta, [&] { return g[eax] + g[edx] + 2u; }, 1));
        site(0x104408u + Delta);
        g[eax] = flags.plus(g[eax], g[edx], 4);
        g[edx] = read(0x10440au + Delta, field(ecx));
        site(0x10440cu + Delta);
        g[edx] = flags.logic(g[edx] & 0xffffu, 4);
        site(0x104412u + Delta);
        g[edx] = flags.shift(g[edx], 4u, 4);
        site(0x104415u + Delta);
        flags.logic(g[ebx] & 0x10u, 1);
        g[edi] = read(0x104418u + Delta, [&] { return g[edx] + g[edi] + 8u; });
        g[edx] = read(0x10441cu + Delta, stack(0x4cu));
        site(0x104420u + Delta);
        if (flags.zf) {
            flags.logic(read(0x104422u + Delta, field(ecx, 4u), 1) & 1u, 1);
            site(0x104426u + Delta);
            if (flags.zf) {
                flags.minus(read(0x104428u + Delta, field(edi, 0x38u)), 0xffffffffu, 4);
                site(0x10442cu + Delta);
                if (!flags.zf) return;
            }
        }
        g[ecx] = read(0x10442eu + Delta, field(ecx, 0x14u));
        site(0x104431u + Delta);
        flags.minus(g[ecx], 0xffffffffu, 4);
        site(0x104434u + Delta);
        if (!flags.zf) {
            site(0x104436u + Delta);
            g[edx] = g[ecx];
        }
    }

    template<bool Start>
    void finish_notification() {
        constexpr auto pc = Start ? 0x104444u : 0x1044ecu;
        access(pc);
        update_field(s, pc, g[esi] + 4u, 1, Start ? 0xffu : 0xf7u, Start ? 8u : 0u);
        if constexpr (Start) site(0x104448u);
        g[edi] = read(0x1044f0u, stack(0x28u));
    }

    template<bool Start, class Dispatch>
    void notify(Dispatch &dispatch) {
        // Both original paths share the same byte layout through CALL.
        constexpr auto delta = Start ? 0u : 0xa8u;
        resolve_notification_datums<delta>();
        push(0x104438u + delta, constant(Start ? 0xffffffffu : 0u));
        push(0x10443au + delta, constant(0u));
        push(0x10443cu + delta, constant(0xffffffffu));
        push(0x10443eu + delta, field(edx));
        push(0x10443fu + delta, constant(0x104444u + delta));
        dispatch(s, 0xbc820u, 0x10443fu + delta);
        finish_notification<Start>();
    }
};

template<class State, class Dispatch>
bool run(State &s, Dispatch dispatch) {
    if (!selected(s) || reference()) return false;
    RecordBlendFrame<State> frame{s};
    frame.mark_pending();
    if (frame.rising()) {
        if (frame.increase()) frame.template notify<true>(dispatch);
    } else if (frame.decrease()) {
        frame.template notify<false>(dispatch);
    }
    return true;
}

} // namespace halo2::native_death_cycle_record_blend
