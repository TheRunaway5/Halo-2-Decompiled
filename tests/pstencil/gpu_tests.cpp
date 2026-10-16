#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "halo2/pstencil/render.hpp"
#include "halo2/pstencil/shadow.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace halo2;
namespace {
void require(bool b,const std::string& why){if(!b)throw FormatError(why);}
void check(VkResult r,const char* why){require(r==VK_SUCCESS,std::string(why)+": "+std::to_string(r));}
std::vector<std::string> diagnostics;
VKAPI_ATTR VkBool32 VKAPI_CALL message(VkDebugUtilsMessageSeverityFlagBitsEXT,VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT* data,void*) {diagnostics.emplace_back(data->pMessage);return VK_FALSE;}
struct GPU {
    VkInstance instance{};VkDebugUtilsMessengerEXT messenger{};VkPhysicalDevice physical{};VkDevice device{};
    VkQueue queue{};VkCommandPool pool{};VkCommandBuffer command{};
    GPU() {
        const char* layer="VK_LAYER_KHRONOS_validation";const char* extension=VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.apiVersion=VK_API_VERSION_1_3;
        VkDebugUtilsMessengerCreateInfoEXT debug{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
        debug.messageSeverity=VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug.messageType=VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug.pfnUserCallback=message;
        VkInstanceCreateInfo info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};info.pApplicationInfo=&app;
        info.enabledLayerCount=info.enabledExtensionCount=1;info.ppEnabledLayerNames=&layer;info.ppEnabledExtensionNames=&extension;info.pNext=&debug;
        check(vkCreateInstance(&info,nullptr,&instance),"instance");
        auto create=reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,"vkCreateDebugUtilsMessengerEXT"));
        check(create(instance,&debug,nullptr,&messenger),"debug messenger");
        unsigned count{};check(vkEnumeratePhysicalDevices(instance,&count,nullptr),"physical count");
        std::vector<VkPhysicalDevice> devices(count);check(vkEnumeratePhysicalDevices(instance,&count,devices.data()),"physical devices");
        for(auto candidate:devices) {VkPhysicalDeviceFeatures f;vkGetPhysicalDeviceFeatures(candidate,&f);if(f.depthClamp){physical=candidate;break;}}
        require(physical,"no depth-clamp graphics device");
        unsigned family_count{};vkGetPhysicalDeviceQueueFamilyProperties(physical,&family_count,nullptr);
        std::vector<VkQueueFamilyProperties> families(family_count);vkGetPhysicalDeviceQueueFamilyProperties(physical,&family_count,families.data());
        unsigned family=0;while(family<family_count && !(families[family].queueFlags&VK_QUEUE_GRAPHICS_BIT))++family;
        require(family<family_count,"no graphics queue");float priority=1;
        VkDeviceQueueCreateInfo queue_info{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};queue_info.queueFamilyIndex=family;queue_info.queueCount=1;queue_info.pQueuePriorities=&priority;
        VkPhysicalDeviceFeatures enabled{};enabled.depthClamp=VK_TRUE;
        VkDeviceCreateInfo device_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};device_info.queueCreateInfoCount=1;device_info.pQueueCreateInfos=&queue_info;device_info.pEnabledFeatures=&enabled;
        check(vkCreateDevice(physical,&device_info,nullptr,&device),"device");vkGetDeviceQueue(device,family,0,&queue);
        VkCommandPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};pool_info.queueFamilyIndex=family;pool_info.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        check(vkCreateCommandPool(device,&pool_info,nullptr,&pool),"command pool");
        VkCommandBufferAllocateInfo allocation{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};allocation.commandPool=pool;allocation.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;allocation.commandBufferCount=1;
        check(vkAllocateCommandBuffers(device,&allocation,&command),"command buffer");
    }
    ~GPU(){if(device){vkDeviceWaitIdle(device);vkDestroyCommandPool(device,pool,nullptr);vkDestroyDevice(device,nullptr);}if(messenger)reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,"vkDestroyDebugUtilsMessengerEXT"))(instance,messenger,nullptr);if(instance)vkDestroyInstance(instance,nullptr);}
    unsigned memory(unsigned mask,VkMemoryPropertyFlags flags) {
        VkPhysicalDeviceMemoryProperties p;vkGetPhysicalDeviceMemoryProperties(physical,&p);
        for(unsigned i=0;i<p.memoryTypeCount;++i)if((mask&(1u<<i))&&(p.memoryTypes[i].propertyFlags&flags)==flags)return i;
        throw FormatError("fixture memory unavailable");
    }
    void begin(){check(vkResetCommandBuffer(command,0),"reset command");VkCommandBufferBeginInfo info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};check(vkBeginCommandBuffer(command,&info),"begin command");}
    void submit(){check(vkEndCommandBuffer(command),"end command");VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;check(vkQueueSubmit(queue,1,&submit,VK_NULL_HANDLE),"submit");check(vkQueueWaitIdle(queue),"wait queue");}
};
struct Target {VkImage image{};VkDeviceMemory memory{};VkImageView view{};};
struct Buffer {VkBuffer buffer{};VkDeviceMemory memory{};void* mapped{};};
struct Fixture {
    GPU& gpu;VkExtent2D size;Target color,depth;Buffer vertices,readback;
    VkRenderPass pass{},continuation{};VkFramebuffer framebuffer{};VkPipelineLayout layout{};
    VkPipeline base{},original_light{},stencil_light{};std::unique_ptr<pstencil::Render> stencil;
    std::vector<pstencil::ShadowVertex> scene;std::vector<Vec3> points;
    std::vector<std::array<std::uint16_t,3>> faces;
    struct Push {glm::mat4 matrix;glm::vec4 color,viewport;};
    Fixture(GPU& g,VkExtent2D extent):gpu(g),size(extent) {
        color=target(VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT,VK_IMAGE_ASPECT_COLOR_BIT);
        depth=target(VK_FORMAT_D24_UNORM_S8_UINT,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT,VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT);
        make_pass();
        const VkImageView attachments[]{color.view,depth.view};VkFramebufferCreateInfo f{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        f.renderPass=pass;f.attachmentCount=2;f.pAttachments=attachments;f.width=size.width;f.height=size.height;f.layers=1;
        check(vkCreateFramebuffer(gpu.device,&f,nullptr,&framebuffer),"fixture framebuffer");
        VkPushConstantRange range{VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(Push)};
        VkPipelineLayoutCreateInfo l{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};l.pushConstantRangeCount=1;l.pPushConstantRanges=&range;
        check(vkCreatePipelineLayout(gpu.device,&l,nullptr,&layout),"fixture layout");
        base=pipeline(false,false);original_light=pipeline(true,false);stencil_light=pipeline(true,true);
        points={{-.4f,-.4f,0},{.4f,-.4f,0},{.4f,.4f,0},{-.4f,.4f,0},{-.4f,-.4f,1},{.4f,-.4f,1},{.4f,.4f,1},{-.4f,.4f,1}};
        faces={{{0,2,1}},{{0,3,2}},{{4,5,6}},{{4,6,7}},{{0,1,5}},{{0,5,4}},{{3,7,6}},{{3,6,2}},{{0,4,7}},{{0,7,3}},{{1,2,6}},{{1,6,5}}};
        for(const auto p:std::array<Vec3,6>{{{-5,-5,0},{5,-5,0},{5,5,0},{-5,-5,0},{5,5,0},{-5,5,0}}})scene.push_back({p,0});
        for(const auto& face:faces)for(auto index:face)scene.push_back({points[index],0});
        vertices=buffer(scene.size()*sizeof(pstencil::ShadowVertex),VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        std::memcpy(vertices.mapped,scene.data(),scene.size()*sizeof(pstencil::ShadowVertex));
        readback=buffer(std::size_t(size.width)*size.height*9,VK_BUFFER_USAGE_TRANSFER_DST_BIT);
        stencil=std::make_unique<pstencil::Render>(pstencil::RenderContext{gpu.device,gpu.physical,size,continuation,depth.image,{&color.view,1},{}});
    }
    ~Fixture(){stencil.reset();for(auto p:{base,original_light,stencil_light})vkDestroyPipeline(gpu.device,p,nullptr);vkDestroyPipelineLayout(gpu.device,layout,nullptr);vkDestroyFramebuffer(gpu.device,framebuffer,nullptr);vkDestroyRenderPass(gpu.device,continuation,nullptr);vkDestroyRenderPass(gpu.device,pass,nullptr);for(auto t:{color,depth}){vkDestroyImageView(gpu.device,t.view,nullptr);vkDestroyImage(gpu.device,t.image,nullptr);vkFreeMemory(gpu.device,t.memory,nullptr);}for(auto b:{vertices,readback}){vkUnmapMemory(gpu.device,b.memory);vkDestroyBuffer(gpu.device,b.buffer,nullptr);vkFreeMemory(gpu.device,b.memory,nullptr);}}
    Target target(VkFormat format,VkImageUsageFlags usage,VkImageAspectFlags aspect) {
        Target t;VkImageCreateInfo i{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};i.imageType=VK_IMAGE_TYPE_2D;i.format=format;i.extent={size.width,size.height,1};i.mipLevels=i.arrayLayers=1;i.samples=VK_SAMPLE_COUNT_1_BIT;i.tiling=VK_IMAGE_TILING_OPTIMAL;i.usage=usage;
        check(vkCreateImage(gpu.device,&i,nullptr,&t.image),"fixture image");VkMemoryRequirements r;vkGetImageMemoryRequirements(gpu.device,t.image,&r);
        VkMemoryAllocateInfo a{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};a.allocationSize=r.size;a.memoryTypeIndex=gpu.memory(r.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        check(vkAllocateMemory(gpu.device,&a,nullptr,&t.memory),"fixture image memory");check(vkBindImageMemory(gpu.device,t.image,t.memory,0),"fixture image binding");
        VkImageViewCreateInfo v{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};v.image=t.image;v.viewType=VK_IMAGE_VIEW_TYPE_2D;v.format=format;v.subresourceRange={aspect,0,1,0,1};check(vkCreateImageView(gpu.device,&v,nullptr,&t.view),"fixture image view");return t;
    }
    Buffer buffer(VkDeviceSize size_bytes,VkBufferUsageFlags usage) {
        Buffer b;VkBufferCreateInfo i{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};i.size=size_bytes;i.usage=usage;check(vkCreateBuffer(gpu.device,&i,nullptr,&b.buffer),"fixture buffer");
        VkMemoryRequirements r;vkGetBufferMemoryRequirements(gpu.device,b.buffer,&r);VkMemoryAllocateInfo a{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};a.allocationSize=r.size;a.memoryTypeIndex=gpu.memory(r.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        check(vkAllocateMemory(gpu.device,&a,nullptr,&b.memory),"fixture buffer memory");check(vkBindBufferMemory(gpu.device,b.buffer,b.memory,0),"fixture buffer binding");check(vkMapMemory(gpu.device,b.memory,0,size_bytes,0,&b.mapped),"fixture buffer map");return b;
    }
    void make_pass() {
        VkAttachmentDescription a[2]{};a[0].format=VK_FORMAT_R8G8B8A8_UNORM;a[1].format=VK_FORMAT_D24_UNORM_S8_UINT;
        for(auto& v:a){v.samples=VK_SAMPLE_COUNT_1_BIT;v.loadOp=v.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;v.storeOp=v.stencilStoreOp=VK_ATTACHMENT_STORE_OP_STORE;v.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;}
        a[0].finalLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;a[1].finalLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        const VkAttachmentReference color_ref{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},depth_ref{1,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        VkSubpassDescription sub{};sub.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;sub.colorAttachmentCount=1;sub.pColorAttachments=&color_ref;sub.pDepthStencilAttachment=&depth_ref;
        VkSubpassDependency dependencies[2]{};dependencies[0].srcSubpass=VK_SUBPASS_EXTERNAL;dependencies[0].dstSubpass=0;
        dependencies[0].srcStageMask=dependencies[0].dstStageMask=VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        dependencies[0].srcAccessMask=VK_ACCESS_MEMORY_WRITE_BIT;dependencies[0].dstAccessMask=VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT;
        dependencies[1]=dependencies[0];dependencies[1].srcSubpass=0;dependencies[1].dstSubpass=VK_SUBPASS_EXTERNAL;
        VkRenderPassCreateInfo info{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};info.attachmentCount=2;info.pAttachments=a;info.subpassCount=1;info.pSubpasses=&sub;info.dependencyCount=2;info.pDependencies=dependencies;
        check(vkCreateRenderPass(gpu.device,&info,nullptr,&pass),"fixture render pass");
        for(auto& v:a){v.loadOp=v.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_LOAD;v.initialLayout=v.finalLayout;}
        check(vkCreateRenderPass(gpu.device,&info,nullptr,&continuation),"fixture continuation");
    }
    VkShaderModule shader(const char* name) {
        std::ifstream f(std::filesystem::path(HALO2_PSTENCIL_SHADER_DIR)/name,std::ios::binary|std::ios::ate);const auto length=f.tellg();require(length>0&&length%4==0,"fixture shader missing");std::vector<std::uint32_t> words(std::size_t(length)/4);f.seekg(0);f.read(reinterpret_cast<char*>(words.data()),length);
        VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};info.codeSize=std::size_t(length);info.pCode=words.data();VkShaderModule s;check(vkCreateShaderModule(gpu.device,&info,nullptr,&s),"fixture shader");return s;
    }
    VkPipeline pipeline(bool light,bool masked) {
        auto v=shader("fixture.vert.spv"),f=shader("fixture.frag.spv");
        VkPipelineShaderStageCreateInfo stages[]{ {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_VERTEX_BIT,v,"main",nullptr},{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_FRAGMENT_BIT,f,"main",nullptr}};
        const VkVertexInputBindingDescription binding{0,sizeof(pstencil::ShadowVertex),VK_VERTEX_INPUT_RATE_VERTEX};const VkVertexInputAttributeDescription attribute{0,0,VK_FORMAT_R32G32B32A32_SFLOAT,0};
        VkPipelineVertexInputStateCreateInfo vertex{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};vertex.vertexBindingDescriptionCount=vertex.vertexAttributeDescriptionCount=1;vertex.pVertexBindingDescriptions=&binding;vertex.pVertexAttributeDescriptions=&attribute;
        VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};assembly.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};viewport.viewportCount=viewport.scissorCount=1;
        VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};raster.polygonMode=VK_POLYGON_MODE_FILL;raster.frontFace=VK_FRONT_FACE_COUNTER_CLOCKWISE;raster.lineWidth=1;
        VkPipelineMultisampleStateCreateInfo samples{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};samples.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
        VkPipelineDepthStencilStateCreateInfo depth_state{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};depth_state.depthTestEnable=VK_TRUE;depth_state.depthWriteEnable=!light;depth_state.depthCompareOp=VK_COMPARE_OP_LESS_OR_EQUAL;depth_state.stencilTestEnable=masked||!light;
        depth_state.front={VK_STENCIL_OP_KEEP,light?VK_STENCIL_OP_KEEP:VK_STENCIL_OP_REPLACE,VK_STENCIL_OP_KEEP,masked?VK_COMPARE_OP_EQUAL:VK_COMPARE_OP_ALWAYS,255,light?0u:255u,masked?128u:1u};depth_state.back=depth_state.front;
        VkPipelineColorBlendAttachmentState blend_attachment{};blend_attachment.colorWriteMask=light?7:15;blend_attachment.blendEnable=light;blend_attachment.srcColorBlendFactor=blend_attachment.dstColorBlendFactor=VK_BLEND_FACTOR_ONE;blend_attachment.colorBlendOp=VK_BLEND_OP_ADD;
        VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};blend.attachmentCount=1;blend.pAttachments=&blend_attachment;
        const VkDynamicState states[]{VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};dynamic.dynamicStateCount=2;dynamic.pDynamicStates=states;
        VkGraphicsPipelineCreateInfo info{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};info.stageCount=2;info.pStages=stages;info.pVertexInputState=&vertex;info.pInputAssemblyState=&assembly;info.pViewportState=&viewport;info.pRasterizationState=&raster;info.pMultisampleState=&samples;info.pDepthStencilState=&depth_state;info.pColorBlendState=&blend;info.pDynamicState=&dynamic;info.layout=layout;info.renderPass=pass;
        VkPipeline result;check(vkCreateGraphicsPipelines(gpu.device,VK_NULL_HANDLE,1,&info,nullptr,&result),"fixture pipeline");vkDestroyShaderModule(gpu.device,v,nullptr);vkDestroyShaderModule(gpu.device,f,nullptr);return result;
    }
    std::vector<unsigned char> capture(const glm::mat4& matrix,bool enabled,bool split,unsigned lights,bool wdepth,bool lightmap=false,bool source_camera=false,unsigned stipple=16) {
        const std::array<Vec3,2> positions{{{-2,-2,3},{2,-2,3}}};std::array<std::vector<pstencil::ShadowVertex>,2> volumes;
        std::vector<pstencil::VolumeBatch> batches;
        for(unsigned i=0;i<lights;++i){pstencil::append_volume(volumes[i],points,faces,positions[i],stipple);batches.push_back({volumes[i]});}
        stencil->prepare(batches);gpu.begin();
        const VkViewport viewport{0,0,float(size.width),float(size.height),0,1};const VkRect2D full{{0,0},size},clip{{0,0},{split?size.width/2:size.width,size.height}};
        const VkClearValue clears[]{ {.color={{.05f,.05f,.05f,1}}},{.depthStencil={1,1}} };
        VkRenderPassBeginInfo begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};begin.renderPass=pass;begin.framebuffer=framebuffer;begin.renderArea=full;begin.clearValueCount=2;begin.pClearValues=clears;
        vkCmdBeginRenderPass(gpu.command,&begin,VK_SUBPASS_CONTENTS_INLINE);vkCmdSetViewport(gpu.command,0,1,&viewport);vkCmdSetScissor(gpu.command,0,1,&full);
        constexpr VkDeviceSize offset=0;vkCmdBindVertexBuffers(gpu.command,0,1,&vertices.buffer,&offset);vkCmdBindPipeline(gpu.command,VK_PIPELINE_BIND_POINT_GRAPHICS,base);
        const glm::vec4 surface{0,0,size.width,size.height};
        const auto projected=source_camera?matrix*(16777215.f/1024.f):matrix;
        const float depth_control=(wdepth?-1.f:1.f)*(source_camera?2.f:1.f);
        const Push ambient{projected,{.2f,.2f,.2f,depth_control},surface};vkCmdPushConstants(gpu.command,layout,VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(ambient),&ambient);vkCmdDraw(gpu.command,unsigned(scene.size()),1,0,0);
        const auto camera_for=[&](unsigned i) {
            pstencil::VolumeCamera camera{};std::memcpy(camera.matrix.data(),&matrix,64);
            camera.control={wdepth?1.f:0.f,1,0,0};
            for(unsigned axis=0;axis<4;++axis)camera.depth_row[axis]=matrix[axis][3]*(16777215.f/1024.f);
            camera.light={positions[i].x,positions[i].y,positions[i].z,pstencil::shadow_extrusion_distance};
            if(source_camera && !std::getenv("PSTENCIL_TEST_OMIT_SOURCE_CAMERA")) {
                for(unsigned column=0;column<4;++column)for(unsigned row=0;row<4;++row)
                    camera.matrix[column*4+row]=matrix[column][row]*(16777215.f/1024.f)*(row==1?-1.f:1.f);
                camera.viewport={surface.x,surface.y,surface.z,surface.w};camera.control[2]=1;
            }
            return camera;
        };
        if(enabled)stencil->begin(gpu.command,0);
        if(enabled && lightmap) {
            const auto camera=camera_for(0);
            // Two identical masks overlap completely. E8350 composites once
            // after all casters, so overlap must not square the multiplier.
            const std::array draws{pstencil::VolumeDraw{0,camera},pstencil::VolumeDraw{0,camera}};
            if(!std::getenv("PSTENCIL_TEST_OMIT_LIGHTMAP"))stencil->lightmap(gpu.command,draws,viewport,clip);
        }
        for(unsigned i=0;i<lights;++i) {
            const auto draw=[&]{vkCmdSetViewport(gpu.command,0,1,&viewport);vkCmdSetScissor(gpu.command,0,1,&clip);vkCmdBindVertexBuffers(gpu.command,0,1,&vertices.buffer,&offset);vkCmdBindPipeline(gpu.command,VK_PIPELINE_BIND_POINT_GRAPHICS,enabled?stencil_light:original_light);const Push lighting{projected,{.1f,.15f,.2f,depth_control},surface};vkCmdPushConstants(gpu.command,layout,VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(lighting),&lighting);vkCmdDraw(gpu.command,unsigned(scene.size()),1,0,0);};
            if(enabled) {const auto camera=camera_for(i);stencil->light(gpu.command,i,camera,viewport,clip,draw);}else draw();
        }
        if(enabled)stencil->end(gpu.command,framebuffer);
        vkCmdEndRenderPass(gpu.command);
        VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};barrier.image=depth.image;barrier.oldLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;barrier.subresourceRange={VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT,0,1,0,1};barrier.srcAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
        vkCmdPipelineBarrier(gpu.command,VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
        const auto pixels=std::size_t(size.width)*size.height;VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={size.width,size.height,1};vkCmdCopyImageToBuffer(gpu.command,color.image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,readback.buffer,1,&copy);
        copy.bufferOffset=pixels*4;copy.imageSubresource.aspectMask=VK_IMAGE_ASPECT_DEPTH_BIT;vkCmdCopyImageToBuffer(gpu.command,depth.image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,readback.buffer,1,&copy);
        copy.bufferOffset=pixels*8;copy.imageSubresource.aspectMask=VK_IMAGE_ASPECT_STENCIL_BIT;vkCmdCopyImageToBuffer(gpu.command,depth.image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,readback.buffer,1,&copy);gpu.submit();
        const auto first=static_cast<unsigned char*>(readback.mapped);return {first,first+pixels*9};
    }
};
// Independent analytic occlusion check. Darker output alone cannot detect
// an inverted stencil predicate. Sample visible floor pixels away from box
// edges and compare the GPU result with light-to-floor segment occlusion.
bool segment_box(glm::vec3 a,glm::vec3 b,float margin) {
    const glm::vec3 low(-.4f-margin,-.4f-margin,-margin),high(.4f+margin,.4f+margin,1.f+margin);
    const auto delta=b-a;float first=0,last=1;
    for(unsigned axis=0;axis<3;++axis) {
        if(std::abs(delta[axis])<1.e-8f) {
            if(a[axis]<low[axis] || a[axis]>high[axis])return false;
        } else {
            auto x=(low[axis]-a[axis])/delta[axis],y=(high[axis]-a[axis])/delta[axis];
            if(x>y)std::swap(x,y);
            first=std::max(first,x);last=std::min(last,y);
        }
    }
    return last>=first && last>1.e-5f && first<.99999f;
}
void verify_floor_mask(VkExtent2D size,const glm::mat4& matrix,glm::vec3 eye,
    const std::vector<unsigned char>& original,const std::vector<unsigned char>& shadowed,bool source_camera=false) {
    const auto inverse=glm::inverse(matrix);unsigned lit{},occluded{};
    for(unsigned y=2;y+2<size.height;y+=2)for(unsigned x=2;x+2<size.width;x+=2) {
        const float sample=source_camera?0.f:.5f;
        auto far=inverse*glm::vec4((float(x)+sample)*2/size.width-1,(float(y)+sample)*2/size.height-1,1,1);
        const auto ray=glm::vec3(far)/far.w-eye;if(ray.z>=0)continue;
        const auto floor=eye+ray*(-eye.z/ray.z);
        if(std::abs(floor.x)>4.8f || std::abs(floor.y)>4.8f)continue;
        const auto start=floor+glm::vec3(0,0,.0001f);
        if(segment_box(start,eye,.02f))continue;
        const glm::vec3 light(-2,-2,3);
        const auto blocked=segment_box(start,light,-.02f);
        if(blocked!=segment_box(start,light,.02f))continue;
        const auto pixel=(std::size_t(y)*size.width+x)*4;
        const bool gpu=original[pixel]>shadowed[pixel]+2;
        require(gpu==blocked,"pStencil floor coverage disagrees with point-light occlusion");
        blocked?++occluded:++lit;
    }
    require(lit>500 && occluded>50,"analytic floor mask lacks both lit and shadowed samples");
    std::cout<<"Analytic point-light floor mask: lit="<<lit<<" shadowed="<<occluded<<" mismatches=0\n";
    if(source_camera) {
        unsigned top{};
        for(unsigned y=2;y+2<size.height;++y)for(unsigned x=2;x+2<size.width;++x) {
            auto far=inverse*glm::vec4(float(x)*2/size.width-1,float(y)*2/size.height-1,1,1);
            const auto ray=glm::vec3(far)/far.w-eye;if(ray.z>=0)continue;
            const auto p=eye+ray*((1.f-eye.z)/ray.z);
            if(std::abs(p.x)>.32f || std::abs(p.y)>.32f)continue;
            const auto at=(std::size_t(y)*size.width+x)*4;
            require(original[at]==shadowed[at],"source viewport falsely self-shadowed a convex light-facing cap");++top;
        }
        require(top>40,"native viewport top-face self-shadow test lacks samples");
        std::cout<<"Source near-cap alignment: light-facing pixels="<<top<<" false self-shadows=0\n";
    }
}
void ppm(const std::filesystem::path& path,VkExtent2D size,const std::vector<unsigned char>& bytes) {
    std::ofstream f(path,std::ios::binary);f<<"P6\n"<<size.width<<' '<<size.height<<"\n255\n";
    for(std::size_t i=0;i<std::size_t(size.width)*size.height;++i)f.write(reinterpret_cast<const char*>(bytes.data()+i*4),3);
}
}
int main(int argc,char** argv){try {
    require(argc==2,"provide a new GPU evidence directory");const std::filesystem::path output=argv[1];require(!std::filesystem::exists(output),"evidence directory already exists");std::filesystem::create_directories(output);
    GPU gpu;unsigned cases{};std::ofstream report(output/"measurements.tsv");report<<"case\tw_depth\tshadow_pixels\tdepth_stencil_changed_bytes\toriginal_restore_changed_bytes\n";
    for(const auto size:{VkExtent2D{320,240},VkExtent2D{255,191},VkExtent2D{320,240}}) {
        Fixture fixture(gpu,size);
        for(unsigned view=0;view<3;++view)for(unsigned lights=1;lights<=2;++lights)for(bool wdepth:{false,true})for(bool source_camera:{false,true}) {
            const glm::vec3 eye=view==0?glm::vec3(4,5,4):view==1?glm::vec3(.8f,.8f,.2f):glm::vec3(.5f,.5f,1.2f);
            const glm::vec3 target=view==1?glm::vec3(1.8f,1.8f,0):glm::vec3(0,0,.3f);
            auto projection=glm::perspective(glm::radians(65.f),float(size.width)/size.height,view==2?.5f:.015625f,1024.f);projection[1][1]*=-1;
            const auto matrix=projection*glm::lookAt(eye,target,glm::vec3(0,0,1));const bool split=lights==2;
            const auto before=fixture.capture(matrix,false,split,lights,wdepth,false,source_camera),enabled=fixture.capture(matrix,true,split,lights,wdepth,false,source_camera),after=fixture.capture(matrix,false,split,lights,wdepth,false,source_camera);
            if(view==0 && lights==1) {
                verify_floor_mask(size,matrix,eye,before,enabled,source_camera);
                const auto partial=fixture.capture(matrix,true,split,lights,wdepth,false,source_camera,8);
                constexpr unsigned rank[16]{0,12,3,10,8,4,15,6,2,11,1,13,14,7,9,5};
                unsigned stippled{},open{};
                for(unsigned y=0;y<size.height;++y)for(unsigned x=0;x<size.width;++x) {
                    const auto pixel=(std::size_t(y)*size.width+x)*4;
                    const auto row=(31u-y)&31u,bit=(x&24u)|(7u-(x&7u));
                    const bool mask=rank[(row&3u)*4u+(bit&3u)]<8;
                    for(unsigned channel=0;channel<4;++channel)
                        require(partial[pixel+channel]==(mask?enabled[pixel+channel]:before[pixel+channel]),
                            "partial-opacity shadow disagrees with the original stipple pattern/phase");
                    if(before[pixel]!=enabled[pixel])mask?++stippled:++open;
                }
                require(stippled>10 && open>10,"partial opacity lacks both covered and open shadow pixels");
                const auto zero=fixture.capture(matrix,true,split,lights,wdepth,false,source_camera,0);
                require(zero==before,"zero-opacity caster changed color/depth/stencil");
                std::cout<<"Original shadow stipple: covered="<<stippled<<" open="<<open<<" pattern/phase=exact\n";
                const auto baked=fixture.capture(matrix,true,split,lights,wdepth,true,source_camera);
                unsigned samples{},unshadowed{};
                for(std::size_t p=0;p<std::size_t(size.width)*size.height;++p) {
                    const auto at=p*4;
                    require(baked[at+3]==enabled[at+3],"baked shadow changed destination alpha");
                    if(enabled[at]==51 && before[at]>enabled[at]+2) {
                        const auto expected=int(std::lround(51.f*float(pstencil::lightmap_shadow_rgb)/255.f));
                        require(std::abs(int(baked[at])-expected)<=1,"baked shadow lacks original RGB152 multiply or compounds overlaps");
                        ++samples;
                    } else if(before[at]==enabled[at]) {
                        require(baked[at]==enabled[at],"lightmap composite darkened outside its stencil mask");++unshadowed;
                    }
                }
                require(samples>50 && unshadowed>500,"lightmap shadow test lacks lit and dark samples");
                const auto restored=fixture.capture(matrix,false,split,lights,wdepth,false,source_camera);
                require(restored==before,"Original did not restore exact color/depth/stencil after baked shadows");
                std::cout<<"Original lightmap multiply: shadow_samples="<<samples<<" RGB="<<unsigned(pstencil::lightmap_shadow_rgb)<<" overlap=once alpha=exact\n";
            }
            const auto pixels=std::size_t(size.width)*size.height;std::size_t changed{},depth_changed{},restore_changed{},outside_changed{};
            for(std::size_t i=0;i<pixels;++i) {
                changed+=!std::equal(before.begin()+i*4,before.begin()+i*4+3,enabled.begin()+i*4);
                if(split && i%size.width>=size.width/2)outside_changed+=!std::equal(before.begin()+i*4,before.begin()+i*4+3,enabled.begin()+i*4);
                for(unsigned c=0;c<3;++c)require(enabled[i*4+c]<=before[i*4+c],"volume added unexplained light");
            }
            for(std::size_t i=pixels*4;i<before.size();++i)depth_changed+=before[i]!=enabled[i];
            for(std::size_t i=0;i<before.size();++i)restore_changed+=before[i]!=after[i];
            require(changed>10,"pStencil produced no volume shadow");require(!depth_changed,"pStencil modified Original depth/stencil");require(!restore_changed,"Original did not restore exact output");require(!outside_changed,"pStencil leaked outside view clip");
            const auto name="case-"+std::to_string(cases++);ppm(output/(name+"-original.ppm"),size,before);ppm(output/(name+"-pstencil.ppm"),size,enabled);
            report<<name<<'\t'<<wdepth<<'\t'<<changed<<'\t'<<depth_changed<<'\t'<<restore_changed<<'\n';
        }
    }
    for(const auto& diagnostic:diagnostics)std::cerr<<diagnostic<<'\n';
    require(diagnostics.empty(),"Vulkan validation reported diagnostics");
    std::cout<<"PASS: "<<cases<<" GPU cases; integer W and Z depth, capped shadows, near-plane/inside-volume cameras, two lights, clipped views, odd resize, exact Original restore and untouched Original D24S8\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';for(const auto& diagnostic:diagnostics)std::cerr<<diagnostic<<'\n';return 1;}}
