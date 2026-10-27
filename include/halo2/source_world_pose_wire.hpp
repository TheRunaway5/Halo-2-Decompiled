#pragma once
#include "halo2/source_world_pose.hpp"
#include <span>

namespace halo2 {
// Native transport owners, derived from the accepted roster and vehicle bank.
// Live bodies/vehicles are required; retired corpses may have no hierarchy.
struct SourceWorldPoseOwner {std::uint32_t object;bool required;};
struct SourceWorldPoseBank {
    bool authoritative{};
    std::vector<SourceWorldPose> poses;
    std::size_t consumed{};
};
// Legacy body versions retain their original numeric bank verbatim and have
// no attachment metadata. Parent-frame banks require explicit source linkage
// for every owner, including an actual unparented NONE value.
enum class SourceWorldPoseWireFormat { legacy, parent_frame };
std::vector<std::byte> encode_source_world_poses(bool authoritative,
    std::span<const SourceWorldPose>,std::span<const SourceWorldPoseOwner>,
    SourceWorldPoseWireFormat = SourceWorldPoseWireFormat::legacy);
SourceWorldPoseBank decode_source_world_poses(std::span<const std::byte>,
    std::span<const SourceWorldPoseOwner>,
    SourceWorldPoseWireFormat = SourceWorldPoseWireFormat::legacy);
}
