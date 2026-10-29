#pragma once
#include "halo2/physics_contact_record.hpp"
#include <stdexcept>

namespace halo2 {
class NativeCampaignAiResourceProtectionTu15;
// A real required lifetime owner, not a successful substitute or lifted call.
class NativeCampaignAiPathArenaRequiredOwnerTu15 final:public std::runtime_error {
    std::uint32_t entry_,identity_;
public:
    NativeCampaignAiPathArenaRequiredOwnerTu15(std::uint32_t entry,std::uint32_t identity);
    std::uint32_t entry()const noexcept{return entry_;}
    std::uint32_t identity()const noexcept{return identity_;}
};
// Actual233120 delete=false profile used by both1498E0 call sites.
void native_campaign_ai_path_allocator_retire_tu15(SourcePhysicsContactMemory &,
    std::uint32_t allocator,const SourcePhysicsContactRecordObserver & = {});
// Original149680: rebuild the retained scratch allocator over actual51DF44.
// No host allocation or invented backing is supplied.
void native_campaign_ai_path_arena_restore_tu15(SourcePhysicsContactMemory &,
    const SourcePhysicsContactRecordObserver & = {});
// Original1498E0, with actual233120(arg0) native destructor dispatch. A real
// secondary allocation still requires130030; that missing native owner throws
// at its actual call phase after preceding source publications.
void native_campaign_ai_path_arena_retire_tu15(SourcePhysicsContactMemory &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete secondary-retirement profile with its required actual page owner.
// GPU-associated resources still require their real device retirement owner.
void native_campaign_ai_path_arena_retire_tu15(SourcePhysicsContactMemory &,
    const NativeCampaignAiResourceProtectionTu15 &,
    const SourcePhysicsContactRecordObserver & = {});
// Actual two-descriptor workspace lease/release ownership. Zero is the source
// refusal result; none of these operations initializes or searches a graph.
std::uint32_t native_campaign_ai_path_workspace_acquire_tu15(SourcePhysicsContactMemory &,
    const SourcePhysicsContactRecordObserver & = {});
std::uint32_t native_campaign_ai_path_workspace_acquire_tu15(SourcePhysicsContactMemory &,
    const NativeCampaignAiResourceProtectionTu15 &,
    const SourcePhysicsContactRecordObserver & = {});
void native_campaign_ai_path_workspace_release_tu15(SourcePhysicsContactMemory &,
    std::uint32_t workspace,const SourcePhysicsContactRecordObserver & = {});
}
