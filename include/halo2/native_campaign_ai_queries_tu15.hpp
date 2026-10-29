#pragma once
#include <cstdint>

namespace halo2 {
// These services expose the real actor/property/query records. They may fail
// when backing is absent; NONE and wrapped null pointers are source decisions.
class NativeCampaignAiThreatAccessTu15 {
public:
    virtual ~NativeCampaignAiThreatAccessTu15() = default;
    virtual std::uint32_t actor_property(std::uint32_t actor) = 0;
    virtual std::uint32_t property_query(std::uint32_t property) = 0;
    virtual bool bind_query(std::uint32_t query) = 0;
    virtual std::int16_t property_state() = 0;
    virtual std::uint8_t query_blocked() = 0;
};
// Original1BE5B0: choose the authored behavior score, without running a CPU
// state machine or supplying a missing perception/query outcome.
std::uint32_t native_campaign_ai_threat_tu15(
    NativeCampaignAiThreatAccessTu15 &, std::uint32_t actor);

class NativeCampaignAiResetAccessTu15 {
public:
    virtual ~NativeCampaignAiResetAccessTu15() = default;
    virtual bool marked() = 0;
    virtual void reset() = 0;
};
// Original1BAE40: preserve an unmarked record only when requested. The reset
// value is read from its actual live owner, rather than guessed as a sentinel.
void native_campaign_ai_reset_tu15(NativeCampaignAiResetAccessTu15 &,
    bool preserve_unmarked);
}
