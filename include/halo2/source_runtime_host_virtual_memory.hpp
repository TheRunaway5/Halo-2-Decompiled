#pragma once
#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_static_allocation.hpp"
#include <memory>

namespace halo2 {
// Native resource owner for the reached process-heap reserve, commit
// and release requests. This supplies host resources, not an Xbox kernel ABI.
// Source pages remain4096 bytes even when the host mapping granule is larger.
// The memory bank must remain in place and outlive this owner. Its reservations must be released
// through this owner, which also releases any remaining banks on destruction.
class SourceRuntimeHostVirtualMemory {
public:
  explicit SourceRuntimeHostVirtualMemory(SourceRuntimeMemory &,
                                         std::uint32_t first_token = 0x10000000);
  ~SourceRuntimeHostVirtualMemory();
  SourceRuntimeHostVirtualMemory(const SourceRuntimeHostVirtualMemory &) = delete;
  SourceRuntimeHostVirtualMemory &operator=(const SourceRuntimeHostVirtualMemory &) = delete;
  SourceVirtualAllocationResult allocate(SourceVirtualAllocationRequest);
  bool backs(const SourceRuntimeMemory &memory) const noexcept { return &memory_ == &memory; }
  // The reached game cleanup ignores all kernel outputs. Do not invent a
  // failure NTSTATUS or post-call address/extent cell for that boundary.
  void release(std::uint32_t base, std::uint32_t bytes, std::uint32_t free_type);

  struct ReservationCheckpoint {
    std::uint32_t base{},bytes{};
    std::vector<std::uint8_t> committed;
    bool operator==(const ReservationCheckpoint &) const = default;
  };
  struct OwnershipCheckpoint {
    std::uint32_t version=1,next{};
    std::vector<ReservationCheckpoint> reservations;
    bool operator==(const OwnershipCheckpoint &) const = default;
  };
  class PreparedOwnership {
  public:
    ~PreparedOwnership();
    PreparedOwnership(PreparedOwnership &&) noexcept;
    PreparedOwnership &operator=(PreparedOwnership &&) noexcept;
    bool for_owner(const SourceRuntimeHostVirtualMemory &) const noexcept;
    bool matches_memory(const SourceRuntimeMemory &) const noexcept;
  private:
    friend class SourceRuntimeHostVirtualMemory;
    PreparedOwnership();
    struct Impl;
    std::unique_ptr<Impl> impl_;
  };
  OwnershipCheckpoint ownership_checkpoint() const;
  // Stage real host reservations against the complete incoming byte bank.
  // Both the current bank and ownership remain untouched if staging fails.
  // Pin the exact independently restored region addresses; an equivalent
  // second restoration cannot substitute for this prepared native backing.
  // Owner and replacement must outlive preparation; execution is quiescent.
  PreparedOwnership prepare_ownership(const OwnershipCheckpoint &,
      const SourceRuntimeMemory::Checkpoint &,const SourceRuntimeMemory &replacement) const;
  // Enclosing world transaction first publishes its admitted byte bank, then
  // commits all prepared resource owners and rebinds borrowed native views.
  // Reject publication before that exact bank arrives, without consuming the
  // token. No memory-bank regions are removed; old host mappings alone retire.
  bool commit_ownership(PreparedOwnership &&) noexcept;

private:
  struct Reservation {
    void *host;
    std::uint32_t bytes;
    std::vector<bool> committed;
  };
  SourceRuntimeMemory &memory_;
  std::uint32_t next_;
  std::map<std::uint32_t, Reservation> reservations_;
};
} // namespace halo2
