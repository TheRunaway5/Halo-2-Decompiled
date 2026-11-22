#pragma once

#include "physics/runtime/native_tu15_census_admission.hpp"
#include "physics/runtime/physics_step_internal.hpp"

#include <span>

namespace halo2::native_phantom_island_detail {

inline constexpr std::uint32_t body_island = 0x44u;
inline constexpr std::uint32_t island_active = 0x29u;
inline constexpr std::uint32_t component_entries = 0x70u;
inline constexpr std::uint32_t component_count = 0x74u;
inline constexpr std::uint32_t entry_body = 0x40u;
inline constexpr std::uint32_t entry_stride = 0x60u;

inline bool reference_enabled() {
    static const bool enabled = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_PHANTOM_ISLAND");
        return value && *value;
    }();
    return enabled;
}

// Store observers use the unit's PC namespace. A composed caller's live
// instruction cursor uses TU1.5 PCs, just as its synchronous remap does.
struct InstructionSites {
    std::uint32_t canonical_delta{};
    std::uint32_t *cursor{};

    void mark(std::uint32_t actual) const {
        if (cursor) *cursor = actual;
    }

    std::uint32_t local(std::uint32_t actual) const { return actual - canonical_delta; }
};

template<class State>
bool admit(const State &state, std::span<const std::uint32_t> sites, InstructionSites &binding) {
    if (reference_enabled() || state.image != SourcePhysicsContactImage::TitleUpdate15) return false;
    if (!state.services.instruction) return !state.observe;

    const auto *policy = state.literal_service_sites ? state.live_instruction_cursor() : nullptr;
    if (const auto *remap = state.services.instruction.template target<PhysicsInstructionRemapTu15>()) {
        for (const auto site : sites) {
            const auto *mapped = remap->find(binding.local(site));
            if (!mapped || mapped->value != site) return false;
        }
        policy = remap->live_cursor();
        if (!policy)
            return !state.observe && remap->source && !remap->source->instruction_cursor.position &&
                native_census_detail::enabled() && !native_census_detail::count_native_reports();
    } else if (!policy) {
        return !binding.canonical_delta && !state.observe && !native_census_detail::count_native_reports() &&
            native_census_detail::uncursored(state, sites.front(), sites.back());
    }

    if ((policy->first_trace && *policy->first_trace) || (policy->second_trace && *policy->second_trace) ||
        (state.observe && !policy->observation_only_stores)) return false;
    if (binding.canonical_delta && !state.services.instruction.template target<PhysicsInstructionRemapTu15>())
        return false;
    binding.cursor = policy->position;
    return true;
}

// 315C90 receives a body in ECX and writes the byte to the caller's out
// pointer. Only the present-island branch changes CL. AL is the activity
// result in the surrounding component query, not in this leaf.
template<class State>
bool query_body(State &state, std::uint32_t canonical_delta = 0) {
    using namespace physics_step_detail;
    static constexpr std::uint32_t sites[] = {
        0x315c90u, 0x315c93u, 0x315c95u, 0x315c97u, 0x315c9bu,
        0x315c9eu, 0x315ca1u, 0x315ca4u, 0x315ca8u, 0x315caau};
    InstructionSites binding{canonical_delta};
    if (!admit(state, sites, binding)) return false;

    auto &registers = state.gp;
    binding.mark(0x315c90u);
    const auto island = registers[0] = r(state.memory, registers[1] + body_island);
    state.flags.logic(island, 4);
    std::uint32_t active = 0;
    if (island) {
        binding.mark(0x315ca1u);
        active = r(state.memory, island + island_active, 1);
        state.set(1, active, 1);
    }
    binding.mark(island ? 0x315ca4u : 0x315c97u);
    const auto output = registers[0] = r(state.memory, registers[4] + 4u);
    const auto store = island ? 0x315ca8u : 0x315c9bu;
    binding.mark(store);
    w(state.memory, state.observe, binding.local(store), output, 1, active);
    binding.mark(island ? 0x315caau : 0x315c9eu);
    state.pop();
    registers[4] += 4u;
    return true;
}

template<class State>
bool query_component(State &state) {
    using namespace physics_step_detail;
    static constexpr std::uint32_t sites[] = {
        0x1d9ec0u, 0x1d9ec1u, 0x1d9ec4u, 0x1d9ec5u, 0x1d9ec6u, 0x1d9ec8u,
        0x1d9ecau, 0x1d9eccu, 0x1d9eceu, 0x1d9ecfu, 0x1d9ed1u, 0x1d9ed4u,
        0x1d9ed8u, 0x1d9edcu, 0x1d9eddu, 0x1d9ee2u, 0x1d9ee5u, 0x1d9ee7u,
        0x1d9eeau, 0x1d9eebu, 0x1d9eeeu, 0x1d9ef0u, 0x1d9ef2u, 0x1d9ef3u,
        0x1d9ef4u, 0x1d9ef6u, 0x1d9ef7u, 0x1d9ef8u, 0x1d9ef9u, 0x1d9efau,
        0x1d9efbu, 0x1d9efdu, 0x1d9efeu, 0x1d9effu, 0x1d9f00u, 0x1d9f01u,
        0x1d9f03u, 0x1d9f04u, 0x1d9f05u};
    InstructionSites binding;
    if (!admit(state, sites, binding)) return false;

    auto &registers = state.gp;
    const auto push = [&](std::uint32_t site, unsigned index) {
        binding.mark(site);
        state.push(site, registers[index]);
    };
    const auto pop = [&](std::uint32_t site, unsigned index) {
        binding.mark(site);
        registers[index] = state.pop();
    };
    push(0x1d9ec0u, 1);
    binding.mark(0x1d9ec1u);
    auto count = registers[0] = r(state.memory, registers[6] + component_count);
    push(0x1d9ec4u, 3);
    push(0x1d9ec5u, 5);
    state.set(3, state.flags.logic(0, 1), 1);
    registers[5] = state.flags.logic(0, 4);
    state.flags.logic(count, 4);
    const bool has_entries = si(count) > 0;
    bool active = false;
    if (has_entries) {
        push(0x1d9eceu, 7);
        registers[7] = state.flags.logic(0, 4);
        do {
            binding.mark(0x1d9ed1u);
            const auto entries = registers[0] = r(state.memory, registers[6] + component_entries);
            binding.mark(0x1d9ed4u);
            registers[1] = r(state.memory, entries + registers[7] + entry_body);
            registers[2] = registers[4] + 15u;
            push(0x1d9edcu, 2);
            binding.mark(0x1d9eddu);
            state.push(0x1d9eddu, 0x1d9ee2u);
            state.required(0x1d9eddu, 0x315c90u, false);
            binding.mark(0x1d9ee2u);
            const auto activity = r(state.memory, registers[0], 1);
            state.flags.minus(activity, 0, 1);
            active = activity != 0;
            if (active) break;

            binding.mark(0x1d9ee7u);
            count = registers[0] = r(state.memory, registers[6] + component_count);
            registers[5] = state.flags.increment(registers[5], 4, false);
            registers[7] = state.flags.plus(registers[7], entry_stride, 4);
            state.flags.minus(registers[5], count, 4);
        } while (si(registers[5]) < si(count));
        pop(active ? 0x1d9ef9u : 0x1d9ef2u, 7);
    }

    struct RestoreSites { std::uint32_t ebp, ebx, ecx, ret; };
    static constexpr RestoreSites empty{0x1d9f00u, 0x1d9f03u, 0x1d9f04u, 0x1d9f05u};
    static constexpr RestoreSites inactive{0x1d9ef3u, 0x1d9ef6u, 0x1d9ef7u, 0x1d9ef8u};
    static constexpr RestoreSites found{0x1d9efau, 0x1d9efdu, 0x1d9efeu, 0x1d9effu};
    const auto &exit = !has_entries ? empty : (active ? found : inactive);
    pop(exit.ebp, 5);
    state.set(0, active ? 1u : state.reg(3, 1), 1);
    pop(exit.ebx, 3);
    pop(exit.ecx, 1);
    binding.mark(exit.ret);
    state.pop();
    return true;
}

} // namespace halo2::native_phantom_island_detail
