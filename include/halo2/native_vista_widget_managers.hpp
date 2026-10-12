#pragma once
#include "halo2/vista_widget_manager_lifetime.hpp"

namespace halo2 {
// Native retained storage for the original D718E0 UI object's3FF0 bytes.
// Fresh PE storage is zero; complete6092E3 then constructs its manager banks,
// request, pair and text queue. This does not manufacture any screen, pending
// request, resource readiness, allocator or session admission.
class NativeVistaWidgetManagers final {
public:
  static constexpr std::uint32_t source_base=0xd718e0;
  static constexpr std::size_t storage_size=0x3ff0;
  NativeVistaWidgetManagers();
  NativeVistaWidgetManagers(const NativeVistaWidgetManagers&)=delete;
  NativeVistaWidgetManagers& operator=(const NativeVistaWidgetManagers&)=delete;
  NativeVistaWidgetManagers(NativeVistaWidgetManagers&&)=delete;
  NativeVistaWidgetManagers& operator=(NativeVistaWidgetManagers&&)=delete;
  std::span<std::byte> bytes(std::uint32_t source_address,std::size_t size);
  std::span<const std::byte> bytes(std::uint32_t source_address,std::size_t size) const;
  std::uint32_t read(std::uint32_t source_address,std::size_t size) const;
  void write(std::uint32_t source_address,std::uint32_t value,std::size_t size);
private:
  std::array<std::byte,storage_size> storage_{};
};
} //namespace halo2
