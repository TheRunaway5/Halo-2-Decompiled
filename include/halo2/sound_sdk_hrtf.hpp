#pragma once
#include "halo2/sound_sdk_spatial.hpp"
#include "halo2/sound_driver_startup.hpp"

namespace halo2 {
struct SoundSdkHrtfBindings {
  std::uint32_t speaker_config_address,null_kernel_identity;
};
struct SoundSdkHrtfServices {
  // Actual retained SDK voice/source/parameter/global backing. Original32-bit
  // identities are translated by this owner, never cast to native pointers.
  std::function<std::span<std::byte>(std::uint32_t,std::size_t)> memory;
  // Real device synchronization. The provider must execute the callback once
  // while holding its actual device lock, releasing it on failure as well.
  std::function<void(const std::function<void()> &)> under_device_lock;
  // Mandatory current SDK algorithm dispatch, including its elevation input.
  // This caller does not substitute a selected algorithm or callee success.
  std::function<std::array<std::uint32_t,2>(std::uint32_t,std::uint32_t,std::uint32_t)> select_kernel;
  std::function<std::span<const std::byte,32>(std::uint32_t)> kernel_bytes;
  // Reached native device/DSP provider. These original MCPX command identities
  // are semantic device requests, not host MMIO addresses. No readiness, FIFO
  // return, command completion or DSP result is fabricated by this CPU owner.
  std::function<std::uint32_t()> fifo_available;
  std::function<void(std::uint32_t,std::uint32_t)> device_write;
};
// Complete3812E0 CPU control/store/packing order. Original kernel lock owners,
// current algorithm dispatch and actual FIFO/device application are required
// external services. This is not a software MCPX DSP or live native mixer.
void source_sound_sdk_hrtf_apply(std::uint32_t voice_identity,
    std::uint32_t source_override,std::uint32_t parameters_override,
    const SoundSdkHrtfBindings &,const SoundSdkSpatialConstants &,
    const SoundDriverFloatingPointContext &,const SoundSdkHrtfServices &,
    const SoundDriverStartupObserver & = {});
} // namespace halo2
