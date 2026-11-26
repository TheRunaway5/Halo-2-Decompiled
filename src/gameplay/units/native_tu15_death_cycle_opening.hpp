#pragma once

#include "physics/runtime/native_tu15_two_bone_solve.hpp"

namespace halo2::native_death_cycle_opening {

using namespace native_object_tick_detail::boundary;
using physics_step_detail::f;
using physics_step_detail::r;
using physics_step_detail::u;
using physics_step_detail::w;
using native_two_bone_solve::push_word;

template<class State>
bool selected(const State &s) {
    return s.image == SourcePhysicsContactImage::TitleUpdate15;
}

inline bool reference() {
    static const bool enabled = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_DEATH_CYCLE_OPENING");
        return value && *value;
    }();
    return enabled;
}

template<class State>
struct Frame {
    State &s;
    decltype(State::gp) &g = s.gp;
    decltype(State::flags) &flags = s.flags;
    const typename State::InstructionCursor *cursor = s.immutable_cursor.cursor;

    bool quiet() const {
        return cursor && !(cursor->first_trace && *cursor->first_trace) &&
            !(cursor->second_trace && *cursor->second_trace);
    }
    void site(std::uint32_t pc) {
        if (!quiet()) s.instruction_literal(pc);
    }
    void access(std::uint32_t pc) {
        if (quiet()) *cursor->position = pc;
        else s.instruction_literal(pc);
    }
    auto field(unsigned reg, std::uint32_t offset = 0) {
        return [this, reg, offset] { return g[reg] + offset; };
    }
    auto constant(std::uint32_t value) {
        return [value] { return value; };
    }
    template<class Address>
    std::uint32_t read(std::uint32_t pc, Address address, unsigned width = 4) {
        access(pc);
        return r(s.memory, address(), width);
    }
    template<class Address, class Value>
    void store(std::uint32_t pc, Address address, Value value, unsigned width = 4) {
        access(pc);
        w(s.memory, s.observe, pc, address(), width, value());
    }
    template<class Value>
    void push(std::uint32_t pc, Value value) {
        access(pc);
        push_word(s, pc, value());
    }

    void prepare_frame() {
        constexpr std::uint32_t object_datums = 0x4ecbd0u, tag_datums = 0x4f0414u;
        site(0x1030f0u);
        g[esp] = flags.minus(g[esp], 0x38u, 4);
        g[ecx] = read(0x1030f3u, field(esp, 0x3cu));
        push(0x1030f7u, field(ebx));
        push(0x1030f8u, field(ebp));
        push(0x1030f9u, field(esi));
        site(0x1030fau);
        g[eax] = g[ecx];
        site(0x1030fcu);
        g[eax] = flags.logic(g[eax] & 0xffffu, 4);
        push(0x103101u, field(edi));
        g[edi] = read(0x103102u, constant(object_datums));
        site(0x103108u);
        g[esi] = g[eax] * 3u;
        g[eax] = read(0x10310bu, field(edi, 0x44u));
        site(0x10310eu);
        g[esi] = flags.shift(g[esi], 2u, 4);
        g[ebp] = read(0x103111u, [&] { return g[eax] + g[esi] + 8u; });
        g[edx] = read(0x103115u, field(ebp));
        g[eax] = read(0x103118u, constant(tag_datums));
        site(0x10311du);
        g[edx] = flags.logic(g[edx] & 0xffffu, 4);
        site(0x103123u);
        g[edx] = flags.shift(g[edx], 4u, 4);
        g[edx] = read(0x103126u, [&] { return g[eax] + g[edx] + 8u; });
        push(0x10312au, field(ecx));
        store(0x10312bu, field(esp, 0x28u), field(esi));
        store(0x10312fu, field(esp, 0x38u), field(ebp));
        store(0x103133u, field(esp, 0x30u), field(edx));
    }

    void prepare_scale_query() {
        site(0x10313cu);
        set_low_byte(g[ebx], g[eax]);
        g[eax] = read(0x10313eu, field(edi, 0x44u));
        g[ecx] = read(0x103141u, [&] { return g[eax] + g[esi] + 8u; });
        set_low_byte(g[edx], read(0x103145u, field(ecx, 0x12cu), 1));
        site(0x10314bu);
        g[eax] = flags.logic(0xffffffffu, 4);
        site(0x10314eu);
        flags.logic(g[edx] & 1u, 1);
        site(0x103151u);
        if (!flags.zf) g[eax] = read(0x103153u, field(ecx, 0x154u));
        g[esi] = read(0x103159u, field(esp, 0x4cu));
    }

    void publish_scale() {
        const auto scale_bits = [&] { return u(_mm_cvtss_f32(s.xmm[0])); };
        g[ecx] = read(0x103162u, field(esp, 0x2cu));
        g[edx] = read(0x103166u, field(ecx, 0x12cu));
        site(0x10316cu);
        g[edx] = flags.right(g[edx], 27u, 4, false);
        site(0x10316fu);
        flags.logic(g[edx] & 1u, 1);
        store(0x103172u, field(esp, 0x38u), scale_bits);
        store(0x103178u, field(esp, 0x16u), constant(0u), 1);
        store(0x10317du, field(esp, 0x30u), constant(0xffffffffu));
        site(0x103185u);
        if (!flags.zf) {
            site(0x103187u);
            flags.logic(low_byte(g[ebx]), 1);
            s.xmm[0] = _mm_set_ss(f(read(0x103189u, constant(0x46a0d0u))));
            site(0x103191u);
            if (flags.zf) {
                site(0x103193u);
                s.xmm[0] = _mm_setzero_ps();
            }
            store(0x103196u, field(esp, 0x38u), scale_bits);
        }
        g[ebx] = read(0x10319cu, field(esp, 0x4cu));
        site(0x1031a0u);
        g[eax] = g[ebx];
    }

    void publish_override() {
        site(0x1031a7u);
        g[esi] = flags.logic(0xffffffffu, 4);
        site(0x1031aau);
        flags.minus(g[eax], g[esi], 4);
        site(0x1031acu);
        if (flags.zf) return;

        g[ecx] = read(0x1031aeu, field(edi, 0x44u));
        site(0x1031b1u);
        g[eax] = flags.logic(g[eax] & 0xffffu, 4);
        site(0x1031b6u);
        g[eax] *= 3u;
        g[eax] = read(0x1031b9u, [&] { return g[ecx] + g[eax] * 4u + 8u; });
        g[eax] = read(0x1031bdu, field(eax, 0x13cu));
        site(0x1031c3u);
        flags.minus(g[eax], g[esi], 4);
        site(0x1031c5u);
        if (!flags.zf) store(0x1031c7u, field(esp, 0x30u), field(eax));
    }
};

template<class State, class Check, class Scale, class Resolve>
bool run(State &s, Check check, Scale scale, Resolve resolve) {
    if (!selected(s) || reference()) return false;

    Frame<State> frame{s};
    frame.prepare_frame();
    frame.push(0x103137u, frame.constant(0x10313cu));
    check(s);
    frame.prepare_scale_query();
    frame.push(0x10315du, frame.constant(0x103162u));
    scale(s);
    frame.publish_scale();
    frame.push(0x1031a2u, frame.constant(0x1031a7u));
    resolve(s);
    frame.publish_override();
    return true;
}

} // namespace halo2::native_death_cycle_opening
