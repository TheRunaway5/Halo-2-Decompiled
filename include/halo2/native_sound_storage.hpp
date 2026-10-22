#pragma once
#include "halo2/sound_cache_startup.hpp"
#include <memory>

namespace halo2 {
// Private native address translation, not retail allocator/arena state. The
// caller explicitly assigns the alias namespace and actual allocator identity.
struct NativeSoundStorageConfiguration {
  std::uint32_t allocator_identity;
  std::uint32_t first_allocation_alias;
  std::uint64_t allocation_alias_limit;
};
// Owns actual OS allocations. It neither initializes source pool headers nor
// chooses physical-arena limits, allocator identities, game fields or readiness.
// Use on the owning update thread; returned spans require a retained owner.
class NativeSoundStorage {
public:
  explicit NativeSoundStorage(const NativeSoundStorageConfiguration &);
  // Explicitly supplied startup/physical backing, zeroed by the OS. Addresses
  // and 4 KiB-rounded extents must fit an unoccupied 32-bit alias range. Each
  // region has independent OS-page-rounded backing; its padding is not exposed.
  void map_zeroed(std::uint32_t address, std::size_t extent);
  std::span<std::byte> memory(std::uint32_t address, std::size_t extent) const;
  std::size_t page_size() const;       // Actual host OS page size.
  std::size_t alias_page_size() const; // Private address-translation granularity.
  std::size_t region_count() const;
  // These callbacks retain the real backing even after this facade is gone.
  // The allocator can return0 only when the host OS allocation actually fails;
  // missing identities/alias space/protection backing throw, never source success.
  SoundCacheStartupServices cache_services() const;
private:
  struct State;
  std::shared_ptr<State> state_;
};
} // namespace halo2
