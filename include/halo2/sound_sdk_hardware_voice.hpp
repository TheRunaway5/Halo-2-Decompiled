#pragma once
#include "halo2/sound_sdk_device.hpp"

namespace halo2 {
struct SoundSdkHardwareVoiceBindings {
  // Selected original mutable cell holding the actual voice-mapping backing.
  std::uint32_t voice_mapping_pointer_address;
};
struct SoundSdkHardwareVoiceServices {
  SoundSdkDeviceServices device;
  // Actual complete3812E0 owner and its reached algorithm/device services.
  // The caller passes zero source/parameter overrides, as in original381AA9.
  std::function<void(std::uint32_t)> apply_hrtf;
};
// Complete37E737. Actual device counts and slot table are bounded retained
// memory; nested device synchronization is mandatory. No slot is fabricated.
std::uint32_t source_sound_sdk_allocate_voice_slots(std::uint32_t device_identity,
    std::uint32_t voice_identity,const SoundSdkDeviceServices &,
    const SoundDriverStartupObserver & = {});
// Complete380AE6 caller scratch packing. Only the second output is a retained
// address; the first is actual caller scratch, as in original381AA9.
void source_sound_sdk_hardware_bin_commands(std::uint32_t voice_identity,
    std::span<std::uint32_t,1> first_output,std::uint32_t second_output_identity,
    const SoundSdkDeviceMemory &,const SoundDriverStartupObserver & = {});
// Complete381AA9 activation and reached381A92/381A74/3808E4/3808BE.
// Gain and pitch use their complete native owners. Real device synchronization,
// FIFO/device application, mapping backing and reached HRTF remain mandatory.
// This is CPU/device-command behavior, not a native MCPX DSP implementation.
std::uint32_t source_sound_sdk_activate_hardware_voice(std::uint32_t voice_identity,
    const SoundSdkHardwareVoiceBindings &,const SoundSdkHardwareVoiceServices &,
    const SoundDriverStartupObserver & = {});
} // namespace halo2
