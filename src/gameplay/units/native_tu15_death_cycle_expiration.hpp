#pragma once

#include "gameplay/units/native_tu15_death_cycle_opening.hpp"

#include <algorithm>
#include <bit>

namespace halo2::native_death_cycle_expiration {

using namespace native_death_cycle_opening;
using native_tu15_leaves::ordered_mul_ss;

enum class Entry { finish_motion, begin_records, expire_record };

inline bool reference() {
    static const bool enabled = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_DEATH_CYCLE_EXPIRATION");
        return value && *value;
    }();
    return enabled;
}

inline __m128 subtract(__m128 left, __m128 right) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("subss %1,%0" : "+x"(left) : "x"(right));
    return left;
#else
    return _mm_sub_ss(left, right);
#endif
}

struct Timer {
    std::uint32_t compare, inactive, rate, multiply, subtract, clamp_compare;
    std::uint32_t store, clamp_branch, clamp_store, mark;
    std::uint32_t value_offset, rate_offset;
};

constexpr Timer first_timer{0x10417du, 0x104185u, 0x104187u, 0x10418fu, 0x104194u, 0x104198u,
                            0x10419bu, 0x1041a0u, 0x1041a2u, 0x1041a7u, 0x14u, 0xd0u};
constexpr Timer second_timer{0x1041b1u, 0x1041b4u, 0x1041b6u, 0x1041beu, 0x1041c3u, 0x1041c7u,
                             0x1041cau, 0x1041cfu, 0x1041d1u, 0x1041d6u, 0x18u, 0xccu};

template<class State>
struct ExpirationFrame : Frame<State> {
    using Base = Frame<State>;
    using Base::s, Base::g, Base::flags, Base::site, Base::access, Base::read, Base::store, Base::field, Base::constant;
    decltype(State::xmm) &x = s.xmm;

    explicit ExpirationFrame(State &state) : Base{state} {}

    auto stack(std::uint32_t offset) { return field(esp, offset); }
    auto scalar_bits(unsigned reg) { return [this, reg] { return u(_mm_cvtss_f32(x[reg])); }; }
    bool positive() const { return !flags.zf && flags.sf == flags.of; }

    template<class Address>
    bool marked(std::uint32_t load_pc, std::uint32_t test_pc, std::uint32_t branch_pc, Address address) {
        set_low_byte(g[eax], read(load_pc, address, 1));
        site(test_pc);
        flags.logic(low_byte(g[eax]), 1);
        site(branch_pc);
        return !flags.zf;
    }

    bool next_motion() {
        if (marked(0x1040f0u, 0x1040f7u, 0x1040f9u, [&] { return g[ebp] + g[ebx] + 0x20cu; }) ||
            marked(0x1040fbu, 0x1040ffu, 0x104101u, stack(0x14u)) ||
            marked(0x104103u, 0x104107u, 0x104109u, stack(0x15u)))
            store(0x10410bu, stack(0x13u), constant(1u), 1);

        g[eax] = read(0x104110u, stack(0x28u));
        g[edi] = read(0x104114u, stack(0x2cu));
        g[ecx] = read(0x104118u, field(edi, 0x2c8u));
        site(0x10411eu);
        g[eax] = flags.increment(g[eax], 4, false);
        store(0x10411fu, stack(0x28u), field(eax));
        site(0x104123u);
        g[eax] = std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(g[eax]))));
        site(0x104126u);
        flags.minus(g[eax], g[ecx], 4);
        site(0x104128u);
        return flags.sf != flags.of;
    }

    bool begin_records() {
        g[ecx] = read(0x10412eu, field(edi, 0x2d0u));
        site(0x104134u);
        g[eax] = flags.logic(0u, 4);
        site(0x104136u);
        flags.minus(g[ecx], g[eax], 4);
        store(0x104138u, stack(0x28u), field(eax));
        site(0x10413cu);
        if (!positive()) return false;

        site(0x104142u);
        x[1] = _mm_setzero_ps();
        store(0x104145u, stack(0x20u), field(eax));
        site(0x104149u);
        g[edi] = g[eax];
        return true;
    }

    void address_record() {
        g[ebp] = read(0x10414bu, stack(0x20u));
        g[ecx] = read(0x10414fu, stack(0x34u));
        g[eax] = read(0x104153u, constant(0x51df54u));
        site(0x104158u);
        g[edx] = g[ebp];
        site(0x10415au);
        g[ebp] = flags.product(g[ebp], 0xecu);
        site(0x104160u);
        g[edx] = flags.product(g[edx], 0x34u);
        x[0] = _mm_set_ss(f(read(0x104163u, [&] { return g[edx]+g[ecx]+0x1b8u; })));
        site(0x10416cu);
        g[esi] = g[edx]+g[ecx]+0x1a4u;
        g[edx] = read(0x104173u, stack(0x2cu));
        access(0x104177u);
        g[ebp] = flags.plus(g[ebp], r(s.memory, g[edx]+0x2d4u, 4), 4);
    }

    void decay_timer(const Timer &timer, bool clear_pending) {
        site(timer.compare);
        flags.scalar_compare(x[0], x[1], false);
        if (clear_pending) store(0x104180u, stack(0x15u), constant(0u), 1);
        site(timer.inactive);
        if (flags.cf || flags.zf) return;

        x[2] = _mm_set_ss(f(read(timer.rate, field(ebp, timer.rate_offset))));
        const auto step = _mm_set_ss(f(read(timer.multiply, field(eax, 4u))));
        x[2] = ordered_mul_ss(x[2], step);
        site(timer.subtract);
        x[0] = subtract(x[0], x[2]);
        site(timer.clamp_compare);
        flags.scalar_compare(x[1], x[0], false);
        store(timer.store, field(esi, timer.value_offset), scalar_bits(0));
        site(timer.clamp_branch);
        if (!flags.cf) store(timer.clamp_store, field(esi, timer.value_offset), scalar_bits(1));
        store(timer.mark, stack(0x13u), constant(1u), 1);
    }

    void decay_record() {
        address_record();
        decay_timer(first_timer, true);
        x[0] = _mm_set_ss(f(read(0x1041acu, field(esi, 0x18u))));
        decay_timer(second_timer, false);

        site(0x1041dbu);
        g[eax] = flags.logic(0u, 4);
        s.set(eax, read(0x1041ddu, field(esi, 2u), 2), 2, 0);
        site(0x1041e1u);
        flags.logic(g[eax] & 0xffffu, 2);
        site(0x1041e4u);
        if (!positive()) return;
        site(0x1041e6u);
        g[eax] = flags.increment(g[eax], 4, true);
        store(0x1041e7u, field(esi, 2u), field(eax), 2);
        store(0x1041ebu, stack(0x13u), constant(1u), 1);
    }

    template<class Translate>
    std::uint32_t next_handler(Translate translate) {
        constexpr std::array destinations{0x104204u, 0x1042a6u, 0x10434cu, 0x104360u};
        const auto type = std::uint8_t(read(0x1041f0u, field(esi, 1u), 1));
        g[eax] = std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(type)));
        site(0x1041f4u);
        flags.minus(g[eax], 3u, 4);
        site(0x1041f7u);
        if (!flags.cf && !flags.zf) return 0x104371u;
        const auto address = read(0x1041fdu, [&] { return 0x1046a4u + g[eax]*4u; });
        const auto destination = translate(address, s.image);
        if (std::ranges::find(destinations, destination) == destinations.end())
            throw std::runtime_error("Required original solver dispatch target unavailable");
        return destination;
    }
};

template<class State, class Translate>
std::uint32_t run(State &s, Entry entry, Translate translate) {
    if (!selected(s) || reference()) return 0;
    ExpirationFrame<State> frame{s};
    if (entry == Entry::finish_motion && frame.next_motion()) return 0x103753u;
    if (entry != Entry::expire_record && !frame.begin_records()) return 0x10463bu;
    frame.decay_record();
    return frame.next_handler(translate);
}

} // namespace halo2::native_death_cycle_expiration
