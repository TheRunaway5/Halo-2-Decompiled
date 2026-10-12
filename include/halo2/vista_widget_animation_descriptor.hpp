#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
// Source addresses are DWORD identities in a retained registry, never narrowed
// host pointers. The caller keeps all widget/cache/manager banks alive for the
// complete call, including callbacks. Reads may have source-visible effects.
struct VistaWidgetAnimationDescriptorServices {
  std::function<std::uint32_t(std::uint32_t address, std::size_t bytes)> read;
  std::function<void(std::uint32_t address, std::uint32_t value)> write_dword;
  // The exact original virtual slot: +1C duration, +38 channel, +34 manager kind.
  // Slot +38 is called before +34. No implicit missing-widget defaults.
  std::function<std::uint32_t(std::uint32_t widget, std::uint32_t slot)> invoke_virtual;
};

// Complete 612250 and its source-bank leaves 60BB1D/60BB89/60BBAC,40809B,
// 60C688/60C701/60F815,612152/609EFD,613C51,609EDE/63913F and6096AE.
// Descriptor is a nullable borrowed buffer: empty means original NULL; otherwise
// at least 0x34 bytes are required. Early returns can leave it untouched or
// partially initialized. Return value is source AL, not a fabricated descriptor.
// Undefined original reads (for example NONE animation bank + nonzero index)
// remain registry admission failures; this owner supplies no substitute bytes.
std::uint8_t vista_widget_animation_descriptor(std::uint32_t widget,
    std::uint32_t animation_index, std::uint32_t reason, std::span<std::byte> descriptor,
    const VistaWidgetAnimationDescriptorServices&);
} // namespace halo2
