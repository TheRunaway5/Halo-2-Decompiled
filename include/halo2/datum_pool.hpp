#pragma once
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include "halo2/native_runtime_address.hpp"

namespace halo2 {
class SourceRuntimeMemory;
// TU16D220/2C0/410/480/4C0/560 identity and element-storage operations.
// Object bodies, type callbacks, world allocation order and ownership binding
// belong to their callers. A datum is absent only when its bits are FFFFFFFF.
class DatumPool {
public:
  static constexpr std::uint32_t none = 0xffffffffu;
  DatumPool(std::string_view name, std::int32_t capacity, std::int32_t stride,
            std::uint16_t flags = 4);
  // View the actual constructor-owned raw pool. No metadata, salts, elements
  // or bitmap are copied or initialized. The bank must outlive this view.
  DatumPool(SourceRuntimeMemory &,std::uint32_t source_pool);
  DatumPool(SourceRuntimeMemory &,NativeRuntimeAddress pool);
  DatumPool(const DatumPool &);
  DatumPool &operator=(const DatumPool &);
  DatumPool(DatumPool &&)=default;
  DatumPool &operator=(DatumPool &&)=default;
  void reset();
  std::uint32_t allocate();
  std::uint32_t allocate_at(std::int32_t index);
  // Internal primitive: caller must own a valid allocated index. Original
  // release masks the index and does not check the supplied salt or occupancy.
  void release_unchecked(std::uint32_t datum);
  std::span<std::uint8_t> lookup(std::uint32_t datum);
  std::span<const std::uint8_t> lookup(std::uint32_t datum) const;
  std::int32_t capacity() const;
  std::int32_t stride() const;
  std::int32_t cursor() const;
  std::int32_t high_water() const;
  std::int32_t count() const;
  std::uint16_t next_salt() const;
  // Explicit oracle/storage boundary; normal reset derives the counter from
  // the actual pool name. This is not an additional generation scheme.
  void set_next_salt(std::uint16_t salt);
  std::span<const std::uint32_t> bitmap() const;
  std::span<const std::uint8_t> elements() const;
  // Owned record backing for recovered source kernels. Allocation metadata
  // and salt/occupancy retirement still belong to allocate/release_unchecked.
  std::span<std::uint8_t> element_storage();
  SourceRuntimeMemory *source_memory() const {return source_memory_;}
  NativeRuntimeAddress address() const {return source_pool_;}
private:
  std::string name_;
  std::int32_t capacity_{}, stride_{}, cursor_{}, high_water_{}, count_{};
  std::uint16_t flags_{}, next_salt_{};
  std::vector<std::uint8_t> elements_;
  std::vector<std::uint32_t> bitmap_;
  SourceRuntimeMemory *source_memory_{};
  NativeRuntimeAddress source_pool_;
  // Read-only display conversion; allocation always reads the actual bank.
  mutable std::vector<std::uint32_t> bitmap_view_;
  std::uint32_t source_field(unsigned offset,unsigned bytes=4) const;
  std::uint32_t source_pool() const;
  std::uint16_t salt(std::int32_t index) const;
  std::uint32_t allocate_element(std::int32_t index);
};
} // namespace halo2
