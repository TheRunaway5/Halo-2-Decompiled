#pragma once
#include "halo2/sound_startup.hpp"

namespace halo2 {
struct SoundPromotionConstants {
  std::uint32_t milliseconds_per_second_bits;
  std::uint32_t seconds_per_millisecond_bits;
  std::uint32_t one_bits;
};
// These are retained mutable owners, not loader defaults or host-device
// availability. The caller must own the actual serialized/mutable ugh rows.
struct SoundPromotionState {
  std::uint32_t system,tag_table,current_ugh,options;
};
struct SoundPromotionServices {
  std::function<std::span<std::byte>(std::uint32_t,std::size_t)> memory;
  // Actual host tick service; the source owner alone writes +80/+84.
  std::function<std::uint32_t()> tick_milliseconds;
  // Whole12D600 mutable global/control query. Required only on the original
  // campaign fade-in branch; no default false or source-field substitutions.
  std::function<bool()> fade_inhibited;
};
// Complete128610. The raw outputDWORD is a numeric promotion selector, never
// a replacement sound datum. All ordered writes, including the intermediate
// negative counter write before clamping, are retained. No RNG/admission.
std::int32_t source_sound_promote(const SoundPromotionState &,std::uint32_t sound,
    std::uint32_t output_address,const SoundPromotionConstants &,
    const SoundPromotionServices &,const SoundStartupObserver & = {});
// Complete128140 clock consumer. Real map activation126DE0 owns initial clock
// origin; this function does not reset it or declare the source available.
// Ordinary NULL-options and actual map buffers are independently measured;
// alternate fade branches retain mandatory source inputs and dependency.
void source_sound_promotion_clock(const SoundPromotionState &,
    const SoundPromotionConstants &,const SoundPromotionServices &,
    const SoundStartupObserver & = {});
} // namespace halo2
