#pragma once
#include "halo2/source_vector_contact.hpp"
#include "halo2/source_object_light_attribution.hpp"

namespace halo2 {
// Literal56-byte attribution record consumed by D1E10. Misses retain partial
// original writes; this is not a fully initialized successful-contact struct.
using SourceRenderContactPacket=std::array<std::uint32_t,14>;
struct SourceRenderContactOwner {
    std::uint32_t full_handle=0xffffffffu;
    std::int8_t current_type=-1; // Actual body+AA, separately from tag type.
    ObjectLightContactDefinition definition;
    const SourceObjectLightAttributionModel* render{};
    const ModelCollision* collision{};
    std::span<const std::uint8_t> active_permutations;
    std::span<const Transform> world_nodes;
};
struct SourceRenderContactInput {
    bool bank_valid{}; // Actual14AF40 active-lightmap/bitmap/checksum guard.
    bool initial_query{}; // Original AL, independently of directional mode.
    Vec3 origin,delta;
    std::uint32_t excluded=0xffffffffu;
    SourceRenderContactPacket caller_packet{};
    // Explicit original stack words if a caller needs all internal packet
    // bytes. Only the low material halfword is initialized by14AF40 itself.
    SourceVectorContactPacket collision_packet{};
};
struct SourceRenderContactAttempt {
    std::uint32_t flags{},excluded=0xffffffffu;
    Vec3 origin;
    SourceVectorContactResult vector;
};
struct SourceRenderContactResult {
    bool hit{};
    Vec3 origin; // Original EDI current start, including admitted retries.
    SourceRenderContactPacket packet;
    std::vector<SourceRenderContactAttempt> attempts;
};
// Complete ordinary14AF40 control with exact1697C0 and14A8E0 data services.
// Explicit mode3 residency is parallel to actual cluster/definition banks:
//0=proved miss,1=actually resident,2=unresolved async scheduling (throws).
// A missing active/native owner bank is an unresolved input, not a source miss.
// No registration, current-pose, residency scheduling or D1E10 sample invented.
SourceRenderContactResult source_render_contact(const SourceVectorContactBank&,
    const SourceObjectSpatialPool&,std::span<const SourceObjectVectorOwner>,
    std::span<const SourceRenderContactOwner>,
    std::span<const std::uint8_t> cluster_resident,
    std::span<const std::uint8_t> definition_resident,
    const SourceObjectVectorTarget&,const SourceRenderContactInput&);
}
