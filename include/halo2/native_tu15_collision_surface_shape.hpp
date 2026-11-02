#pragma once
#include "halo2/native_tu15_contact_producer.hpp"
namespace halo2 {
// Native DATA record for actual1F5FE0's five inputs: collision model,
// category byte, resource identity, surface index, and encoded component key.
// The owner replaces fields0/1 with the resolved surface and first edge. These
// bytes can be reused by a later contact-record constructor; they are retained
// caller data, never an emulated CPU argument/register file.
struct NativeTu15CollisionSurfaceArguments {std::uint32_t identity{};};
struct NativeTu15CollisionSurfaceTransform {std::uint32_t identity{};};
struct NativeTu15CollisionSurfaceScratch {std::uint32_t transformed_vertex{};};
struct NativeTu15CollisionSurfaceStats {std::uint64_t calls{},vertices{},transformed_vertices{};};
// Complete actual1F5FE0: follow the authored surface's edge ring and publish
// its polygon shape, with the optional original34-byte placement transform.
// No callback, recorded geometry, CPU register postimage or execution backend.
NativeTu15ContactComponent native_tu15_build_collision_surface_shape(
    SourcePhysicsContactMemory&,NativeTu15ContactComponent,
    NativeTu15CollisionSurfaceArguments,NativeTu15CollisionSurfaceTransform,
    NativeTu15CollisionSurfaceScratch,NativeTu15PhysicsFrameFp&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15CollisionSurfaceStats* = nullptr);
} // namespace halo2
