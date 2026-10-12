#pragma once
#include "halo2/sound_sdk_notifications.hpp"

namespace halo2 {
struct SoundSdkVoiceStopServices {
  SoundSdkNotificationServices notifications;
  // Reached complete382DD2 parent buffer stop. The common voice owner cannot
  // replace the parent's cursor/notification/lifecycle work with a flag clear.
  std::function<std::uint32_t(std::uint32_t,std::uint32_t)> stop_buffer;
};
// Complete3811FA pause/mute flags, including37ED0F/37ED36 counted slots.
std::uint32_t source_sound_sdk_voice_pause_flags(std::uint32_t voice,
    std::uint32_t flags,const SoundSdkVoiceStopServices &,
    const SoundDriverStartupObserver & = {});
// Complete381122 envelope stop and original per-slot command order.
void source_sound_sdk_voice_stop_envelope(std::uint32_t voice,
    const SoundSdkVoiceStopServices &,const SoundDriverStartupObserver & = {});
// Complete381EF8 common stop composes actual pause, notification and pending
// owners. Kernel/FIFO/device services and a reached parent stop are mandatory.
void source_sound_sdk_common_voice_stop(std::uint32_t voice,std::uint32_t wait,
    const SoundSdkNotificationBindings &,const SoundSdkVoiceStopServices &,
    const SoundDriverStartupObserver & = {});
} // namespace halo2
