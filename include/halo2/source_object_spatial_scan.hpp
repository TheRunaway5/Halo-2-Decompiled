#pragma once
#include "halo2/source_object_spatial_pool.hpp"

namespace halo2 {
// The ordinary1697C0 contact scan over literal cluster heads and records.
// Orphan entries remain visible after independent reference allocation fails.
// Collision leaves, plane/instance contacts and posed targets are explicit
// source services, with the same contract as ObjectLightSpatialLists overload.
bool source_object_light_spatial_scan(const SourceObjectSpatialPool&,
    std::span<const std::int16_t> leaf_clusters,std::uint32_t query_flags,
    Vec3 origin,Vec3 delta,std::uint32_t excluded_a,std::uint32_t excluded_b,
    bool initial_contact,const std::function<bool(std::int16_t)>& plane_and_instances,
    const std::function<bool(const ObjectLightSpatialRecord&)>& posed_target);
}
