#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "halo2/temporal_upscaler.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cstring>
#include <functional>
#include <unordered_set>
#include <cstdlib>
using namespace halo2;
// Only the statically linked application allocations are wrapped. SDK shared
// libraries resolve Vulkan themselves, so their private images stay outside
// this set. Keep every diagnostic red; annotate ownership for the small report.
#ifdef HALO2_UPSCALER_IMAGE_OWNERSHIP
static std::unordered_set<std::uint64_t> application_images;
extern "C" VkResult __real_vkCreateImage(VkDevice,const VkImageCreateInfo*,const VkAllocationCallbacks*,VkImage*);
extern "C" VkResult __wrap_vkCreateImage(VkDevice device,const VkImageCreateInfo* create,const VkAllocationCallbacks* allocator,VkImage* image) {
    const auto result=__real_vkCreateImage(device,create,allocator,image);
    if(result==VK_SUCCESS)application_images.insert(reinterpret_cast<std::uintptr_t>(*image));
    return result;
}
extern "C" void __real_vkDestroyImage(VkDevice,VkImage,const VkAllocationCallbacks*);
extern "C" void __wrap_vkDestroyImage(VkDevice device,VkImage image,const VkAllocationCallbacks* allocator) {
    application_images.erase(reinterpret_cast<std::uintptr_t>(image));__real_vkDestroyImage(device,image,allocator);
}
#endif
namespace {
unsigned validation_errors{};
void check(bool pass,const char* message){if(!pass)throw std::runtime_error(message);}
void vkcheck(VkResult v){if(v!=VK_SUCCESS)throw std::runtime_error("Vulkan "+std::to_string(v));}
VKAPI_ATTR VkBool32 VKAPI_CALL validation(VkDebugUtilsMessageSeverityFlagBitsEXT,VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT* d,void*){
    ++validation_errors;
#ifdef HALO2_UPSCALER_IMAGE_OWNERSHIP
    const std::string message=d->pMessage;const auto at=message.find("expects VkImage 0x");
    if(at!=std::string::npos) {
        const auto image=std::strtoull(message.c_str()+at+16,nullptr,16);
        std::cerr<<"Diagnostic image owner: "<<(application_images.contains(image)?"application":"SDK private")<<'\n';
    }
#endif
    std::cerr<<d->pMessage<<'\n';return VK_FALSE;
}
struct GPU {
    VkInstance instance{};VkDebugUtilsMessengerEXT debug{};VkPhysicalDevice physical{};VkDevice device{};VkQueue queue{};
    unsigned family{};VkCommandPool pool{};VkCommandBuffer command{};
    GPU(){
        auto names=NativeUpscaler::instance_extensions();names.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        std::vector<const char*> extensions;for(const auto& n:names)extensions.push_back(n.c_str());
        VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.pApplicationName="Halo2 upscaling acceptance";app.apiVersion=VK_API_VERSION_1_3;
        const char* layer="VK_LAYER_KHRONOS_validation";
        VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};ci.pApplicationInfo=&app;ci.enabledLayerCount=1;ci.ppEnabledLayerNames=&layer;
        ci.enabledExtensionCount=extensions.size();ci.ppEnabledExtensionNames=extensions.data();vkcheck(vkCreateInstance(&ci,nullptr,&instance));
        VkDebugUtilsMessengerCreateInfoEXT di{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
        di.messageSeverity=VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        di.messageType=VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;di.pfnUserCallback=validation;
        vkcheck(reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,"vkCreateDebugUtilsMessengerEXT"))(instance,&di,nullptr,&debug));
        unsigned count{};vkcheck(vkEnumeratePhysicalDevices(instance,&count,nullptr));std::vector<VkPhysicalDevice> devices(count);
        vkcheck(vkEnumeratePhysicalDevices(instance,&count,devices.data()));physical=devices.at(0);
        for(auto candidate:devices){VkPhysicalDeviceProperties p{};vkGetPhysicalDeviceProperties(candidate,&p);if(p.deviceType==VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)physical=candidate;}
        vkGetPhysicalDeviceQueueFamilyProperties(physical,&count,nullptr);std::vector<VkQueueFamilyProperties> queues(count);
        vkGetPhysicalDeviceQueueFamilyProperties(physical,&count,queues.data());
        while(family<count && (queues[family].queueFlags&(VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT))!=(VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT))++family;
        check(family<count,"No graphics + compute queue");float priority=1;
        VkDeviceQueueCreateInfo qi{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};qi.queueFamilyIndex=family;qi.queueCount=1;qi.pQueuePriorities=&priority;
        names=NativeUpscaler::device_extensions(instance,physical);extensions.clear();for(const auto& n:names)extensions.push_back(n.c_str());
        VkPhysicalDeviceFeatures features{};features.shaderStorageImageExtendedFormats=VK_TRUE;
        VkDeviceCreateInfo dc{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};dc.queueCreateInfoCount=1;dc.pQueueCreateInfos=&qi;dc.pEnabledFeatures=&features;
        VkPhysicalDeviceBufferDeviceAddressFeatures address{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES};address.bufferDeviceAddress=VK_TRUE;dc.pNext=&address;
        dc.enabledExtensionCount=extensions.size();dc.ppEnabledExtensionNames=extensions.data();vkcheck(vkCreateDevice(physical,&dc,nullptr,&device));
        vkGetDeviceQueue(device,family,0,&queue);
        VkCommandPoolCreateInfo pc{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};pc.queueFamilyIndex=family;pc.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        vkcheck(vkCreateCommandPool(device,&pc,nullptr,&pool));
        VkCommandBufferAllocateInfo allocate{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};allocate.commandPool=pool;allocate.commandBufferCount=1;
        vkcheck(vkAllocateCommandBuffers(device,&allocate,&command));
    }
    ~GPU(){if(device){vkDeviceWaitIdle(device);if(pool)vkDestroyCommandPool(device,pool,nullptr);vkDestroyDevice(device,nullptr);}
        if(debug)reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,"vkDestroyDebugUtilsMessengerEXT"))(instance,debug,nullptr);
        if(instance)vkDestroyInstance(instance,nullptr);}
    unsigned memory(unsigned bits,VkMemoryPropertyFlags flags){VkPhysicalDeviceMemoryProperties p{};vkGetPhysicalDeviceMemoryProperties(physical,&p);
        for(unsigned i=0;i<p.memoryTypeCount;++i)if((bits&(1u<<i)) && (p.memoryTypes[i].propertyFlags&flags)==flags)return i;throw std::runtime_error("Memory unavailable");}
    void commands(const std::function<void(VkCommandBuffer)>& f){vkcheck(vkResetCommandBuffer(command,0));VkCommandBufferBeginInfo b{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        vkcheck(vkBeginCommandBuffer(command,&b));f(command);vkcheck(vkEndCommandBuffer(command));VkSubmitInfo s{VK_STRUCTURE_TYPE_SUBMIT_INFO};s.commandBufferCount=1;s.pCommandBuffers=&command;
        vkcheck(vkQueueSubmit(queue,1,&s,{}));vkcheck(vkQueueWaitIdle(queue));}
};
struct Image {
    GPU& gpu;VkImage image{};VkImageView view{};VkDeviceMemory memory{};VkExtent2D size;VkImageAspectFlags aspect;
    Image(GPU& g,VkExtent2D size,VkFormat format,bool depth,bool storage=false):gpu(g),size(size),aspect(depth?(format==VK_FORMAT_D32_SFLOAT?VK_IMAGE_ASPECT_DEPTH_BIT:VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT):VK_IMAGE_ASPECT_COLOR_BIT){
        VkImageCreateInfo c{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};c.imageType=VK_IMAGE_TYPE_2D;c.format=format;c.extent={size.width,size.height,1};c.mipLevels=c.arrayLayers=1;
        c.samples=VK_SAMPLE_COUNT_1_BIT;c.tiling=VK_IMAGE_TILING_OPTIMAL;c.usage=VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|
            (depth?VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT:VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
        if(storage)c.usage|=VK_IMAGE_USAGE_STORAGE_BIT;
        vkcheck(vkCreateImage(g.device,&c,nullptr,&image));VkMemoryRequirements r{};vkGetImageMemoryRequirements(g.device,image,&r);
        VkMemoryAllocateInfo a{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};a.allocationSize=r.size;a.memoryTypeIndex=g.memory(r.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        vkcheck(vkAllocateMemory(g.device,&a,nullptr,&memory));vkcheck(vkBindImageMemory(g.device,image,memory,0));
        VkImageViewCreateInfo v{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};v.image=image;v.viewType=VK_IMAGE_VIEW_TYPE_2D;v.format=format;v.subresourceRange={depth?VK_IMAGE_ASPECT_DEPTH_BIT:VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
        vkcheck(vkCreateImageView(g.device,&v,nullptr,&view));}
    ~Image(){vkDestroyImageView(gpu.device,view,nullptr);vkDestroyImage(gpu.device,image,nullptr);vkFreeMemory(gpu.device,memory,nullptr);}
    void barrier(VkCommandBuffer c,VkImageLayout old,VkImageLayout next){VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};b.image=image;b.oldLayout=old;b.newLayout=next;
        b.subresourceRange={aspect,0,1,0,1};b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
        b.srcAccessMask=old==VK_IMAGE_LAYOUT_UNDEFINED?0:VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT;b.dstAccessMask=VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT;
        vkCmdPipelineBarrier(c,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,0,0,nullptr,0,nullptr,1,&b);}
};
// Minimal SDK-only reproduction. No game shaders, motion reconstruction,
// depth conversion or temporal compositor participate in this dispatch.
void native_only(GPU& g) {
    constexpr VkExtent2D input{320,240},output{640,480};
    Image color(g,input,VK_FORMAT_R16G16B16A16_SFLOAT,false),depth(g,input,VK_FORMAT_D32_SFLOAT,true),
        motion(g,input,VK_FORMAT_R16G16_SFLOAT,false),
        result(g,output,VK_FORMAT_R16G16B16A16_SFLOAT,false,true);
    NativeUpscaler native(g.instance,g.physical,g.device,g.queue,g.family,input,output,Upscaler::DLSS45,UpscalingQuality::Performance);
    g.commands([&](VkCommandBuffer command) {
        const VkClearColorValue zero{};
        for(auto* image:{&color,&motion}) {
            image->barrier(command,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            const VkImageSubresourceRange range{image->aspect,0,1,0,1};
            vkCmdClearColorImage(command,image->image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,&zero,1,&range);
            image->barrier(command,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
        depth.barrier(command,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        const VkClearDepthStencilValue value{.5f,0};const VkImageSubresourceRange range{depth.aspect,0,1,0,1};
        vkCmdClearDepthStencilImage(command,depth.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,&value,1,&range);
        depth.barrier(command,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        result.barrier(command,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_GENERAL);
        native.dispatch({command,{color.image,color.view,VK_FORMAT_R16G16B16A16_SFLOAT},
            {depth.image,depth.view,VK_FORMAT_D32_SFLOAT},{motion.image,motion.view,VK_FORMAT_R16G16_SFLOAT},
            {},{result.image,result.view,VK_FORMAT_R16G16B16A16_SFLOAT},
            0,0,1.f/60,.05f,10000.f,1.2f,true});
    });
    check(validation_errors==0,"SDK-only DLSS Performance dispatch reported validation diagnostics");
    std::cout<<"SDK-only first DLSS Performance frame: zero validation diagnostics\n";
}
void run(GPU& g,Upscaler method,UpscalingQuality quality,VkExtent2D output,const char* shaders){
    const auto [w,h]=upscaling_render_size(output.width,output.height,method,quality);VkExtent2D input{w,h};
    Image color(g,input,VK_FORMAT_R8G8B8A8_UNORM,false),depth(g,input,VK_FORMAT_D24_UNORM_S8_UINT,true),result(g,output,VK_FORMAT_R8G8B8A8_UNORM,false);
    TemporalUpscaler temporal(g.instance,g.physical,g.device,g.queue,g.family,input,output,method,quality,shaders,depth.image,depth.view);
    for(unsigned frame=0;frame<4;++frame)g.commands([&](VkCommandBuffer c){
        color.barrier(c,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        const VkClearColorValue value{{.3f,.5f,.7f,1.f}};const VkImageSubresourceRange range{VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
        vkCmdClearColorImage(c,color.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,&value,1,&range);
        color.barrier(c,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        depth.barrier(c,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        const VkClearDepthStencilValue d{frame%2?2.f/1024.f:(10000.f/(10000.f-.05f)-.05f*10000.f/((10000.f-.05f)*2.f)),0};const VkImageSubresourceRange dr{depth.aspect,0,1,0,1};
        vkCmdClearDepthStencilImage(c,depth.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,&d,1,&dr);
        depth.barrier(c,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        temporal.begin(c);TemporalView view;view.rectangle={{0,0},input};view.w_depth=frame%2;
        view.matrix=glm::perspective(1.2f,float(w)/h,.05f,10000.f);view.foreground_matrix=view.matrix;
        std::array<Vertex,3> triangle{};triangle[0].position={-.5f,-.5f,-2};triangle[1].position={.5f,-.5f,-2};triangle[2].position={0,.5f,-2};
        for(auto& vertex:triangle)vertex.position.x+=frame*.02f;
        view.vertices=triangle;view.foreground=triangle;temporal.view(c,0,view);
        view.rectangle.extent.width=w/2;temporal.view(c,1,view);view.rectangle.offset.x=w/2;view.rectangle.extent.width=w-w/2;temporal.view(c,2,view);
        temporal.present(c,color.image,color.view,result.image,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    });
    check(temporal.frames()==4,"Vendor frames not dispatched");
    VkBuffer buffer{};VkDeviceMemory memory{};void* mapped{};const auto bytes=VkDeviceSize(output.width)*output.height*4;
    struct Cleanup{GPU& g;VkBuffer& b;VkDeviceMemory& m;void*& p;~Cleanup(){if(p)vkUnmapMemory(g.device,m);if(b)vkDestroyBuffer(g.device,b,nullptr);if(m)vkFreeMemory(g.device,m,nullptr);}} cleanup{g,buffer,memory,mapped};
    VkBufferCreateInfo b{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};b.size=bytes;b.usage=VK_BUFFER_USAGE_TRANSFER_DST_BIT;vkcheck(vkCreateBuffer(g.device,&b,nullptr,&buffer));
    VkMemoryRequirements requirements{};vkGetBufferMemoryRequirements(g.device,buffer,&requirements);VkMemoryAllocateInfo a{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};a.allocationSize=requirements.size;
    a.memoryTypeIndex=g.memory(requirements.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);vkcheck(vkAllocateMemory(g.device,&a,nullptr,&memory));vkcheck(vkBindBufferMemory(g.device,buffer,memory,0));
    g.commands([&](VkCommandBuffer c){result.barrier(c,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={output.width,output.height,1};
        vkCmdCopyImageToBuffer(c,result.image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,buffer,1,&copy);});
    vkcheck(vkMapMemory(g.device,memory,0,bytes,0,&mapped));const auto* pixels=static_cast<const unsigned char*>(mapped);
    const auto center=(std::size_t(output.height/2)*output.width+output.width/2)*4;
    check(pixels[center]>40 && pixels[center]<115 && pixels[center+1]>90 && pixels[center+1]<165 && pixels[center+2]>140,"Vendor output did not preserve the input color");
    std::cout<<upscaler_name(method)<<' '<<upscaling_quality_name(quality,method)<<' '<<w<<'x'<<h<<" -> "<<output.width<<'x'<<output.height<<" : 4 GPU dispatches passed\n";
}
}
int main(int argc,char** argv){try{
    check(argc==2,"Shader directory required");GPU gpu;
    if(std::string_view(argv[1])=="--native-only") {
        const auto unavailable=NativeUpscaler::availability(gpu.physical,Upscaler::DLSS45);
        if(!unavailable.empty()){std::cout<<unavailable<<'\n';return 77;}
        native_only(gpu);return 0;
    }
    for(auto method:{Upscaler::FSR31,Upscaler::DLSS45}) {
        const auto unavailable=NativeUpscaler::availability(gpu.physical,method);if(!unavailable.empty()){std::cout<<unavailable<<'\n';if(method==Upscaler::FSR31)throw std::runtime_error(unavailable);continue;}
        for(unsigned q=0;q<6;++q)run(gpu,method,UpscalingQuality(q),{640,480},argv[1]);
        for(auto quality:{UpscalingQuality::Quality,UpscalingQuality::Performance,UpscalingQuality::UltraPerformance})
            run(gpu,method,quality,{853,479},argv[1]);
    }
    check(validation_errors==0,"Vulkan validation reported errors or warnings");return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
