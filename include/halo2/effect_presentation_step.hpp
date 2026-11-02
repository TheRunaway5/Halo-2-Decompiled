#pragma once
#include "halo2/world_effects.hpp"
#include <functional>

namespace halo2 {
struct EffectPresentationTickSounds {
  std::function<void(std::uint32_t)> world;
  std::function<void(std::uint32_t)> first_person;
};
// Finish only the ticks preceding the received current tick. Current world,
// animation/audio and FP phases stay with their original caller. Actual pose
// inputs must already be staged. Each sound sink consumes its genuine queued
// requests before the next consumer can draw from the shared RNG owner.
void catch_up_effect_presentation(NativeWorldEffects &, NativeMuzzleEffects &,
                                  std::uint32_t current_tick,
                                  float tick_seconds,
                                  const EffectPresentationTickSounds &);
} // namespace halo2
