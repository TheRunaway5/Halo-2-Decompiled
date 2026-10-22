#include "halo2/campaign_frame_clock.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <cmath>

namespace halo2 {
float source_campaign_frame_interval_tu15(float seconds) {
  // Actual counters/frequency produce finite intervals. Reject host-only
  // nonfinite inputs; no original NaN/Inf counter result is invented.
  if(!std::isfinite(seconds))throw FormatError("Source incoming frame interval must be finite");
  if(seconds<0.f)return 0.f;
  return seconds>10.f?10.f:seconds;
}
float accepted_campaign_frame_elapsed(float seconds,int requested,int completed) {
  if(!std::isfinite(seconds) || seconds<0 || requested<0 || completed<0 || completed>requested)
    throw FormatError("Campaign frame completion differs from its admitted budget");
  if(completed==requested)return seconds;
  if(!completed)return 0;
  return std::max(0.f,seconds-float(requested-completed)*(1.f/30.f));
}
CampaignFrameClock::CampaignFrameClock(float fraction):leftover_(fraction) {
  if(!std::isfinite(fraction) || fraction<0.f || fraction>=1.f)
    throw FormatError("Campaign frame clock requires its actual fractional tick");
}
CampaignFrameTime CampaignFrameClock::advance(float seconds,bool cinematic,bool paused) {
  if(!std::isfinite(seconds) || seconds<0.f || double(seconds)*30.>=2147483648.)
    throw FormatError("Campaign frame clock requires a finite nonnegative source interval");
  CampaignFrameTime result;
  if(!paused) {
    // MULSS, ADDSS, CVTTSS2SI; each source operation rounds independently.
    const float elapsed_ticks=30.f*seconds;
    const float total=elapsed_ticks+leftover_;
    if(total>=2147483648.f)throw FormatError("Campaign frame interval exceeds the admitted source integer range");
    const auto ticks=std::int32_t(total),maximum=cinematic?150:2;
    if(ticks<=maximum) {
      result.ticks=ticks;result.elapsed=seconds;result.fraction=total-float(ticks);
    } else {
      result.ticks=maximum;result.elapsed=float(maximum)*(1.f/30.f);
    }
  }
  leftover_=result.fraction;return result;
}
}
