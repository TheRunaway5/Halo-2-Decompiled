#pragma once
#include "halo2/sound_submit.hpp"
#include "halo2/sound_driver_parameters.hpp"

namespace halo2 {
// Literal operands from the paired whole owners, never mutable driver/map
// state. No fallback values or host-device-derived source policy.
struct SoundCreateConstants {
  std::uint32_t default_extra_gain_bits, minimum_gain_bits, maximum_float_bits;
  std::uint32_t rng_scale_bits, unsigned_conversion_correction_bits;
  std::uint32_t one_bits, upper_gain_bits, db_exponent_scale_bits;
  std::uint32_t db_exponent_log_bits, linear_db_scale_bits, fade_epsilon_bits;
};
struct SoundCreateState {
  std::uint32_t system, tag_table, current_matg, current_ugh, current_scenario;
  std::uint32_t sources, controllers, class_fades, local_rng, environment;
  std::uint32_t cache_pool, delay_scale_operand;
};
struct SoundCreateServices {
  std::function<std::span<std::byte>(std::uint32_t,std::size_t)> memory;
  // Whole192F20 effect identifier query; required only after an actual
  // authored nonempty custom/default effect reaches its original call site.
  std::function<std::uint16_t(std::uint32_t)> effect_identifier;
  // Original21BA00(controller,1)->21C070(owner). No successful default.
  std::function<void(std::uint32_t)> enable_controller;
  // Complete21A960 resource/cache service. Missing service stops after the
  // actual source construction/selection writes, before cache admission.
  std::function<std::uint32_t(std::uint32_t sound,std::uint32_t chunk,
                             std::uint32_t flags)> cache_request;
};
// Complete127750 CPU owner and its gain/pitch/range/permutation/source-record
// helpers. The call itself is an explicit raw ABI; it does not establish
// successful128380 admission, original startup, listener/PVS or playback.
// Mandatory160-byte caller backing retains bytes absent from semantic FP
// requests. Missing dependencies throw at their original phase.
std::uint32_t source_sound_create(const SoundCreateState &,std::uint32_t sound,
    std::uint32_t local_association,std::uint32_t raw_request,
    std::uint32_t promotion_selector,const SoundCreateConstants &,
    const SoundDriverFloatingPointContext &,const SoundCreateServices &,
    const SoundStartupObserver & = {});
} // namespace halo2
