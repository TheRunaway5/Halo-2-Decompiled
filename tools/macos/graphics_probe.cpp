#include "halo2/macos/bundle.hpp"
#include "halo2/macos/vulkan_math.hpp"
#include "halo2/vulkan_portability.hpp"
#include <SDL.h>
#include <SDL_vulkan.h>
#include <algorithm>
#include <dlfcn.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>

namespace {
void check(VkResult result, const char *operation) {
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

std::string run(unsigned frames) {
    Graphics g;
    halo2::configure_bundled_moltenvk();
    if(SDL_Init(SDL_INIT_VIDEO)!=0) throw std::runtime_error(SDL_GetError());
    g.window=SDL_CreateWindow("Halo 2 macOS graphics check",SDL_WINDOWPOS_CENTERED,
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
    application.pApplicationName="Halo 2 macOS graphics check";
    application.apiVersion=VK_API_VERSION_1_1;
    VkInstanceCreateInfo instance_info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instance_info.pApplicationInfo=&application;
    instance_info.flags=halo2::enable_vulkan_portability_instance(available,extensions);
    instance_info.enabledExtensionCount=std::uint32_t(extensions.size());
    instance_info.ppEnabledExtensionNames=extensions.data();
    halo2::macos::SourceShaderMath source_math;
    source_math.configure(instance_info,available,extensions);
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
        check(vkAcquireNextImageKHR(g.device,g.swapchain,UINT64_MAX,g.acquired,VK_NULL_HANDLE,&image),"acquire image");
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
        check(vkQueuePresentKHR(queue,&present),"present clear");
        check(vkDeviceWaitIdle(g.device),"complete frame");
    }
    VkPhysicalDeviceProperties properties{};vkGetPhysicalDeviceProperties(physical,&properties);
    VkPhysicalDeviceFeatures features{};vkGetPhysicalDeviceFeatures(physical,&features);
    VkFormatProperties depth{};vkGetPhysicalDeviceFormatProperties(physical,VK_FORMAT_D24_UNORM_S8_UINT,&depth);
    constexpr auto depth_flags=VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT|
        VK_FORMAT_FEATURE_TRANSFER_SRC_BIT|VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
    VkImageFormatProperties depth_image{};
    const auto depth_result=vkGetPhysicalDeviceImageFormatProperties(physical,VK_FORMAT_D24_UNORM_S8_UINT,
        VK_IMAGE_TYPE_2D,VK_IMAGE_TILING_OPTIMAL,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|
        VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT,0,&depth_image);
    const bool d24=(depth.optimalTilingFeatures&depth_flags)==depth_flags && depth_result==VK_SUCCESS &&
        (depth_image.sampleCounts&VK_SAMPLE_COUNT_1_BIT);
#if defined(__aarch64__) || defined(__arm64__)
    constexpr auto architecture="arm64";
#else
    constexpr auto architecture="x86_64";
#endif
    std::ostringstream report;
    Dl_info moltenvk_image{}, sdl_image{};
    if(!dladdr(reinterpret_cast<const void *>(&vkGetInstanceProcAddr),&moltenvk_image) ||
       !dladdr(reinterpret_cast<const void *>(&SDL_CreateWindow),&sdl_image))
        throw std::runtime_error("Could not identify the loaded graphics libraries");
    report << "{\n  \"schema\": \"H2_MACOS_GRAPHICS_BRINGUP_V1\",\n  \"architecture\": " << std::quoted(architecture)
        << ",\n  \"loaded_moltenvk\": " << std::quoted(moltenvk_image.dli_fname)
        << ",\n  \"loaded_sdl\": " << std::quoted(sdl_image.dli_fname)
        << ",\n  \"gpu\": " << std::quoted(properties.deviceName)
        << ",\n  \"presented_frames\": " << frames << ",\n  \"host_page_bytes\": " << sysconf(_SC_PAGESIZE)
        << ",\n  \"portability_enumeration\": " << (instance_info.flags?"true":"false")
        << ",\n  \"portability_subset\": " << (enabled.size()>1?"true":"false")
        << ",\n  \"source_d24s8_supported\": " << (d24?"true":"false")
        << ",\n  \"depth_clamp\": " << (features.depthClamp?"true":"false")
        << ",\n  \"geometry_shader\": " << (features.geometryShader?"true":"false")
        << ",\n  \"engine_port_complete\": false\n}\n";
    return report.str();
}
}
int main(int argc,char **argv) {
    try {
        unsigned frames=3;
        std::string report_path;
        for(int i=1;i<argc;++i) {
            const std::string arg=argv[i];
            if((arg=="--frames" || arg=="--report") && i+1<argc) {
                if(arg=="--frames") frames=unsigned(std::stoul(argv[++i]));
                else report_path=argv[++i];
            } else throw std::runtime_error("Usage: graphics-check [--frames 1..120] [--report path]");
        }
        if(!frames || frames>120) throw std::runtime_error("Frame count must be 1..120");
        const auto report=run(frames);
        if(!report_path.empty()) {
            std::ofstream file(report_path);
            if(!(file<<report)) throw std::runtime_error("Cannot write graphics check report");
        }
        std::cout<<report;
        return 0;
    } catch(const std::exception &error) {std::cerr<<error.what()<<'\n';return 1;}
}
