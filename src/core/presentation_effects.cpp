#include "halo2/scene.hpp"
#include "halo2/kernels.hpp"
#include <algorithm>
#include <cmath>

namespace halo2 {
ScreenEffects Presentation::effects(float render_fraction) const {
  if (!std::isfinite(render_fraction) || render_fraction < 0 ||
      render_fraction > 1)
    throw FormatError("Invalid screen transition render fraction");
  if (render_fraction == 0)
    return effects();
  const auto phase = [render_fraction](const Transition &transition) {
    return transition.duration > 0
               ? std::clamp((float(transition.elapsed) + render_fraction) /
                                transition.duration,
                            0.f, 1.f)
               : 1.f;
  };
  const auto value = [&](const Transition &transition) {
    return transition.duration <= 0
               ? transition.to
               : transition.from +
                     (transition.to - transition.from) * phase(transition);
  };
  ScreenEffects result{fade_color_, 0, value(letterbox_), value(hud_)};
  if (fade_.to != 1 && fade_.from != 1)
    return result;
  if (!content_.original_data)
    throw FormatError("fade transition requires original game data");
  const float curve =
      fade_curve_fraction(phase(fade_), content_.original_data->fade_curve);
  result.fade = fade_.to == 1 ? curve : 1.f - curve;
  return result;
}
} // namespace halo2
