#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
// Literal executable operands, in the paired driver authority's field order.
// Source opcode integer pitch bases remain code, not replacement tuning data.
struct SoundDriverParameterConstants {
  std::uint32_t source_pitch_to_driver_pitch_bits;
  std::uint32_t driver_pitch_min_bits, driver_pitch_max_bits;
  std::uint32_t gain_normalized_to_driver_volume_bits;
  std::uint32_t minimum_source_gain_bits, parameter_change_threshold_bits;
  std::uint32_t driver_pitch_change_threshold_bits;
  std::uint32_t driver_pitch_octave_scale_bits, relative_rate_ln2_bits;
  std::uint32_t mono_channel_count_bits, stereo_channel_count_bits;
  std::uint32_t routing_db_ln10_bits, routing_db_scale_bits;
  std::uint32_t routing_linear_db_scale_bits;
  std::uint32_t normalized_parameter_range_epsilon_bits, silent_source_gain_bits;
};
struct SoundDriverFloatingPointContext {
  std::uint16_t x87_control_word;
  std::uint32_t mxcsr;
};
struct SoundDriverMemory {
  // Actual retained driver state; never a serialized pointer/default bank.
  std::uint32_t state;
  std::function<std::span<const std::byte>(std::uint32_t, std::size_t)> read;
};
struct SoundDriverPitchCalculation {
  std::uint32_t clamped_float_bits;
  std::int32_t driver_pitch;
};
struct SoundDriverVolumeCalculation {
  std::uint32_t normalized_bits, scaled_float_bits;
  std::int32_t driver_volume;
};

// Complete12C760: x87 range comparison, separate SSE operations, original
// clamping/degenerate byte comparison. Raw bits preserve nonfinite controls.
std::uint32_t source_sound_normalized_parameter(std::uint32_t low_bits,
    std::uint32_t high_bits, std::uint32_t value_bits, std::uint8_t direction,
    const SoundDriverParameterConstants &, const SoundDriverFloatingPointContext &);
// Original12C434..12C475 numeric distance-gain tail. The caller resolves
// authored/override bounds and floors minimum distance at original epsilon.
// This neither selects/admit listeners nor creates a spatial platform voice.
std::uint32_t source_sound_distance_gain(std::uint32_t minimum_bits,
    std::uint32_t maximum_bits,std::uint32_t distance_bits,
    const SoundDriverParameterConstants &,const SoundDriverFloatingPointContext &);
// Explicit221B90 numeric slices only. These perform no driver method calls,
// parameter-change gating, routing, effects, admission or success callbacks.
SoundDriverPitchCalculation calculate_source_sound_driver_pitch(std::uint32_t rate_index,
    std::uint32_t source_pitch_bits, const SoundDriverParameterConstants &,
    const SoundDriverFloatingPointContext &);
SoundDriverVolumeCalculation calculate_source_sound_driver_volume(std::uint32_t source_gain_bits,
    const SoundDriverParameterConstants &, const SoundDriverFloatingPointContext &);
std::int32_t source_sound_driver_pitch(std::uint32_t rate_index,
    std::uint32_t source_pitch_bits, const SoundDriverParameterConstants &,
    const SoundDriverFloatingPointContext &);
std::int32_t source_sound_driver_volume(std::uint32_t source_gain_bits,
    const SoundDriverParameterConstants &, const SoundDriverFloatingPointContext &);
// Complete221760 numerical calculation with its source-selected stored pitch.
// The memory overload includes original wrapped bank/index addressing.
std::uint32_t source_sound_relative_rate(std::int32_t stored_driver_pitch,
    std::uint32_t rate_index, const SoundDriverParameterConstants &,
    const SoundDriverFloatingPointContext &);
std::uint32_t source_sound_relative_rate(std::uint32_t voice_index,
    std::uint32_t rate_index, const SoundDriverMemory &,
    const SoundDriverParameterConstants &, const SoundDriverFloatingPointContext &);
} // namespace halo2
