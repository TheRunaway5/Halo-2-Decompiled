#pragma once
#include "campaign/ai/native_campaign_ai_tick_systems.hpp"
#include "physics/runtime/native_tu15_census_admission.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <array>
#include <bit>
#include <cstdlib>

namespace halo2::native_campaign_ai_tick {

template<class State>
using Subsystems = std::array<void (*)(State &), 5>;

// The dispatcher owns the original call order and stack. Subsystems keep
// their own state and use the same live world and retained caller frame.
template<class State, class Cleanup>
class Frame {
    enum Register : unsigned { eax, ecx, edx };
    static constexpr std::uint32_t ai_state = 0x5020a8u;
    static constexpr std::uint32_t game_options = 0x4f3218u;
    static constexpr std::uint32_t update_enabled = 1;
    static constexpr std::uint32_t last_tick_active = 2;
    static constexpr std::uint32_t activity_counter = 0x22;

    State &state_;
    void (*prepare_)(State &);
    const Subsystems<State> &subsystems_;
    Cleanup &cleanup_;

    void at(std::uint32_t pc) {
        if (!native_census_detail::skip_report(state_.services, pc)) state_.instruction_literal(pc);
    }
    std::uint32_t load(std::uint32_t pc, std::uint32_t address, unsigned width = 4) {
        at(pc);
        return physics_world_runtime_detail::r(state_.memory, address, width);
    }
    void store(std::uint32_t pc, std::uint32_t address, unsigned width, std::uint32_t value) {
        at(pc);
        physics_world_runtime_detail::w(state_.memory, state_.observe, pc, address, width, value);
    }
    void child(std::uint32_t pc, void (*call)(State &)) {
        at(pc);
        state_.push(pc, pc + 5u);
        call(state_);
    }

public:
    Frame(State &state, void (*prepare)(State &), const Subsystems<State> &subsystems, Cleanup &cleanup)
        : state_(state), prepare_(prepare), subsystems_(subsystems), cleanup_(cleanup) {}

    std::uint32_t state() {
        return state_.gp[eax] = load(0x1d0670u, ai_state);
    }
    bool enabled(std::uint32_t ai) {
        const auto enabled = load(0x1d0675u, ai + update_enabled, 1);
        state_.set(ecx, enabled, 1, 0);
        at(0x1d0678u);
        state_.flags.logic(enabled, 1);
        at(0x1d067au);
        return enabled != 0;
    }
    bool game_mode_excludes_ai() {
        const auto options = load(0x1d067cu, game_options);
        state_.gp[ecx] = options;
        const auto mode = load(0x1d0682u, options + 0xcu, 1);
        state_.flags.minus(mode, 4, 1);
        at(0x1d0686u);
        return mode == 4;
    }
    bool update_requested(std::uint32_t ai) {
        const auto requested = load(0x1d0688u, ai, 1);
        state_.flags.minus(requested, 0, 1);
        at(0x1d068bu);
        return requested != 0;
    }
    bool was_active(std::uint32_t ai) {
        const auto active = load(0x1d06ccu, ai + last_tick_active, 1);
        state_.set(ecx, active, 1, 0);
        at(0x1d06cfu);
        state_.flags.logic(active, 1);
        at(0x1d06d1u);
        return active != 0;
    }
    void retire_active_state() {
        at(0x1d06d3u);
        state_.push(0x1d06d3u, 0x1d06d8u);
        cleanup_(state_);
    }
    void clear_active() {
        const auto ai = load(0x1d06d8u, ai_state);
        state_.gp[eax] = ai;
        store(0x1d06ddu, ai + last_tick_active, 1, 0);
    }
    void finish_idle() {
        at(0x1d06e1u);
        state_.pop();
    }
    void prepare_update() { child(0x1d068du, prepare_); }
    std::uint32_t counter_owner() {
        return state_.gp[ecx] = load(0x1d0692u, ai_state);
    }
    std::int16_t update_counter(std::uint32_t ai) {
        at(0x1d0698u);
        state_.gp[eax] = state_.flags.logic(0, 4);
        const auto count = std::uint16_t(load(0x1d069au, ai + activity_counter, 2));
        state_.set(eax, count, 2, 0);
        at(0x1d069eu);
        state_.flags.logic(count, 2);
        at(0x1d06a1u);
        return std::bit_cast<std::int16_t>(count);
    }
    void decrement_counter(std::uint32_t ai, std::int16_t count) {
        at(0x1d06a3u);
        state_.gp[eax] = state_.flags.increment(std::uint32_t(count), 4, true);
        store(0x1d06a4u, ai + activity_counter, 2, state_.reg(eax, 2));
    }
    void update_subsystems() {
        child(0x1d06a8u, subsystems_[0]);
        child(0x1d06adu, subsystems_[1]);
        child(0x1d06b2u, subsystems_[2]);
        child(0x1d06b7u, subsystems_[3]);
        child(0x1d06bcu, subsystems_[4]);
    }
    void mark_active() {
        const auto ai = load(0x1d06c1u, ai_state);
        state_.gp[edx] = ai;
        store(0x1d06c7u, ai + last_tick_active, 1, 1);
    }
    void finish_update() {
        at(0x1d06cbu);
        state_.pop();
    }
};

inline bool reference_requested() {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_CAMPAIGN_AI_TICK");
        return value && *value;
    }();
    return reference;
}

template<class State, class Cleanup>
bool bridge(State &state, void (*prepare)(State &), const Subsystems<State> &subsystems, Cleanup cleanup) {
    if (state.image != SourcePhysicsContactImage::TitleUpdate15 || reference_requested()) return false;
    Frame frame(state, prepare, subsystems, cleanup);
    update(frame);
    return true;
}

} // namespace halo2::native_campaign_ai_tick
