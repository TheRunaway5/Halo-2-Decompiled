#pragma once
#include <algorithm>
#include <cstdlib>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>
#include <vulkan/vulkan.h>

namespace halo2::pstencil {
// Recording-only override. Ordinary game presentation retains FIFO. Some
// output-less Xwayland surfaces advertise immediate presentation but release
// FIFO images at roughly one-second intervals, even for an asset-free clear.
inline VkPresentModeKHR capture_present_mode(std::string_view requested,
                                             std::span<const VkPresentModeKHR> offered) {
    if(requested.empty() || requested=="fifo")return VK_PRESENT_MODE_FIFO_KHR;
    if(requested!="immediate")throw std::runtime_error("Unknown pStencil capture presentation mode");
    if(std::find(offered.begin(),offered.end(),VK_PRESENT_MODE_IMMEDIATE_KHR)==offered.end())
        throw std::runtime_error("This surface does not advertise immediate capture presentation");
    return VK_PRESENT_MODE_IMMEDIATE_KHR;
}
inline VkPresentModeKHR capture_present_mode(VkPhysicalDevice physical,VkSurfaceKHR surface) {
    const auto* value=std::getenv("HALO2_PSTENCIL_CAPTURE_PRESENT_MODE");
    const std::string_view requested=value?value:"";
    if(requested.empty() || requested=="fifo")return VK_PRESENT_MODE_FIFO_KHR;
    if(requested!="immediate")throw std::runtime_error("Unknown pStencil capture presentation mode");
    std::uint32_t count{};std::vector<VkPresentModeKHR> offered;VkResult result;
    do {
        if(vkGetPhysicalDeviceSurfacePresentModesKHR(physical,surface,&count,nullptr)!=VK_SUCCESS)
            throw std::runtime_error("Cannot query capture presentation mode count");
        offered.resize(count);
        result=vkGetPhysicalDeviceSurfacePresentModesKHR(physical,surface,&count,offered.data());
    }while(result==VK_INCOMPLETE);
    if(result!=VK_SUCCESS)throw std::runtime_error("Cannot query capture presentation modes");
    offered.resize(count);return capture_present_mode(requested,offered);
}
} // namespace halo2::pstencil
