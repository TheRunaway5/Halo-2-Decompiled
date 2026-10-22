#pragma once
#include "halo2/pc_settings.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace halo2 {
// PC device adaptation only. The original command, aim, weapon and camera
// owners still consume the resulting angles. Lenses are vertical radians from
// the last submitted world camera, including its current zoom transition.
class PcMouseLook {
public:
  explicit PcMouseLook(float baseline_gain) : baseline_gain_(baseline_gain) {}
  void reset() { scale_=1.f; }
  void projection(float current,float unzoomed) {
    if(!(std::isfinite(current) && current>0 && current<3.141592654f &&
         std::isfinite(unzoomed) && unzoomed>0 && unzoomed<3.141592654f))
      throw std::invalid_argument("Invalid mouse-look camera projection");
    // Match small movements in screen space. Using the submitted projection
    // also accounts for FOV preferences, aspect and intermediate scope lenses.
    // A wider non-scope lens must never accelerate the base mouse response.
    scale_=current>=unzoomed?1.f:
        float(std::tan(double(current)*.5)/std::tan(double(unzoomed)*.5));
  }
  void motion(int x,int y,const PcKeyboardSettings &settings,float &yaw,float &pitch) const {
    const float gain=baseline_gain_*float(settings.sensitivity)/3.f*scale_;
    yaw-=float(x)*gain;
    pitch-=float(y)*gain*(settings.invert?-1.f:1.f);
  }
private:
  float baseline_gain_,scale_=1.f;
};
} // namespace halo2
