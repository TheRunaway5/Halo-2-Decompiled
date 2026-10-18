#pragma once
#include "halo2/cache.hpp"
#include <functional>

namespace halo2 {
struct MccGameplayReferenceIdentity {
    TagHandle handle;
    TagGroup group;
    std::string_view name;
};
struct MccGameplayReferenceAccess {
    // All views must remain valid until this call finishes. The converter
    // neither grows a bank nor borrows a dependency from another directory.
    std::function<std::span<std::byte>(std::uint32_t,std::size_t)> bytes;
    std::function<std::optional<MccGameplayReferenceIdentity>(TagHandle)> resolve;
    std::function<std::optional<MccGameplayReferenceIdentity>(TagGroup,std::string_view)> find;
};
enum class MccGameplayReferenceStatus {
    Converted, AlreadyEquivalent, AuthoredTupleUnrecognized, RequiredDependencyUnavailable
};
struct MccGameplayReferenceField {
    std::uint32_t address{};
    TagReference before{},after{};
    TagGroup required_group;
    std::string_view required_name;
};
struct MccGameplayReferenceReceipt {
    std::string_view profile;
    MccGameplayReferenceStatus status{};
    std::array<MccGameplayReferenceField,2> fields{};
    std::uint8_t field_count{};
};
// Restore only explicit named retail dependency tuples. A full salt, group
// and path round trip is required before publishing a local handle. Unknown
// authored tuples are preserved. Missing local targets are reported without
// writing any field; resource-bank presence alone never supplies a handle.
// This restores dependency identity, not equivalence of the target's payload.
std::optional<MccGameplayReferenceReceipt> convert_mcc_gameplay_references(
    TagGroup,std::string_view name,std::uint32_t root,const MccGameplayReferenceAccess&);

struct MccMultiplayerWeaponTableReceipt {
    MccGameplayReferenceStatus status{};
    std::uint32_t count_address{},entries_address{},before_count{},after_count{};
};
// The original option decoder indexes an 18-entry table. Restore only the
// complete known MCC 20-entry identity tuple, including its literal null row.
// Local full salted handles are retained; unknown/modded tuples are preserved.
std::optional<MccMultiplayerWeaponTableReceipt> convert_mcc_multiplayer_weapon_table(
    TagGroup,std::string_view name,std::uint32_t root,const MccGameplayReferenceAccess&);

struct MccBspInstanceTransformReceipt {
    MccGameplayReferenceStatus status{};
    std::uint32_t address{};
};
// Restore the authenticated earthcity_3 instance-zero transform only. Names,
// bounding spheres, collision prototypes and other instance fields are retained.
std::optional<MccBspInstanceTransformReceipt> convert_mcc_bsp_instance_transform(
    TagGroup,std::string_view name,std::uint32_t root,
    const std::function<std::span<std::byte>(std::uint32_t,std::size_t)>&);
} // namespace halo2
