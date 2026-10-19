#pragma once
#include "halo2/campaign_frame_clock.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>

namespace halo2 {
// A native render submission may contain several original game ticks. A
// script's capture request must reach its GPU consumer before a later tick
// replaces that scene. Retain the unused, already admitted budget; never
// change a script sleep, manufacture a game tick, or count it twice.
class CampaignCaptureCarry {
public:
  CampaignFrameTime admit(CampaignFrameTime current,bool paused) {
    if(paused || !ticks_)return current;
    if(current.ticks<0 || ticks_>std::numeric_limits<int>::max()-current.ticks)
      throw std::invalid_argument("Deferred campaign tick budget overflows");
    current.ticks+=ticks_;
    current.elapsed=float(double(current.elapsed)+seconds_);
    if(!std::isfinite(current.elapsed))
      throw std::invalid_argument("Deferred campaign elapsed time overflows");
    clear();return current;
  }
  void defer(const CampaignFrameTime &frame,int completed,int remaining) {
    if(remaining<0 || completed<0 || remaining!=frame.ticks-completed)
      throw std::invalid_argument("Deferred campaign capture lost its tick budget");
    const auto seconds=double(frame.elapsed)-accepted_campaign_frame_elapsed(frame.elapsed,frame.ticks,completed);
    ticks_=remaining;seconds_=seconds;
  }
  void clear() noexcept {ticks_=0;seconds_=0;}
  int pending_ticks()const noexcept{return ticks_;}
private:
  int ticks_{};
  double seconds_{};
};

// Shared by the production caller and the scheduling regression. Admission
// and step own the original lifecycle/restore decisions. Only the actual
// pending capture consumer can request an intervening render submission.
template<class Admitted,class Step,class CapturePending>
void campaign_frame_ticks(int &remaining,Admitted admitted,Step step,CapturePending capture_pending) {
  while(remaining>0 && admitted()) {
    --remaining;step();
    if(capture_pending())break;
  }
}
}
