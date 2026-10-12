#pragma once
#include "halo2/sound_driver_startup.hpp"

namespace halo2 {
using SoundSdkDeviceMemory =
    std::function<std::span<std::byte>(std::uint32_t,std::size_t)>;
struct SoundSdkDeviceServices {
  SoundSdkDeviceMemory memory;
  // Real device synchronization; execute each body exactly once, including
  // nested pitch-list calls. Release the actual lock on errors as well.
  std::function<void(const std::function<void()> &)> under_device_lock;
  std::function<std::uint32_t()> fifo_available;
  // Original MCPX command identities describe device requests. The CPU owner
  // does not cast them to host pointers or manufacture DSP completion.
  std::function<void(std::uint32_t,std::uint32_t)> device_write;
};
// Complete380B97 CPU helper. Only the three words belonging to each reached
// voice are written; other caller scratch words retain their previous values.
// Its caller has nine words of backing (three arrays of three). A zero count
// would reach original DIV-by-zero; counts exceeding that backing stop here.
void source_sound_sdk_gain_commands(std::uint32_t voice_identity,
    std::span<std::uint32_t,9> output,const SoundSdkDeviceMemory &);
// Complete380DEB integer pitch packing, including parent retained parameters.
std::uint32_t source_sound_sdk_pitch_command(std::uint32_t voice_identity,
    const SoundSdkDeviceMemory &);
// Complete381CE4/381D7C/381710 CPU owners. Original synchronization, FIFO
// readiness and device application remain mandatory reached services.
std::uint32_t source_sound_sdk_gain_apply(std::uint32_t voice_identity,
    const SoundSdkDeviceServices &);
std::uint32_t source_sound_sdk_pitch_apply(std::uint32_t voice_identity,
    const SoundSdkDeviceServices &);
std::uint32_t source_sound_sdk_filter_apply(std::uint32_t voice_identity,
    std::uint32_t filter_override,const SoundSdkDeviceServices &,
    const SoundDriverStartupObserver & = {});
} // namespace halo2
