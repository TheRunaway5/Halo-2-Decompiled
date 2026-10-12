#include "halo2/unit_weapon_selection.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
void require(bool condition, const char *message) {
  if (!condition) throw std::runtime_error(message);
}
std::uint32_t read(std::span<const std::byte> bytes, std::size_t at, unsigned size) {
  std::uint32_t value = 0;
  for (unsigned i = 0; i < size; ++i)
    value |= std::uint32_t(std::to_integer<unsigned>(bytes[at + i])) << (i * 8);
  return value;
}
}
std::int16_t select_source_inventory_weapon(std::uint32_t unit,
    std::uint32_t mode, std::int16_t previous, std::int16_t direction,
    const UnitWeaponSelectionServices &services) {
  require(previous >= -1 && previous < 4, "Original weapon-selection start is outside native storage");
  require(direction >= -1 && direction <= 1, "Original weapon-selection direction is outside measured ABI");
  require(bool(services.unit_body), "Original weapon-selection unit storage is unavailable");
  const auto body = services.unit_body(unit);
  require(body.size() >= 0x238, "Truncated original weapon-selection unit storage");
  const int start = previous == -1 ? 0 : (int(previous) + 4 + int(direction)) % 4;
  int slot = start;
  std::int16_t selected = -1;
  bool selected_flag = false;
  std::int32_t selected_timestamp = 0;
  do {
    const auto item = read(body, 0x218 + std::size_t(slot) * 4, 4);
    if (item != 0xffffffffu) {
      require(bool(services.accepts_weapon), "Original CEAF0 equipped-weapon capability is unavailable");
      const bool accepted = services.accepts_weapon(unit, item, mode);
      // CF382's capability runs before CF386's other-hand exclusion.
      const auto other = std::bit_cast<std::int8_t>(std::uint8_t(read(body, 0x213, 1)));
      if (accepted && slot != other) {
        // CF3A3 reads the signed timestamp before looking up the definition.
        const auto timestamp = std::bit_cast<std::int32_t>(read(body, 0x228 + std::size_t(slot) * 4, 4));
        require(bool(services.weapon_definition), "Original weapon-selection definition is unavailable");
        const auto definition = services.weapon_definition(item);
        require(definition.size() >= 0x130, "Truncated original weapon-selection definition");
        const bool flag = (read(definition, 0x12c, 4) & 8) != 0;
        if (selected == -1 || (!selected_flag && (flag || (direction == 0 && selected_timestamp < timestamp)))) {
          selected = std::int16_t(slot);
          selected_flag = flag;
          selected_timestamp = timestamp;
        }
      }
    }
    slot = (slot + (direction < 0 ? 3 : 1)) % 4;
  } while (slot != start);
  return selected;
}
} // namespace halo2
