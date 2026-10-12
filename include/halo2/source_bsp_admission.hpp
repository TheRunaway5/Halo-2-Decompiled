#pragma once
#include "halo2/source_portal_visibility.hpp"
#include "halo2/object_light_spatial.hpp"

namespace halo2 {
//131FF0 initializer→134980.134390 reserves one slot from each queue.
inline constexpr std::uint32_t source_bsp_main_cluster_capacity=128;
inline constexpr std::uint32_t source_bsp_main_instance_capacity=384;
inline constexpr std::uint32_t source_bsp_light_cluster_capacity=64,
    source_bsp_light_instance_capacity=96;
struct SourceBspSphereClusters {
    std::vector<std::int16_t> clusters;
    std::vector<std::uint32_t> sorted_indices;
};
//132D60→14A5B0 with the literal512-entry sphere traversal storage. The
// original authored portal order determines creation order;134950 sorts
// a separate indirection by cluster ID. Camera/frustum banks are unaffected.
SourceBspSphereClusters source_bsp_sphere_clusters(const ObjectLightSpatialBsp&,
    std::int16_t containing_cluster,Vec3 position,float radius);
// Literal BSP fields consumed by134390. Instance spheres are authored world
// spheres at instance+3C/+48; these are not reconstructed from render vertices.
struct SourceBspAdmissionCluster {
    std::uint16_t geometry_count{}; // cluster+24
    std::array<float,6> bounds{}; // cluster+54: min/max axis pairs
    std::vector<std::uint16_t> instances; // ordered cluster+98/+9C list
};
struct SourceBspAdmissionInstance {
    std::int16_t definition=-1; // instance+34, stride58
    Vec3 center{};
    float radius{};
};
struct SourceBspAdmissionDefinition {
    std::uint16_t geometry_count{}; // definition+24, strideC8
};
struct SourceBspAdmissionBsp {
    std::vector<SourceBspAdmissionCluster> clusters;
    std::vector<SourceBspAdmissionInstance> instances;
    std::vector<SourceBspAdmissionDefinition> definitions;
};
struct SourceBspFogAdmissionInput {
    bool scenario_loaded{},bsp_loaded{}; // actual4E0350/4E0348 presence
    std::uint32_t datum=0xffffffffu; // actual4B9F8C = render fog bank+9C
    std::uint32_t flags_4e64c8{}; // bit4 overrides authored membership
};
// Exact16E210 results for every authored cluster. Native atmosphere retains
// cluster+70 signed designators, BSP fogplane+0 palette and scenario palette+8
// datums. Availability/override are explicit current source services.
std::vector<std::uint8_t> source_bsp_fog_admission(const BspAtmosphere&,
    const SourceBspFogAdmissionInput&);
bool source_bsp_cluster_contains_fog(const BspAtmosphere&,int cluster,
    const SourceBspFogAdmissionInput&);
struct SourceBspAdmission;
struct SourceBspAdmissionInput {
    // Original queueheader+0 capacities.134390 reserves the final slot; a
    // capacity of1 admits no records. Defaults are the literal131FF0 main
    // initializer; other contexts/capacity probes must supply their own values.
    std::uint32_t cluster_capacity=source_bsp_main_cluster_capacity,
        instance_capacity=source_bsp_main_instance_capacity;
    // Normal main-mode0 is flags0/context+2A78=0/auxNULL. The verified optional
    // branches are20(skip),80(forceinstance admission),200(clipping plane).
    std::uint32_t flags{},context_2a78{};
    ShaderVector clipping_plane{};
    // Exact16E210 result for each authored cluster, produced with the caller's
    // actual scenario/BSP availability, fog datum and4E64C8bit4 state.
    std::span<const std::uint8_t> containing_fog;
    //132B80/132E60 sphere-volume context, used by ordinary spherical lights.
    // Mode0 retains the original portal-frustum path above.
    std::uint32_t mode{};
    Vec3 sphere_center{};
    float sphere_radius{};
    //Actual prior context's cluster/instance queues. Flag1 enables its source
    //163D20 membership lookup; absence does not create a visible-all bank.
    const SourceBspAdmission *auxiliary{};
};
struct SourceBspAdmissionRecord {
    std::uint32_t id{};
    std::uint16_t flags{},association=0xffff;
    std::uint32_t other{}; // source queueheader+10 stream;134390 writes zero
};
struct SourceBspAdmission {
    std::array<std::uint8_t,512> cluster_entries{}; // authored cluster→entry;FF absent
    std::array<std::uint32_t,16> marked_clusters{};
    std::vector<SourceBspAdmissionRecord> clusters,instances;
    // Four words per source association group. Bits refer to constructed
    // cluster-entry ordinal, not authored cluster ID. At512 groups allocator
    // returns0 without initializing a new mask, exactly as134300 does.
    std::vector<std::array<std::uint32_t,4>> associations;
    bool association_capacity_reached{}; // diagnostic; not a source flag
    const SourceBspAdmissionRecord *instance(std::size_t authored_id) const;
    // BSP vertex encoding>=0 cluster,<=-2 instance,-1 unowned/dynamic.
    bool geometry_visible(int encoding) const;
    bool geometry_contains_fog(int encoding) const;
};
// Original134390→132E60/165010/181800/134300 after1348E0 reset and a fresh
// visited epoch. Supports main mode0 and light sphere mode1 with explicit
// auxiliary queues. Dynamic object
// admission132220 and partial part sorting/culling remain separate services.
SourceBspAdmission source_bsp_admission(const SourceBspAdmissionBsp&,
    const SourcePortalVisibilitySet&,const SourceBspAdmissionInput&);
}
