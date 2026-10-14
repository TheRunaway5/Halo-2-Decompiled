#pragma once
#include "halo2/cache.hpp"
#include <array>
#include <functional>
#include <optional>

namespace halo2 {
struct SourceMachineDoorDefinition {
    float activation_radius{}, open_seconds{};
};
struct SourceMachineDoorState {
    std::uint32_t flags{}; // Original machine+1CC, distinct from device flags.
    std::int32_t open_ticks{}; // Original machine+1D0, signed and wrapping.
};
struct SourceMachineDoorUnit {
    std::uint32_t handle=~0u, player=~0u;
    std::uint32_t unit_flags{}, vehicle_flags{}; // Definition+BC/+1EC.
    std::uint8_t type{}, flags_10a{};
    Vec3 center;
    float radius{}; // Used by the native bound-query adapter, not the gates.
};
struct SourceMachineDoorMarker { Vec3 position, forward; };
struct SourceMachineDoorInput {
    std::uint32_t handle=~0u, tick{};
    bool editor{}, position_group_present=true;
    float position{}, radius{};
    Vec3 center, forward{1,0,0};
    std::optional<SourceMachineDoorMarker> one_sided_marker;
};
// The caller supplies the actual ordered BB050(1,3) candidate bank, capacity16.
// It is queried only on the original four-tick phase. Registration and posed
// bounds belong to the world owner; this reducer does not invent them.
using SourceMachineDoorQuery = std::function<std::span<const SourceMachineDoorUnit>(float)>;
struct SourceMachineDoorStep {
    bool queried{};
    float query_radius{};
    std::array<float,2> position_targets{};
    unsigned target_count{};
};
// Retail115571..11599B. Vista58DB44..58DF44 confirms the owner and gates,
// but uses an x87 timer product; retain Xbox's rounded SSE float product here.
// Group writes remain ordered requests for the existing group owner.
SourceMachineDoorStep source_machine_door_step(const SourceMachineDoorDefinition &,
    SourceMachineDoorState &,const SourceMachineDoorInput &,float delta_seconds,
    const SourceMachineDoorQuery &);
}
