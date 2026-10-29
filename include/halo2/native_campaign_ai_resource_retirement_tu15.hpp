#pragma once
#include "halo2/physics_contact_record.hpp"
#include <stdexcept>

namespace halo2 {
// Required platform page ownership. This operation changes permissions on the
// actual retained allocation; it does not return a game or allocation result.
class NativeCampaignAiResourceProtectionTu15 {
public:
    virtual ~NativeCampaignAiResourceProtectionTu15() = default;
    virtual void protect_pages(std::uint32_t address, std::uint32_t bytes,
                               std::uint32_t protection) const = 0;
};
class NativeCampaignAiResourceRetirementRequiredOwnerTu15 final : public std::runtime_error {
    std::uint32_t entry_, identity_;
public:
    NativeCampaignAiResourceRetirementRequiredOwnerTu15(std::uint32_t entry,
                                                       std::uint32_t identity);
    std::uint32_t entry() const noexcept { return entry_; }
    std::uint32_t identity() const noexcept { return identity_; }
};
// Actual12FD10 resource callback. The scratch allocation producer leaves the
// association DWORD zero. Associated GPU resources require their real fence/
// device retirement owner at12FD40; this function never supplies completion.
void native_campaign_ai_unassociated_resource_retire_tu15(SourcePhysicsContactMemory &,
    std::uint32_t datum, const SourcePhysicsContactRecordObserver & = {});
// Actual140310 list removal, including the live delete callback and generic
// data-array deletion. The observer receives stores owned here; the shared
// native16E8A0 owner publishes through SourceMemory directly.
void native_campaign_ai_resource_metadata_retire_tu15(SourcePhysicsContactMemory &,
    std::uint32_t container, std::uint32_t datum,
    const SourcePhysicsContactRecordObserver & = {});
// Actual130030 backing retirement, with2DB4CA native page protection. The
// allocation is retained: the original changes permissions and metadata here,
// rather than releasing the physical pages to the operating system.
void native_campaign_ai_resource_backing_retire_tu15(SourcePhysicsContactMemory &,
    std::uint32_t user, const NativeCampaignAiResourceProtectionTu15 &,
    const SourcePhysicsContactRecordObserver & = {});
}
