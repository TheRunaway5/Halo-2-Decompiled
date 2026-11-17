#pragma once
#include "halo2/physics_contact_record.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <span>

// Native object factory coordinator owners (track "core"): type dispatch,
// placement-data initialization and birth helpers. See
// docs/NATIVE_OBJECT_FACTORY.md and docs/native_object_factory_tracks/core.md.
namespace halo2::native_object_factory_tu15 {
// Object type definition table 0x474B90 (TU1.5 .data, literal). Slots used by
// the factory: +28 validate, +2C new, +38 place, +3C new failed, +74 nodes.
inline constexpr std::uint32_t type_slot_validate=0x28,type_slot_new=0x2c,type_slot_place=0x38,
  type_slot_new_failed=0x3c,type_slot_nodes=0x74;
// Callback addresses of `slot` along the type's parent chain (object first),
// zero where a level has none. Types outside 0..12 have no chain.
std::array<std::uint32_t,3> object_type_callbacks(std::uint32_t type,std::uint32_t slot);
// Chain level definition addresses (0x473E48 object, ...), zero past the end.
std::array<std::uint32_t,3> object_type_chain(std::uint32_t type);

// B9960: initialize a 0xC4-byte object placement request for `tag`, owned by
// `owner` (copies its team and unit fields when it is a live biped/vehicle),
// at the optional 12-byte location `location` (0 when none). Uses the default
// forward/up vectors at [0x474D08]/[0x474D10].
void placement_data_new(SourcePhysicsContactMemory&,std::uint32_t request,std::uint32_t tag,std::uint32_t owner,
    std::uint32_t location);
// BBE00: the object's world position into `out` (3 floats): body +64 for a
// root; otherwise transformed by its parent's node matrix (body +18 node),
// scaled unless the matrix scale is exactly 1.0. SSE under `mxcsr`.
void object_world_position(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t out,std::uint32_t mxcsr);
// BABA0: refresh the object's spatial records (header bit 0x40 with body +4
// bit 9): native BA910 packet into the [0x4EABB0] chain from body +60.
void object_refresh_spatial(SourcePhysicsContactMemory&,std::uint32_t object);
// C0200: with a model whose +4 and +14 tags exist, the type chain's nodes
// slot (+74) receives the node count and matrices. The only such callback is
// the unit's 1157D0, supplied by the caller (units track); empty refuses.
using ObjectNodesCallback=std::function<void(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t count,
    std::uint32_t matrices)>;
void object_notify_nodes(SourcePhysicsContactMemory&,std::uint32_t object,const ObjectNodesCallback& unit_nodes={});
// 149AD0: physics memory pressure level 0..3 from the two Havok pools
// ([0x486074]/[0x486070] +20) and the memory owner [0x48606C] (+F0340
// usage, 2335B0 weighted block sum). SSE under `mxcsr`.
std::uint32_t physics_memory_pressure(const SourcePhysicsContactMemory&,std::uint32_t mxcsr);
// 1496F0: unless already inside it ([0x48B873]), measure the pressure and,
// below level 2, publish/clear the guard byte. Reclamation (level 2+) deletes
// objects and is a named refusal, raised before any store.
void physics_memory_reserve(SourcePhysicsContactMemory&,std::uint32_t mxcsr);
}
