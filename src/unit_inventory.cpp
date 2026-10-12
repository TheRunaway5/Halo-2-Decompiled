#include "halo2/unit_inventory.hpp"
#include <stdexcept>

namespace halo2 {
namespace {
constexpr std::uint32_t none = 0xffffffffu;
void require(bool available, const char *message) {
  if (!available) throw std::runtime_error(message);
}
std::span<std::byte> body(const UnitInventoryServices &services, std::uint32_t unit) {
  require(bool(services.unit_body), "Original inventory unit storage is unavailable");
  auto bytes = services.unit_body(unit);
  require(bytes.size() >= 0x238, "Truncated original inventory unit storage");
  return bytes;
}
std::uint32_t read(std::span<const std::byte> bytes, std::size_t at, unsigned count) {
  std::uint32_t result = 0;
  for (unsigned i = 0; i < count; ++i)
    result |= std::uint32_t(std::to_integer<unsigned>(bytes[at + i])) << (i * 8);
  return result;
}
void store(std::span<std::byte> bytes, std::size_t at, std::uint32_t value,
           unsigned count, std::uint32_t instruction,
           const UnitInventoryStoreObserver &observe) {
  const auto before = read(bytes, at, count);
  for (unsigned i = 0; i < count; ++i) bytes[at + i] = std::byte((value >> (i * 8)) & 0xff);
  if (observe) observe({instruction, before, read(bytes, at, count), at, count});
}
void dirty(const UnitInventoryServices &services, std::uint32_t entity, std::uint32_t mask) {
  if (entity == none) return;
  require(bool(services.network_dirty), "Original inventory network-dirty producer is unavailable");
  services.network_dirty(entity, mask);
}
}
void advance_source_inventory_sequence(std::uint32_t unit,
    const UnitInventoryServices &services, const UnitInventoryStoreObserver &observe) {
  require(bool(services.options_mode), "Original inventory options mode is unavailable");
  if (services.options_mode() == 4) return;
  auto bytes = body(services, unit);
  const std::uint16_t next = (std::to_integer<unsigned>(bytes[0x210]) + 1) & 0xf;
  store(bytes, 0x210, next, 2, 0xcfe9b, observe);
  store(bytes, 0x214, next, 2, 0xcfea2, observe);
  // The original reloads the header/body pointer before reading network+D4.
  dirty(services, read(body(services, unit), 0xd4, 4), 0x2000);
}
void set_source_inventory_slot(std::uint32_t unit, std::uint32_t item,
    std::int16_t slot, std::uint32_t removal_mode, const UnitInventoryServices &services,
    const UnitInventoryStoreObserver &observe) {
  require(slot >= 0 && slot < 4, "Original inventory slot is outside native storage");
  auto bytes = body(services, unit);
  const auto at = 0x218 + std::size_t(slot) * 4;
  if (item != none) {
    require(bool(services.assign_owner), "Original inventory owner producer is unavailable");
    services.assign_owner(item, unit);
    require(bool(services.remove_world_membership), "Original inventory world-removal producer is unavailable");
    services.remove_world_membership(item);
    // Preserve the body pointer used by the original two slot stores. Owning
    // services must retain that storage until this phase completes.
    store(bytes, at, item, 4, 0xcff0f, observe);
    store(bytes, 0x228 + std::size_t(slot) * 4, 0, 4, 0xcff16, observe);
  } else {
    const auto old_item = read(bytes, at, 4);
    store(bytes, at, none, 4, 0xcff30, observe);
    if (removal_mode != 4) {
      require(bool(services.release_item), "Original carried-item release producer is unavailable");
      services.release_item(unit, old_item, removal_mode);
    }
  }
  const auto mask = (std::uint32_t(1) << (unsigned(slot) + 0x12)) |
                    (std::uint32_t(1) << (unsigned(slot) + 0xe));
  dirty(services, read(body(services, unit), 0xd4, 4), mask);
  advance_source_inventory_sequence(unit, services, observe);
}
} // namespace halo2
