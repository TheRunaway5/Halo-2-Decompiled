#pragma once
#include "halo2/cache.hpp"
#include <functional>

namespace halo2 {
struct ObjectLightSpatialDefinition {
    std::uint16_t flags{}; // Object definition+2, including primitive selector bit12.
    float dynamic_radius{}; // +20.
    Vec3 dynamic_offset; // +24/+28/+2C; added directly in world coordinates.
};
struct ObjectLightSpatialSphere {Vec3 center;float radius{};};
struct ObjectLightSpatialBounds {ObjectLightSpatialSphere dynamic,primitive;};
// BDEF0 after the query-sphere producer. Supply actual object+30/+3C,
// including any admitted type11 physics override; no pose proxy is chosen.
ObjectLightSpatialBounds source_object_light_spatial_bounds(
    const ObjectLightSpatialDefinition&,ObjectLightSpatialSphere query,float object_scale);
// Registered primitives move only when squared displacement exceeds the
// source .01 constant, or their radius differs. An unregistered sphere writes
// immediately; membership registration itself is a separate source event.
bool source_object_light_spatial_bounds_changed(ObjectLightSpatialSphere previous,
    ObjectLightSpatialSphere next,bool registered);
// Geometry used by retail14A370/14A5B0. Portal vertex order is authored; the
// visibility/PVS state does not participate in sphere spatial membership.
struct ObjectLightSpatialBsp {
    struct Portal {
        std::int16_t back=-1,front=-1;
        std::uint32_t plane{};
        Vec3 center;
        float radius{};
        std::vector<Vec3> vertices;
    };
    std::vector<std::array<float,4>> planes;
    std::vector<std::vector<std::uint16_t>> clusters;
    std::vector<Portal> portals;
};
bool source_object_light_portal_intersects(const ObjectLightSpatialBsp&,
    std::uint16_t portal,Vec3 center,float radius);
struct ObjectLightSpatialMembership {
    std::vector<std::int16_t> clusters;
    std::uint32_t total{};
    bool truncated{};
};
// Retail1CAC60 with either its normal source location or an explicit forced
// cluster bitset (object flag21). Default object storage is the original256;
//132D60's light sphere traversal explicitly supplies its original512 capacity.
ObjectLightSpatialMembership source_object_light_spatial_membership(
    const ObjectLightSpatialBsp&,std::int16_t initial_cluster,Vec3 center,float radius,
    std::span<const std::uint32_t> forced_clusters={},std::uint32_t storage_capacity=256);
// Retail11E5E0 before the posed target query. No nearest-point approximation.
bool source_object_light_segment_intersects(Vec3 origin,Vec3 delta,Vec3 center,float radius);
struct ObjectLightSpatialPrimitive {
    std::int16_t type{};
    std::uint16_t flags{};
    Vec3 center;
    float radius{};
};
struct ObjectLightSpatialRecord {
    std::uint32_t object=0xffffffffu;
    ObjectLightSpatialPrimitive primitive;
};
// Source1CAC60 prepends each successful registration,1CADF0 updates in place,
// and1CAE40 removes it. Callers supply the actual event chronology and source
// primitives (object+40/+4C), not geometry order or query bounds+30/+3C.
// This representation assumes successful original data-array allocations.
class ObjectLightSpatialLists {
public:
    explicit ObjectLightSpatialLists(std::size_t clusters):clusters_(clusters) {}
    void insert(std::uint32_t object,const ObjectLightSpatialPrimitive&,
        std::span<const std::int16_t> memberships);
    void update(std::uint32_t object,const ObjectLightSpatialPrimitive&);
    void erase(std::uint32_t object);
    std::span<const ObjectLightSpatialRecord> cluster(std::int16_t index) const;
    std::size_t size() const {return clusters_.size();}
private:
    std::vector<std::vector<ObjectLightSpatialRecord>> clusters_;
};
// Retail1697C0's per-cluster composition. `leaf_clusters` retains the original
// collision traversal order. The plane/instance callback runs before objects
// in each newly visited cluster and both callbacks update the shared source
// closest-contact packet. Attachment recursion/current poses remain explicit
// target-service inputs; this helper never substitutes a nearest render mesh.
bool source_object_light_spatial_scan(const ObjectLightSpatialLists&,
    std::span<const std::int16_t> leaf_clusters,std::uint32_t query_flags,
    Vec3 origin,Vec3 delta,std::uint32_t excluded_a,std::uint32_t excluded_b,
    bool initial_contact,const std::function<bool(std::int16_t)>& plane_and_instances,
    const std::function<bool(const ObjectLightSpatialRecord&)>& posed_target);
} // namespace halo2
