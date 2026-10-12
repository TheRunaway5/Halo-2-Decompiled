#pragma once
#include "halo2/source_engine_entry.hpp"
#include "halo2/source_king_wire.hpp"

namespace halo2 {
struct SourceEngineEntryAllocation {
  std::uint32_t address{};
  std::span<std::uint8_t> bytes;
};
// Allocator methods supply real host backing. outstanding is the wrapper's
// literal unsigned counter, updated here rather than by the methods themselves.
struct SourceEngineEntryAllocator {
  std::uint32_t outstanding{};
  std::function<SourceEngineEntryAllocation(std::int32_t)> allocate;
  std::function<void()> compact;
  std::function<void(const SourceEngineEntryAllocation&)> inspect,release;
};
struct SourceEngineEntryDecoder {
  std::function<bool(std::int32_t creation_size,std::uint32_t creation_address,
      std::int32_t state_size,std::span<std::uint8_t>)> initialize;
  std::function<bool(bool initial,std::uint32_t& mask,std::int32_t state_size,
      std::span<std::uint8_t>,SourceEngineBitstream&)> read;
};
// Whole retail8AAC0/TU1.5 8E8F0 update decoder after handle&3FF indexing.
// Returns0 on one appended typeD bank,3 on identity/allocation/codec failure.
// capacity is an original unused argument. Explicit output backing is still
// required; codec success does not imply a non-overflowing accepted packet.
// The live bank and entry metadata are left unchanged. On success the caller
// owns the allocated bank; rejection inspects/releases it in original order.
std::int32_t source_engine_entry_update_decode(const SourceEngineEntryRetail&,
    std::uint32_t handle,std::span<const SourceEngineEntryDecoder>,
    std::span<const std::uint8_t> live_bank,SourceEngineEntryAllocator&,
    std::uint32_t& mask,std::int32_t capacity,std::int32_t& produced,
    std::span<SourceEngineEntryBlock>,SourceEngineBitstream&);
std::int32_t source_engine_entry_update_decode(const SourceEngineEntryTitleUpdate&,
    std::uint32_t handle,std::span<const SourceEngineEntryDecoder>,
    std::span<const std::uint8_t> live_bank,SourceEngineEntryAllocator&,
    std::uint32_t& mask,std::int32_t capacity,std::int32_t& produced,
    std::span<SourceEngineEntryBlock>,SourceEngineBitstream&);
} // namespace halo2
