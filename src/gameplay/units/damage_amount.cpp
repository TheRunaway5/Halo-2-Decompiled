#include "halo2/damage_amount.hpp"
namespace halo2 {
float damage_amount(float lower, float selected_upper, float fraction,
                    float event_scale, float multiplayer_scale) {
  const float low = (1.f - fraction) * lower;
  const float high = selected_upper * fraction;
  const float scaled = (low + high) * event_scale;
  return scaled * multiplayer_scale;
}
float damage_strength_scale(std::uint8_t strength) {
  switch (strength) {
  case 1: return .75f;
  case 2: return .5f;
  case 3: return .25f;
  default: return 1.f;
  }
}
}
