#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_required_owner_policy.hpp"

namespace halo2::native_script_offset_rebase_detail {

using native_object_tick_detail::Registers;
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::flag;

inline constexpr std::uint32_t no_datum = 0xffffffffu;
inline constexpr auto entry_pc = 0x2aa6d0u;
inline constexpr auto return_pc = 0x2aa7a5u;

inline VerifySummary &verification() {
    static VerifySummary value{"Script expression offset rebase"};
    return value;
}

template<class State, class Dispatch>
struct OffsetRebase {
    State &state;
    std::uint32_t *cursor;
    Trace<State> *trace;
    Dispatch &dispatch;
    std::uint64_t hit{};

    // Keep the original fault ABI in the live register file, including a
    // decremented stack pointer before a write that can throw.
    void load() const {}

    void at(std::uint32_t pc) const { if (cursor) *cursor = pc; }

    std::uint32_t read(std::uint32_t pc, std::uint32_t address, unsigned width = 4) {
        at(pc);
        const auto value = physics_step_detail::r(state.memory, address, width);
        if (trace) trace->events.push_back({0, pc, address, width, value});
        return value;
    }

    void write(std::uint32_t pc, std::uint32_t address, std::uint32_t value) {
        at(pc);
        physics_step_detail::w(state.memory, state.observe, pc, address, 4, value);
        if (trace) trace->events.push_back({1, pc, address, 4, value});
    }

    void push(std::uint32_t pc, std::uint32_t value) {
        auto &stack = state.gp[esp];
        stack -= 4u;
        write(pc, stack, value);
    }

    std::uint32_t pop(std::uint32_t pc) {
        auto &stack = state.gp[esp];
        const auto value = read(pc, stack);
        stack += 4u;
        return value;
    }

    void first_index() {
        constexpr auto target = 0x16ea60u;
        constexpr auto site = 0x2aa6ebu;
        push(site, 0x2aa6f0u);
        if (trace) trace->calls.push_back({target, trace->events.size(), Registers<State>::capture(state), {}});
        dispatch(state, target, site);
        if (trace) trace->calls.back().exit = Registers<State>::capture(state);
    }

    void first_datum() {
        auto &gp = state.gp;
        auto &flags = state.flags;
        gp[ecx] = flags.logic(no_datum, 4);
        flags.minus(gp[eax], no_datum, 4);
        if (flags.zf) return;

        gp[edx] = read(0x2aa6f8u, gp[ebx] + 0x24u);
        gp[ecx] = read(0x2aa6fbu, gp[ebx] + 0x44u);
        gp[edx] = flags.product(gp[edx], gp[eax]);
        gp[ecx] = native_object_tick_detail::sx16(read(0x2aa701u, gp[edx] + gp[ecx], 2));
        gp[ecx] = flags.logic((gp[ecx] << 16u) | gp[eax], 4);
    }

    void rebase_string() {
        auto &gp = state.gp;
        auto &flags = state.flags;
        gp[esi] = read(0x2aa720u, gp[ebx] + 0x44u);
        gp[eax] = flags.logic(gp[edx] & 0xffffu, 4);
        gp[ecx] = gp[eax] * 5u;
        constexpr auto string_type = 9u;
        flags.minus(read(0x2aa72du, gp[esi] + gp[ecx] * 4u + 4u, 2), string_type, 2);
        gp[ecx] = gp[esi] + gp[ecx] * 4u;
        if (!flags.zf) return;

        gp[esi] = read(0x2aa738u, gp[ecx] + 0xcu);
        gp[esi] = flags.plus(gp[esi], read(0x2aa73bu, gp[esp] + 0x10u), 4);
        write(0x2aa73fu, gp[ecx] + 0x10u, gp[esi]);
        hit |= 1u;
    }

    void scan_active_datums() {
        auto &gp = state.gp;
        auto &flags = state.flags;
        flags.minus(gp[edx], no_datum, 4);
        gp[eax] = flags.zf ? flags.logic(0, 4) : flags.increment(gp[eax], 4, false);
        gp[esi] = flags.logic(no_datum, 4);
        flags.logic(gp[eax], 4);
        if (flags.sf != flags.of) return;

        gp[edi] = read(0x2aa753u, gp[ebx] + 0x38u);
        flags.minus(gp[eax], gp[edi], 4);
        if (flags.sf == flags.of) return;

        gp[edx] = read(0x2aa75au, gp[ebx] + 0x48u);
        do {
            gp[ecx] = flags.logic(gp[eax] & 31u, 4);
            gp[ebp] = flags.shift(1u, gp[ecx], 4);
            gp[ecx] = flags.right(gp[eax], 5u, 4, true);
            flags.logic(read(0x2aa771u, gp[edx] + gp[ecx] * 4u) & gp[ebp], 4);
            if (!flags.zf) {
                gp[esi] = gp[eax];
                hit |= 2u;
                return;
            }
            gp[eax] = flags.increment(gp[eax], 4, false);
            flags.minus(gp[eax], gp[edi], 4);
        } while (flags.sf != flags.of);
    }

    void advance_datum() {
        auto &gp = state.gp;
        auto &flags = state.flags;
        scan_active_datums();
        gp[eax] = flags.logic(no_datum, 4);
        flags.minus(gp[esi], no_datum, 4);
        if (!flags.zf) {
            gp[edx] = read(0x2aa787u, gp[ebx] + 0x24u);
            gp[eax] = read(0x2aa78au, gp[ebx] + 0x44u);
            gp[edx] = flags.product(gp[edx], gp[esi]);
            gp[eax] = native_object_tick_detail::sx16(read(0x2aa790u, gp[edx] + gp[eax], 2));
            gp[eax] = flags.logic((gp[eax] << 16u) | gp[esi], 4);
        }
        flags.minus(gp[eax], no_datum, 4);
        gp[edx] = gp[eax];
    }

    void run() {
        auto &gp = state.gp;
        auto &flags = state.flags;
        push(entry_pc, gp[ecx]);
        gp[eax] = read(0x2aa6d1u, 0x4ecc20u);
        gp[ecx] = read(0x2aa6d6u, gp[eax] + 0x1b4u);
        push(0x2aa6dcu, gp[ebx]);
        gp[ebx] = read(0x2aa6ddu, 0x505e6cu);
        write(0x2aa6e3u, gp[esp] + 4u, gp[ecx]);
        gp[edx] = flags.logic(0, 4);
        gp[ecx] = gp[ebx];
        first_index();
        first_datum();

        flags.minus(gp[ecx], no_datum, 4);
        gp[edx] = gp[ecx];
        if (!flags.zf) {
            push(0x2aa715u, gp[ebp]);
            push(0x2aa716u, gp[esi]);
            push(0x2aa717u, gp[edi]);
            do {
                rebase_string();
                advance_datum();
            } while (!flags.zf);
            gp[edi] = pop(0x2aa7a0u);
            gp[esi] = pop(0x2aa7a1u);
            gp[ebp] = pop(0x2aa7a2u);
        }
        gp[ebx] = pop(0x2aa7a3u);
        gp[ecx] = pop(0x2aa7a4u);
        pop(return_pc);
    }
};

template<class State, class Reference, class Dispatch>
bool bridge(State &state, Reference reference, Dispatch dispatch) {
    static const bool forced = flag("HALO2_REFERENCE_SCRIPT_OFFSET_REBASE");
    static const bool verify = flag("HALO2_VERIFY_SCRIPT_OFFSET_REBASE");
    static std::atomic<std::uint64_t> coverage{};
    if (!native_required_owner_policy_detail::preserves_instruction_policy(state.services) ||
        ((state.services.instruction || state.observe) && !state.immutable_cursor.cursor))
        return false;
    return native_small_queries_detail::run_owner(state, entry_pc, return_pc, forced, verify,
        coverage, verification(), reference, [&](State &s, std::uint32_t *cursor, Trace<State> *trace) {
            return OffsetRebase<State, Dispatch>{s, cursor, trace, dispatch};
        });
}

} // namespace halo2::native_script_offset_rebase_detail
