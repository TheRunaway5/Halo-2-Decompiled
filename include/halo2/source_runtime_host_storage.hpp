#pragma once
#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_static_allocation.hpp"

namespace halo2 {
// Linux raw-storage adapter for the reached source kernel allocation request.
// Native32-bit tokens identify owned SourceRuntimeMemory regions. They are
// host address-space choices, not recovered Xbox addresses. Initial bytes
// come from actual anonymous host mmap pages, without game-field writes.
class SourceRuntimeHostStorage {
public:
  explicit SourceRuntimeHostStorage(SourceRuntimeMemory &,
                                    std::uint32_t first_token = 0x10000000);
  SourceVirtualAllocationResult allocate(SourceVirtualAllocationRequest);

private:
  SourceRuntimeMemory &memory_;
  std::uint32_t next_;
};
} // namespace halo2
