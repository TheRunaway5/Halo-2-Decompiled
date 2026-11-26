#pragma once

#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_havok_plane_314a30_detail {

using namespace native_object_tick_detail::boundary;
using native_tu15_leaves::ordered_mul_ps;
using native_tu15_leaves::ordered_mul_ss;
using native_world_leaves_detail::ordered_add_ss;
using native_world_leaves_detail::ordered_sub_ps;

inline constexpr std::uint32_t entry = 0x314a30u, last = 0x314b1au;

inline __m128 load_aligned(const physics_step_detail::M &memory, std::uint32_t address) {
    if (address & 15u)
        throw std::runtime_error("Original Havok plane MOVAPS load requires aligned data");
    return physics_step_detail::v(memory, address);
}

inline void store_aligned(physics_step_detail::M &memory, const physics_step_detail::O &observer,
                          std::uint32_t site, std::uint32_t address, __m128 value) {
    if (address & 15u)
        throw std::runtime_error("Original Havok plane MOVAPS store requires aligned data");
    physics_step_detail::vw(memory, observer, site, address, value);
}

template<class State>
void push_frame(State &state) {
    const auto address = state.gp[esp] - 4u;
    const auto saved = state.gp[ebp];
    if (state.observe) {
        const SourcePhysicsContactRecordObserver observer = [&](const SourcePhysicsContactRecordStore &record) {
            state.gp[esp] = address;
            state.observe(record);
        };
        physics_step_detail::w(state.memory, observer, entry, address, 4u, saved);
    } else {
        physics_step_detail::w(state.memory, {}, entry, address, 4u, saved);
        state.gp[esp] = address;
    }
}

template<class State, class Estimate>
void estimate_length(State &state, Estimate &estimate) {
    const auto previous = state.xmm[2];
    const auto argument = native_tu15_leaves::low_bits(state.xmm[0]);
    const auto bits = estimate(state, argument);
    const auto low = _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(bits)));
    state.xmm[2] = _mm_move_ss(previous, low);
}

template<class State, bool Observed>
class Frame {
    State &state_;
    const typename State::InstructionCursor *policy_;
    bool observing_{};
    decltype(State::gp) &gp_;
    decltype(State::xmm) &xmm_;

    void pc(std::uint32_t site) {
        if constexpr (Observed)
            state_.instruction_literal(site);
        else if (observing_)
            state_.instruction_literal(site);
    }

    void access(std::uint32_t site) {
        pc(site);
        if (policy_)
            *policy_->position = site;
    }

    void after_access() {
        if constexpr (!Observed) {
            if (policy_) {
                if ((policy_->first_trace && *policy_->first_trace) ||
                    (policy_->second_trace && *policy_->second_trace)) {
                    policy_ = nullptr;
                    observing_ = true;
                }
            } else if (state_.services.instruction) {
                observing_ = true;
            }
        }
    }

    std::uint32_t load(std::uint32_t site, Register base, unsigned displacement = 0) {
        access(site);
        const auto value = physics_step_detail::r(state_.memory, gp_[base] + displacement, 4);
        after_access();
        return value;
    }

    __m128 vector(std::uint32_t site, Register base, unsigned displacement = 0) {
        access(site);
        const auto value = load_aligned(state_.memory, gp_[base] + displacement);
        after_access();
        return value;
    }

    void store_vector(std::uint32_t site, Register base, unsigned displacement, unsigned lane) {
        access(site);
        store_aligned(state_.memory, state_.observe, site, gp_[base] + displacement, xmm_[lane]);
        after_access();
    }

    std::uint32_t pop(std::uint32_t site) {
        const auto value = load(site, esp);
        gp_[esp] += 4u;
        return value;
    }

    void cross_product() {
        gp_[eax] = load(0x314a39u, ecx, 0x24u);
        xmm_[1] = vector(0x314a3cu, eax);
        gp_[eax] = load(0x314a3fu, ecx, 0x14u);
        xmm_[0] = vector(0x314a42u, eax);
        gp_[edx] = load(0x314a45u, ecx, 0x34u);
        xmm_[2] = vector(0x314a48u, edx);
        pc(0x314a4bu); xmm_[0] = ordered_sub_ps(xmm_[0], xmm_[1]);
        pc(0x314a4eu); xmm_[1] = ordered_sub_ps(xmm_[1], xmm_[2]);
        pc(0x314a51u); xmm_[3] = xmm_[0];
        pc(0x314a54u); xmm_[3] = _mm_shuffle_ps(xmm_[3], xmm_[0], 210);
        pc(0x314a58u); xmm_[2] = xmm_[1];
        pc(0x314a5bu); xmm_[2] = _mm_shuffle_ps(xmm_[2], xmm_[1], 201);
        pc(0x314a5fu); xmm_[3] = ordered_mul_ps(xmm_[3], xmm_[2]);
        pc(0x314a62u); xmm_[2] = xmm_[1];
        pc(0x314a65u); xmm_[2] = _mm_shuffle_ps(xmm_[2], xmm_[1], 210);
        pc(0x314a69u); xmm_[1] = xmm_[0];
        pc(0x314a6cu); xmm_[1] = _mm_shuffle_ps(xmm_[1], xmm_[0], 201);
        pc(0x314a70u); xmm_[1] = ordered_mul_ps(xmm_[1], xmm_[2]);
        pc(0x314a73u); xmm_[1] = ordered_sub_ps(xmm_[1], xmm_[3]);
    }

    template<class Estimate>
    void normalize(Estimate &estimate) {
        using namespace native_tu15_leaves;
        pc(0x314a76u); xmm_[0] = xmm_[1];
        pc(0x314a79u); xmm_[0] = ordered_mul_ps(xmm_[0], xmm_[1]);
        pc(0x314a7cu); xmm_[2] = xmm_[0];
        pc(0x314a7fu); xmm_[2] = _mm_shuffle_ps(xmm_[2], xmm_[0], 85);
        pc(0x314a83u); xmm_[2] = ordered_add_ss(xmm_[2], xmm_[0]);
        pc(0x314a87u); xmm_[3] = xmm_[0];
        pc(0x314a8au); xmm_[3] = _mm_shuffle_ps(xmm_[3], xmm_[0], 170);
        pc(0x314a8eu); xmm_[0] = xmm_[3];
        pc(0x314a91u); xmm_[0] = ordered_add_ss(xmm_[0], xmm_[2]);
        store_vector(0x314a95u, esp, 0x10u, 0);

        access(0x314a9au);
        estimate_length(state_, estimate);
        after_access();
        access(0x314a9eu);
        physics_step_detail::w(state_.memory, state_.observe, 0x314a9eu, gp_[esp] + 0x10u, 4, low_bits(xmm_[2]));
        after_access();
        xmm_[2] = vector(0x314aa4u, esp, 0x10u);

        pc(0x314aa9u); xmm_[0] = ordered_mul_ss(xmm_[0], xmm_[2]);
        pc(0x314aadu); xmm_[0] = ordered_mul_ss(xmm_[0], xmm_[2]);
        access(0x314ab1u);
        physics_step_detail::w(state_.memory, state_.observe, 0x314ab1u, gp_[esp] + 0xcu, 4, 0x40400000u);
        after_access();
        xmm_[3] = _mm_set_ss(as_float(load(0x314ab9u, esp, 0xcu)));
        pc(0x314abfu); xmm_[3] = _mm_sub_ss(xmm_[3], xmm_[0]);
        access(0x314ac3u);
        physics_step_detail::w(state_.memory, state_.observe, 0x314ac3u, gp_[esp] + 0xcu, 4, 0x3f000000u);
        after_access();
        xmm_[0] = _mm_set_ss(as_float(load(0x314acbu, esp, 0xcu)));
        pc(0x314ad1u); xmm_[0] = ordered_mul_ss(xmm_[0], xmm_[2]);
        pc(0x314ad5u); xmm_[0] = ordered_mul_ss(xmm_[0], xmm_[3]);
        pc(0x314ad9u); xmm_[2] = xmm_[0];
        pc(0x314adcu); xmm_[2] = _mm_shuffle_ps(xmm_[2], xmm_[0], 0);
    }

    void publish_plane() {
        store_vector(0x314ae0u, ecx, 0, 1);
        pc(0x314ae3u); xmm_[0] = xmm_[2];
        pc(0x314ae6u); xmm_[0] = ordered_mul_ps(xmm_[0], xmm_[1]);
        store_vector(0x314ae9u, ecx, 0, 0);
        xmm_[1] = vector(0x314aecu, eax);
        pc(0x314aefu); xmm_[0] = ordered_mul_ps(xmm_[0], xmm_[1]);
        pc(0x314af2u); xmm_[1] = xmm_[0];
        pc(0x314af5u); xmm_[1] = _mm_shuffle_ps(xmm_[1], xmm_[0], 85);
        pc(0x314af9u); xmm_[2] = xmm_[0];
        pc(0x314afcu); xmm_[1] = ordered_add_ss(xmm_[1], xmm_[0]);
        pc(0x314b00u); xmm_[2] = _mm_shuffle_ps(xmm_[2], xmm_[0], 170);
        pc(0x314b04u); gp_[eax] = gp_[esp] + 0xcu;
        pc(0x314b08u); xmm_[2] = ordered_add_ss(xmm_[2], xmm_[1]);
        access(0x314b0cu);
        physics_step_detail::w(state_.memory, state_.observe, 0x314b0cu, gp_[eax], 4, native_tu15_leaves::low_bits(xmm_[2]));
        after_access();
        gp_[edx] = load(0x314b10u, esp, 0xcu);
        access(0x314b14u);
        physics_step_detail::w(state_.memory, state_.observe, 0x314b14u, gp_[ecx] + 0x10u, 4, gp_[edx]);
        after_access();
    }

public:
    Frame(State &state, const typename State::InstructionCursor *policy)
        : state_(state), policy_(policy), gp_(state.gp), xmm_(state.xmm) {}

    template<class Estimate>
    void run(Estimate &estimate) {
        access(entry);
        push_frame(state_);
        after_access();
        pc(0x314a31u); gp_[ebp] = gp_[esp];
        pc(0x314a33u); gp_[esp] = state_.flags.logic(gp_[esp] & 0xfffffff0u, 4);
        pc(0x314a36u); gp_[esp] = state_.flags.minus(gp_[esp], 0x20u, 4);
        cross_product();
        normalize(estimate);
        publish_plane();
        pc(0x314b17u); gp_[esp] = gp_[ebp];
        gp_[ebp] = pop(0x314b19u);
        (void)pop(last);
    }
};

template<class State, class Estimate>
bool bridge(State &state, Estimate estimate) {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_HAVOK_PLANE_314A30");
        return value && *value;
    }();
    if (reference || state.image != SourcePhysicsContactImage::TitleUpdate15 || state.active_mmx)
        return false;
    const auto *policy = state.immutable_cursor.cursor;
    std::uint32_t *position;
    if ((!state.services.instruction || policy) &&
        native_object_tick_detail::admitted(state, entry, last, position, false))
        Frame<State, false>{state, position ? policy : nullptr}.run(estimate);
    else
        Frame<State, true>{state, nullptr}.run(estimate);
    return true;
}

} // namespace halo2::native_havok_plane_314a30_detail
