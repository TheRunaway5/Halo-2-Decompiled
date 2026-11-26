#pragma once

#include "gameplay/units/native_tu15_death_cycle_opening.hpp"

#include <algorithm>
#include <bit>

namespace halo2::native_death_cycle_activity {

using namespace native_death_cycle_opening;

inline bool reference() {
    static const bool enabled = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_DEATH_CYCLE_ACTIVITY");
        return value && *value;
    }();
    return enabled;
}

enum class ActivityExit { retained, motion, next_record };

template<class State>
struct ActivityFrame : Frame<State> {
    using Base = Frame<State>;
    using Base::s, Base::g, Base::flags, Base::site;
    using Base::read, Base::store, Base::push, Base::field, Base::constant;

    explicit ActivityFrame(State &state) : Base{state} {}

    auto stack(std::uint32_t offset) { return field(esp, offset); }
    auto object(std::uint32_t offset) { return field(ebp, offset); }

    bool positive() const { return !flags.zf && flags.sf == flags.of; }
    static std::uint32_t signed_byte(std::uint32_t value) {
        return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(value))));
    }
    static std::uint32_t signed_word(std::uint32_t value) {
        return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));
    }

    void clear_effect_magnitude() {
        site(0x103657u);
        s.xmm[0] = _mm_setzero_ps();
        site(0x10365au);
        g[eax] = flags.logic(0u, 4);
        s.set(eax, read(0x10365cu, object(0x17cu), 2), 2, 0);
        site(0x103663u);
        flags.logic(g[eax] & 0xffffu, 2);
        store(0x103666u, object(0x188u), [&] { return u(_mm_cvtss_f32(s.xmm[0])); });
        site(0x10366eu);
        if (!positive()) return;
        site(0x103670u);
        g[eax] = flags.increment(g[eax], 4, true);
        site(0x103671u);
        set_low_byte(g[ebx], 1u);
        store(0x103673u, object(0x17cu), field(eax), 2);
        store(0x10367au, stack(0x13u), field(ebx), 1);
    }

    void publish_activity_flags() {
        s.set(eax, read(0x10367eu, object(0x16eu), 2), 2, 0);
        site(0x103685u);
        set_low_byte(g[ecx], g[eax]);
        site(0x103687u);
        set_low_byte(g[ecx], flags.right(low_byte(g[ecx]), 3u, 1, false));
        site(0x10368au);
        set_low_byte(g[edx], g[eax]);
        site(0x10368cu);
        set_low_byte(g[ecx], flags.logic(g[ecx] & 1u, 1));
        site(0x10368fu);
        set_low_byte(g[edx], flags.right(low_byte(g[edx]), 4u, 1, false));
        site(0x103692u);
        set_low_byte(g[edx], flags.logic(g[edx] & 1u, 1));
        store(0x103695u, stack(0x20u), field(ecx), 1);
        site(0x103699u);
        set_low_byte(g[ecx], g[eax]);
        store(0x10369bu, stack(0x21u), field(edx), 1);
        site(0x10369fu);
        set_low_byte(g[edx], g[eax]);
        site(0x1036a1u);
        set_low_byte(g[ecx], flags.right(low_byte(g[ecx]), 1u, 1, false));
        site(0x1036a3u);
        set_low_byte(g[eax], g[eax] >> 8u);
        site(0x1036a5u);
        set_low_byte(g[eax], flags.logic(g[eax] & 1u, 1));
        site(0x1036a7u);
        set_low_byte(g[ecx], flags.logic(g[ecx] & 1u, 1));
        site(0x1036aau);
        set_low_byte(g[edx], flags.right(low_byte(g[edx]), 2u, 1, false));
        site(0x1036adu);
        set_low_byte(g[edx], flags.logic(g[edx] & 1u, 1));
        store(0x1036b0u, stack(0x1eu), field(eax), 1);
        g[eax] = read(0x1036b4u, constant(0x4ecbd0u));
        store(0x1036b9u, stack(0x1cu), field(ecx), 1);
        g[ecx] = read(0x1036bdu, field(eax, 0x44u));
        store(0x1036c0u, stack(0x1du), field(edx), 1);
    }

    bool needs_activity_reset() {
        g[edx] = read(0x1036c4u, stack(0x24u));
        g[eax] = read(0x1036c8u, [&] { return g[ecx] + g[edx] + 8u; });
        set_low_byte(g[ecx], read(0x1036ccu, field(eax, 0x16eu), 1));
        site(0x1036d2u);
        set_low_byte(g[ecx], flags.right(low_byte(g[ecx]), 5u, 1, false));
        site(0x1036d5u);
        flags.logic(g[ecx] & 1u, 1);
        store(0x1036d8u, stack(0x22u), constant(0u), 1);
        site(0x1036ddu);
        if (!flags.zf) return true;
        flags.minus(read(0x1036dfu, field(eax, 0x17cu), 2), 0u, 2);
        site(0x1036e7u);
        if (positive()) return true;
        set_low_byte(g[edx], read(0x1036e9u, field(eax, 0x16cu), 1));
        site(0x1036efu);
        set_low_byte(g[edx], flags.right(low_byte(g[edx]), 1u, 1, false));
        site(0x1036f1u);
        flags.logic(g[edx] & 1u, 1);
        site(0x1036f4u);
        return !flags.zf;
    }

    void reset_activity() {
        site(0x1036f6u);
        set_low_byte(g[ebx], 1u);
        store(0x1036f8u, object(0x174u), constant(0u), 1);
        store(0x1036ffu, stack(0x16u), constant(1u), 1);
        store(0x103704u, stack(0x13u), field(ebx), 1);
    }

    template<class Child>
    void visit_tag_entries(Child child) {
        g[edi] = read(0x103708u, stack(0x2cu));
        g[eax] = read(0x10370cu, field(edi, 0x2c0u));
        site(0x103712u);
        g[esi] = flags.logic(0u, 4);
        site(0x103714u);
        flags.logic(g[eax], 4);
        site(0x103716u);
        if (!positive()) return;

        site(0x103718u);
        g[eax] = flags.logic(0u, 4);
        site(0x10371au); // Authored identity LEA.
        do {
            push(0x103720u, field(eax));
            g[eax] = read(0x103721u, stack(0x50u));
            push(0x103725u, field(eax));
            push(0x103726u, constant(0x10372bu));
            child(s);
            g[ecx] = read(0x10372bu, field(edi, 0x2c0u));
            site(0x103731u);
            set_low_byte(g[ebx], flags.logic(low_byte(g[ebx] | g[eax]), 1));
            site(0x103733u);
            g[esi] = flags.increment(g[esi], 4, false);
            site(0x103734u);
            g[eax] = signed_word(g[esi]);
            site(0x103737u);
            flags.minus(g[eax], g[ecx], 4);
            site(0x103739u);
        } while (flags.sf != flags.of);
        store(0x10373bu, stack(0x13u), field(ebx), 1);
    }

    ActivityExit first_motion_record() {
        g[ecx] = read(0x10373fu, field(edi, 0x2c8u));
        site(0x103745u);
        g[eax] = flags.logic(0u, 4);
        site(0x103747u);
        flags.minus(g[ecx], g[eax], 4);
        store(0x103749u, stack(0x28u), field(eax));
        site(0x10374du);
        return positive() ? ActivityExit::motion : ActivityExit::next_record;
    }

    void set_motion_record() {
        g[ecx] = read(0x103753u, field(edi, 0x2ccu));
        site(0x103759u);
        g[esi] = g[eax];
        site(0x10375bu);
        g[esi] = flags.shift(g[esi], 6u, 4);
        site(0x10375eu);
        g[ebx] = g[eax]*3u;
        site(0x103761u);
        g[esi] = flags.plus(g[esi], g[ecx], 4);
        site(0x103763u);
        g[ebx] = flags.shift(g[ebx], 2u, 4);
        site(0x103766u);
        g[ecx] = flags.logic(0u, 4);
        const auto record = [&](std::uint32_t offset) { return g[ebp] + g[ebx] + offset; };
        s.set(ecx, read(0x103768u, [&] { return record(0x20eu); }, 2), 2, 0);
        site(0x103770u);
        flags.logic(g[ecx] & 0xffffu, 2);
        store(0x103773u, stack(0x14u), constant(0u), 1);
        store(0x103778u, stack(0x15u), constant(0u), 1);
        site(0x10377du);
        if (positive()) {
            site(0x10377fu);
            g[ecx] = flags.increment(g[ecx], 4, true);
            store(0x103780u, [&] { return record(0x20eu); }, field(ecx), 2);
            store(0x103788u, stack(0x13u), constant(1u), 1);
        }
        set_low_byte(g[ecx], read(0x10378du, stack(0x16u), 1));
        site(0x103791u);
        flags.logic(low_byte(g[ecx]), 1);
        site(0x103793u);
        if (!flags.zf) return;

        g[edi] = signed_word(read(0x103795u, field(esi, 4u), 2));
        set_low_byte(g[ecx], read(0x103799u, [&] { return g[esp] + g[edi] + 0x1cu; }, 1));
        set_low_byte(g[edx], read(0x10379du, [&] { return g[esp] + g[edi] + 0x20u; }, 1));
        store(0x1037a1u, stack(0x14u), field(ecx), 1);
        set_low_byte(g[ecx], read(0x1037a5u, [&] { return record(0x210u); }, 1));
        site(0x1037acu);
        flags.logic(low_byte(g[ecx]), 1);
        store(0x1037aeu, stack(0x15u), field(edx), 1);
        site(0x1037b2u);
        if (flags.sf) store(0x1037b4u, stack(0x14u), constant(1u), 1);
    }

    template<class Translate, std::size_t N>
    std::uint32_t target(Translate translate, std::uint32_t pc, std::uint32_t table,
                         const std::array<std::uint32_t, N> &destinations) {
        const auto address = read(pc, [&] { return table + g[ecx]*4u; });
        const auto result = translate(address, s.image);
        if (std::ranges::find(destinations, result) == destinations.end())
            throw std::runtime_error("Required original solver dispatch target unavailable");
        return result;
    }

    template<class Translate>
    std::uint32_t select_motion_handler(Translate translate) {
        constexpr std::array modes{0x1037d3u, 0x103d68u, 0x103e32u, 0x103ed4u, 0x103f1du, 0x104006u, 0x1040f0u};
        constexpr std::array states{0x1037e7u, 0x10387cu, 0x1038f5u, 0x103994u, 0x103a6du, 0x103d0fu};
        set_low_byte(g[edx], read(0x1037b9u, [&] { return g[ebp] + g[ebx] + 0x20cu; }, 1));
        site(0x1037c0u);
        g[ecx] = signed_byte(g[edx]);
        site(0x1037c3u);
        flags.minus(g[ecx], 7u, 4);
        site(0x1037c6u);
        if (!flags.cf && !flags.zf) return 0x1040f0u;
        const auto mode = target(translate, 0x1037ccu, 0x10464cu, modes);
        if (mode != 0x1037d3u) return mode;

        g[ecx] = signed_word(read(0x1037d3u, field(esi, 6u), 2));
        site(0x1037d7u);
        flags.minus(g[ecx], 5u, 4);
        site(0x1037dau);
        if (!flags.cf && !flags.zf) return 0x1040f0u;
        return target(translate, 0x1037e0u, 0x10466cu, states);
    }
};

template<class State, class Child>
ActivityExit start(State &s, Child child) {
    if (!selected(s) || reference()) return ActivityExit::retained;
    ActivityFrame<State> frame{s};
    frame.clear_effect_magnitude();
    frame.publish_activity_flags();
    if (frame.needs_activity_reset()) frame.reset_activity();
    frame.visit_tag_entries(child);
    return frame.first_motion_record();
}

template<class State, class Translate>
std::uint32_t motion(State &s, Translate translate) {
    if (!selected(s) || reference()) return 0;
    ActivityFrame<State> frame{s};
    frame.set_motion_record();
    return frame.select_motion_handler(translate);
}

} // namespace halo2::native_death_cycle_activity
