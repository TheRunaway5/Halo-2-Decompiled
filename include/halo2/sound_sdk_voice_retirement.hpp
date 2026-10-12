#pragma once
#include "halo2/sound_sdk_notifications.hpp"

namespace halo2 {
struct SoundSdkVoiceRetirementBindings {
  std::uint32_t group_register_table,force_counter_address;
  std::uint32_t slot_mapping_pointer_address,voice_mapping_pointer_address;
  std::uint32_t voice_vtable,status_method,arm_method;
};
struct SoundSdkVoiceRetirementServices {
  SoundSdkNotificationServices notifications;
  // Reached MCPX status/register reads and original KeStallExecutionProcessor
  // /KeInsertQueueDpc calls. The providers retain actual readiness/results.
  std::function<std::uint32_t(std::uint32_t,unsigned)> device_read;
  std::function<void(std::uint32_t)> stall_execution;
  std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> insert_queue_dpc;
  // Complete381EF8 common stop owner, mandatory only when status reaches it.
  std::function<void(std::uint32_t,std::uint32_t)> stop_voice;
};
// Complete384A36 gate, with original FIFO/status/mapping/stall semantics.
// Its ECX argument is unused in the original. No native readiness is invented.
std::uint32_t source_sound_sdk_gate_voice_device(std::uint32_t enable,
    const SoundSdkVoiceRetirementBindings &,const SoundSdkVoiceRetirementServices &,
    const SoundDriverStartupObserver & = {});
// Complete380EE1 active-list and slot-map removal. Synchronization belongs to
// its original caller. Device register reads/writes remain mandatory services.
void source_sound_sdk_unlink_active_voice(std::uint32_t voice,std::uint32_t queue_idle,
    const SoundSdkVoiceRetirementBindings &,const SoundSdkVoiceRetirementServices &,
    const SoundDriverStartupObserver & = {});
// Complete3829F7 mapping-status consumer, including reached notification arm.
std::uint32_t source_sound_sdk_voice_mapping_status(std::uint32_t voice,
    const SoundSdkVoiceRetirementBindings &,const SoundSdkVoiceRetirementServices &,
    const SoundDriverStartupObserver & = {});
// Complete3810BD force owner composes the actual gate/unlink/status CPU owners.
// This neither supplies native DSP completion nor skips a reached stop owner.
void source_sound_sdk_force_pending_voice(std::uint32_t voice,
    const SoundSdkVoiceRetirementBindings &,const SoundSdkVoiceRetirementServices &,
    const SoundDriverStartupObserver & = {});
} // namespace halo2
