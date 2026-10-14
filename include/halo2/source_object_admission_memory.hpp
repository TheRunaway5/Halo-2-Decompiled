#pragma once
#include "halo2/source_object_admission.hpp"

namespace halo2 {
// Current object data-array header and the two literal source cluster-list
// head/data-array pairs. The caller selects the correct title-update globals.
struct SourceObjectAdmissionMemory {
    std::uint32_t objects{},collidable_heads{},collidable_entries{},
        noncollidable_heads{},noncollidable_entries{};
    std::function<std::span<const std::byte>(std::uint32_t,std::size_t)> read;
};
// Read-only admission over current original-layout banks. Retains actual head
// links, orphan entries and full salted handles; no re-registration or sorted
// scene reconstruction. Query lifetime/epoch and rendering remain caller owned.
SourceObjectAdmission source_object_admission_memory(const SourcePortalVisibilitySet&,
    const SourceObjectAdmissionMemory&,const SourceObjectAdmissionInput&,
    std::span<const std::array<std::uint32_t,4>> prior_associations={});
}
