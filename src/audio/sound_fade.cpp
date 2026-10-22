#include "halo2/sound_fade.hpp"
#include "halo2/sound_classes.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
namespace halo2 {
namespace {
std::int32_t add(std::int32_t a, std::int32_t b) {
  return std::bit_cast<std::int32_t>(std::uint32_t(a)+std::uint32_t(b));
}
std::int32_t subtract(std::int32_t a, std::int32_t b) {
  return std::bit_cast<std::int32_t>(std::uint32_t(a)-std::uint32_t(b));
}
}
float source_sound_playback_fade(SoundPlaybackFade &fade, std::int32_t time) {
  if (!fade.fading) return 0.f;
  if (fade.start == -1 || fade.end == -1) {
    const auto shift = add(time,1);
    fade.start = add(fade.start,shift); fade.end = add(fade.end,shift);
  }
  const auto latest = std::max(fade.start,fade.end);
  const float a = float(subtract(fade.start,latest)),
              b = float(subtract(fade.end,latest)),
              c = float(subtract(time,latest));
  float t;
  if (!(.0001f > std::abs(double(a)-double(b))))
    t = std::clamp((c-a)/(b-a),0.f,1.f);
  else t = ((a>c) == (fade.start<fade.end)) ? 0.f : 1.f;
  const float lower_db = fade.start<fade.end ? fade.gain_db : -64.f,
              upper_db = fade.start<fade.end ? 0.f : fade.gain_db;
  const float lower = sound_db_to_linear(lower_db);
  float value;
  if (fade.curve == 0) {
    // Original keeps the upper gain on x87 until the final float store.
    const double upper = std::exp(double(std::clamp(upper_db,-64.f,0.f)) *
                                  double(.05f) * double(2.3025851f));
    value = float((upper-double(lower))*double(t)+double(lower));
  } else if (fade.curve == 1) {
    const float upper = sound_db_to_linear(upper_db);
    const float fraction = upper>lower ? float(std::sqrt(double(t))) :
        float(1.0-std::sqrt(1.0-double(t)));
    const float difference = upper-lower;
    const float product = difference*fraction;
    value = product+lower;
  } else return 0.f;
  return sound_linear_to_db(value);
}
void source_sound_request_stop_tu15(SoundPlaybackFade &fade, std::int32_t time) {
  const auto gain = source_sound_playback_fade(fade,time);
  fade = SoundPlaybackFade{0,gain,300,-1,true};
}
} // namespace halo2
