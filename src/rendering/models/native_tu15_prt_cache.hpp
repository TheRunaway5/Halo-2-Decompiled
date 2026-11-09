#pragma once
#include "rendering/models/native_tu15_prt_helpers.hpp"

namespace halo2::prt_cache_detail {
using prt_helpers_detail::read;
struct Section {
    std::uint32_t row, definition, array, selector;
};
inline Section section(const SourcePhysicsContactMemory& memory, std::uint32_t tag,
    std::uint32_t selector_table, std::uint32_t selector_index) {
    const auto tags = read(memory, 0x4f0414u);
    const auto definition = read(memory, tags + (tag & 65535u) * 16u + 8u);
    const auto selector = read(memory, selector_table + selector_index, 1);
    const auto array = read(memory, definition + 0x28u);
    return {array + selector * 0x5cu, definition, array, selector};
}
struct Descriptor {
    std::uint32_t model, coefficient_offset, incident_offset;
};
inline Descriptor descriptor(const SourcePhysicsContactMemory& memory, std::uint32_t object,
    std::uint32_t section_row, std::uint32_t lod_slot) {
    const auto pool = read(memory, 0x4ecbd0u);
    const auto rows = read(memory, pool + 0x44u);
    const auto model = read(memory, rows + (object & 65535u) * 12u + 8u);
    const auto geometry = read(memory, section_row + 0x34u);
    const auto selectors = read(memory, geometry + 0x68u);
    const auto selector = read(memory, selectors + read(memory, lod_slot), 1);
    const auto bias = std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(
        std::uint16_t(read(memory, model + 0x116u, 2)))));
    return {model, selector * 0x34u, bias};
}
// Complete235E0 lookup, retaining the real130980 ownership boundary. Its
// resident/missing/request lifecycle belongs to that owner; this wrapper does
// not manufacture availability, callee state, stack stores, or completion.
template<class State> void bridge(State& state) {
    const auto tag = state.gp[0];
    state.push(0x235e0u, state.gp[3]); state.gp[3] = 0;
    state.flags.minus(tag, 0xffffffffu, 4);
    state.push(0x235e6u, state.gp[7]); state.gp[7] = state.gp[1];
    if (tag != 0xffffffffu) {
        const auto tags = read(state.memory, 0x4f0414u);
        state.gp[0] = (tag & 65535u) * 16u;
        state.gp[1] = tags;
        state.gp[2] = read(state.memory, tags + state.gp[0] + 8u);
        state.gp[0] = read(state.memory, state.gp[4] + 0x10u);
        state.gp[1] = read(state.memory, state.gp[4] + 0xcu);
        state.push(0x23605u, state.gp[6]);
        const auto selector = read(state.memory, state.gp[1] + state.gp[0], 1);
        const auto array = read(state.memory, state.gp[2] + 0x28u);
        state.gp[6] = array + selector * 0x5cu;
        state.gp[0] = 3; state.gp[1] = state.gp[6] + 0x38u;
        state.flags.plus(selector * 0x5cu, array, 4);
        state.push(0x2361au, 0x2361fu);
        state.required(0x2361au, 0x130980u, false);
        state.flags.logic(state.gp[0] & 255u, 1);
        if (state.gp[0] & 255u) {
            state.gp[7] &= 65535u;
            const auto found = descriptor(state.memory, state.gp[7], state.gp[6], state.gp[4] + 0x18u);
            state.gp[0] = found.model; state.gp[2] = found.incident_offset;
            state.gp[1] = found.coefficient_offset;
            state.gp[6] = state.pop();
            state.gp[1] = state.flags.plus(state.gp[1], state.gp[0], 4);
            state.gp[7] = state.pop();
            state.gp[0] = state.gp[1] + state.gp[2] + 0x28u;
            state.gp[3] = state.pop(); (void)state.pop(); state.gp[4] += 12u;
            return;
        }
        state.gp[6] = state.pop();
    }
    state.gp[7] = state.pop(); state.gp[0] = state.gp[3];
    state.gp[3] = state.pop(); (void)state.pop(); state.gp[4] += 12u;
}
}
