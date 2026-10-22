#include "halo2/biped_component_storage.hpp"
#include <stdexcept>

namespace halo2 {
void source_initialize_biped_component_storage_1_1(
    const SourceRawDatumPoolCreationServices &services,
    const SourceRawDatumPoolObserver &observe) {
  const auto read = [&](std::uint32_t p, unsigned n = 4) {
    if (!services.storage.read && !services.storage.memory)
      throw std::runtime_error("Original component storage backing unavailable");
    const std::span<const std::byte> b = services.storage.read
        ? services.storage.read(p, n) : services.storage.memory(p, n);
    if (b.size() != n)
      throw std::runtime_error("Original component storage extent unavailable");
    std::uint32_t v = 0;
    for (unsigned i = 0; i < n; ++i)
      v |= std::to_integer<std::uint32_t>(b[i]) << (8 * i);
    return v;
  };
  const auto write = [&](std::uint32_t site, std::uint32_t p,
                         std::uint32_t v, unsigned n) {
    const auto before = read(p, n);
    if (!services.storage.memory)
      throw std::runtime_error("Original component storage write owner unavailable");
    auto b = services.storage.memory(p, n);
    if (b.size() != n)
      throw std::runtime_error("Original component storage write extent unavailable");
    for (unsigned i = 0; i < n; ++i)
      b[i] = std::byte(v >> (8 * i));
    if (observe)
      observe({site, p, before, v, n});
  };
  const auto context = read(0x46fbf8), method = read(read(context));
  if (!services.allocate)
    throw std::runtime_error("Original component storage allocator unavailable");
  const auto pool = services.allocate(context, method, 0x1409b);
  if (pool) {
    source_raw_datum_initialize_inplace(pool, 0x45cc3c, 512, 160, 4,
                                        context, services.storage, observe);
    write(0x1d2655, pool + 0x2a, read(pool + 0x2a, 1) | 4u, 1);
  }
  write(0x1d265b, 0x526248, pool, 4);
}
} // namespace halo2
