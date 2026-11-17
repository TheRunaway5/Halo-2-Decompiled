#pragma once
#include "halo2/physics_world_runtime.hpp"
#include <functional>

namespace halo2 {
// Actual 1B0AD9 CALL EAX of descriptor 48A9D8 slot +40 (1BBEE0, RET 8, AL).
bool native_campaign_ai_area_perform_tu15_owns_call(const SourcePhysicsContactRuntimeCall &);
// protect: the live owner's real page protection (address, bytes, flags).
// translated: the live AI-frame body for an unported callee. It is used only
// when HALO2_REFERENCE_AI_PERFORM_CALLEES=translated selects the reference
// census; production never calls it and stops at the named refusal instead.
void native_campaign_ai_area_perform_tu15_run(SourcePhysicsContactMemory &,
    const SourcePhysicsContactRuntimeCall &,SourcePhysicsWorldRuntimeAbi &,
    const std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> &protect,
    const std::function<void(std::uint32_t,SourcePhysicsWorldRuntimeAbi &)> &translated,
    const SourcePhysicsContactRecordObserver & = {});
// Actual 1B0F56 CALL ECX of descriptor 48A9D8 slot +48 (1BC0A0, RET 8).
bool native_campaign_ai_area_perform_release_tu15_owns_call(const SourcePhysicsContactRuntimeCall &);
void native_campaign_ai_area_perform_release_tu15_run(SourcePhysicsContactMemory &,
    const SourcePhysicsContactRuntimeCall &,SourcePhysicsWorldRuntimeAbi &,
    const SourcePhysicsContactRecordObserver & = {});
std::uint64_t native_campaign_ai_area_perform_calls_tu15();
std::uint64_t native_campaign_ai_area_perform_release_calls_tu15();
}
