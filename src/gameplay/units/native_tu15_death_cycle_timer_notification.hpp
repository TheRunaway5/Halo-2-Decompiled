#pragma once

#include "gameplay/units/native_tu15_death_cycle_motion.hpp"
#include "gameplay/units/native_tu15_death_cycle_expiration.hpp"

namespace halo2::native_death_cycle_timer_notification {

using namespace native_death_cycle_opening;
using native_death_cycle_expiration::subtract;
using native_death_cycle_motion::update_field;
using native_tu15_leaves::ordered_mul_ss;

inline bool reference() {
    static const bool enabled = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_DEATH_CYCLE_TIMER_NOTIFICATION");
        return value && *value;
    }();
    return enabled;
}

inline __m128 divide(__m128 left, __m128 right) {
    asm volatile("divss %1,%0" : "+x"(left) : "x"(right));
    return left;
}

template<class State>
struct NotificationFrame : native_death_cycle_expiration::ExpirationFrame<State> {
    using Base = native_death_cycle_expiration::ExpirationFrame<State>;
    using Base::s, Base::g, Base::x, Base::flags, Base::site, Base::access;
    using Base::read, Base::store, Base::push, Base::field, Base::constant;
    using Base::stack, Base::scalar_bits;

    explicit NotificationFrame(State &state) : Base{state} {}

    auto object(std::uint32_t offset) { return field(ebp, offset); }

    template<class Address>
    __m128 scalar(std::uint32_t pc, Address address) {
        return _mm_set_ss(f(read(pc, address)));
    }

    void change_status(std::uint32_t pc, std::uint32_t mask, std::uint32_t bits) {
        access(pc);
        update_field(s, pc, g[ebp] + 0x16cu, 1, mask, bits);
    }

    bool load_status(std::uint32_t input, std::uint32_t shift, std::uint32_t test, std::uint32_t branch) {
        set_low_byte(g[eax], read(input, object(0x16cu), 1));
        site(shift);
        set_low_byte(g[eax], flags.right(low_byte(g[eax]), 1u, 1, false));
        site(test);
        flags.logic(g[eax] & 1u, 1);
        site(branch);
        return !flags.zf;
    }

    template<class Call>
    void update_threshold(Call &call) {
        const auto threshold = scalar(0x10344bu, field(ecx, 0x154u));
        flags.scalar_compare(x[0], threshold, false);
        site(0x103452u);
        if (flags.cf || load_status(0x103454u, 0x10345au, 0x10345cu, 0x10345eu)) return;

        g[ebx] = read(0x103460u, stack(0x4cu));
        change_status(0x103464u, 0xffu, 2u);
        site(0x10346bu);
        g[eax] = g[ebx];
        push(0x10346du, constant(0x103472u));
        call(s, 0x106ff0u, 0x10346du);
        site(0x103472u);
        flags.minus(g[eax], 0xffffffffu, 4);
        site(0x103475u);
        if (!flags.zf) {
            push(0x103477u, constant(0u));
            push(0x103479u, constant(0x10347eu));
            call(s, 0xcabc0u, 0x103479u);
        }

        g[ecx] = read(0x10347eu, stack(0x2cu));
        flags.minus(read(0x103482u, field(ecx, 0x292u), 2), 3u, 2);
        site(0x10348au);
        if (flags.zf) {
            set_low_byte(g[edx], read(0x10348cu, object(0x16cu), 1));
            site(0x103492u);
            set_low_byte(g[edx], flags.right(low_byte(g[edx]), 3u, 1, false));
            site(0x103495u);
            flags.logic(g[edx] & 1u, 1);
            site(0x103498u);
            if (!flags.zf) {
                change_status(0x10349au, 0xf7u, 0u);
                site(0x1034a1u);
                return;
            }
        }
        push(0x1034a3u, constant(0xeu));
        push(0x1034a5u, constant(0x1034aau));
        call(s, 0x10b060u, 0x1034a5u);
    }

    bool scale_timer() {
        x[0] = scalar(0x1034aau, object(0x188u));
        site(0x1034b2u);
        x[2] = _mm_setzero_ps();
        site(0x1034b5u);
        flags.scalar_compare(x[0], x[2], true);
        site(0x1034b8u);
        s.set(eax, flags.bits() | 2u, 1, 8);
        site(0x1034b9u);
        flags.logic((g[eax] >> 8u) & 0x44u, 1);
        site(0x1034bcu);
        if (flags.pf) return false;

        s.set(ecx, read(0x1034c2u, object(0x16cu), 2), 2, 0);
        g[edx] = read(0x1034c9u, stack(0x2cu));
        site(0x1034cdu);
        set_low_byte(g[eax], g[ecx]);
        site(0x1034cfu);
        set_low_byte(g[eax], flags.right(low_byte(g[eax]), 1u, 1, false));
        site(0x1034d1u);
        set_low_byte(g[eax], flags.logic(g[eax] & 1u, 1));
        site(0x1034d3u);
        bool alternate = false;
        if (!flags.zf) {
            x[0] = scalar(0x1034d5u, field(edx, 0x168u));
            site(0x1034ddu);
            flags.scalar_compare(x[0], x[2], false);
            site(0x1034e0u);
            alternate = !flags.cf && !flags.zf;
            if (alternate) site(0x1034e2u);
        }
        if (!alternate) x[0] = scalar(0x1034e4u, field(edx, 0x160u));

        g[esi] = read(0x1034ecu, constant(0x51df54u));
        x[1] = scalar(0x1034f2u, field(esi, 4u));
        site(0x1034f7u);
        x[1] = ordered_mul_ss(x[1], x[0]);
        x[0] = scalar(0x1034fbu, field(edx, 0x254u));
        site(0x103503u);
        flags.scalar_compare(x[0], x[2], false);
        store(0x103506u, stack(0x28u), scalar_bits(1));
        site(0x10350cu);
        if (!flags.cf && !flags.zf) {
            x[0] = scalar(0x10350eu, object(0x184u));
            const auto ratio = scalar(0x103516u, field(edx, 0x254u));
            x[0] = ordered_mul_ss(x[0], ratio);
            x[3] = scalar(0x10351eu, constant(0x46a0d0u));
            site(0x103526u);
            x[3] = subtract(x[3], x[0]);
            site(0x10352au);
            x[3] = ordered_mul_ss(x[3], x[1]);
            store(0x10352eu, stack(0x28u), scalar_bits(3));
            site(0x103534u);
            x[1] = x[3];
        }

        x[0] = scalar(0x103537u, object(0x180u));
        site(0x10353fu);
        x[0] = subtract(x[0], x[1]);
        site(0x103543u);
        flags.scalar_compare(x[2], x[0], false);
        store(0x103546u, object(0x180u), scalar_bits(0));
        site(0x10354eu);
        if (!flags.cf && !flags.zf) store(0x103550u, object(0x180u), scalar_bits(2));
        return true;
    }

    template<class Call>
    void play_late_threshold(Call &call) {
        site(0x103558u);
        flags.logic(low_byte(g[eax]), 1);
        site(0x10355au);
        if (flags.zf) return;
        site(0x10355cu);
        set_low_byte(g[ecx], flags.right(low_byte(g[ecx]), 2u, 1, false));
        site(0x10355fu);
        flags.logic(g[ecx] & 1u, 1);
        site(0x103562u);
        if (!flags.zf) return;

        g[ebx] = read(0x103564u, stack(0x4cu));
        push(0x103568u, constant(1u));
        push(0x10356au, constant(0x8000071u));
        site(0x10356fu);
        g[ecx] = g[ebx];
        push(0x103571u, constant(0x103576u));
        call(s, 0x16b44au, 0x103571u);
        site(0x103576u);
        flags.minus(g[eax] & 0xffffu, 1u, 2);
        site(0x10357au);
        if (flags.zf || flags.sf != flags.of) {
            site(0x10357cu);
            g[eax] = 1u;
        }

        g[ecx] = read(0x103581u, stack(0x2cu));
        x[0] = scalar(0x103585u, object(0x180u));
        const auto threshold = scalar(0x10358du, field(ecx, 0x150u));
        x[0] = subtract(x[0], threshold);
        const auto step = scalar(0x103595u, stack(0x28u));
        x[0] = divide(x[0], step);
        site(0x10359bu);
        g[edx] = std::uint32_t(std::bit_cast<std::int16_t>(std::uint16_t(g[eax])));
        site(0x10359eu);
        x[1] = _mm_cvtsi32_ss(x[1], std::bit_cast<std::int32_t>(g[edx]));
        site(0x1035a2u);
        flags.scalar_compare(x[1], x[0], false);
        site(0x1035a5u);
        if (flags.cf) return;
        change_status(0x1035a7u, 0xffu, 4u);
        push(0x1035aeu, constant(0xfu));
        push(0x1035b0u, constant(0x1035b5u));
        call(s, 0x10b060u, 0x1035b0u);
    }

    void complete_timer() {
        if (load_status(0x1035b5u, 0x1035bbu, 0x1035bdu, 0x1035bfu)) {
            g[ecx] = read(0x1035c1u, stack(0x2cu));
            x[0] = scalar(0x1035c5u, field(ecx, 0x150u));
            const auto timer = scalar(0x1035cdu, object(0x180u));
            flags.scalar_compare(x[0], timer, false);
            site(0x1035d4u);
            if (!flags.cf && !flags.zf) change_status(0x1035d6u, 0xf9u, 0u);
        }
        g[ecx] = read(0x1035ddu, stack(0x2cu));
        site(0x1035e1u);
        set_low_byte(g[ebx], 1u);
        store(0x1035e3u, stack(0x13u), field(ebx), 1);
    }

    bool effect_needs_update() {
        g[esi] = read(0x1035e7u, field(ecx, 0x170u));
        site(0x1035edu);
        flags.minus(g[esi], 0xffffffffu, 4);
        site(0x1035f0u);
        if (flags.zf) return false;

        set_low_byte(g[ecx], read(0x1035f2u, object(0x12cu), 1));
        set_low_byte(g[edx], read(0x1035f8u, object(0x16cu), 1));
        site(0x1035feu);
        set_low_byte(g[ecx], flags.right(low_byte(g[ecx]), 1u, 1, false));
        site(0x103600u);
        set_low_byte(g[edx], flags.right(low_byte(g[edx]), 1u, 1, false));
        site(0x103602u);
        set_low_byte(g[ecx], flags.logic(g[ecx] & 1u, 1));
        site(0x103605u);
        flags.logic(g[edx] & 1u, 1);
        site(0x103608u);
        bool starts = false;
        if (!flags.zf) {
            site(0x10360au);
            flags.logic(low_byte(g[ecx]), 1);
            site(0x10360cu);
            starts = flags.zf;
        }
        if (starts) {
            site(0x10360eu);
            set_low_byte(g[edx], 1u);
            site(0x103610u);
        } else {
            site(0x103612u);
            set_low_byte(g[edx], flags.logic(0u, 1));
        }

        g[edi] = read(0x103614u, object(0x244u));
        site(0x10361au);
        flags.minus(g[edi], 0xffffffffu, 4);
        site(0x10361du);
        set_low_byte(g[eax], !flags.zf);
        site(0x103620u);
        flags.minus(low_byte(g[edx]), low_byte(g[eax]), 1);
        site(0x103622u);
        return !flags.zf;
    }

    template<class Call>
    void update_effect(Call &call) {
        if (!effect_needs_update()) return;
        site(0x103624u);
        flags.logic(low_byte(g[edx]), 1);
        site(0x103626u);
        if (!flags.zf) {
            g[eax] = read(0x103628u, stack(0x4cu));
            push(0x10362cu, constant(0x103631u));
            call(s, 0x108b30u, 0x10362cu);
            store(0x103631u, object(0x244u), field(eax));
            site(0x103637u);
            return;
        }
        site(0x103639u);
        flags.logic(low_byte(g[ecx]), 1);
        site(0x10363bu);
        if (!flags.zf) {
            site(0x10363du);
            g[eax] = g[edi];
            push(0x10363fu, constant(0x103644u));
            call(s, 0x17a4a0u, 0x10363fu);
            site(0x103644u);
        } else {
            push(0x103646u, constant(1u));
            push(0x103648u, constant(0x10364du));
            call(s, 0x17a0f0u, 0x103648u);
        }
        store(0x10364du, object(0x244u), constant(0xffffffffu));
    }

    template<class Call>
    void update_timer(Call &call) {
        x[0] = scalar(0x103436u, object(0x180u));
        const auto threshold = scalar(0x10343eu, constant(0x46a0e8u));
        flags.scalar_compare(x[0], threshold, false);
        site(0x103445u);
        if (flags.cf || flags.zf) return;
        update_threshold(call);
        if (scale_timer()) play_late_threshold(call);
        complete_timer();
    }
};

template<class State, class Call>
bool run(State &s, Call call) {
    if (!selected(s) || reference()) return false;
    NotificationFrame<State> frame{s};
    frame.update_timer(call);
    frame.update_effect(call);
    return true;
}

} // namespace halo2::native_death_cycle_timer_notification
