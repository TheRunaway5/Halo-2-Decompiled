#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_live_support_systems.hpp"
#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"
#include "rendering/effects/native_tu15_particle_property_systems.hpp"

namespace halo2::native_particle_property_detail {
using native_object_tick_detail::Trace;
using native_object_tick_detail::sx16;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::run_owner;
using native_small_queries_detail::VerifySummary;
using native_tu15_leaves::ordered_mul_ss;
using native_world_leaves_detail::ordered_add_ss;
namespace logic = halo2::native_particle_property;
namespace x87 = native_live_support::x87;
namespace fp_math = physics_step_detail;
using logic::Access;
using logic::Modifier;

enum Branch : unsigned { authored, below, between, above, absent, negative, add, multiply, other_mode, branch_count };
inline std::atomic<std::uint64_t> &coverage() {
    static std::atomic<std::uint64_t> value{};
    return value;
}

inline long double load_operand(std::uint32_t bits) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    return x87::load_single(bits);
#else
    return fp_math::ext(fp_math::f(bits));
#endif
}

inline std::uint32_t spill_word(long double value, std::uint16_t &status) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    const auto word = x87::store_single(value);
    status = std::uint16_t((status & ~0x200u) | (word.rounded_up ? 0x200u : 0u));
    return std::uint32_t(word.bits);
#else
    throw std::runtime_error("Required TU1.5 FSTP32 backend unavailable");
#endif
}

template<class State, class Dispatch>
struct Adapter : Port<State> {
    Dispatch &dispatch;
    std::uint32_t exit_pc{0x250082u};

    Adapter(State &state, std::uint32_t *cursor, Trace<State> *trace, Dispatch &route)
        : Port<State>(state, cursor, trace, {}), dispatch(route) {}

    std::uint32_t property() const { return this->gp[esi]; }
    std::uint32_t curve_address() const { return this->gp[edi]; }
    std::uint32_t input_address(std::uint32_t index) const {
        return this->gp[ebx] + sx16(index) * 4u;
    }

    std::uint32_t instruction(Access access, std::uint32_t) {
        using enum Access;
        switch (access) {
        case upper_index: return 0x24ffe3u;
        case lower_index: return 0x24ffe7u;
        case curve: return 0x250009u;
        case curve_type: return 0x25000cu;
        case minimum_value: return 0x250018u;
        case maximum_value: return 0x25001du;
        case unit_value: return 0x250024u;
        case extra_index: return 0x250043u;
        case extra_mode: return 0x250053u;
        case extra_add: this->gp[edx] = sx16(this->gp[eax]); return 0x250073u;
        case extra_multiply: this->gp[ecx] = sx16(this->gp[eax]); return 0x250060u;
        }
        return 0;
    }
    void loaded(Access access, std::uint32_t, std::uint32_t value) {
        using enum Access;
        switch (access) {
        case upper_index: this->gp[eax] = sx16(value); break;
        case lower_index: this->gp[ecx] = sx16(value); break;
        case curve: this->gp[edi] = value; break;
        case minimum_value: this->s.xmm[2] = _mm_set_ss(fp_math::f(value)); break;
        case maximum_value: this->s.xmm[1] = _mm_set_ss(fp_math::f(value)); break;
        case extra_index: this->gp[eax] = (this->gp[eax] & 0xffff0000u) | value; break;
        case extra_mode: this->gp[ecx] = value; break;
        default: break;
        }
    }

    void spill(std::uint32_t pc, std::uint32_t address) {
        auto &fp = this->s.fp;
        this->at(pc);
        const auto stored = spill_word(fp.st(0), fp.status);
        this->store(pc, address, 4, stored);
        fp.pop();
    }
    void evaluate_curve(std::uint32_t upper, std::uint32_t lower) {
        auto &gp = this->gp;
        auto &fp = this->s.fp;
        fp.push(load_operand(this->read(0x24ffeau, input_address(upper), 4)));
        spill(0x24ffedu, gp[esp]);
        gp[edx] = this->read(0x24fff0u, gp[esp], 4);
        this->push(0x24fff3u, gp[edi]);
        fp.push(load_operand(this->read(0x24fff4u, input_address(lower), 4)));
        this->push(0x24fff7u, gp[edx]);
        spill(0x24fff8u, gp[esp] + 0xcu);
        gp[eax] = this->read(0x24fffcu, gp[esp] + 0xcu, 4);
        gp[edi] = property() + logic::curve_reference;
        this->push(0x250003u, gp[eax]);
        this->call_routed(dispatch, 0x250004u, 0x250009u, 0x13de70u);
    }

    bool has_range(std::uint32_t flags) {
        this->test8(flags & logic::curve_type_mask);
        if (!this->flags.zf) this->reached(authored);
        return this->flags.zf;
    }
    bool below_zero() {
        auto &x = this->s.xmm;
        x[3] = _mm_setzero_ps();
        this->flags.scalar_compare(x[3], x[0], false);
        return !this->flags.cf && !this->flags.zf;
    }
    void use_zero() {
        this->reached(below);
        this->s.xmm[0] = this->s.xmm[3];
    }
    void limit_to_one(std::uint32_t one) {
        auto &x = this->s.xmm;
        x[3] = _mm_set_ss(fp_math::f(one));
        this->flags.scalar_compare(x[0], x[3], false);
        if (!this->flags.cf && !this->flags.zf) {
            this->reached(above);
            x[0] = x[3];
        } else this->reached(between);
    }
    void expand_range() {
        auto &x = this->s.xmm;
        x[1] = _mm_sub_ss(x[1], x[2]);
        x[1] = ordered_mul_ss(x[1], x[0]);
        x[0] = x[1] = ordered_add_ss(x[1], x[2]);
    }
    bool restore_and_check_modifier(std::uint32_t index) {
        this->flags.minus(index, 0xffffu, 2);
        this->gp[edi] = this->pop(0x25004bu);
        if (this->flags.zf) {
            this->reached(absent);
            return false;
        }
        this->flags.logic(index, 2);
        if (this->flags.sf) this->reached(negative);
        return !this->flags.sf;
    }
    Modifier choose_modifier(std::uint32_t mode) {
        auto &ecx_value = this->gp[ecx];
        ecx_value = this->flags.increment(mode, 4, true);
        if (this->flags.zf) return Modifier::add;
        ecx_value = this->flags.increment(ecx_value, 4, true);
        if (this->flags.zf) return Modifier::multiply;
        this->reached(other_mode);
        return Modifier::none;
    }
    void modify(std::uint32_t value, Modifier modifier) {
        auto &x = this->s.xmm;
        x[1] = _mm_set_ss(fp_math::f(value));
        if (modifier == Modifier::add) {
            this->reached(add);
            x[1] = ordered_add_ss(x[1], x[0]);
        } else {
            this->reached(multiply);
            x[1] = ordered_mul_ss(x[1], x[0]);
            exit_pc = 0x25006fu;
        }
        x[0] = x[1];
    }
    void run() {
        auto &stack = this->gp[esp];
        stack = this->flags.minus(stack, 8u, 4);
        Memory<Adapter> memory{*this};
        logic::evaluate(memory, *this);
        stack = this->flags.plus(stack, 8u, 4);
        this->ret(exit_pc, 0u);
        this->publish(exit_pc);
    }
};

template<class State, class Reference, class Dispatch>
bool bridge(State &state, Reference reference, Dispatch dispatch) {
    static const bool use_reference = flag("HALO2_REFERENCE_PARTICLE_PROPERTY");
    static const bool verify = flag("HALO2_VERIFY_PARTICLE_PROPERTY");
    static VerifySummary summary{"Particle property"};
    return run_owner(state, 0x24ffe0u, 0x250082u, use_reference, verify, coverage(), summary, reference,
        [&](State &value, std::uint32_t *cursor, Trace<State> *trace) {
            return Adapter<State, Dispatch>{value, cursor, trace, dispatch};
        });
}
} // namespace halo2::native_particle_property_detail
