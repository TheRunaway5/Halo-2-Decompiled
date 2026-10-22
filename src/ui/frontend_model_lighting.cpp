#include "halo2/frontend_model_lighting.hpp"
#include <bit>
#include <cmath>
namespace halo2 {
namespace {
Vec3 direction(float elevation,float azimuth) {
  // Original x87 retains the PC53 degree conversion and trig products until
  // FSTP float, then SSE subtracts each result from positive zero.
  constexpr float radians=std::bit_cast<float>(0x3c8efa35u);
  const double a=double(elevation)*double(radians),b=double(azimuth)*double(radians);
  return {0.f-float(std::cos(a)*std::cos(b)),
          0.f-float(std::cos(a)*std::sin(b)),0.f-float(std::sin(a))};
}
}
void frontend_cinematic_primary(ObjectLightingState &state,float elevation,float azimuth,Vec3 color) {
  state.primary_direction=direction(elevation,azimuth);
  state.primary_color=color;state.shadow_direction=state.primary_direction;
  state.accuracy=state.shadow_opacity=std::bit_cast<float>(0x3f4ccccdu);
}
void frontend_cinematic_secondary(ObjectLightingState &state,float elevation,float azimuth,Vec3 color) {
  state.secondary_direction=direction(elevation,azimuth);state.secondary_color=color;
}
void frontend_cinematic_ambient(ObjectLightingState &state,Vec3 color) {state.ambient=color;}
}
