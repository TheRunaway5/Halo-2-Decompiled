#pragma once

#include "physics/runtime/native_tu15_two_bone_solve.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <cstdlib>

namespace halo2::native_effect_observer_retire_detail {
using namespace native_object_tick_detail::boundary;
using Cursor = SourcePhysicsContactRuntimeServices::InstructionCursor;

constexpr std::uint32_t entry = 0x17e250u;
constexpr std::uint32_t last = 0x17e319u;
constexpr std::uint32_t done = 1u;
constexpr std::uint32_t none = 0xffffffffu;

struct Resume { std::uint32_t pc; };

template<class State>
class Retirement {
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
        return physics_step_detail::r(s.memory, address, width);
    }

    void load(std::uint32_t pc, std::uint32_t next, unsigned dest, std::uint32_t address, unsigned width = 4) {
        gp[dest] = read(pc, address, width);
        recheck(next);
    }

    void load_salt(std::uint32_t pc, std::uint32_t next, unsigned dest, std::uint32_t address) {
        gp[dest] = std::uint32_t(std::bit_cast<std::int16_t>(std::uint16_t(read(pc, address, 2))));
        recheck(next);
    }

    void save(std::uint32_t pc, std::uint32_t next, unsigned reg) {
        at(pc);
        native_two_bone_solve::push_word(s, pc, gp[reg]);
        recheck(next);
    }

    void restore(std::uint32_t pc, std::uint32_t next, unsigned reg) {
        at(pc);
        gp[reg] = s.pop();
        recheck(next);
    }

    template<class Child>
    void call(std::uint32_t pc, std::uint32_t next, Child child) {
        at(pc);
        native_two_bone_solve::push_word(s, pc, next);
        child(s);
        recheck(next);
    }

    template<class First>
    bool begin(First first) {
        save(entry, 0x17e251u, ebx);
        load(0x17e251u, 0x17e257u, ebx, 0x4f720cu);
        save(0x17e257u, 0x17e258u, edi);
        gp[edx] = flags.logic(0, 4);
        gp[ecx] = gp[ebx];
        call(0x17e25cu, 0x17e261u, first);
        gp[edi] = flags.logic(none, 4);
        flags.minus(gp[eax], none, 4);
        if (!flags.zf) {
            load(0x17e269u, 0x17e26cu, ecx, gp[ebx] + 0x24u);
            load(0x17e26cu, 0x17e26fu, edx, gp[ebx] + 0x44u);
            gp[ecx] = flags.product(gp[ecx], gp[eax]);
            load_salt(0x17e272u, 0x17e276u, edi, gp[ecx] + gp[edx]);
            gp[edi] = flags.shift(gp[edi], 16, 4);
            gp[edi] = flags.logic(gp[edi] | gp[eax], 4);
        }
        flags.minus(gp[edi], none, 4);
        return !flags.zf;
    }

    void next_occupied_index() {
        flags.minus(gp[edi], none, 4);
        gp[eax] = flags.zf ? flags.logic(0, 4) : gp[esi] + 1u;
        gp[esi] = flags.logic(none, 4);
        flags.logic(gp[eax], 4);
        if (flags.sf != flags.of)
            return;
        load(0x17e2c7u, 0x17e2cau, edi, gp[ebx] + 0x38u);
        flags.minus(gp[eax], gp[edi], 4);
        if (flags.sf == flags.of)
            return;
        load(0x17e2ceu, 0x17e2d1u, edx, gp[ebx] + 0x48u);
        do {
            gp[ecx] = flags.logic(gp[eax] & 31u, 4);
            gp[ebp] = flags.shift(1, gp[ecx], 4);
            gp[ecx] = flags.right(gp[eax], 5, 4, true);
            flags.logic(read(0x17e2e2u, gp[edx] + gp[ecx] * 4u) & gp[ebp], 4);
            recheck(0x17e2e5u);
            if (!flags.zf) {
                gp[esi] = gp[eax];
                return;
            }
            gp[eax] = flags.increment(gp[eax], 4, false);
            flags.minus(gp[eax], gp[edi], 4);
        } while (flags.sf != flags.of);
    }

    bool advance() {
        next_occupied_index();
        gp[eax] = flags.logic(none, 4);
        flags.minus(gp[esi], none, 4);
        if (!flags.zf) {
            load(0x17e2f8u, 0x17e2fbu, edx, gp[ebx] + 0x24u);
            load(0x17e2fbu, 0x17e2feu, eax, gp[ebx] + 0x44u);
            gp[edx] = flags.product(gp[edx], gp[esi]);
            load_salt(0x17e301u, 0x17e305u, eax, gp[edx] + gp[eax]);
            gp[eax] = flags.shift(gp[eax], 16, 4);
            gp[eax] = flags.logic(gp[eax] | gp[esi], 4);
        }
        flags.minus(gp[eax], none, 4);
        gp[edi] = gp[eax];
        return !flags.zf;
    }

public:
    Retirement(State &state, const Cursor *cursor) : s(state), cursor(cursor) {}

    template<class First, class Retire>
    void run(First first, Retire retire) {
        if (begin(first)) {
            save(0x17e284u, 0x17e285u, ebp);
            save(0x17e285u, 0x17e286u, esi);
            do {
                load(0x17e290u, 0x17e293u, ecx, gp[ebx] + 0x44u);
                gp[esi] = flags.logic(gp[edi] & 0xffffu, 4);
                gp[eax] = flags.product(gp[esi], 0x190u);
                load(0x17e2a3u, 0x17e2a7u, edx, gp[eax] + gp[ecx] + 0x48u);
                flags.minus(gp[edx], read(0x17e2a7u, gp[esp] + 0x14u), 4);
                recheck(0x17e2abu);
                if (flags.zf) {
                    gp[eax] = gp[edi];
                    call(0x17e2afu, 0x17e2b4u, retire);
                }
            } while (advance());
            restore(0x17e315u, 0x17e316u, esi);
            restore(0x17e316u, 0x17e317u, ebp);
        }
        restore(0x17e317u, 0x17e318u, edi);
        restore(0x17e318u, last, ebx);
        at(last);
        (void)s.pop();
        gp[esp] += 4u;
    }
};

template<class State, class First, class Retire>
std::uint32_t bridge(State &s, First first, Retire retire) {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_EFFECT_OBSERVER_RETIRE_17E250");
        return value && *value;
    }();
    if (reference || s.image != SourcePhysicsContactImage::TitleUpdate15)
        return 0;
    const auto *cursor = s.immutable_cursor.cursor;
    if (s.services.instruction) {
        if (!cursor || (cursor->first_trace && *cursor->first_trace) ||
            (cursor->second_trace && *cursor->second_trace) || (s.observe && !cursor->observation_only_stores))
            return 0;
        for (const auto pc : cursor->service_sites)
            if (pc >= entry && pc <= last)
                return 0;
    }
    try {
        Retirement<State>(s, cursor).run(first, retire);
        return done;
    } catch (Resume next) {
        return next.pc;
    }
}
} // namespace halo2::native_effect_observer_retire_detail
