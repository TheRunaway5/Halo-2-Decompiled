#pragma once
#include "halo2/tags/types.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <vector>

namespace halo2 {
struct MccMapGameplayIdentity {
    TagHandle handle;
    TagGroup group;
    std::string_view name;
    std::uint32_t source_token{}, bytes{};
};
struct MccMapGameplaySource {
    std::string_view scenario_path;
    TagHandle scenario, globals;
    // Resolve the complete salted handle. find returns null for ambiguous
    // names. Both callbacks return only this directory's resident identities.
    std::function<std::optional<MccMapGameplayIdentity>(TagHandle)> resolve;
    std::function<std::optional<MccMapGameplayIdentity>(TagGroup,std::string_view)> find;
    // Directory identities and backing ownership stay immutable through
    // planning, destination resolution, preflight and publication. Preflight
    // rechecks captured bytes; it does not re-resolve a replaced directory.
    // Checked borrowed views must remain valid and unchanged while planning.
    std::function<std::span<const std::byte>(std::uint32_t,std::size_t)> read;
};
enum class MccMapGameplayStatus {
    NoProfile, AuthoredTupleUnrecognized, RequiredDependencyUnavailable,
    InvalidMetadata, Ready, AlreadyEquivalent
};
struct MccMapGameplayWrite {
    std::uint32_t source_token{};
    std::vector<std::byte> before,after;
    std::string field;
};
struct MccMapGameplayGuard {
    std::uint32_t source_token{};
    std::vector<std::byte> bytes;
};
struct MccMapGameplayPlan {
    MccMapGameplayStatus status=MccMapGameplayStatus::NoProfile;
    std::string profile,reason;
    std::vector<MccMapGameplayGuard> guards;
    std::vector<MccMapGameplayWrite> writes;
};
// Authenticated scope: 28 original map contexts (five campaign, 23 multiplayer).
// Campaign Gauss-only conversion requires no resident multiplayer owner. The full
// scenario bank and complete affected owners qualify the map variant. Unknown
// or modified authoring is preserved; filenames never qualify a profile.
// This only plans writes. Call before existing scalar/reference/weapon-table
// conversion changes the donor owners; no Cache layout or host-pointer change.
MccMapGameplayPlan plan_mcc_map_gameplay_vanilla(const MccMapGameplaySource&);

// Independent authenticated Floodzone collision-material component. Unknown
// model/globals authoring stays unchanged without suppressing the map's Gauss
// component. Only two serialized 16-bit global-material indices can change.
MccMapGameplayPlan plan_mcc_material_gameplay_vanilla(const MccMapGameplaySource&);

// Independent authenticated Chief secondary references:90 named action/material
// classes in27 stock contexts. Pass the exact native group-only matg handle.
// Only complete donor, prior seed and union owners qualify. Required sound
// owners follow the still-active rows; union has no inactive-secondary owner.
MccMapGameplayPlan plan_mcc_foot_gameplay_vanilla(const MccMapGameplaySource&,
    TagHandle selected_material_globals);

// Recheck EVERY captured guard and write before committing ANY write. The
// caller must retain exclusive ownership between this check and its bounded,
// nonthrowing copies. Source tokens/serialized descriptors remain 32 bits.
void preflight_mcc_map_gameplay_vanilla(const MccMapGameplayPlan&,
    const std::function<std::span<const std::byte>(std::uint32_t,std::size_t)>&);

// Check all accepted components and cross-component write overlap before any
// publication. Rejected optional components must have no guards or writes;
// InvalidMetadata fails the entire transaction. Borrowed ownership is unchanged.
void preflight_mcc_map_gameplay_vanilla(std::span<const MccMapGameplayPlan>,
    const std::function<std::span<const std::byte>(std::uint32_t,std::size_t)>&);
} // namespace halo2
