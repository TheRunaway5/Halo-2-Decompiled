#pragma once

#include "physics/runtime/native_tu15_census_admission.hpp"
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"
#include "physics/runtime/native_tu15_required_owner_policy.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <thread>

namespace halo2::native_light_visibility_detail {
enum Register : unsigned { eax, ecx, edx, ebx, esp, ebp, esi, edi };

struct Noop {
    static constexpr bool silent_hook = true;
    template<class State> void operator()(State&, std::uint32_t) const {}
};

inline bool reference() {
    static const bool active = [] {
        const auto* flag = std::getenv("HALO2_REFERENCE_LIGHT_VISIBILITY");
        return flag && *flag;
    }();
    return active;
}

template<class State>
bool admitted(const State& state, std::uint32_t first, std::uint32_t last) {
    if(reference() || state.image != SourcePhysicsContactImage::TitleUpdate15 || state.active_mmx ||
       !native_required_owner_policy_detail::preserves_instruction_policy(state.services))
        return false;
    if constexpr(requires { state.selected_observation_pcs; }) {
        if(!state.selected_observation_pcs) return false;
    }
    if(native_census_detail::uncursored(state, first, last)) return true;
    if(!state.services.instruction)
        return !state.instruction_cursor && !state.literal_service_sites && !state.observe;
    const auto* cursor = state.instruction_cursor;
    return state.literal_service_sites && cursor && cursor == &state.services.instruction_cursor &&
        cursor->position && cursor->callback_type &&
        state.services.instruction.target_type() == *cursor->callback_type &&
        cursor->executor == std::this_thread::get_id() && cursor->literal_service_sites &&
        cursor->service_sites.size() == std::size(native_census_detail::service_sites) &&
        std::equal(cursor->service_sites.begin(), cursor->service_sites.end(),
            std::begin(native_census_detail::service_sites)) &&
        (!state.observe || cursor->observation_only_stores) &&
        (!cursor->first_trace || !*cursor->first_trace) && (!cursor->second_trace || !*cursor->second_trace);
}

template<class State, class Call, class Before>
class Frame : public native_mopp_ray_caller_detail::Frame<State, Before> {
    using Base = native_mopp_ray_caller_detail::Frame<State, Before>;
    Call& call_;
protected:
    using Base::s_; using Base::at; using Base::word; using Base::push; using Base::pop;
    using Base::store;

    std::uint32_t& reg(Register name) { return s_.gp[name]; }
    std::uint32_t low(Register name) const { return s_.reg(name, 1, 0); }
    void low(Register name, std::uint32_t value) { s_.set(name, value, 1, 0); }

    std::uint32_t load(std::uint32_t pc, std::uint32_t address, unsigned width = 4) {
        at(pc);
        return word(address, width);
    }

    void write(std::uint32_t pc, std::uint32_t address, std::uint32_t value, unsigned width) {
        at(pc);
        physics_world_runtime_detail::w(s_.memory, s_.observe, pc, address, width, value);
    }

    void enter(std::uint32_t pc, std::uint32_t target, std::uint32_t after) {
        push(pc, after);
        call_(s_, target, pc);
    }

    void finish_return(std::uint32_t pc, unsigned argument_bytes) {
        pop(pc);
        reg(esp) += argument_bytes;
    }

public:
    Frame(State& state, Call& call, Before& before) : Base(state, before), call_(call) {}
};
} // namespace halo2::native_light_visibility_detail
