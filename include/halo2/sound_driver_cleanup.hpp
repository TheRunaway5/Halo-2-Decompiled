#pragma once
#include "halo2/sound_driver_owners.hpp"

namespace halo2 {
// Literal current source globals and retained cache, supplied by their
// runtime owners. Pointer cells are distinct from device identities.
struct SoundDriverCleanupBindings {
  std::uint32_t driver_pointer_address;
  std::uint32_t cache_address;
  std::uint32_t hardware_queue_pointer_address;
  std::uint32_t static_queue_address;
};
enum class SoundDriverCleanupRelease { Buffer, Device };
struct SoundDriverCleanupReleaseRequest {
  SoundDriverCleanupRelease operation;
  std::uint32_t identity;
};
struct SoundDriverCleanupServices {
  SoundDriverOwnersServices owners;
  std::function<void(const SoundDriverCleanupReleaseRequest &)> native_release;
};
// Original220BF0/2254F0 entry, directly callable without manufacturing a
// startup failure. Real nonempty methods/releases remain mandatory services.
void source_sound_driver_cleanup(const SoundDriverCleanupBindings &,
    const SoundDriverCleanupServices &,const SoundDriverStartupObserver & = {});
} // namespace halo2
