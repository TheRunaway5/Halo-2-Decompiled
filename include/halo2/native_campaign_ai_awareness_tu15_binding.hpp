#pragma once
#include "halo2/physics_world_runtime.hpp"
#include <stdexcept>
namespace halo2 {
// Only genuine nested2AF0F0/2AF13F caller frames are admitted. Their final
// pre-CALL moves/pushes leave the typed input values in EDX/ECX or EAX/ECX;
// the binding needs no argument-memory prepeek or invented fresh FP context.
bool native_campaign_ai_awareness_tu15_owns_entry(std::uint32_t);
bool native_campaign_ai_awareness_tu15_owns_call(const SourcePhysicsContactRuntimeCall &);
class NativeCampaignAiAwarenessReturnRequiredTu15:public std::runtime_error {
    std::uint32_t target_;
public:
    explicit NativeCampaignAiAwarenessReturnRequiredTu15(std::uint32_t);
    std::uint32_t target()const noexcept{return target_;}
};
void native_campaign_ai_awareness_tu15_run(SourcePhysicsContactMemory &,
    const SourcePhysicsContactRuntimeCall &,SourcePhysicsWorldRuntimeAbi &,
    const SourcePhysicsContactRecordObserver & = {});
std::uint64_t native_campaign_ai_awareness_calls_tu15();
}
