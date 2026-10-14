#pragma once
#include "halo2/source_runtime_memory.hpp"
#include "halo2/unit_animation_selection_runtime.hpp"
#include <memory>

namespace halo2 {
struct SourcePhysicalAllocationRequest {
  std::uint32_t bytes, lowest, highest, alignment, protection;
};
// Actual host page storage behind the original physical-address namespace.
// The kernel request bounds choose the retained token; no game arena pointer,
// offset, allocator, pool or ready flag is initialized here. Host errors stop
// instead of inventing Xbox NTSTATUS/TLS results. The bank must outlive this
// owner; explicit native teardown releases its own reservations.
class SourceRuntimeHostPhysicalMemory {
public:
  explicit SourceRuntimeHostPhysicalMemory(SourceRuntimeMemory &);
  ~SourceRuntimeHostPhysicalMemory();
  SourceRuntimeHostPhysicalMemory(const SourceRuntimeHostPhysicalMemory &) = delete;
  SourceRuntimeHostPhysicalMemory &operator=(const SourceRuntimeHostPhysicalMemory &) = delete;
  std::uint32_t allocate(SourcePhysicalAllocationRequest);
  void protect(std::uint32_t address, std::uint32_t bytes, std::uint32_t protection);
  bool backs(const SourceRuntimeMemory &memory) const noexcept { return &memory_ == &memory; }
  struct AllocationCheckpoint {
    std::uint32_t base{},bytes{};
    std::vector<std::uint8_t> read_only;
    bool operator==(const AllocationCheckpoint &) const = default;
  };
  struct OwnershipCheckpoint {
    std::uint32_t version=1;
    std::vector<AllocationCheckpoint> allocations;
    bool operator==(const OwnershipCheckpoint &) const = default;
  };
  class PreparedOwnership {
  public:
    ~PreparedOwnership();
    PreparedOwnership(PreparedOwnership &&) noexcept;
    PreparedOwnership &operator=(PreparedOwnership &&) noexcept;
    bool for_owner(const SourceRuntimeHostPhysicalMemory &) const noexcept;
  private:
    friend class SourceRuntimeHostPhysicalMemory;
    PreparedOwnership();
    struct Impl;
    std::unique_ptr<Impl> impl_;
  };
  OwnershipCheckpoint ownership_checkpoint() const;
  // Stage actual pages/protections and retain the protection lock until commit
  // or cancellation. The complete world owner must first quiesce all users.
  PreparedOwnership prepare_ownership(const OwnershipCheckpoint &,
                                      const SourceRuntimeMemory::Checkpoint &) const;
  // After admitted bank publication. Releases only old host mappings; the
  // imported logical regions and registered write-guard identity survive.
  bool commit_ownership(PreparedOwnership &&) noexcept;
private:
  struct Allocation { void *host; std::uint32_t bytes; };
  SourceRuntimeMemory &memory_;
  std::map<std::uint32_t, Allocation> allocations_;
};
// Native ABI at the authenticated MmAllocateContiguousMemoryEx and
// MmSetAddressProtect imports. Reads real source frames, consumes the stdcall
// stack and preserves other CPU state. Unsupported calls leave state intact;
// actual host resource failure stops before returning to source code.
bool source_try_native_death_physical_call_1_1(
    SourceRuntimeMemory &, const SourceUnitAnimationRuntimeCall &,
    SourceUnitAnimationRuntimeAbi &, SourceRuntimeHostPhysicalMemory &);
} // namespace halo2
