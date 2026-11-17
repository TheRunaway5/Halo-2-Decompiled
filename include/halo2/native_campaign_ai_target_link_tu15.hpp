#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// 274670: EAX clump-record pointer, stack group datum, void RET4. Reuses the
// qualified perception actor/property owners. Preserves saved GP, caller XMM
// and live x87 values; volatile GP, EFLAGS and private child frames are scratch.
void native_campaign_ai_target_link_call_tu15(SourcePhysicsContactMemory &,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &,
    const SourcePhysicsContactRecordObserver & = {});
}
