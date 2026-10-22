#include "halo2/raw_datum_pool.hpp"
#include <stdexcept>

namespace halo2 {
namespace {
struct Storage {
  const SourceRawDatumPoolServices &s;
  const SourceRawDatumPoolObserver &observe;
  std::uint32_t read(std::uint32_t p, unsigned n = 4) const {
    if (!s.read && !s.memory)
      throw std::runtime_error(
          "Original raw pool allocation backing unavailable");
    const std::span<const std::byte> b = s.read ? s.read(p, n) : s.memory(p, n);
    if (b.size() != n)
      throw std::runtime_error(
          "Original raw pool allocation extent unavailable");
    std::uint32_t v = 0;
    for (unsigned i = 0; i < n; ++i)
      v |= std::to_integer<std::uint32_t>(b[i]) << (8 * i);
    return v;
  }
  void write(std::uint32_t site, std::uint32_t p, std::uint32_t value,
             unsigned n = 4) const {
    const auto before = read(p, n);
    if (!s.memory)
      throw std::runtime_error(
          "Original raw pool allocation write owner unavailable");
    auto b = s.memory(p, n);
    if (b.size() != n)
      throw std::runtime_error(
          "Original raw pool allocation write extent unavailable");
    if (n < 4)
      value &= (1u << (8 * n)) - 1;
    for (unsigned i = 0; i < n; ++i)
      b[i] = std::byte(value >> (8 * i));
    if (observe)
      observe({site, p, before, value, n});
  }
};
} // namespace

std::uint32_t
source_raw_datum_create(std::uint32_t capacity, std::uint32_t name,
                        std::uint32_t stride, std::uint32_t alignment,
                        std::uint32_t context,
                        const SourceRawDatumPoolCreationServices &services,
                        const SourceRawDatumPoolObserver &observe) {
  Storage a{services.storage, observe};
  const auto table = a.read(context);
  const auto sum = capacity + 31u;
  const auto bitmap_words =
      (sum >> 5) | ((sum & 0x80000000u) ? 0xf8000000u : 0u);
  const auto bytes =
      capacity * stride + bitmap_words * 4u + (1u << (alignment & 31u)) + 0x4bu;
  const auto method = a.read(table);
  if (!services.allocate)
    throw std::runtime_error("Original raw pool virtual allocator unavailable");
  const auto pool = services.allocate(context, method, bytes);
  if (pool) {
    source_raw_datum_initialize_inplace(pool, name, capacity, stride,
                                        std::uint8_t(alignment), context,
                                        services.storage, observe);
    a.write(0x16d035, pool + 0x2a, a.read(pool + 0x2a, 1) | 4u, 1);
  }
  return pool;
}

std::uint32_t
source_allocate_game_arena_1_1(std::uint32_t bytes,
                               const SourceRawDatumPoolServices &services,
                               const SourceRawDatumPoolObserver &observe) {
  Storage a{services, observe};
  const auto offset = a.read(0x4ed654), base = a.read(0x4ed650);
  const auto rounded = (bytes + 3u) & ~3u;
  a.write(0x125e7f, 0x4ed654, offset + rounded);
  if (!a.read(0x566305, 1)) {
    for (std::uint32_t i = 0; i < 256; ++i) {
      auto value = i;
      for (unsigned bit = 0; bit < 8; ++bit)
        value = (value >> 1) ^ ((value & 1u) ? 0xedb88320u : 0u);
      a.write(0x165729, 0x566338 + 4 * i, value);
    }
    a.write(0x1656bb, 0x566305, 1, 1);
  }
  auto crc = a.read(0x4ed65c);
  for (unsigned i = 0; i < 4; ++i)
    crc = (crc >> 8) ^
          a.read(0x566338 + 4 * (((rounded >> (8 * i)) ^ crc) & 255u));
  a.write(0x1656ed, 0x4ed65c, crc);
  return base + offset;
}
} // namespace halo2
