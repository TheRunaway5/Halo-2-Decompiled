#pragma once
#include "halo2/sound_driver_parameters.hpp"
#include <array>

namespace halo2 {
struct SoundSdkSpatialConstants {
  std::uint32_t zero_bits,one_bits,half_bits,position_sentinel_bits;
  std::uint32_t angle_round_bias_bits,half_turn_bits,azimuth_scale_bits;
  std::uint32_t octant_bits,quarter_turn_bits,volume_scale_bits;
  std::array<std::uint32_t,3> mode_one_side,mode_one_forward;
  std::uint32_t kernel_table_a,kernel_table_b;
};
struct SoundSdkPosition {
  std::array<std::uint32_t,3> direction;
  std::uint32_t distance_bits;
};
struct SoundSdkAngles {std::uint32_t azimuth_bits,elevation_bits;};
struct SoundSdkFrontRear {std::int32_t front,rear;};
struct SoundSdkConeConstants {std::uint32_t octant_bits,quarter_turn_bits,angle_scale_bits;};
struct SoundSdkVoiceConstants {
  std::uint32_t zero_bits,one_bits,unsigned_correction_bits;
  std::uint32_t gain_scale_bits,volume_scale_bits;
  std::uint32_t doppler_max_bits,doppler_min_bits,doppler_reciprocal_bits;
  std::uint32_t environment_log_floor_bits,sample_rate_bits,two_pi_bits;
  std::uint32_t filter_scale_bits,db_exponent_scale_bits;
  std::uint64_t one_double_bits;
};

// Complete original SDK numerical owners 37DEBF/380168,37DEF9,37DFE1
// (including37DB3E/379D4B) and37E098. Their semantic outputs are raw SDK
// words, not a native panning policy or a complete DirectSound implementation.
// Unused original arguments and private stack layout are outside these APIs.
SoundSdkPosition source_sound_sdk_position(std::uint32_t mode,
    const std::array<std::uint32_t,3> &listener,
    const std::array<std::uint32_t,3> &source,
    const SoundSdkSpatialConstants &,const SoundDriverFloatingPointContext &);
SoundSdkAngles source_sound_sdk_angles(std::uint32_t mode,
    const std::array<std::uint32_t,3> &forward,
    const std::array<std::uint32_t,3> &side,
    const std::array<std::uint32_t,3> &direction,std::uint32_t distance_bits,
    const SoundSdkSpatialConstants &,const SoundDriverFloatingPointContext &);
SoundSdkFrontRear source_sound_sdk_front_rear(std::uint32_t distance_factor_bits,
    std::uint32_t scale_bits,std::uint32_t azimuth_bits,std::uint32_t enabled,
    const SoundSdkSpatialConstants &,const SoundDriverFloatingPointContext &);
// Original32-bit table identities, including original wrapped arithmetic.
// A reached kernel/DSP provider must resolve actual bytes separately. These
// values must never be cast to native process pointers or used as readiness.
std::array<std::uint32_t,2> source_sound_sdk_kernel(std::uint32_t azimuth_bits,
    std::uint32_t fold_rear,const SoundSdkSpatialConstants &,
    const SoundDriverFloatingPointContext &);
// Complete37DB87 cross product,37DBC4/37DAE0 cone-angle calculation and
//37DC5A projected relative velocity. These preserve original float spills.
std::array<std::uint32_t,3> source_sound_sdk_cross(
    const std::array<std::uint32_t,3> &,const std::array<std::uint32_t,3> &,
    const SoundDriverFloatingPointContext &);
std::uint32_t source_sound_sdk_cone(const std::array<std::uint32_t,3> &,
    const std::array<std::uint32_t,3> &,const SoundSdkConeConstants &,
    const SoundDriverFloatingPointContext &);
std::uint32_t source_sound_sdk_velocity(std::uint32_t mode,
    const std::array<std::uint32_t,3> &listener_velocity,
    const std::array<std::uint32_t,3> &source_velocity,
    const std::array<std::uint32_t,3> &direction,const SoundDriverFloatingPointContext &);
} // namespace halo2
