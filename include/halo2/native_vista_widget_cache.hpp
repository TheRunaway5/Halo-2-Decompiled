#pragma once
#include "halo2/vista_widget_cache.hpp"
#include <memory>

namespace halo2 {
// Writable native lifetime for the original loaded UI tag region. The supplied
// source32 base is registry identity, never a host pointer. Construction copies
// the retained source bytes only; it does not publish loaded-cache globals,
// request a file, allocate a GPU resource or mark any bitmap resident.
class NativeVistaWidgetCache final {
public:
  NativeVistaWidgetCache(std::shared_ptr<const VistaWidgetCacheBank>,
      std::uint32_t source_base);
  NativeVistaWidgetCache(const NativeVistaWidgetCache&)=delete;
  NativeVistaWidgetCache& operator=(const NativeVistaWidgetCache&)=delete;
  NativeVistaWidgetCache(NativeVistaWidgetCache&&)=delete;
  NativeVistaWidgetCache& operator=(NativeVistaWidgetCache&&)=delete;

  std::uint32_t source_base() const noexcept { return base_; }
  std::size_t storage_size() const noexcept { return storage_.size(); }
  const VistaWidgetCacheBank& original() const noexcept { return *original_; }
  std::span<std::byte> bytes(std::uint32_t source_address,std::size_t size);
  std::span<const std::byte> bytes(std::uint32_t source_address,std::size_t size) const;
  std::uint32_t read(std::uint32_t source_address,std::size_t size) const;
  void write(std::uint32_t source_address,std::uint32_t value,std::size_t size);

private:
  std::size_t offset(std::uint32_t,std::size_t) const;
  std::shared_ptr<const VistaWidgetCacheBank> original_;
  std::uint32_t base_{};
  std::vector<std::byte> storage_;
};
} // namespace halo2
