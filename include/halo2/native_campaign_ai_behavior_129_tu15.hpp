#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Behavior129 initialization1CACD0. Stack actor (unused), state pointer;
// clear state+C DWORD, return (state & FFFFFF00)|1 in EAX, RET8. GP/XMM/FP
// and flags otherwise retain their input values. Reads precede aliased stores.
//1CACB0 selects the original sentinel from state+10, RETC.1CAE70 publishes
// the actor's facing mode for property kinds1/2, RET8. These two leaves retain
// complete GP/XMM/FP outputs; arithmetic flags are caller scratch.
void native_campaign_ai_behavior_129_call_tu15(SourcePhysicsContactMemory &,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &,
    const SourcePhysicsContactRecordObserver & = {});
}
