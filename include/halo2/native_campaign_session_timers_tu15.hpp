#pragma once
#include "halo2/native_campaign_transition_request_tu15.hpp"

namespace halo2 {
class NativeCampaignSessionTimerClockRequiredTu15 final:public std::runtime_error {
public:
    static constexpr std::uint32_t instruction=0x33b595u;
    std::uint32_t cell;
    explicit NativeCampaignSessionTimerClockRequiredTu15(std::uint32_t);
};
// Complete777E0 data algorithm over its five record groups and actual33B590
// imported millisecond read. The genuine producer contract runs after the
//41B774 pointer read and before the volatile DWORD read, once per reached
// live-clock branch. It must be the existing adjusted/journalled World owner;
// this API creates no clock, timestamp, epoch or gameplay result.
// An absent producer stops at that real phase after prior state4 publication.
// Cached/no-clock branches never require or invoke it. No FP policy is needed.
void native_campaign_session_retire_timers_tu15(SourcePhysicsContactMemory &,
    std::uint32_t record_filter,std::uint32_t slot,
    const NativeCampaignTransitionClockTu15 &actual_clock_producer);
// Same single algorithm, including the actual caller-visible continuation
// value: final-group filter, slot offset or captured/cached milliseconds.
// Those values are derived from the inputs and already-read final-group state;
// no extra retained reads or guest register/scratch model is introduced.
std::uint32_t native_campaign_session_retire_timers_value_tu15(SourcePhysicsContactMemory &,
    std::uint32_t record_filter,std::uint32_t slot,
    const NativeCampaignTransitionClockTu15 &actual_clock_producer,
    const SourcePhysicsContactRecordObserver & = {});
}
