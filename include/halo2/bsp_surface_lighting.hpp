#pragma once
#include "halo2/content.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/source_instance_accelerator.hpp"
#include <optional>
namespace halo2 {
// 14AC60 returns render geometry attribution, not collision fan barycentrics.
struct RenderSurfaceHit {
    std::int32_t cluster=-1, instance=-1, geometry=-1, part=-1;
    std::uint16_t strip{}, lightmap_triangle{};
    float weight_b{},weight_c{};
};
struct BspSurfaceSample {
    Vec3 incident;
    Vec2 diffuse_uv,lightmap_uv;
    std::uint32_t shader=0xffffffffu,lightmap{};
    std::optional<Vec3> radiance;
};
// 11E800's projected point-in-triangle test, including its authored tolerance
// and dominant-axis tie order. False leaves barycentric outputs untouched.
bool original_render_barycentrics(Vec3 a,Vec3 b,Vec3 c,Vec3 point,float &b_weight,float &c_weight);
Vec3 decode_cpu_lightmap_incident(std::uint32_t);
struct BspSurfaceLighting {
    struct Part {std::uint16_t first{},count{},material{},type{};};
    struct Geometry {
        std::vector<Vec3> positions;
        std::vector<Vec2> diffuse_uv,lightmap_uv;
        std::vector<std::uint16_t> indices;
        // D31C0's CPU-selected resident geometry stream, including literal
        // fallback-to-last. GPU instance buckets remain in `lighting`.
        std::vector<std::uint32_t> incident;
        std::vector<Part> parts;
    };
    struct Reference {std::uint16_t strip{},lightmap_triangle{};std::int32_t collision_node=-1;};
    struct Leaf {std::int16_t cluster=-1;std::uint16_t count{};std::uint32_t first{};};
    struct CollisionBank {
        struct Node {std::uint16_t plane{};std::uint32_t negative{},positive{};};
        struct CollisionLeaf {std::uint8_t flags{},count{};std::uint16_t first{};};
        struct Reference2 {std::uint16_t plane{},node{};};
        struct Node2 {Vec2 normal;float distance{};std::array<std::uint16_t,2> children{};};
        struct Surface {std::uint16_t plane{},edge{};std::uint8_t flags{},breakable{};std::int16_t material=-1;};
        struct Edge {std::array<std::uint16_t,2> vertex{},next{};std::array<std::int16_t,2> surface{};};
        std::vector<Node> nodes;
        std::vector<ShaderVector> planes;
        std::vector<CollisionLeaf> collision_leaves;
        std::vector<Reference2> references2;
        std::vector<Node2> nodes2;
        std::vector<Surface> surfaces;
        std::vector<Edge> edges;
        std::vector<Vec3> vertices;
        std::vector<std::uint16_t> node_planes,surface_planes;
        std::vector<std::vector<std::uint32_t>> surface_leaves;
        std::vector<Leaf> leaves;
        std::vector<Reference> references;
    };
    struct Instance {
        std::uint16_t definition{},policy{};
        float scale=1;
        std::array<Vec3,3> axes;
        Vec3 translation;
        Vec3 collision_center;
        float collision_radius{};
    };
    std::int32_t bsp=-1;
    std::vector<Geometry> clusters,definitions;
    std::vector<CollisionBank> collision,definition_collision;
    std::vector<Instance> instances;
    std::vector<std::vector<std::uint16_t>> cluster_instances;
    std::vector<std::uint32_t> shaders;
    BspLighting lighting;
    // An explicit original collision leaf reproduces 14AC60's ordered scan.
    // Host hits without that leaf are accepted only when every authored
    // surface membership gives the same render result; ambiguity stays empty.
    std::optional<RenderSurfaceHit> attribute(const CollisionSurfaceIdentity &,Vec3 world_point,
        std::optional<std::uint32_t> original_leaf={}) const;
    std::optional<BspSurfaceSample> interpolate(const RenderSurfaceHit &) const;
    std::optional<ObjectLightmapSample> sample(const RenderSurfaceHit &,float detail=1) const;
    // D1E10's authored part4/5 branch initializes a fixed sample for a new
    // query and otherwise leaves all five caller words untouched. A retained
    // query reaching that branch requires the actual cache+30 target.
    std::optional<ObjectLightmapSample> sample(const RenderSurfaceHit &,float detail,
        bool initial_query,const std::optional<ObjectLightmapSample>& caller_sample) const;
    // D30D0's selected CPU diffuse images, parallel to source material slots.
    std::vector<Texture> diffuse;
    // Literal cluster+A8/+AC MoppInfo banks. Empty means the source caller
    // skips this cluster; membership is not substituted for bytecode order.
    std::vector<std::optional<SourceInstanceAccelerator>> cluster_accelerators;
};
struct BspVectorContact {
    float fraction{};
    std::uint32_t leaf{},surface{},plane{};
    std::uint16_t oriented_plane{};
    std::uint8_t flags{},breakable{};
    std::int16_t material=-1;
};
struct BspVectorResult {
    std::optional<BspVectorContact> contact;
    std::vector<std::uint32_t> leaves;
};
// Original 1DE630/1DE6D0/1DEA00/296520, with explicit source breakable state.
BspVectorResult original_bsp_vector_query(const BspSurfaceLighting::CollisionBank&,Vec3 origin,
    Vec3 displacement,float maximum_fraction=1,std::uint32_t test_flags=9,
    std::span<const std::uint32_t> breakable_mask={});
struct MapVectorContact {
    CollisionSurfaceIdentity source;
    Vec3 point;
    BspVectorContact contact;
};
struct MapInstanceCallback {
    std::uint32_t cluster{},key{};
    float fraction{};
    std::uint32_t previous_type{},previous_owner=0xffffffffu;
};
// Read-only observation of the244C10 entry before instance deduplication.
using MapInstanceCallbackObserver=std::function<void(const MapInstanceCallback&)>;
// Static source BSP leaf/cluster traversal followed by original MOPP order.
std::optional<MapVectorContact> original_instance_vector_contact(const BspSurfaceLighting&,std::uint16_t instance,
    Vec3 origin,Vec3 displacement,float maximum_fraction=1,std::uint32_t test_flags=9);
std::optional<MapVectorContact> original_map_vector_contact(const BspSurfaceLighting&,Vec3 origin,Vec3 displacement,
    bool* unresolved_instance_order=nullptr,std::uint32_t source_query_flags=0x4480000d,
    const MapInstanceCallbackObserver& observer={});
} // namespace halo2
