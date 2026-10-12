#pragma once
#include "halo2/sound_driver_parameters.hpp"
#include "halo2/sound_startup.hpp"

namespace halo2 {
struct SoundSpatialState {
  std::uint32_t voices,sources,system,driver,shared_voices,tag_table,current_matg;
  std::uint32_t current_local_index_address;
};
// Literal operands of the paired original CPU owners, not host policy.
struct SoundSpatialConstants {
  std::uint32_t one_bits,minimum_gain_bits,linear_db_scale_bits;
};
struct SoundSpatialServices {
  std::function<std::span<std::byte>(std::uint32_t,std::size_t)> memory;
  // Whole original222F10 driver owner. The nine words are its exact36-byte
  // request, before its SDK-axis permutation, thresholds and environment work.
  // A missing provider stops at the call; no platform result is supplied.
  std::function<void(std::int32_t,const std::array<std::uint32_t,9> &)> driver_update;
};
// Complete21B900 numeric gain conversion, also used by world-distance output.
// It owns no listener, admission flag, device state or playback handle.
std::uint32_t source_sound_spatial_gain_db(std::uint32_t linear_gain_bits,
    const SoundSpatialConstants &,const SoundDriverFloatingPointContext &);
// Complete original12AEE0 CPU owner, including220100 shared-voice update,
//12C330 position selection,143E70 inverse transform and21B900 gain conversion.
// position_backing is the actual caller's12-byte private vector backing; an
// unsupported original location type leaves those bytes in place. Source
// readiness, local bindings, listeners, pools and driver must already be real.
// This owner neither creates a spatial device nor implements the DSP/mixer.
void source_sound_spatial_update(const SoundSpatialState &,std::int16_t voice_index,
    std::uint32_t attenuation_address,std::span<std::byte,12> position_backing,
    const SoundSpatialConstants &,const SoundDriverFloatingPointContext &,
    const SoundSpatialServices &,const SoundStartupObserver & = {});
} // namespace halo2
