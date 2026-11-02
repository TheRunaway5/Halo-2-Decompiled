#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <stdexcept>

namespace halo2 {
// The original private visited mask has32 bits; its real clump pool has20
// records. Malformed references outside that private capacity require the
// original private-frame contract, rather than an invented traversal result.
class NativeCampaignAiAwarenessPrivateStateRequiredTu15:public std::runtime_error {
    std::uint32_t instruction_;
public:
    explicit NativeCampaignAiAwarenessPrivateStateRequiredTu15(std::uint32_t);
    std::uint32_t instruction()const noexcept{return instruction_;}
};
// Native27D7D0/27DA40: update existing perception links only. These commands
// never allocate, perform visibility tests, or manufacture a perceived object.
// The caller supplies its retained FP policy/status and actual live x87 depth.
// Masked FP exceptions and at most7 older x87 values are admitted; the older
// values themselves are not consumed or changed by either algorithm.
// Scoped host preservation covers CW/SW/TAG, all80 register-value bytes and
// MXCSR. Host FIP/FOP/FDP bookkeeping is outside that contract: independent
// no-engine probes observe asynchronous changes to it on the qualified host.
void native_campaign_ai_magically_see_tu15(SourcePhysicsContactMemory &,
    std::uint32_t observers,std::uint32_t targets,NativeTu15PhysicsFrameFp &,
    unsigned occupied_x87,const SourcePhysicsContactRecordObserver & = {});
void native_campaign_ai_magically_see_object_tu15(SourcePhysicsContactMemory &,
    std::uint32_t observers,std::uint32_t object,NativeTu15PhysicsFrameFp &,
    unsigned occupied_x87,const SourcePhysicsContactRecordObserver & = {});
}
