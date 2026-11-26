#pragma once

#include "physics/runtime/native_tu15_portal_predicate_leaves.hpp"
#include "physics/runtime/native_tu15_required_owner_policy.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

#include <bit>
#include <cstdint>
#include <cstdlib>
#include <type_traits>

namespace halo2::native_contrail_point_update_detail {

inline constexpr std::uint32_t none = 0xffffffffu;
inline constexpr unsigned eax = 0, ecx = 1, edx = 2, ebx = 3, esp = 4, ebp = 5, esi = 6, edi = 7;
inline constexpr unsigned expired = 4u, finishes_segment = 2u, just_created = 1u;
inline constexpr std::uint32_t contrails = 0x4f7214u, points = 0x4f7210u, tags = 0x4f0414u;
inline constexpr std::uint32_t zero = 0x46a0e8u, one = 0x46a0d0u, effect_scale = 0x46a0ccu;
inline constexpr unsigned point_stride = 0x38u, segment_stride = 0x40u;
inline constexpr unsigned frame_bytes = 0x101cu;
inline constexpr unsigned count_address = 0x14u, head_address = 0x18u, current_point = 0x1cu;
inline constexpr unsigned chain_number = 0x20u, chains_remaining = 0x24u, contrail_address = 0x28u;
inline constexpr unsigned visited_handles = 0x2cu, elapsed_argument = 0x1034u, transition_spill = 0x10u;

// Both transitions store the real curve child's x87 result and recompute
// the point's reciprocal rate at the same authored rounding boundaries.
struct Transition {
    std::uint32_t spill, zero_read, bits_read, value_read, rate_write;
    std::uint32_t compare, discard, reciprocal, reciprocal_write, flag_write;
    bool finish;
};
inline constexpr Transition end_segment{
    0x17f4f0u, 0x17f4f4u, 0x17f4fau, 0x17f4feu, 0x17f502u,
    0x17f505u, 0x17f507u, 0x17f50fu, 0x17f515u, 0x17f51au, true};
inline constexpr Transition start_segment{
    0x17f55cu, 0x17f560u, 0x17f566u, 0x17f56au, 0x17f56eu,
    0x17f571u, 0x17f573u, 0x17f57bu, 0x17f581u, 0x17f586u, false};

template<class T> std::uint32_t signed_operand(std::uint32_t value) {
    using U = std::make_unsigned_t<T>;
    return std::uint32_t(std::int32_t(std::bit_cast<T>(static_cast<U>(value))));
}

template<class State, class Dispatch, class Effect, class Spill>
class PointUpdate {
    State &s;
    std::uint32_t &cursor;
    Dispatch &dispatch;
    Effect &effect;
    Spill &spill;

    void at(std::uint32_t pc) { cursor = pc; }
    std::uint32_t read(std::uint32_t pc, std::uint32_t address, unsigned width = 4) {
        at(pc);
        return physics_step_detail::r(s.memory, address, width);
    }
    void write(std::uint32_t pc, std::uint32_t address, std::uint32_t value, unsigned width = 4) {
        at(pc);
        physics_step_detail::w(s.memory, s.observe, pc, address, width, value);
    }
    __m128 scalar(std::uint32_t pc, std::uint32_t address) {
        return _mm_set_ss(std::bit_cast<float>(read(pc, address)));
    }
    std::uint32_t bits(unsigned lane) const {
        return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(s.xmm[lane]));
    }
    std::uint32_t local(std::uint32_t pc, unsigned offset) {
        return read(pc, s.gp[esp] + offset);
    }
    void save(std::uint32_t pc, unsigned offset, std::uint32_t value) {
        write(pc, s.gp[esp] + offset, value);
    }
    void push(std::uint32_t pc, std::uint32_t value) {
        at(pc);
        s.push(pc, value);
    }
    std::uint32_t pop(std::uint32_t pc) {
        at(pc);
        return s.pop();
    }
    void call(std::uint32_t pc, std::uint32_t returned, std::uint32_t target) {
        push(pc, returned);
        dispatch(s, target, pc);
    }

    void open_frame() {
        auto &g = s.gp;
        auto &f = s.flags;
        g[eax] = frame_bytes;
        call(0x17f3d5u, 0x17f3dau, 0x32a660u);
        g[eax] = local(0x17f3dau, frame_bytes + 4u);
        g[ecx] = read(0x17f3e1u, contrails);
        g[edx] = read(0x17f3e7u, g[ecx] + 0x44u);
        g[eax] = f.logic(g[eax] & 0xffffu, 4) * 9u;
        g[ecx] = read(0x17f3f2u, g[edx] + g[eax] * 8u + 4u);
        g[eax] = g[edx] + g[eax] * 8u;
        g[edx] = read(0x17f3f9u, tags);
        push(0x17f3ffu, g[ebx]);
        g[ebx] = read(0x17f400u, points);
        g[ecx] = f.shift(f.logic(g[ecx] & 0xffffu, 4), 4, 4);
        push(0x17f40fu, g[ebp]);
        g[ebp] = read(0x17f410u, g[ecx] + g[edx] + 8u);
        g[ecx] = g[eax] + 0x30u;
        // ESI and EDI aren't on the stack yet.
        save(0x17f417u, contrail_address - 8u, g[eax]);
        push(0x17f41bu, g[esi]);
        g[eax] = f.plus(g[eax], 0x38u, 4);
        push(0x17f41fu, g[edi]);
        save(0x17f420u, chain_number, 0);
        save(0x17f428u, count_address, g[ecx]);
        save(0x17f42cu, head_address, g[eax]);
        save(0x17f430u, chains_remaining, 4);
    }

    bool begin_chain() {
        auto &g = s.gp;
        g[eax] = read(0x17f438u, g[eax]);
        g[edi] = s.flags.logic(0, 4);
        s.flags.minus(g[eax], none, 4);
        save(0x17f43fu, current_point, g[eax]);
        return !s.flags.zf;
    }

    bool load_point() {
        auto &g = s.gp;
        auto &f = s.flags;
        g[esi] = local(0x17f450u, current_point);
        g[ecx] = read(0x17f454u, g[ebx] + 0x44u);
        g[esi] = f.product(f.logic(g[esi] & 0xffffu, 4), point_stride);
        s.set(eax, read(0x17f460u, g[esi] + g[ecx] + 2u, 1), 1);
        g[esi] = f.plus(g[esi], g[ecx], 4);
        return f.logic(s.reg(eax, 1) & expired, 1) == 0;
    }

    void advance_phase() {
        auto &x = s.xmm;
        x[0] = scalar(0x17f46eu, s.gp[esi] + 8u);
        x[0] = native_tu15_leaves::ordered_mul_ss(x[0], scalar(0x17f473u, s.gp[esp] + elapsed_argument));
        x[0] = native_tu15_leaves::ordered_add_ss(x[0], scalar(0x17f47cu, s.gp[esi] + 4u));
        write(0x17f481u, s.gp[esi] + 4u, bits(0));
    }

    bool nonzero_or_unordered() {
        s.set(eax, s.flags.bits() | 2u, 1, 8);
        s.flags.logic(s.reg(eax, 1, 8) & 0x44u, 1);
        return s.flags.pf;
    }

    void use_transition(const Transition &sites) {
        auto &g = s.gp;
        auto &x = s.xmm;
        at(sites.spill);
        const auto rate = spill(s.fp.st(0), s.fp.status);
        save(sites.spill, transition_spill, rate);
        s.fp.pop();
        s.fp.push(physics_step_detail::ext(std::bit_cast<float>(read(sites.zero_read, zero))));
        g[eax] = local(sites.bits_read, transition_spill);
        s.fp.push(physics_step_detail::ext(std::bit_cast<float>(local(sites.value_read, transition_spill))));
        write(sites.rate_write, g[esi] + 8u, g[eax]);
        at(sites.compare);
        s.unordered_compare_and_pop(s.fp.st(1));
        at(sites.discard);
        (void)s.fp.st(0);
        s.fp.pop();
        if (nonzero_or_unordered()) {
            x[3] = _mm_div_ss(x[3], scalar(sites.reciprocal, g[esp] + transition_spill));
            write(sites.reciprocal_write, g[esi] + 8u, bits(3));
        }
        const auto flags = read(sites.flag_write, g[esi] + 2u, 1);
        write(sites.flag_write, g[esi] + 2u,
            s.flags.logic(sites.finish ? flags & 0xfdu : flags | finishes_segment, 1), 1);
    }

    void finish_segment() {
        auto &g = s.gp;
        auto &f = s.flags;
        s.set(ecx, read(0x17f4bdu, g[esi] + 3u, 1), 1);
        g[edx] = read(0x17f4c0u, g[ebp] + 0xecu);
        s.xmm[2] = scalar(0x17f4c6u, g[esi] + 0xcu);
        g[eax] = f.increment(signed_operand<std::int8_t>(s.reg(ecx, 1)), 4, false);
        g[eax] = f.plus(f.shift(g[eax], 6, 4), g[edx], 4);
        s.set(ecx, f.increment(s.reg(ecx, 1), 1, false), 1);
        write(0x17f4d6u, g[esi] + 3u, s.reg(ecx, 1), 1);
        write(0x17f4d9u, g[esi] + 4u, bits(0));
        g[edx] = read(0x17f4deu, g[eax] + 4u);
        s.xmm[1] = scalar(0x17f4e1u, g[eax]);
        push(0x17f4e5u, 0);
        push(0x17f4e7u, g[edx]);
        g[edx] = read(0x17f4e8u, g[eax] + 0x3cu);
        call(0x17f4ebu, 0x17f4f0u, 0x17ed80u);
        use_transition(end_segment);
    }

    bool begin_segment() {
        auto &g = s.gp;
        auto &f = s.flags;
        g[eax] = signed_operand<std::int8_t>(read(0x17f523u, g[esi] + 3u, 1));
        g[edx] = read(0x17f527u, g[ebp] + 0xe8u);
        g[ecx] = g[eax] + 1u;
        f.minus(g[ecx], g[edx], 4);
        if (f.sf == f.of) {
            constexpr std::uint32_t expire_pc = 0x17f58fu;
            const auto flags = read(expire_pc, g[esi] + 2u, 1);
            write(expire_pc, g[esi] + 2u, f.logic(flags | expired, 1), 1);
            return false;
        }
        g[ecx] = read(0x17f534u, g[ebp] + 0xecu);
        s.xmm[2] = scalar(0x17f53au, g[esi] + 0xcu);
        g[eax] = f.plus(f.shift(g[eax], 6, 4), g[ecx], 4);
        write(0x17f544u, g[esi] + 4u, bits(0));
        g[edx] = read(0x17f549u, g[eax] + 0xcu);
        s.xmm[1] = scalar(0x17f54cu, g[eax] + 8u);
        push(0x17f551u, 2);
        push(0x17f553u, g[edx]);
        g[edx] = read(0x17f554u, g[eax] + 0x3cu);
        call(0x17f557u, 0x17f55cu, 0x17ed80u);
        use_transition(start_segment);
        return true;
    }

    bool transition_if_ready() {
        auto &x = s.xmm;
        auto &f = s.flags;
        x[1] = scalar(0x17f490u, s.gp[esi] + 8u);
        x[0] = _mm_setzero_ps();
        x[3] = scalar(0x17f498u, one);
        at(0x17f4a0u);
        f.scalar_compare(x[1], x[0], true);
        if (nonzero_or_unordered()) {
            x[1] = scalar(0x17f4a9u, s.gp[esi] + 4u);
            at(0x17f4aeu);
            f.scalar_compare(x[1], x[3], false);
            if (f.cf || f.zf) return false;
        }
        const auto flags = read(0x17f4b7u, s.gp[esi] + 2u, 1);
        if (!f.logic(flags & finishes_segment, 1))
            return begin_segment();
        finish_segment();
        return true;
    }

    void point_effect() {
        auto &g = s.gp;
        auto &f = s.flags;
        s.set(eax, read(0x17f593u, g[esi] + 2u, 1), 1);
        if (f.logic(s.reg(eax, 1) & just_created, 1)) {
            s.set(eax, f.logic(s.reg(eax, 1) & 0xfeu, 1), 1);
            write(0x17f59cu, g[esi] + 2u, s.reg(eax, 1), 1);
            return;
        }
        if (f.logic(s.reg(eax, 1) & expired, 1)) return;
        g[eax] = signed_operand<std::int8_t>(read(0x17f5a5u, g[esi] + 3u, 1));
        g[ecx] = read(0x17f5a9u, g[ebp] + 0xecu);
        g[eax] = f.plus(f.shift(g[eax], 6, 4), g[ecx], 4);
        g[ecx] = read(0x17f5b4u, g[eax] + 0x14u);
        f.minus(g[ecx], none, 4);
        if (f.zf) return;

        g[edx] = local(0x17f5bcu, elapsed_argument);
        s.xmm[0] = scalar(0x17f5c3u, g[eax] + 0x18u);
        s.xmm[0] = native_tu15_leaves::ordered_mul_ss(s.xmm[0], scalar(0x17f5c8u, effect_scale));
        push(0x17f5d0u, g[edx]);
        push(0x17f5d1u, g[ecx]);
        save(0x17f5d2u, 0, bits(0));
        push(0x17f5d7u, 0);
        push(0x17f5d9u, 0);
        push(0x17f5dbu, 0);
        g[edx] = g[esi] + 0x1cu;
        push(0x17f5e0u, g[edx]);
        push(0x17f5e1u, none);
        g[edx] = g[esi] + 0x14u;
        push(0x17f5e6u, g[edx]);
        g[edx] = read(0x17f5e7u, tags);
        g[ecx] = f.shift(f.logic(g[ecx] & 0xffffu, 4), 4, 4);
        g[ecx] = read(0x17f5f6u, g[ecx] + g[edx] + 8u);
        push(0x17f5fau, g[ecx]);
        g[eax] = g[esi] + 0x28u;
        push(0x17f5feu, 0);
        push(0x17f600u, 0x17f605u);
        effect(s);
        g[ebx] = read(0x17f605u, points);
    }

    bool next_point() {
        auto &g = s.gp;
        auto &f = s.flags;
        g[esi] = read(0x17f60bu, g[esi] + 0x34u);
        g[eax] = local(0x17f60eu, current_point);
        g[edx] = signed_operand<std::int16_t>(g[edi]);
        g[edi] = f.increment(g[edi], 4, false);
        f.minus(g[esi], none, 4);
        save(0x17f619u, visited_handles + g[edx] * 4u, g[eax]);
        save(0x17f61du, current_point, g[esi]);
        return !f.zf;
    }

    bool remove_expired_pair() {
        auto &g = s.gp;
        auto &f = s.flags;
        g[esi] = read(0x17f630u, g[ebx] + 0x44u);
        g[edi] = f.increment(g[edi], 4, true);
        g[edx] = signed_operand<std::int16_t>(g[edi]);
        g[eax] = local(0x17f637u, visited_handles + g[edx] * 4u);
        g[edx] = g[esp] + visited_handles + g[edx] * 4u;
        g[edx] = read(0x17f63fu, g[edx] - 4u);
        g[ecx] = f.logic(g[eax] & 0xffffu, 4);
        g[edx] = f.logic(g[edx] & 0xffffu, 4);
        g[ecx] = f.product(g[ecx], point_stride);
        g[edx] = f.product(g[edx], point_stride);
        g[ecx] = f.plus(g[ecx], g[esi], 4);
        g[edx] = f.plus(g[edx], g[esi], 4);
        if (!f.logic(read(0x17f65au, g[ecx] + 2u, 1) & expired, 1)) return false;
        if (!f.logic(read(0x17f660u, g[edx] + 2u, 1) & expired, 1)) return false;
        f.minus(read(0x17f666u, g[ecx] + 0x34u), none, 4);
        if (!f.zf) return false;

        g[ecx] = local(0x17f66cu, chain_number);
        write(0x17f670u, g[edx] + 0x34u, none);
        g[edx] = local(0x17f677u, contrail_address);
        constexpr std::uint32_t count_pc = 0x17f67bu;
        const auto count = g[edx] + 0x30u + g[ecx] * 2u;
        write(count_pc, count, f.increment(read(count_pc, count, 2), 2, true), 2);
        g[edx] = g[ebx];
        call(0x17f682u, 0x17f687u, 0x16e8a0u);
        return multiple_points();
    }

    bool multiple_points() {
        s.flags.minus(s.reg(edi, 2), 1, 2);
        return !s.flags.zf && s.flags.sf == s.flags.of;
    }

    void remove_last_point() {
        auto &g = s.gp;
        auto &f = s.flags;
        g[ecx] = local(0x17f68du, count_address);
        f.minus(read(0x17f691u, g[ecx], 2), 1, 2);
        if (!f.zf) return;
        g[edx] = local(0x17f697u, head_address);
        g[eax] = read(0x17f69bu, g[edx]);
        g[edx] = read(0x17f69du, g[ebx] + 0x44u);
        g[ecx] = f.product(f.logic(g[eax] & 0xffffu, 4), point_stride);
        if (!f.logic(read(0x17f6abu, g[ecx] + g[edx] + 2u, 1) & expired, 1)) return;

        g[edx] = g[ebx];
        call(0x17f6b4u, 0x17f6b9u, 0x16e8a0u);
        g[eax] = local(0x17f6b9u, head_address);
        g[ecx] = local(0x17f6bdu, count_address);
        write(0x17f6c1u, g[eax], none);
        write(0x17f6c7u, g[ecx], 0, 2);
    }

    bool next_chain() {
        auto &g = s.gp;
        auto &f = s.flags;
        g[edi] = local(0x17f6ccu, chain_number);
        g[eax] = local(0x17f6d0u, head_address);
        g[edx] = local(0x17f6d4u, count_address);
        g[ecx] = local(0x17f6d8u, chains_remaining);
        g[edi] = f.increment(g[edi], 4, false);
        g[eax] = f.plus(g[eax], 4, 4);
        g[edx] = f.plus(g[edx], 2, 4);
        g[ecx] = f.increment(g[ecx], 4, true);
        save(0x17f6e4u, chain_number, g[edi]);
        save(0x17f6e8u, head_address, g[eax]);
        save(0x17f6ecu, count_address, g[edx]);
        save(0x17f6f0u, chains_remaining, g[ecx]);
        return !f.zf;
    }

    void close_frame() {
        auto &g = s.gp;
        g[edi] = pop(0x17f6fau);
        g[esi] = pop(0x17f6fbu);
        g[ebp] = pop(0x17f6fcu);
        g[ebx] = pop(0x17f6fdu);
        g[esp] = s.flags.plus(g[esp], frame_bytes, 4);
        pop(0x17f704u);
        g[esp] += 8u;
    }

public:
    PointUpdate(State &state, std::uint32_t &position, Dispatch &route, Effect &spawn, Spill &round)
        : s(state), cursor(position), dispatch(route), effect(spawn), spill(round) {}

    void run() {
        open_frame();
        do {
            if (begin_chain()) {
                do {
                    if (load_point()) {
                        advance_phase();
                        while (transition_if_ready()) {}
                    }
                    point_effect();
                } while (next_point());
                if (multiple_points())
                    while (remove_expired_pair()) {}
            }
            remove_last_point();
        } while (next_chain());
        close_frame();
    }
};

template<class State, class Dispatch, class Effect, class Spill>
bool bridge(State &s, Dispatch dispatch, Effect effect, Spill spill) {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_CONTRAIL_POINT_UPDATE");
        return value && *value;
    }();
    const auto *cursor = s.immutable_cursor.cursor;
    if (reference || s.image != SourcePhysicsContactImage::TitleUpdate15 || !cursor ||
        !cursor->position || (cursor->first_trace && *cursor->first_trace) ||
        (cursor->second_trace && *cursor->second_trace) ||
        (s.observe && !cursor->observation_only_stores) ||
        !native_required_owner_policy_detail::preserves_instruction_policy(s.services))
        return false;
    PointUpdate{s, *cursor->position, dispatch, effect, spill}.run();
    return true;
}

} // namespace halo2::native_contrail_point_update_detail
