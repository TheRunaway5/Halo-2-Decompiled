#pragma once
#include "halo2/sound_sdk_spatial.hpp"
#include "halo2/sound_sdk_construction.hpp"
#include "halo2/sound_sdk_voice_initialization.hpp"
#include "halo2/sound_sdk_hardware_voice.hpp"
#include "halo2/sound_sdk_voice_dispatch.hpp"
#include "halo2/sound_sdk_sample_pages.hpp"
#include "halo2/sound_sdk_voice_attachment.hpp"
#include "halo2/sound_sdk_notifications.hpp"
#include "halo2/sound_sdk_voice_retirement.hpp"
#include <stdexcept>

namespace halo2 {
// Immutable authored SDK data imported from the selected owned executable.
// This resolves original identities to retained bytes; it supplies no device,
// mutable voice state, HRTF parameter transition or platform readiness.
struct SoundSdkSpatialData {
  SoundSdkSpatialConstants constants;
  SoundSdkConeConstants cone_constants;
  SoundSdkVoiceConstants voice_constants;
  SoundSdkConstructionData construction;
  SoundSdkVoiceInitializationData voice_initialization;
  SoundSdkHardwareVoiceBindings hardware_voice;
  SoundSdkVoiceDispatchBindings voice_dispatch;
  SoundSdkSamplePageBindings sample_pages;
  SoundSdkVoiceAttachmentBindings voice_attachment;
  SoundSdkNotificationBindings notifications;
  SoundSdkVoiceRetirementBindings voice_retirement;
  std::uint32_t null_kernel_identity;
  std::array<std::byte,61 * 64> kernels;
  std::array<std::byte,32> null_kernel;

  std::span<const std::byte,32> kernel(std::uint32_t identity) const {
    if (identity == null_kernel_identity) return null_kernel;
    if (constants.kernel_table_b != constants.kernel_table_a + 32u ||
        identity < constants.kernel_table_a)
      throw std::out_of_range("Original SDK kernel identity unavailable");
    const auto offset = std::uint64_t(identity) - constants.kernel_table_a;
    if (offset % 32 || offset + 32 > kernels.size())
      throw std::out_of_range("Original SDK kernel identity unavailable");
    return std::span<const std::byte,32>(kernels.data() + offset, 32);
  }
};
} // namespace halo2
