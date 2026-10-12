#include "halo2/source_lift_contact.hpp"
#include <bit>

namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &m, std::uint32_t p,
                   unsigned n = 4) {
  std::array<std::byte, 4> b{};
  m.read(p, std::span(b).first(n));
  std::uint32_t value{};
  for (unsigned i = 0; i < n; ++i)
    value |= std::to_integer<std::uint32_t>(b[i]) << (i * 8u);
  return value;
}
std::int32_t signed_byte(std::uint32_t v) {
  return std::bit_cast<std::int8_t>(std::uint8_t(v));
}
} // namespace
bool source_lift_contact_allowed(const SourcePhysicsContactMemory &m,
                                 std::uint32_t shape, std::uint32_t own,
                                 std::uint32_t other,
                                 SourcePhysicsContactImage image) {
  if (other == 0xffffffffu)
    return false;
  const bool later = image == SourcePhysicsContactImage::TitleUpdate15;
  const auto component_bank =
      read(m, read(m, later ? 0x52be5cu : 0x526248u) + 0x44);
  const auto own_object = read(m, component_bank + (own & 0xffffu) * 0xa0u + 8);
  const auto objects = read(m, read(m, later ? 0x4ecbd0u : 0x4e78d0u) + 0x44);
  const auto object = read(m, objects + (own_object & 0xffffu) * 12u + 8);
  const auto definition = read(m, object);
  const auto tags = read(m, later ? 0x4f0414u : 0x4eb114u);
  const auto model =
      read(m, read(m, tags + (definition & 0xffffu) * 16u + 8) + 0x38);
  const auto physics =
      read(m, read(m, tags + (model & 0xffffu) * 16u + 8) + 0x24);
  const auto phmo = read(m, tags + (physics & 0xffffu) * 16u + 8);
  const auto shapes = read(m, phmo + 0x44);
  const auto rule_index = std::bit_cast<std::int16_t>(
      std::uint16_t(read(m, shapes + shape * 12u + 8, 2)));
  const auto rules = read(m, phmo + 0x2c);
  const auto counterpart = component_bank + (other & 0xffffu) * 0xa0u;
  const auto counterpart_object = read(m, counterpart + 8);
  const auto rule = rules + std::uint32_t(std::int32_t(rule_index) * 0x68);
  const auto header = objects + (counterpart_object & 0xffffu) * 12u;
  const auto counterpart_body = read(m, header + 8);
  const auto size = signed_byte(read(m, counterpart + 0x1c, 1));
  const auto kind = read(m, header + 3, 1);
  const auto kind_bit = 1u << (kind & 31u);
  bool player{};
  if (kind_bit & 3u)
    player = read(m, read(m, header + 8) + 0x13c) != 0xffffffffu;
  bool control{};
  if (kind_bit & 1u)
    control = (read(m, counterpart_body + 0x10a, 1) & 4u) != 0;
  if (size) {
    if (signed_byte(read(m, rule + 4, 1)) > size)
      return false;
    if (signed_byte(read(m, rule + 5, 1)) < size)
      return false;
  }
  const auto flags = read(m, rule);
  if (flags & (player ? 8u : 16u))
    return false;
  if (control && (flags & 0x08000000u))
    return false;
  return !(flags & (1u << ((kind + 5u) & 31u)));
}
} // namespace halo2
