#pragma once
#include <cstdint>
#include <functional>

namespace halo2 {
// Actual 66B808 scheduling. Services supply the live E4AF24/E4AFDC owners,
// Sleep, and complete 66A512 presentation. This routine owns no OS thread,
// Vulkan device, map task, cancellation, or fabricated progress publisher.
struct VistaLoadingThreadServices {
  std::function<void(std::uint32_t)> sleep;
  std::function<std::uint8_t()> mode;
  std::function<std::uint8_t(std::uint8_t)> present;
  std::function<std::uint8_t()> fade_active;
};
// Ordinary draws ignore presentation's AL. After observing fade-active,
// present immediately once, then sleep33/present until AL is zero. In the
// fade phase the source never polls fade-active again. Returns literal zero.
std::uint32_t vista_loading_thread_run(const VistaLoadingThreadServices&);
} // namespace halo2
