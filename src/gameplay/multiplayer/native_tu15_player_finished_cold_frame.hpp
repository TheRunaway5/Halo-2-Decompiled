#pragma once

#include "gameplay/multiplayer/native_tu15_player_finished_1f0670.hpp"

namespace halo2::native_player_finished_cold_detail {
using namespace native_player_finished_detail;

template<class State>
class ColdFrame {
protected:
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

    std::uint32_t read(std::uint32_t pc, std::uint32_t address, unsigned width = 4) {
        at(pc);
        return memory::r(s.memory, address, width);
    }

    void load(std::uint32_t pc, std::uint32_t next, unsigned dest, std::uint32_t address, unsigned width = 4) {
        s.set(dest, read(pc, address, width), width);
        recheck(next);
    }

    void store(std::uint32_t pc, std::uint32_t next, std::uint32_t address, std::uint32_t value, unsigned width = 4) {
        at(pc);
        memory::w(s.memory, s.observe, pc, address, width, value);
        recheck(next);
    }

    void restore(std::uint32_t pc, std::uint32_t next, unsigned reg) {
        at(pc);
        gp[reg] = s.pop();
        recheck(next);
    }

    void save(std::uint32_t pc, std::uint32_t next, std::uint32_t value) {
        at(pc);
        save_word(s, pc, value);
        recheck(next);
    }

    template<class Child>
    void invoke(std::uint32_t pc, std::uint32_t next, Child child) {
        at(pc);
        save_word(s, pc, next);
        child(s);
        recheck(next);
    }

    void leave(std::uint32_t ret) {
        gp[esp] = flags.plus(gp[esp], 0x54u, 4);
        at(ret);
        (void)s.pop();
    }

    void finish_success(std::uint32_t first_pop) {
        restore(first_pop, first_pop + 1u, ebx);
        restore(first_pop + 1u, first_pop + 2u, edi);
        store(first_pop + 2u, first_pop + 7u, gp[esp] + 0xbu, 1, 1);
        load(first_pop + 7u, first_pop + 0xbu, eax, gp[esp] + 0xbu, 1);
        restore(first_pop + 0xbu, first_pop + 0xcu, esi);
        restore(first_pop + 0xcu, first_pop + 0xdu, ebp);
        leave(first_pop + 0x10u);
    }

    static std::uint32_t signed_halfword(std::uint32_t value) {
        return std::uint32_t(std::bit_cast<std::int16_t>(std::uint16_t(value)));
    }

    __m128 scalar(std::uint32_t pc, std::uint32_t address) {
        return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(read(pc, address))));
    }

    void load_scalar(std::uint32_t pc, std::uint32_t next, unsigned lane, std::uint32_t address) {
        s.xmm[lane] = scalar(pc, address);
        recheck(next);
    }

public:
    ColdFrame(State &state, const Cursor *cursor) : s(state), cursor(cursor) {}
};

template<class State, class Invoke>
std::uint32_t bridge(State &s, Invoke invoke) {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_PLAYER_FINISHED_COLD_SLOTS");
        return value && *value;
    }();
    const Cursor *cursor;
    if (reference || !admit(s, cursor))
        return 0;
    try {
        return invoke(cursor);
    } catch (Resume next) {
        return next.pc;
    }
}
} // namespace halo2::native_player_finished_cold_detail
