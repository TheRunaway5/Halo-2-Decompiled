#pragma once
#include "halo2/sound_cache_startup.hpp"
#include "halo2/source_bitmap_async_read.hpp"
namespace halo2 {
struct SoundCacheRequestState {
  // Genuinely constructed cache roots. No cold/global/default cache exists.
  std::uint32_t entry_pool,page_allocator,physical_buffer;
};
struct SoundCacheRequestServices {
  std::function<std::span<std::byte>(std::uint32_t,std::size_t)> memory;
  // Complete19D8C0/1A4870 owning request. Its actual publication is required;
  // allocation is not read completion. File/token/worker ownership is explicit.
  std::function<void(const SourceBitmapReadRequest &)> async_read;
  // Real host scheduling opportunity for original blocking request bit0.
  // A missing owner stops; it cannot set the source completion byte.
  std::function<void()> yield;
};
using SoundCacheRequestObserver=SoundCacheStartupObserver;
// Complete21A960 CPU/cache/page owner including21AB20,13EAE0 and genuine
// constructed callback identities21AAB0/21AAF0. No readiness/admission result
// is supplied. Missing synchronization/file owners stop at async_read.
std::uint32_t source_sound_cache_request(const SoundCacheRequestState &,
    std::uint32_t sound,std::uint32_t chunk,std::uint32_t flags,
    const SoundCacheStartupConstants &,const SoundCacheRequestServices &,
    const SoundCacheRequestObserver & = {});
} // namespace halo2
