#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
bool native_campaign_screen_timed_tu15_owns_entry(std::uint32_t);
// Ordinary void1FC10 (six lazy words, focus inXMM0;RET24), pointer-return1FD00
// (noargs;RET). Decode public arguments once; preserve callee-saved/live FP
// values and marshal persistent hardware FP status plus actual return/SP.
// No dead volatile registers, flags or private scratch are reconstructed.
void native_campaign_screen_timed_tu15_run(SourcePhysicsContactMemory &,
    std::uint32_t,SourcePhysicsWorldRuntimeAbi &,
    const SourcePhysicsContactRecordObserver & = {});
struct NativeCampaignScreenTimedActivityTu15 {std::uint64_t dof_setup{},update{};};
NativeCampaignScreenTimedActivityTu15 native_campaign_screen_timed_activity_tu15();
}
