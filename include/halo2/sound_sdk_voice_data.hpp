#pragma once
#include "halo2/sound_sdk_calculate.hpp"

namespace halo2 {
struct SoundSdkDistanceParameters {
  std::uint32_t rolloff_bits,min_distance_bits,max_distance_bits;
  std::uint32_t curve_identity,curve_count,distance_bits,clip_at_maximum;
};
using SoundSdkCurveReader=std::function<std::uint32_t(std::uint32_t)>;
// Complete original37DCA5,37DDB6,37DE46,37E112 and38368B numeric owners,
// including their conversion/logarithm/filter callees. Curve identities must
// resolve retained original words; never cast them to native pointers.
std::uint32_t source_sound_sdk_distance(const SoundSdkDistanceParameters &,
    const SoundSdkVoiceConstants &,const SoundDriverFloatingPointContext &,
    const SoundSdkCurveReader & = {});
std::uint32_t source_sound_sdk_cone_volume(std::uint32_t inner,std::uint32_t outer,
    std::uint32_t volume,std::uint32_t angle_bits,const SoundSdkVoiceConstants &,
    const SoundDriverFloatingPointContext &);
std::uint32_t source_sound_sdk_doppler(std::uint32_t distance_factor_bits,
    std::uint32_t doppler_factor_bits,std::uint32_t velocity_bits,
    const SoundSdkVoiceConstants &,const SoundDriverFloatingPointContext &);
std::array<std::uint32_t,2> source_sound_sdk_rear_widen(std::uint32_t azimuth_bits,
    std::uint32_t elevation_bits);
std::array<std::uint32_t,4> source_sound_sdk_environment(std::uint32_t distance_bits,
    std::uint32_t rolloff_bits,std::uint32_t frequency_bits,
    const std::array<std::uint32_t,10> &source,const SoundSdkVoiceConstants &,
    const SoundDriverFloatingPointContext &);
std::uint32_t source_sound_sdk_gain(std::uint32_t,const SoundSdkVoiceConstants &,
    const SoundDriverFloatingPointContext &);
std::uint32_t source_sound_sdk_volume(std::uint32_t,const SoundSdkVoiceConstants &,
    const SoundDriverFloatingPointContext &);

struct SoundSdkVoiceDataServices {
  std::function<std::span<std::byte>(std::uint32_t,std::size_t)> memory;
  // Real SDK state synchronization and current initialized algorithm dispatch.
  // Execute once under the actual lock and release it on errors. Reached
  // dispatch services are mandatory; this owner supplies no replacement mode.
  std::function<void(const std::function<void()> &)> under_state_lock;
  std::function<std::uint32_t(const SoundSdkDistanceParameters &)> distance;
  std::function<std::uint32_t(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> cone_volume;
  std::function<SoundSdkFrontRear(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> front_rear;
  std::function<std::array<std::uint32_t,2>(std::uint32_t,std::uint32_t)> rear_widen;
  std::function<std::uint32_t(std::uint32_t,std::uint32_t,std::uint32_t)> doppler;
};
// Complete37AE57 CPU parameter/store order. Retained identities and actual SDK
// ownership are required. Observer PCs use base original instruction identities.
// This does not publish native device commands or establish audible DSP parity.
void source_sound_sdk_voice_data(std::uint32_t listener,std::uint32_t source,
    std::uint32_t listener_environment,std::uint32_t source_environment,
    std::uint32_t voice_data,const SoundSdkVoiceConstants &,
    const SoundDriverFloatingPointContext &,const SoundSdkVoiceDataServices &,
    const SoundDriverStartupObserver & = {});
} // namespace halo2
