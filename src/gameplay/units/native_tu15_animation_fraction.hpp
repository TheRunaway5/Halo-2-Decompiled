#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/units/native_tu15_animation_fraction_systems.hpp"
#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include <bit>

namespace halo2::native_animation_fraction_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::run_owner;
using native_small_queries_detail::VerifySummary;
namespace logic = halo2::native_animation_fraction;
using logic::Access;
using logic::Exit;

enum Branch : unsigned { no_animation, lookup, negative, within_or_unordered, past_limit, branch_count };
inline std::atomic<std::uint64_t> &coverage() {
    static std::atomic<std::uint64_t> value{};
    return value;
}

template<class State, class Dispatch>
struct Adapter : Port<State> {
    Dispatch &dispatch;
    Exit exit{};

    Adapter(State &s, std::uint32_t *c, Trace<State> *t, Dispatch &route)
        : Port<State>(s, c, t, {}), dispatch(route) {}

    std::uint32_t argument_address() const { return this->gp[esp] + 4u; }

    std::uint32_t instruction(Access access, std::uint32_t) {
        using enum Access;
        switch (access) {
        case animation_index: return 0x1ce8d2u;
        case graph_handle: return 0x1ce8d9u;
        case tag_instances: return 0x1ce8dbu;
        case graph_address:
            this->gp[eax] = this->flags.shift(
                this->flags.logic(this->gp[eax] & 0xffffu, 4), logic::layout::instance_shift, 4);
            return 0x1ce8e9u;
        case animation_key: return 0x1ce8edu;
        case frame_count: return 0x1ce8f6u;
        case frame_margin: return 0x1ce902u;
        case fraction: return 0x1ce90du;
        case store_zero: return 0x1ce918u;
        case store_limit: return 0x1ce930u;
        case store_scaled: return 0x1ce938u;
        case reload_argument: return exit == Exit::negative ? 0x1ce91eu : 0x1ce93eu;
        case store_argument: return exit == Exit::negative ? 0x1ce922u : 0x1ce942u;
        }
        return 0;
    }
    void loaded(Access access, std::uint32_t, std::uint32_t value) {
        using enum Access;
        switch (access) {
        case animation_index:
            this->flags.minus(value, 0xffffu, 2);
            if (this->flags.zf) this->reached(no_animation);
            break;
        case graph_handle:
        case animation_key: this->gp[eax] = value; break;
        case tag_instances: this->gp[ecx] = value; break;
        case graph_address:
        case reload_argument: this->gp[edx] = value; break;
        default: break;
        }
    }
    void stored(Access, std::uint32_t, std::uint32_t) {}

    std::uint32_t lookup_animation(std::uint32_t graph, std::uint32_t key) {
        this->reached(lookup);
        this->gp[edx] = graph;
        this->push(0x1ce8f0u, key);
        this->call(0x1ce8f1u, 0x1ce8f6u, 0x1e3150u,
            [&](State &v) { dispatch(v, 0x1e3150u, 0x1ce8f1u, false); });
        return this->gp[eax];
    }
    void convert_count(std::uint32_t count) {
        auto &x = this->s.xmm;
        x[2] = _mm_setzero_ps();
        this->gp[ecx] = this->flags.increment(count, 4, true);
        x[0] = _mm_cvtsi32_ss(x[0], std::bit_cast<std::int32_t>(this->gp[ecx]));
    }
    std::uint32_t frame_limit(std::uint32_t margin) {
        auto &x = this->s.xmm;
        x[0] = _mm_add_ss(x[0], logic::layout::sse::scalar(margin));
        x[1] = x[0];
        return logic::layout::sse::bits(x[0]);
    }
    std::uint32_t scale_fraction(std::uint32_t fraction) {
        auto &x = this->s.xmm;
        x[1] = native_tu15_leaves::ordered_mul_ss(x[1], logic::layout::sse::scalar(fraction));
        return logic::layout::sse::bits(x[1]);
    }
    bool scaled_is_negative() {
        const auto &x = this->s.xmm;
        this->flags.scalar_compare(x[2], x[1], false);
        const bool below_zero = !this->flags.cf && !this->flags.zf;
        exit = below_zero ? Exit::negative : Exit::nonnegative;
        if (below_zero) this->reached(negative);
        return below_zero;
    }
    bool scaled_does_not_exceed_limit() {
        const auto &x = this->s.xmm;
        this->flags.scalar_compare(x[1], x[0], false);
        const bool fits = this->flags.cf || this->flags.zf;
        this->reached(fits ? within_or_unordered : past_limit);
        return fits;
    }
    void run() {
        this->gp[eax] = this->flags.logic(0u, 4);
        Memory<Adapter> memory{*this};
        const auto result = logic::position(memory, *this, this->gp[esi]);
        const std::uint32_t site = result == Exit::negative ? 0x1ce928u : 0x1ce948u;
        this->gp[eax] = this->gp[esi];
        this->at(site);
        this->enter(0x1ce810u, [&](State &v) { dispatch(v, 0x1ce810u, site, true); });
    }
};

// ESI identifies the animation state. The tail dispatch must retain the
// caller's return address and the frame argument in the original stack slot.
template<class State, class Reference, class Dispatch>
bool bridge(State &s, Reference reference, Dispatch dispatch) {
    if (!logic::layout::sse::available) return false;
    static const bool use_reference = native_small_queries_detail::flag("HALO2_REFERENCE_ANIMATION_FRACTION");
    static const bool verify = native_small_queries_detail::flag("HALO2_VERIFY_ANIMATION_FRACTION");
    static VerifySummary summary{"Animation fraction"};
    return run_owner(s, 0x1ce8d0u, 0x1ce948u, use_reference, verify, coverage(), summary, reference,
        [&](State &v, std::uint32_t *c, Trace<State> *t) { return Adapter<State, Dispatch>(v, c, t, dispatch); });
}
} // namespace halo2::native_animation_fraction_detail
