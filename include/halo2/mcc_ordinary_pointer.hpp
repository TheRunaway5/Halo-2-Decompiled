#pragma once
#include "halo2/tags/types.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <vector>

namespace halo2 {
// This is a serialized metadata address, never a narrowed host pointer. A
// child descriptor may reside in another declared allocation through MCC's
// shared banks, so field_address is absolute rather than root-relative.
struct MccOrdinaryPointerField {
    std::uint32_t field_address{}, address{}, bytes{};
};
enum class MccOrdinaryPointerFlavor { Mcc, Xbox };
using MccOrdinaryPointerRead =
    std::function<std::span<const std::byte>(std::uint32_t,std::size_t)>;
using MccOrdinaryClosedNativeRoot = std::function<bool(std::uint32_t,std::size_t)>;
// Traverses only pinned named fields. No input is modified. nullopt means the
// group has no admitted schema; a supported but invalid graph throws. MCC
// bipd/crea/vehi must already have passed normalize_mcc_unit_physics; sbsp must
// have passed normalize_mcc_structure_bsp_physics. The Xbox flavor is reserved
// for the existing preserved native payloads and returns nullopt: their library
// fixup map is authoritative. Runtime slots reject bound nonzero values.
// Scenario BSP resource mapping addresses are supplied separately by canonical
// resource delivery; expression string offsets are relative and preserved.
std::optional<std::vector<MccOrdinaryPointerField>>
ordinary_mcc_pointer_fields(TagGroup, std::uint32_t root_address,
    const MccOrdinaryPointerRead&,
    MccOrdinaryPointerFlavor flavor=MccOrdinaryPointerFlavor::Mcc,
    // Only consulted for matg's embedded Rasterizer Data record. The caller
    // must recognize an exact authenticated native-library root whose entire
    // pointer map is handled separately; it cannot bypass other child banks.
    const MccOrdinaryClosedNativeRoot& closed_native_root={});
} // namespace halo2
