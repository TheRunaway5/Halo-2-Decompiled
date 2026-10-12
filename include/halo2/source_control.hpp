#pragma once
#include "halo2/cache.hpp"

namespace halo2 {
// 1071E0's shared group value and flags. Bit0 is authored one-change-only;
// bit1 records a successful change. Power and position can share these groups.
struct SourceDeviceGroup {
    float value{};
    std::uint16_t flags{};
};
struct SourceControlDefinition {
    std::uint32_t datum{}, model=0xffffffffu, action_string{};
    std::uint16_t type{}, triggers_when{};
    float call_value{};
    std::uint32_t on=0xffffffffu, off=0xffffffffu, deny=0xffffffffu;
};
struct SourceControlActivation {
    bool attempted{}, changed{};
    float requested{};
    std::uint32_t effect=0xffffffffu;
};
// The value/flags portion of complete1071E0. Object wake, physics-dirty and
// replication notifications are separate consumers of a successful change.
bool source_device_group_set(SourceDeviceGroup &,float requested);
// 11BA50 after an actual admitted control activation. This does not select a
// nearby control, admit a player action, or manufacture a private group.
// A missing position group returns without an effect, as the source does.
SourceControlActivation source_control_activate(const SourceControlDefinition &,
                                                SourceDeviceGroup *position);
}
