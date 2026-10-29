#pragma once
#include <cstdint>

namespace halo2 {
struct NativeCampaignAudioFp {
    std::uint16_t control_word=0x27f,status{};
    std::uint32_t mxcsr=0x1f80;
};
// Views retain their actual world storage. Reads are deliberately lazy: a
// publication observer may change a subsequent record or the voice count.
class NativeCampaignClassFades {
public:
    virtual ~NativeCampaignClassFades()=default;
    virtual std::uint32_t remaining_seconds(unsigned)const=0;
    virtual std::uint32_t target_gain(unsigned)const=0;
    virtual std::uint32_t current_gain(unsigned)const=0;
    virtual void current_gain(unsigned,std::uint32_t,bool interpolated)=0;
    virtual void remaining_seconds(unsigned,std::uint32_t,bool interpolated)=0;
};
// Complete TU1.5 228310: 54 sound classes, original single-precision operation
// order, nonpositive/unordered delta gate and unordered remaining-time branch.
// There are no residual calls, machine registers or instruction dispatcher.
void native_campaign_advance_class_fades(NativeCampaignClassFades&,
    std::uint32_t elapsed_seconds_bits,NativeCampaignAudioFp&);

enum class NativeCampaignRouteWrite { clear,available,category,claimed,first_selector,second_selector };
class NativeCampaignVoiceRoutes {
public:
    virtual ~NativeCampaignVoiceRoutes()=default;
    virtual std::int16_t voice_capacity()const=0;
    virtual std::int16_t voice_count()const=0;
    virtual std::uint8_t voice_flags(unsigned)const=0;
    virtual std::uint8_t source_voice(unsigned)const=0;
    virtual std::int16_t selector(unsigned)const=0;
    // Signed word offsets preserve the original signed three-bit category and
    // signed source-voice indexing. The backing owner decides valid extents.
    virtual std::uint32_t word(std::int32_t)const=0;
    virtual void word(std::int32_t,std::uint32_t,NativeCampaignRouteWrite)=0;
};
// Complete TU1.5 225D70 with the ordinary compiled caller's clear direction
// flag. Builds category/availability masks and copies six retained selectors.
void native_campaign_build_voice_routes(NativeCampaignVoiceRoutes&);
} // namespace halo2
