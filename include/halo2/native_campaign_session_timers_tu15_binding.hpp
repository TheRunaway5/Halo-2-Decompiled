#pragma once
#include "halo2/native_campaign_session_timers_tu15.hpp"
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// The four real TU1.5 CALL instructions leave both pushed argument values
// available in held caller registers or a literal. Their valid private pushed
// words, saved-register cells and RET word are disjoint from persistent timer
// storage. No argument-memory prepeek or fresh platform/FP state is created.
bool native_campaign_session_timers_tu15_owns_entry(std::uint32_t);
bool native_campaign_session_timers_tu15_owns_call(const SourcePhysicsContactRuntimeCall &);
class NativeCampaignSessionTimerReturnRequiredTu15 final:public std::runtime_error {
    std::uint32_t target_;
public:
    explicit NativeCampaignSessionTimerReturnRequiredTu15(std::uint32_t);
    std::uint32_t target()const noexcept{return target_;}
};
// EAX is the real typed continuation value, including branches where outer
// parents forward it. ECX/EDX, callee-saved state and the complete FP packet
// are retained. Arithmetic flags are dead in each admitted continuation.
// RET8 reads its real return word only after the data/clock algorithm, then
// advances ESP by12. Faulted private callee scratch-register transport is
// outside this completed-call profile; persistent effects keep their prefix.
void native_campaign_session_timers_tu15_run(SourcePhysicsContactMemory &,
    const SourcePhysicsContactRuntimeCall &,SourcePhysicsWorldRuntimeAbi &,
    const NativeCampaignTransitionClockTu15 &actual_clock_producer,
    const SourcePhysicsContactRecordObserver & = {});
std::uint64_t native_campaign_session_timers_calls_tu15();
}
