#pragma once
#include "halo2/cinematic_crossfade.hpp"
#include <filesystem>
#include <memory>
#include <vulkan/vulkan.h>
namespace halo2 {
struct CinematicCrossfadeRenderContext {
  VkDevice device{};VkPhysicalDevice physical{};
  VkFormat color_format{};VkRenderPass color_pass{};
  std::filesystem::path shader_directory;
};
struct CinematicCrossfadeRenderTarget {
  VkCommandBuffer command{};VkImage color{};VkExtent2D extent{};
  VkImageLayout layout{};VkRenderPass color_pass{},continuation_pass{};
  VkFramebuffer color_framebuffer{},continuation_framebuffer{};
};
//13B70/35190/22050: one fixed640x480 BGRA8 capture shared by the completed
// views. Reconfiguring the swapchain preserves this independent history image.
class CinematicCrossfadeRender {
public:
  explicit CinematicCrossfadeRender(CinematicCrossfadeRenderContext);
  ~CinematicCrossfadeRender();
  void reconfigure(VkFormat,VkRenderPass);
  // Call after the previous GPU fence; the uniform is shared across frames.
  void prepare(const CinematicCrossfadeFrame &);
  // Ends/resumes the active main render pass. Returns actual capture admission.
  bool record(const CinematicCrossfadeRenderTarget &);
private:
  struct Impl;std::unique_ptr<Impl> impl_;
};
}
