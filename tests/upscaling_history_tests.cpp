#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "halo2/temporal_upscaler.hpp"
#include "halo2/source_view_projection.hpp"
#include "rendering/renderer/temporal_geometry.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string_view>
using namespace halo2;
namespace {
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void checked(VkResult value){if(value!=VK_SUCCESS)throw std::runtime_error("Vulkan "+std::to_string(value));}
unsigned validation_errors{};
VKAPI_ATTR VkBool32 VKAPI_CALL validation(VkDebugUtilsMessageSeverityFlagBitsEXT,VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT* data,void*) {
    ++validation_errors;std::cerr<<data->pMessage<<'\n';return VK_FALSE;
}
struct GPU {
    VkInstance instance{};VkDebugUtilsMessengerEXT debug{};VkPhysicalDevice physical{};VkDevice device{};
    VkQueue queue{};unsigned family{};VkCommandPool pool{};VkCommandBuffer command{};
    GPU() {
        auto names=NativeUpscaler::instance_extensions();names.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        std::vector<const char*> extensions;for(const auto& name:names)extensions.push_back(name.c_str());
        unsigned count{};checked(vkEnumerateInstanceLayerProperties(&count,nullptr));std::vector<VkLayerProperties> layers(count);
        checked(vkEnumerateInstanceLayerProperties(&count,layers.data()));const char* layer="VK_LAYER_KHRONOS_validation";
        const bool has_validation=std::any_of(layers.begin(),layers.end(),[&](const auto& item){return std::string_view(item.layerName)==layer;});
        VkApplicationInfo application{VK_STRUCTURE_TYPE_APPLICATION_INFO};application.pApplicationName="Halo2 prepared temporal inputs";application.apiVersion=VK_API_VERSION_1_3;
        VkInstanceCreateInfo create{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};create.pApplicationInfo=&application;
        create.enabledExtensionCount=extensions.size();create.ppEnabledExtensionNames=extensions.data();
        create.enabledLayerCount=has_validation?1:0;create.ppEnabledLayerNames=&layer;checked(vkCreateInstance(&create,nullptr,&instance));
        VkDebugUtilsMessengerCreateInfoEXT diagnostics{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
        diagnostics.messageSeverity=VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        diagnostics.messageType=VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;diagnostics.pfnUserCallback=validation;
        checked(reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,"vkCreateDebugUtilsMessengerEXT"))(instance,&diagnostics,nullptr,&debug));
        checked(vkEnumeratePhysicalDevices(instance,&count,nullptr));require(count!=0,"No Vulkan GPU");std::vector<VkPhysicalDevice> devices(count);
        checked(vkEnumeratePhysicalDevices(instance,&count,devices.data()));physical=devices.front();
        for(auto candidate:devices){VkPhysicalDeviceProperties properties{};vkGetPhysicalDeviceProperties(candidate,&properties);if(properties.deviceType==VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)physical=candidate;}
        vkGetPhysicalDeviceQueueFamilyProperties(physical,&count,nullptr);std::vector<VkQueueFamilyProperties> queues(count);
        vkGetPhysicalDeviceQueueFamilyProperties(physical,&count,queues.data());
        while(family<count && (queues[family].queueFlags&(VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT))!=(VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT))++family;
        require(family<count,"No graphics/compute queue");float priority=1;
        VkDeviceQueueCreateInfo queue_info{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};queue_info.queueFamilyIndex=family;queue_info.queueCount=1;queue_info.pQueuePriorities=&priority;
        names=NativeUpscaler::device_extensions(instance,physical);extensions.clear();for(const auto& name:names)extensions.push_back(name.c_str());
        VkPhysicalDeviceFeatures features{};features.shaderStorageImageExtendedFormats=VK_TRUE;
        VkPhysicalDeviceBufferDeviceAddressFeatures address{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES};address.bufferDeviceAddress=VK_TRUE;
        VkDeviceCreateInfo device_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};device_info.pNext=&address;device_info.pEnabledFeatures=&features;
        device_info.queueCreateInfoCount=1;device_info.pQueueCreateInfos=&queue_info;device_info.enabledExtensionCount=extensions.size();device_info.ppEnabledExtensionNames=extensions.data();
        checked(vkCreateDevice(physical,&device_info,nullptr,&device));vkGetDeviceQueue(device,family,0,&queue);
        VkCommandPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};pool_info.queueFamilyIndex=family;pool_info.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        checked(vkCreateCommandPool(device,&pool_info,nullptr,&pool));
        VkCommandBufferAllocateInfo allocate{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};allocate.commandPool=pool;allocate.commandBufferCount=1;
        checked(vkAllocateCommandBuffers(device,&allocate,&command));
    }
    ~GPU(){if(device){vkDeviceWaitIdle(device);if(pool)vkDestroyCommandPool(device,pool,nullptr);vkDestroyDevice(device,nullptr);}
        if(debug)reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,"vkDestroyDebugUtilsMessengerEXT"))(instance,debug,nullptr);
        if(instance)vkDestroyInstance(instance,nullptr);}
    unsigned memory_type(unsigned bits,VkMemoryPropertyFlags flags) {
        VkPhysicalDeviceMemoryProperties properties{};vkGetPhysicalDeviceMemoryProperties(physical,&properties);
        for(unsigned index=0;index<properties.memoryTypeCount;++index)if((bits&(1u<<index)) && (properties.memoryTypes[index].propertyFlags&flags)==flags)return index;
        throw std::runtime_error("No compatible Vulkan memory");
    }
    void commands(const std::function<void(VkCommandBuffer)>& record) {
        checked(vkResetCommandBuffer(command,0));VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};checked(vkBeginCommandBuffer(command,&begin));
        record(command);checked(vkEndCommandBuffer(command));VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;
        checked(vkQueueSubmit(queue,1,&submit,{}));checked(vkQueueWaitIdle(queue));
    }
};
void barrier(VkCommandBuffer command,VkImage image,VkImageAspectFlags aspect,VkImageLayout old,VkImageLayout next) {
    VkImageMemoryBarrier item{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};item.image=image;item.oldLayout=old;item.newLayout=next;
    item.subresourceRange={aspect,0,1,0,1};item.srcQueueFamilyIndex=item.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
    item.srcAccessMask=old==VK_IMAGE_LAYOUT_UNDEFINED?0:VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT;item.dstAccessMask=VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT;
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,0,0,nullptr,0,nullptr,1,&item);
}
struct Image {
    GPU& gpu;VkImage image{};VkImageView view{};VkDeviceMemory memory{};VkImageAspectFlags aspect;
    Image(GPU& gpu,VkExtent2D extent,bool depth):gpu(gpu),aspect(depth?VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT:VK_IMAGE_ASPECT_COLOR_BIT) {
        const auto format=depth?VK_FORMAT_D24_UNORM_S8_UINT:VK_FORMAT_R8G8B8A8_UNORM;
        VkImageCreateInfo create{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};create.imageType=VK_IMAGE_TYPE_2D;create.format=format;create.extent={extent.width,extent.height,1};
        create.mipLevels=create.arrayLayers=1;create.samples=VK_SAMPLE_COUNT_1_BIT;create.tiling=VK_IMAGE_TILING_OPTIMAL;
        create.usage=VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|(depth?VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT:VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
        checked(vkCreateImage(gpu.device,&create,nullptr,&image));VkMemoryRequirements requirements{};vkGetImageMemoryRequirements(gpu.device,image,&requirements);
        VkMemoryAllocateInfo allocate{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocate.allocationSize=requirements.size;allocate.memoryTypeIndex=gpu.memory_type(requirements.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        checked(vkAllocateMemory(gpu.device,&allocate,nullptr,&memory));checked(vkBindImageMemory(gpu.device,image,memory,0));
        VkImageViewCreateInfo description{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};description.image=image;description.viewType=VK_IMAGE_VIEW_TYPE_2D;description.format=format;
        description.subresourceRange={depth?VK_IMAGE_ASPECT_DEPTH_BIT:VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};checked(vkCreateImageView(gpu.device,&description,nullptr,&view));
    }
    ~Image(){vkDestroyImageView(gpu.device,view,nullptr);vkDestroyImage(gpu.device,image,nullptr);vkFreeMemory(gpu.device,memory,nullptr);}
};
float half_float(std::uint16_t bits) {
    const float sign=bits&0x8000?-1.f:1.f;const unsigned exponent=(bits>>10)&31,mantissa=bits&1023;
    return exponent==0?sign*std::ldexp(float(mantissa),-24):exponent==31?sign*INFINITY:sign*std::ldexp(1.f+float(mantissa)/1024,int(exponent)-15);
}
struct Capture {
    GPU& gpu;VkExtent2D extent;VkBuffer buffer{};VkDeviceMemory memory{};void* mapped{};VkDeviceSize count;
    glm::vec2 dispatch_jitter{};bool reset{};
    Capture(GPU& gpu,VkExtent2D extent):gpu(gpu),extent(extent),count(VkDeviceSize(extent.width)*extent.height) {
        VkBufferCreateInfo create{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};create.size=count*9;create.usage=VK_BUFFER_USAGE_TRANSFER_DST_BIT;checked(vkCreateBuffer(gpu.device,&create,nullptr,&buffer));
        VkMemoryRequirements requirements{};vkGetBufferMemoryRequirements(gpu.device,buffer,&requirements);VkMemoryAllocateInfo allocate{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocate.allocationSize=requirements.size;allocate.memoryTypeIndex=gpu.memory_type(requirements.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        checked(vkAllocateMemory(gpu.device,&allocate,nullptr,&memory));checked(vkBindBufferMemory(gpu.device,buffer,memory,0));checked(vkMapMemory(gpu.device,memory,0,count*9,0,&mapped));
    }
    ~Capture(){vkUnmapMemory(gpu.device,memory);vkDestroyBuffer(gpu.device,buffer,nullptr);vkFreeMemory(gpu.device,memory,nullptr);}
    void record(const UpscalingDispatch& dispatch) {
        dispatch_jitter={dispatch.jitter_x,dispatch.jitter_y};reset=dispatch.reset;
        const std::array inputs{dispatch.depth,dispatch.motion,dispatch.reactive};
        const std::array<VkDeviceSize,3> offsets{0,count*4,count*8};
        for(unsigned index=0;index<inputs.size();++index) {
            const auto aspect=index==0?VK_IMAGE_ASPECT_DEPTH_BIT:VK_IMAGE_ASPECT_COLOR_BIT;
            barrier(dispatch.command,inputs[index].image,aspect,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
            VkBufferImageCopy copy{};copy.bufferOffset=offsets[index];copy.imageSubresource={aspect,0,0,1};copy.imageExtent={extent.width,extent.height,1};
            vkCmdCopyImageToBuffer(dispatch.command,inputs[index].image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,buffer,1,&copy);
            barrier(dispatch.command,inputs[index].image,aspect,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
        VkBufferMemoryBarrier ready{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};ready.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;ready.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
        ready.srcQueueFamilyIndex=ready.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;ready.buffer=buffer;ready.size=VK_WHOLE_SIZE;
        vkCmdPipelineBarrier(dispatch.command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,0,nullptr,1,&ready,0,nullptr);
    }
    std::size_t pixel(unsigned x,unsigned y)const{return std::size_t(y)*extent.width+x;}
    float depth(unsigned x,unsigned y)const{return static_cast<const float*>(mapped)[pixel(x,y)];}
    glm::vec2 motion(unsigned x,unsigned y)const {
        const auto* values=reinterpret_cast<const std::uint16_t*>(static_cast<const std::byte*>(mapped)+count*4)+pixel(x,y)*2;
        return {half_float(values[0]),half_float(values[1])};
    }
    float reactive(unsigned x,unsigned y)const{return float(static_cast<const unsigned char*>(mapped)[count*8+pixel(x,y)])/255.f;}
};
constexpr VkExtent2D input{320,240},output{640,480};
constexpr float near_plane=.015625f,far_plane=1024.f,fov=1.2f,distance=4.f;
glm::mat4 matrix(float x=0,float yaw=0) {
    auto projection=glm::perspective(fov,float(input.width)/input.height,near_plane,far_plane);projection[1][1]*=-1;
    const glm::vec3 eye{x,0,0};return projection*glm::lookAt(eye,eye+glm::vec3(std::sin(yaw),0,-std::cos(yaw)),glm::vec3(0,1,0));
}
float source_depth(float scale=1) {
    SourceViewProjectionInput camera;camera.forward={0,0,-1};camera.up={0,1,0};camera.near_distance=near_plane;camera.far_distance=far_plane;
    camera.projection_program[10]=1;camera.projection_program[11]=1;
    const auto constants=source_view_shader_constants(source_view_projection(camera),far_plane,scale!=1);
    const auto& row=constants[3];const float raw=row[2]*-distance+row[3];
    // Literal worldD24Depth floor/2^24/+one-float-ULP before attachment UNORM conversion.
    return std::bit_cast<float>(std::bit_cast<std::uint32_t>(std::floor(raw)/16777216.f)+1u);
}
float expected_depth(){return far_plane/(far_plane-near_plane)-near_plane*far_plane/((far_plane-near_plane)*distance);}
std::array<Vertex,3> triangle(float x=0) {
    std::array<Vertex,3> vertices{};vertices[0].position={x-.45f,-.7f,-distance};vertices[1].position={x+.45f,-.7f,-distance};vertices[2].position={x,.7f,-distance};
    for(auto& vertex:vertices){vertex.material=7;vertex.source_section=2;vertex.source_part=3;vertex.source_local_position=vertex.position;}
    return vertices;
}
void run(GPU& gpu,const char* shaders,std::string_view name) {
    Image color(gpu,input,false),raw_depth(gpu,input,true),result(gpu,output,false);Capture capture(gpu,input);
    TemporalUpscaler temporal(gpu.instance,gpu.physical,gpu.device,gpu.queue,gpu.family,input,output,Upscaler::FSR31,UpscalingQuality::Performance,
        shaders,raw_depth.image,raw_depth.view,{},[&](const auto& dispatch){capture.record(dispatch);});
    TemporalView view;view.rectangle={{0,0},input};view.matrix=matrix();view.foreground_matrix=view.matrix;
    view.w_depth=true;view.near_plane=near_plane;view.far_plane=far_plane;view.vertical_fov=fov;
    glm::vec2 actual_jitter{};
    const auto frame=[&](float depth) {
        actual_jitter=temporal.jitter();
        gpu.commands([&](VkCommandBuffer command) {
            barrier(command,color.image,color.aspect,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            const VkClearColorValue value{{.3f,.5f,.7f,1}};const VkImageSubresourceRange color_range{color.aspect,0,1,0,1};
            vkCmdClearColorImage(command,color.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,&value,1,&color_range);
            barrier(command,color.image,color.aspect,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            barrier(command,raw_depth.image,raw_depth.aspect,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            const VkClearDepthStencilValue value_depth{depth,0};const VkImageSubresourceRange depth_range{raw_depth.aspect,0,1,0,1};
            vkCmdClearDepthStencilImage(command,raw_depth.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,&value_depth,1,&depth_range);
            barrier(command,raw_depth.image,raw_depth.aspect,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
            temporal.begin(command);temporal.view(command,0,view);temporal.present(command,color.image,color.view,result.image,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        });
    };
    const auto center_x=input.width/2,center_y=input.height/2;
    auto foreground=triangle();auto left=triangle(-1.1f),right=triangle(1.1f);std::vector<Vertex> ordered;
    std::array<TemporalVertexIdentity,3> foreground_ids;
    for(unsigned index=0;index<foreground_ids.size();++index)foreground_ids[index]={0x10001,0x20002,0x30003,0,index};
    const bool deferred=name=="deferred-world" || name=="deferred-foreground";
    const bool is_foreground=name=="foreground" || name=="deferred-foreground";
    float raw=name=="sky"?1.f:source_depth(is_foreground?.15f:1.f);
    std::shared_ptr<ModelSkinGeometry> geometry;std::array<ModelSkinSubmission,1> submissions;
    if(deferred) {
        geometry=std::make_shared<ModelSkinGeometry>();geometry->owner=0x10001;geometry->definition=0x20002;geometry->render_model=0x30003;
        geometry->vertices.assign(foreground.begin(),foreground.end());geometry->bindings.resize(3);geometry->source_vertices={0,1,2};
        for(auto& vertex:geometry->vertices) {vertex.position.z=0;vertex.source_local_position=vertex.position;vertex.normal={0,0,1};vertex.tangent={1,0,0};vertex.binormal={0,1,0};}
        for(auto& binding:geometry->bindings){binding.bones={0,0,0,0};binding.weights={1,0,0,0};}
        geometry->palette.push_back({1,0,0,0,0,1,0,0,0,0,1,-distance});geometry->post_identity_placement=is_foreground;
        submissions={ModelSkinSubmission{0,geometry}};
        // This negative witness reproduces renderer.cpp's original handoff:
        // actual draw uses deferred WORLD skin, temporal receives local bytes.
        if(is_foreground)view.foreground=geometry->vertices;else view.vertices=geometry->vertices;
    }
    if(name=="foreground") {view.foreground=foreground;view.foreground_identities=foreground_ids;}
    if(name=="reorder") {ordered.insert(ordered.end(),left.begin(),left.end());ordered.insert(ordered.end(),right.begin(),right.end());view.vertices=ordered;}
    // Cold SDK compilation can trigger the production half-second reset.
    // Warm to a real continuous frame rather than overriding its reset policy.
    for(unsigned warm=0;warm<8;++warm){frame(raw);if(warm && !capture.reset)break;}
    require(!capture.reset,"Temporal history did not become continuous after warmup");
    if(deferred) {
        geometry->palette[0][3]=.1f;frame(raw);const auto unposed=capture.motion(center_x,center_y);
        require(glm::length(unposed)<.03f,"Unposed deferred witness unexpectedly received palette animation");
        geometry->palette[0][3]=0;TemporalGeometry posed;posed.append(geometry->vertices,submissions);
        const auto bind_pose=[&] {
            if(is_foreground) {view.foreground=posed.vertices;view.foreground_identities=posed.identities;}
            else {view.vertices=posed.vertices;view.identities=posed.identities;}
        };
        bind_pose();
        temporal.reset_history();frame(raw);frame(raw);
        geometry->palette[0][3]=.1f;posed={};posed.append(geometry->vertices,submissions);bind_pose();
        frame(raw);const auto corrected=capture.motion(center_x,center_y);
        const float expected=-matrix()[0][0]*.1f/distance*input.width*.5f;
        std::cout<<name<<" actual deferred palette: unposed MV="<<unposed.x<<','<<unposed.y
            <<" posed MV="<<corrected.x<<','<<corrected.y<<" expected="<<expected<<",0\n";
        require(std::abs(corrected.x-expected)<.03f && std::abs(corrected.y)<.03f,
            "Existing source skin materialization did not restore animated motion at vendor boundary");
        require(std::abs(capture.depth(center_x,center_y)-expected_depth())<.000002f,
            "Deferred WORLD-space pose did not restore perspective depth");
        // A recycled source slot with the same material/part and WORLD bytes
        // belongs to a different salted owner; even equal positions cannot
        // authorize its predecessor's shading history.
        geometry->owner+=0x10000;posed={};posed.append(geometry->vertices,submissions);bind_pose();frame(raw);
        require(glm::length(capture.motion(center_x,center_y))<.03f && capture.reactive(center_x,center_y)>.99f,
            "Replacement deferred owner reused its predecessor's correspondence");
    } else if(name=="world") {
        const auto still=capture.motion(center_x,center_y);require(glm::length(still)<.02f,"Static world has nonzero motion");
        require(std::abs(capture.depth(center_x,center_y)-expected_depth())<.000002f,"Actual source W-depth did not convert to perspective depth");
        view.matrix=matrix(.125f);frame(raw);
        const float expected=matrix()[0][0]*.125f/distance*input.width*.5f;const auto motion=capture.motion(center_x,center_y);
        std::cout<<"world pan motion="<<motion.x<<','<<motion.y<<" expected="<<expected<<",0\n";
        require(std::abs(motion.x-expected)<.03f && std::abs(motion.y)<.03f,"World camera pan motion is incorrect");
    } else if(name=="foreground") {
        require(std::abs(capture.depth(center_x,center_y)-expected_depth())<.000002f,"Foreground .15 W-depth was interpreted as world distance");
        for(auto& vertex:foreground)vertex.position.x+=.1f;frame(raw);
        const float expected=-matrix()[0][0]*.1f/distance*input.width*.5f;const auto motion=capture.motion(center_x,center_y);
        std::cout<<"foreground animation motion="<<motion.x<<','<<motion.y<<" expected="<<expected<<",0\n";
        require(std::abs(motion.x-expected)<.03f && std::abs(motion.y)<.03f,"Animated foreground did not produce object motion");
    } else if(name=="sky") {
        constexpr float yaw=.04f;view.matrix=matrix(0,yaw);frame(raw);
        const auto motion=capture.motion(center_x,center_y);const float expected=matrix()[0][0]*std::tan(yaw)*input.width*.5f;
        std::cout<<"sky rotation motion="<<motion.x<<','<<motion.y<<" approximately expected="<<expected<<",0\n";
        require(std::abs(motion.x-expected)<.15f,"Camera rotation leaves sky motion at zero");
    } else if(name=="reorder") {
        ordered.clear();ordered.insert(ordered.end(),right.begin(),right.end());ordered.insert(ordered.end(),left.begin(),left.end());view.vertices=ordered;frame(raw);
        const unsigned x=unsigned(input.width*.5f+matrix()[0][0]*1.1f/distance*input.width*.5f+actual_jitter.x);
        const auto motion=capture.motion(x,center_y);const auto reactive=capture.reactive(x,center_y);
        std::cout<<"reordered shared-material owners motion="<<motion.x<<','<<motion.y<<" reactive="<<reactive<<'\n';
        require(glm::length(motion)<.03f && reactive>.99f,"Reordered owners reused unrelated indexed history");
    } else if(name=="jitter") {
        std::cout<<"viewport jitter="<<actual_jitter.x<<','<<actual_jitter.y<<" vendor jitter="<<capture.dispatch_jitter.x<<','<<capture.dispatch_jitter.y<<'\n';
        require(glm::length(actual_jitter-capture.dispatch_jitter)<.00001f,"Vendor jitter differs from actual positive Vulkan viewport offsets");
    } else throw std::runtime_error("Unknown temporal input case");
    require(validation_errors==0,"Prepared-input capture reported Vulkan validation errors");
}
}
int main(int argc,char** argv) {try {
    require(argc==3,"Usage: upscaling_history_tests SHADERS CASE");GPU gpu;
    const auto unavailable=NativeUpscaler::availability(gpu.physical,Upscaler::FSR31);
    if(!unavailable.empty()){std::cout<<unavailable<<'\n';return 77;}
    run(gpu,argv[1],argv[2]);return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
