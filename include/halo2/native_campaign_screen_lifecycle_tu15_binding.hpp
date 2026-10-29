#pragma once
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
bool native_campaign_screen_lifecycle_tu15_owns_entry(std::uint32_t);
// Void ordinary-caller transport for1FCB0/1FCE0. Invoke semantic kernels, read
// the actual return word, consume4 stack bytes, and preserve registers/FP.
// Dead volatile outputs, EFLAGS and private EDI-save scratch are not rebuilt.
// Reset requires the ordinary ABI's clear direction flag; a refused caller
// stops explicitly rather than entering a lifted compatibility implementation.
void native_campaign_screen_lifecycle_tu15_run(SourcePhysicsContactMemory &,
    std::uint32_t,SourcePhysicsWorldRuntimeAbi &,
    const SourcePhysicsContactRecordObserver & = {});
struct NativeCampaignScreenLifecycleActivityTu15 { std::uint64_t stop{}, capture_complete{}; };
NativeCampaignScreenLifecycleActivityTu15 native_campaign_screen_lifecycle_activity_tu15();
}
