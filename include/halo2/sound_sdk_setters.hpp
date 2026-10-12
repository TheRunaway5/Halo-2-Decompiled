#pragma once
#include "halo2/sound_sdk_commit.hpp"
#include "halo2/sound_driver_spatial.hpp"

namespace halo2 {
struct SoundSdkSetterBindings {std::uint32_t busy_address;};
struct SoundSdkSetterServices {
  SoundSdkDeviceMemory memory;
  // Real379E9E state access: acquire only at the platform's permitted level;
  // return whether a lock was acquired. Release that actual acquisition once.
  // Both providers are required before attempting acquisition.
  std::function<bool()> acquire_state_access;
  std::function<void()> release_state_access;
  // Reached complete CPU owners, not chosen HRESULT/device responses.
  std::function<void(std::uint32_t)> commit;
  std::function<void(std::uint32_t,std::uint32_t)> filter;
};
// Complete public wrappers37C668/37C6E5/37B68B, synchronized callers,
// retained setters37C031/37C0E9 and immediate bridges37A669/37A616.
// interface_identity is the original object's +1C identity, never a host cast.
// Environmental/filter payloads resolve retained words through memory; input
// aliasing and per-word destination reloads follow the original instructions.
// apply bit0 means deferred. Filter applies immediately. These APIs return
// the source's0 or80004005 semantic result, not a fabricated device response.
std::uint32_t source_sound_sdk_set_position(std::uint32_t interface_identity,
    std::array<std::uint32_t,3> position,std::uint32_t apply,
    const SoundSdkSetterBindings &,const SoundDriverFloatingPointContext &,
    const SoundSdkSetterServices &,const SoundDriverStartupObserver & = {});
std::uint32_t source_sound_sdk_set_environment(std::uint32_t interface_identity,
    std::uint32_t payload_identity,std::uint32_t apply,const SoundSdkSetterBindings &,
    const SoundSdkSetterServices &,const SoundDriverStartupObserver & = {});
std::uint32_t source_sound_sdk_set_filter(std::uint32_t interface_identity,
    std::uint32_t payload_identity,const SoundSdkSetterBindings &,
    const SoundSdkSetterServices &);
// Native ABI publication: retain these exact SDK words at the supplied32-bit
// identity throughout one body invocation. Never serialize a native pointer.
using SoundSdkPayloadPublisher=std::function<void(std::span<const std::uint32_t>,
    const std::function<void(std::uint32_t)> &)>;
// Connect the actual driver's operation boundary to the complete SDK setters.
// Position words already have the source x,z,y ordering; do not swap them again.
// The initialized retained buffer/state owners and payload publication remain
// mandatory services. This bridge does not allocate SDK objects or install DSP.
std::uint32_t source_sound_sdk_driver_operation(const SoundDriverSpatialRequest &,
    const SoundSdkSetterBindings &,const SoundDriverFloatingPointContext &,
    const SoundSdkSetterServices &,const SoundSdkPayloadPublisher &,
    const SoundDriverStartupObserver & = {});
} // namespace halo2
