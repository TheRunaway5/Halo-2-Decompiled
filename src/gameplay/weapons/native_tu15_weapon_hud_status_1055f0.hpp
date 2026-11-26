#pragma once

#include "physics/runtime/native_tu15_two_bone_solve.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <cstdlib>

namespace halo2::native_weapon_hud_status_detail {
using namespace native_object_tick_detail::boundary;
using Cursor = SourcePhysicsContactRuntimeServices::InstructionCursor;

constexpr std::uint32_t first = 0x1055f0u;
constexpr std::uint32_t last = 0x105942u;
constexpr std::uint32_t magazine_loop = 0x1056b0u;
constexpr std::uint32_t reload_state = 0x105774u;
constexpr std::uint32_t clear_status_word = 0x10563cu;
constexpr std::uint32_t done = 1u;

struct Resume { std::uint32_t pc; };

template<class State>
void clear_word(State &s) {
    const auto stride = (s.other_flags & 0x400u) ? 0xfffffffcu : 4u;
    native_two_bone_solve::store_and_commit(s, clear_status_word, s.gp[edi], s.gp[eax], [&] {
        s.gp[edi] += stride;
        --s.gp[ecx];
    });
}

template<class State>
class StatusFrame {
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
        return physics_step_detail::r(s.memory, address, width);
    }

    void load(std::uint32_t pc, std::uint32_t next, unsigned reg, std::uint32_t address,
              unsigned read_width = 4, unsigned reg_width = 4) {
        s.set(reg, read(pc, address, read_width), reg_width);
        recheck(next);
    }

    void store(std::uint32_t pc, std::uint32_t next, std::uint32_t address, std::uint32_t value, unsigned width = 4) {
        at(pc);
        physics_step_detail::w(s.memory, s.observe, pc, address, width, value);
        recheck(next);
    }

    void push(std::uint32_t pc, std::uint32_t next, std::uint32_t value) {
        at(pc);
        native_two_bone_solve::push_word(s, pc, value);
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

    void load_signed_halfword(std::uint32_t pc, std::uint32_t next, unsigned reg, std::uint32_t address) {
        gp[reg] = signed_halfword(read(pc, address, 2));
        recheck(next);
    }

    static std::uint32_t signed_halfword(std::uint32_t value) {
        return std::uint32_t(std::bit_cast<std::int16_t>(std::uint16_t(value)));
    }

    void clear_status() {
        gp[ecx] = 15;
        gp[edi] = gp[ebp];
        do {
            at(clear_status_word);
            clear_word(s);
            recheck(gp[ecx] ? clear_status_word : 0x10563eu);
        } while (gp[ecx]);
    }

    void store_initial_fields() {
        load(0x10563eu, 0x105644u, ecx, gp[ebx] + 0x180u);
        store(0x105644u, 0x105647u, gp[ebp], gp[ecx]);
        load(0x105647u, 0x10564du, eax, gp[ebx] + 0x184u);
        store(0x10564du, 0x105650u, gp[ebp] + 4u, gp[eax]);
        load(0x105650u, 0x105657u, ecx, gp[ebx] + 0x175u, 1);
        s.xmm[0] = _mm_cvtsi32_ss(s.xmm[0], std::bit_cast<std::int32_t>(gp[ecx]));
        const auto scale = std::bit_cast<float>(read(0x10565bu, 0x46a0c4u));
        s.xmm[0] = native_tu15_leaves::ordered_mul_ss(s.xmm[0], _mm_set_ss(scale));
        recheck(0x105663u);
        store(0x105663u, 0x105668u, gp[ebp] + 0xcu, std::bit_cast<std::uint32_t>(_mm_cvtss_f32(s.xmm[0])));
        load(0x105668u, 0x10566eu, eax, gp[ebx] + 0x16cu, 1, 1);
        s.set(eax, flags.right(s.reg(eax, 1), 1, 1, false), 1);
        s.set(eax, flags.logic(s.reg(eax, 1) & 1u, 1), 1);
        store(0x105672u, 0x105675u, gp[ebp] + 8u, s.reg(eax, 1), 1);
        load(0x105675u, 0x10567bu, ecx, gp[ebx] + 0x16eu, 1);
        s.set(ecx, flags.right(s.reg(ecx, 1), 1, 1, false), 1);
        s.set(ecx, flags.logic(s.reg(ecx, 1) & 1u, 1), 1);
        store(0x105680u, 0x105683u, gp[ebp] + 9u, s.reg(ecx, 1), 1);
    }

    void leave() {
        restore(0x105938u, 0x105939u, edi);
        restore(0x105939u, 0x10593au, esi);
        restore(0x10593au, 0x10593bu, ebp);
        restore(0x10593bu, 0x10593cu, ebx);
        gp[esp] = flags.plus(gp[esp], 0x84u, 4);
        at(last);
        (void)s.pop();
        gp[esp] += 8u;
    }

    void prepare_magazine_bytes() {
        load_signed_halfword(0x1056d3u, 0x1056dbu, edx, gp[edx] + gp[ecx] + 0x224u);
        flags.logic(gp[edx], 4);
        store(0x1056ddu, 0x1056e2u, gp[esp] + 0x13u, 0, 1);
        if (!flags.zf && flags.sf == flags.of) {
            flags.minus(gp[edx], 6, 4);
            s.set(edx, 1, 1);
            if (flags.zf || flags.sf != flags.of)
                return;
        }
        load(0x1056ebu, 0x1056efu, edx, gp[esp] + 0x13u, 1);
    }

public:
    StatusFrame(State &state, const Cursor *cursor) : s(state), cursor(cursor) {}

    std::uint32_t begin() {
        gp[esp] = flags.minus(gp[esp], 0x84u, 4);
        load(0x1055f6u, 0x1055fdu, eax, gp[esp] + 0x88u);
        gp[eax] = flags.logic(gp[eax] & 0xffffu, 4);
        push(0x105602u, 0x105603u, gp[ebx]);
        push(0x105603u, 0x105604u, gp[ebp]);
        load(0x105604u, 0x10560bu, ebp, gp[esp] + 0x94u);
        push(0x10560bu, 0x10560cu, gp[esi]);
        gp[esi] = gp[eax] * 3u;
        load(0x10560fu, 0x105614u, eax, 0x4ecbd0u);
        load(0x105614u, 0x105617u, ecx, gp[eax] + 0x44u);
        load(0x105617u, 0x10561cu, eax, 0x4f0414u);
        gp[esi] = flags.shift(gp[esi], 2, 4);
        load(0x10561fu, 0x105623u, ebx, gp[ecx] + gp[esi] + 8u);
        load(0x105623u, 0x105625u, edx, gp[ebx]);
        gp[edx] = flags.logic(gp[edx] & 0xffffu, 4);
        gp[edx] = flags.shift(gp[edx], 4, 4);
        load(0x10562eu, 0x105632u, edx, gp[edx] + gp[eax] + 8u);
        gp[eax] = flags.logic(0, 4);
        push(0x105634u, 0x105635u, gp[edi]);
        clear_status();
        store_initial_fields();
        load(0x105683u, 0x10568au, eax, gp[edx] + 0x2c0u, 2, 2);
        store(0x10568au, 0x10568eu, gp[ebp] + 0x24u, s.reg(eax, 2), 2);
        load(0x10568eu, 0x105694u, ecx, gp[edx] + 0x2c0u);
        gp[eax] = flags.logic(0, 4);
        flags.minus(gp[ecx], gp[eax], 4);
        store(0x105698u, 0x10569cu, gp[esp] + 0x14u, gp[esi]);
        store(0x10569cu, 0x1056a0u, gp[esp] + 0x18u, gp[ebx]);
        store(0x1056a0u, 0x1056a4u, gp[esp] + 0x1cu, gp[edx]);
        store(0x1056a4u, 0x1056a8u, gp[esp] + 0x20u, gp[eax]);
        return flags.zf || flags.sf != flags.of ? reload_state : magazine_loop;
    }

    template<class Query>
    std::uint32_t magazines(Query query) {
        do {
            load(0x1056b0u, 0x1056b6u, esi, gp[edx] + 0x2c4u);
            load(0x1056b6u, 0x1056bcu, edx, 0x4ecbd0u);
            load(0x1056bcu, 0x1056bfu, edx, gp[edx] + 0x44u);
            gp[edi] = flags.product(gp[eax], 0x5cu);
            gp[edi] = flags.plus(gp[edi], gp[esi], 4);
            load(0x1056c6u, 0x1056cau, esi, gp[esp] + 0x14u);
            load(0x1056cau, 0x1056ceu, edx, gp[edx] + gp[esi] + 8u);
            gp[ecx] = flags.shift(gp[eax], 4, 4);
            prepare_magazine_bytes();
            gp[esi] = gp[eax] * 5u;
            store(0x1056f2u, 0x1056f6u, gp[ebp] + gp[esi] * 2u + 0x26u, s.reg(edx, 1), 1);
            flags.minus(read(0x1056f6u, gp[ecx] + gp[ebx] + 0x224u, 2), 0, 2);
            recheck(0x1056ffu);
            load(0x1056ffu, 0x105703u, ebx, gp[esp] + 0x14u);
            gp[esi] = gp[ebp] + gp[esi] * 2u;
            s.set(edx, flags.zf, 1);
            store(0x10570au, 0x10570du, gp[esi] + 0x27u, s.reg(edx, 1), 1);
            load(0x10570du, 0x105713u, edx, 0x4ecbd0u);
            load(0x105713u, 0x105716u, edx, gp[edx] + 0x44u);
            load(0x105716u, 0x10571au, edx, gp[edx] + gp[ebx] + 8u);
            load(0x10571au, 0x105722u, ecx, gp[edx] + gp[ecx] + 0x22cu, 2, 2);
            gp[ebx] = gp[eax] * 5u + 0x14u;
            store(0x105726u, 0x10572bu, gp[ebp] + gp[ebx] * 2u, s.reg(ecx, 2), 2);
            load(0x10572bu, 0x105732u, ecx, gp[esp] + 0x98u);
            load(0x105732u, 0x105736u, edx, gp[edi] + 0xau, 2, 2);
            push(0x105736u, 0x105738u, 0);
            push(0x105738u, 0x105739u, gp[ecx]);
            store(0x105739u, 0x10573du, gp[esi] + 0x2au, s.reg(edx, 2), 2);
            call(0x10573du, 0x105742u, query);
            load(0x105742u, 0x105746u, ebx, gp[esp] + 0x18u);
            store(0x105746u, 0x10574au, gp[esi] + 0x2cu, s.reg(eax, 2), 2);
            load(0x10574au, 0x10574eu, eax, gp[esp] + 0x20u);
            load(0x10574eu, 0x105752u, edx, gp[edi] + 0xcu, 2, 2);
            gp[eax] = flags.increment(gp[eax], 4, false);
            store(0x105753u, 0x105757u, gp[esi] + 0x2eu, s.reg(edx, 2), 2);
            load(0x105757u, 0x10575bu, edx, gp[esp] + 0x1cu);
            load(0x10575bu, 0x105761u, ecx, gp[edx] + 0x2c0u);
            store(0x105761u, 0x105765u, gp[esp] + 0x20u, gp[eax]);
            gp[eax] = signed_halfword(gp[eax]);
            flags.minus(gp[eax], gp[ecx], 4);
        } while (flags.sf != flags.of);
        load(0x105770u, reload_state, esi, gp[esp] + 0x14u);
        return reload_state;
    }

    std::uint32_t finish() {
        load(0x1058e6u, 0x1058ecu, edx, 0x4ecbd0u);
        load(0x1058ecu, 0x1058efu, eax, gp[edx] + 0x44u);
        load(0x1058efu, 0x1058f3u, esi, gp[eax] + gp[esi] + 8u);
        load(0x1058f3u, 0x1058f5u, ecx, gp[esi]);
        load(0x1058f5u, 0x1058fbu, edx, 0x4f0414u);
        gp[ecx] = flags.logic(gp[ecx] & 0xffffu, 4);
        gp[ecx] = flags.shift(gp[ecx], 4, 4);
        load(0x105904u, 0x105908u, eax, gp[ecx] + gp[edx] + 8u);
        load(0x105908u, 0x10590eu, ecx, gp[eax] + 0x2c8u);
        flags.logic(gp[ecx], 4);
        if (!flags.zf && flags.sf == flags.of) {
            load(0x105912u, 0x105918u, edx, gp[esi] + 0x20cu, 1, 1);
            load(0x105918u, 0x10591eu, eax, gp[eax] + 0x2ccu);
            gp[ecx] = 2;
            flags.minus(s.reg(edx, 1), s.reg(ecx, 1), 1);
            if (flags.zf) {
                flags.minus(read(0x105927u, gp[eax] + 6u, 2), s.reg(ecx, 2), 2);
                recheck(0x10592bu);
                if (flags.zf) {
                    flags.minus(read(0x10592du, gp[eax] + 0x18u, 2), 1, 2);
                    recheck(0x105932u);
                    if (flags.zf)
                        store(0x105934u, 0x105938u, gp[ebp] + 0x10u, 1, 1);
                }
            }
        }
        leave();
        return done;
    }
};

template<template<class> class Frame = StatusFrame, class State, class Phase>
std::uint32_t bridge(State &s, Phase phase) {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_WEAPON_HUD_STATUS_1055F0");
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
            if (pc >= first && pc <= last)
                return 0;
    }
    try {
        return phase(Frame<State>(s, cursor));
    } catch (Resume next) {
        return next.pc;
    }
}
} // namespace halo2::native_weapon_hud_status_detail
