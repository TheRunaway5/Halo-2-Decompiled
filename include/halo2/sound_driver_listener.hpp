#pragma once
#include "halo2/sound_driver_owners.hpp"

namespace halo2 {
struct SoundDriverListenerConstants {
  std::uint32_t orientation_epsilon_bits,environment_epsilon_bits;
  std::array<std::byte,72> default_environment;
};
enum class SoundDriverListenerOperation { Position, Orientation };
struct SoundDriverListenerRequest {
  SoundDriverListenerOperation operation;
  std::uint32_t device_identity;
  // Literal platform ABI order, including source axis permutation. The
  // final apply word follows three or six raw float words respectively.
  std::span<const std::uint32_t> words;
};
struct SoundDriverListenerServices {
  std::function<std::span<std::byte>(std::uint32_t,std::size_t)> memory;
  std::function<void(const SoundDriverListenerRequest &)> native_operation;
};
// Complete original2228A0/2271A0 CPU owner. The 56-byte input, its pointed
// environment records, current driver pointer cell and real platform methods
// must be owned by the caller; no listener/device activation is generated.
void source_sound_driver_listener_install(std::uint32_t driver_pointer_address,
    std::uint32_t input_address,const SoundDriverListenerConstants &,
    const SoundDriverListenerServices &,const SoundDriverStartupObserver & = {});
} // namespace halo2
