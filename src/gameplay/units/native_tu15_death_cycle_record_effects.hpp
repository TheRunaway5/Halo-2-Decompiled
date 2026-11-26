#pragma once

#include "gameplay/units/native_tu15_death_cycle_motion.hpp"

#include <array>
#include <bit>

namespace halo2::native_death_cycle_record_effects {

using namespace native_death_cycle_opening;
using native_death_cycle_motion::update_field;

enum class Result { retained, next_effect, remove_effect, reset_effect, expire };

struct LinkSites {
    std::array<std::uint32_t, 19> pc;
    std::uint32_t counterpart_offset;
    unsigned index_reg, table_reg, address_reg;
};

inline constexpr LinkSites primary_link{{
    0x103b18u, 0x103b1au, 0x103b1eu, 0x103b22u, 0x103b28u, 0x103b2au, 0x103b2bu,
    0x103b2cu, 0x103b31u, 0x103b33u, 0x103b39u, 0x103b3du, 0x103b42u, 0x103b45u,
    0x103b48u, 0x103b4cu, 0x103b50u, 0x103b58u, 0x103b5fu}, 8u, edx, eax, ecx};
inline constexpr LinkSites secondary_link{{
    0x103b61u, 0x103b63u, 0x103b67u, 0x103b6bu, 0x103b6du, 0x103b6fu, 0x103b70u,
    0x103b71u, 0x103b76u, 0x103b78u, 0x103b7au, 0x103b7eu, 0x103b84u, 0x103b87u,
    0x103b8au, 0x103b8eu, 0x103b92u, 0x103b9au, 0x103ba1u}, 0xau, ecx, edx, eax};
inline constexpr LinkSites secondary_removal_link{{
    0x103bf5u, 0x103bf7u, 0x103bfbu, 0x103bffu, 0x103c05u, 0x103c07u, 0x103c08u,
    0x103c09u, 0x103c0eu, 0x103c10u, 0x103c16u, 0x103c1au, 0x103c20u, 0x103c23u,
    0x103c26u, 0x103c2au, 0x103c2eu, 0x103c36u, 0x103c3du}, 0xau, eax, ecx, edx};
inline constexpr LinkSites primary_removal_link{{
    0x103c3fu, 0x103c41u, 0x103c45u, 0x103c49u, 0x103c4bu, 0x103c4du, 0x103c4eu,
    0x103c4fu, 0x103c54u, 0x103c56u, 0x103c58u, 0x103c5cu, 0x103c61u, 0x103c64u,
    0x103c67u, 0x103c6bu, 0x103c6fu, 0x103c77u, 0x103c7eu}, 8u, edx, eax, ecx};

inline bool reference() {
    static const bool enabled = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_DEATH_CYCLE_RECORD_EFFECTS");
        return value && *value;
    }();
    return enabled;
}

template<class State, class Dispatch, class Branch>
struct EffectsFrame : Frame<State> {
    using Base = Frame<State>;
    using Base::s, Base::g, Base::flags, Base::site, Base::access;
    using Base::read, Base::push, Base::field, Base::constant;
    Dispatch &dispatch;
    Branch &branch;

    EffectsFrame(State &state, Dispatch &call, Branch &target) : Base{state}, dispatch(call), branch(target) {}

    auto stack(std::uint32_t offset) { return field(esp, offset); }
    auto record(std::uint32_t offset) { return field(esi, offset); }
    bool above() const { return !flags.cf && !flags.zf; }
    static std::uint32_t signed_word(std::uint32_t value) {
        return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));
    }

    void call(std::uint32_t pc, std::uint32_t target) {
        push(pc, constant(pc+5u));
        dispatch(s, target, pc);
    }

    bool local_enabled(unsigned reg, std::uint32_t offset,
                       std::uint32_t move, std::uint32_t test, std::uint32_t branch) {
        set_low_byte(g[reg], read(move, stack(offset), 1));
        site(test);
        flags.logic(low_byte(g[reg]), 1);
        site(branch);
        return !flags.zf;
    }

    bool returned_true(std::uint32_t test, std::uint32_t branch) {
        site(test);
        flags.logic(low_byte(g[eax]), 1);
        site(branch);
        return !flags.zf;
    }

    void set_status(std::uint32_t pc, std::uint32_t mask) {
        access(pc);
        update_field(s, pc, g[ebx]+g[ebp]+0x210u, 4, 0xffffffffu, mask);
    }

    template<const LinkSites &layout>
    void mark_linked_record() {
        const auto &p = layout.pc;
        const auto index = layout.index_reg, table = layout.table_reg, data = layout.address_reg;
        site(p[0]);
        g[eax] = flags.logic(0u, 4);
        g[eax] = read(p[1], record(layout.counterpart_offset), 2);
        site(p[2]);
        flags.minus(g[eax], 0xffffu, 2);
        site(p[3]);
        if (flags.zf) return;
        push(p[4], constant(0u));
        push(p[5], field(eax));
        push(p[6], field(edi));
        call(p[7], 0x107610u);
        if (!returned_true(p[8], p[9])) return;
        g[index] = signed_word(read(p[10], record(layout.counterpart_offset), 2));
        g[table] = read(p[11], constant(0x4ecbd0u));
        site(p[12]);
        g[index] = flags.product(g[index], 0x34u);
        g[data] = read(p[13], field(table, 0x44u));
        g[table] = read(p[14], stack(0x24u));
        g[data] = read(p[15], [&] { return g[data]+g[table]+8u; });
        access(p[16]);
        update_field(s, p[16], g[data]+g[index]+0x1a8u, 1, 0xffu, 1u);
        site(p[17]);
        g[eax] = g[data]+g[index]+0x1a4u;
        site(p[18]);
    }

    Result set_match_effect() {
        flags.minus(read(0x103a95u, record(0x18u), 2), 1u, 2);
        site(0x103a9au);
        if (!flags.zf) return Result::expire;

        g[esi] = read(0x103aa0u, stack(0x28u));
        g[edi] = read(0x103aa4u, stack(0x4cu));
        push(0x103aa8u, field(esi));
        site(0x103aa9u);
        g[eax] = g[edi];
        call(0x103aabu, 0x108b60u);
        if (!returned_true(0x103ab0u, 0x103ab2u)) return Result::expire;
        push(0x103ab8u, field(esi));
        site(0x103ab9u);
        g[ecx] = g[edi];
        call(0x103abbu, 0x108db0u);
        set_status(0x103ac0u, 0x40u);
        site(0x103ac8u);
        return Result::expire;
    }

    void query_effects() {
        g[edi] = read(0x103ad6u, stack(0x4cu));
        site(0x103adau);
        g[edx] = g[esp]+0x1au;
        push(0x103adeu, field(edx));
        site(0x103adfu);
        g[ecx] = g[esp]+0x1du;
        push(0x103ae3u, field(ecx));
        site(0x103ae4u);
        g[edx] = g[esp]+0x20u;
        push(0x103ae8u, field(edx));
        site(0x103ae9u);
        g[ecx] = g[edi];
        call(0x103aebu, 0x107880u);
    }

    void apply_current_effect() {
        if (!local_enabled(eax, 0x18u, 0x103af0u, 0x103af4u, 0x103af6u)) return;
        set_status(0x103afcu, 8u);
        g[eax] = signed_word(read(0x103b04u, record(0x1au), 2));
        site(0x103b08u);
        flags.minus(g[eax], 3u, 4);
        site(0x103b0bu);
        if (above()) return;

        switch (branch(read(0x103b11u, [&] { return 0x104684u+g[eax]*4u; }), s.image)) {
            case 0x103b18u: mark_linked_record<primary_link>(); return;
            case 0x103b61u: mark_linked_record<secondary_link>(); return;
            case 0x103ba3u:
                g[ecx] = read(0x103ba3u, stack(0x28u));
                push(0x103ba7u, field(ecx));
                site(0x103ba8u);
                g[eax] = g[edi];
                call(0x103baau, 0x108b60u);
                if (!returned_true(0x103bafu, 0x103bb1u)) return;
                g[edx] = read(0x103bb3u, stack(0x28u));
                push(0x103bb7u, field(edx));
                site(0x103bb8u);
                g[ecx] = g[edi];
                call(0x103bbau, 0x108db0u);
                site(0x103bbfu);
                return;
            case 0x103bc1u:
                g[eax] = read(0x103bc1u, stack(0x28u));
                push(0x103bc5u, field(eax));
                site(0x103bc6u);
                g[eax] = g[edi];
                call(0x103bc8u, 0x108be0u);
                return;
            default: throw std::runtime_error("Required original solver dispatch target unavailable");
        }
    }

    void remove_current_effect() {
        if (!local_enabled(eax, 0x19u, 0x103bcdu, 0x103bd1u, 0x103bd3u)) return;
        set_status(0x103bd9u, 0x10u);
        g[eax] = signed_word(read(0x103be1u, record(0x18u), 2));
        site(0x103be5u);
        flags.minus(g[eax], 3u, 4);
        site(0x103be8u);
        if (above()) return;

        switch (branch(read(0x103beeu, [&] { return 0x104694u+g[eax]*4u; }), s.image)) {
            case 0x103bf5u: mark_linked_record<secondary_removal_link>(); return;
            case 0x103c3fu: mark_linked_record<primary_removal_link>(); return;
            case 0x103c80u:
                g[edx] = read(0x103c80u, stack(0x28u));
                push(0x103c84u, field(edx));
                site(0x103c85u);
                g[eax] = g[edi];
                call(0x103c87u, 0x108b60u);
                if (!returned_true(0x103c8cu, 0x103c8eu)) return;
                g[eax] = read(0x103c90u, stack(0x28u));
                push(0x103c94u, field(eax));
                site(0x103c95u);
                g[ecx] = g[edi];
                call(0x103c97u, 0x108db0u);
                site(0x103c9cu);
                return;
            case 0x103c9eu:
                flags.minus(read(0x103c9eu, stack(0x30u)), 0xffffffffu, 4);
                site(0x103ca3u);
                g[eax] = g[edi];
                site(0x103ca5u);
                if (!flags.zf) {
                    g[ecx] = read(0x103ca7u, stack(0x28u));
                    push(0x103cabu, field(ecx));
                    call(0x103cacu, 0x108be0u);
                    site(0x103cb1u);
                } else {
                    g[edx] = read(0x103cb3u, stack(0x28u));
                    push(0x103cb7u, field(edx));
                    call(0x103cb8u, 0x108ca0u);
                }
                return;
            default: throw std::runtime_error("Required original solver dispatch target unavailable");
        }
    }

    Result run() {
        if (local_enabled(ecx, 0x16u, 0x103a6du, 0x103a71u, 0x103a73u)) return Result::reset_effect;
        set_low_byte(g[ecx], read(0x103a79u, field(ebp, 0x170u), 1));
        site(0x103a7fu);
        flags.logic(low_byte(g[ecx]), 1);
        site(0x103a81u);
        bool has_effect = above();
        if (!has_effect) {
            set_low_byte(g[ecx], read(0x103a83u, field(ebp, 0x171u), 1));
            site(0x103a89u);
            flags.logic(low_byte(g[ecx]), 1);
            site(0x103a8bu);
            has_effect = above();
        }
        if (!has_effect) {
            if (local_enabled(eax, 0x15u, 0x103a8du, 0x103a91u, 0x103a93u)) return set_match_effect();
            g[edi] = read(0x103acdu, stack(0x4cu));
            site(0x103ad1u);
            return Result::remove_effect;
        }
        query_effects();
        apply_current_effect();
        remove_current_effect();
        return Result::next_effect;
    }
};

template<class State, class Dispatch, class Branch>
Result run(State &s, Dispatch dispatch, Branch branch) {
    if (!selected(s) || reference()) return Result::retained;
    return EffectsFrame<State, Dispatch, Branch>{s, dispatch, branch}.run();
}

} // namespace halo2::native_death_cycle_record_effects
