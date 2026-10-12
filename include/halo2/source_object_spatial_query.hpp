#pragma once
#include "halo2/source_object_spatial_pool.hpp"

namespace halo2 {
// BB050/14A5B0's retained global visit banks. The caller owns these across
// queries, including unsigned epoch wrap; neither bank is cleared per call.
struct SourceObjectSpatialQueryState {
    std::uint32_t object_epoch{},cluster_epoch{};
    std::array<std::uint32_t,2048> objects{};
    std::array<std::uint32_t,512> clusters{};
};
struct SourceObjectSpatialQueryInput {
    // Zero selects all in both fields, as in BB050. Lists bit0 is collidable;
    // bit1 is noncollidable. Type shifts retain x86's low-five-bit selector.
    std::uint32_t lists{},types{};
    SourceObjectLocation location;
    Vec3 center;
    float radius{};
    std::uint16_t capacity=64;
};
// Complete BB050 enumeration and original14A5B0 ordered sphere traversal,
// using actual independent spatial pools. Duplicate stamping precedes the
// distance/capacity tests. Results retain cluster/link order and full handles;
// the two list kinds preserve their distinct SSE squared-distance order.
// Registration, primitive bounds and source location are caller-owned inputs.
// This does not discover objects from render meshes or manufacture lifetimes.
std::vector<std::uint32_t> source_object_spatial_query(
    const ObjectLightSpatialBsp&,const SourceObjectSpatialPool &collidable,
    const SourceObjectSpatialPool &noncollidable,
    const SourceObjectSpatialQueryInput&,SourceObjectSpatialQueryState&);
}
