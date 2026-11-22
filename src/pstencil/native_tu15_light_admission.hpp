#pragma once

#include "pstencil/native_tu15_light_admission_systems.hpp"
#include "pstencil/native_tu15_light_visibility_frame.hpp"

#include <bit>

namespace halo2::native_light_admission_detail {
namespace logic = native_light_admission;

template<class State, class Call, class Before>
class Adapter : public native_light_visibility_detail::Frame<State, Call, Before> {
    using Base = native_light_visibility_detail::Frame<State, Call, Before>;
    using enum native_light_visibility_detail::Register;
    using Base::s_; using Base::at; using Base::reg; using Base::low;
    using Base::load; using Base::write; using Base::push; using Base::pop;
    using Base::enter; using Base::finish_return;
    static constexpr std::uint32_t light_pool = 0x4ecbdc;
    static constexpr std::uint32_t tag_table = 0x4f0414;
    static constexpr std::uint32_t view_mode = 0x491e22;
    static constexpr std::uint32_t visibility_filter = 0x4c6841;

public:
    using Base::Base;

    std::uint32_t begin() {
        push(0x31570, reg(ecx));
        push(0x31571, reg(ebx));
        low(ebx, s_.flags.logic(0, 1));
        const auto light = reg(edi);
        s_.flags.minus(light, logic::no_light, 4);
        // These temporaries occupy the high bytes of the saved ECX word.
        write(0x31577, reg(esp) + 6, 0, 1);
        write(0x3157c, reg(esp) + 7, 1, 1);
        return light;
    }

    bool light_enabled(std::uint32_t light) {
        reg(eax) = light;
        enter(0x31589, 0xc5170, 0x3158e);
        return s_.flags.logic(low(eax), 1) != 0;
    }

    std::uint32_t light_record(std::uint32_t index, unsigned stride) {
        reg(eax) = load(0x31596, light_pool);
        reg(ecx) = load(0x3159b, reg(eax) + offsetof(logic::LightPool, records));
        reg(edx) = load(0x3159e, tag_table);
        push(0x315a4, reg(ebp));
        push(0x315a5, reg(esi));
        reg(esi) = s_.flags.logic(index, 4);
        reg(esi) = s_.flags.product(reg(esi), stride);
        return reg(esi) = s_.flags.plus(reg(esi), reg(ecx), 4);
    }

    std::uint32_t definition(std::uint32_t record, unsigned id_offset, std::uint32_t mask,
        unsigned stride, unsigned definition_offset) {
        reg(ecx) = load(0x315b6, record + id_offset);
        reg(ecx) = s_.flags.logic(reg(ecx) & mask, 4);
        reg(ecx) = s_.flags.shift(reg(ecx), std::countr_zero(stride), 4);
        return reg(ebp) = load(0x315c2, reg(edx) + reg(ecx) + definition_offset);
    }

    bool has_visible_shape(std::uint32_t light) {
        reg(eax) = light;
        enter(0x315c8, 0x31500, 0x315cd);
        return s_.flags.logic(low(eax), 1) != 0;
    }

    void build_shape(std::uint32_t light) {
        reg(eax) = load(0x315d1, reg(esp) + 20);
        push(0x315d5, 1);
        reg(edx) = light;
        enter(0x315d9, 0xc3820, 0x315de);
    }

    std::uint32_t shape_flags(std::uint32_t definition, std::uint32_t mask) {
        reg(ecx) = load(0x315de, definition);
        const auto flags = reg(ecx);
        s_.flags.logic((flags & mask) >> 8, 1);
        low(ebx, low(eax));
        low(eax, 1);
        return flags;
    }

    bool in_view_mode(std::uint16_t mode) {
        return s_.flags.minus(load(0x315ea, view_mode, 2), mode, 2) == 0;
    }

    void complete_view_gate(bool allowed) {
        if(!allowed) low(eax, s_.flags.logic(0, 1));
        low(ecx, low(eax));
    }

    void read_view_gate() { low(ecx, load(0x315fa, reg(esp) + 14, 1)); }

    void apply_visibility_gate(std::uint32_t definition, std::uint32_t mask) {
        low(eax, load(0x315fe, visibility_filter, 1));
        if(s_.flags.logic(low(eax), 1)) {
            reg(eax) = load(0x31607, definition);
            reg(eax) = s_.flags.right(reg(eax), std::countr_zero(mask), 4, false);
            low(eax, s_.flags.logic(low(eax) & 1, 1));
        } else {
            low(eax, load(0x31611, reg(esp) + 15, 1));
        }
        low(ebx, s_.flags.logic(low(ebx) & low(ecx), 1));
        low(eax, s_.flags.logic(low(eax) & low(ebx), 1));
    }

    std::uint16_t cluster(std::uint32_t record, unsigned offset, std::uint16_t none) {
        const auto value = load(0x31619, record + offset, 2);
        s_.flags.minus(value, none, 2);
        return std::uint16_t(value);
    }

    std::uint32_t unclustered_flags(std::uint32_t definition, std::uint32_t mask) {
        reg(ecx) = load(0x31620, definition);
        s_.flags.logic(reg(ecx) & mask, 1);
        return reg(ecx);
    }

    bool uses_view_cluster(std::uint32_t flags, std::uint32_t mask) {
        return s_.flags.logic(flags & mask, 4) != 0;
    }

    void finish(bool allowed) {
        reg(esi) = pop(allowed ? 0x3163b : 0x31630);
        reg(ecx) = allowed ? 1 : s_.flags.logic(0, 4);
        reg(ebp) = pop(allowed ? 0x31641 : 0x31633);
        low(eax, s_.flags.logic(low(eax) & low(ecx), 1));
        reg(ebx) = pop(allowed ? 0x31644 : 0x31636);
        reg(ecx) = pop(allowed ? 0x31645 : 0x31637);
        finish_return(allowed ? 0x31646 : 0x31638, 4);
    }

    void reject() {
        low(eax, s_.flags.logic(0, 1));
        reg(ebx) = pop(0x3164b);
        reg(ecx) = pop(0x3164c);
        finish_return(0x3164d, 4);
    }
};

template<class State, class Call, class Before = native_light_visibility_detail::Noop>
bool bridge(State& state, Call call, Before before = {}) {
    if(!native_light_visibility_detail::admitted(state, 0x31570, 0x3164d)) return false;
    Adapter<State, Call, Before> frame(state, call, before);
    logic::admit(frame);
    return true;
}
} // namespace halo2::native_light_admission_detail
