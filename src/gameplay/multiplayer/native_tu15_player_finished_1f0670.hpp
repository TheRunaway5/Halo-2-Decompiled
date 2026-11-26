#pragma once

#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <cstdlib>

namespace halo2::native_player_finished_detail {
namespace memory = physics_step_detail;
using Cursor = SourcePhysicsContactRuntimeServices::InstructionCursor;

enum Register : unsigned { eax, ecx, edx, ebx, esp, ebp, esi, edi };
inline constexpr std::uint32_t first = 0x1f0670u, last = 0x1f0edfu;
inline constexpr std::uint32_t dispatch_slot = 0x1f06ebu;
inline constexpr std::uint32_t restore_all = 0x1f0ed4u, restore_edi = 0x1f0ed5u, restore_frame = 0x1f0ed6u;
inline constexpr std::uint32_t done = 1u;

struct Resume {
    std::uint32_t pc;
};

template<class State>
void save_word(State &s, std::uint32_t pc, std::uint32_t word) {
    const auto stack = s.gp[esp] - 4u;
    const std::array bytes{
        std::byte(word), std::byte(word >> 8u), std::byte(word >> 16u), std::byte(word >> 24u)};
    if (s.observe) {
        SourcePhysicsContactRecordStore event{pc, stack, std::vector<std::byte>(4), {bytes.begin(), bytes.end()}};
        s.memory.read(stack, event.before);
        s.memory.write(stack, bytes);
        s.gp[esp] = stack;
        s.observe(event);
    } else {
        s.memory.write(stack, bytes);
        s.gp[esp] = stack;
    }
}

template<class State>
bool admit(const State &s, const Cursor *&cursor) {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_PLAYER_FINISHED_1F0670");
        return value && *value;
    }();
    if (reference || s.image != SourcePhysicsContactImage::TitleUpdate15)
        return false;
    cursor = s.immutable_cursor.cursor;
    if (!s.services.instruction)
        return true;
    if (!cursor || (cursor->first_trace && *cursor->first_trace) ||
        (cursor->second_trace && *cursor->second_trace) ||
        (s.observe && !cursor->observation_only_stores))
        return false;
    for (const auto pc : cursor->service_sites)
        if (pc >= first && pc <= last)
            return false;
    return true;
}

template<class State>
class Frame {
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

public:
    Frame(State &state, const Cursor *cursor) : s(state), cursor(cursor) {}

    std::uint32_t slot_target() {
        return word(dispatch_slot, 0x1f0ee0u + gp[edx] * 4u);
    }

    std::uint32_t enter() {
        gp[esp] = flags.minus(gp[esp], 0x54u, 4);
        save(0x1f0673u, 0x1f0674u, ebp);
        gp[ebp] = flags.product(gp[eax], 0x1b0u);
        save(0x1f067cu, 0x1f067du, esi);
        gp[ebp] = flags.plus(gp[ebp], word(0x1f067du, 0x52be64u), 4);
        recheck(0x1f0683u);
        flags.minus(gp[eax], 0xffffffffu, 4);
        at(0x1f0686u);
        memory::w(s.memory, s.observe, 0x1f0686u, gp[esp] + 0xbu, 1, 0);
        recheck(0x1f068bu);
        if (flags.zf)
            return restore_frame;

        load(0x1f0691u, 0x1f0697u, ecx, 0x4f54f0u);
        load(0x1f0697u, 0x1f069bu, eax, gp[ecx] + gp[eax] * 4u + 0xcu);
        flags.minus(gp[eax], 0xffffffffu, 4);
        if (flags.zf)
            return restore_frame;

        load(0x1f06a4u, 0x1f06aau, ecx, 0x4f54f4u);
        load(0x1f06aau, 0x1f06adu, esi, gp[ecx] + 0x44u);
        gp[eax] = flags.logic(gp[eax] & 65535u, 4);
        gp[eax] = flags.product(gp[eax], 0x21cu);
        gp[eax] = flags.plus(gp[eax], gp[esi], 4);
        save(0x1f06bau, 0x1f06bbu, edi);
        gp[edi] = gp[eax];
        load(0x1f06bdu, 0x1f06c0u, eax, gp[edi] + 0x2cu);
        flags.minus(gp[eax], 0xffffffffu, 4);
        if (flags.zf)
            return restore_edi;

        load(0x1f06c9u, 0x1f06cfu, ecx, 0x4ecbd0u);
        load(0x1f06cfu, 0x1f06d2u, esi, gp[ecx] + 0x44u);
        gp[ecx] = flags.logic(gp[eax] & 65535u, 4);
        flags.minus(gp[edx], 31u, 4);
        gp[ecx] *= 3u;
        save(0x1f06e0u, 0x1f06e1u, ebx);
        load(0x1f06e1u, 0x1f06e5u, ebx, gp[esi] + gp[ecx] * 4u + 8u);
        return flags.cf || flags.zf ? dispatch_slot : restore_all;
    }

    template<std::uint32_t Start>
    void leave() {
        if constexpr (Start == restore_all)
            restore(0x1f0ed4u, 0x1f0ed5u, ebx);
        if constexpr (Start != restore_frame)
            restore(0x1f0ed5u, 0x1f0ed6u, edi);
        load(0x1f0ed6u, 0x1f0edau, eax, gp[esp] + 0xbu, 1);
        restore(0x1f0edau, 0x1f0edbu, esi);
        restore(0x1f0edbu, 0x1f0edcu, ebp);
        gp[esp] = flags.plus(gp[esp], 0x54u, 4);
        at(last);
        (void)s.pop();
    }
};

template<class State>
std::uint32_t prologue(State &s) {
    const Cursor *cursor;
    if (!admit(s, cursor))
        return 0;
    try {
        return Frame<State>(s,cursor).enter();
    } catch (Resume next) {
        return next.pc;
    }
}

template<class State, class Branch>
bool select_slot(State &s, Branch branch, std::uint32_t &target) {
    const Cursor *cursor;
    if (!admit(s, cursor))
        return false;
    target = branch(Frame<State>(s,cursor).slot_target());
    // Once the table word is loaded the original jump is complete. A
    // provider that opened tracing resumes at the selected destination.
    return true;
}

template<std::uint32_t Start, class State>
std::uint32_t epilogue(State &s) {
    const Cursor *cursor;
    if (!admit(s, cursor))
        return 0;
    try {
        Frame<State>(s,cursor).template leave<Start>();
        return done;
    } catch (Resume next) {
        return next.pc;
    }
}
} // namespace halo2::native_player_finished_detail
