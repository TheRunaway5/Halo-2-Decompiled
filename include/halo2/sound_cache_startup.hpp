#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
// Immutable callback identities from the selected owned executable. They are
// retained in source page-allocator records; they are not successful callbacks.
struct SoundCacheStartupConstants {
  std::uint32_t delete_callback_identity,busy_callback_identity;
};
// Actual existing source allocator identities and12CB70/arena lifecycle state.
// No default allocator, physical limit/index, cache pointer or readiness exists.
struct SoundCacheStartupState {
  std::uint32_t general_allocator_identity,static_allocator_identity;
  std::uint32_t entry_pool,page_allocator,physical_buffer;
  std::uint32_t physical_arena_index;
  std::array<std::uint32_t,5> physical_lower_limits,physical_upper_limits;
};
struct SoundCacheStartupServices {
  // Actual retained32-bit addresses resolve to real owned raw backing.
  std::function<std::span<std::byte>(std::uint32_t,std::size_t)> memory;
  // Separately named host adaptation of the source general allocator's raw
  // allocation method. Returns an alias of actual zero-filled owned storage,
  // or a genuine failed allocation0. It does not construct a pool or page.
  std::function<std::uint32_t(std::uint32_t,std::uint32_t)> raw_allocate;
  // Real host protection of the already owned physical-arena backing. A
  // missing/failed service stops; no guest kernel result is fabricated.
  std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> raw_protect;
};
struct SoundCacheStartupStore {
  std::uint32_t instruction,address;
  unsigned bytes;
  std::uint32_t before,after;
  friend bool operator==(const SoundCacheStartupStore &,const SoundCacheStartupStore &)=default;
};
using SoundCacheStartupObserver=std::function<void(const SoundCacheStartupStore &)>;
// Complete21A6E0 CPU constructor, including13E8E0 and actual16D070/16D0D0/
//16D220 pool layout/reset. Every external memory/allocator/protection boundary
// is required. Physical reservation preserves signed comparison and32-bit
// arithmetic/order. It does not activate a map or enable/complete cache reads.
// Store instruction identities use verified1.1 owner PCs;u5 bodies correspond.
void source_sound_cache_startup(SoundCacheStartupState &,
    const SoundCacheStartupConstants &,const SoundCacheStartupServices &,
    const SoundCacheStartupObserver & = {});
} // namespace halo2
