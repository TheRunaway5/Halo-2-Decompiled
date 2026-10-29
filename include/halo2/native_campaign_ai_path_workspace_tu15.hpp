#pragma once
#include "halo2/physics_contact_record.hpp"
namespace halo2 {
// Lazy authored data records, not a CPU register packet. The main navigation
// record has20 DWORDs; optional policy/transport records have7. Implementations
// may use typed host records or the retained byte-layout adapter below.
class NativeCampaignAiPathWordsTu15 {
public:
    virtual ~NativeCampaignAiPathWordsTu15()=default;
    virtual std::uint32_t word(unsigned index)const=0;
};
class NativeCampaignAiPathMemoryWordsTu15 final:public NativeCampaignAiPathWordsTu15 {
    const SourcePhysicsContactMemory &memory_;std::uint32_t address_;
public:
    NativeCampaignAiPathMemoryWordsTu15(const SourcePhysicsContactMemory &memory,std::uint32_t address):memory_(memory),address_(address){}
    std::uint32_t word(unsigned index)const override;
};
// Complete27B010 initialization of an actual leased navigation workspace.
// Main/policy/transport inputs remain lazy across publications and may alias
// the workspace. This does not allocate, search, or supply path success.
void native_campaign_ai_path_workspace_initialize_tu15(SourcePhysicsContactMemory &,
    std::uint32_t workspace,const NativeCampaignAiPathWordsTu15 &main,
    const NativeCampaignAiPathWordsTu15 *policy,const NativeCampaignAiPathWordsTu15 *transport,
    std::uint32_t application_context,const SourcePhysicsContactRecordObserver & = {});
}
