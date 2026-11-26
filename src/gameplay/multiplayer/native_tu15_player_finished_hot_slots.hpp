#pragma once

#include "gameplay/multiplayer/native_tu15_player_finished_1f0670.hpp"

namespace halo2::native_player_finished_hot_detail {
using namespace native_player_finished_detail;

inline constexpr std::uint32_t shared_gate = 0x1f085bu;
inline constexpr std::uint32_t shared_success = 0x1f0861u;

template<class State>
class CompletionFrame {
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

    void store_byte(std::uint32_t pc, std::uint32_t next, std::uint32_t address, std::uint32_t value) {
        at(pc);
        memory::w(s.memory, s.observe, pc, address, 1, value);
        recheck(next);
    }

    void restore(std::uint32_t pc, std::uint32_t next, unsigned reg) {
        at(pc);
        gp[reg] = s.pop();
        recheck(next);
    }

    template<class Child>
    void invoke(std::uint32_t pc, std::uint32_t next, Child child) {
        at(pc);
        save_word(s, pc, next);
        child(s);
        recheck(next);
    }

    template<std::uint32_t SavedEbx, std::uint32_t Latch = 0>
    void finish() {
        constexpr auto restore_ebp = SavedEbx + (Latch ? 0x13u : 0xcu);
        restore(SavedEbx, SavedEbx + 1u, ebx);
        restore(SavedEbx + 1u, SavedEbx + 2u, edi);
        store_byte(SavedEbx + 2u, SavedEbx + 7u, gp[esp] + 0xbu, 1);
        load(SavedEbx + 7u, SavedEbx + 0xbu, eax, gp[esp] + 0xbu, 1);
        restore(SavedEbx + 0xbu, SavedEbx + 0xcu, esi);
        if constexpr (Latch)
            store_byte(SavedEbx + 0xcu, restore_ebp, gp[ebp] + Latch, 0);
        restore(restore_ebp, restore_ebp + 1u, ebp);
        gp[esp] = flags.plus(gp[esp], 0x54u, 4);
        at(restore_ebp + 4u);
        (void)s.pop();
    }

public:
    CompletionFrame(State &state, const Cursor *cursor) : s(state), cursor(cursor) {}

    template<std::uint32_t Entry, std::uint32_t Latch>
    std::uint32_t consume_latch() {
        load(Entry, Entry + 6u, eax, gp[ebp] + Latch, 1);
        flags.logic(s.reg(eax, 1), 1);
        if (flags.cf || flags.zf)
            return restore_all;
        finish<Entry + 0xeu, Latch>();
        return done;
    }

    template<class Mode, class Lookup>
    std::uint32_t multiplayer_player(Mode mode, Lookup lookup) {
        load(0x1f0bb6u, 0x1f0bbcu, ecx, 0x4f3218u);
        flags.minus(read(0x1f0bbcu, gp[ecx] + 8u), 2, 4);
        recheck(0x1f0bc0u);
        if (!flags.zf)
            return restore_all;
        invoke(0x1f0bc6u, 0x1f0bcbu, mode);
        flags.logic(s.reg(eax, 1), 1);
        if (flags.zf)
            return restore_all;
        load(0x1f0bd3u, 0x1f0bd6u, eax, gp[edi] + 0x24u);
        flags.minus(gp[eax], 0xffffffffu, 4);
        if (flags.zf)
            return restore_all;
        s.set(ecx, s.reg(eax, 2), 2);
        invoke(0x1f0be2u, 0x1f0be7u, lookup);
        flags.logic(gp[eax], 4);
        if (flags.zf)
            return restore_all;
        load(0x1f0befu, 0x1f0bf2u, ecx, gp[eax] + 0x15u, 1);
        flags.logic(s.reg(ecx, 1), 1);
        if (!flags.cf && !flags.zf)
            return shared_success;
        load(0x1f0bfau, 0x1f0bfdu, ecx, gp[eax] + 0x18u, 1);
        flags.logic(s.reg(ecx, 1), 1);
        return shared_gate;
    }

    std::uint32_t complete() {
        finish<shared_success>();
        return done;
    }
};

template<class State>
bool admits(const State &s, const Cursor *&cursor) {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_PLAYER_FINISHED_HOT_SLOTS");
        return value && *value;
    }();
    return !reference && admit(s, cursor);
}

template<std::uint32_t Entry, std::uint32_t Latch, class State>
std::uint32_t consume_latch(State &s) {
    const Cursor *cursor;
    if (!admits(s, cursor))
        return 0;
    try {
        return CompletionFrame<State>(s, cursor).template consume_latch<Entry, Latch>();
    } catch (Resume next) {
        return next.pc;
    }
}

template<class State, class Mode, class Lookup>
std::uint32_t multiplayer_player(State &s, Mode mode, Lookup lookup) {
    const Cursor *cursor;
    if (!admits(s, cursor))
        return 0;
    try {
        return CompletionFrame<State>(s, cursor).multiplayer_player(mode, lookup);
    } catch (Resume next) {
        return next.pc;
    }
}

template<class State>
std::uint32_t gate(State &s) {
    const Cursor *cursor;
    if (!admits(s, cursor))
        return 0;
    return s.flags.cf || s.flags.zf ? restore_all : shared_success;
}

template<class State>
std::uint32_t complete(State &s) {
    const Cursor *cursor;
    if (!admits(s, cursor))
        return 0;
    try {
        return CompletionFrame<State>(s, cursor).complete();
    } catch (Resume next) {
        return next.pc;
    }
}
} // namespace halo2::native_player_finished_hot_detail
