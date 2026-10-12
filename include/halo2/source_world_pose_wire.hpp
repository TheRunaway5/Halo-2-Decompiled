#pragma once
#include "halo2/source_world_pose.hpp"
#include <span>

namespace halo2 {
// Native transport owners, derived from the accepted roster. A living current
// body is required; a retired corpse may no longer have a source hierarchy.
struct SourceWorldPoseOwner {std::uint32_t object;bool required;};
struct SourceWorldPoseBank {
    bool authoritative{};
    std::vector<SourceWorldPose> poses;
    std::size_t consumed{};
};
std::vector<std::byte> encode_source_world_poses(bool authoritative,
    std::span<const SourceWorldPose>,std::span<const SourceWorldPoseOwner>);
SourceWorldPoseBank decode_source_world_poses(std::span<const std::byte>,
    std::span<const SourceWorldPoseOwner>);
}
