#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
// Immutable126B20 operands, imported from the selected owned executable.
// The sixteen policy bytes are not device capabilities or a host voice cap.
struct SoundStartupConstants {
  std::uint32_t ambient_transition_bits;
  std::array<std::byte,16> voice_policy;
};
// Retained source globals. Arena backing and allocator identity must be owned
// by the caller; this type does not produce them or infer live-map readiness.
struct SoundStartupState {
  std::uint32_t references,arena_base,arena_used,arena_crc,allocator_identity;
  std::uint32_t system,sources,static_voices,controllers,controller_root;
  std::uint32_t looping,effects,shared_voices;
};
struct SoundStartupServices {
  // Resolve actual retained32-bit source addresses to real owned raw storage.
  std::function<std::span<std::byte>(std::uint32_t,std::size_t)> memory;
  // Complete21A6E0 cache owner. It owns its OS allocations, page fields,
  // physical-arena reservation and protection. No default success is allowed.
  std::function<void(SoundStartupState &)> cache_initialize;
  // Actual source32803E/328505/337A40 inputs, called independently in order.
  // The rand callback must consume the caller's retained CRT stream.
  std::function<std::uint32_t()> utc_seconds,crt_random,tick_milliseconds;
  // Separately named native platform adaptation of2205C0. True requires an
  // actual completed host-owned device/stream constructor. It is not a guest
  // DirectSound return, capability, EEPROM setting or initialized flag.
  std::function<bool(std::span<const std::byte,16>)> native_platform_initialize;
};
struct SoundStartupStore {
  std::uint32_t instruction,address;
  unsigned bytes;
  std::uint32_t before,after;
  friend bool operator==(const SoundStartupStore &,const SoundStartupStore &)=default;
};
using SoundStartupObserver=std::function<void(const SoundStartupStore &)>;
// Complete represented126B20 CPU owner and its21BC50/21C1D0/21F5A0/21FEB0
// constructors, including11E340/16D070/16D0D0/16D220 static pool layout and
// allocation checksum. Actual cache/platform/storage/clock owners are required.
// A missing service throws at that phase and cannot increment references or
// become an original false/success return. Ordinary map activation is separate.
void source_sound_startup(SoundStartupState &,const SoundStartupConstants &,
                         const SoundStartupServices &,
                         const SoundStartupObserver & = {});
// Independently bounded126C9D..126D39 CPU fragment for the retained post-device
// state. This does not execute/check a device constructor, increment the startup
// reference count, activate a map or prove an accepted submit. Ordinary callers
// must use the complete owner above with its real platform constructor.
void source_sound_startup_commit_fragment(SoundStartupState &,
    const SoundStartupConstants &,const SoundStartupServices &,
    const SoundStartupObserver & = {});
} // namespace halo2
