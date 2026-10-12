#pragma once
#include "halo2/sound_sdk_device.hpp"

namespace halo2 {
struct SoundSdkReferenceBinding {
  std::uint32_t vtable_identity;
  std::uint32_t method_identity;
};
struct SoundSdkConstructionData {
  std::uint32_t singleton_address, speaker_override_address, speaker_config_address, busy_address;
  std::uint32_t device_vtable, device_data_vtable, buffer_base_vtable, buffer_vtable;
  std::uint32_t voice_base_vtable, voice_vtable;
  std::uint32_t plain_reference_method, synchronized_reference_method;
  std::array<SoundSdkReferenceBinding,8> references;
  std::array<std::uint32_t,15> listener_defaults;
  std::array<std::uint32_t,12> environment_defaults;
};
struct SoundSdkConstructionServices {
  SoundSdkDeviceMemory memory;
  // Actual platform state-access owner, required only by synchronized AddRef.
  std::function<bool()> acquire_state_access;
  std::function<void()> release_state_access;
  // Original3739FA setting owner. Called only for an invalid explicit override.
  std::function<std::uint32_t()> saved_audio_setting;
};
// Original virtual AddRef379D0C/37A14F and nullable wrapper37B16F.
// Dispatch uses the selected image's retained table identities. Unknown tables
// stop instead of assuming an object layout or casting an original pointer.
std::uint32_t source_sound_sdk_add_reference(std::uint32_t identity,
    const SoundSdkConstructionData &, const SoundSdkConstructionServices &,
    const SoundDriverStartupObserver & = {});
std::uint32_t source_sound_sdk_retain(std::uint32_t identity,
    const SoundSdkConstructionData &, const SoundSdkConstructionServices &,
    const SoundDriverStartupObserver & = {});
// Complete retained constructors on caller-owned, preallocated memory. These
// preserve bytes the source does not initialize. Allocation, failure cleanup,
// hardware initialization and Release are separate owners, not supplied here.
std::uint32_t source_sound_sdk_construct_device(std::uint32_t identity,
    const SoundSdkConstructionData &, const SoundSdkConstructionServices &,
    const SoundDriverStartupObserver & = {});
std::uint32_t source_sound_sdk_construct_device_data(std::uint32_t identity,
    const SoundSdkConstructionData &, const SoundSdkConstructionServices &,
    const SoundDriverStartupObserver & = {});
std::uint32_t source_sound_sdk_construct_buffer(std::uint32_t identity, std::uint32_t device_identity,
    const SoundSdkConstructionData &, const SoundSdkConstructionServices &,
    const SoundDriverStartupObserver & = {});
std::uint32_t source_sound_sdk_construct_voice(std::uint32_t identity, std::uint32_t hardware_device_identity,
    std::uint32_t data_identity, const SoundSdkConstructionData &, const SoundSdkConstructionServices &,
    const SoundDriverStartupObserver & = {});
void source_sound_sdk_bind_buffer(std::uint32_t identity, std::uint32_t voice_identity,
    std::uint32_t data_identity, const SoundSdkConstructionData &, const SoundSdkConstructionServices &,
    const SoundDriverStartupObserver & = {});
// Complete37A110; the second payload read follows the first destination store.
void source_sound_sdk_initialize_pair(std::uint32_t identity, std::uint32_t payload_identity,
    const SoundSdkConstructionServices &, const SoundDriverStartupObserver & = {});
} // namespace halo2
