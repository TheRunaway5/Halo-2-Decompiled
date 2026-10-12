#pragma once
#include "halo2/sound_sdk_device.hpp"
#include "halo2/sound_sdk_hrtf.hpp"
#include "halo2/sound_sdk_voice_data.hpp"

namespace halo2 {
struct SoundSdkDispatchServices {
  SoundSdkDeviceMemory memory;
  // Reached source CPU owners, not chosen success/empty device results.
  std::function<void(std::uint32_t)> gain,pitch;
  std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> hrtf;
  std::function<void(std::uint32_t,std::uint32_t)> filter;
};
struct SoundSdkUpdateServices {
  SoundSdkDeviceMemory memory;
  // Actual SDK state lock, including original nested calculation calls.
  std::function<void(const std::function<void()> &)> under_state_lock;
  std::function<void(std::uint32_t,std::uint32_t)> calculate;
  std::function<void(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> voice_data;
  std::function<void(std::uint32_t)> dispatch;
};
// Complete381E2A/382032 CPU caller control, reload and retained-store order.
// The dispatcher returns its final original parameter identity, not a device
// result. Update's kernel return register is outside this semantic API.
std::uint32_t source_sound_sdk_dispatch(std::uint32_t voice_identity,
    const SoundSdkDispatchServices &,const SoundDriverStartupObserver & = {});
void source_sound_sdk_update(std::uint32_t voice_identity,
    const SoundSdkUpdateServices &,const SoundDriverStartupObserver & = {});

struct SoundSdkPipelineServices {
  SoundSdkDeviceMemory memory;
  std::function<void(const std::function<void()> &)> under_state_lock;
  SoundSdkCalculateServices calculate;
  SoundSdkVoiceDataServices voice_data;
  SoundSdkHrtfServices hrtf;
  SoundSdkDeviceServices device;
};
// Connect all verified source CPU owners through one retained memory/state
// owner and the real device lock/FIFO/command provider. Numerical dispatch and
// immutable kernel ownership remain mandatory initialized SDK services. This
// does not allocate SDK voices or replace the native sound platform with DSP.
void source_sound_sdk_spatial_commit(std::uint32_t voice_identity,
    const SoundSdkSpatialConstants &,const SoundSdkVoiceConstants &,
    const SoundSdkHrtfBindings &,const SoundDriverFloatingPointContext &,
    const SoundSdkPipelineServices &,const SoundDriverStartupObserver & = {});
} // namespace halo2
