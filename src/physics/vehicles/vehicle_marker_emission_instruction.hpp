#pragma once

#include "physics/runtime/physics_instruction_remap_tu15.hpp"

namespace halo2 {
namespace vehicle_marker_emission_instruction_detail {
inline constexpr std::uint32_t actual_first = 0x1e1200u;
inline constexpr std::uint32_t actual_last = 0x1e135au;
inline constexpr std::uint32_t image_delta = 0x48c0u;
inline constexpr std::uint32_t canonical_sites[]{
    0x1dc940u, 0x1dc943u, 0x1dc944u, 0x1dc946u, 0x1dc949u, 0x1dc94fu, 0x1dc955u,
    0x1dc95bu, 0x1dc95eu, 0x1dc962u, 0x1dc965u, 0x1dc968u, 0x1dc96bu, 0x1dc96eu,
    0x1dc972u, 0x1dc976u, 0x1dc97cu, 0x1dc97du, 0x1dc981u, 0x1dc982u, 0x1dc986u,
    0x1dc987u, 0x1dc990u, 0x1dc993u, 0x1dc997u, 0x1dc99bu, 0x1dc99du, 0x1dc99fu,
    0x1dc9a1u, 0x1dc9a3u, 0x1dc9a5u, 0x1dc9a7u, 0x1dc9aau, 0x1dc9adu, 0x1dc9b0u,
    0x1dc9b3u, 0x1dc9b5u, 0x1dc9bbu, 0x1dc9bfu, 0x1dc9c5u, 0x1dc9c9u, 0x1dc9cdu,
    0x1dc9cfu, 0x1dc9d0u, 0x1dc9d3u, 0x1dc9d5u, 0x1dc9d7u, 0x1dc9dau, 0x1dc9ddu,
    0x1dc9e0u, 0x1dc9e3u, 0x1dc9e6u, 0x1dc9ebu, 0x1dc9eeu, 0x1dc9f3u, 0x1dc9f8u,
    0x1dc9fdu, 0x1dca05u, 0x1dca09u, 0x1dca0cu, 0x1dca0eu, 0x1dca11u, 0x1dca14u,
    0x1dca17u, 0x1dca1au, 0x1dca1du, 0x1dca20u, 0x1dca23u, 0x1dca26u, 0x1dca27u,
    0x1dca2bu, 0x1dca30u, 0x1dca33u, 0x1dca37u, 0x1dca39u, 0x1dca3cu, 0x1dca40u,
    0x1dca42u, 0x1dca45u, 0x1dca48u, 0x1dca4du, 0x1dca52u, 0x1dca55u, 0x1dca5au,
    0x1dca5fu, 0x1dca64u, 0x1dca69u, 0x1dca6du, 0x1dca71u, 0x1dca74u, 0x1dca75u,
    0x1dca78u, 0x1dca7au, 0x1dca7eu, 0x1dca82u, 0x1dca88u, 0x1dca89u, 0x1dca8au,
    0x1dca8bu, 0x1dca8du, 0x1dca8eu, 0x1dca91u, 0x1dca94u, 0x1dca96u, 0x1dca97u,
    0x1dca9au,
};

inline bool covers_marker(const PhysicsInstructionRemapTu15 &remap,
                          std::span<const std::uint32_t> sites = canonical_sites,
                          std::uint32_t delta = image_delta) {
    if (sites.empty()) return false;
    const auto *first = remap.find(sites.front());
    if (!first) return false;
    const auto remaining = remap.bindings.size() - std::size_t(first - remap.bindings.data());
    if (remaining < sites.size() || (remaining > sites.size() && first[sites.size()].key <= sites.back()))
        return false;
    return std::equal(sites.begin(), sites.end(), first,
        [delta](auto pc, const auto &row) { return row.key == pc && row.value == pc + delta; });
}

inline bool native_marker_position(const SourcePhysicsContactRuntimeServices &source,
                                   std::span<const std::uint32_t> sites, std::uint32_t delta,
                                   std::uint32_t *&position) {
    position = nullptr;
    const auto *mapped = source.instruction.target<PhysicsInstructionRemapTu15>();
    if (!mapped || !mapped->source || !covers_marker(*mapped, sites, delta))
        return false;
    if (!mapped->source->instruction) return true;
    const auto *cursor = mapped->live_cursor();
    if (!cursor || (cursor->first_trace && *cursor->first_trace) ||
        (cursor->second_trace && *cursor->second_trace))
        return false;
    position = cursor->position;
    return true;
}
} // namespace vehicle_marker_emission_instruction_detail

// The selected emitter publishes TU1.5 PCs. The vehicle composition expects
// canonical observations and maps those PCs back to the selected image.
class VehicleMarkerEmissionInstruction {
    const SourcePhysicsContactRuntimeServices &source_;

public:
    explicit VehicleMarkerEmissionInstruction(const SourcePhysicsContactRuntimeServices &source) : source_(source) {}

    void operator()(std::uint32_t pc) const {
        using namespace vehicle_marker_emission_instruction_detail;
        source_.instruction(pc >= actual_first && pc <= actual_last ? pc - image_delta : pc);
    }

    // This cursor is valid for the marker emitter's own sites. Calls into any
    // other owner keep their existing forwarding and observation contracts.
    // An exact remap whose source callback is empty succeeds with no cursor.
    bool native_position(std::uint32_t *&position) const {
        using namespace vehicle_marker_emission_instruction_detail;
        return native_marker_position(source_, canonical_sites, image_delta, position);
    }
};
} // namespace halo2
