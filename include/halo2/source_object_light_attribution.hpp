#pragma once
#include "halo2/object_light_contacts.hpp"
#include "halo2/posed_collision.hpp"

namespace halo2 {
// Literal resident CPU geometry used by23AC30. These bytes are distinct from
// the GPU position decoder and expanded/posed RenderModel mesh vertices.
struct SourceObjectLightPositionStream {
    std::uint8_t format{};
    std::uint32_t vertex_count{};
    std::vector<std::byte> bytes;
};
struct SourceObjectLightPosition {Vec3 point;std::uint32_t node_hint{};};
SourceObjectLightPosition source_object_light_render_position(
    const SourceObjectLightPositionStream&,std::span<const float,6> bounds,
    std::uint32_t vertex);
//142700, rather than constructing an inverse matrix and multiplying it.
Vec3 source_object_light_inverse_point(const Transform&,Vec3);
struct SourceObjectLightAttributionModel {
    struct Section {
        // Explicit12DE70 mode3 result. A native synchronous loader may supply
        // its actually resident bank; this does not reproduce async scheduling.
        bool resident{};
        SourceObjectLightPositionStream positions;
        std::vector<std::uint16_t> indices;
        ObjectLightRenderLeaves::Section leaves;
    };
    std::uint32_t render_model=0xffffffffu;
    std::array<float,6> position_bounds{}; //mode+18 first compression bounds.
    std::vector<Transform> inverse_binds; //mode+4C node stride60,+28.
    //mode+20 region/permutation,+C highest geometry section (LOD5).
    std::vector<std::vector<std::int16_t>> highest_sections;
    std::vector<Section> sections;
};
struct SourceObjectLightAttributionInput {
    std::uint32_t owner=0xffffffffu;
    std::span<const std::uint8_t> active_permutations; //current body+11A.
    std::span<const Transform> world_nodes; //current body+114/+116.
    //Actual1EFB40 resolved bank; nullptr denotes its original failure.
    const ModelCollision* collision{};
    PosedCollisionHit contact;
    Vec3 world_point; //1697C0 contact+8, already rounded by original query.
};
struct SourceObjectLightAttribution {
    std::uint32_t owner{},render_model{},section{};
    std::int16_t region{};
    std::uint16_t strip{},lightmap_triangle{};
    std::uint32_t node_hint{},part{};
    float weight_b{},weight_c{};
};
//Complete14A8E0 attribution after explicit14AF40 object-action admission.
//No owner registration, pose/active producer or D1E10 sampling is inferred.
//nullopt is a proved source miss; incomplete/malformed native banks throw.
std::optional<SourceObjectLightAttribution> source_object_light_attribution(
    const SourceObjectLightAttributionModel&,const SourceObjectLightAttributionInput&);
} // namespace halo2
