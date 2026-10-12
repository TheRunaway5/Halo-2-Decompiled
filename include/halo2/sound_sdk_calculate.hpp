#pragma once
#include "halo2/sound_sdk_spatial.hpp"
#include "halo2/sound_driver_startup.hpp"

namespace halo2 {
struct SoundSdkCalculateServices {
  std::function<std::span<std::byte>(std::uint32_t,std::size_t)> memory;
  // Actual SDK retained-state synchronization. Execute the body once while
  // holding the real lock, releasing it on errors. No kernel result is supplied.
  std::function<void(const std::function<void()> &)> under_state_lock;
  // Current SDK algorithm dispatch, as established by its real initializer.
  // Reached methods are mandatory; this caller supplies no selected mode or
  // fabricated numerical outputs. Unused LightHRTF up-basis input is retained.
  std::function<SoundSdkPosition(std::uint32_t,const std::array<std::uint32_t,3> &,
      const std::array<std::uint32_t,3> &)> position;
  std::function<SoundSdkAngles(std::uint32_t,const std::array<std::uint32_t,3> &,
      const std::array<std::uint32_t,3> &,const std::array<std::uint32_t,3> &,
      const std::array<std::uint32_t,3> &,std::uint32_t)> angles;
  std::function<std::uint32_t(const std::array<std::uint32_t,3> &,
      const std::array<std::uint32_t,3> &)> cone;
  std::function<std::uint32_t(std::uint32_t,const std::array<std::uint32_t,3> &,
      const std::array<std::uint32_t,3> &,const std::array<std::uint32_t,3> &)> velocity;
};
// Complete37CA0A retained CPU control/store order. Optional store observations
// use base original instruction identities; they are not native code pointers.
// Kernel synchronization and current SDK algorithm ownership remain required
// services. This does not apply voice parameters to a native device/DSP.
void source_sound_sdk_calculate(std::uint32_t listener_identity,
    std::uint32_t source_identity,const SoundDriverFloatingPointContext &,
    const SoundSdkCalculateServices &,const SoundDriverStartupObserver & = {});
} // namespace halo2
