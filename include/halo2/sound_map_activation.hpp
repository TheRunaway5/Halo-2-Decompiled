#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
struct SoundMapActivationConstants {
  std::uint32_t one_bits,ambient_transition_bits;
};
// Actual runtime owners, without current-map/device/environment defaults.
// Driver/environment pointers must come from their genuine owning lifecycle.
struct SoundMapActivationState {
  std::uint32_t current_matg_address,sound_system_address,current_ugh_address;
  std::uint32_t driver_address,environment_address,options_address;
  std::uint32_t runtime_environment_parameters_address,queue_budget;
};
struct SoundMapPluginInitializeRequest {
  std::uint32_t plugin_identity,parameters_address,output_buffers_address;
  std::uint8_t saved_output_policy;
  std::span<const std::byte,64> parameters;
};
struct SoundMapPluginEnvironmentRequest {
  std::uint32_t plugin_identity,parameters_address;
  std::uint8_t saved_output_policy;
  // Original2B5260 consumes36 I3DL2 bytes plus its two filter words.
  std::span<const std::byte,44> parameters;
};
struct SoundMapEffectRequest {
  std::uint32_t device_identity,index,offset,parameters_address,flags;
  std::span<const std::byte,8> parameters;
};
struct SoundMapActivationServices {
  std::function<std::span<const std::byte>(std::uint32_t,std::size_t)> read;
  std::function<std::span<std::byte>(std::uint32_t,std::size_t)> writable;
  // Literal original low16 tag-table lookup; no implicit group/salt fixup.
  std::function<std::uint32_t(std::uint16_t)> tag_address;
  std::function<std::uint32_t()> tick_milliseconds;
  // Separately named native adaptations of actual retained plugin/device
  // ownership. Missing or unsupported routing/effects must stop. These must
  // not be no-op/default success replacements for Xbox platform callbacks.
  std::function<void(std::uint32_t)> native_plugin_release;
  std::function<void(const SoundMapPluginInitializeRequest &)> native_plugin_initialize;
  std::function<void(const SoundMapPluginEnvironmentRequest &)> native_plugin_environment;
  std::function<void(const SoundMapEffectRequest &)> native_set_effect_data;
};
struct SoundMapActivationStore {
  std::uint32_t instruction,address;
  unsigned bytes;
  std::uint32_t before,after;
  friend bool operator==(const SoundMapActivationStore &,const SoundMapActivationStore &)=default;
};
using SoundMapActivationObserver=std::function<void(const SoundMapActivationStore &)>;
// Complete represented126DE0 CPU owner,127540/2217E0/2204C0 and192CA0
// ordering. External plugin/device methods remain mandatory real services.
// Source writes+7A before those requests: callers must wait for the complete
// owner return, not treat a partially written flag as successful activation.
// It does not set+78, reset cache/class fades, initialize saved audio settings,
// produce a current matg binding or admit a voice. Missing backing stops.
void source_sound_map_activation(SoundMapActivationState &,
    const SoundMapActivationConstants &,const SoundMapActivationServices &,
    const SoundMapActivationObserver & = {});
} // namespace halo2
