#pragma once
#include "halo2/model.hpp"
namespace halo2 {
// Original 0003EBD0 sky scale, yaw and camera-parallax transform.
std::vector<Vertex> pose_sky(const RenderModel &, const SkyDefinition &,
                             Vec3 eye);
} // namespace halo2
