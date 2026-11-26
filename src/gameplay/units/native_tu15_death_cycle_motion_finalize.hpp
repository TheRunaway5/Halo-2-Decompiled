#pragma once

#include "gameplay/units/native_tu15_death_cycle_motion.hpp"

namespace halo2::native_death_cycle_motion_finalize {

using namespace native_death_cycle_opening;
using native_death_cycle_motion::update_field;

enum class Entry { conditional_pair, clear_pair, apply_pair, reset_fields, check_motion };

inline bool reference() {
    static const bool enabled = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_DEATH_CYCLE_MOTION_FINALIZE");
        return value && *value;
    }();
    return enabled;
}

template<class State>
struct FinalizeFrame : Frame<State> {
    using Base = Frame<State>;
    using Base::s, Base::g, Base::flags, Base::site, Base::access;
    using Base::read, Base::store, Base::push, Base::field, Base::constant;

    explicit FinalizeFrame(State &state) : Base{state} {}

    auto stack(std::uint32_t offset) { return field(esp, offset); }
    auto record_flags() { return [this] { return g[ebx] + g[ebp] + 0x210u; }; }

    bool test_local(std::uint32_t input, std::uint32_t test, std::uint32_t branch, std::uint32_t offset) {
        set_low_byte(g[eax], read(input, stack(offset), 1));
        site(test);
        flags.logic(low_byte(g[eax]), 1);
        site(branch);
        return !flags.zf;
    }

    void clear_pair_flags() {
        g[eax] = read(0x103cc9u, record_flags());
        site(0x103cd0u);
        g[eax] = flags.logic(g[eax] & 0xffffffe7u, 4);
    }

    template<class Apply>
    void apply_record(Apply &apply, std::uint32_t load, std::uint32_t move, std::uint32_t call,
                      std::uint32_t resume, std::uint32_t offset) {
        s.set(ecx, read(load, field(esi, offset), 2), 2, 0);
        site(move);
        g[eax] = g[edi];
        push(call, constant(resume));
        apply(s);
    }

    template<class Apply>
    void apply_pair(Apply &apply) {
        store(0x103cd3u, record_flags(), field(eax));
        apply_record(apply, 0x103cdau, 0x103cdeu, 0x103ce0u, 0x103ce5u, 8u);
        apply_record(apply, 0x103ce5u, 0x103ce9u, 0x103cebu, 0x103cf0u, 0xau);
        site(0x103cf0u);
    }

    void reset_fields() {
        store(0x103cf5u, field(ebp, 0x171u), constant(0u), 1);
        store(0x103cfcu, field(ebp, 0x170u), constant(0u), 1);
        store(0x103d03u, field(ebp, 0x173u), constant(0u), 1);
        site(0x103d0au);
    }

    template<class Dispatch>
    bool apply_if_available(Dispatch &dispatch) {
        g[eax] = read(0x103d1fu, stack(0x28u));
        g[edi] = read(0x103d23u, stack(0x4cu));
        push(0x103d27u, field(eax));
        site(0x103d28u);
        g[eax] = g[edi];
        push(0x103d2au, constant(0x103d2fu));
        dispatch(s, 0x108b60u, 0x103d2au);
        site(0x103d2fu);
        flags.logic(low_byte(g[eax]), 1);
        site(0x103d31u);
        if (flags.zf) return false;
        if (test_local(0x103d33u, 0x103d37u, 0x103d39u, 0x15u)) {
            access(0x103d3bu);
            update_field(s, 0x103d3bu, g[ebx] + g[ebp] + 0x210u, 4, 0xffffffffu, 0x40u);
        }

        g[ecx] = read(0x103d43u, stack(0x28u));
        push(0x103d47u, field(ecx));
        site(0x103d48u);
        g[ecx] = g[edi];
        push(0x103d4au, constant(0x103d4fu));
        dispatch(s, 0x108db0u, 0x103d4au);
        site(0x103d4fu);
        return true;
    }

    template<class Dispatch, class Apply>
    void check_motion(Dispatch &dispatch, Apply &apply) {
        if (test_local(0x103d0fu, 0x103d13u, 0x103d15u, 0x14u) ||
            test_local(0x103d17u, 0x103d1bu, 0x103d1du, 0x15u)) {
            if (apply_if_available(dispatch)) return;
        } else {
            g[edi] = read(0x103d54u, stack(0x4cu));
        }
        apply_record(apply, 0x103d58u, 0x103d5cu, 0x103d5eu, 0x103d63u, 8u);
        site(0x103d63u);
    }
};

template<class State, class Dispatch, class Apply>
bool run(State &s, Entry entry, Dispatch dispatch, Apply apply) {
    if (!selected(s) || reference()) return false;
    FinalizeFrame<State> frame{s};
    switch (entry) {
    case Entry::conditional_pair:
        if (!frame.test_local(0x103cbdu, 0x103cc1u, 0x103cc3u, 0x1au)) break;
        [[fallthrough]];
    case Entry::clear_pair:
        frame.clear_pair_flags();
        [[fallthrough]];
    case Entry::apply_pair:
        frame.apply_pair(apply);
        break;
    case Entry::reset_fields:
        frame.reset_fields();
        break;
    case Entry::check_motion:
        frame.check_motion(dispatch, apply);
        break;
    }
    return true;
}

} // namespace halo2::native_death_cycle_motion_finalize
