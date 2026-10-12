#pragma once
#include "halo2/cache.hpp"

namespace halo2 {
struct SoundPitchRange {
  std::array<std::byte,10> interval{};
  std::int16_t permutations{};
};
// Original 1269B0/126A40/21B180 inputs. These are resident metadata and
// caller-owned runtime state, not scheduler admission or an audio device.
struct SoundVariationDefinition {
  std::uint32_t datum{};
  std::array<std::byte,20> sound{};
  std::array<std::byte,56> playback{};
  std::array<std::byte,92> sound_class{};
  std::array<std::byte,12> range{};
  std::array<std::byte,20> scale{};
  std::vector<SoundPitchRange> pitch_ranges;
  std::vector<std::array<std::byte,16>> permutations;
  std::vector<std::byte> used;
};
struct SoundRangeState {
  std::int8_t last{}, forced{};
  std::uint32_t used{};
};
struct SoundRngStore {
  std::uint32_t instruction{}, before{}, after{};
};
struct SoundLocalRandom {
  explicit SoundLocalRandom(std::uint32_t supplied_seed) : seed(supplied_seed) {}
  std::uint32_t seed;
  std::vector<SoundRngStore> stores;
  std::uint16_t advance(std::uint32_t instruction);
};
SoundRangeState imported_sound_range_state(const SoundVariationDefinition &);
float source_sound_gain(const SoundVariationDefinition &, SoundLocalRandom &);
double source_sound_pitch(const SoundVariationDefinition &, SoundLocalRandom &);
// Original1279D0 caller control and21B060 range selection. Control also
// interpolates the authored linear-volume bounds in126930; it is not a
// multiplier applied to the upper dB bound.
float source_sound_control_pitch(const SoundVariationDefinition &,float base,float control);
float source_sound_control_gain(const SoundVariationDefinition &,float control);
std::int16_t source_sound_pitch_range(std::span<const SoundPitchRange>,
                                     std::int16_t previous,float pitch);
std::int16_t source_sound_permutation(const SoundVariationDefinition &,
                                     SoundRangeState &, SoundLocalRandom &);
} // namespace halo2
