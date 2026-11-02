#include "halo2/effect_presentation_step.hpp"

namespace halo2 {
void catch_up_effect_presentation(NativeWorldEffects &world,
                                  NativeMuzzleEffects &first_person,
                                  std::uint32_t current, float seconds,
                                  const EffectPresentationTickSounds &sounds) {
  const auto world_tick = world.completed_tick();
  const auto fp_tick = first_person.completed_tick();
  // The first received snapshot establishes both lifetime baselines. It does
  // not prove that its retained old journal entries were received as events.
  if (!world_tick && !fp_tick)
    return;
  if (!world_tick || !fp_tick || *world_tick != *fp_tick)
    throw FormatError("Effect catch-up owners do not share a completed tick");
  if (current < *world_tick)
    throw FormatError("Effect catch-up reversed its completed tick");
  for (auto tick = *world_tick + 1; tick < current; ++tick) {
    world.advance(tick, seconds);
    if (sounds.world)
      sounds.world(std::uint32_t(tick));
    first_person.advance(std::uint32_t(tick), seconds);
    if (sounds.first_person)
      sounds.first_person(std::uint32_t(tick));
  }
}
} // namespace halo2
