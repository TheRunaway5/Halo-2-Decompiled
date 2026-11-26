#pragma once

#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_particle_cleanup_251650_detail {

using namespace native_object_tick_detail::boundary;

inline constexpr std::uint32_t entry = 0x251650u, exit = 0x2516afu;
inline constexpr std::uint32_t none = 0xffffffffu;

template<class State, class Commit>
void write_and_commit(State &state, std::uint32_t pc, std::uint32_t address, unsigned width,
                      std::uint32_t value, Commit commit) {
    if (state.observe) {
        const SourcePhysicsContactRecordObserver observer = [&](const SourcePhysicsContactRecordStore &record) {
            commit();
            state.observe(record);
        };
        physics_step_detail::w(state.memory, observer, pc, address, width, value);
    } else {
        physics_step_detail::w(state.memory, {}, pc, address, width, value);
        commit();
    }
}

template<class State>
void push(State &state, std::uint32_t pc, std::uint32_t value) {
    const auto address = state.gp[esp] - 4u;
    write_and_commit(state, pc, address, 4, value, [&] { state.gp[esp] = address; });
}

template<class State>
void decrement_particle_count(State &state) {
    const auto address = state.gp[ebp] + 2u;
    const auto old_count = physics_step_detail::r(state.memory, address, 2);
    auto committed_flags = state.flags;
    const auto count = committed_flags.increment(old_count, 2, true);
    // The memory form of DEC commits its flags only after the write succeeds.
    write_and_commit(state, 0x2516a9u, address, 2, count, [&] { state.flags = committed_flags; });
}

template<class State, bool Observed>
class Frame {
    State &state_;
    const typename State::InstructionCursor *policy_;
    bool observing_{};

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
            // A provider or a genuine child may activate tracing in this frame.
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
        const auto value = physics_step_detail::r(state_.memory, state_.gp[base] + displacement, 4);
        after_access();
        return value;
    }

    void save(std::uint32_t site, Register source) {
        access(site);
        push(state_, site, state_.gp[source]);
        after_access();
    }

    std::uint32_t pop(std::uint32_t site) {
        const auto value = load(site, esp);
        state_.gp[esp] += 4u;
        return value;
    }

    template<class Child>
    void call(std::uint32_t site, std::uint32_t next, Child child) {
        access(site);
        push(state_, site, next);
        child(state_);
        after_access();
    }

    template<class Dispatch>
    void call_slot(std::uint32_t site, std::uint32_t next, Register base, unsigned displacement, Dispatch &dispatch) {
        access(site);
        const auto target = physics_step_detail::r(state_.memory, state_.gp[base] + displacement, 4);
        push(state_, site, next);
        dispatch(state_, target, site);
        after_access();
    }

    template<class Lookup, class Dispatch>
    void remove_attachment(Lookup &lookup, Dispatch &dispatch) {
        auto &gp = state_.gp;
        pc(0x251676u); gp[ecx] = gp[edi];
        call(0x251678u, 0x25167du, lookup);
        gp[edx] = load(0x25167du, esi);
        gp[edi] = load(0x25167fu, eax, 4u);
        pc(0x251682u); gp[ecx] = gp[esi];
        call_slot(0x251684u, 0x251687u, edx, 12u, dispatch);
        gp[edx] = load(0x251687u, esi);
        pc(0x251689u); gp[ecx] = gp[esi];
        access(0x25168bu);
        physics_step_detail::w(state_.memory, state_.observe, 0x25168bu, gp[esp] + 16u, 4, gp[eax]);
        after_access();
        call_slot(0x25168fu, 0x251692u, edx, 8u, dispatch);
        save(0x251692u, edi);
        pc(0x251693u); gp[edx] = gp[eax];
        gp[eax] = load(0x251695u, esp, 20u);
        save(0x251699u, ebx);
        pc(0x25169au); gp[ecx] = state_.flags.logic(0u, 4);
        call(0x25169cu, 0x2516a1u, [&](auto &state) { dispatch(state, 0x250170u, 0x25169cu); });
    }

public:
    Frame(State &state, const typename State::InstructionCursor *policy)
        : state_(state), policy_(policy) {}

    template<class Lookup, class Resolve, class Dispatch>
    void run(Lookup &lookup, Resolve &resolve, Dispatch &dispatch) {
        auto &gp = state_.gp;
        save(entry, ebp);
        gp[ebp] = load(0x251651u, esp, 8u);
        save(0x251655u, esi);
        save(0x251656u, edi);
        pc(0x251657u); gp[edi] = gp[eax];
        pc(0x251659u); gp[ecx] = gp[edi];
        call(0x25165bu, 0x251660u, lookup);
        gp[edx] = load(0x251660u, eax, 4u);
        call(0x251663u, 0x251668u, resolve);
        pc(0x251668u); gp[esi] = gp[eax];
        gp[eax] = load(0x25166au, esi);
        pc(0x25166cu); gp[ecx] = gp[esi];
        call_slot(0x25166eu, 0x251671u, eax, 12u, dispatch);
        pc(0x251671u); state_.flags.minus(gp[eax], none, 4);
        pc(0x251674u);
        if (!state_.flags.zf)
            remove_attachment(lookup, dispatch);

        gp[edi] = pop(0x2516a1u);
        access(0x2516a2u);
        physics_step_detail::w(state_.memory, state_.observe, 0x2516a2u, gp[ebx] + 4u, 4, none);
        after_access();
        access(0x2516a9u);
        decrement_particle_count(state_);
        after_access();
        gp[esi] = pop(0x2516adu);
        gp[ebp] = pop(0x2516aeu);
        (void)pop(exit);
        gp[esp] += 4u;
    }
};

template<class State, class Lookup, class Resolve, class Dispatch>
bool bridge(State &state, Lookup lookup, Resolve resolve, Dispatch dispatch) {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_PARTICLE_CLEANUP_251650");
        return value && *value;
    }();
    if (reference || state.image != SourcePhysicsContactImage::TitleUpdate15 || state.active_mmx)
        return false;
    const auto *policy = state.immutable_cursor.cursor;
    std::uint32_t *position;
    if ((!state.services.instruction || policy) &&
        native_object_tick_detail::admitted(state, entry, exit, position, false))
        Frame<State, false>{state, position ? policy : nullptr}.run(lookup, resolve, dispatch);
    else
        Frame<State, true>{state, nullptr}.run(lookup, resolve, dispatch);
    return true;
}

} // namespace halo2::native_particle_cleanup_251650_detail
