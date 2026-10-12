#include "halo2/source_main_heap_storage.hpp"
#include <stdexcept>

namespace halo2 {
std::uint32_t source_initialize_main_heap_storage_1_1(
    SourcePhysicsContactMemory &memory,
    const SourceStaticAllocationServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
  const auto write = [&](std::uint32_t site, std::uint32_t p, std::uint32_t v) {
    SourcePhysicsContactRecordStore store{site, p, std::vector<std::byte>(4),
                                         std::vector<std::byte>(4)};
    memory.read(p, store.before);
    for (unsigned i = 0; i < 4; ++i)
      store.written[i] = std::byte(v >> (8 * i));
    memory.write(p, store.written);
    if (observe) observe(store);
  };
  const auto raw = source_virtual_allocate_1_1(0, 0x106470, 0x101000, 4, services);
  if (!raw) {
    if (!services.last_error)
      throw std::runtime_error("Original main-heap last-error owner unavailable");
    const auto error = services.last_error();
    write(0x1482be, 0x480d90, 0);
    return error;
  }
  const auto aligned = (raw + 0x1000u) & 0xfffff000u;
  write(0x1482d6, aligned - 8u, 0x105470);
  write(0x1482dd, aligned - 4u, aligned - raw);
  write(0x1482e0, 0x480d90, aligned);
  return raw;
}
} // namespace halo2
