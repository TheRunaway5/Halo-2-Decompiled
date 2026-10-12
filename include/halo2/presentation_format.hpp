#pragma once
#include <algorithm>
#include <optional>
#include <span>
#include <vulkan/vulkan.h>

namespace halo2 {
inline std::optional<VkSurfaceFormatKHR>
presentation_format(std::span<const VkSurfaceFormatKHR> formats) {
  // The Xbox A8R8G8B8 pipeline already emits display code values. An sRGB
  // attachment would encode them again and brighten the entire presentation.
  for (const auto format : {VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM})
    for (const auto &offered : formats)
      if (offered.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR &&
          (offered.format == format || offered.format == VK_FORMAT_UNDEFINED))
        return VkSurfaceFormatKHR{format, offered.colorSpace};
  return {};
}
} // namespace halo2
