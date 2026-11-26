#pragma once

#include "gameplay/multiplayer/native_tu15_player_finished_1f0670.hpp"

namespace halo2::native_player_finished_weapon_detail {
using namespace native_player_finished_detail;

template<class State>
class WeaponFrame {
    State &s;
    const Cursor *cursor;
    decltype(State::gp) &gp = s.gp;
    decltype(State::flags) &flags = s.flags;

    void at(std::uint32_t pc) const {
        if (cursor)
            *cursor->position = pc;
    }

    void recheck(std::uint32_t next) const {
        if (cursor ? (cursor->first_trace && *cursor->first_trace) ||
                         (cursor->second_trace && *cursor->second_trace)
                   : bool(s.services.instruction))
            throw Resume{next};
    }

    std::uint32_t word(std::uint32_t pc, std::uint32_t address, unsigned width = 4) {
        at(pc);
        return memory::r(s.memory, address, width);
    }

    void load(std::uint32_t pc, std::uint32_t next, unsigned dest, std::uint32_t address, unsigned width = 4) {
        s.set(dest, word(pc, address, width), width);
        recheck(next);
    }

    void load_weapon_index(std::uint32_t pc, std::uint32_t next, std::uint32_t unit) {
        const auto raw = std::uint8_t(word(pc, unit + 0x212u, 1));
        s.set(ecx, std::uint32_t(std::bit_cast<std::int8_t>(raw)), 2);
        recheck(next);
    }

    void save(std::uint32_t pc, std::uint32_t next, unsigned reg) {
        at(pc);
        save_word(s, pc, gp[reg]);
        recheck(next);
    }

    void restore(std::uint32_t pc, std::uint32_t next, unsigned reg) {
        at(pc);
        gp[reg] = s.pop();
        recheck(next);
    }

    template<class Dispatch>
    void invoke(Dispatch &dispatch, std::uint32_t pc, std::uint32_t target, std::uint32_t next) {
        at(pc);
        save_word(s, pc, next);
        dispatch(s, target, pc, false);
        recheck(next);
    }

    void return_finished() {
        restore(0x1f0d3cu, 0x1f0d3du, ebx);
        restore(0x1f0d3du, 0x1f0d3eu, edi);
        at(0x1f0d3eu);
        memory::w(s.memory, s.observe, 0x1f0d3eu, gp[esp] + 0xbu, 1, 1);
        recheck(0x1f0d43u);
        load(0x1f0d43u, 0x1f0d47u, eax, gp[esp] + 0xbu, 1);
        restore(0x1f0d47u, 0x1f0d48u, esi);
        restore(0x1f0d48u, 0x1f0d49u, ebp);
        gp[esp] = flags.plus(gp[esp], 0x54u, 4);
        at(0x1f0d4cu);
        (void)s.pop();
    }

public:
    WeaponFrame(State &state, const Cursor *cursor) : s(state), cursor(cursor) {}

    template<class Dispatch>
    std::uint32_t test(Dispatch &dispatch) {
        load(0x1f0cb5u, 0x1f0cb8u, eax, gp[edi] + 0x2cu);
        gp[ecx] = flags.logic(gp[eax] & 0xffffu, 4);
        gp[ecx] *= 3u;
        load(0x1f0cc3u, 0x1f0cc7u, edx, gp[esi] + gp[ecx] * 4u + 8u);
        load_weapon_index(0x1f0cc7u, 0x1f0ccfu, gp[edx]);
        invoke(dispatch, 0x1f0ccfu, 0xce230u, 0x1f0cd4u);
        flags.minus(gp[eax], 0xffffffffu, 4);
        if (flags.zf)
            return restore_all;

        load(0x1f0cddu, 0x1f0ce0u, eax, gp[edi] + 0x2cu);
        gp[ecx] = gp[esp] + 0x28u;
        save(0x1f0ce4u, 0x1f0ce5u, ecx);
        gp[ecx] = flags.logic(gp[eax] & 0xffffu, 4);
        gp[edx] = gp[ecx] * 3u;
        load(0x1f0cf0u, 0x1f0cf4u, ecx, gp[esi] + gp[edx] * 4u + 8u);
        load_weapon_index(0x1f0cf4u, 0x1f0cfcu, gp[ecx]);
        invoke(dispatch, 0x1f0cfcu, 0xce230u, 0x1f0d01u);
        save(0x1f0d01u, 0x1f0d02u, eax);
        invoke(dispatch, 0x1f0d02u, 0x1055f0u, 0x1f0d07u);

        flags.minus(word(0x1f0d07u, gp[esp] + 0x4cu, 2), 0, 2);
        recheck(0x1f0d0du);
        if (flags.zf || flags.sf != flags.of)
            return restore_all;
        load(0x1f0d13u, 0x1f0d18u, eax, gp[esp] + 0x52u, 2);
        flags.logic(s.reg(eax, 2), 2);
        if (flags.zf)
            return restore_all;
        flags.minus(word(0x1f0d21u, gp[esp] + 0x50u, 2), s.reg(eax, 2), 2);
        recheck(0x1f0d26u);
        if (flags.sf == flags.of)
            return restore_all;
        flags.logic(word(0x1f0d2cu, gp[ebx] + 0x148u) & 0x10000000u, 4);
        recheck(0x1f0d36u);
        if (flags.zf)
            return restore_all;

        return_finished();
        return done;
    }
};

template<class State, class Dispatch>
std::uint32_t bridge(State &s, Dispatch dispatch) {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_PLAYER_FINISHED_WEAPON_SLOT");
        return value && *value;
    }();
    const Cursor *cursor;
    if (reference || !admit(s, cursor))
        return 0;
    try {
        return WeaponFrame<State>(s, cursor).test(dispatch);
    } catch (Resume next) {
        return next.pc;
    }
}
} // namespace halo2::native_player_finished_weapon_detail
