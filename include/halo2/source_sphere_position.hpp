#pragma once
#include "halo2/cache.hpp"
#include "halo2/posed_collision.hpp"

namespace halo2 {
// Literal108-byte polygon produced by245AA0 in16A440's terrain contact bank.
// Plane+10, thickness+20, projection+24/+26, vertex count+28, eightUVs+2C.
struct SourceCollisionPolygon { std::array<std::byte,108> bytes; };
struct SourceCollisionSphere { std::array<std::byte,32> bytes; };
struct SourceCollisionCapsule { std::array<std::byte,44> bytes; };
struct SourceSphereContactBank {
    std::span<const SourceCollisionSphere> spheres;
    std::span<const SourceCollisionCapsule> capsules;
    std::span<const SourceCollisionPolygon> polygons;
};
struct SourceSphereContactStorage {
    std::vector<SourceCollisionSphere> spheres;
    std::vector<SourceCollisionCapsule> capsules;
    std::vector<SourceCollisionPolygon> polygons;
    SourceSphereContactBank view() const {return {spheres,capsules,polygons};}
};
//16A440's0D800001 branch, including the +1/16 query expansion, whole
//1DDE10/1DDEA0/1DE220/1DE2C0 traversal and245AA0 terrain contact emission.
// This source query has no runtime breakable filtering, transforms, instances
// or object contacts. It preserves independent256-record caps and dedup order.
SourceSphereContactStorage source_light_spawn_contacts(const RawCollisionBsp&,
    Vec3 center,float radius);
struct SourceSpherePosition {
    bool corrected{};
    Vec3 position;
    float radius{};
};
//16A8E0 and2469B0/2461A0/246360/2466B0 over16A440's contact bank. Terrain
// vertices and edges produce spheres/capsules alongside surface polygons.
// The actual ordered banks must come from the collision owner; an empty bank
// means an actual miss. No terrain/instance/object contact producer is implied.
// This computes both outputs even thoughC2D00 consumes only the new center.
SourceSpherePosition source_sphere_position(Vec3 center,float radius,
    const SourceSphereContactBank&);
}
