#pragma once
#include "halo2/source_campaign_environment_frame_tu15.hpp"
#include "halo2/bsp_atmosphere.hpp"

namespace halo2 {
struct SourceCampaignEnvironmentViewTu15 {
    SourceCampaignEnvironmentFrameTu15 source;
    BspFogSample fog;
    std::int32_t leaf=-1;
    bool location_found{};
};
// The completed source environment is the renderer's fog input. This reads
// existing immutable fog/patchy resources only; it never resamples a sky or
// evaluates an independent approximation of the live mixer.
BspFogSample source_campaign_environment_sample_tu15(
    const SourceCampaignEnvironmentFrameTu15 &,const BspAtmosphere &,
    std::int32_t cluster);
// Actual14CDB0 plus2B700/2B7A2..2B811 retained-location policy precede the
// already authenticated14DE90/1310F0/full publication boundary. Failed point
// queries keep a valid previous pair, but never publish failed local repairs
// into the persistent original per-view history.
SourceCampaignEnvironmentViewTu15 source_campaign_environment_view_tu15(
    PhysicsLiveWorldTu15 &,const BspAtmosphere &,Vec3 eye,Vec3 forward,
    float near_distance,float configured_far_distance,unsigned local_view=0);
}
