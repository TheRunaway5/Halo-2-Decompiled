#pragma once
#include "halo2/mcc_gameplay_reference.hpp"

namespace halo2 {
struct MccBspCollisionReferenceField {
    std::uint32_t address{},before_bits{},after_bits{};
};
struct MccBspCollisionReferenceReceipt {
    MccGameplayReferenceStatus status{};
    std::array<MccBspCollisionReferenceField,17> fields{};
    std::uint8_t field_count{};
};
// Restore only the authenticated earthcity_3 collision preimage. All main and
// definition collision payloads, including topology, must match as a whole.
// Only 17 float words are written; instance transforms, spheres and every
// other byte are retained. The complete original postimage is idempotent.
// The caller must resolve the local full salted sbsp identity before calling.
// All checked views must remain valid and stable until the call finishes.
std::optional<MccBspCollisionReferenceReceipt> convert_mcc_bsp_collision_reference(
    TagGroup,std::string_view name,std::uint32_t root,
    const std::function<std::span<std::byte>(std::uint32_t,std::size_t)>&);
} // namespace halo2
