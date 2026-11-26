#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/vehicles/native_tu15_weapon_magnification_systems.hpp"
#include "physics/runtime/native_tu15_live_support_systems.hpp"
#include "physics/runtime/physics_solver_internal.hpp"
#include <bit>

namespace halo2::native_weapon_magnification_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::run_owner;
using native_small_queries_detail::VerifySummary;
namespace logic = native_weapon_magnification;
namespace fp_math = physics_step_detail;
using logic::Access;
using logic::Local;

enum Branch : unsigned {
    negative_level, unavailable_level, single_level, multiple_levels,
    minimum_positive, minimum_default, maximum_positive, maximum_default, branch_count
};
inline std::atomic<std::uint64_t> &coverage() {
    static std::atomic<std::uint64_t> value{};
    return value;
}

inline __m128 scalar(std::uint32_t word) { return _mm_set_ss(fp_math::f(word)); }
inline std::uint32_t word(__m128 value) { return fp_math::u(_mm_cvtss_f32(value)); }
inline std::int32_t signed_word(std::uint32_t value) {
    return std::bit_cast<std::int16_t>(std::uint16_t(value));
}

template<class State, class Power>
struct Adapter : Port<State> {
    Power &power;
    bool single{};
    bool use_minimum{};

    Adapter(State &s, std::uint32_t *cursor, Trace<State> *trace, Power &child)
        : Port<State>(s, cursor, trace, {}), power(child) {}

    std::uint32_t local(Local slot) const { return this->gp[esp] + std::uint32_t(slot); }
    std::uint32_t instruction(Access access, std::uint32_t) {
        auto &gp = this->gp;
        using enum Access;
        switch (access) {
        case objects: return 0x106160u;
        case headers: return 0x106166u;
        case one: return 0x106169u;
        case weapon:
            gp[eax] = 3u * this->flags.logic(gp[eax] & logic::layout::handle_mask, 4);
            return 0x106179u;
        case tag: return 0x10617du;
        case definitions: return 0x10617fu;
        case definition:
            gp[edx] = this->flags.shift(this->flags.logic(gp[edx] & logic::layout::handle_mask, 4), 4u, 4);
            return 0x10618du;
        case level_count: return 0x10619du;
        case store_fraction: return single ? 0x1061cdu : 0x1061c5u;
        case minimum: return 0x1061d3u;
        case store_minimum: return use_minimum ? 0x1061e0u : 0x1061e7u;
        case maximum: return 0x1061ecu;
        case store_maximum: return 0x1061fcu;
        case ratio_maximum: return 0x106202u;
        case ratio_minimum: return 0x106206u;
        case ratio_fraction: return 0x106209u;
        case result_minimum: return 0x106212u;
        case store_result: return 0x106215u;
        case result: return 0x106219u;
        }
        return 0;
    }
    void loaded(Access access, std::uint32_t, std::uint32_t value) {
        auto &gp = this->gp;
        auto &x = this->s.xmm;
        using enum Access;
        switch (access) {
        case objects:
        case headers:
        case tag: gp[edx] = value; break;
        case one:
        case result: x[0] = scalar(value); break;
        case weapon:
        case definitions:
        case definition: gp[eax] = value; break;
        case level_count: gp[edx] = (gp[edx] & 0xffff0000u) | value; break;
        case minimum:
        case maximum: x[2] = scalar(value); break;
        default: break;
        }
    }
    void stored(Access, std::uint32_t, std::uint32_t) {}

    void reserve_locals() { this->gp[esp] = this->flags.minus(this->gp[esp], 12u, 4); }
    bool level_is_negative(std::uint32_t level) {
        this->flags.logic(level, 2);
        if (this->flags.sf) this->reached(negative_level);
        return this->flags.sf;
    }
    bool level_is_available(std::uint32_t level, std::uint32_t count) {
        this->flags.minus(level, count, 2);
        const bool valid = this->flags.sf != this->flags.of;
        if (!valid) this->reached(unavailable_level);
        return valid;
    }
    std::uint32_t fraction(std::uint32_t level, std::uint32_t count) {
        auto &x = this->s.xmm;
        this->flags.minus(count, 1u, 2);
        x[1] = _mm_setzero_ps();
        single = this->flags.zf || this->flags.sf != this->flags.of;
        this->reached(single ? single_level : multiple_levels);
        if (single) return 0;
        auto &gp = this->gp;
        gp[ecx] = std::uint32_t(signed_word(level));
        gp[edx] = this->flags.increment(std::uint32_t(signed_word(count)), 4, true);
        x[2] = _mm_cvtsi32_ss(x[2], signed_word(level));
        x[3] = _mm_cvtsi32_ss(x[3], fp_math::si(gp[edx]));
        x[2] = _mm_div_ss(x[2], x[3]);
        return word(x[2]);
    }
    bool endpoint_is_positive() {
        const auto &x = this->s.xmm;
        this->flags.scalar_compare(x[2], x[1], false);
        return !this->flags.cf && !this->flags.zf;
    }
    bool minimum_is_positive() {
        use_minimum = endpoint_is_positive();
        this->reached(use_minimum ? minimum_positive : minimum_default);
        return use_minimum;
    }
    std::uint32_t maximum_or_default(std::uint32_t maximum, std::uint32_t default_scale) {
        const bool use_maximum = endpoint_is_positive();
        this->reached(use_maximum ? maximum_positive : maximum_default);
        if (use_maximum) this->s.xmm[0] = this->s.xmm[2];
        return use_maximum ? maximum : default_scale;
    }

    void push_float(std::uint32_t value) { this->s.fp.push(fp_math::ext(fp_math::f(value))); }
    void divide_by(std::uint32_t minimum) {
        auto &top = this->s.fp.st(0);
        top = physics_solver_detail::divide(top, fp_math::ext(fp_math::f(minimum)));
    }
    void raise_ratio() { this->call(0x10620du, 0x106212u, 0x37cde8u, power); }
    void multiply_by(std::uint32_t minimum) {
        auto &top = this->s.fp.st(0);
        top = fp_math::mul(top, fp_math::ext(fp_math::f(minimum)));
    }
    std::uint32_t round_result() {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        this->at(0x106215u);
        auto &fp = this->s.fp;
        const auto rounded = native_live_support::x87::store_single(fp.st(0));
        fp.status = (fp.status & ~0x200u) | (rounded.rounded_up ? 0x200u : 0u);
        return std::uint32_t(rounded.bits);
#else
        throw std::runtime_error("Required TU1.5 FSTP32 backend unavailable");
#endif
    }
    void pop_result() { this->s.fp.pop(); }

    void run() {
        Memory<Adapter> memory{*this};
        logic::compute(memory, *this, this->gp[eax], this->gp[ecx] & 0xffffu);
        this->gp[esp] = this->flags.plus(this->gp[esp], 12u, 4);
        this->ret(0x106222u, 0);
        this->publish(0x106222u);
    }
};

// The power helper consumes ST(0) and ST(1), leaving its result on ST(0).
// Keep that call on its original route and spill the result with source rounding.
template<class State, class Reference, class Power>
bool bridge(State &s, Reference reference, Power power) {
    static const bool use_reference = native_small_queries_detail::flag("HALO2_REFERENCE_WEAPON_MAGNIFICATION");
    static const bool verify = native_small_queries_detail::flag("HALO2_VERIFY_WEAPON_MAGNIFICATION");
    static VerifySummary summary{"Weapon magnification"};
    return run_owner(s, 0x106160u, 0x106222u, use_reference, verify, coverage(), summary, reference,
        [&](State &v, std::uint32_t *c, Trace<State> *t) { return Adapter<State, Power>(v, c, t, power); });
}
} // namespace halo2::native_weapon_magnification_detail
