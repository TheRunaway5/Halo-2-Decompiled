#include "halo2/death_camera_binding.hpp"
#include <bit>

namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,
                   std::uint32_t address, unsigned size = 4) {
  std::array<std::byte, 4> bytes{};
  memory.read(address, std::span(bytes).first(size));
  std::uint32_t result = 0;
  for (unsigned i = 0; i < size; ++i)
    result |= std::to_integer<std::uint32_t>(bytes[i]) << (8 * i);
  return result;
}
std::int32_t signed_word(std::uint32_t value) {
  return std::bit_cast<std::int32_t>(value);
}
std::int16_t signed_short(std::uint32_t value) {
  return std::bit_cast<std::int16_t>(std::uint16_t(value));
}
} // namespace

std::optional<DeathCameraTarget>
source_death_camera_target(const SourcePhysicsContactMemory &memory,
                           std::uint32_t pool, std::uint32_t datum) {
  if (datum == 0xffffffffu)
    return {};
  const auto index = datum & 0xffffu;
  if (std::int32_t(index) >= signed_word(read(memory, pool + 0x38)))
    return {};
  const auto header =
      read(memory, pool + 0x44) + index * read(memory, pool + 0x24);
  const auto salt = signed_short(read(memory, header, 2));
  if (!salt || salt != signed_short(datum >> 16))
    return {};
  // BBCB0 is called with mask FFFFFFFF, admitting every header type.
  const auto body = read(memory, header + 8);
  if (!body)
    return {};
  const auto node =
      body +
      std::uint32_t(std::int32_t(signed_short(read(memory, body + 0x116, 2))));
  DeathCameraTarget target;
  target.node0 = {std::bit_cast<float>(read(memory, node + 0x28)),
                  std::bit_cast<float>(read(memory, node + 0x2c)),
                  std::bit_cast<float>(read(memory, node + 0x30))};
  target.type = std::uint8_t(read(memory, body + 0xaa, 1));
  // The source uses BT rather than an equality test on this byte.
  if ((target.type & 31u) == 0)
    target.biped_flags = std::uint16_t(read(memory, body + 0x348, 2));
  return target;
}

std::vector<DeathCameraPlayer>
source_death_camera_players(const SourcePhysicsContactMemory &memory,
                            std::uint32_t pool) {
  std::vector<DeathCameraPlayer> players;
  const auto high_water = signed_word(read(memory, pool + 0x38));
  for (std::int32_t index = 0; index < high_water; ++index) {
    const auto occupied = read(memory, read(memory, pool + 0x48) +
                                           (std::uint32_t(index) >> 5) * 4);
    if (!(occupied & (1u << (std::uint32_t(index) & 31))))
      continue;
    const auto row = read(memory, pool + 0x44) +
                     std::uint32_t(index) * read(memory, pool + 0x24);
    players.push_back({(read(memory, row, 2) << 16) | std::uint32_t(index),
                       read(memory, row + 0x2c), read(memory, row + 0x30),
                       std::bit_cast<std::int8_t>(
                           std::uint8_t(read(memory, row + 0xc0, 1)))});
  }
  return players;
}
} // namespace halo2
