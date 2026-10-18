#pragma once
#include "halo2/model.hpp"
namespace halo2 {
struct BspAtmosphere;
struct BspFogSample;
// Original 0003EBD0 sky scale, yaw and camera-parallax transform.
std::vector<Vertex> pose_sky(const RenderModel &, const SkyDefinition &,
                             Vec3 eye);
// Per-BSP resources, indexed by the original scenario sky ordinal. The
// completed current-view atmosphere selects the background independently
// of the lighting-sky ordinal and retains ownership of the clear policy.
class BspSkyModels {
public:
  BspSkyModels(const Cache &,const BspAtmosphere &,Mesh &destination);
  std::vector<Vertex> pose(const BspFogSample &,Vec3 eye)const;
private:
  struct Model {
    SkyDefinition definition;
    RenderModel geometry;
    std::uint32_t material_base{};
  };
  std::vector<std::optional<Model>> models_;
  std::vector<bool> referenced_;
};
} // namespace halo2
