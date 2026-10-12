#include "halo2/raw_datum_pool.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
std::uint32_t
source_raw_datum_allocate(std::uint32_t pool,
                          const SourceRawDatumPoolServices &services,
                          const SourceRawDatumPoolObserver &observer) {
  const auto memory = [&](std::uint32_t address, unsigned size) {
    if (!services.memory)
      throw std::runtime_error("Original raw datum-pool storage unavailable");
    const auto data = services.memory(address, size);
    if (data.size() != size)
      throw std::runtime_error("Original raw datum-pool extent unavailable");
    return data;
  };
  const auto read = [&](std::uint32_t address, unsigned size = 4) {
    const std::span<const std::byte> data =
        services.read ? services.read(address, size) : memory(address, size);
    std::uint32_t value = 0;
    if (data.size() != size)
      throw std::runtime_error(
          "Original raw datum-pool read extent unavailable");
    for (unsigned i = 0; i < size; ++i)
      value |= std::to_integer<std::uint32_t>(data[i]) << (8 * i);
    return value;
  };
  const auto write = [&](std::uint32_t pc, std::uint32_t address,
                         std::uint32_t value, unsigned size = 4) {
    const auto before = read(address, size);
    const auto data = memory(address, size);
    if (size < 4)
      value &= (1u << (size * 8)) - 1;
    for (unsigned i = 0; i < size; ++i)
      data[i] = std::byte(value >> (8 * i));
    if (observer)
      observer({pc, address, before, value, size});
  };
  const auto signed_word = [](std::uint32_t value) {
    return std::bit_cast<std::int32_t>(value);
  };
  const auto bitmap_word = [](std::uint32_t value) {
    return (value >> 5) | ((value & 0x80000000u) ? 0xf8000000u : 0);
  };
  auto cursor = read(pool + 0x34);
  const auto high = read(pool + 0x38);
  std::uint32_t index = 0xffffffffu;
  if (signed_word(cursor) < signed_word(high)) {
    const auto bitmap = read(pool + 0x48);
    for (;;) {
      if (!(read(bitmap + bitmap_word(cursor) * 4) & (1u << (cursor & 31)))) {
        index = cursor;
        break;
      }
      ++cursor;
      if (signed_word(cursor) >= signed_word(read(pool + 0x38)))
        break;
    }
  }
  if (index == 0xffffffffu) {
    if (signed_word(high) >= signed_word(read(pool + 0x20)) ||
        high == 0xffffffffu)
      return 0xffffffffu;
    index = high;
  }
  const auto element = read(pool + 0x44) + read(pool + 0x24) * index;
  const auto bitmap = read(pool + 0x48) + bitmap_word(index) * 4;
  write(0x16d32e, bitmap, read(bitmap) | (1u << (index & 31)));
  const auto count = read(pool + 0x3c), retained_high = read(pool + 0x38),
             next = index + 1;
  write(0x16d33c, pool + 0x3c, count + 1);
  write(0x16d33f, pool + 0x34, next);
  if (signed_word(retained_high) <= signed_word(index))
    write(0x16d344, pool + 0x38, next);
  const auto size = read(pool + 0x24);
  for (std::uint32_t n = 0; n < (size >> 2); ++n)
    write(0x16d353, element + n * 4, 0);
  for (std::uint32_t n = 0; n < (size & 3); ++n)
    write(0x16d35a, element + (size & ~3u) + n, 0, 1);
  write(0x16d360, element, read(pool + 0x40, 2), 2);
  write(0x16d363, pool + 0x40, read(pool + 0x40, 2) + 1, 2);
  if (read(pool + 0x40, 2) == 0xffffu)
    write(0x16d36e, pool + 0x40, 0x8000, 2);
  return (read(element, 2) << 16) | index;
}
} // namespace halo2
