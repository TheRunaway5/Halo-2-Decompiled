#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace halo2 {
// Common device tag fields BC..118, consumed by106790 before1152E0.
struct SourceDeviceMotionDefinition {
    std::uint32_t flags{};
    float power_rate{},power_acceleration{};
    float position_rate{},position_acceleration{};
    float unpowered_rate{},unpowered_acceleration{};
    float delay{};
    // Opening, closing, opened, closed; raw authored tag datums.
    std::array<std::uint32_t,4> effects{~0u,~0u,~0u,~0u};
    std::uint32_t delay_effect=~0u;
};
struct SourceDeviceMotionState {
    float position{},position_velocity{},power{},power_velocity{};
    std::uint32_t flags{}; // object+12C, separate from the machine flags.
    std::int16_t delay_ticks{}; // object+148, signed and wrapping.
};
// Entire11EED0 scalar controller, including its mixed float/double rounding.
// dt must be positive; low/high are finite ordered channel bounds.
bool source_device_channel_step(float &position,float &velocity,float target,
    float dt,float acceleration,float maximum_speed,bool loop,float low=0,float high=1);
struct SourceDeviceMotionStep {
    bool updated{},position_dirty{},power_dirty{};
    std::vector<std::uint32_t> effects;
};
//106790's ordinary group-driven path. Scripted animation flags8/10 need the
// original107CC0 producer and reject here. Pose/physics/event consumers receive
// the actual channel changes, rather than being performed by this reducer.
SourceDeviceMotionStep source_device_motion_step(const SourceDeviceMotionDefinition &,
    SourceDeviceMotionState &,std::optional<float> position_target,
    std::optional<float> power_target,float dt,bool retain_initialization=false);
}
