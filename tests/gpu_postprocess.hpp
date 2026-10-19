#pragma once
#include <vulkan/vulkan.h>
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <span>
#include <stdexcept>
#include <vector>

// Offscreen execution of the production fullscreen shaders on captured inputs.
// The test owns its device and has no dependency on a window or live frontend.
namespace halo2_test {
enum class PostBlend {Disabled,Premultiplied,Screen,AlphaRGB};
struct PostImage {
    unsigned binding{},width{},height{};
    VkFormat format=VK_FORMAT_R8G8B8A8_UNORM;
    VkFilter filter=VK_FILTER_LINEAR;
    VkSamplerAddressMode address=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    std::vector<std::byte> pixels;
    unsigned levels=1;
    VkImageViewType view_type=VK_IMAGE_VIEW_TYPE_2D;
    unsigned array_element{};
};
class Postprocess {
    struct Buffer {VkBuffer handle{};VkDeviceMemory memory{};void *mapped{};};
    struct Image {VkImage handle{};VkDeviceMemory memory{};VkImageView view{};VkSampler sampler{};};
    VkInstance instance{};VkPhysicalDevice physical{};VkDevice device{};VkQueue queue{};
    unsigned family{};VkCommandPool commands{};VkDescriptorSetLayout set_layout{};
    VkDescriptorPool pool{};VkPipelineLayout layout{};VkPipeline pipeline{};
    VkRenderPass pass{};VkFramebuffer framebuffer{};
    std::vector<Buffer> buffers;std::vector<Image> images;std::vector<VkShaderModule> shaders;
    static void check(VkResult result) {
        if(result!=VK_SUCCESS)throw std::runtime_error("postprocess capture Vulkan failure: "+std::to_string(result));
    }
    unsigned memory_type(unsigned bits,VkMemoryPropertyFlags flags) {
        VkPhysicalDeviceMemoryProperties properties;vkGetPhysicalDeviceMemoryProperties(physical,&properties);
        for(unsigned i=0;i<properties.memoryTypeCount;++i)
            if((bits&(1u<<i)) && (properties.memoryTypes[i].propertyFlags&flags)==flags)return i;
        throw std::runtime_error("postprocess capture memory unavailable");
    }
    Buffer make_buffer(VkDeviceSize size,VkBufferUsageFlags usage) {
        Buffer b;VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};info.size=size;info.usage=usage;
        check(vkCreateBuffer(device,&info,nullptr,&b.handle));
        VkMemoryRequirements requirements;vkGetBufferMemoryRequirements(device,b.handle,&requirements);
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocation.allocationSize=requirements.size;
        allocation.memoryTypeIndex=memory_type(requirements.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        check(vkAllocateMemory(device,&allocation,nullptr,&b.memory));check(vkBindBufferMemory(device,b.handle,b.memory,0));
        check(vkMapMemory(device,b.memory,0,size,0,&b.mapped));buffers.push_back(b);return b;
    }
    Image make_image(const PostImage &source,VkImageUsageFlags usage) {
        Image image;VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        info.imageType=source.view_type==VK_IMAGE_VIEW_TYPE_3D?VK_IMAGE_TYPE_3D:VK_IMAGE_TYPE_2D;
        info.format=source.format;info.extent={source.width,source.height,1};
        info.flags=source.view_type==VK_IMAGE_VIEW_TYPE_CUBE?VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT:0;
        info.mipLevels=source.levels;info.arrayLayers=source.view_type==VK_IMAGE_VIEW_TYPE_CUBE?6:1;
        info.samples=VK_SAMPLE_COUNT_1_BIT;info.tiling=VK_IMAGE_TILING_OPTIMAL;info.usage=usage;
        check(vkCreateImage(device,&info,nullptr,&image.handle));
        VkMemoryRequirements requirements;vkGetImageMemoryRequirements(device,image.handle,&requirements);
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocation.allocationSize=requirements.size;
        allocation.memoryTypeIndex=memory_type(requirements.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        check(vkAllocateMemory(device,&allocation,nullptr,&image.memory));check(vkBindImageMemory(device,image.handle,image.memory,0));
        VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};view.image=image.handle;view.format=source.format;
        view.viewType=source.view_type;
        view.subresourceRange={source.format==VK_FORMAT_D24_UNORM_S8_UINT?VK_IMAGE_ASPECT_DEPTH_BIT:VK_IMAGE_ASPECT_COLOR_BIT,0,source.levels,0,info.arrayLayers};
        check(vkCreateImageView(device,&view,nullptr,&image.view));
        VkSamplerCreateInfo sampler{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};sampler.magFilter=sampler.minFilter=source.filter;
        sampler.mipmapMode=source.levels>1?VK_SAMPLER_MIPMAP_MODE_LINEAR:VK_SAMPLER_MIPMAP_MODE_NEAREST;
        sampler.maxLod=float(source.levels-1);sampler.addressModeU=sampler.addressModeV=sampler.addressModeW=source.address;
        check(vkCreateSampler(device,&sampler,nullptr,&image.sampler));images.push_back(image);return image;
    }
    VkShaderModule load_shader(const std::filesystem::path &path) {
        std::ifstream file(path,std::ios::binary|std::ios::ate);
        if(!file || file.tellg()%4)throw std::runtime_error("postprocess capture shader unavailable: "+path.string());
        std::vector<unsigned> words(std::size_t(file.tellg())/4);file.seekg(0);file.read(reinterpret_cast<char*>(words.data()),words.size()*4);
        VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};info.codeSize=words.size()*4;info.pCode=words.data();
        VkShaderModule shader;check(vkCreateShaderModule(device,&info,nullptr,&shader));shaders.push_back(shader);return shader;
    }
    static void barrier(VkCommandBuffer command,VkImage image,VkImageLayout before,VkImageLayout after,
                        VkAccessFlags source,VkAccessFlags destination,VkPipelineStageFlags first,VkPipelineStageFlags last,unsigned levels=1,
                        VkImageAspectFlags aspect=VK_IMAGE_ASPECT_COLOR_BIT) {
        VkImageMemoryBarrier transition{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};transition.image=image;
        transition.oldLayout=before;transition.newLayout=after;transition.srcAccessMask=source;transition.dstAccessMask=destination;
        transition.srcQueueFamilyIndex=transition.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
        transition.subresourceRange={aspect,0,levels,0,1};
        vkCmdPipelineBarrier(command,first,last,0,0,nullptr,0,nullptr,1,&transition);
    }
public:
    Postprocess() {
        VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.apiVersion=VK_API_VERSION_1_1;
        VkInstanceCreateInfo info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};info.pApplicationInfo=&app;
        check(vkCreateInstance(&info,nullptr,&instance));unsigned count=0;check(vkEnumeratePhysicalDevices(instance,&count,nullptr));
        std::vector<VkPhysicalDevice> devices(count);check(vkEnumeratePhysicalDevices(instance,&count,devices.data()));
        for(auto candidate:devices) {
            vkGetPhysicalDeviceQueueFamilyProperties(candidate,&count,nullptr);std::vector<VkQueueFamilyProperties> families(count);
            vkGetPhysicalDeviceQueueFamilyProperties(candidate,&count,families.data());
            for(unsigned i=0;i<count;++i)if(families[i].queueFlags&VK_QUEUE_GRAPHICS_BIT){physical=candidate;family=i;break;}
            if(physical)break;
        }
        if(!physical)throw std::runtime_error("postprocess capture graphics device unavailable");
        const float priority=1;VkDeviceQueueCreateInfo queue_info{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        queue_info.queueFamilyIndex=family;queue_info.queueCount=1;queue_info.pQueuePriorities=&priority;
        VkDeviceCreateInfo create{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};create.queueCreateInfoCount=1;create.pQueueCreateInfos=&queue_info;
        VkPhysicalDeviceFeatures supported{},features{};vkGetPhysicalDeviceFeatures(physical,&supported);
        features.textureCompressionBC=supported.textureCompressionBC;
        features.geometryShader=supported.geometryShader;create.pEnabledFeatures=&features;
        check(vkCreateDevice(physical,&create,nullptr,&device));vkGetDeviceQueue(device,family,0,&queue);
        VkCommandPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};pool_info.queueFamilyIndex=family;
        check(vkCreateCommandPool(device,&pool_info,nullptr,&commands));
    }
    ~Postprocess() {
        if(device) {
            vkDeviceWaitIdle(device);
            if(framebuffer)vkDestroyFramebuffer(device,framebuffer,nullptr);
            if(pipeline)vkDestroyPipeline(device,pipeline,nullptr);
            if(pass)vkDestroyRenderPass(device,pass,nullptr);
            if(layout)vkDestroyPipelineLayout(device,layout,nullptr);
            if(pool)vkDestroyDescriptorPool(device,pool,nullptr);
            if(set_layout)vkDestroyDescriptorSetLayout(device,set_layout,nullptr);
            for(auto shader:shaders)vkDestroyShaderModule(device,shader,nullptr);
            for(auto image:images) {
                vkDestroySampler(device,image.sampler,nullptr);vkDestroyImageView(device,image.view,nullptr);
                vkDestroyImage(device,image.handle,nullptr);vkFreeMemory(device,image.memory,nullptr);
            }
            for(auto b:buffers) {vkUnmapMemory(device,b.memory);vkDestroyBuffer(device,b.handle,nullptr);vkFreeMemory(device,b.memory,nullptr);}
            vkDestroyCommandPool(device,commands,nullptr);vkDestroyDevice(device,nullptr);
        }
        if(instance)vkDestroyInstance(instance,nullptr);
    }
    Postprocess(const Postprocess &)=delete;Postprocess &operator=(const Postprocess &)=delete;
    VkPhysicalDeviceProperties physical_properties()const {
        VkPhysicalDeviceProperties result;vkGetPhysicalDeviceProperties(physical,&result);return result;
    }
    std::vector<std::byte> render(const std::filesystem::path &vertex,const std::filesystem::path &fragment,
        unsigned uniform_binding,std::span<const std::byte> uniform,std::span<const PostImage> sampled,
        unsigned width,unsigned height,PostBlend blend,std::span<const std::byte> previous={},
        std::span<const std::byte> vertex_uniform={},unsigned vertex_count=3,
        unsigned vertex_binding=4,VkCullModeFlags cull_mode=VK_CULL_MODE_NONE,
        VkFrontFace front_face=VK_FRONT_FACE_COUNTER_CLOCKWISE,
        const std::filesystem::path &geometry={},std::span<const std::byte> previous_depth={}) {
        if(pipeline)throw std::runtime_error("postprocess fixture can render one pass");
        if(width*height*4!=previous.size() && !previous.empty())throw std::runtime_error("postprocess backdrop size differs");
        if(width*height*4!=previous_depth.size() && !previous_depth.empty())throw std::runtime_error("postprocess depth size differs");
        auto ubo=make_buffer(uniform.size(),VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);std::memcpy(ubo.mapped,uniform.data(),uniform.size());
        Buffer vertex_ubo;
        if(!vertex_uniform.empty()) {
            vertex_ubo=make_buffer(vertex_uniform.size(),VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
            std::memcpy(vertex_ubo.mapped,vertex_uniform.data(),vertex_uniform.size());
        }
        auto readback=make_buffer(width*height*4,VK_BUFFER_USAGE_TRANSFER_DST_BIT);
        std::vector<VkDescriptorSetLayoutBinding> bindings{{uniform_binding,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr}};
        if(!vertex_uniform.empty())bindings.push_back({vertex_binding,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_VERTEX_BIT,nullptr});
        std::vector<Image> sampled_images;std::vector<Buffer> staging;
        for(const auto &source:sampled) {
            unsigned bytes=0;
            for(unsigned m=0;m<source.levels;++m) {
                const auto w=std::max(1u,source.width>>m),h=std::max(1u,source.height>>m);
                bytes+=source.format==VK_FORMAT_BC1_RGBA_UNORM_BLOCK?((w+3)/4)*((h+3)/4)*8:w*h*4;
            }
            if(source.view_type==VK_IMAGE_VIEW_TYPE_CUBE)bytes*=6;
            if(source.pixels.size()!=bytes)throw std::runtime_error("postprocess source size differs");
            const auto found=std::find_if(bindings.begin(),bindings.end(),[&](const auto &b){return b.binding==source.binding;});
            if(found==bindings.end())bindings.push_back({source.binding,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,source.array_element+1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr});
            else found->descriptorCount=std::max(found->descriptorCount,source.array_element+1);
            sampled_images.push_back(make_image(source,VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT));
            staging.push_back(make_buffer(source.pixels.size(),VK_BUFFER_USAGE_TRANSFER_SRC_BIT));
            std::memcpy(staging.back().mapped,source.pixels.data(),source.pixels.size());
        }
        const auto target=make_image({0,width,height},VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
        Image depth_target;Buffer depth_staging;
        if(!previous_depth.empty()) {
            depth_target=make_image({0,width,height,VK_FORMAT_D24_UNORM_S8_UINT},VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT);
            depth_staging=make_buffer(previous_depth.size(),VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
            std::memcpy(depth_staging.mapped,previous_depth.data(),previous_depth.size());
        }
        Buffer backdrop;if(!previous.empty()) {backdrop=make_buffer(previous.size(),VK_BUFFER_USAGE_TRANSFER_SRC_BIT);std::memcpy(backdrop.mapped,previous.data(),previous.size());}
        VkDescriptorSetLayoutCreateInfo set_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};set_info.bindingCount=bindings.size();set_info.pBindings=bindings.data();
        check(vkCreateDescriptorSetLayout(device,&set_info,nullptr,&set_layout));
        VkDescriptorPoolSize sizes[]{{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,vertex_uniform.empty()?1u:2u},{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,unsigned(sampled.size())}};
        VkDescriptorPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pool_info.maxSets=1;pool_info.poolSizeCount=2;pool_info.pPoolSizes=sizes;
        check(vkCreateDescriptorPool(device,&pool_info,nullptr,&pool));
        VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};allocate.descriptorPool=pool;allocate.descriptorSetCount=1;allocate.pSetLayouts=&set_layout;
        VkDescriptorSet set;check(vkAllocateDescriptorSets(device,&allocate,&set));
        VkDescriptorBufferInfo buffer_info{ubo.handle,0,uniform.size()};
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};write.dstSet=set;write.dstBinding=uniform_binding;
        write.descriptorCount=1;write.descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;write.pBufferInfo=&buffer_info;
        vkUpdateDescriptorSets(device,1,&write,0,nullptr);
        if(!vertex_uniform.empty()) {
            VkDescriptorBufferInfo vertex_info{vertex_ubo.handle,0,vertex_uniform.size()};
            write.dstBinding=vertex_binding;write.pBufferInfo=&vertex_info;
            vkUpdateDescriptorSets(device,1,&write,0,nullptr);
        }
        for(unsigned i=0;i<sampled.size();++i) {
            VkDescriptorImageInfo image_info{sampled_images[i].sampler,sampled_images[i].view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            write.dstBinding=sampled[i].binding;write.dstArrayElement=sampled[i].array_element;write.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;write.pBufferInfo=nullptr;write.pImageInfo=&image_info;
            vkUpdateDescriptorSets(device,1,&write,0,nullptr);
        }
        VkPipelineLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};layout_info.setLayoutCount=1;layout_info.pSetLayouts=&set_layout;
        check(vkCreatePipelineLayout(device,&layout_info,nullptr,&layout));
        VkAttachmentDescription attachment{};attachment.format=VK_FORMAT_R8G8B8A8_UNORM;attachment.samples=VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp=previous.empty()?VK_ATTACHMENT_LOAD_OP_CLEAR:VK_ATTACHMENT_LOAD_OP_LOAD;attachment.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
        attachment.initialLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;attachment.finalLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        VkAttachmentReference color{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};VkSubpassDescription subpass{};
        subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;subpass.colorAttachmentCount=1;subpass.pColorAttachments=&color;
        VkAttachmentDescription attachments[]{attachment,{}};
        VkAttachmentReference depth_reference{1,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        if(!previous_depth.empty()) {
            auto &a=attachments[1];a.format=VK_FORMAT_D24_UNORM_S8_UINT;a.samples=VK_SAMPLE_COUNT_1_BIT;
            a.loadOp=VK_ATTACHMENT_LOAD_OP_LOAD;a.storeOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
            a.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;a.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
            a.initialLayout=a.finalLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            subpass.pDepthStencilAttachment=&depth_reference;
        }
        VkSubpassDependency dependencies[]{{VK_SUBPASS_EXTERNAL,0,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,0},
            {0,VK_SUBPASS_EXTERNAL,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_ACCESS_TRANSFER_READ_BIT,0}};
        VkRenderPassCreateInfo render_info{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};render_info.attachmentCount=previous_depth.empty()?1:2;render_info.pAttachments=attachments;
        render_info.subpassCount=1;render_info.pSubpasses=&subpass;render_info.dependencyCount=2;render_info.pDependencies=dependencies;
        check(vkCreateRenderPass(device,&render_info,nullptr,&pass));
        VkImageView target_views[]{target.view,depth_target.view};
        VkFramebufferCreateInfo framebuffer_info{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};framebuffer_info.renderPass=pass;framebuffer_info.attachmentCount=render_info.attachmentCount;
        framebuffer_info.pAttachments=target_views;framebuffer_info.width=width;framebuffer_info.height=height;framebuffer_info.layers=1;
        check(vkCreateFramebuffer(device,&framebuffer_info,nullptr,&framebuffer));
        std::vector<VkPipelineShaderStageCreateInfo> stages{{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_VERTEX_BIT,load_shader(vertex),"main",nullptr},
            {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_FRAGMENT_BIT,load_shader(fragment),"main",nullptr}};
        if(!geometry.empty())stages.push_back({VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_GEOMETRY_BIT,load_shader(geometry),"main",nullptr});
        VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};ia.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkViewport viewport{0,0,float(width),float(height),0,1};VkRect2D scissor{{0,0},{width,height}};
        VkPipelineViewportStateCreateInfo vs{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};vs.viewportCount=vs.scissorCount=1;vs.pViewports=&viewport;vs.pScissors=&scissor;
        VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};rs.polygonMode=VK_POLYGON_MODE_FILL;rs.lineWidth=1;
        rs.cullMode=cull_mode;rs.frontFace=front_face;
        VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};ms.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
        VkPipelineColorBlendAttachmentState cb{};cb.colorWriteMask=15;cb.blendEnable=blend!=PostBlend::Disabled;
        cb.srcColorBlendFactor=VK_BLEND_FACTOR_ONE;cb.dstColorBlendFactor=VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;cb.colorBlendOp=VK_BLEND_OP_ADD;
        cb.srcAlphaBlendFactor=VK_BLEND_FACTOR_ZERO;cb.dstAlphaBlendFactor=VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;cb.alphaBlendOp=VK_BLEND_OP_ADD;
        if(blend==PostBlend::Screen) {
            cb.srcColorBlendFactor=VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;cb.dstColorBlendFactor=VK_BLEND_FACTOR_ONE;
            cb.srcAlphaBlendFactor=VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;cb.dstAlphaBlendFactor=VK_BLEND_FACTOR_ONE;
        }
        if(blend==PostBlend::AlphaRGB) {
            cb.srcColorBlendFactor=VK_BLEND_FACTOR_SRC_ALPHA;
            cb.colorWriteMask=7;
        }
        VkPipelineColorBlendStateCreateInfo bs{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};bs.attachmentCount=1;bs.pAttachments=&cb;
        VkPipelineDepthStencilStateCreateInfo ds{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
        ds.depthTestEnable=!previous_depth.empty();ds.depthCompareOp=VK_COMPARE_OP_LESS_OR_EQUAL;
        VkGraphicsPipelineCreateInfo pipeline_info{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};pipeline_info.stageCount=stages.size();pipeline_info.pStages=stages.data();
        pipeline_info.pVertexInputState=&vi;pipeline_info.pInputAssemblyState=&ia;pipeline_info.pViewportState=&vs;
        pipeline_info.pRasterizationState=&rs;pipeline_info.pMultisampleState=&ms;pipeline_info.pColorBlendState=&bs;pipeline_info.layout=layout;pipeline_info.renderPass=pass;
        pipeline_info.pDepthStencilState=&ds;
        check(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&pipeline_info,nullptr,&pipeline));
        VkCommandBufferAllocateInfo command_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};command_info.commandPool=commands;command_info.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;command_info.commandBufferCount=1;
        VkCommandBuffer command;check(vkAllocateCommandBuffers(device,&command_info,&command));
        VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};check(vkBeginCommandBuffer(command,&begin));
        const auto upload=[&](VkImage image,const Buffer &buffer,unsigned w,unsigned h,VkImageLayout destination,VkAccessFlags access,VkPipelineStageFlags stage,unsigned levels=1,VkFormat format=VK_FORMAT_R8G8B8A8_UNORM,unsigned layers=1) {
            const VkImageAspectFlags aspect=format==VK_FORMAT_D24_UNORM_S8_UINT?VK_IMAGE_ASPECT_DEPTH_BIT:VK_IMAGE_ASPECT_COLOR_BIT;
            const VkImageAspectFlags transition_aspect=format==VK_FORMAT_D24_UNORM_S8_UINT?VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT:VK_IMAGE_ASPECT_COLOR_BIT;
            // Cube transfers include all six faces in one copy.
            VkImageMemoryBarrier initial{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};initial.image=image;
            initial.oldLayout=VK_IMAGE_LAYOUT_UNDEFINED;initial.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            initial.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;initial.srcQueueFamilyIndex=initial.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
            initial.subresourceRange={transition_aspect,0,levels,0,layers};
            vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&initial);
            std::vector<VkBufferImageCopy> copies;VkDeviceSize offset=0;
            for(unsigned m=0;m<levels;++m) {
                const auto mw=std::max(1u,w>>m),mh=std::max(1u,h>>m);
                VkBufferImageCopy copy{};copy.bufferOffset=offset;copy.imageSubresource={aspect,m,0,layers};copy.imageExtent={mw,mh,1};copies.push_back(copy);
                offset+=(format==VK_FORMAT_BC1_RGBA_UNORM_BLOCK?((mw+3)/4)*((mh+3)/4)*8:mw*mh*4)*layers;
            }
            vkCmdCopyBufferToImage(command,buffer.handle,image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,copies.size(),copies.data());
            auto ready=initial;ready.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;ready.newLayout=destination;
            ready.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;ready.dstAccessMask=access;
            vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,stage,0,0,nullptr,0,nullptr,1,&ready);
        };
        for(unsigned i=0;i<sampled.size();++i)upload(sampled_images[i].handle,staging[i],sampled[i].width,sampled[i].height,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_ACCESS_SHADER_READ_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,sampled[i].levels,sampled[i].format,
            sampled[i].view_type==VK_IMAGE_VIEW_TYPE_CUBE?6:1);
        if(!previous_depth.empty())upload(depth_target.handle,depth_staging,width,height,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,1,VK_FORMAT_D24_UNORM_S8_UINT);
        if(previous.empty())barrier(command,target.handle,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            0,VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        else upload(target.handle,backdrop,width,height,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        VkClearValue clear{};VkRenderPassBeginInfo render{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};render.renderPass=pass;render.framebuffer=framebuffer;
        render.renderArea=scissor;render.clearValueCount=1;render.pClearValues=&clear;
        vkCmdBeginRenderPass(command,&render,VK_SUBPASS_CONTENTS_INLINE);vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline);
        vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,layout,0,1,&set,0,nullptr);vkCmdDraw(command,vertex_count,1,0,0);vkCmdEndRenderPass(command);
        VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={width,height,1};
        vkCmdCopyImageToBuffer(command,target.handle,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,readback.handle,1,&copy);
        VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER};host.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;host.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&host,0,nullptr,0,nullptr);
        check(vkEndCommandBuffer(command));VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;
        check(vkQueueSubmit(queue,1,&submit,VK_NULL_HANDLE));check(vkQueueWaitIdle(queue));
        std::vector<std::byte> result(width*height*4);std::memcpy(result.data(),readback.mapped,result.size());return result;
    }
};
}
