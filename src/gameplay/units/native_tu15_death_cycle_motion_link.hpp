#pragma once

#include "gameplay/units/native_tu15_death_cycle_motion_finalize.hpp"

namespace halo2::native_death_cycle_motion_link {

using namespace native_death_cycle_opening;
using native_death_cycle_motion::update_field;

enum class Entry { select_left, update_left, select_right };

inline bool reference() {
    static const bool enabled = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_DEATH_CYCLE_MOTION_LINK");
        return value && *value;
    }();
    return enabled;
}

template<class State>
struct LinkFrame : native_death_cycle_motion_finalize::FinalizeFrame<State> {
    using Base = native_death_cycle_motion_finalize::FinalizeFrame<State>;
    using Base::s, Base::g, Base::flags, Base::site, Base::access;
    using Base::read, Base::store, Base::push, Base::field, Base::constant;
    using Base::stack, Base::test_local;

    explicit LinkFrame(State &state) : Base{state} {}

    auto motion(std::uint32_t offset) { return [this, offset] { return g[ebx] + g[ebp] + offset; }; }

    void clear_related() {
        store(0x10406au, [&] { return g[ebx] + g[eax] + 0x20cu; }, constant(0u), 1);
        store(0x104072u, [&] { return g[ebx] + g[eax] + 0x20eu; }, constant(0u), 2);
        site(0x10407cu);
    }

    void resolve_left() {
        g[ecx] = read(0x103f79u, constant(0x4ecbd0u));
        g[edx] = read(0x103f7fu, field(ecx, 0x44u));
        g[eax] = read(0x103f82u, stack(0x24u));
        g[eax] = read(0x103f86u, [&] { return g[edx] + g[eax] + 8u; });
        site(0x103f8au);
        clear_related();
    }

    template<class Dispatch>
    void bind_left(Dispatch &dispatch) {
        site(0x103f2du);
        g[eax] = flags.logic(0u, 4);
        s.set(eax, read(0x103f2fu, field(esi, 8u), 2), 2, 0);
        site(0x103f33u);
        flags.minus(g[eax] & 0xffffu, 0xffffu, 2);
        site(0x103f37u);
        if (!flags.zf) {
            g[ecx] = read(0x103f39u, stack(0x4cu));
            push(0x103f3du, constant(0u));
            push(0x103f3fu, field(eax));
            push(0x103f40u, field(ecx));
            push(0x103f41u, constant(0x103f46u));
            dispatch(s, 0x107610u, 0x103f41u);
            site(0x103f46u);
            flags.logic(low_byte(g[eax]), 1);
            site(0x103f48u);
            if (!flags.zf) {
                g[ecx] = std::uint32_t(std::bit_cast<std::int16_t>(std::uint16_t(read(0x103f4au, field(esi, 8u), 2))));
                g[edx] = read(0x103f4eu, constant(0x4ecbd0u));
                site(0x103f54u);
                g[ecx] = flags.product(g[ecx], 0x34u);
                g[eax] = read(0x103f57u, field(edx, 0x44u));
                g[edx] = read(0x103f5au, stack(0x24u));
                g[eax] = read(0x103f5eu, [&] { return g[eax] + g[edx] + 8u; });
                access(0x103f62u);
                update_field(s, 0x103f62u, g[eax] + g[ecx] + 0x1a8u, 1, 0xffu, 1u);
                site(0x103f6au);
                g[eax] += g[ecx] + 0x1a4u;
                access(0x103f71u);
                update_field(s, 0x103f71u, g[ebx] + g[ebp] + 0x210u, 4, 0xfffffffeu, 0u);
            }
        }
        resolve_left();
    }

    template<class Dispatch>
    void apply_left(Dispatch &dispatch) {
        g[esi] = read(0x103eefu, stack(0x4cu));
        site(0x103ef3u);
        g[ecx] = g[esp] + 0x1bu;
        push(0x103ef7u, field(ecx));
        site(0x103ef8u);
        g[edx] = g[esp] + 0x48u;
        push(0x103efcu, field(edx));
        site(0x103efdu);
        g[eax] = g[esi];
        push(0x103effu, constant(0x103f04u));
        dispatch(s, 0x10b330u, 0x103effu);
        site(0x103f04u);
        flags.logic(low_byte(g[eax]), 1);
        site(0x103f06u);
        if (flags.zf) return;
        g[eax] = read(0x103f0cu, stack(0x28u));
        push(0x103f10u, field(eax));
        site(0x103f11u);
        g[eax] = g[esi];
        push(0x103f13u, constant(0x103f18u));
        dispatch(s, 0x108d10u, 0x103f13u);
        site(0x103f18u);
    }

    template<class Dispatch>
    void select_left(Dispatch &dispatch) {
        if (test_local(0x103ed4u, 0x103ed8u, 0x103edau, 0x14u) ||
            test_local(0x103edcu, 0x103ee0u, 0x103ee2u, 0x16u)) {
            apply_left(dispatch);
            return;
        }
        site(0x103ee4u);
        flags.minus(low_byte(g[edx]), 7u, 1);
        site(0x103ee7u);
        if (flags.zf) {
            resolve_left();
        } else {
            site(0x103eedu);
            bind_left(dispatch);
        }
    }

    template<class Dispatch>
    void apply_motion_index(Dispatch &dispatch) {
        g[ecx] = read(0x1040e6u, stack(0x28u));
        push(0x1040eau, field(ecx));
        push(0x1040ebu, constant(0x1040f0u));
        dispatch(s, 0x108c40u, 0x1040ebu);
    }

    template<class Dispatch>
    void publish_left_link(Dispatch &dispatch) {
        g[esi] = read(0x103fa8u, stack(0x3cu));
        const auto current = read(0x103facu, field(ebp, 0x194u));
        flags.minus(current, g[esi], 4);
        site(0x103fb2u);
        if (!flags.zf) {
            g[edx] = read(0x103fdeu, stack(0x28u));
            push(0x103fe2u, field(edx));
            site(0x103fe3u);
            g[eax] = g[edi];
            push(0x103fe5u, constant(0x103feau));
            dispatch(s, 0x108d10u, 0x103fe5u);
            set_low_byte(g[eax], read(0x103feau, stack(0x17u), 1));
            store(0x103feeu, field(ebp, 0x194u), field(esi));
            store(0x103ff4u, field(ebp, 0x177u), field(eax), 1);
            site(0x103ffau);
            return;
        }

        set_low_byte(g[eax], read(0x103fb4u, stack(0x17u), 1));
        store(0x103fb8u, field(ebp, 0x177u), field(eax), 1);
        flags.minus(read(0x103fbeu, motion(0x20eu), 2), 0u, 2);
        site(0x103fc7u);
        if (!flags.zf) return;
        g[ecx] = read(0x103fcdu, stack(0x28u));
        push(0x103fd1u, field(ecx));
        site(0x103fd2u);
        g[eax] = g[edi];
        push(0x103fd4u, constant(0x103fd9u));
        dispatch(s, 0x108ca0u, 0x103fd4u);
        site(0x103fd9u);
    }

    template<class Dispatch>
    void update_left(Dispatch &dispatch) {
        if (!test_local(0x103f1du, 0x103f21u, 0x103f23u, 0x14u) &&
            !test_local(0x103f25u, 0x103f29u, 0x103f2bu, 0x16u)) {
            bind_left(dispatch);
            return;
        }
        g[edi] = read(0x103f8fu, stack(0x4cu));
        site(0x103f93u);
        g[ecx] = g[esp] + 0x17u;
        push(0x103f97u, field(ecx));
        site(0x103f98u);
        g[edx] = g[esp] + 0x40u;
        push(0x103f9cu, field(edx));
        site(0x103f9du);
        g[eax] = g[edi];
        push(0x103f9fu, constant(0x103fa4u));
        dispatch(s, 0x10b330u, 0x103f9fu);
        site(0x103fa4u);
        flags.logic(low_byte(g[eax]), 1);
        site(0x103fa6u);
        if (!flags.zf) {
            publish_left_link(dispatch);
        } else {
            site(0x103fffu);
            g[eax] = g[edi];
            site(0x104001u);
            apply_motion_index(dispatch);
        }
    }

    template<class Dispatch>
    void bind_right(Dispatch &dispatch) {
        site(0x104016u);
        g[eax] = flags.logic(0u, 4);
        s.set(eax, read(0x104018u, field(esi, 0xau), 2), 2, 0);
        site(0x10401cu);
        flags.minus(g[eax] & 0xffffu, 0xffffu, 2);
        site(0x104020u);
        if (!flags.zf) {
            g[edx] = read(0x104022u, stack(0x4cu));
            push(0x104026u, constant(0u));
            push(0x104028u, field(eax));
            push(0x104029u, field(edx));
            push(0x10402au, constant(0x10402fu));
            dispatch(s, 0x107610u, 0x10402au);
            site(0x10402fu);
            flags.logic(low_byte(g[eax]), 1);
            site(0x104031u);
            if (!flags.zf) {
                g[edx] = std::uint32_t(std::bit_cast<std::int16_t>(std::uint16_t(read(0x104033u, field(esi, 0xau), 2))));
                g[eax] = read(0x104037u, constant(0x4ecbd0u));
                site(0x10403cu);
                g[edx] = flags.product(g[edx], 0x34u);
                g[ecx] = read(0x10403fu, field(eax, 0x44u));
                g[eax] = read(0x104042u, stack(0x24u));
                g[ecx] = read(0x104046u, [&] { return g[ecx] + g[eax] + 8u; });
                access(0x10404au);
                update_field(s, 0x10404au, g[ecx] + g[edx] + 0x1a8u, 1, 0xffu, 1u);
                site(0x104052u);
                g[eax] = g[ecx] + g[edx] + 0x1a4u;
            }
        }
        g[edx] = read(0x104059u, constant(0x4ecbd0u));
        g[eax] = read(0x10405fu, field(edx, 0x44u));
        g[ecx] = read(0x104062u, stack(0x24u));
        g[eax] = read(0x104066u, [&] { return g[eax] + g[ecx] + 8u; });
        clear_related();
    }

    template<class Dispatch>
    void complete_right(Dispatch &dispatch) {
        flags.minus(read(0x1040d0u, motion(0x20eu), 2), 0u, 2);
        site(0x1040d9u);
        if (!flags.zf) return;
        flags.minus(read(0x1040dbu, stack(0x30u)), 0xffffffffu, 4);
        site(0x1040e0u);
        if (flags.zf) return;
        g[eax] = read(0x1040e2u, stack(0x4cu));
        apply_motion_index(dispatch);
    }

    template<class Dispatch>
    void apply_right(Dispatch &dispatch) {
        g[edx] = read(0x10407eu, constant(0x4ecbd0u));
        g[eax] = read(0x104084u, field(edx, 0x44u));
        g[ecx] = read(0x104087u, stack(0x24u));
        g[ecx] = read(0x10408bu, [&] { return g[eax] + g[ecx] + 8u; });
        set_low_byte(g[edx], read(0x10408fu, field(ecx, 0x12cu), 1));
        site(0x104095u);
        g[eax] = flags.logic(0xffffffffu, 4);
        site(0x104098u);
        flags.logic(g[edx] & 1u, 1);
        site(0x10409bu);
        if (!flags.zf) g[eax] = read(0x10409du, field(ecx, 0x154u));
        site(0x1040a3u);
        g[edx] = g[esp] + 0x1bu;
        push(0x1040a7u, field(edx));
        site(0x1040a8u);
        g[ecx] = g[esp] + 0x44u;
        push(0x1040acu, field(ecx));
        push(0x1040adu, constant(0x1040b2u));
        dispatch(s, 0x10b270u, 0x1040adu);
        site(0x1040b2u);
        flags.logic(low_byte(g[eax]), 1);
        site(0x1040b4u);
        if (!flags.zf) {
            g[edx] = read(0x1040b6u, field(ebp, 0x194u));
            const auto link = read(0x1040bcu, stack(0x40u));
            flags.minus(g[edx], link, 4);
            site(0x1040c0u);
            if (flags.zf) {
                g[eax] = read(0x1040c2u, stack(0x28u));
                push(0x1040c6u, field(eax));
                g[eax] = read(0x1040c7u, stack(0x50u));
                push(0x1040cbu, constant(0x1040d0u));
                dispatch(s, 0x108ca0u, 0x1040cbu);
            }
        }
        complete_right(dispatch);
    }

    template<class Dispatch>
    void select_right(Dispatch &dispatch) {
        if (test_local(0x104006u, 0x10400au, 0x10400cu, 0x14u) ||
            test_local(0x10400eu, 0x104012u, 0x104014u, 0x16u)) apply_right(dispatch);
        else bind_right(dispatch);
    }
};

template<class State, class Dispatch>
bool run(State &s, Entry entry, Dispatch dispatch) {
    if (!selected(s) || reference()) return false;
    LinkFrame<State> frame{s};
    switch (entry) {
    case Entry::select_left: frame.select_left(dispatch); break;
    case Entry::update_left: frame.update_left(dispatch); break;
    case Entry::select_right: frame.select_right(dispatch); break;
    }
    return true;
}

} // namespace halo2::native_death_cycle_motion_link
