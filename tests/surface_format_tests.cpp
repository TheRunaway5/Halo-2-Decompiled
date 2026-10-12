#include "halo2/presentation_format.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
using namespace halo2;
int main() {
  try {
    std::array<VkSurfaceFormatKHR, 4> formats{{
      {VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
      {VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
      {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
      {VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}}};
    const auto less = [](auto a, auto b) { return a.format < b.format; };
    std::sort(formats.begin(), formats.end(), less);
    unsigned permutations = 0;
    do {
      const auto chosen = presentation_format(formats);
      if (!chosen || chosen->format != VK_FORMAT_B8G8R8A8_UNORM)
        throw std::runtime_error("driver format order adds an sRGB output conversion");
      ++permutations;
    } while (std::next_permutation(formats.begin(), formats.end(),
                                  less));
    const std::array<VkSurfaceFormatKHR, 1> rgba{{
      {VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}}};
    if (presentation_format(rgba)->format != VK_FORMAT_R8G8B8A8_UNORM)
      throw std::runtime_error("RGBA display format unavailable");
    const std::array<VkSurfaceFormatKHR, 1> unrestricted{{
      {VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}}};
    if (presentation_format(unrestricted)->format != VK_FORMAT_B8G8R8A8_UNORM)
      throw std::runtime_error("unrestricted surface chooses the wrong transfer");
    const std::array<VkSurfaceFormatKHR, 1> srgb_only{{
      {VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}}};
    if (presentation_format(srgb_only) || presentation_format({}))
      throw std::runtime_error("unsupported surface silently changes display transfer");
    std::cout << "Display transfer permutations: " << permutations << '\n';
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
