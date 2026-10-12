#pragma once
#include "halo2/bsp_surface_lighting.hpp"
#include "halo2/source_object_spatial_scan.hpp"
#include "halo2/source_object_vector_contact.hpp"
#include <array>

namespace halo2 {
// Literal1697C0 packet. Fields not written by the selected source path retain
// the caller's words, including the upper material halfword and last dword.
using SourceVectorContactPacket=std::array<std::uint32_t,24>;
struct SourceVectorFogPlane {
    std::int16_t material=-1; // Authored BSP fog plane+2, not a shader datum.
    CollisionPlane plane;
};
struct SourceVectorContactBank {
    const BspSurfaceLighting& bsp;
    std::int16_t bsp_index{}; // Actual4686C4, independently supplied.
    std::int16_t default_material{}; // Literal47D8E0 fallback.
    std::span<const std::int16_t> global_materials; // sbsp+10, stride14,+8.
    std::span<const std::int8_t> cluster_fog; // cluster+70 signed designator.
    std::span<const SourceVectorFogPlane> fog_planes;
    std::span<const std::uint32_t> breakable_bits;
};
struct SourceVectorContactResult {
    bool hit{};
    SourceVectorContactPacket packet;
    // Actual admitted1EFB40 order, including misses/non-improving targets.
    std::vector<std::uint32_t> targets;
};
// Ordinary1697C0 over explicit actual map/accelerator/registered spatial and
// owner banks. Each visited cluster checks fog, instances, then dynamic objects
// against the same fraction. The target service is the separately proved
//1EFB40 lookup/1EFF40/168B40 contract; it must query the whole segment.
// Query20000000's subsequent location-repair service is deliberately rejected.
// No live registration, pose, breakable or active-permutation state is inferred.
SourceVectorContactResult source_vector_contact(const SourceVectorContactBank&,
    const SourceObjectSpatialPool& collidable,
    std::span<const SourceObjectVectorOwner>,const SourceObjectVectorInput&,
    const SourceObjectVectorTarget&,
    SourceVectorContactPacket caller_packet={});
}
