#pragma once
#include "halo2/source_runtime_memory.hpp"
#include "halo2/unit_animation_selection_runtime.hpp"

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
