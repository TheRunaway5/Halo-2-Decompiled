#pragma once
#include "halo2/mcc_gameplay_reference.hpp"

namespace halo2 {
struct MccBspInstanceBoundsField {
    std::uint32_t address{},before_bits{},after_bits{};
    std::uint16_t instance{};
    std::uint8_t component{}; // Stored center x/y/z, radius at instance+3C..48.
};
struct MccBspInstanceBoundsReceipt {
    MccGameplayReferenceStatus status{};
    std::array<MccBspInstanceBoundsField,49> fields{};
    std::uint8_t field_count{};
};
// Restore only the complete authenticated earthcity_3 stored-sphere tuple.
// All 243 instances and 43 definition headers must match. The sole accepted
// instance-zero transform difference is its exact independently verified
// before/after tuple; this converter preserves that transform as supplied.
// Unknown, custom and mixed banks are unchanged; the full postimage is
// idempotent. No sphere is recomputed and no reference file is opened.
// The caller supplies the actual local full-salt Tag, plus checked mutable
// views of its allocation that remain stable through the complete call.
std::optional<MccBspInstanceBoundsReceipt> convert_mcc_bsp_instance_bounds(
    const Tag& owner,
    const std::function<std::span<std::byte>(std::uint32_t,std::size_t)>&);
} // namespace halo2
