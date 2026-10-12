#pragma once
#include <cstdint>
#include <functional>

namespace halo2 {
struct SourceVirtualAllocationRequest {
  std::uint32_t base, zero_bits, bytes, allocation_type, protection;
};
struct SourceVirtualAllocationResult {
  std::uint32_t status, base, bytes;
};
struct SourceStaticAllocationServices {
  // Actual NtAllocateVirtualMemory resource boundary reached by33785C.
  // The provider owns the returned raw backing and its allocation outcome.
  std::function<SourceVirtualAllocationResult(SourceVirtualAllocationRequest)>
      allocate;
  // Original2D81D4 NTSTATUS-to-last-error and2D817E TLS error getter.
  // Required only when their actual source branch is reached.
  std::function<void(std::uint32_t status)> status_error;
  std::function<std::uint32_t()> last_error;
};
// Complete33785C. Its four caller arguments become the actual kernel request
// with ZeroBits=0. A zero base does not call GetLastError here: that belongs
// to the caller. Negative NTSTATUS requires the original status-error owner.
std::uint32_t source_virtual_allocate_1_1(
    std::uint32_t base, std::uint32_t bytes, std::uint32_t allocation_type,
    std::uint32_t protection, const SourceStaticAllocationServices &);
// Complete11E150→33785C over the actual platform resource request. This
// does not construct or clear pool contents, choose a datum or mark validity.
std::uint32_t source_allocate_static_memory_1_1(
    std::uint32_t bytes, const SourceStaticAllocationServices &);
} // namespace halo2
