#pragma once

#include "gameplay/units/native_tu15_death_cycle_motion_finalize.hpp"

namespace halo2::native_death_cycle_motion_cleanup {

using namespace native_death_cycle_opening;
using native_death_cycle_motion::update_field;

enum class Entry { active_record, pending_record };

inline bool reference() {
    static const bool enabled = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_DEATH_CYCLE_MOTION_CLEANUP");
        return value && *value;
    }();
    return enabled;
}

struct ClearSites {
    std::uint32_t header, rows, offset, object, state, counter;
};

struct EffectSites {
    std::uint32_t handle, compare, branch, argument, call, resume;
};

constexpr ClearSites active_clear{0x103dd6u, 0x103ddcu, 0x103ddfu, 0x103de3u, 0x103de7u, 0x103defu};
constexpr ClearSites pending_clear{0x103e86u, 0x103e8cu, 0x103e8fu, 0x103e93u, 0x103e97u, 0x103e9fu};
constexpr EffectSites active_effect{0x103df9u, 0x103e00u, 0x103e03u, 0x103e09u, 0x103e0bu, 0x103e10u};
constexpr EffectSites pending_effect{0x103ea9u, 0x103eb0u, 0x103eb3u, 0x103eb5u, 0x103eb7u, 0x103ebcu};

template<class State>
struct CleanupFrame : native_death_cycle_motion_finalize::FinalizeFrame<State> {
    using Base = native_death_cycle_motion_finalize::FinalizeFrame<State>;
    using Base::s, Base::g, Base::flags, Base::site, Base::access;
    using Base::read, Base::store, Base::push, Base::field, Base::constant;
    using Base::stack, Base::test_local;

    explicit CleanupFrame(State &state) : Base{state} {}

    auto motion(std::uint32_t offset) { return [this, offset] { return g[ebx] + g[ebp] + offset; }; }

    void clear_related(const ClearSites &pcs) {
        g[ecx] = read(pcs.header, constant(0x4ecbd0u));
        g[edx] = read(pcs.rows, field(ecx, 0x44u));
        g[eax] = read(pcs.offset, stack(0x24u));
        g[eax] = read(pcs.object, [&] { return g[edx] + g[eax] + 8u; });
        store(pcs.state, [&] { return g[ebx] + g[eax] + 0x20cu; }, constant(0u), 1);
        store(pcs.counter, [&] { return g[ebx] + g[eax] + 0x20eu; }, constant(0u), 2);
    }

    template<class Teardown>
    bool release_effect(const EffectSites &pcs, Teardown &teardown) {
        g[edi] = read(pcs.handle, motion(0x214u));
        site(pcs.compare);
        flags.minus(g[edi], 0xffffffffu, 4);
        site(pcs.branch);
        if (flags.zf) return false;
        push(pcs.argument, constant(1u));
        push(pcs.call, constant(pcs.resume));
        teardown(s);
        store(pcs.resume, motion(0x214u), constant(0xffffffffu));
        return true;
    }

    void mark_related() {
        s.set(esi, read(0x103da6u, field(esi, 8u), 2), 2, 0);
        site(0x103daau);
        flags.minus(g[esi] & 0xffffu, 0xffffu, 2);
        site(0x103daeu);
        if (flags.zf) return;
        g[edx] = read(0x103db0u, constant(0x4ecbd0u));
        g[eax] = read(0x103db6u, field(edx, 0x44u));
        g[edx] = read(0x103db9u, stack(0x24u));
        g[eax] = read(0x103dbdu, [&] { return g[eax] + g[edx] + 8u; });
        site(0x103dc1u);
        g[ecx] = std::uint32_t(std::bit_cast<std::int16_t>(std::uint16_t(g[esi])));
        site(0x103dc4u);
        g[ecx] = flags.product(g[ecx], 0x34u);
        access(0x103dc7u);
        update_field(s, 0x103dc7u, g[eax] + g[ecx] + 0x1a8u, 1, 0xffu, 1u);
        site(0x103dcfu);
        g[eax] += g[ecx] + 0x1a4u;
    }

    template<class Dispatch, class Teardown>
    void active(Dispatch &dispatch, Teardown &teardown) {
        flags.minus(read(0x103d68u, motion(0x20eu), 2), 0u, 2);
        site(0x103d71u);
        if (flags.zf) {
            g[ecx] = read(0x103e20u, stack(0x28u));
            g[eax] = read(0x103e24u, stack(0x4cu));
            push(0x103e28u, constant(0x103e2du));
            dispatch(s, 0x107e30u, 0x103e28u);
            site(0x103e2du);
            return;
        }
        if (test_local(0x103d77u, 0x103d7bu, 0x103d7du, 0x14u) ||
            test_local(0x103d83u, 0x103d87u, 0x103d89u, 0x15u)) return;

        g[eax] = read(0x103d8fu, motion(0x210u));
        site(0x103d96u);
        flags.logic(g[eax] & 0x40u, 1);
        site(0x103d98u);
        if (flags.zf) {
            mark_related();
        } else {
            site(0x103d9au);
            g[eax] = flags.logic(g[eax] & 0xffffffbfu, 4);
            store(0x103d9du, motion(0x210u), field(eax));
            site(0x103da4u);
        }
        clear_related(active_clear);
        if (release_effect(active_effect, teardown)) site(0x103e1bu);
    }

    template<class Dispatch, class Teardown>
    void pending(Dispatch &dispatch, Teardown &teardown) {
        flags.logic(read(0x103e32u, motion(0x210u), 1) & 0x40u, 1);
        site(0x103e3au);
        if (!flags.zf) {
            if (test_local(0x103e7au, 0x103e7eu, 0x103e80u, 0x15u)) return;
            clear_related(pending_clear);
            release_effect(pending_effect, teardown);
            access(0x103ec7u);
            update_field(s, 0x103ec7u, g[ebx] + g[ebp] + 0x210u, 4, 0xffffffbfu, 0u);
            site(0x103ecfu);
            return;
        }
        if (!test_local(0x103e3cu, 0x103e40u, 0x103e42u, 0x14u) &&
            !test_local(0x103e44u, 0x103e48u, 0x103e4au, 0x16u)) {
            g[edx] = read(0x103e4cu, stack(0x28u));
            g[eax] = read(0x103e50u, stack(0x4cu));
            push(0x103e54u, constant(0x103e59u));
            dispatch(s, 0x107ee0u, 0x103e54u);
        }
        flags.minus(read(0x103e59u, motion(0x20eu), 2), 0u, 2);
        site(0x103e62u);
        if (!flags.zf) return;
        g[edx] = read(0x103e68u, stack(0x28u));
        g[eax] = read(0x103e6cu, stack(0x4cu));
        push(0x103e70u, constant(0x103e75u));
        dispatch(s, 0x107fe0u, 0x103e70u);
        site(0x103e75u);
    }
};

template<class State, class Dispatch, class Teardown>
bool run(State &s, Entry entry, Dispatch dispatch, Teardown teardown) {
    if (!selected(s) || reference()) return false;
    CleanupFrame<State> frame{s};
    if (entry == Entry::active_record) frame.active(dispatch, teardown);
    else frame.pending(dispatch, teardown);
    return true;
}

} // namespace halo2::native_death_cycle_motion_cleanup
