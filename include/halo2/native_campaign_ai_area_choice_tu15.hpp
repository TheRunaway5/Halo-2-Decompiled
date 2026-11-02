#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <exception>
#include <stdexcept>

namespace halo2 {
// Real mathematical quantities owed when a retained score operand is rejected.
// Older caller x87 values stay owned by the caller; no CPU stack is modeled.
struct NativeCampaignAiAreaScorePendingTu15 {
    enum class Stage {Magnitude,DirectionZ,DirectionYAndZScore,DirectionXAndYZScore,DistanceAndFacingScore};
    Stage stage;
    unsigned count;
    std::array<std::array<std::byte,10>,2> quantities;
};
class NativeCampaignAiAreaScorePendingErrorTu15 final:public std::runtime_error {
    NativeCampaignAiAreaScorePendingTu15 pending_;
    std::exception_ptr cause_;
public:
    NativeCampaignAiAreaScorePendingErrorTu15(NativeCampaignAiAreaScorePendingTu15,std::exception_ptr);
    const auto &pending()const noexcept{return pending_;}
    const auto &cause()const noexcept{return cause_;}
};
// Retained caller inputs stay lazy. The actor datum is consumed after the
// actual pool identity, and each candidate/publication resolves its current
// selection record. A numeric caller can retain its real argument words;
// native value callers supply ordinary values without mounting source scratch.
struct NativeCampaignAiAreaChoiceInputsTu15 {
    virtual ~NativeCampaignAiAreaChoiceInputsTu15()=default;
    virtual std::uint32_t actor()const=0;
    virtual std::uint32_t selection()const=0;
};
// Actual1BBBD0 chooses among authored area records, retaining its mask/zone/area
// publication and reset/retry. FP policy and occupied caller temporaries are
// explicit; the masked profile needs two free x87 temporaries while scoring.
bool native_campaign_ai_choose_area_tu15(SourcePhysicsContactMemory &,std::uint32_t actor,
    std::uint32_t selection,NativeTu15PhysicsFrameFp &,unsigned occupied_x87,
    const SourcePhysicsContactRecordObserver & = {});
bool native_campaign_ai_choose_area_tu15(SourcePhysicsContactMemory &,
    const NativeCampaignAiAreaChoiceInputsTu15 &,NativeTu15PhysicsFrameFp &,unsigned occupied_x87,
    const SourcePhysicsContactRecordObserver & = {});
// Clear the real initial-choice record and return its location. The numeric
// wrapper still owns its retained argument-pointer publication at1BBECD.
std::uint32_t native_campaign_ai_prepare_area_choice_tu15(SourcePhysicsContactMemory &,
    std::uint32_t request,const SourcePhysicsContactRecordObserver & = {});
// Actual1BBEC0 starts a choice using the mask at request+C. Its zero store
// precedes all actor/iterator reads, even when no choice is available.
bool native_campaign_ai_begin_area_choice_tu15(SourcePhysicsContactMemory &,std::uint32_t actor,
    std::uint32_t request,NativeTu15PhysicsFrameFp &,unsigned occupied_x87,
    const SourcePhysicsContactRecordObserver & = {});
}
