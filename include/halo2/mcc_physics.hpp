#pragma once
#include "halo2/mcc_normalized_pointer.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <vector>

namespace halo2 {
// Owned, pre-construction phmo metadata for the existing TU1.5 consumers.
// Serialized addresses retain their format width; TagSystem resolves the
// result to checked native 64-bit backing. MCC tool/runtime pointers are not
// host pointers and are never narrowed into this definition. The verified
// TU1.5 1F2C70 constructor must bind the runtime-only slots before use.
// Authored values and MOPP bytecode are preserved, not declared Xbox-equal.
std::vector<std::byte> normalize_mcc_physics_model(
    std::span<const std::byte> payload, std::uint32_t source_address,
    std::uint32_t target_address,
    const std::function<std::span<const std::byte>(std::uint32_t,std::size_t)>& resident_read={});
// Collision geometry and MOPP bytes are copied without arithmetic changes.
// Original TU1.5 21A580/24FF70 consume 0x70-byte BSP-physics records; the
// generic plugin's 0x74 element size is not the compiled Xbox array stride.
std::vector<std::byte> normalize_mcc_collision_model(
    std::span<const std::byte> payload, std::uint32_t source_address,
    std::uint32_t target_address,
    const std::function<std::span<const std::byte>(std::uint32_t,std::size_t)>& resident_read={});
// Enumerate the normalized graph using the same target offsets and strides
// as the converters above. Every nonempty reference must be wholly owned by
// payload. The returned fields are complete before any caller applies writes.
std::vector<MccNormalizedPointerField> normalized_mcc_physics_pointer_fields(
    std::span<const std::byte> payload, std::uint32_t source_address);
std::vector<MccNormalizedPointerField> normalized_mcc_collision_pointer_fields(
    std::span<const std::byte> payload, std::uint32_t source_address);
// Preserve the BSP's allocation, geometry/resource addresses and render data.
// Only the single instanced collision-physics record consumed by actual
// TU1.5 171120/24FF70 is repacked in its existing reserved 0xA0-byte bank.
std::vector<std::byte> normalize_mcc_structure_bsp_physics(
    std::span<const std::byte> payload, std::uint32_t source_address);
enum class MccUnitPhysicsKind { Biped, Creature, Vehicle };
// Preserve the complete object/unit definition and its allocation. Compact
// only the embedded shape banks consumed by actual1CAF40 /20D810. Original
// constructors must bind their runtime-only slots before gameplay admission.
std::vector<std::byte> normalize_mcc_unit_physics(
    std::span<const std::byte> payload, std::uint32_t source_address, MccUnitPhysicsKind);
}
