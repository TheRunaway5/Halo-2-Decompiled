#include "halo2/adaptive_interpolation.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <cmath>

namespace halo2 {
AdaptiveInterpolationClock::AdaptiveInterpolationClock(AdaptiveInterpolationSettings settings)
    :settings_(settings) {
  const auto &s=settings_;
  if(!(s.tick_seconds>0) || !(s.processing_seconds>=0) || !(s.window_seconds>0) ||
     !(s.maximum_seconds>=s.tick_seconds+s.processing_seconds) || !(s.jitter_gain>0 && s.jitter_gain<=1) ||
     !(s.frame_gain>0 && s.frame_gain<=1) || !(s.slew>=0 && s.slew<1) || !(s.slew_seconds>0) ||
     !(s.snap_seconds>0) || !(s.jitter_scale>=0))
    throw FormatError("Adaptive interpolation settings are out of range");
}
void AdaptiveInterpolationClock::reset() {
  minimum_.clear();maximum_.clear();deficit_.clear();
  have_arrival_=placed_=holding_=advanced_=false;
  newest_tick_={};newest_seconds_=jitter_=frame_=playback_=advanced_at_=underrun_seconds_=0;
  hold_started_=hold_delay_=0;
  arrivals_=underruns_=snaps_=0;
}
void AdaptiveInterpolationClock::receive(std::uint32_t tick,double seconds) {
  if(!std::isfinite(seconds))throw FormatError("Snapshot arrival time is not finite");
  if(have_arrival_ && tick<=newest_tick_)return;
  const double offset=seconds-double(tick)*settings_.tick_seconds;
  // Lateness is measured against the tick after the previous newest one:
  // playback needs some newer snapshot before it passes that tick, so ticks
  // lost or dropped as stale count as waiting time.
  double waited=offset;
  if(have_arrival_) {
    // RFC 3550: deviation of the arrival spacing from the tick spacing.
    const double deviation=(seconds-newest_seconds_)-double(tick-newest_tick_)*settings_.tick_seconds;
    jitter_+=(std::abs(deviation)-jitter_)*settings_.jitter_gain;
    waited=seconds-double(newest_tick_+1u)*settings_.tick_seconds;
  }
  // Sliding-window minimum of the arrival offset (the arrival line) and
  // maximum of the waited lateness (monotonic deques).
  while(!minimum_.empty() && minimum_.back().value>=offset)minimum_.pop_back();
  minimum_.push_back({seconds,offset});
  while(!maximum_.empty() && maximum_.back().value<=waited)maximum_.pop_back();
  maximum_.push_back({seconds,waited});
  while(minimum_.front().seconds<seconds-settings_.window_seconds)minimum_.pop_front();
  while(maximum_.front().seconds<seconds-settings_.window_seconds)maximum_.pop_front();
  have_arrival_=true;newest_tick_=tick;newest_seconds_=seconds;++arrivals_;
}
double AdaptiveInterpolationClock::target_delay()const {
  const auto &s=settings_;
  const double minimum=s.tick_seconds+s.processing_seconds;
  if(!have_arrival_)return minimum;
  const double peak=maximum_.front().value-base();
  const double allowance=std::max(s.jitter_scale*jitter_,peak);
  // Polled lateness includes up to one frame the previous frame never needed.
  // A held underrun proves its own shortfall (gaps whose older snapshots
  // were lost or dropped as stale leave no late arrival to measure).
  const double deficit=deficit_.empty()?0.0:deficit_.front().value;
  return std::clamp(minimum+std::max({0.0,allowance-frame_,deficit}),minimum,s.maximum_seconds);
}
double AdaptiveInterpolationClock::delay(double seconds)const {
  if(!have_arrival_)return 0;
  return seconds-base()-playback_*settings_.tick_seconds;
}
double AdaptiveInterpolationClock::advance(double seconds) {
  if(!have_arrival_)return playback_;
  if(!std::isfinite(seconds))throw FormatError("Presentation time is not finite");
  const auto &s=settings_;
  // An earlier frame time never runs playback backwards.
  if(advanced_ && seconds<advanced_at_)return playback_;
  const double dt=advanced_?seconds-advanced_at_:0.0;
  if(dt>0)frame_+=(std::min(dt,s.tick_seconds)-frame_)*s.frame_gain;
  advanced_=true;advanced_at_=seconds;
  const double target=target_delay();
  // Delay error after a nominal-rate step to this frame.
  const double current=delay(seconds)-dt;
  if(!placed_ || std::abs(current-target)>s.snap_seconds) {
    if(placed_)++snaps_;
    placed_=true;
    playback_=(seconds-base()-target)/s.tick_seconds;
  } else {
    // Positive error: more delay than wanted, so run faster (and vice versa).
    const double rate=1+std::clamp((current-target)/s.slew_seconds,-s.slew,s.slew);
    playback_+=dt*rate/s.tick_seconds;
  }
  const double newest=double(newest_tick_);
  if(playback_>newest) {
    if(!holding_){++underruns_;hold_started_=seconds;hold_delay_=target-(s.tick_seconds+s.processing_seconds);}
    else underrun_seconds_+=dt;
    holding_=true;playback_=newest;
  } else if(holding_) {
    // The hold ended with this frame's snapshot: that much more delay above
    // the minimum would have kept playback moving. Held over the window.
    holding_=false;
    const double needed=hold_delay_+(seconds-hold_started_);
    while(!deficit_.empty() && deficit_.back().value<=needed)deficit_.pop_back();
    deficit_.push_back({seconds,needed});
  }
  while(!deficit_.empty() && deficit_.front().seconds<seconds-s.window_seconds)deficit_.pop_front();
  return playback_;
}
AdaptiveInterpolationClock::Statistics AdaptiveInterpolationClock::statistics(double seconds)const {
  Statistics out;
  out.target=target_delay();out.jitter=jitter_;out.frame=frame_;
  out.underrun_seconds=underrun_seconds_;out.arrivals=arrivals_;out.underruns=underruns_;
  out.snaps=snaps_;out.holding=holding_;
  if(have_arrival_) {
    out.delay=delay(seconds);
    out.peak=maximum_.front().value-base();
    out.deficit=deficit_.empty()?0.0:deficit_.front().value;
  }
  return out;
}
} // namespace halo2
