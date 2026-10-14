#pragma once
#include <cstdint>
#include <array>
#include <memory>
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
  // A source-backed view saves only its logical binding and immutable pool
  // shape. Allocation metadata, salt/occupancy and elements belong to the
  // enclosing whole-bank checkpoint; no native pointer is serialized.
  struct SourceBinding {
    std::uint32_t version=1,pool{},capacity{},stride{};
    std::array<std::byte,32> name{};
    bool operator==(const SourceBinding &) const = default;
  };
  class PreparedSourceView {
  public:
    ~PreparedSourceView();
    PreparedSourceView(PreparedSourceView &&) noexcept;
    PreparedSourceView &operator=(PreparedSourceView &&) noexcept;
    bool for_owner(const DatumPool &) const noexcept;
  private:
    friend class DatumPool;
    PreparedSourceView();
    struct Impl;
    std::unique_ptr<Impl> impl_;
  };
  SourceBinding source_binding() const;
  // Both pools/banks must be externally quiescent. The replacement is the
  // exact staged backing that the enclosing world will publish, not a second
  // independently restored copy. Preparation never changes the current view.
  // The view owner and staged backing must outlive preparation; cancel staged
  // views before cancelling their enclosing replacement bank.
  PreparedSourceView prepare_source_view(const SourceBinding &,
                                         const SourceRuntimeMemory &replacement) const;
  // True only after the exact staged backing has moved into source_memory().
  // Commit is allocation-free and consumes the token on success; a premature
  // or foreign commit leaves the view/token intact. Borrowed spans expire.
  bool source_view_ready(const PreparedSourceView &) const noexcept;
  bool commit_source_view(PreparedSourceView &&) noexcept;
private:
  friend struct SourceAuthorityArchiveAccess;
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
