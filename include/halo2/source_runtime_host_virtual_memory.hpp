#pragma once
#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_static_allocation.hpp"

namespace halo2 {
// Native Linux resource owner for the reached process-heap reserve, commit
// and release requests. This supplies host resources, not an Xbox kernel ABI.
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
