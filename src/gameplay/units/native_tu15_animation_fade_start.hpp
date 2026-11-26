#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/units/native_tu15_animation_fade_start_systems.hpp"
#include "physics/runtime/physics_contact_runtime_internal.hpp"
#include <algorithm>

namespace halo2::native_animation_fade_start_detail {
using native_object_tick_detail::Trace;
using native_object_tick_detail::sx8;
using native_object_tick_detail::sx16;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::run_owner;
using native_small_queries_detail::VerifySummary;
namespace logic = halo2::native_animation_fade_start;
namespace fp_math = halo2::physics_step_detail;
using logic::Access;

enum Branch : unsigned { cleared, retained_counter, rescaled, initial_forward, initial_reverse, clamped_high, branch_count };
inline std::atomic<std::uint64_t> &coverage() {
    static std::atomic<std::uint64_t> value{0};
    return value;
}

template<class State>
struct Adapter : Port<State> {
    Adapter(State &s, std::uint32_t *cursor, Trace<State> *trace)
        : Port<State>(s, cursor, trace, {}) {}

    std::uint32_t instruction(Access access, std::uint32_t) {
        using enum Access;
        switch (access) {
        case clock_pointer: return 0x1e14f1u;
        case clock_rate: return 0x1e14f6u;
        case previous_length: return 0x1e1533u;
        case previous_direction: return 0x1e153au;
        case matching_length: return 0x1e1545u;
        case rescale_length: return 0x1e154du;
        case unit_scale: return 0x1e1554u;
        case previous_position: return 0x1e156bu;
        case scaled_length:
        case initial_length:
            exit_pc = access == scaled_length ? 0x1e159fu : 0x1e15bcu;
            this->test8(this->gp[ebx]);
            return access == scaled_length ? 0x1e1593u : 0x1e15b0u;
        case scaled_position: return 0x1e1599u;
        case scaled_direction: return 0x1e159bu;
        case initial_position: return 0x1e15b6u;
        case initial_direction: return 0x1e15b8u;
        case clear_length: return 0x1e15bfu;
        case clear_position: return 0x1e15c3u;
        case clear_direction: return 0x1e15c6u;
        }
        return 0;
    }
    void loaded(Access access, std::uint32_t, std::uint32_t value) {
        using enum Access;
        switch (access) {
        case clock_pointer:
        case previous_direction: this->gp[eax] = value; break;
        case previous_length:
        case rescale_length: set_low_byte(this->gp[eax], value); break;
        case matching_length:
        case previous_position: this->gp[eax] = sx8(value); break;
        case clock_rate: this->gp[edx] = sx16(value); break;
        default: break;
        }
    }
    void stored(Access access, std::uint32_t, std::uint32_t) {
        if (access == Access::scaled_length || access == Access::initial_length)
            set_low_byte(this->gp[edx], !this->flags.zf);
    }

    std::uint32_t duration(std::uint32_t rate) {
        auto &x = this->s.xmm[0];
        auto &gp = this->gp;
        auto &fp = this->s.fp;
        const auto argument = gp[esp] + 8u;
        x = _mm_cvtsi32_ss(x, fp_math::si(sx16(rate)));
        const auto seconds = this->read(0x1e14feu, argument, 4);
        x = _mm_mul_ss(x, _mm_set_ss(fp_math::f(seconds)));
        this->store(0x1e1504u, argument, 4, fp_math::u(_mm_cvtss_f32(x)));
        const auto scaled = this->read(0x1e150au, argument, 4);
        fp.push(fp_math::ext(fp_math::f(scaled)));
        this->at(0x1e150eu);
        const auto rounded = physics_contact_runtime_detail::integer_spill(fp.st(0), fp.status);
        this->store(0x1e150eu, gp[esp], 4, rounded);
        fp.pop();
        const auto ticks = fp_math::si(this->read(0x1e1511u, gp[esp], 4));
        if (ticks > std::int32_t(logic::max_ticks)) this->reached(clamped_high);
        const auto clamped = std::clamp(ticks, 1, std::int32_t(logic::max_ticks));
        auto &out = gp[edx];
        out = std::uint32_t(clamped - 1);
        this->test32(out);
        if (!out) this->reached(cleared);
        return out;
    }
    bool has_previous(std::uint32_t previous) {
        this->test8(previous);
        return !this->flags.zf;
    }
    bool same_direction(std::uint32_t active, std::uint32_t previous) {
        this->gp[eax] = this->flags.logic(previous & 1u, 4);
        this->flags.minus(active, this->gp[eax], 1);
        return this->flags.zf;
    }
    bool same_length(std::uint32_t duration, std::uint32_t previous) {
        this->compare32(duration, sx8(previous));
        return this->flags.zf;
    }
    void retained() { this->reached(retained_counter); }

    void select_divisor(std::uint32_t previous, std::uint32_t unit) {
        auto &x = this->s.xmm;
        x[1] = _mm_set_ss(fp_math::f(unit));
        this->gp[eax] = sx8(previous);
        x[0] = _mm_cvtsi32_ss(x[0], fp_math::si(this->gp[eax]));
        this->flags.scalar_compare(x[0], x[1], false);
        if (this->flags.cf || this->flags.zf) x[0] = x[1];
    }
    std::uint32_t rescale(std::uint32_t previous, std::uint32_t duration) {
        this->reached(rescaled);
        auto &x = this->s.xmm;
        auto &gp = this->gp;
        x[1] = _mm_cvtsi32_ss(x[1], fp_math::si(sx8(previous)));
        x[1] = _mm_div_ss(x[1], x[0]);
        gp[eax] = duration - 1u;
        x[0] = _mm_cvtsi32_ss(x[0], fp_math::si(gp[eax]));
        x[1] = _mm_mul_ss(x[1], x[0]);
        const auto slot = gp[esp] + 8u;
        this->store(0x1e1581u, slot, 4, fp_math::u(_mm_cvtss_f32(x[1])));
        const auto adjusted = this->read(0x1e1587u, slot, 4);
        x[0] = _mm_set_ss(fp_math::f(adjusted));
        return gp[eax] = std::uint32_t(_mm_cvttss_si32(x[0]));
    }
    std::uint32_t initial_position(std::uint32_t duration, std::uint32_t active) {
        this->test8(active);
        this->reached(active ? initial_forward : initial_reverse);
        return this->gp[eax] = active ? 1u : duration - 1u;
    }
    void run() {
        this->push(0x1e14f0u, this->gp[ecx]);
        Memory<Adapter> memory{*this};
        logic::start(memory, *this, this->gp[ecx], low_byte(this->gp[ebx]));
        this->gp[ecx] = this->pop(exit_pc - 1u);
        this->ret(exit_pc, 4u);
        this->publish(exit_pc);
    }
    std::uint32_t exit_pc{0x1e15cbu};
};

template<class State, class Reference>
bool bridge(State &state, Reference reference) {
    static const bool use_reference = native_small_queries_detail::flag("HALO2_REFERENCE_ANIMATION_FADE_START");
    static const bool verify = native_small_queries_detail::flag("HALO2_VERIFY_ANIMATION_FADE_START");
    static VerifySummary summary{"Animation fade start"};
    return run_owner(state, 0x1e14f0u, 0x1e15cbu, use_reference, verify, coverage(), summary, reference,
        [](State &value, std::uint32_t *cursor, Trace<State> *trace) { return Adapter<State>{value, cursor, trace}; });
}
} // namespace halo2::native_animation_fade_start_detail
