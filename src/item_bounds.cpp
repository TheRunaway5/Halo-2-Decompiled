#include "halo2/item_bounds.hpp"
#include <array>
#include <bit>
#include <cstring>
#include <stdexcept>

namespace halo2 {
namespace {
template<class T> T read(std::span<const std::byte> bytes, std::size_t offset) {
  if (offset + sizeof(T) > bytes.size()) throw std::runtime_error("Incomplete original bounds storage");
  T value; std::memcpy(&value, bytes.data() + offset, sizeof value); return value;
}
void store(std::span<std::byte> body, std::size_t offset, std::uint32_t value,
    std::uint32_t instruction, const SourceMatrixMathObserver &observer) {
  std::array<std::byte, 4> raw; std::memcpy(raw.data(), &value, 4);
  SourceMatrixMathStore event{instruction, offset,
      {body.begin() + offset, body.begin() + offset + 4}, {raw.begin(), raw.end()}};
  std::memcpy(body.data() + offset, raw.data(), 4); if (observer) observer(event);
}
} // namespace
bool source_item_bounds(std::span<std::byte> body,
    std::span<const std::byte> definition, const SourceItemBoundsInputs &input,
    const SourceMatrixMathObserver &observer) {
  if (read<std::uint32_t>(body, 0xb4) != 0xffffffffu)
    throw std::runtime_error("Original physical item bounds producer unavailable");
  const bool registered = (read<std::uint32_t>(body, 4) & 0x100u) != 0;
  const auto node_offset = read<std::int16_t>(body, 0x116);
  if (node_offset < 0 || unsigned(node_offset) + 0x34u > body.size())
    throw std::runtime_error("Original current item node bank unavailable");
  const auto matrix = std::span<const std::byte>(body).subspan(unsigned(node_offset), 0x34);
  const auto scale = read<float>(matrix, 0);
  std::array<float, 3> center{read<float>(definition, 8),
      read<float>(definition, 12), read<float>(definition, 16)};
  if (scale != input.one) for (auto &value : center) value = scale * value;
  constexpr std::array<std::uint32_t, 3> primary_stores{0xbeeff, 0xbef2b, 0xbef58};
  for (unsigned row = 0; row < 3; ++row) {
    const auto z = read<float>(matrix, 0x1c + row * 4) * center[2];
    const auto y = read<float>(matrix, 0x10 + row * 4) * center[1];
    const auto zy = z + y;
    const auto x = read<float>(matrix, 4 + row * 4) * center[0];
    const auto zyx = zy + x;
    const auto translated = zyx + read<float>(matrix, 0x28 + row * 4);
    store(body, 0x30 + row * 4, std::bit_cast<std::uint32_t>(translated), primary_stores[row], observer);
  }
  const auto radius = read<float>(body, 0xa0) * read<float>(definition, 4);
  store(body, 0x3c, std::bit_cast<std::uint32_t>(radius), 0xbef6a, observer);
  if (read<float>(definition, 0x20) > input.zero)
    throw std::runtime_error("Unmeasured original positive alternate bounds producer");
  constexpr std::array<std::uint32_t, 4> alternate_stores{0xbefc5, 0xbefcd, 0xbefd0, 0xbefd6};
  for (unsigned row = 0; row < 4; ++row)
    store(body, 0x50 + row * 4, read<std::uint32_t>(body, 0x30 + row * 4), alternate_stores[row], observer);
  const auto sphere = (read<std::uint16_t>(definition, 2) & 0x1000u) ? 0x50u : 0x30u;
  if (registered) {
    const auto dz = read<float>(body, sphere + 8) - read<float>(body, 0x48);
    const auto dx = read<float>(body, sphere) - read<float>(body, 0x40);
    const auto dy = read<float>(body, sphere + 4) - read<float>(body, 0x44);
    const auto zz = dz * dz; const auto xx = dx * dx; const auto zx = zz + xx;
    const auto yy = dy * dy; const auto distance_squared = zx + yy;
    const bool moved = distance_squared > input.squared_movement_limit;
    const bool same_radius = read<float>(body, 0x4c) == read<float>(body, sphere + 12);
    if (!moved && same_radius) return false;
    throw std::runtime_error("Original changed-sphere unregister/register producers unavailable");
  }
  constexpr std::array<std::uint32_t, 4> registration_stores{0xbf08d, 0xbf08f, 0xbf092, 0xbf095};
  for (unsigned row = 0; row < 4; ++row)
    store(body, 0x40 + row * 4, read<std::uint32_t>(body, sphere + row * 4), registration_stores[row], observer);
  return true;
}
} // namespace halo2
