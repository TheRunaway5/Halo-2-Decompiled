#pragma once
#include "halo2/sound_cache_startup.hpp"

namespace halo2 {
// Mutable operand addresses, not values copied from executable startup data.
// The owner rereads them at the original wait/delete boundaries.
struct SoundCacheLifecycleBindings {
  std::uint32_t entry_pool_pointer,page_allocator_pointer,physical_buffer_pointer;
  std::uint32_t work_mutex_slot,work_head,requests_added,sound_system_pointer;
};
struct SoundCacheLifecycleServices {
  // Access must belong to the caller's complete CPU-entry lease. Spans cannot
  // escape that lease. No concurrent safety is implied by this interface.
  std::function<std::span<std::byte>(std::uint32_t,std::size_t)> memory;
  std::function<std::uint32_t()> tick_count;
  // Genuine native wait/release. A failed or unavailable service must stop;
  // a timeout is not successful acquisition. Source EAX is unconsumed here.
  std::function<void(std::uint32_t,std::uint32_t)> wait;
  std::function<void(std::uint32_t)> release_mutex;
};
using SoundCacheLifecycleObserver=SoundCacheStartupObserver;
// Complete21A920 clock and13E9A0 overflow helper. The source epoch is not host
// time. Cache metadata and callbacks must come from their genuine producers.
void source_sound_cache_clock(const SoundCacheLifecycleBindings &,
    const SoundCacheLifecycleServices &,const SoundCacheLifecycleObserver & = {});
void source_sound_cache_rebase(std::uint32_t page_allocator,
    const SoundCacheLifecycleServices &,const SoundCacheLifecycleObserver & = {});
// Complete21AA60 increments its source temporary-use byte, saturating at255,
// and computes the physical alias. No readiness check exists in this owner.
std::uint32_t source_sound_cache_alias(const SoundCacheLifecycleBindings &,
    std::uint32_t chunk,const SoundCacheLifecycleServices &,
    const SoundCacheLifecycleObserver & = {});
// Complete21A800 including13EFA0/21AAF0/16D4C0 for the actual constructed
// cache delete callback. Busy entries use genuine native time and work mutex.
void source_sound_cache_drain(const SoundCacheLifecycleBindings &,
    const SoundCacheStartupConstants &,const SoundCacheLifecycleServices &,
    const SoundCacheLifecycleObserver & = {});
// Bounded127290 cold caller: all source availability guards are actually read.
// The active driver branch is unavailable and stops, never forced to cold.
void source_sound_cache_audio_update_cold(const SoundCacheLifecycleBindings &,
    const SoundCacheLifecycleServices &,const SoundCacheLifecycleObserver & = {});
} // namespace halo2
