// Diagnostic SDL/Vulkan presentation loop. No game assets or shaders are replaced.
#include <SDL.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
using Clock=std::chrono::steady_clock;
using Json=nlohmann::json;
void check(VkResult r,const char* what){if(r!=VK_SUCCESS)throw std::runtime_error(std::string(what)+": "+std::to_string(r));}
double ms(Clock::time_point a,Clock::time_point b){return std::chrono::duration<double,std::milli>(b-a).count();}
int main(int argc,char** argv){try {
    if(argc!=4)throw std::runtime_error("MODE(fifo/mailbox/immediate) SECONDS FRESH_OUTPUT");
    const std::string mode=argv[1];const auto seconds=std::stod(argv[2]);const std::filesystem::path out=argv[3];
    if(std::filesystem::exists(out))throw std::runtime_error("Output exists");std::filesystem::create_directories(out);
    VkPresentModeKHR requested=mode=="fifo"?VK_PRESENT_MODE_FIFO_KHR:mode=="mailbox"?VK_PRESENT_MODE_MAILBOX_KHR:VK_PRESENT_MODE_IMMEDIATE_KHR;
    if(mode!="fifo" && mode!="mailbox" && mode!="immediate")throw std::runtime_error("Unknown mode");
    if(SDL_Init(SDL_INIT_VIDEO))throw std::runtime_error(SDL_GetError());
    auto* window=SDL_CreateWindow("pStencil presentation diagnostic",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,1600,900,
        SDL_WINDOW_VULKAN|SDL_WINDOW_RESIZABLE|SDL_WINDOW_ALLOW_HIGHDPI|SDL_WINDOW_SHOWN);
    if(!window)throw std::runtime_error(SDL_GetError());
    unsigned count{};if(!SDL_Vulkan_GetInstanceExtensions(window,&count,nullptr))throw std::runtime_error(SDL_GetError());
    std::vector<const char*> extensions(count);SDL_Vulkan_GetInstanceExtensions(window,&count,extensions.data());
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.apiVersion=VK_API_VERSION_1_0;app.pApplicationName="pStencil presentation diagnostic";
    const char* layer="VK_LAYER_KHRONOS_validation";
    VkInstanceCreateInfo instance_info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};instance_info.pApplicationInfo=&app;
    instance_info.enabledExtensionCount=count;instance_info.ppEnabledExtensionNames=extensions.data();
    instance_info.enabledLayerCount=1;instance_info.ppEnabledLayerNames=&layer;
    VkInstance instance{};check(vkCreateInstance(&instance_info,nullptr,&instance),"create instance");
    VkSurfaceKHR surface{};if(!SDL_Vulkan_CreateSurface(window,instance,&surface))throw std::runtime_error(SDL_GetError());
    vkEnumeratePhysicalDevices(instance,&count,nullptr);std::vector<VkPhysicalDevice> devices(count);vkEnumeratePhysicalDevices(instance,&count,devices.data());
    VkPhysicalDevice physical{};unsigned family{};
    for(auto candidate:devices){unsigned n{};vkGetPhysicalDeviceQueueFamilyProperties(candidate,&n,nullptr);std::vector<VkQueueFamilyProperties> queues(n);vkGetPhysicalDeviceQueueFamilyProperties(candidate,&n,queues.data());
        for(unsigned i=0;i<n;++i){VkBool32 supported{};vkGetPhysicalDeviceSurfaceSupportKHR(candidate,i,surface,&supported);
            if(supported && queues[i].queueFlags&VK_QUEUE_GRAPHICS_BIT){physical=candidate;family=i;break;}}
        if(physical)break;}
    if(!physical)throw std::runtime_error("No graphics/present queue");VkPhysicalDeviceProperties properties{};vkGetPhysicalDeviceProperties(physical,&properties);
    float priority=1;VkDeviceQueueCreateInfo queue_info{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};queue_info.queueFamilyIndex=family;queue_info.queueCount=1;queue_info.pQueuePriorities=&priority;
    const char* swapchain_extension=VK_KHR_SWAPCHAIN_EXTENSION_NAME;VkDeviceCreateInfo device_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    device_info.queueCreateInfoCount=1;device_info.pQueueCreateInfos=&queue_info;device_info.enabledExtensionCount=1;device_info.ppEnabledExtensionNames=&swapchain_extension;
    VkDevice device{};check(vkCreateDevice(physical,&device_info,nullptr,&device),"create device");VkQueue queue{};vkGetDeviceQueue(device,family,0,&queue);
    VkSurfaceCapabilitiesKHR caps{};check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical,surface,&caps),"surface capabilities");
    check(vkGetPhysicalDeviceSurfaceFormatsKHR(physical,surface,&count,nullptr),"format count");std::vector<VkSurfaceFormatKHR> formats(count);check(vkGetPhysicalDeviceSurfaceFormatsKHR(physical,surface,&count,formats.data()),"formats");
    auto format=formats.at(0);for(auto f:formats)if(f.format==VK_FORMAT_B8G8R8A8_UNORM){format=f;break;}
    check(vkGetPhysicalDeviceSurfacePresentModesKHR(physical,surface,&count,nullptr),"mode count");std::vector<VkPresentModeKHR> modes(count);check(vkGetPhysicalDeviceSurfacePresentModesKHR(physical,surface,&count,modes.data()),"modes");
    if(std::find(modes.begin(),modes.end(),requested)==modes.end())throw std::runtime_error("Requested mode unavailable");
    int width{},height{};SDL_Vulkan_GetDrawableSize(window,&width,&height);auto extent=caps.currentExtent;
    if(extent.width==UINT32_MAX)extent={unsigned(width),unsigned(height)};
    auto images_count=caps.minImageCount+1;if(caps.maxImageCount)images_count=std::min(images_count,caps.maxImageCount);
    VkCompositeAlphaFlagBitsKHR alpha=VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    for(auto a:{VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR})if(caps.supportedCompositeAlpha&a){alpha=a;break;}
    if(!(caps.supportedUsageFlags&VK_IMAGE_USAGE_TRANSFER_DST_BIT))throw std::runtime_error("No transfer-destination presentation");
    VkSwapchainCreateInfoKHR create{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};create.surface=surface;create.minImageCount=images_count;create.imageFormat=format.format;create.imageColorSpace=format.colorSpace;
    create.imageExtent=extent;create.imageArrayLayers=1;create.imageUsage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT;create.imageSharingMode=VK_SHARING_MODE_EXCLUSIVE;
    create.preTransform=caps.currentTransform;create.compositeAlpha=alpha;create.presentMode=requested;create.clipped=VK_TRUE;
    VkSwapchainKHR swapchain{};check(vkCreateSwapchainKHR(device,&create,nullptr,&swapchain),"swapchain");
    check(vkGetSwapchainImagesKHR(device,swapchain,&count,nullptr),"image count");std::vector<VkImage> images(count);check(vkGetSwapchainImagesKHR(device,swapchain,&count,images.data()),"images");
    std::vector<bool> initialized(count);std::vector<VkSemaphore> finished(count);VkSemaphoreCreateInfo sem{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};for(auto& s:finished)check(vkCreateSemaphore(device,&sem,nullptr,&s),"finished semaphore");
    VkCommandPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};pool_info.queueFamilyIndex=family;pool_info.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VkCommandPool pool{};check(vkCreateCommandPool(device,&pool_info,nullptr,&pool),"pool");
    std::array<VkCommandBuffer,2> commands{};VkCommandBufferAllocateInfo allocate{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};allocate.commandPool=pool;allocate.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;allocate.commandBufferCount=2;
    check(vkAllocateCommandBuffers(device,&allocate,commands.data()),"commands");std::array<VkSemaphore,2> ready{};std::array<VkFence,2> fences{};
    VkFenceCreateInfo fence_info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};fence_info.flags=VK_FENCE_CREATE_SIGNALED_BIT;
    for(unsigned i=0;i<2;++i){check(vkCreateSemaphore(device,&sem,nullptr,&ready[i]),"ready semaphore");check(vkCreateFence(device,&fence_info,nullptr,&fences[i]),"fence");}
    std::ofstream timings(out/"timings.tsv");timings<<"frame\tinterval_ms\tfence_ms\tacquire_ms\tpresent_ms\twindow_flags\n";
    Json report{{"scope","Minimal SDL/Vulkan presentation diagnostic; no game visual parity claim"},{"device",properties.deviceName},{"video_driver",SDL_GetCurrentVideoDriver()},
        {"mode",mode},{"available_modes",modes},{"image_count",count},{"extent",{extent.width,extent.height}},{"format",format.format},{"surface_min_images",caps.minImageCount}};
    const auto start=Clock::now();auto previous=start;unsigned frame{};bool running=true;
    while(running && ms(start,Clock::now())<seconds*1000){SDL_Event event;while(SDL_PollEvent(&event))if(event.type==SDL_QUIT)running=false;
        const auto begin=Clock::now();const unsigned slot=frame%2;check(vkWaitForFences(device,1,&fences[slot],VK_TRUE,UINT64_MAX),"wait fence");auto waited=Clock::now();
        unsigned image{};check(vkAcquireNextImageKHR(device,swapchain,UINT64_MAX,ready[slot],VK_NULL_HANDLE,&image),"acquire");auto acquired=Clock::now();
        check(vkResetFences(device,1,&fences[slot]),"reset fence");check(vkResetCommandBuffer(commands[slot],0),"reset command");
        VkCommandBufferBeginInfo begin_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};check(vkBeginCommandBuffer(commands[slot],&begin_info),"begin");
        VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};barrier.oldLayout=initialized[image]?VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:VK_IMAGE_LAYOUT_UNDEFINED;barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;barrier.image=images[image];barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};barrier.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(commands[slot],VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
        const VkClearColorValue color{{.025f,.04f,float(frame%60)/600.f,1}};vkCmdClearColorImage(commands[slot],images[image],VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,&color,1,&barrier.subresourceRange);
        barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;barrier.dstAccessMask=0;
        vkCmdPipelineBarrier(commands[slot],VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,0,0,nullptr,0,nullptr,1,&barrier);check(vkEndCommandBuffer(commands[slot]),"end");
        VkPipelineStageFlags stage=VK_PIPELINE_STAGE_TRANSFER_BIT;VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.waitSemaphoreCount=1;submit.pWaitSemaphores=&ready[slot];submit.pWaitDstStageMask=&stage;
        submit.commandBufferCount=1;submit.pCommandBuffers=&commands[slot];submit.signalSemaphoreCount=1;submit.pSignalSemaphores=&finished[image];check(vkQueueSubmit(queue,1,&submit,fences[slot]),"submit");
        VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};present.waitSemaphoreCount=1;present.pWaitSemaphores=&finished[image];present.swapchainCount=1;present.pSwapchains=&swapchain;present.pImageIndices=&image;
        auto before_present=Clock::now();check(vkQueuePresentKHR(queue,&present),"present");auto end=Clock::now();initialized[image]=true;
        timings<<frame<<'\t'<<ms(previous,begin)<<'\t'<<ms(begin,waited)<<'\t'<<ms(waited,acquired)<<'\t'<<ms(before_present,end)<<'\t'<<SDL_GetWindowFlags(window)<<'\n';timings.flush();previous=begin;++frame;
    }
    report["frames"]=frame;report["seconds"]=ms(start,Clock::now())/1000;std::ofstream(out/"result.json")<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';
    check(vkDeviceWaitIdle(device),"idle");for(auto s:finished)vkDestroySemaphore(device,s,nullptr);for(unsigned i=0;i<2;++i){vkDestroySemaphore(device,ready[i],nullptr);vkDestroyFence(device,fences[i],nullptr);}
    vkDestroyCommandPool(device,pool,nullptr);vkDestroySwapchainKHR(device,swapchain,nullptr);vkDestroyDevice(device,nullptr);vkDestroySurfaceKHR(instance,surface,nullptr);vkDestroyInstance(instance,nullptr);SDL_DestroyWindow(window);SDL_Quit();return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
