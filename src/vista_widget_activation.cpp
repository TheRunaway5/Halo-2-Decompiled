#include "halo2/vista_widget_activation.hpp"
#include "halo2/cache.hpp"
#include <bit>

namespace halo2 {
namespace {
std::uint32_t read(std::span<const std::byte> bank, std::size_t offset, std::size_t size) {
  if (offset > bank.size() || size > bank.size() - offset)
    throw FormatError("Vista widget animation read exceeds its retained bank");
  std::uint32_t value{};
  for (std::size_t i = 0; i < size; ++i)
    value |= std::uint32_t(std::to_integer<std::uint8_t>(bank[offset + i])) << (8 * i);
  return value;
}
void write(std::span<std::byte> bank, std::size_t offset, std::uint32_t value, std::size_t size) {
  if (offset > bank.size() || size > bank.size() - offset)
    throw FormatError("Vista widget animation write exceeds its retained bank");
  for (std::size_t i = 0; i < size; ++i)
    bank[offset + i] = std::byte{std::uint8_t(value >> (8 * i))};
}
} // namespace

void vista_widget_animation_birth(std::span<std::byte> bank,
    std::span<const std::byte, 0x34> descriptor,
    const std::function<std::uint32_t()>& clock) {
  if (bank.size() < 0x68 || !clock)
    throw FormatError("Vista widget animation lacks its source bank or cached clock");
  const auto now = clock();
  write(bank, 0x34, read(descriptor, 0, 4), 4);
  write(bank, 0x38, read(descriptor, 4, 4), 4);
  write(bank, 0x3c, read(descriptor, 8, 2), 2);
  write(bank, 0x3e, read(descriptor, 0xa, 2), 2);
  const auto direction = std::bit_cast<std::int16_t>(std::uint16_t(read(descriptor, 0xc, 2)));
  write(bank, 0x40, direction < 0 ? 0xffffu : 1u, 2);
  write(bank, 0x42, read(descriptor, 0xe, 2), 2);
  write(bank, 0x44, read(descriptor, 0x10, 4), 4);
  const auto field14 = read(descriptor, 0x14, 4);
  write(bank, 0x4c, now, 4);
  write(bank, 0x48, field14, 4);
  write(bank, 0x50, read(descriptor, 0x10, 4) + now, 4);
  const auto field20 = read(descriptor, 0x20, 4);
  write(bank, 0x60, 0, 4);
  write(bank, 0x5c, 0, 4);
  write(bank, 0x58, 0, 4);
  write(bank, 0x54, field20, 4);
  write(bank, 0x64, 0x3f800000u, 4);
}

void vista_widget_activate(VistaWidgetActivationNode& node, std::uint32_t reason,
    const VistaWidgetActivationServices& services) {
  if (node.bank.size() < 0x6a || !services.descriptor_scratch || !services.descriptor)
    throw FormatError("Vista widget activation lacks its retained descriptor owner");
  const auto index = std::uint16_t(read(node.bank, 0x68, 2));
  auto descriptor = services.descriptor_scratch(node, reason);
  (void)services.descriptor(node, index, reason, descriptor);
  if (read(node.bank, 4, 4) == 2) {
    if (!services.kind2_animation)
      throw FormatError("Vista kind2 animation requires its virtual method");
    services.kind2_animation(node, descriptor);
    return;
  }
  vista_widget_animation_birth(node.bank, descriptor, services.read_cached_ui_clock);
  auto* child = node.first_child;
  while (child) {
    vista_widget_activate(*child, reason, services);
    child = child->next_sibling;
  }
}
} // namespace halo2
