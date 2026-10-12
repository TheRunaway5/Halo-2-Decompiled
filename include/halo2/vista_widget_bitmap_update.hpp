#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>

namespace halo2 {
struct VistaWidgetBitmapUpdateServices {
  // Caller-owned source32 registry. Reads/writes are live, checked and borrowed.
  // Bitmap runtime metadata must be writable; an immutable asset bank alone is
  // not the original loaded-resource state. No pointer narrowing or zero reads.
  std::function<std::uint32_t(std::uint32_t,std::size_t)> read;
  std::function<void(std::uint32_t,std::uint32_t,std::size_t)> write;
  std::function<void(std::uint32_t widget)> ambient_update; // Actual612CD8.
  // Required only when reached by complete665986. These are real backend
  // services, not successful residency/read/allocation defaults.
  std::function<std::uint32_t(std::uint32_t descriptor)> resource_size; //4647DA
  std::function<std::uint32_t(std::uint32_t bytes)> allocate; //687C86
  std::function<std::uint32_t(const std::array<std::uint32_t,7>&)> read_async; //464E6F
  std::function<void(const std::array<std::uint32_t,4>&)> read_sync; //464D01
  std::function<void(std::uint32_t token,std::uint32_t priority)> reprioritize; //432BE3
  std::function<void(std::uint32_t flag,std::uint32_t pump,std::uint32_t control)> wait; //432CAE
  std::function<std::uint32_t(std::uint32_t frame,std::uint32_t zero)> prepare; //503A2F
  std::function<void(std::uint32_t frame)> finalize; //675010
  std::function<void(std::uint32_t allocation)> free; //687C23
  std::function<void(std::uint32_t frame)> touch; //65EFC2
};

// Complete61C941: override or loaded-cache low-WORD datum/frame selection.
// NONE block pointers and signed frame/count tests retain their source meaning;
// no tag salt/group validation, fallback thumbnail or frame-count repair.
std::uint32_t vista_widget_bitmap_frame(std::uint32_t widget,
    const VistaWidgetBitmapUpdateServices&);
// Complete665986 and665C37 wrapper. Return is the captured resource handle even
// if the touch provider changes it. Provider exceptions retain write prefixes.
std::uint32_t vista_widget_bitmap_resource(std::uint32_t frame,
    std::uint32_t flags,std::uint32_t out_flag,const VistaWidgetBitmapUpdateServices&);
std::uint32_t vista_widget_bitmap_resource_request(std::uint32_t frame,
    std::uint32_t z_bits,const VistaWidgetBitmapUpdateServices&);
// Complete61C9AC. First cached-clock read precedes actual ambient update;
// elapsed subtraction wraps, period uses x87 truncation, fill uses caller x87
// rounding and UV uses original ordered SSE operations. Masked FP contexts only.
// Source DIV-zero and provably non-progressing UV loops are explicit rejected
// admissions after their exact completed source prefix, never clamped/repaired.
void vista_widget_bitmap_update(std::uint32_t widget,
    const VistaWidgetBitmapUpdateServices&);
} // namespace halo2
