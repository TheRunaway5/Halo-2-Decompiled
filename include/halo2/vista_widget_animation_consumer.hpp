#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>

namespace halo2 {
struct VistaWidgetAnimationConsumerServices {
  // Source32 registry identities, never narrowed host pointers. Writes may be
  // observed by later reads/callbacks; banks remain alive through callbacks.
  // Reads access resident memory; they do not invoke bank-mutating services.
  std::function<std::uint32_t(std::uint32_t,std::size_t)> read;
  std::function<void(std::uint32_t,std::uint32_t,std::size_t)> write;
  std::function<void(std::uint32_t,std::uint32_t)> activate; // 612604
  std::function<void(std::uint32_t)> dismiss; // 609920
  // Actual child class virtual+0C. The generic wrapper does not replace class
  // overrides (button color/focus, bitmap spin, screen update) with612CD8.
  std::function<void(std::uint32_t)> update_child;
};

// Original612758 finite raw-key update. Uses cached D71900 independently of the
// caller's now, signed frame/count WORDs, wrapping DWORD clocks, source endpoint
// policies, and individual SSE scalar operations under the caller's MXCSR
// (masked exceptions required). No fallback interpolation on non-SSE hosts.
// Raw frames use stride20 with alpha+4 and XYZ+8/+C/+10. Leading DWORD is unused
// by this owner. Count1 IDIV faults and a repeated zero-step loop state are
// explicit admission failures, never normalized to another animation. A finite
// zero-step endpoint can still dismiss/reactivate as in the source. Memory faults are
// registry failures; activation/dismiss remain actual caller services.
void vista_widget_animation_update(std::uint32_t widget,std::uint32_t now,
    const VistaWidgetAnimationConsumerServices&);

// Complete612CD8: independent cached-clock read before612758, then live direct
// child traversal. It reads the current child's next link after its callback.
void vista_widget_animation_ambient_update(std::uint32_t widget,
    const VistaWidgetAnimationConsumerServices&);
} // namespace halo2
