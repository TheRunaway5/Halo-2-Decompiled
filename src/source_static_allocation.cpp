#include "halo2/source_static_allocation.hpp"
#include <stdexcept>

namespace halo2 {
std::uint32_t source_virtual_allocate_1_1(
    std::uint32_t base, std::uint32_t bytes, std::uint32_t allocation_type,
    std::uint32_t protection, const SourceStaticAllocationServices &services) {
  if (!services.allocate)
    throw std::runtime_error("Original static virtual-memory owner unavailable");
  const auto result = services.allocate({base, 0, bytes, allocation_type, protection});
  base = result.base;
  if (result.status & 0x80000000u) {
    if (!services.status_error)
      throw std::runtime_error("Original virtual-memory status-error owner unavailable");
    services.status_error(result.status);
    base = 0;
  }
  return base;
}
std::uint32_t source_allocate_static_memory_1_1(
    std::uint32_t bytes, const SourceStaticAllocationServices &services) {
  const auto base = source_virtual_allocate_1_1(0, bytes, 0x101000, 4, services);
  if (!base) {
    if (!services.last_error)
      throw std::runtime_error("Original static-allocation last-error owner unavailable");
    services.last_error();
  }
  return base;
}
} // namespace halo2
