#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
using VistaWidgetAnimationDescriptor = std::array<std::byte, 0x34>;

// Native links project the original +14 first-child / +18 next-sibling graph.
// The byte bank keeps the source kind +4 and animation index WORD +68.
// Nodes and their banks must remain alive across all callbacks.
struct VistaWidgetActivationNode {
  std::span<std::byte> bank;
  VistaWidgetActivationNode* first_child{};
  VistaWidgetActivationNode* next_sibling{};
};

struct VistaWidgetActivationServices {
  // Explicit caller stack bytes: 612604 ignores 612250's AL result. If the
  // producer returns before writing, these bytes remain input, never guessed
  // zeroes or a fabricated successful animation descriptor.
  std::function<VistaWidgetAnimationDescriptor(VistaWidgetActivationNode&, std::uint32_t)> descriptor_scratch;
  // Actual 612250 descriptor producer; UI globals, reason-specific authored
  // tables and focus/context services are separate owners. Return AL is
  // deliberately ignored by 612604.
  std::function<std::uint8_t(VistaWidgetActivationNode&, std::uint16_t,
      std::uint32_t, std::span<std::byte, 0x34>)> descriptor;
  std::function<std::uint32_t()> read_cached_ui_clock; // 6096AE, peek D71900.
  // Kind 2 dispatches virtual+18 and does not recurse into its children here.
  std::function<void(VistaWidgetActivationNode&, std::span<const std::byte, 0x34>)> kind2_animation;
};

// Complete 6115FE. The clock is read before any descriptor load or bank write.
// Only +34..+67 is changed. Descriptor storage may alias the byte bank: source
// reads/writes retain their order. The unsigned +50 clock addition wraps.
void vista_widget_animation_birth(std::span<std::byte> bank,
    std::span<const std::byte, 0x34> descriptor,
    const std::function<std::uint32_t()>& read_cached_ui_clock);

// Complete 612604 orchestration, including descriptor-before-kind dispatch,
// independent per-widget clock refresh and live linked-list traversal after
// each callback. The descriptor producer and kind2 virtual method stay explicit
// services. This is not a manager, renderer or live-screen implementation.
void vista_widget_activate(VistaWidgetActivationNode&, std::uint32_t reason,
    const VistaWidgetActivationServices&);
} // namespace halo2
