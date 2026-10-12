#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
// Addresses of the actual retained mutable globals, not pool/heap values.
struct SourceObjectStorageReleaseBindings {
  std::uint32_t object_pool_slot,body_heap_slot;
};
struct SourceObjectStorageReleaseServices {
  std::function<std::span<std::byte>(std::uint32_t,std::size_t)> memory;
};
struct SourceObjectStorageReleaseStore {
  std::uint32_t address,before,after;
  unsigned bytes;
  friend bool operator==(const SourceObjectStorageReleaseStore &,
                         const SourceObjectStorageReleaseStore &)=default;
};
using SourceObjectStorageReleaseObserver=std::function<void(const SourceObjectStorageReleaseStore &)>;
// Complete BD1F0 and its16D4C0 pool retirement. For a live object the caller
// must already have executed the real world/physics/model deletion owners. This function
// also owns the construction-failure release tail; it does not perform full
// teardown or create a successful reset/round-transition service.
void release_source_object_storage(const SourceObjectStorageReleaseBindings &,
    std::uint32_t datum,const SourceObjectStorageReleaseServices &,
    const SourceObjectStorageReleaseObserver & = {});
} // namespace halo2
