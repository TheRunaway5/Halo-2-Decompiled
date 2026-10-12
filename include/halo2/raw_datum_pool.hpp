#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
struct SourceRawDatumPoolServices {
  std::function<std::span<std::byte>(std::uint32_t, std::size_t)> memory;
  // Source names, allocator records and method tables can be read-only.
  // If omitted, legacy caller-owned mutable backing supplies reads too.
  std::function<std::span<const std::byte>(std::uint32_t, std::size_t)> read;
};
struct SourceRawDatumPoolStore {
  std::uint32_t instruction, address, before, after;
  unsigned bytes;
  friend bool operator==(const SourceRawDatumPoolStore &,
                         const SourceRawDatumPoolStore &) = default;
};
using SourceRawDatumPoolObserver =
    std::function<void(const SourceRawDatumPoolStore &)>;
struct SourceRawDatumPoolCreationServices {
  SourceRawDatumPoolServices storage;
  // Actual virtual allocator request at16D01B. Must own real backing and
  // failure behavior for this context/method, not choose an allocation result.
  std::function<std::uint32_t(std::uint32_t context, std::uint32_t method,
                              std::uint32_t bytes)>
      allocate;
};
// Complete16CFF0 allocation sizing, actual virtual dispatch and conditional
// 16D070 construction. It does not mark the new pool valid or clear payload.
std::uint32_t
source_raw_datum_create(std::uint32_t capacity, std::uint32_t name,
                        std::uint32_t stride, std::uint32_t alignment,
                        std::uint32_t context,
                        const SourceRawDatumPoolCreationServices &,
                        const SourceRawDatumPoolObserver & = {});
// Complete125E50 game-arena allocation and1656A0/165700 checksum writes.
// The arena/base/offset/checksum globals and backing must already belong to
// their startup owner. No arena size, readiness or allocation result is added.
std::uint32_t
source_allocate_game_arena_1_1(std::uint32_t bytes,
                               const SourceRawDatumPoolServices &,
                               const SourceRawDatumPoolObserver & = {});
// Complete16D0D0/16D070 on caller-owned original address storage. The
// allocator supplies the backing extent and context; payload is not cleared.
void source_raw_datum_header(std::uint32_t pool, std::uint32_t name,
                             std::uint32_t capacity, std::uint32_t stride,
                             std::uint8_t alignment, std::uint32_t context,
                             std::uint32_t bitmap,
                             const SourceRawDatumPoolServices &,
                             const SourceRawDatumPoolObserver & = {});
void source_raw_datum_initialize_inplace(
    std::uint32_t pool, std::uint32_t name, std::uint32_t capacity,
    std::uint32_t stride, std::uint8_t alignment, std::uint32_t context,
    const SourceRawDatumPoolServices &,
    const SourceRawDatumPoolObserver & = {});
// Complete16D130 recounts existing element salts, preserving payload and
// respecting the source debug-fill flag.16D210 marks valid then resets;
// 16D220 resets without changing validity. Salt comes from the real name.
void source_raw_datum_bind_existing(std::uint32_t pool, std::uint32_t elements,
                                    std::uint32_t capacity,
                                    const SourceRawDatumPoolServices &,
                                    const SourceRawDatumPoolObserver & = {});
void source_raw_datum_reset(std::uint32_t pool,
                            const SourceRawDatumPoolServices &,
                            const SourceRawDatumPoolObserver & = {});
void source_raw_datum_make_valid(std::uint32_t pool,
                                 const SourceRawDatumPoolServices &,
                                 const SourceRawDatumPoolObserver & = {});
// Complete16D2C0 on actual raw pool/header/bitmap/element storage. This does
// not create a pool, derive a capacity, replace a salt or own a host vector.
std::uint32_t
source_raw_datum_allocate(std::uint32_t pool,
                          const SourceRawDatumPoolServices &,
                          const SourceRawDatumPoolObserver & = {});
// Complete16D410/16D480 requested-slot allocation,16D4C0 unchecked retirement
// and16D560 full-salt lookup over the same source-owned storage.
std::uint32_t source_raw_datum_allocate_at(std::uint32_t pool,std::int32_t index,
    const SourceRawDatumPoolServices &,const SourceRawDatumPoolObserver & = {});
// Complete16D390. Unlike16D410, this entry binds the caller's full identity
// after the actual slot reset/salt-counter writes. It never replaces a live
// slot and preserves the original failure/bitmap/high-water semantics.
std::uint32_t source_raw_datum_allocate_identity(std::uint32_t pool,std::uint32_t datum,
    const SourceRawDatumPoolServices &,const SourceRawDatumPoolObserver & = {});
void source_raw_datum_release_unchecked(std::uint32_t pool,std::uint32_t datum,
    const SourceRawDatumPoolServices &,const SourceRawDatumPoolObserver & = {});
std::uint32_t source_raw_datum_lookup(std::uint32_t pool,std::uint32_t datum,
    const SourceRawDatumPoolServices &);
} // namespace halo2
