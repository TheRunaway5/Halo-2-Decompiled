#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"
#include "rendering/renderer/native_tu15_quaternion_relative_product_systems.hpp"

namespace halo2::native_quaternion_relative_product_detail {

using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::run_owner;
using native_small_queries_detail::VerifySummary;
namespace logic = native_quaternion_relative_product;
using Access = logic::Access;

enum Branch : unsigned { composed, branch_count };

inline std::atomic<std::uint64_t> &coverage() {
    static std::atomic<std::uint64_t> value{0};
    return value;
}

inline VerifySummary &summary() {
    static VerifySummary value{"Quaternion relative product"};
    return value;
}

inline constexpr std::uint32_t sites[]{
    0x2e56aau, 0x2e56adu, 0x2e56b0u, 0x2e56b3u, 0x2e56dbu,
    0x2e56deu, 0x2e56e1u, 0x2e56e6u, 0x2e56eau, 0x2e56fdu,
    0x2e5700u, 0x2e5703u, 0x2e5706u, 0x2e570au, 0x2e571du,
    0x2e5720u, 0x2e5723u, 0x2e5741u, 0x2e5745u, 0x2e5749u,
};
static_assert(std::size(sites) == unsigned(Access::scalar_result) + 1u);

template<class State, class Spill> struct Adapter : Port<State> {
    Spill &spill;

    Adapter(State &s, std::uint32_t *cursor, Trace<State> *trace, Spill &spill_word)
        : Port<State>(s, cursor, trace, {}), spill(spill_word) {}

    std::uint32_t instruction(Access access, std::uint32_t) const { return sites[unsigned(access)]; }
    void loaded(Access access, std::uint32_t, std::uint32_t value) {
        switch (access) {
        case Access::first_argument: this->gp[eax] = value; break;
        case Access::second_argument:
        case Access::dot_reload: this->gp[edx] = value; break;
        case Access::second_scalar: this->gp[esi] = value; break;
        default: break;
        }
    }
    void loaded_vector(Access, std::uint32_t, __m128) {}
    void stored(Access, std::uint32_t, std::uint32_t) {}

    __m128 xmm(unsigned index, __m128 value) { return this->s.xmm[index] = value; }

    void load_first_scalar(std::uint32_t bits) {
        auto &fp = this->s.fp;
        fp.push(physics_step_detail::ext(std::bit_cast<float>(bits)));
        fp.status &= ~0x200u;
    }

    std::uint32_t negated_scalar() {
        auto &fp = this->s.fp;
        fp.st(0) = -fp.st(0);
        fp.status &= ~0x200u;
        this->at(0x2e56e6u);
        return spill(fp.st(0), fp.status);
    }

    void pop_first_scalar() { this->s.fp.pop(); }
    std::uint32_t scalar_slot() const { return this->gp[esp] + 12u; }
    void publish_scalar_slot() { this->gp[eax] = scalar_slot(); }

    void run() {
        auto &gp = this->gp;
        this->push(0x2e56a0u, gp[ebp]);
        gp[ebp] = gp[esp];
        gp[esp] = this->flags.logic(gp[esp] & 0xfffffff0u, 4);
        gp[esp] = this->flags.minus(gp[esp], 12u, 4);
        this->push(0x2e56a9u, gp[esi]);

        Memory<Adapter> memory{*this};
        logic::compose(memory, *this, gp[ebp] + 8u, gp[ecx]);
        this->reached(composed);

        gp[esi] = this->pop(0x2e574cu);
        gp[esp] = gp[ebp];
        gp[ebp] = this->pop(0x2e574fu);
        this->ret(0x2e5750u, 8u);
        this->publish(0x2e5750u);
    }
};

// ECX points to the output and the two inputs are on the caller's stack.
// The spill function is the scene device unit's existing FSTP32 helper.
template<class State, class Reference, class Spill>
bool bridge(State &s, Reference reference, Spill spill) {
    static const bool use_reference = flag("HALO2_REFERENCE_QUATERNION_RELATIVE_PRODUCT");
    static const bool verify = flag("HALO2_VERIFY_QUATERNION_RELATIVE_PRODUCT");
    return run_owner(s, 0x2e56a0u, 0x2e5750u, use_reference, verify, coverage(), summary(), reference,
        [&](State &state, std::uint32_t *cursor, Trace<State> *trace) {
            return Adapter<State, Spill>(state, cursor, trace, spill);
        });
}

} // namespace halo2::native_quaternion_relative_product_detail
