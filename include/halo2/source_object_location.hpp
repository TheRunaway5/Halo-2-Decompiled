#pragma once
#include "halo2/object_light_spatial.hpp"

namespace halo2 {
struct SourceObjectLocation {
    std::uint32_t leaf=0xffffffffu;
    std::int16_t cluster=-1,bsp=-1;
};
//11BED0→14A280 with literal collision nodes/planes/render-leaf mapping from
// the active BSP. The source includes the positive child on a plane boundary.
SourceObjectLocation source_object_point_location(const BspAtmosphere&,
    std::int16_t selected_bsp,Vec3 point);
//1DDE10→1DDEA0's third output bank, retaining its first256 collision leaves.
// No surface/edge/vertex contact output or function return status is claimed.
// BF510 consumes only this independent ordered leaf count/prefix.
std::vector<std::uint32_t> source_object_sphere_leaves(const BspAtmosphere&,
    ObjectLightSpatialSphere);
//BF510's normal initial-location branch: primitive center first, then first
// sphere leaf, then object+64 only if the sphere leaf bank is empty. The input
// is the actual retained object+40/+4C primitive, not a render-mesh centroid.
SourceObjectLocation source_object_initial_location(const BspAtmosphere&,
    std::int16_t selected_bsp,ObjectLightSpatialSphere primitive,Vec3 root_position);
//B8600's NULL supplied-location branch has its own point→root fallback. The
// caller must retain old object+28/+2C when this returns clusterNONE, as retail
// does; that historical registration field is not initialized by this helper.
SourceObjectLocation source_object_default_registration_location(const BspAtmosphere&,
    std::int16_t selected_bsp,Vec3 primitive_center,Vec3 root_position);
}
