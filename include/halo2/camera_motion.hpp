#pragma once
#include "halo2/math.hpp"
#include <array>

namespace halo2 {
// Position and orientation channels of the retail observer. Derivatives survive
// camera_set so overlapping commands in mainmenu's flythrough stay smooth.
struct CameraMotion {
  Vec3 position, forward{1, 0, 0}, up{0, 0, 1};
  std::array<float, 6> velocity{}, acceleration{};
  void step(Vec3 target, Vec3 target_forward, Vec3 target_up,
            float remaining_seconds, float dt);
};
} // namespace halo2
