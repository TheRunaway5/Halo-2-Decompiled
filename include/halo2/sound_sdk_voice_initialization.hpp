#pragma once
#include "halo2/sound_sdk_device.hpp"
#include "halo2/sound_driver_parameters.hpp"
#include <vector>
namespace halo2 {
struct SoundSdkImmutableRegion {
  std::uint32_t identity;
  std::vector<std::byte> bytes;
};
struct SoundSdkVoiceInitializationData {
  std::array<std::uint32_t,18> source_defaults;
  std::array<std::uint32_t,9> environment_defaults;
  std::array<std::uint32_t,4> pcm_guid,adpcm_guid;
  std::uint32_t default_format_identity,spatial_bins_identity,parent_bins_identity,channel_bins_table_identity;
  std::uint32_t unsigned_correction_bits,frequency_reciprocal_bits;
  // Original per-image bin headers, pointer table, records and default format.
  // These include relocated pointers, so their bytes are imported per image.
  std::vector<SoundSdkImmutableRegion> immutable;
};
struct SoundSdkVoiceInitializationServices {
  SoundSdkDeviceMemory memory;
  // Complete original SDK allocation/free owners backed by real platform
  // allocations. Return0 only for actual allocation failure; no fake identity.
  std::function<std::uint32_t(std::uint32_t,std::uint32_t,std::uint32_t)> allocate;
  std::function<void(std::uint32_t)> release_allocation;
  std::function<void(const std::function<void()> &)> under_state_lock;
  std::function<void(const std::function<void()> &)> under_irql_guard;
};
// Complete37CEF1/37CDCA and reached format/mix-bin/buffer CPU owners.
// The caller owns an actual E0B voice-data object. Original allocation/lock
// providers are mandatory when reached. Hardware voice init is separate.
std::uint32_t source_sound_sdk_initialize_voice_data(std::uint32_t identity,std::uint32_t descriptor_identity,
    const SoundSdkVoiceInitializationData &,const SoundDriverFloatingPointContext &,
    const SoundSdkVoiceInitializationServices &,const SoundDriverStartupObserver & = {});
// Complete37C825,37DA87/37DA3E/37DA17/37D9EE,379F7A and3801DF.
std::uint32_t source_sound_sdk_voice_format(std::uint32_t identity,std::uint32_t format_identity,
    std::uint32_t default_bins,const SoundSdkVoiceInitializationData &,
    const SoundDriverFloatingPointContext &,const SoundSdkVoiceInitializationServices &,
    const SoundDriverStartupObserver & = {});
// Complete37BC89 with reached37A500; zero selects the source's default bins.
void source_sound_sdk_voice_mix_bins(std::uint32_t identity,std::uint32_t bins_identity,
    const SoundSdkVoiceInitializationData &,const SoundSdkVoiceInitializationServices &,
    const SoundDriverStartupObserver & = {});
// Complete37C8AB/37C1B3. Existing owned allocation is freed by its actual
// owner; borrowed backing is retained. Source lifetime/cursor writes are exact.
std::uint32_t source_sound_sdk_voice_buffer(std::uint32_t identity,std::uint32_t buffer_identity,
    std::uint32_t bytes,const SoundSdkVoiceInitializationData &,
    const SoundSdkVoiceInitializationServices &,const SoundDriverStartupObserver & = {});
} // namespace halo2
