#pragma once

#include "core/native_tu15_authored_function_systems.hpp"
#include "gameplay/native_tu15_small_queries_common.hpp"

namespace halo2::native_authored_function_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic = native_authored_function;
using logic::Access;
using logic::Local;

enum Branch : unsigned {
    null_blob, empty_blob, invalid_kind, first, second, second_disabled,
    zero_kind, deferred, low_clamp, high_clamp, within_clamp, branch_count
};
inline std::atomic<std::uint64_t> &coverage() {
    static std::atomic<std::uint64_t> value{};
    return value;
}

struct RouteCounts {
    std::atomic<std::uint64_t> completed{}, resumed{};
    ~RouteCounts() {
        if (completed || resumed)
            std::fprintf(stderr, "Authored function routes: %llu completed in the entry adapter, %llu continued at case labels\n",
                static_cast<unsigned long long>(completed.load()), static_cast<unsigned long long>(resumed.load()));
    }
};

template<class State>
struct Frame : Port<State> {
    Frame(State &state, std::uint32_t *cursor, Trace<State> *trace)
        : Port<State>(state, cursor, trace, {}) {}

    std::uint32_t local(Local slot) const { return this->gp[esp] + std::uint32_t(slot); }
    void reserve_frame() {
        auto &gp = this->gp;
        gp[esp] = this->flags.minus(gp[esp], 0x10u, 4);
        this->s.xmm[5] = _mm_setzero_ps();
        this->push(0x13de76u, gp[ebx]);
    }
    void save_registers() {
        this->push(0x13de7cu, this->gp[ebp]);
        this->push(0x13de7du, this->gp[esi]);
    }
    std::uint32_t instruction(Access access, std::uint32_t) const {
        using enum Access;
        switch (access) {
        case blob: return 0x13de77u;
        case seed_result: return 0x13de7eu;
        case count: return 0x13de8au;
        case kind: return 0x13de93u;
        case case_target: return 0x13dea4u;
        case first_parameter: return 0x13deabu;
        case store_first: return 0x13deb1u;
        case second_parameter_enabled: return 0x13debcu;
        case second_parameter: return 0x13dec2u;
        case store_second: return 0x13dec8u;
        case disabled_result: return 0x13ded5u;
        case lower_compare: return 0x13e0d2u;
        case upper_compare: return 0x13e5d5u;
        case result: return 0x13e5e2u;
        }
        return 0;
    }
    void loaded(Access access, std::uint32_t, std::uint32_t value) {
        auto &gp = this->gp;
        auto &x = this->s.xmm;
        using enum Access;
        switch (access) {
        case blob:
            gp[ebx] = value;
            this->test32(value);
            if (!value) this->reached(null_blob);
            break;
        case kind:
            set_low_byte(gp[eax], value);
            gp[ecx] = value;
            this->compare32(value, logic::kind_count - 1u);
            gp[esi] = gp[ebx] + 0x14u;
            if (value >= logic::kind_count) this->reached(invalid_kind);
            break;
        case first_parameter:
            this->reached(first);
            x[0] = _mm_set_ss(physics_step_detail::f(value));
            break;
        case second_parameter_enabled:
            this->test8(value & 1u);
            if (this->flags.zf) this->reached(second_disabled);
            break;
        case second_parameter:
            this->reached(second);
            [[fallthrough]];
        case upper_compare:
        case result: x[0] = _mm_set_ss(physics_step_detail::f(value)); break;
        case lower_compare: x[1] = _mm_set_ss(physics_step_detail::f(value)); break;
        default: break;
        }
    }
    void stored(Access, std::uint32_t, std::uint32_t) {}
    bool has_data(std::uint32_t count) {
        this->compare32(count, 0u);
        const bool present = !this->flags.zf && this->flags.sf == this->flags.of;
        if (!present) this->reached(empty_blob);
        return present;
    }
    [[noreturn]] void unknown_target() const {
        throw std::runtime_error("Required original solver dispatch target unavailable");
    }
    bool below_zero() {
        const auto &x = this->s.xmm;
        this->flags.scalar_compare(x[5], x[1], false);
        return !this->flags.cf && !this->flags.zf;
    }
    bool above_one() {
        const auto &x = this->s.xmm;
        this->flags.scalar_compare(x[1], x[0], false);
        const bool clamped = !this->flags.cf && !this->flags.zf;
        this->reached(clamped ? high_clamp : within_clamp);
        return clamped;
    }

    void restore_disabled_arguments() {
        this->gp[esi] = this->pop(0x13ded3u);
        this->gp[ebp] = this->pop(0x13ded4u);
    }
    void return_to_caller(std::uint32_t pc) {
        this->gp[esp] = this->flags.plus(this->gp[esp], 0x10u, 4);
        this->ret(pc, 8u);
        this->publish(pc);
    }
    void finish_disabled() {
        this->s.xmm[0] = this->s.xmm[5];
        this->gp[ebx] = this->pop(0x13dedeu);
        return_to_caller(0x13dee2u);
    }
    void finish_at_zero() {
        this->reached(low_clamp);
        this->gp[esi] = this->pop(0x13e0e1u);
        this->gp[ebp] = this->pop(0x13e0e2u);
        this->s.xmm[0] = this->s.xmm[5];
        this->gp[ebx] = this->pop(0x13e0e6u);
        return_to_caller(0x13e0eau);
    }
    void finish() {
        this->gp[esi] = this->pop(0x13e5e8u);
        this->gp[ebp] = this->pop(0x13e5e9u);
        this->gp[ebx] = this->pop(0x13e5eau);
        return_to_caller(0x13e5eeu);
    }
    template<class Body>
    void preserve_fault_abi(Body body) {
        try {
            body();
        } catch (...) {
            this->s.gp = this->gp;
            this->s.flags = this->flags;
            throw;
        }
    }
};

template<class State, class CanonicalBranch>
struct Adapter : Frame<State> {
    CanonicalBranch &canonical_branch;
    std::uint32_t &continuation;

    Adapter(State &state, std::uint32_t *cursor, Trace<State> *trace,
        CanonicalBranch &resolve_target, std::uint32_t &result)
        : Frame<State>(state, cursor, trace), canonical_branch(resolve_target), continuation(result) {}

    std::uint32_t resolve(std::uint32_t target) {
        const auto result = canonical_branch(target, this->s.image);
        if (result == std::uint32_t(logic::Target::zero)) this->reached(zero_kind);
        return result;
    }

    void run() {
        Memory<Adapter> memory{*this};
        this->preserve_fault_abi([&] {
            continuation = logic::evaluate(memory, *this, this->gp[edi]);
            if (continuation != logic::completed) {
                this->reached(deferred);
                this->publish(0x13dea4u);
            }
        });
    }
};

template<class State>
struct Clamp : Frame<State> {
    using Frame<State>::Frame;

    void run() {
        Memory<Clamp> memory{*this};
        this->preserve_fault_abi([&] { logic::clamp_result(memory, *this); });
    }
};

inline std::atomic<std::uint64_t> &clamp_coverage() {
    static std::atomic<std::uint64_t> value{};
    return value;
}

template<class State, class Reference>
bool bridge_clamp(State &state, Reference reference) {
    static const bool use_reference = native_small_queries_detail::flag("HALO2_REFERENCE_AUTHORED_FUNCTION_BOUNDED") ||
        native_small_queries_detail::flag("HALO2_REFERENCE_AUTHORED_CLAMP");
    static const bool verify = native_small_queries_detail::flag("HALO2_VERIFY_AUTHORED_CLAMP");
    static VerifySummary summary{"Authored function clamp"};
    return run_owner(state, 0x13e0d2u, 0x13e5eeu, use_reference, verify, clamp_coverage(), summary, reference,
        [](State &value, std::uint32_t *cursor, Trace<State> *trace) {
            return Clamp<State>{value, cursor, trace};
        });
}


// A nonzero case target continues in the retained body after the native
// entry. The verification reference likewise stops before executing that
// case so a residual call or write still happens just once.
template<class State, class Reference, class CanonicalBranch>
std::uint32_t bridge(State &state, Reference reference, CanonicalBranch canonical_branch) {
    static const bool use_reference = native_small_queries_detail::flag("HALO2_REFERENCE_AUTHORED_FUNCTION_BOUNDED");
    static const bool verify = native_small_queries_detail::flag("HALO2_VERIFY_AUTHORED_FUNCTION_BOUNDED");
    static VerifySummary summary{"Authored function prefix"};
    std::uint32_t continuation = logic::retained_entry;
    const auto verify_continuation = [&](State &original) {
        if (reference(original) != continuation)
            throw std::runtime_error("Native TU1.5 authored function continuation differs from the original");
    };
    run_owner(state, 0x13de70u, 0x13e5eeu, use_reference, verify, coverage(), summary, verify_continuation,
        [&](State &value, std::uint32_t *cursor, Trace<State> *trace) {
            return Adapter<State, CanonicalBranch>{value, cursor, trace, canonical_branch, continuation};
        });
    if (continuation != logic::retained_entry && (verify || native_census_detail::enabled())) {
        static RouteCounts counts;
        auto &counter = continuation == logic::completed ? counts.completed : counts.resumed;
        counter.fetch_add(1, std::memory_order_relaxed);
    }
    return continuation;
}
} // namespace halo2::native_authored_function_detail
