#pragma once

#include "gameplay/multiplayer/native_tu15_player_finished_cold_frame.hpp"

namespace halo2::native_player_finished_cold_detail {

inline constexpr std::uint32_t team_result = 0x1f0eceu;
inline constexpr std::uint32_t shared_position_gate = 0x1f085bu;

template<class State>
class FlagFrame : public ColdFrame<State> {
    using Base = ColdFrame<State>;
    using Base::s;
    using Base::gp;
    using Base::flags;
    using Base::read;
    using Base::load;
    using Base::store;
    using Base::restore;
    using Base::leave;
    using Base::recheck;
    using Base::finish_success;

public:
    using Base::Base;

    template<std::uint32_t Entry, std::uint32_t ClearMask>
    std::uint32_t consume_engine_bit() {
        flags.logic(read(Entry, gp[ebp] + 0x88u, 1) & 4u, 1);
        recheck(Entry + 7u);
        if (flags.zf)
            return restore_all;
        load(Entry + 0xdu, Entry + 0x13u, eax, gp[ebp] + 0x194u, 1);
        restore(Entry + 0x13u, Entry + 0x14u, ebx);
        restore(Entry + 0x14u, Entry + 0x15u, edi);
        s.set(eax, flags.logic(s.reg(eax, 1) & ~ClearMask, 1), 1);
        store(Entry + 0x17u, Entry + 0x1cu, gp[esp] + 0xbu, 1, 1);
        store(Entry + 0x1cu, Entry + 0x22u, gp[ebp] + 0x194u, s.reg(eax, 1), 1);
        load(Entry + 0x22u, Entry + 0x26u, eax, gp[esp] + 0xbu, 1);
        restore(Entry + 0x26u, Entry + 0x27u, esi);
        restore(Entry + 0x27u, Entry + 0x28u, ebp);
        leave(Entry + 0x2bu);
        return done;
    }

    std::uint32_t consume_second_engine_bit() {
        load(0x1f0b35u, 0x1f0b3bu, eax, gp[ebp] + 0x88u, 1);
        gp[ecx] = 1;
        flags.logic(s.reg(eax, 1) & 1u, 1);
        if (flags.zf)
            return restore_all;
        load(0x1f0b48u, 0x1f0b4eu, eax, gp[ebp] + 0x194u, 1);
        restore(0x1f0b4eu, 0x1f0b4fu, ebx);
        restore(0x1f0b4fu, 0x1f0b50u, edi);
        s.set(eax, flags.logic(s.reg(eax, 1) & 0xfdu, 1), 1);
        store(0x1f0b52u, 0x1f0b58u, gp[ebp] + 0x194u, s.reg(eax, 1), 1);
        restore(0x1f0b58u, 0x1f0b59u, esi);
        store(0x1f0b59u, 0x1f0b5du, gp[esp] + 7u, s.reg(ecx, 1), 1);
        s.set(eax, s.reg(ecx, 1), 1);
        restore(0x1f0b5fu, 0x1f0b60u, ebp);
        leave(0x1f0b63u);
        return done;
    }

    template<std::uint32_t Entry, std::uint32_t Mask>
    std::uint32_t test_engine_bit() {
        flags.logic(read(Entry, gp[ebp] + 0x88u) & Mask, 4);
        recheck(Entry + 0xau);
        if (flags.zf)
            return restore_all;
        finish_success(Entry + 0x10u);
        return done;
    }

    template<std::uint32_t Entry, std::uint32_t Mask>
    std::uint32_t test_engine_high_byte() {
        load(Entry, Entry + 6u, eax, gp[ebp] + 0x88u);
        flags.logic(s.reg(eax, 1, 8) & Mask, 1);
        if (flags.zf)
            return restore_all;
        finish_success(Entry + 0xfu);
        return done;
    }

    std::uint32_t unit_threshold() {
        this->load_scalar(0x1f0e22u, 0x1f0e2au, 0, gp[ebx] + 0xf0u);
        flags.scalar_compare(s.xmm[0], this->scalar(0x1f0e2au, 0x46a188u), false);
        recheck(0x1f0e31u);
        if (flags.cf)
            return restore_all;
        finish_success(0x1f0e37u);
        return done;
    }

    std::uint32_t position_flag() {
        load(0x1f0e48u, 0x1f0e4eu, eax, gp[ebp] + 0x195u, 1);
        flags.logic(s.reg(eax, 1), 1);
        return shared_position_gate;
    }

    template<bool FirstTeam>
    std::uint32_t test_team() {
        constexpr auto compare_team = FirstTeam ? 0x1f0e89u : 0x1f0eb5u;
        gp[ecx] = 1;
        flags.minus(read(compare_team, gp[ebp] + 0x140u, 2), 1, 2);
        recheck(compare_team + 7u);
        if (!flags.zf)
            return restore_all;
        if constexpr (FirstTeam) {
            load(0x1f0e92u, 0x1f0e99u, eax, gp[ebp] + 0x14cu, 2);
            flags.logic(s.reg(eax, 1) & 3u, 1);
            if (flags.zf)
                return restore_all;
            flags.minus(read(0x1f0e9du, gp[ebp] + 0x148u, 2), 3, 2);
            recheck(0x1f0ea5u);
            if (flags.cf)
                return restore_all;
            flags.logic(read(0x1f0ea7u, gp[ebp] + 0xa4u, 1) & 2u, 1);
            recheck(0x1f0eaeu);
        } else {
            flags.logic(read(0x1f0ebeu, gp[ebp] + 0x14cu, 1) & 0xcu, 1);
            recheck(0x1f0ec5u);
            if (flags.zf)
                return restore_all;
            flags.logic(read(0x1f0ec7u, gp[ebp] + 0xa4u, 1) & 0x20u, 1);
            recheck(team_result);
        }
        return team_result;
    }

    std::uint32_t publish_team_result() {
        if (!flags.zf)
            store(0x1f0ed0u, restore_all, gp[esp] + 0x13u, s.reg(ecx, 1), 1);
        return restore_all;
    }
};
} // namespace halo2::native_player_finished_cold_detail
