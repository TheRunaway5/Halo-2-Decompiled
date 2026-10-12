#pragma once
#include "halo2/sound_sdk_device.hpp"

namespace halo2 {
struct SoundSdkSamplePageBindings {
  std::uint32_t vtable_identity,available_count_address;
};
struct SoundSdkSamplePageServices {
  SoundSdkDeviceServices device;
  // Actual37D952 allocation owner; zero is an actual allocation failure.
  std::function<std::uint32_t(std::uint32_t,std::uint32_t,std::uint32_t)> allocate;
  // Actual ordinal175/173 platform owners. Original identities are never
  // cast to host addresses. The bool is original unlock (false pins pages).
  std::function<void(std::uint32_t,std::uint32_t,bool)> lock_pages;
  std::function<std::uint32_t(std::uint32_t)> physical_address;
};
// Complete38025A/38029C retained manager construction and pool initialization.
std::uint32_t source_sound_sdk_construct_sample_pages(std::uint32_t manager,
    const SoundSdkSamplePageBindings &,const SoundSdkDeviceMemory &,
    const SoundDriverStartupObserver & = {});
std::uint32_t source_sound_sdk_initialize_sample_pages(std::uint32_t manager,
    std::uint32_t pages,const SoundSdkSamplePageServices &,
    const SoundDriverStartupObserver & = {});
// Complete3806BE and reached380327/379C60/3801FD pool owners. Existing
// mappings retain their reference; new mappings pin/translate/map real pages.
std::uint32_t source_sound_sdk_acquire_sample_pages(std::uint32_t manager,
    std::uint32_t buffer,std::uint32_t bytes,const SoundSdkSamplePageServices &,
    const SoundDriverStartupObserver & = {});
// Complete380419. Platform mapping, real synchronization, FIFO and device
// application are mandatory services. This CPU owner produces no PCM output.
void source_sound_sdk_map_sample_pages(std::uint32_t first_index,
    std::uint32_t buffer,std::uint32_t pages,const SoundSdkSamplePageServices &);
// Complete38085D/38074D/3803F8/37E612/380486. Final release merges free
// neighbors under the real lock and unlocks the original page extent afterward.
void source_sound_sdk_release_sample_pages(std::uint32_t manager,std::uint32_t entry,
    const SoundSdkSamplePageServices &,const SoundDriverStartupObserver & = {});
} // namespace halo2
