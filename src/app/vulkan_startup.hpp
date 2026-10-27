#pragma once
#include "halo2/macos/bundle.hpp"
#include "halo2/vulkan_portability.hpp"
#include <SDL.h>
#include <SDL_vulkan.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

// Asset-free native startup diagnostic. It exercises the executable's SDL
// window, Vulkan loader, presentation surface, device and real queue submits.
// Engine shader, map and gameplay acceptance remains a separate validation.
namespace halo2::vulkan_startup {
inline void check(VkResult result, const char *operation) {
    if(result!=VK_SUCCESS) throw std::runtime_error(std::string(operation)+": "+std::to_string(result));
}
struct Graphics {
    SDL_Window *window{};
    VkInstance instance{};
    VkSurfaceKHR surface{};
    VkDevice device{};
    VkSwapchainKHR swapchain{};
    VkCommandPool pool{};
    VkSemaphore acquired{}, rendered{};
    ~Graphics() {
        if(device) {
            vkDeviceWaitIdle(device);
            if(acquired) vkDestroySemaphore(device,acquired,nullptr);
            if(rendered) vkDestroySemaphore(device,rendered,nullptr);
            if(pool) vkDestroyCommandPool(device,pool,nullptr);
            if(swapchain) vkDestroySwapchainKHR(device,swapchain,nullptr);
            vkDestroyDevice(device,nullptr);
        }
        if(surface) vkDestroySurfaceKHR(instance,surface,nullptr);
        if(instance) vkDestroyInstance(instance,nullptr);
        if(window) SDL_DestroyWindow(window);
        SDL_Quit();
    }
};

inline nlohmann::json run() {
    constexpr unsigned frames=3;
    Graphics g;
    halo2::configure_bundled_moltenvk();
    if(SDL_Init(SDL_INIT_VIDEO)!=0) throw std::runtime_error(SDL_GetError());
    g.window=SDL_CreateWindow("Halo 2 Vulkan startup check",SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,640,360,SDL_WINDOW_VULKAN|SDL_WINDOW_ALLOW_HIGHDPI);
    if(!g.window) throw std::runtime_error(SDL_GetError());
    unsigned count{};
    if(!SDL_Vulkan_GetInstanceExtensions(g.window,&count,nullptr)) throw std::runtime_error(SDL_GetError());
    std::vector<const char *> extensions(count);
    if(!SDL_Vulkan_GetInstanceExtensions(g.window,&count,extensions.data())) throw std::runtime_error(SDL_GetError());
    std::uint32_t available_count{};
    check(vkEnumerateInstanceExtensionProperties(nullptr,&available_count,nullptr),"instance extension count");
    std::vector<VkExtensionProperties> available(available_count);
    check(vkEnumerateInstanceExtensionProperties(nullptr,&available_count,available.data()),"instance extensions");
    VkApplicationInfo application{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    application.pApplicationName="Halo 2 Vulkan startup check";
    application.apiVersion=VK_API_VERSION_1_1;
    VkInstanceCreateInfo instance_info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instance_info.pApplicationInfo=&application;
    instance_info.flags=halo2::enable_vulkan_portability_instance(available,extensions);
    instance_info.enabledExtensionCount=std::uint32_t(extensions.size());
    instance_info.ppEnabledExtensionNames=extensions.data();
    check(vkCreateInstance(&instance_info,nullptr,&g.instance),"create instance");
    if(!SDL_Vulkan_CreateSurface(g.window,g.instance,&g.surface)) throw std::runtime_error(SDL_GetError());
    std::uint32_t device_count{};
    check(vkEnumeratePhysicalDevices(g.instance,&device_count,nullptr),"GPU count");
    std::vector<VkPhysicalDevice> devices(device_count);
    check(vkEnumeratePhysicalDevices(g.instance,&device_count,devices.data()),"GPUs");
    VkPhysicalDevice physical{};
    std::uint32_t family{};
    for(auto candidate:devices) {
        std::uint32_t queues_count{};
        vkGetPhysicalDeviceQueueFamilyProperties(candidate,&queues_count,nullptr);
        std::vector<VkQueueFamilyProperties> queues(queues_count);
        vkGetPhysicalDeviceQueueFamilyProperties(candidate,&queues_count,queues.data());
        for(unsigned i=0;i<queues_count;++i) {
            VkBool32 present{};
            check(vkGetPhysicalDeviceSurfaceSupportKHR(candidate,i,g.surface,&present),"present support");
            constexpr auto required=VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT;
            if(present && (queues[i].queueFlags&required)==required) { physical=candidate;family=i;break; }
        }
        if(physical) break;
    }
    if(!physical) throw std::runtime_error("No GPU supports graphics, compute and window presentation");
    std::uint32_t extension_count{};
    check(vkEnumerateDeviceExtensionProperties(physical,nullptr,&extension_count,nullptr),"device extension count");
    std::vector<VkExtensionProperties> device_extensions(extension_count);
    check(vkEnumerateDeviceExtensionProperties(physical,nullptr,&extension_count,device_extensions.data()),"device extensions");
    std::vector<const char *> enabled{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    halo2::enable_vulkan_portability_device(device_extensions,enabled);
    const float priority=1;
    VkDeviceQueueCreateInfo queue_info{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queue_info.queueFamilyIndex=family;queue_info.queueCount=1;queue_info.pQueuePriorities=&priority;
    VkDeviceCreateInfo device_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    device_info.queueCreateInfoCount=1;device_info.pQueueCreateInfos=&queue_info;
    device_info.enabledExtensionCount=std::uint32_t(enabled.size());device_info.ppEnabledExtensionNames=enabled.data();
    check(vkCreateDevice(physical,&device_info,nullptr,&g.device),"create device");
    VkQueue queue{};vkGetDeviceQueue(g.device,family,0,&queue);
    VkSurfaceCapabilitiesKHR capabilities{};
    check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical,g.surface,&capabilities),"surface capabilities");
    if(!(capabilities.supportedUsageFlags&VK_IMAGE_USAGE_TRANSFER_DST_BIT))
        throw std::runtime_error("Swapchain does not support transfer clear");
    std::uint32_t format_count{};
    check(vkGetPhysicalDeviceSurfaceFormatsKHR(physical,g.surface,&format_count,nullptr),"surface format count");
    std::vector<VkSurfaceFormatKHR> formats(format_count);
    check(vkGetPhysicalDeviceSurfaceFormatsKHR(physical,g.surface,&format_count,formats.data()),"surface formats");
    if(formats.empty()) throw std::runtime_error("Surface has no formats");
    auto selected=formats.front();
    if(selected.format==VK_FORMAT_UNDEFINED) selected.format=VK_FORMAT_B8G8R8A8_UNORM;
    auto extent=capabilities.currentExtent;
    if(extent.width==UINT32_MAX) {
        int width{},height{};SDL_Vulkan_GetDrawableSize(g.window,&width,&height);
        extent={std::clamp(std::uint32_t(width),capabilities.minImageExtent.width,capabilities.maxImageExtent.width),
                std::clamp(std::uint32_t(height),capabilities.minImageExtent.height,capabilities.maxImageExtent.height)};
    }
    VkSwapchainCreateInfoKHR swapchain_info{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchain_info.surface=g.surface;swapchain_info.minImageCount=capabilities.minImageCount+1;
    if(capabilities.maxImageCount) swapchain_info.minImageCount=std::min(swapchain_info.minImageCount,capabilities.maxImageCount);
    swapchain_info.imageFormat=selected.format;swapchain_info.imageColorSpace=selected.colorSpace;
    swapchain_info.imageExtent=extent;swapchain_info.imageArrayLayers=1;
    swapchain_info.imageUsage=VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    swapchain_info.imageSharingMode=VK_SHARING_MODE_EXCLUSIVE;swapchain_info.preTransform=capabilities.currentTransform;
    for(auto alpha:{VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
                    VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR})
        if(capabilities.supportedCompositeAlpha&alpha) {swapchain_info.compositeAlpha=alpha;break;}
    swapchain_info.presentMode=VK_PRESENT_MODE_FIFO_KHR;swapchain_info.clipped=VK_TRUE;
    check(vkCreateSwapchainKHR(g.device,&swapchain_info,nullptr,&g.swapchain),"create swapchain");
    std::uint32_t image_count{};
    check(vkGetSwapchainImagesKHR(g.device,g.swapchain,&image_count,nullptr),"swapchain image count");
    std::vector<VkImage> images(image_count);
    check(vkGetSwapchainImagesKHR(g.device,g.swapchain,&image_count,images.data()),"swapchain images");
    VkCommandPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool_info.queueFamilyIndex=family;pool_info.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    check(vkCreateCommandPool(g.device,&pool_info,nullptr,&g.pool),"create command pool");
    VkCommandBufferAllocateInfo allocate{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocate.commandPool=g.pool;allocate.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;allocate.commandBufferCount=1;
    VkCommandBuffer buffer{};
    check(vkAllocateCommandBuffers(g.device,&allocate,&buffer),"allocate command buffer");
    VkSemaphoreCreateInfo semaphore_info{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    check(vkCreateSemaphore(g.device,&semaphore_info,nullptr,&g.acquired),"acquire semaphore");
    check(vkCreateSemaphore(g.device,&semaphore_info,nullptr,&g.rendered),"render semaphore");
    for(unsigned frame=0;frame<frames;++frame) {
        SDL_PumpEvents();
        std::uint32_t image{};
        const auto acquired=vkAcquireNextImageKHR(g.device,g.swapchain,10'000'000'000ull,g.acquired,VK_NULL_HANDLE,&image);
        if(acquired!=VK_SUBOPTIMAL_KHR)check(acquired,"acquire image");
        check(vkResetCommandBuffer(buffer,0),"reset command buffer");
        VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        check(vkBeginCommandBuffer(buffer,&begin),"begin commands");
        VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.oldLayout=VK_IMAGE_LAYOUT_UNDEFINED;barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
        barrier.image=images[image];barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
        barrier.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(buffer,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,
            0,nullptr,0,nullptr,1,&barrier);
        const VkClearColorValue color{{.035f,.075f,.16f,1.f}};
        vkCmdClearColorImage(buffer,images[image],VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,&color,1,&barrier.subresourceRange);
        barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;barrier.dstAccessMask=0;
        vkCmdPipelineBarrier(buffer,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,0,
            0,nullptr,0,nullptr,1,&barrier);
        check(vkEndCommandBuffer(buffer),"end commands");
        const VkPipelineStageFlags stage=VK_PIPELINE_STAGE_TRANSFER_BIT;
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submit.waitSemaphoreCount=1;submit.pWaitSemaphores=&g.acquired;submit.pWaitDstStageMask=&stage;
        submit.commandBufferCount=1;submit.pCommandBuffers=&buffer;
        submit.signalSemaphoreCount=1;submit.pSignalSemaphores=&g.rendered;
        check(vkQueueSubmit(queue,1,&submit,VK_NULL_HANDLE),"submit clear");
        VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        present.waitSemaphoreCount=1;present.pWaitSemaphores=&g.rendered;
        present.swapchainCount=1;present.pSwapchains=&g.swapchain;present.pImageIndices=&image;
        const auto presented=vkQueuePresentKHR(queue,&present);
        if(presented!=VK_SUBOPTIMAL_KHR)check(presented,"present clear");
        check(vkDeviceWaitIdle(g.device),"complete frame");
    }
    VkPhysicalDeviceProperties properties{};vkGetPhysicalDeviceProperties(physical,&properties);
#ifdef _WIN32
    constexpr auto platform="windows";
#elif defined(__APPLE__)
    constexpr auto platform="macos";
#else
    constexpr auto platform="linux";
#endif
    return {{"schema","H2_VULKAN_STARTUP_V1"},{"platform",platform},
        {"gpu",properties.deviceName},{"api_version",properties.apiVersion},
        {"presented_frames",frames},{"width",extent.width},{"height",extent.height},
        {"engine_gameplay_validated",false}};
}
} // namespace halo2::vulkan_startup
