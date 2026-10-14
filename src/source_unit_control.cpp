#include "halo2/source_unit_control.hpp"
#include "halo2/native_unit_interaction.hpp"
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace halo2 {
namespace {
constexpr std::uint32_t none = 0xffffffffu;
std::uint32_t read(std::span<const std::uint8_t> bytes, unsigned offset,
                   unsigned count = 4) {
  if (offset > bytes.size() || count > bytes.size()-offset)
    throw std::invalid_argument("Unit control field exceeds its actual body");
  std::uint32_t result = 0;
  for (unsigned i = 0; i < count; ++i)
    result |= std::uint32_t(bytes[offset+i]) << (8*i);
  return result;
}
void write(std::span<std::uint8_t> bytes, unsigned offset,
           std::uint32_t value, unsigned count = 4) {
  if (offset > bytes.size() || count > bytes.size()-offset)
    throw std::invalid_argument("Unit control field exceeds its actual body");
  for (unsigned i = 0; i < count; ++i)
    bytes[offset+i] = std::uint8_t(value >> (8*i));
}
bool is_unit(unsigned type) { return type < 2; }
const NativeVehicleSeatDefinition &seat(
    const NativeUnitInteractionDefinition &definition, std::int16_t index) {
  if (index < 0 || std::size_t(index) >= definition.seats.size())
    throw std::invalid_argument("Unit control seat exceeds its authored bank");
  return definition.seats[index];
}
}

SourceUnitControl::SourceUnitControl(SourceUnitControlServices services)
    : services_(std::move(services)) {
  if (!services_.object || !services_.object_type || !services_.definition ||
      !services_.refresh_item_effect || !services_.driver_changed)
    throw std::invalid_argument("Unit control requires every live owner service");
}
std::span<std::uint8_t>
SourceUnitControl::object(std::uint32_t datum, unsigned minimum_size) const {
  auto bytes = services_.object(datum);
  if (bytes.size() < minimum_size)
    throw std::invalid_argument("Unit control received an incomplete source body");
  return bytes;
}
void SourceUnitControl::set_active(std::uint32_t datum, bool requested) {
  const auto unit = object(datum, 0x228);
  bool active = requested;
  if (read(unit, 0x12c) != none || read(unit, 0x130) != none ||
      read(unit, 0x13c) != none)
    active = true;
  if (read(unit, 0x10a, 1) & 4)
    active = false;
  // Original stores bit1 before bit0, then reads each inventory slot live.
  auto flags = read(unit, 0x134);
  flags = active ? flags | 2u : flags & ~2u;
  write(unit, 0x134, flags);
  flags = active ? flags | 1u : flags & ~1u;
  write(unit, 0x134, flags);
  for (unsigned i = 0; i < 4; ++i)
    if (const auto item = read(unit, 0x218+i*4); item != none)
      refresh_item(item);
  refresh_seats(datum);
}
void SourceUnitControl::refresh_item(std::uint32_t datum) {
  const auto item = object(datum, 0x16d);
  auto flags = read(item, 0x12c, 1);
  bool held_by_player = false;
  if (flags & 1)
    held_by_player = read(object(read(item, 0x154), 0x140), 0x13c) != none;
  flags = held_by_player ? flags | 8u : flags & ~8u;
  write(item, 0x12c, flags, 1);
  if (services_.object_type(datum) == 2 && (read(item, 0x16c, 1) & 0x40))
    services_.refresh_item_effect(datum);
}
void SourceUnitControl::refresh_seats(std::uint32_t datum) {
  std::vector<std::uint32_t> parents;
  refresh_seats(datum, parents);
}
void SourceUnitControl::refresh_seats(std::uint32_t datum,
                                     std::vector<std::uint32_t> &parents) {
  if (parents.size() >= 2048 ||
      std::find(parents.begin(), parents.end(), datum) != parents.end())
    throw std::invalid_argument("Unit control hierarchy is cyclic");
  parents.push_back(datum);
  const auto unit = object(datum, 0x250);
  const auto &definition = services_.definition(read(unit, 0));
  const auto previous_driver = read(unit, 0x248);
  write(unit, 0x248, none);
  write(unit, 0x24c, none);
  const auto parent = read(unit, 0x14);
  if ((definition.unit_flags & (1u << 26)) && parent != none &&
      read(unit, 0x12c) == none && is_unit(services_.object_type(parent)))
    write(unit, 0x24c, parent);
  for (unsigned pass = 0; pass < 2; ++pass) {
    unsigned visited = 0;
    for (auto child_id = read(unit, 0x10); child_id != none;) {
      if (++visited > 2048)
        throw std::invalid_argument("Unit control sibling chain is cyclic");
      const auto child = object(child_id, 0xab);
      if (is_unit(read(child, 0xaa, 1))) {
        const auto seat_index = std::int16_t(read(child, 0x1fc, 2));
        bool driver = false, gunner = false, assign = false;
        if (seat_index == -1) {
          const auto &child_definition = services_.definition(read(child, 0));
          gunner = (child_definition.unit_flags & (1u << 25)) != 0;
          assign = true;
        } else {
          const auto &authored_seat = seat(definition, seat_index);
          const auto flags = authored_seat.flags;
          const bool grouped = (flags & (1u << 11)) != 0;
          if (!grouped || (flags & (1u << 13))) {
            driver = (flags & 4) != 0;
            gunner = (flags & 8) != 0;
            if ((flags & (1u << 18)) && read(child, 0x13c) != none)
              gunner = true;
            if ((pass == 0 && !grouped) || (pass == 1 && grouped)) {
              const auto state = std::int16_t(read(child, 0x346, 2));
              if (state < 0)
                throw std::invalid_argument("Unit control needs the real seat-state block");
              assign = pass == 0 ? !(read(child, unsigned(state)+8, 1) & 1)
                                 : (read(child, unsigned(state)+4) & 0x80000000u) != 0;
            }
            if (grouped && authored_seat.boarding_seat != -1) {
              const auto other_flags = seat(definition, authored_seat.boarding_seat).flags;
              driver |= (other_flags & 4) != 0;
              gunner |= (other_flags & 8) != 0;
            }
          }
        }
        if (assign) {
          if (driver && !(read(unit, 0x134) & 2) && read(unit, 0x248) == none) {
            write(unit, 0x248, child_id);
            if (gunner && read(unit, 0x24c) == none)
              write(unit, 0x24c, child_id);
          } else if (gunner &&
                     (read(unit, 0x24c) == none ||
                      read(unit, 0x24c) == read(unit, 0x248)))
            write(unit, 0x24c, child_id);
        }
        refresh_seats(child_id, parents);
      }
      child_id = read(child, 0xc);
    }
  }
  if (previous_driver != read(unit, 0x248))
    services_.driver_changed(datum);
  parents.pop_back();
}
} // namespace halo2
