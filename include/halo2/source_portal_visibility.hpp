#pragma once
#include "halo2/source_frustum_camera.hpp"
#include "halo2/source_portal_frustum.hpp"
#include "halo2/source_visibility.hpp"

namespace halo2 {
struct SourcePortalBsp {
    std::vector<ShaderVector> planes;
    std::vector<SourcePortalGeometry> portals;
    // Literal BSP cluster+8C/+90 portal order, not inferred adjacency.
    std::vector<std::vector<std::uint16_t>> cluster_portals;
};
struct SourcePortalVisibilitySet {
    std::vector<SourceFrustumCamera> groups;
    std::vector<SourceClusterFrustumRanges> clusters;
    std::vector<SourceCameraFrustum> frustums;
    // Source134950 sorts this indirection; cluster entries retain creation order.
    std::vector<std::uint32_t> sorted_cluster_indices;
    // Diagnostics for bounded source arenas. 173130 retains the emitted prefix
    // when its callees exhaust these arenas; it does not replace it with PVS.
    enum Limit : std::uint32_t {nodes=1,projected_polygon_arena=2,
        traversal_polygon_arena=4,queue=8,clusters_limit=16,frustums_limit=32};
    std::uint32_t exhausted_limits{};
    SourceClusterFrustumSet sphere_bank() const;
};
// Original173130/172840/172C70/1730A0/172EE0 construction from supplied source
// camera banks and the actual512bit4F93A4 open-portal mask. Up to six groups,
// 512 authored clusters/portals,256 traversal nodes,128 cluster entries and
// 512 output frustums. A containing_cluster of-1 produces the source empty set.
// Live camera construction, cluster query and door-mask chronology are callers.
SourcePortalVisibilitySet source_portal_visibility(
    const SourcePortalBsp&,std::span<const SourceFrustumCamera> groups,
    int containing_cluster,std::span<const std::uint32_t> open_portals);
}
