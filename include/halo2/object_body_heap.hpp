#pragma once
#include "halo2/object_construction.hpp"

namespace halo2 {
struct SourceObjectBodyHeapServices {
  std::function<std::span<const std::byte>(std::uint32_t,std::size_t)> read;
  std::function<std::span<std::byte>(std::uint32_t,std::size_t)> memory;
  // Required only by a source-owned debug(+3F) branch. No assumed tick.
  std::function<std::uint32_t()> tick_count;
};
struct SourceObjectBodyHeapFrame {
  // 18FD70 reuses its original stack argument cell for the new allocation's
  // pointer. A host caller supplies genuinely owned private scratch storage;
  // this alias is never a Unit/header offset or an allocation result.
  std::uint32_t resize_temporary_pointer;
};
// Complete18FC80 and its bounded strncpy32 primitive on actual raw storage.
// Only the64-byte header is cleared. The capacity/backing belongs to the
// owning allocator (source125520 requests (capacity+53h)&~3 bytes).
void initialize_source_object_body_heap(std::uint32_t heap,std::uint32_t capacity,
    std::uint32_t name_address,std::uint32_t context,
    const SourceObjectBodyHeapServices &,const ObjectConstructionObserver & = {});
// Complete18FCE0/18FD70 with18FF20/18FF90/190030/1900C0 and18FE60.
// Return only the defined AL success; requested byte arithmetic wraps32 bits.
// Neither allocation nor resize zeroes payload. Free retains the pointer cell.
bool allocate_source_object_body(std::uint32_t heap,std::uint32_t output_pointer,
    std::uint32_t requested_bytes,std::uint32_t first_metadata,std::uint32_t second_metadata,
    const SourceObjectBodyHeapServices &,const ObjectConstructionObserver & = {});
bool resize_source_object_body(std::uint32_t heap,std::uint32_t output_pointer,
    std::uint32_t requested_bytes,std::uint32_t first_metadata,std::uint32_t second_metadata,
    const SourceObjectBodyHeapFrame &,const SourceObjectBodyHeapServices &,
    const ObjectConstructionObserver & = {});
void free_source_object_body(std::uint32_t heap,std::uint32_t output_pointer,
    const SourceObjectBodyHeapServices &,const ObjectConstructionObserver & = {});
} // namespace halo2
