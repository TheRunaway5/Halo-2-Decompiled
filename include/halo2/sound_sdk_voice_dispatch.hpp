#pragma once
#include "halo2/sound_sdk_hardware_voice.hpp"
#include "halo2/sound_sdk_construction.hpp"

namespace halo2 {
struct SoundSdkVoiceDispatchBindings {
  std::array<SoundSdkReferenceBinding,2> format_methods;
  std::uint32_t base_format_method,reconfigure_method;
};
struct SoundSdkVoiceDispatchServices {
  SoundSdkHardwareVoiceServices hardware;
  // Actual remaining382DD2/381E0E/382F96 lifecycle owners. Reached calls
  // require real providers; no stop/drain/release success is manufactured.
  std::function<std::uint32_t(std::uint32_t,std::uint32_t)> stop;
  std::function<void(std::uint32_t)> wait_pending,release_channels;
  // Actual382BE3/382B7E stream/buffer attachment owners. Stream's caller
  // ignores its EAX; buffer attachment's actual HRESULT is propagated.
  std::function<void(std::uint32_t)> attach_stream;
  std::function<std::uint32_t(std::uint32_t)> attach_buffer;
};
// Complete38094B format/channel update and reached device command order.
std::uint32_t source_sound_sdk_voice_format_apply(std::uint32_t voice_identity,
    const SoundSdkDeviceServices &,const SoundDriverStartupObserver & = {});
// Complete380900 dirty marking and selected-original virtual format dispatch.
std::uint32_t source_sound_sdk_voice_mark_format(std::uint32_t voice_identity,
    std::uint32_t changed,const SoundSdkVoiceDispatchBindings &,
    const SoundSdkHardwareVoiceBindings &,const SoundSdkVoiceDispatchServices &,
    const SoundDriverStartupObserver & = {});
// Complete382FE3,382CD4 and382F58 callers. Hardware activation executes its
// complete native CPU owner. Reached remaining lifecycle/attachment owners are
// mandatory services; this does not implement their platform/DMA internals.
std::uint32_t source_sound_sdk_voice_reconfigure(std::uint32_t voice_identity,
    const SoundSdkVoiceDispatchBindings &,const SoundSdkHardwareVoiceBindings &,
    const SoundSdkVoiceDispatchServices &,const SoundDriverStartupObserver & = {});
std::uint32_t source_sound_sdk_voice_prepare(std::uint32_t voice_identity,
    const SoundSdkHardwareVoiceBindings &,const SoundSdkVoiceDispatchServices &,
    const SoundDriverStartupObserver & = {});
std::uint32_t source_sound_sdk_voice_initialize(std::uint32_t voice_identity,
    const SoundSdkVoiceDispatchBindings &,const SoundSdkHardwareVoiceBindings &,
    const SoundSdkVoiceDispatchServices &,const SoundDriverStartupObserver & = {});
} // namespace halo2
