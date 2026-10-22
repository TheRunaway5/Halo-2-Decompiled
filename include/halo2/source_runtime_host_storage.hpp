#pragma once
#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_static_allocation.hpp"

namespace halo2 {
// Native raw-storage adapter for the reached source kernel allocation request.
// Native32-bit tokens identify owned SourceRuntimeMemory regions. They are
// host address-space choices, not recovered Xbox addresses. Initial bytes
// come from actual anonymous host mmap pages, without game-field writes.
class SourceRuntimeHostStorage {
public:
  explicit SourceRuntimeHostStorage(SourceRuntimeMemory &,
                                    std::uint32_t first_token = 0x10000000);
  SourceVirtualAllocationResult allocate(SourceVirtualAllocationRequest);
  SourceVirtualAllocationResult allocate(SourceVirtualAllocationRequest,std::string owner);
  struct CursorCheckpoint {
    std::uint32_t version=1,next{};
    bool operator==(const CursorCheckpoint &) const = default;
  };
  class PreparedCursor {
  public:
    PreparedCursor(PreparedCursor &&) noexcept;
    PreparedCursor &operator=(PreparedCursor &&) noexcept;
    bool for_owner(const SourceRuntimeHostStorage &) const noexcept;
  private:
    friend class SourceRuntimeHostStorage;
    PreparedCursor(const SourceRuntimeHostStorage *,std::uint32_t);
    const SourceRuntimeHostStorage *owner_;
    std::uint32_t next_;
  };
  CursorCheckpoint cursor_checkpoint() const noexcept;
  // The enclosing world owner validates all live allocation extents against
  // this cursor and the incoming bank. Released extents still advance it.
  PreparedCursor prepare_cursor(const CursorCheckpoint &) const;
  bool commit_cursor(PreparedCursor &&) noexcept;

private:
  SourceRuntimeMemory &memory_;
  std::uint32_t next_;
};
} // namespace halo2
