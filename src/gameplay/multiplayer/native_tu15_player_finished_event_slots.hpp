#pragma once

#include "gameplay/multiplayer/native_tu15_player_finished_cold_frame.hpp"

namespace halo2::native_player_finished_cold_detail {

template<class State>
void increment_elapsed(State &s, std::uint32_t pc, std::uint32_t address) {
    std::array<std::byte, 4> before, after;
    const auto value = memory::r(s.memory, address, 4);
    auto next_flags = s.flags;
    const auto next = next_flags.increment(value, 4, false);
    for (unsigned i = 0; i < after.size(); ++i) {
        before[i] = std::byte(value >> (i * 8u));
        after[i] = std::byte(next >> (i * 8u));
    }
    s.memory.write(address, after);
    s.flags = next_flags;
    if (s.observe)
        s.observe({pc, address, {before.begin(), before.end()}, {after.begin(), after.end()}});
}

template<class State>
class EventFrame : public ColdFrame<State> {
    using Base = ColdFrame<State>;
    using Base::s;
    using Base::gp;
    using Base::flags;
    using Base::at;
    using Base::read;
    using Base::load;
    using Base::store;
    using Base::restore;
    using Base::save;
    using Base::invoke;
    using Base::leave;
    using Base::recheck;
    using Base::finish_success;
    using Base::signed_halfword;

public:
    using Base::Base;

    template<std::uint32_t Entry, std::uint32_t Event, unsigned TimeReg, std::uint32_t Success, class Child>
    std::uint32_t test_recent_event(Child child) {
        constexpr auto time_read = Entry + (TimeReg == eax ? 5u : 6u);
        constexpr auto arguments = time_read + 4u;
        constexpr auto call = arguments + 11u;
        load(Entry, time_read, TimeReg, 0x51df54u);
        gp[edi] = signed_halfword(read(time_read, gp[TimeReg] + 2u, 2));
        recheck(arguments);
        gp[edx] = gp[ebp] + 0x150u;
        gp[ebx] = Event;
        invoke(call, call + 5u, child);
        flags.logic(s.reg(eax, 1), 1);
        if (flags.zf)
            return restore_all;
        finish_success(Success);
        return done;
    }

    template<class Child>
    std::uint32_t test_unit_unavailable(Child child) {
        invoke(0x1f09afu, 0x1f09b4u, child);
        flags.minus(s.reg(eax, 2), 0xffffu, 2);
        if (!flags.zf)
            return restore_all;
        finish_success(0x1f09beu);
        return done;
    }

    template<class Child>
    std::uint32_t test_elapsed(Child child) {
        flags.minus(read(0x1f0c04u, gp[ebp] + 0x80u), 16, 4);
        recheck(0x1f0c0bu);
        if (!flags.zf)
            return restore_all;
        load(0x1f0c11u, 0x1f0c17u, ecx, gp[ebp] + 0x1a0u, 1);
        flags.logic(s.reg(ecx, 1), 1);
        if (flags.zf) {
            load(0x1f0c1bu, 0x1f0c1fu, eax, gp[esp] + 0x13u, 1);
            restore(0x1f0c1fu, 0x1f0c20u, ebx);
            restore(0x1f0c20u, 0x1f0c21u, edi);
            restore(0x1f0c21u, 0x1f0c22u, esi);
            store(0x1f0c22u, 0x1f0c29u, gp[ebp] + 0x1a0u, 1, 1);
            store(0x1f0c29u, 0x1f0c33u, gp[ebp] + 0x1a4u, 0);
            restore(0x1f0c33u, 0x1f0c34u, ebp);
            leave(0x1f0c37u);
            return done;
        }

        invoke(0x1f0c38u, 0x1f0c3du, child);
        flags.minus(s.reg(eax, 2), 0xffffu, 2);
        if (flags.zf) {
            store(0x1f0c4bu, 0x1f0c55u, gp[ebp] + 0x1a4u, 0);
        } else {
            at(0x1f0c43u);
            increment_elapsed(s, 0x1f0c43u, gp[ebp] + 0x1a4u);
            recheck(0x1f0c49u);
        }

        load(0x1f0c55u, 0x1f0c5bu, edx, 0x51df54u);
        gp[eax] = signed_halfword(read(0x1f0c5bu, gp[edx] + 2u, 2));
        recheck(0x1f0c5fu);
        load(0x1f0c5fu, 0x1f0c65u, ecx, gp[ebp] + 0x1a4u);
        gp[edx] = std::bit_cast<std::int32_t>(gp[eax]) < 0 ? 0xffffffffu : 0;
        gp[eax] = flags.minus(gp[eax], gp[edx], 4);
        gp[eax] = flags.right(gp[eax], 1, 4, true);
        flags.minus(gp[ecx], gp[eax], 4);
        if (flags.zf || flags.sf != flags.of)
            return restore_all;
        finish_success(0x1f0c72u);
        return done;
    }

    template<class Player, class Unit>
    std::uint32_t test_award(Player player, Unit unit) {
        save(0x1f0d8fu, 0x1f0d91u, 0);
        save(0x1f0d91u, 0x1f0d93u, 4);
        invoke(0x1f0d93u, 0x1f0d98u, player);
        flags.logic(s.reg(eax, 1), 1);
        if (flags.zf)
            return restore_all;
        load(0x1f0da0u, 0x1f0da3u, eax, gp[ebx] + 0x14u);
        invoke(0x1f0da3u, 0x1f0da8u, unit);
        flags.logic(s.reg(eax, 1), 1);
        if (flags.zf)
            return restore_all;
        finish_success(0x1f0db0u);
        return done;
    }
};
} // namespace halo2::native_player_finished_cold_detail
