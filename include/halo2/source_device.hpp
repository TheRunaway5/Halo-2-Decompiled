#pragma once
#include "halo2/model.hpp"
#include "halo2/source_machine_door.hpp"

namespace halo2 {
struct SourceDeviceChannel {
    std::uint32_t graph=0xffffffffu;
    std::uint16_t animation{};
    std::uint8_t animation_type{}; // jmad clip+10: 1 adds authored overlay channels.
    AnimationClip clip;
};
struct SourceMachineDefinition {
    std::string name;
    std::uint32_t model=0xffffffffu;
    std::uint32_t render_model=0xffffffffu;
    std::uint16_t type{}; // mach+11C: door0, platform1, gear2.
    std::uint16_t object_flags{},machine_flags{}; // Definition +2/+11E.
    float powered_position_rate{},unpowered_position_rate{}; // mach+C8/D0.
    SourceMachineDoorDefinition door; // Device+118 and machine+120.
    std::optional<AnimationClip> initialized_idle;
    std::optional<SourceDeviceChannel> position,power;
};
struct SourceMachinePlacement {
    std::int16_t power_group=-1,position_group=-1;
    std::uint32_t device_flags{},machine_flags{};
    // 107A30 initializes named groups; 1076E0 creates private groups when NONE.
    float initial_power{},initial_position{};
    std::uint16_t power_group_flags{},position_group_flags{};
};
// Original 2818E0, no node remapping/mask and full overlay weight. Absent
// components preserve the existing pose, including a nonidentity bind pose.
BonePose source_device_overlay(BonePose base,BonePose delta,unsigned components);
// 1C6620 phase setter. This does not loop a device channel independently.
float source_device_channel_frame(unsigned frame_count,float normalized_position);
struct SourceMachineGearStep {float position{},rate{};bool changed{},wrapped{};};
// 1152E0's type2 gear arithmetic. Position/power and dt are actual tick state.
// Source subtracts one period once; scripts/events and other machine types
// have separate state producers and are not substituted here.
SourceMachineGearStep source_machine_gear_step(const SourceMachineDefinition &,
    float position,float power,float delta_seconds);
// 115020's placement+3C bits, stored separately from base object/device flags.
std::uint32_t source_machine_placement_flags(std::uint32_t authored_flags);
// 115190 constructor: machine bit2 sets object bits12/13; bit11 is always set.
std::uint32_t source_machine_object_flags(std::uint32_t existing,
                                        const SourceMachineDefinition &);
// BD090/BDB60/107000 order: bind channels, initialized primary action,
// position, then power. base_frame is the actual primary controller frame;
// the position/power frames come from normalized device state, not seconds.
// Returns model-order local channels for the existing hierarchy/skin path.
std::vector<BonePose> source_machine_local_pose(const RenderModel &,
    const SourceMachineDefinition &,float position,float power,float base_frame=0);
}
