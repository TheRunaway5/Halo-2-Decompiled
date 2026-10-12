#pragma once
#include "halo2/sound_sdk_device.hpp"

namespace halo2 {
struct SoundSdkNotificationBindings {
  // Selected original derived-voice virtual+20 owner. Base pure-virtual or
  // unknown tables do not acquire a fabricated notification implementation.
  std::uint32_t voice_vtable,cancel_method,arm_method;
};
struct SoundSdkNotificationServices {
  SoundSdkDeviceServices device;
  // Original KeQuerySystemTime128, KeCancelTimer97, KeRemoveQueueDpc137 and
  // KeSetTimer149 platform boundaries. Time is the raw original64-bit ABI.
  std::function<std::uint64_t()> query_system_time;
  std::function<void(std::uint32_t)> cancel_timer,remove_queue_dpc;
  std::function<void(std::uint32_t,std::uint64_t,std::uint32_t)> set_timer;
  // Actual complete3810BD forced-stop owner, mandatory only on timeout.
  std::function<void(std::uint32_t)> force_pending_voice;
};
// Complete37F068/37EB07 cancellation and reached379C60 list detachment.
void source_sound_sdk_cancel_notification(std::uint32_t device,std::uint32_t notification,
    const SoundSdkNotificationServices &,const SoundDriverStartupObserver & = {});
// Complete37F245/37EAE2/37F11A immediate or sorted timed notification arm.
std::uint32_t source_sound_sdk_arm_notification(std::uint32_t device,std::uint32_t notification,
    const SoundSdkNotificationServices &,const SoundDriverStartupObserver & = {});
// Complete382B08/382AD9 voice notification wrappers; original volatile EAX is
// ignored by their callers. Index arithmetic preserves the original32-bit ABI.
void source_sound_sdk_voice_cancel_notification(std::uint32_t voice,std::uint32_t index,
    const SoundSdkNotificationServices &,const SoundDriverStartupObserver & = {});
void source_sound_sdk_voice_arm_notification(std::uint32_t voice,std::uint32_t index,
    std::uint32_t time_low,std::uint32_t time_high,std::uint32_t context,
    const SoundSdkNotificationServices &,const SoundDriverStartupObserver & = {});
// Complete3811A6/381E0E original pending polling and drain. No timeout is
// invented for the drain and no pending flag is cleared without its owner.
void source_sound_sdk_poll_pending_voice(std::uint32_t voice,const SoundSdkNotificationBindings &,
    const SoundSdkNotificationServices &,const SoundDriverStartupObserver & = {});
void source_sound_sdk_wait_pending_voice(std::uint32_t voice,const SoundSdkNotificationBindings &,
    const SoundSdkNotificationServices &,const SoundDriverStartupObserver & = {});
// Complete37F407 mapping flag query. Only literal flag1 reports completion.
std::uint32_t source_sound_sdk_mapping_completed(std::uint32_t descriptor,std::uint32_t index,
    const SoundSdkDeviceMemory &);
} // namespace halo2
