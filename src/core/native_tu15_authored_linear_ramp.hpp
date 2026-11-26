#pragma once

#include "core/native_tu15_authored_linear_ramp_systems.hpp"
#include "gameplay/native_tu15_small_queries_common.hpp"

namespace halo2::native_authored_linear_ramp {
using native_object_tick_detail::Trace;
using native_object_tick_detail::boundary::Port;
using native_object_tick_detail::boundary::ebx;
using native_object_tick_detail::boundary::esi;
using native_object_tick_detail::boundary::esp;
using systems::Layer;
using systems::Segment;

enum class Kind : std::uint32_t { linear = 0x13e0edu, ramp = 0x13e146u };
enum Branch : unsigned {
    linear_single, linear_blended, ramp_single, ramp_blended,
    ramp_low, ramp_high, ramp_unordered, ramp_in_range, branch_count
};

inline std::atomic<std::uint64_t> &coverage() {
    static std::atomic<std::uint64_t> value{};
    return value;
}

inline void count(Kind kind) {
    struct Totals {
        std::atomic<std::uint64_t> linear{}, ramp{};
        ~Totals() {
            if (auto value = linear.load())
                std::fprintf(stderr, "Authored function linear route 13E0ED->13E0D2: %llu\n",
                    static_cast<unsigned long long>(value));
            if (auto value = ramp.load())
                std::fprintf(stderr, "Authored function ramp route 13E146->13E0D2: %llu\n",
                    static_cast<unsigned long long>(value));
        }
    };
    static Totals totals;
    auto &route = kind == Kind::linear ? totals.linear : totals.ramp;
    route.fetch_add(1, std::memory_order_relaxed);
}

struct Operand {
    std::uint32_t pc, displacement;
};

template<class State>
struct Adapter : Port<State> {
    Kind kind;
    std::uint32_t exit{};

    Adapter(State &state, std::uint32_t *cursor, Trace<State> *trace, Kind shape)
        : Port<State>{state, cursor, trace, {}}, kind(shape) {}

    __m128 scalar(std::uint32_t pc, std::uint32_t address) {
        return _mm_set_ss(physics_step_detail::f(this->read(pc, address, 4)));
    }
    __m128 coefficient(Operand source) {
        return scalar(source.pc, this->gp[esi] + source.displacement);
    }
    __m128 argument(std::uint32_t pc, std::uint32_t offset) {
        return scalar(pc, this->gp[esp] + offset);
    }
    __m128 input() {
        return argument(kind == Kind::linear ? 0x13e0f2u : 0x13e146u, 0x20u);
    }
    __m128 blend_amount() {
        return argument(kind == Kind::linear ? 0x13e112u : 0x13e27au, 0x24u);
    }
    __m128 base_result() {
        return argument(kind == Kind::linear ? 0x13e130u : 0x13e289u, 0xcu);
    }
    __m128 unit(Layer layer) {
        const auto pc = kind == Kind::linear ? 0x13e121u : layer == Layer::base ? 0x13e14cu : 0x13e1efu;
        return scalar(pc, 0x46a0d0u);
    }
    __m128 linear_slope(Layer layer) {
        return coefficient(layer == Layer::base ? Operand{0x13e0edu, 0x10u} : Operand{0x13e10du, 0x28u});
    }
    __m128 linear_offset(Layer layer) {
        return coefficient(layer == Layer::base ? Operand{0x13e100u, 0x14u} : Operand{0x13e11cu, 0x2cu});
    }
    __m128 ramp_threshold(Layer layer, Segment segment) {
        constexpr std::uint32_t sites[2][3]{{0x13e157u, 0x13e179u, 0x13e19bu}, {0x13e1fdu, 0x13e21fu, 0x13e23bu}};
        const auto index = unsigned(segment);
        return coefficient({sites[unsigned(layer)][index], 0x24u + 4u * index});
    }
    __m128 ramp_slope(Layer layer, Segment segment) {
        constexpr std::uint32_t sites[2][3]{{0x13e15cu, 0x13e17eu, 0x13e1a0u}, {0x13e202u, 0x13e224u, 0x13e240u}};
        const auto index = unsigned(segment);
        return coefficient({sites[unsigned(layer)][index], 0x34u + 4u * index});
    }
    __m128 ramp_coefficient(Layer layer, Segment segment) {
        constexpr std::uint32_t sites[2][3]{{0x13e1d1u, 0x13e1c4u, 0x13e1b7u}, {0x13e269u, 0x13e260u, 0x13e257u}};
        const auto index = unsigned(segment);
        return coefficient({sites[unsigned(layer)][index], 0x44u + 4u * index});
    }
    __m128 ramp_offset(Layer layer) {
        return coefficient({layer == Layer::base ? 0x13e1deu : 0x13e280u, 0x40u});
    }
    void store_result(Layer layer, __m128 result) {
        const auto base = layer == Layer::base;
        const auto pc = kind == Kind::linear ? (base ? 0x13e105u : 0x13e13eu) : (base ? 0x13e1e3u : 0x13e297u);
        this->store(pc, this->gp[esp] + 0xcu, 4, physics_step_detail::u(_mm_cvtss_f32(result)));
    }
    bool has_blend() {
        const auto pc = kind == Kind::linear ? 0x13e0f8u : 0x13e1bcu;
        this->test8(this->read(pc, this->gp[ebx] + 1u, 1) & 1u);
        const bool blended = !this->flags.zf;
        if (kind == Kind::linear) {
            this->reached(blended ? linear_blended : linear_single);
            exit = blended ? 0x13e144u : 0x13e10bu;
        } else {
            this->reached(blended ? ramp_blended : ramp_single);
            exit = blended ? 0x13e29du : 0x13e1e9u;
        }
        return blended;
    }
    void advance_layer() {
        const auto stride = kind == Kind::linear ? 0x18u : 0x50u;
        this->gp[esi] = this->flags.plus(this->gp[esi], stride, 4);
    }
    __m128 bound_sample(__m128 sample, __m128 zero, __m128 unit) {
        this->flags.scalar_compare(zero, sample, false);
        if (!this->flags.cf && !this->flags.zf) {
            this->reached(ramp_low);
            return zero;
        }
        this->flags.scalar_compare(sample, unit, false);
        if (!this->flags.cf && !this->flags.zf) {
            this->reached(ramp_high);
            return unit;
        }
        this->reached(this->flags.pf ? ramp_unordered : ramp_in_range);
        return sample;
    }

    void run() {
        auto &x = this->s.xmm;
        try {
            if (kind == Kind::linear) systems::evaluate_linear(*this, {x[0], x[1], x[2]});
            else systems::evaluate_ramp(*this, {x[0], x[1], x[2], x[3], x[4], x[5]});
            this->publish(exit);
        } catch (...) {
            this->s.gp = this->gp;
            this->s.flags = this->flags;
            throw;
        }
    }
};

// Both native case bodies rejoin the original shared clamp at 13E0D2.
template<class State, class Reference>
bool bridge(State &state, Kind kind, Reference reference) {
    using namespace native_small_queries_detail;
    static const bool use_reference = flag("HALO2_REFERENCE_AUTHORED_FUNCTION_BOUNDED") ||
        flag("HALO2_REFERENCE_AUTHORED_LINEAR_RAMP");
    static const bool verify = flag("HALO2_VERIFY_AUTHORED_LINEAR_RAMP");
    static VerifySummary summary{"Authored function linear/ramp"};
    const auto last = kind == Kind::linear ? 0x13e144u : 0x13e29du;
    const bool admitted = run_owner(state, std::uint32_t(kind), last, use_reference, verify, coverage(), summary, reference,
        [&](State &value, std::uint32_t *cursor, Trace<State> *trace) {
            return Adapter<State>{value, cursor, trace, kind};
        });
    if (admitted && (verify || native_census_detail::enabled())) count(kind);
    return admitted;
}
} // namespace halo2::native_authored_linear_ramp
