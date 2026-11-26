#pragma once

#include "gameplay/units/native_tu15_death_cycle_expiration.hpp"

namespace halo2::native_death_cycle_timer_tail {

using namespace native_death_cycle_opening;
using native_death_cycle_expiration::subtract;
using native_tu15_leaves::ordered_mul_ss;
using native_world_leaves_detail::ordered_add_ss;

enum class Result { retained, next_record, returned };

inline bool reference() {
    static const bool enabled = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_DEATH_CYCLE_TIMER_TAIL");
        return value && *value;
    }();
    return enabled;
}

template<bool Multiply, class State>
void scale_extended(State &s, std::uint32_t bits) {
    auto &top = s.fp.st(0);
    long double result;
    std::uint16_t status;
    if constexpr (Multiply) {
        asm volatile("fldt %2\n\tfmuls %3\n\tfnstsw %1\n\tfstpt %0"
            : "=m"(result), "=m"(status) : "m"(top), "m"(bits) : "st");
    } else {
        asm volatile("fldt %2\n\tfadds %3\n\tfnstsw %1\n\tfstpt %0"
            : "=m"(result), "=m"(status) : "m"(top), "m"(bits) : "st");
    }
    top = result;
    s.fp.status = (s.fp.status & ~0x200u) | (status & 0x200u);
}

template<class State>
void load_extended_constant(State &s, std::uint32_t address) {
    std::array<std::byte, 8> bytes;
    s.memory.read(address, bytes);
    long double result;
    asm volatile("fldl %1\n\tfstpt %0" : "=m"(result) : "m"(bytes) : "st");
    s.fp.push(result);
}

template<class State>
void pop_extended(State &s) {
    s.fp.pop();
    s.fp.status &= ~0x200u;
}

template<class State>
void compare_extended(State &s) {
    s.compare_and_pop(s.fp.st(1));
    s.fp.status &= ~0x200u;
}

template<class State>
struct TimerTailFrame : native_death_cycle_expiration::ExpirationFrame<State> {
    using Base = native_death_cycle_expiration::ExpirationFrame<State>;
    using Base::s, Base::g, Base::x, Base::flags, Base::site, Base::access;
    using Base::read, Base::store, Base::push, Base::field, Base::constant;
    using Base::stack, Base::scalar_bits;

    explicit TimerTailFrame(State &state) : Base{state} {}

    template<class Address>
    __m128 scalar(std::uint32_t pc, Address address) {
        return _mm_set_ss(f(read(pc, address)));
    }

    template<class Address>
    void load_single(std::uint32_t pc, Address address) {
        s.fp.push(native_live_support::x87::load_single(read(pc, address)));
    }

    void load_minimum(std::uint32_t pc) {
        access(pc);
        load_extended_constant(s, 0x46a0f0u);
    }

    template<class Dispatch>
    void update_quantized_timer(Dispatch &dispatch) {
        x[0] = scalar(0x1044f4u, field(esi, 0x10u));
        site(0x1044f9u);
        x[1] = _mm_setzero_ps();
        site(0x1044fcu);
        flags.scalar_compare(x[0], x[1], false);
        site(0x1044ffu);
        if (flags.cf || flags.zf) return;

        g[ebx] = read(0x104501u, stack(0x34u));
        g[eax] = read(0x104505u, field(ebx, 0x175u), 1);
        x[2] = scalar(0x10450cu, field(ebp, 0x14u));
        site(0x104511u);
        flags.scalar_compare(x[2], x[1], false);
        site(0x104514u);
        x[0] = _mm_cvtsi32_ss(x[0], std::bit_cast<std::int32_t>(g[eax]));
        const auto byte_scale = scalar(0x104518u, constant(0x46a0c4u));
        x[0] = ordered_mul_ss(x[0], byte_scale);
        store(0x104520u, stack(0x40u), scalar_bits(0));
        x[0] = scalar(0x104526u, field(esi, 0x10u));
        const auto timer_scale = scalar(0x10452bu, constant(0x449ea4u));
        x[0] = ordered_mul_ss(x[0], timer_scale);
        store(0x104533u, stack(0x30u), scalar_bits(0));
        site(0x104539u);
        if (!flags.cf && !flags.zf) {
            site(0x10453bu);
            x[0] = ordered_mul_ss(x[0], x[2]);
            store(0x10453fu, stack(0x30u), scalar_bits(0));
        }

        load_single(0x104545u, stack(0x30u));
        scale_extended<false>(s, read(0x104549u, stack(0x40u)));
        scale_extended<true>(s, read(0x10454du, constant(0x46a148u)));
        push(0x104553u, constant(0x104558u));
        dispatch(s, 0x32cc0cu, 0x104553u);
        store(0x104558u, field(ebx, 0x175u), field(eax), 1);
    }

    template<class Dispatch>
    bool notify_record(Dispatch &dispatch) {
        set_low_byte(g[eax], read(0x10455eu, stack(0x15u), 1));
        site(0x104562u);
        flags.logic(low_byte(g[eax]), 1);
        site(0x104564u);
        if (flags.zf) return false;

        g[eax] = read(0x104566u, stack(0x20u));
        g[ecx] = read(0x10456au, stack(0x4cu));
        push(0x10456eu, constant(0x104573u));
        dispatch(s, 0x102fb0u, 0x10456eu);
        site(0x104573u);
        x[1] = _mm_setzero_ps();
        site(0x104576u);
        return true;
    }

    void decay_recovery_timer() {
        x[2] = scalar(0x10458cu, field(ebp, 0xc8u));
        load_minimum(0x104594u);
        g[eax] = read(0x10459au, constant(0x51df54u));
        const auto step = scalar(0x10459fu, field(eax, 4u));
        x[2] = ordered_mul_ss(x[2], step);
        x[0] = scalar(0x1045a4u, field(esi, 0x20u));
        site(0x1045a9u);
        x[0] = subtract(x[0], x[2]);
        store(0x1045adu, stack(0x1cu), scalar_bits(0));
        load_single(0x1045b3u, stack(0x1cu));
        site(0x1045b7u);
        compare_extended(s);
        site(0x1045b9u);
        pop_extended(s);
        store(0x1045bbu, field(esi, 0x20u), scalar_bits(0));
        site(0x1045c0u);
        if (flags.cf || flags.zf) {
            load_minimum(0x1045c8u);
        } else {
            load_single(0x1045c2u, stack(0x1cu));
            site(0x1045c6u);
        }
        x[0] = scalar(0x1045ceu, stack(0x38u));
        access(0x1045d4u);
        native_two_bone_solve::x87::store_pop(s, 0x1045d4u, g[esi] + 0x20u);
    }

    bool update_recovery() {
        x[0] = scalar(0x10457bu, field(esi, 0x1cu));
        site(0x104580u);
        x[1] = _mm_setzero_ps();
        site(0x104583u);
        flags.scalar_compare(x[0], x[1], false);
        site(0x104586u);
        if (flags.cf || flags.zf) return false;

        decay_recovery_timer();
        x[2] = scalar(0x1045d7u, constant(0x46a0d0u));
        x[3] = scalar(0x1045dfu, field(ebp, 0x54u));
        site(0x1045e4u);
        x[2] = subtract(x[2], x[0]);
        const auto base_rate = scalar(0x1045e8u, field(ebp, 0xe0u));
        x[2] = ordered_mul_ss(x[2], base_rate);
        site(0x1045f0u);
        x[3] = ordered_mul_ss(x[3], x[0]);
        x[0] = scalar(0x1045f4u, field(esi, 0x1cu));
        site(0x1045f9u);
        x[2] = ordered_add_ss(x[2], x[3]);
        const auto step = scalar(0x1045fdu, field(eax, 4u));
        x[2] = ordered_mul_ss(x[2], step);
        site(0x104602u);
        x[0] = subtract(x[0], x[2]);
        site(0x104606u);
        flags.scalar_compare(x[0], x[1], false);
        store(0x104609u, field(esi, 0x1cu), scalar_bits(0));
        site(0x10460eu);
        if (flags.cf || flags.zf) {
            site(0x104610u);
            x[0] = x[1];
        }
        store(0x104613u, field(esi, 0x1cu), scalar_bits(0));
        return true;
    }

    bool advance() {
        g[ecx] = read(0x10461du, stack(0x2cu));
        g[edx] = read(0x104621u, field(ecx, 0x2d0u));
        site(0x104627u);
        g[edi] = flags.increment(g[edi], 4, false);
        site(0x104628u);
        g[eax] = std::uint32_t(std::bit_cast<std::int16_t>(std::uint16_t(g[edi])));
        site(0x10462bu);
        flags.minus(g[eax], g[edx], 4);
        store(0x10462du, stack(0x28u), field(edi));
        store(0x104631u, stack(0x20u), field(eax));
        site(0x104635u);
        return flags.sf != flags.of;
    }

    void finish() {
        set_low_byte(g[eax], read(0x10463bu, stack(0x13u), 1));
        access(0x10463fu);
        g[edi] = s.pop();
        access(0x104640u);
        g[esi] = s.pop();
        access(0x104641u);
        g[ebp] = s.pop();
        access(0x104642u);
        g[ebx] = s.pop();
        site(0x104643u);
        g[esp] = flags.plus(g[esp], 0x38u, 4);
        access(0x104646u);
        s.pop();
        g[esp] += 4u;
    }
};

template<class State, class Dispatch>
Result run(State &s, Dispatch dispatch) {
    if (!selected(s) || reference() || s.active_mmx || s.fp.count > 6u) return Result::retained;
    TimerTailFrame<State> frame{s};
    frame.update_quantized_timer(dispatch);
    if (frame.notify_record(dispatch) || frame.update_recovery())
        frame.store(0x104618u, frame.stack(0x13u), frame.constant(1u), 1);
    if (frame.advance()) return Result::next_record;
    frame.finish();
    return Result::returned;
}

template<class State>
bool finish(State &s) {
    if (!selected(s) || reference()) return false;
    TimerTailFrame<State>{s}.finish();
    return true;
}

} // namespace halo2::native_death_cycle_timer_tail
