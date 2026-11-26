#pragma once

#include "core/native_tu15_authored_control_point_systems.hpp"
#include "physics/runtime/native_tu15_census_admission.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_authored_control_point {
inline constexpr std::uint32_t entry = 0x13e9a0u;
inline constexpr std::uint32_t last = 0x13e9d7u;
inline constexpr bool owns_entry(std::uint32_t target) { return target == entry; }

enum Register : unsigned { eax = 0, ecx = 1, edx = 2, esp = 4 };

template<class State>
struct Frame {
    State &state;
    bool count_only_translated;

    template<std::uint32_t pc>
    void before() const {
        if (!count_only_translated) state.instruction_literal(pc);
    }

    void load_blob() {
        before<0x13e9a0u>();
        state.gp[eax] = physics_step_detail::r(state.memory, state.gp[eax] + 4u, 4);
    }
    bool supports_control_points() {
        auto &registers = state.gp;
        auto &flags = state.flags;
        before<0x13e9a3u>();
        registers[ecx] = (registers[ecx] & 0xffffff00u) | physics_step_detail::r(state.memory, registers[eax], 1);
        before<0x13e9a5u>();
        flags.minus(registers[ecx] & 0xffu, systems::first_control_point_kind, 1);
        before<0x13e9a8u>();
        if (flags.cf) return false;
        before<0x13e9aau>();
        flags.minus(registers[ecx] & 0xffu, systems::last_contiguous_kind, 1);
        before<0x13e9adu>();
        if (flags.cf || flags.zf) return true;
        before<0x13e9afu>();
        flags.minus(registers[ecx] & 0xffu, systems::powered_cubic_kind, 1);
        before<0x13e9b2u>();
        return flags.zf;
    }
    void load_layer_stride() {
        auto &registers = state.gp;
        before<0x13e9b4u>();
        registers[ecx] = physics_step_detail::r(state.memory, registers[eax], 1);
        before<0x13e9b7u>();
        registers[edx] = physics_step_detail::r(state.memory, systems::stride_table + registers[ecx], 1);
    }
    void select_layer() {
        auto &registers = state.gp;
        before<0x13e9beu>();
        registers[ecx] = physics_step_detail::r(state.memory, registers[esp] + 4u, 1);
        before<0x13e9c3u>();
        // Preserve the engine's existing IMUL convention for the undefined
        // status bits. The two zero-extended bytes never overflow a dword.
        registers[edx] = state.flags.product(registers[edx], registers[ecx]);
    }
    void select_point() {
        auto &registers = state.gp;
        before<0x13e9c6u>();
        registers[ecx] = physics_step_detail::r(state.memory, registers[esp] + 8u, 1);
        before<0x13e9cbu>();
        registers[edx] += systems::words_per_point * registers[ecx];
    }
    void finish_with_point() {
        auto &registers = state.gp;
        before<0x13e9ceu>();
        registers[eax] += systems::first_point_offset + 4u * registers[edx];
        before<0x13e9d2u>();
        state.pop();
        registers[esp] += 8u;
    }
    void finish_without_point() {
        before<0x13e9d5u>();
        state.gp[eax] = state.flags.logic(0u, 4);
        before<0x13e9d7u>();
        state.pop();
        state.gp[esp] += 8u;
    }
};

template<class State>
void run(State &state) {
    const bool count_only_translated = native_census_detail::uncursored(state, entry, last) &&
        !native_census_detail::count_native_reports();
    Frame<State> frame{state, count_only_translated};
    systems::resolve(frame);
}

// The TU1.5 required-owner entry transports the live ABI, including partial
// register progress when a callback or guest read throws.
inline void run_entry(SourcePhysicsContactMemory &memory, std::uint32_t target,
    SourcePhysicsContactRuntimeAbi &abi, SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsContactRuntimeServices &services, const SourcePhysicsContactRecordObserver &observe = {}) {
    if (!owns_entry(target)) throw std::runtime_error("Required original contact entry unavailable");
    physics_step_detail::FpScope scope(abi.floating_point.value_or(fp));
    physics_world_runtime_detail::WorldState state{memory, observe, services, SourcePhysicsContactImage::TitleUpdate15};
    state.load(abi);
    try { run(state); }
    catch (...) {
        abi = state.save();
        throw;
    }
    abi = state.save();
}

} // namespace halo2::native_authored_control_point
