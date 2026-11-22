#pragma once

#include "physics/runtime/native_tu15_census_admission.hpp"
#include "physics/runtime/native_tu15_required_owner_policy.hpp"
#include "physics/runtime/native_tu15_simplex_owner_retained_frame.hpp"

#include <cstdint>
#include <cstdlib>
#include <thread>

namespace halo2::native_simplex_source_owner_detail {

struct NoInstructionHook {
    static constexpr bool silent_hook = true;
    template<class State>
    void operator()(State&, std::uint32_t) const noexcept {}
};

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
template<class State>
bool admits(State const& state, std::uint32_t first, std::uint32_t last) {
    if (state.image != SourcePhysicsContactImage::TitleUpdate15 || !state.selected_observation_pcs ||
        state.fp.count > 4u || !native_raw_x87_detail::supported(state) ||
        !native_required_owner_policy_detail::preserves_instruction_policy(state.services))
        return false;

    const auto* cursor = state.instruction_cursor;
    if (state.services.instruction) {
        if (cursor) {
            if (!state.literal_service_sites || cursor != &state.services.instruction_cursor ||
                !cursor->position || !cursor->callback_type || !cursor->literal_service_sites ||
                state.services.instruction.target_type() != *cursor->callback_type ||
                cursor->executor != std::this_thread::get_id() ||
                (cursor->first_trace && *cursor->first_trace) ||
                (cursor->second_trace && *cursor->second_trace))
                return false;
            for (const auto site : cursor->service_sites)
                if (site >= first && site <= last) return false;
        } else if (!native_census_detail::uncursored(state, first, last)) {
            return false;
        }
    } else if (cursor) {
        return false;
    }
    return !state.observe || (cursor && cursor->observation_only_stores);
}
#endif

template<class State, class Call>
bool bridge(State& state, Call child) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    static const bool reference = [] {
        const auto* value = std::getenv("HALO2_REFERENCE_SIMPLEX_OWNER");
        return value && *value;
    }();
    if (reference || !state.services.reciprocal_sqrt || !admits(state, 0x30e930u, 0x30f6ddu)) return false;
    native_simplex_owner_retained_detail::run_private_simplex_retained(state, child, NoInstructionHook{});
    return true;
#else
    return false;
#endif
}

template<class State, class Call>
bool bridge_hull(State& state, Call child) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    static const bool reference = [] {
        const auto* value = std::getenv("HALO2_REFERENCE_SIMPLEX_HULL_OWNER");
        return value && *value;
    }();
    if (reference || !admits(state, 0x30de60u, 0x30e8d8u)) return false;
    native_simplex_hull_retained_detail::run_private_hull_retained(state, child, NoInstructionHook{});
    return true;
#else
    return false;
#endif
}

} // namespace halo2::native_simplex_source_owner_detail
