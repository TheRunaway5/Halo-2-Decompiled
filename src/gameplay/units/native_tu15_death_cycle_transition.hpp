#pragma once

#include "gameplay/units/native_tu15_death_cycle_opening.hpp"

namespace halo2::native_death_cycle_transition {

using namespace native_death_cycle_opening;

inline bool reference() {
    static const bool enabled = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_DEATH_CYCLE_TRANSITION");
        return value && *value;
    }();
    return enabled;
}

template<class State>
struct TransitionFrame : Frame<State> {
    using Base = Frame<State>;
    using Base::s, Base::g, Base::flags, Base::site;
    using Base::read, Base::store, Base::push, Base::field, Base::constant;

    explicit TransitionFrame(State &state) : Base{state} {}

    auto stack(std::uint32_t offset) { return field(esp, offset); }
    auto object(std::uint32_t offset) { return field(ebp, offset); }
    auto descriptor(std::uint32_t offset) { return field(esi, offset); }

    template<class Dispatch>
    void clear_stale_link(Dispatch &dispatch) {
        g[eax] = read(0x1031cbu, object(0x194u));
        site(0x1031d1u);
        flags.minus(g[eax], g[esi], 4);
        site(0x1031d3u);
        if (flags.zf) return;
        push(0x1031d5u, field(esi));
        push(0x1031d6u, constant(0x1031dbu));
        dispatch(s, 0xbcdf0u, 0x1031d6u);
        site(0x1031dbu);
        flags.logic(g[eax], 4);
        site(0x1031ddu);
        if (flags.zf) store(0x1031dfu, object(0x194u), field(esi));
    }

    template<class Check>
    bool refresh_activity_byte(Check &check) {
        push(0x1031e5u, field(ebx));
        push(0x1031e6u, constant(0x1031ebu));
        check(s);
        g[edx] = read(0x1031ebu, constant(0x4ecbd0u));
        g[ecx] = read(0x1031f1u, stack(0x24u));
        site(0x1031f5u);
        set_low_byte(g[ebx], g[eax]);
        g[eax] = read(0x1031f7u, field(edx, 0x44u));
        g[edx] = read(0x1031fau, [&] { return g[eax] + g[ecx] + 8u; });
        const auto displacement = read(0x1031feu, field(edx, 0x12au), 2);
        flags.minus(displacement, g[esi] & 0xffffu, 2);
        store(0x103205u, stack(0x13u), field(ebx), 1);
        site(0x103209u);
        return !flags.zf;
    }

    template<class Dispatch>
    void resolve_activity_tag(Dispatch &dispatch) {
        const auto displacement = std::uint16_t(read(0x10320fu, object(0x12au), 2));
        g[esi] = std::uint32_t(std::bit_cast<std::int16_t>(displacement));
        site(0x103216u);
        g[esi] = flags.plus(g[esi], g[ebp], 4);
        site(0x103218u);
        g[edi] = flags.logic(0u, 4);
        const auto reference = read(0x10321au, descriptor(6u), 2);
        flags.minus(reference, 0xffffffffu, 2);
        site(0x10321fu);
        if (flags.zf) return;
        g[eax] = read(0x103221u, descriptor(0u));
        g[ecx] = read(0x103223u, constant(0x4f0414u));
        site(0x103229u);
        g[eax] = flags.logic(g[eax] & 0xffffu, 4);
        site(0x10322eu);
        g[eax] = flags.shift(g[eax], 4u, 4);
        g[edx] = read(0x103231u, [&] { return g[eax] + g[ecx] + 8u; });
        g[eax] = read(0x103235u, descriptor(4u));
        push(0x103238u, field(eax));
        push(0x103239u, constant(0x10323eu));
        dispatch(s, 0x1e3150u, 0x103239u);
        site(0x10323eu);
        g[edi] = g[eax];
    }

    template<class Dispatch>
    void notify_transition(Dispatch &dispatch) {
        g[ecx] = read(0x103240u, stack(0x4cu));
        push(0x103244u, constant(0u));
        push(0x103246u, constant(0u));
        push(0x103248u, constant(0u));
        push(0x10324au, field(ecx));
        push(0x10324bu, constant(0xc1630u));
        push(0x103250u, constant(0x103255u));
        dispatch(s, 0x1d39c0u, 0x103250u);
    }

    template<class Dispatch, class Followup>
    void run_followup(Dispatch &dispatch, Followup &followup) {
        const auto activity_flags = read(0x103255u, descriptor(0x14u), 1);
        flags.logic(activity_flags & 1u, 1);
        g[eax] = read(0x103259u, descriptor(0x11u), 1);
        site(0x10325du);
        if (!flags.zf) {
            g[eax] = read(0x10325fu, stack(0x4cu));
            push(0x103263u, constant(0x103268u));
            dispatch(s, 0x10a810u, 0x103263u);
            site(0x103268u);
            return;
        }
        site(0x10326au);
        flags.logic(g[eax] & 2u, 1);
        site(0x10326cu);
        if (flags.zf) {
            site(0x10326eu);
            flags.logic(g[edi], 4);
            site(0x103270u);
            if (flags.zf) return;
            const auto count = read(0x103272u, field(edi, 0x14u), 2);
            flags.minus(count, 1u, 2);
            site(0x103277u);
            if (!flags.zf) return;
        }
        g[ecx] = read(0x103279u, stack(0x4cu));
        push(0x10327du, constant(0x103282u));
        followup(s);
    }

    void override_activity_byte() {
        const auto datum = read(0x103282u, descriptor(0u));
        flags.minus(datum, 0xffffffffu, 4);
        site(0x103285u);
        if (flags.zf) return;
        const auto link = read(0x103287u, descriptor(6u), 2);
        flags.minus(link, 0xffffffffu, 2);
        site(0x10328cu);
        if (flags.zf) return;
        const auto enabled = read(0x10328eu, descriptor(0x12u), 1);
        flags.logic(enabled & 1u, 1);
        site(0x103292u);
        if (flags.zf) return;
        const auto mask = read(0x103294u, descriptor(0x11u), 1);
        flags.logic(mask & 9u, 1);
        site(0x103298u);
        if (!flags.zf) return;
        const auto count = read(0x10329au, field(edi, 0x14u), 2);
        flags.minus(count, 1u, 2);
        site(0x10329fu);
        if (flags.zf || flags.sf != flags.of) return;
        site(0x1032a1u);
        set_low_byte(g[ebx], 1u);
        store(0x1032a3u, stack(0x13u), field(ebx), 1);
    }
};

template<class State, class Dispatch, class Check, class Followup>
bool run(State &s, Dispatch dispatch, Check check, Followup followup) {
    if (!selected(s) || reference()) return false;
    TransitionFrame<State> frame{s};
    frame.clear_stale_link(dispatch);
    if (frame.refresh_activity_byte(check)) {
        frame.resolve_activity_tag(dispatch);
        frame.notify_transition(dispatch);
        frame.run_followup(dispatch, followup);
        frame.override_activity_byte();
    }
    return true;
}

} // namespace halo2::native_death_cycle_transition
