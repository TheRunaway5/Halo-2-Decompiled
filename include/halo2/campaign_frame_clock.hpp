#pragma once
#include <cstdint>

namespace halo2 {
struct CampaignFrameTime {
  std::int32_t ticks{};
  float elapsed{},fraction{};
};
// TU1.5 149160, admitted solo/30Hz/scale1 branch. Cinematic interface+5
// admits five seconds of ticks; ordinary solo frames admit two. This is
// independent of native free-camera/input smoothing and audio callback time.
// Network synchronization and speed ramps require their source owners.
class CampaignFrameClock {
public:
  explicit CampaignFrameClock(float source_fraction=0.f);
  CampaignFrameTime advance(float seconds,bool cinematic,bool paused=false);
  float fraction()const{return leftover_;}
private:
  float leftover_{};
};
//12DBF0 scalar boundary after the counter ratio is rounded tobinary32:
// keep signedzero/subnormals, replace negative intervals with+0 and cap10s.
// Native wallcounter selection/frequency are external inputs, not sampled here.
float source_campaign_frame_interval_tu15(float binary32_seconds);
//13AA50 shortens149160's elapsed value if the ordinary game-update owner
// stops before consuming the requested tick budget. Render clock13A9B0 adds
// this accepted float interval to its independent double accumulator.
float accepted_campaign_frame_elapsed(float seconds,int requested,int completed);
}
