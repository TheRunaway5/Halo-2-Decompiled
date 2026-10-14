#pragma once
#include "halo2/source_world_pose.hpp"
namespace halo2 {
// Offline shader verification inputs retained from an unchanged original
// frame. They never alter gameplay or its source banks. Ordinary binaries
// have no reference lookup; only vehicle_shading_capture compiles this seam.
struct VehicleShadingReference {
  std::uint32_t definition{};
  int netgame_row{};
  std::vector<Transform> nodes;
  ShadowCasterSphere sphere;
  Vec3 origin;
  Vec3 velocity;
  std::uint32_t flags{};
  bool frozen{};
  std::uint32_t geometry_flags{};
  std::uint16_t geometry_control_flags{};
  std::array<std::int8_t,4> expected_geometry_lods{};
  std::array<std::uint32_t,21> expected_lighting{};
};
void vehicle_shading_reference_material_storage(Material&);
const VehicleShadingReference* vehicle_shading_reference(std::uint32_t definition,int netgame_row);
}
