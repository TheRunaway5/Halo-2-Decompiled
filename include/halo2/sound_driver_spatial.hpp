#pragma once
#include "halo2/sound_driver_parameters.hpp"
#include "halo2/sound_startup.hpp"
#include <optional>

namespace halo2 {
struct SoundDriverSpatialBindings {
  std::uint32_t driver_pointer_address,filter_frequency_address,filter_gain_address;
};
struct SoundDriverSpatialConstants {
  std::uint32_t position_epsilon_bits,environment_epsilon_bits,zero_bits,one_bits;
  std::uint32_t quarter_bits,volume_log_scale_bits,room_scale_bits;
  std::uint32_t frequency_min_bits,frequency_max_bits,frequency_sample_period_bits;
  std::uint32_t frequency_pi_bits,frequency_log_scale_bits;
  std::uint32_t filter_gain_max_bits,filter_gain_exponent_bits,filter_gain_scale_bits;
};
enum class SoundDriverSpatialOperation { Position,I3dl2Source,Filter };
struct SoundDriverSpatialRequest {
  SoundDriverSpatialOperation operation;
  std::uint32_t buffer_identity;
  std::optional<std::uint32_t> apply;
  // Exact SDK argument/payload words:3 position words in original x,z,y
  // order,9 I3DL2 words or6 filter words. No native stack pointer is serialized.
  std::span<const std::uint32_t> words;
};
struct SoundDriverSpatialServices {
  std::function<std::span<std::byte>(std::uint32_t,std::size_t)> memory;
  // Required real SDK/platform provider. The original caller ignores HRESULT;
  // this callback therefore supplies no invented original return or success.
  std::function<void(const SoundDriverSpatialRequest &)> native_operation;
};
// Complete222F10/2232F0 CPU owners and220380/2203E0 filter calculations.
// Retained driver/voice identity and controls must be real. Reached SDK methods
// are mandatory. This does not implement the SDK's HRTF/mix-bin/DSP backend.
void source_sound_driver_spatial_update(const SoundDriverSpatialBindings &,
    std::int32_t voice_index,const std::array<std::uint32_t,9> &parameters,
    const SoundDriverSpatialConstants &,const SoundDriverParameterConstants &,
    const SoundDriverFloatingPointContext &,const SoundDriverSpatialServices &,
    const SoundStartupObserver & = {});
// Whole original filter helper ABIs, independently measured against originals.
std::uint32_t source_sound_filter_frequency(std::uint32_t frequency_bits,
    const SoundDriverSpatialConstants &,const SoundDriverFloatingPointContext &);
std::uint32_t source_sound_filter_gain(std::uint32_t gain_bits,
    const SoundDriverSpatialConstants &,const SoundDriverFloatingPointContext &);
} // namespace halo2
