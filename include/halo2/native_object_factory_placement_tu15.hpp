#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include "halo2/physics_contact_record.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <array>
#include <span>

// Native world placement owners (track "placement"): map locations, cluster
// connection and the relocation driver. See
// docs/native_object_factory_tracks/placement.md. Every owner works on the
// live world memory; floating-point steps run under the caller's live x87
// control word and MXCSR (`fp`). Callees that are not native yet are named
// refusals (std::runtime_error), never a call into original code.
namespace halo2::native_object_factory_tu15 {
using PlacementFp=NativeTu15PhysicsFrameFp;

// 14CDB0: BSP3D leaf containing `point` from node `node` of collision BSP
// `bsp` (0x7FFFFF-masked leaf, or 0xFFFFFFFF).
std::uint32_t bsp3d_leaf_from_point(const SourcePhysicsContactMemory&,std::uint32_t bsp,std::uint32_t node,
    std::uint32_t point,std::uint32_t mxcsr);
// The 8-byte s_location: leaf, cluster (int16), structure BSP (int16).
struct Location {std::uint32_t leaf;std::uint16_t cluster,bsp;};
// 11E600 into a caller-local location (the original's stack buffers).
Location location_of_point(const SourcePhysicsContactMemory&,std::uint32_t point,std::uint32_t mxcsr);
// 11E600: the 8-byte s_location of `point` (leaf, cluster, structure BSP),
// written to `out`.
void location_from_point(SourcePhysicsContactMemory&,std::uint32_t out,std::uint32_t point,std::uint32_t mxcsr);
// 11E5C0: the s_location of collision leaf `leaf`, written to `out`.
void location_from_leaf(SourcePhysicsContactMemory&,std::uint32_t out,std::uint32_t leaf);
// C1540: the object's location from its cluster sphere (+40/+4C), falling
// back to the first collision leaf the sphere touches and then to its origin
// (+64). Returns true when the location has a cluster.
bool object_get_location(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t out,const PlacementFp&);

// BA7E0: disconnect the object from the map clusters (C0F60 light detach,
// 1D30F0 reference release, connected flags cleared).
void object_disconnect_from_map(SourcePhysicsContactMemory&,std::uint32_t object,PlacementFp&);
// B9390 (native native_tu15_wake_object_ancestors): mark the object and its
// ancestors for update.
void object_mark_for_update(SourcePhysicsContactMemory&,std::uint32_t object);
// BA8C0: activate the object's physics component (1D97F0, track havok's native)
// and set body +C0 bit 6.
void object_defer_physics(SourcePhysicsContactMemory&,std::uint32_t object,PlacementFp&);

// ---- Cluster connection (1D2F10 and its leaves) ----
// 123210: dominant axis (0..2) of the vector at `vector`.
std::uint16_t dominant_axis(const SourcePhysicsContactMemory&,std::uint32_t vector,std::uint32_t mxcsr);
// 243450: the circle (point, radius) lies inside every edge of the 2-D
// polygon at `vertices` (count int16 vertex pairs).
bool circle_inside_portal(const SourcePhysicsContactMemory&,std::uint32_t vertices,std::uint32_t point,float radius,
    std::uint16_t count,std::uint32_t mxcsr);
// 14CEA0: structure BSP `sbsp` portal `portal` intersects the sphere.
bool portal_intersects_sphere(const SourcePhysicsContactMemory&,std::uint16_t portal,std::uint32_t center,
    std::uint32_t sbsp,float radius,const PlacementFp&);
// Receives cluster list entry `slot` (the original's caller-owned word list).
using ClusterSink=std::function<void(std::uint32_t slot,std::uint16_t cluster)>;
// 14D0E0: depth-first portal flood from `cluster`; entries go to `out` from
// `slot` while `maximum` (low word) is positive. Returns the reached count
// (low word).
std::uint32_t cluster_portal_flood(SourcePhysicsContactMemory&,std::uint16_t cluster,std::uint32_t center,float radius,
    std::uint32_t maximum,const ClusterSink& out,std::uint32_t slot,const PlacementFp&);
// 14D080: clusters whose bit is set, up to `maximum` stored; `total` counts all.
std::uint32_t clusters_from_bits(const SourcePhysicsContactMemory&,const std::function<bool(std::uint32_t)>& bit,
    std::uint16_t maximum,const ClusterSink& out,std::uint16_t& total);
// 1D2F10 cluster choice: a forced bit vector in memory, BA630's local
// all-clusters vector, or (neither) the location plus a portal flood.
struct ClusterSelection {std::uint32_t forced=0;bool all=false;};
struct ClusterLinks {
  std::uint32_t partition{},owner{},owner_head{},center{},radius{},location{},payload_size{};
  std::span<const std::byte> payload;   // caller-local payload (empty: none)
  std::uint32_t overflow{};             // overflow byte address (0: caller-local)
};
// 1D2F10: link the owner into each selected cluster of the partition.
void cluster_partition_reconnect(SourcePhysicsContactMemory&,const ClusterSelection&,const ClusterLinks&,const PlacementFp&);

// ---- Activation and physics creation ----
// B8D90: the object is always active or in an active cluster of `bits`.
bool object_in_active_clusters(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t bits);
// B92C0 / B9330: activate / deactivate a root object (1CC3F0 / 1CC440).
void object_activate(SourcePhysicsContactMemory&,std::uint32_t object,PlacementFp&);
void object_deactivate(SourcePhysicsContactMemory&,std::uint32_t object,PlacementFp&);
// B1870: deletion is protected (game mode 4 and a simulation entity).
bool object_delete_protected(const SourcePhysicsContactMemory&,std::uint32_t object);
// 1CC190: create the object's physics component (Havok creation refused).
void object_create_physics(SourcePhysicsContactMemory&,std::uint32_t object,PlacementFp&);
// 282C10 through native_tu15_process_dirty_island_objects.
void process_dirty_islands(SourcePhysicsContactMemory&,PlacementFp&);
// 1CC290: physics creation over the object and its children (not `root`).
void object_create_physics_tree(SourcePhysicsContactMemory&,std::uint32_t root,std::uint32_t object,PlacementFp&);
// BA630: reconnect the object to the map (location, clusters, lights,
// activation, physics on first connection). `location` 0 computes it.
void object_reconnect_to_map(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t location,PlacementFp&);

// ---- Relocation driver ----
// A relocation input vector: an original memory address (0: none) or a
// caller-local copy (the original's stack locals).
struct PlacementVector {
  std::uint32_t address=0;std::array<std::uint32_t,3>* local=nullptr;
  bool present()const{return address || local;}
};
// Overrides for other tracks' owners the relocation driver calls. Unbound,
// rebuild_nodes is BF050 = object_rebuild_nodes (track nodebuild).
struct PlacementServices {std::function<void(std::uint32_t object)> rebuild_nodes;};
// B9460's byte/dword options after the vectors.
struct RelocationOptions {
  std::uint32_t location=0;bool rebuild_nodes=true,update_physics=true;std::uint32_t physics_force=0;bool keep_links=false;
};
// B7E50: mark simulation entity `entity` dirty with `mask` (game mode 4).
void simulation_entity_dirty(SourcePhysicsContactMemory&,std::uint32_t entity,std::uint32_t mask);
// 1CD620: the Havok body follows a moved object (1DC270, not native yet).
void object_update_physics_position(SourcePhysicsContactMemory&,std::uint32_t object,bool position_absent,bool force);
// E24F0: biped relocation filter (roots keep the global up; forward loses z).
void biped_relocation_filter(SourcePhysicsContactMemory&,std::uint32_t object,const PlacementVector& forward,
    const PlacementVector& up,PlacementFp&);
// 10E340: the type chain's relocation callbacks (+68).
void object_type_relocation(SourcePhysicsContactMemory&,std::uint32_t object,const PlacementVector& position,
    const PlacementVector& forward,const PlacementVector& up,PlacementFp&);
// B9460: move the object (bounds-checked root position, forward/up), then
// physics, node rebuild and cluster reconnection. Returns false when a root
// position lies outside the world bounds (not stored).
bool object_relocate(SourcePhysicsContactMemory&,std::uint32_t object,const PlacementVector& position,
    const PlacementVector& forward,const PlacementVector& up,const RelocationOptions&,PlacementFp&,
    const PlacementServices& = {});
// B95D0: object_set_position(position, forward, up addresses, 0 = none).
void object_set_position(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t position,std::uint32_t forward,
    std::uint32_t up,std::uint32_t location,std::uint32_t physics_force,PlacementFp&,const PlacementServices& = {});

// ---- Object markers ----
// BAC70: the object definition's attachment `attachment` marker name.
std::uint32_t object_attachment_marker(const SourcePhysicsContactMemory&,std::uint32_t object,std::uint16_t attachment);
// BC330: the root ancestor's 8-byte location (+28) into `out`.
void object_root_location(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t out);
// BACD0: the object whose model carries its markers (an item in parent
// mode, FC510, names its +154 owner).
std::uint32_t object_marker_owner(const SourcePhysicsContactMemory&,std::uint32_t object);
// 1E11B0: marker group index of `name` in render model `model`, or NONE.
std::uint32_t model_marker_index(const SourcePhysicsContactMemory&,std::uint32_t model,std::uint32_t name);
// 144910: rotation (3x3) of the quaternion at `quaternion` into `out`.
void quaternion_rotation(SourcePhysicsContactMemory&,std::uint32_t quaternion,std::uint32_t out,std::uint32_t mxcsr);
// 1E1200: up to `maximum` 0x70-byte marker entries (node, local and world
// matrices, extra word) of group `index`; returns the count.
std::uint32_t model_marker_matrices(SourcePhysicsContactMemory&,std::uint32_t index,std::uint32_t model,
    std::uint32_t regions,std::uint32_t node_map,std::uint32_t nodes,bool mirrored,std::uint32_t out,
    std::uint32_t maximum,std::uint32_t mxcsr);
// BAD60: the object's marker matrices for `name` (identity at node 0 when it
// has none). `owner_given` skips BACD0. Returns the count (1 for name 0).
std::uint32_t object_marker_matrices(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t name,
    std::uint16_t maximum,bool owner_given,std::uint32_t out,std::uint32_t mxcsr);

// ---- Collision sphere fit (16D740) ----
// The collision geometry a fitted sphere is tested against (16D740's flags):
// bit 0 the structure BSP, bit 2 instanced geometry, bit 3 objects.
inline constexpr std::uint32_t collision_test_structure=1u,collision_test_instances=4u,collision_test_objects=8u;
// 16D740's result: the fitted centre and radius, and AL (true when a
// nearby collision surface moved the sphere).
struct SphereFit {
  std::array<float,3> center{};
  float radius{};
  bool fitted{};
};
// 16D740 (stack: flags, center, radius, two ignored objects, out center,
// out radius; RET 1C): fit a sphere into the collision geometry around it.
// For every collision surface the sphere reaches, the point of the surface
// nearest to its centre decides a direction; rays along it (or along the
// surface normal) against every collision feature give the free span, whose
// midpoint and half length are averaged over the surfaces. The structure
// BSP (flags bit 0) and instanced geometry (bit 2, 16CF00) are native; a
// query for instances or objects stamps the clusters, instances and objects
// it visits in the .bss visit blocks (0x4F3CE0, 0x4F44E8, 0x4EABC8), as the
// original does. Objects (bit 3, 16D0E0) are a refusal at the first object
// not yet visited. C4D30 calls it for a light whose definition has +0
// bit 15 (flags 0x0D800001); the effects' 17D625 passes 0x0D800005.
SphereFit collision_sphere_fit(SourcePhysicsContactMemory&,std::uint32_t flags,const std::array<float,3>& center,
    float radius,const PlacementFp&);

// ---- Light reconnection ----
// C4C80: the light's bounding sphere (+28/+34): sphere lights copy +18/+24;
// frustum lights bound their eight corners (C55D0, 1E7330).
void light_update_sphere(SourcePhysicsContactMemory&,std::uint32_t light,PlacementFp&);
// C48E0: light bounds (+18/+24, +38) from the definition and placement
// (frustum lights through C3AB0), then C4C80.
void light_update_bounds(SourcePhysicsContactMemory&,std::uint32_t light,PlacementFp&);
// 1E7330: sphere through the farthest pair of `count` 12-byte points, grown
// to hold every point; stores the center (12 bytes) and radius.
void points_bounding_sphere(SourcePhysicsContactMemory&,std::uint32_t points,std::uint32_t count,
    std::uint32_t center,std::uint32_t radius,const PlacementFp&);
// C2870 (ESI position, EAX target, stack: point, distance): march from the
// position toward the target (steps 0.001 doubling to 0.1, while below the
// x87 length) until a point lies in a structure BSP cluster; then stores the
// distance and the point and returns true. Nothing is stored otherwise.
bool light_frame_point(SourcePhysicsContactMemory&,std::uint32_t position,std::uint32_t target,std::uint32_t point,
    std::uint32_t distance,const PlacementFp&);
// C4D30: place the light at its owner's marker (C2870 first for +2 bit 2),
// update its bounds and location, and link it into the light partition
// (0x4ECBE0).
void light_reconnect(SourcePhysicsContactMemory&,std::uint32_t light,PlacementFp&);
}
