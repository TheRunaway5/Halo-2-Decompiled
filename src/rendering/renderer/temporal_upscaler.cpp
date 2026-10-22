#include "halo2/temporal_upscaler.hpp"
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <glm/gtc/matrix_inverse.hpp>
namespace halo2 {
namespace {
void checked(VkResult value,const char* what) {if(value!=VK_SUCCESS)throw std::runtime_error(std::string(what)+": "+std::to_string(value));}
float halton(unsigned i,unsigned base) {float value=0,f=1;while(i){f/=base;value+=f*(i%base);i/=base;}return value-.5f;}
struct Uniform {glm::mat4 inverse{1},previous{1},previous_foreground{1};glm::vec4 rectangle{},planes{},jitter{},size{};};
struct MotionPush {glm::mat4 matrix;float depth_scale;unsigned foreground{};};
struct MotionVertex {Vec3 current,previous;float valid{};};
struct HistoryVertex {Vec3 position;unsigned material,source_section,source_part;};
}
struct TemporalUpscaler::Impl {
    VkPhysicalDevice physical{};VkDevice device{};VkExtent2D input{},output{};
    std::unique_ptr<NativeUpscaler> native;
    struct Image {VkImage image{};VkImageView view{};VkDeviceMemory memory{};VkFormat format{};};
    Image color,motion,linear_depth,native_depth,reactive,result,history;
    VkImage raw_depth{};VkImageView raw_depth_view{};
    VkSampler sampler{};VkDescriptorSetLayout descriptors{};VkDescriptorPool pool{};
    VkPipelineLayout layout{};VkPipeline prepare{},motion_pipeline{};
    VkRenderPass motion_pass{};VkFramebuffer motion_framebuffer{};
    struct Buffer {VkBuffer buffer{};VkDeviceMemory memory{};void* mapped{};VkDeviceSize size{};};
    struct Slot {
        VkDescriptorSet descriptor{};Buffer uniform,vertices;
        glm::mat4 previous{1},previous_foreground{1};
        std::vector<HistoryVertex> positions,foreground;
        std::uint64_t last=~std::uint64_t{};Uniform data{};
    };
    std::array<Slot,32> slots;
    Buffer depth_transfer;
    std::uint64_t frame{};bool reset=true,force_reset{};unsigned views{},previous_views{};float delta=1.f/60;TemporalView last_view{};
    std::chrono::steady_clock::time_point previous_time{};
    bool images_initialized{};
    unsigned memory_type(unsigned bits,VkMemoryPropertyFlags flags) {
        VkPhysicalDeviceMemoryProperties props{};vkGetPhysicalDeviceMemoryProperties(physical,&props);
        for(unsigned i=0;i<props.memoryTypeCount;++i)if((bits&(1u<<i)) && (props.memoryTypes[i].propertyFlags&flags)==flags)return i;
        throw std::runtime_error("No compatible temporal image memory");
    }
    void free_buffer(Buffer& b) {
        if(b.mapped)vkUnmapMemory(device,b.memory);
        if(b.buffer)vkDestroyBuffer(device,b.buffer,nullptr);
        if(b.memory)vkFreeMemory(device,b.memory,nullptr);b={};
    }
    void make_buffer(Buffer& b,VkDeviceSize size,VkBufferUsageFlags usage,bool host=true) {
        if(b.size>=size)return;
        free_buffer(b);b.size=size;
        VkBufferCreateInfo create{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};create.size=size;create.usage=usage;
        checked(vkCreateBuffer(device,&create,nullptr,&b.buffer),"create temporal buffer");
        VkMemoryRequirements requirements{};vkGetBufferMemoryRequirements(device,b.buffer,&requirements);
        VkMemoryAllocateInfo allocate{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocate.allocationSize=requirements.size;
        allocate.memoryTypeIndex=memory_type(requirements.memoryTypeBits,host?VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT:VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        checked(vkAllocateMemory(device,&allocate,nullptr,&b.memory),"allocate temporal buffer");
        checked(vkBindBufferMemory(device,b.buffer,b.memory,0),"bind temporal buffer");
        if(host)checked(vkMapMemory(device,b.memory,0,size,0,&b.mapped),"map temporal buffer");
    }
    void image(Image& target,VkExtent2D size,VkFormat format) {
        target.format=format;
        VkImageCreateInfo create{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};create.imageType=VK_IMAGE_TYPE_2D;create.format=format;
        create.extent={size.width,size.height,1};create.mipLevels=create.arrayLayers=1;create.samples=VK_SAMPLE_COUNT_1_BIT;
        create.tiling=VK_IMAGE_TILING_OPTIMAL;create.usage=VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_STORAGE_BIT|
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        if(format==VK_FORMAT_D32_SFLOAT)create.usage=VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        checked(vkCreateImage(device,&create,nullptr,&target.image),"create temporal image");
        VkMemoryRequirements requirements{};vkGetImageMemoryRequirements(device,target.image,&requirements);
        VkMemoryAllocateInfo allocate{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocate.allocationSize=requirements.size;
        allocate.memoryTypeIndex=memory_type(requirements.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        checked(vkAllocateMemory(device,&allocate,nullptr,&target.memory),"allocate temporal image");
        checked(vkBindImageMemory(device,target.image,target.memory,0),"bind temporal image");
        VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};view.image=target.image;view.viewType=VK_IMAGE_VIEW_TYPE_2D;
        view.format=format;view.subresourceRange={format==VK_FORMAT_D32_SFLOAT?VK_IMAGE_ASPECT_DEPTH_BIT:VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
        checked(vkCreateImageView(device,&view,nullptr,&target.view),"create temporal view");
    }
    VkShaderModule shader(const std::filesystem::path& path) {
        std::ifstream file(path,std::ios::binary|std::ios::ate);
        if(!file || file.tellg()<=0 || file.tellg()%4)throw std::runtime_error("Missing temporal shader: "+path.string());
        std::vector<unsigned> words(std::size_t(file.tellg())/4);file.seekg(0);file.read(reinterpret_cast<char*>(words.data()),words.size()*4);
        VkShaderModuleCreateInfo create{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};create.codeSize=words.size()*4;create.pCode=words.data();
        VkShaderModule module{};checked(vkCreateShaderModule(device,&create,nullptr,&module),"create temporal shader");return module;
    }
    void barrier(VkCommandBuffer cmd,VkImage image,VkImageLayout old,VkImageLayout next,VkImageAspectFlags aspect=VK_IMAGE_ASPECT_COLOR_BIT) {
        VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};b.image=image;b.oldLayout=old;b.newLayout=next;
        b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;b.subresourceRange={aspect,0,1,0,1};
        b.srcAccessMask=old==VK_IMAGE_LAYOUT_UNDEFINED?0:VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT;
        b.dstAccessMask=VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT;
        vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,0,0,nullptr,0,nullptr,1,&b);
    }
    void write_images(Slot& slot,VkImageView scene) {
        const std::array<VkDescriptorImageInfo,7> infos{{
            {sampler,raw_depth_view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
            {sampler,scene,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
            {{},color.view,VK_IMAGE_LAYOUT_GENERAL},{{},motion.view,VK_IMAGE_LAYOUT_GENERAL},
            {{},linear_depth.view,VK_IMAGE_LAYOUT_GENERAL},{{},reactive.view,VK_IMAGE_LAYOUT_GENERAL},
            {sampler,history.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}}};
        std::array<VkWriteDescriptorSet,8> writes{};
        for(unsigned i=0;i<7;++i) {
            auto& w=writes[i];w.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;w.dstSet=slot.descriptor;
            w.dstBinding=i<6?i:7;w.descriptorCount=1;w.pImageInfo=&infos[i];
            w.descriptorType=(i==0 || i==1 || i==6)?VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        }
        VkDescriptorBufferInfo buffer{slot.uniform.buffer,0,sizeof(Uniform)};
        auto& w=writes[7];w.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;w.dstSet=slot.descriptor;w.dstBinding=6;
        w.descriptorCount=1;w.descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;w.pBufferInfo=&buffer;
        vkUpdateDescriptorSets(device,writes.size(),writes.data(),0,nullptr);
    }
    void initialize(const std::filesystem::path& shaders) {
        image(color,input,VK_FORMAT_R16G16B16A16_SFLOAT);image(history,input,VK_FORMAT_R16G16B16A16_SFLOAT);
        image(motion,input,VK_FORMAT_R16G16_SFLOAT);image(linear_depth,input,VK_FORMAT_R32_SFLOAT);
        image(native_depth,input,VK_FORMAT_D32_SFLOAT);
        make_buffer(depth_transfer,VkDeviceSize(input.width)*input.height*4,VK_BUFFER_USAGE_TRANSFER_SRC_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT,false);
        image(reactive,input,VK_FORMAT_R8_UNORM);image(result,output,VK_FORMAT_R16G16B16A16_SFLOAT);
        VkSamplerCreateInfo sample{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};sample.magFilter=sample.minFilter=VK_FILTER_NEAREST;
        sample.addressModeU=sample.addressModeV=sample.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        checked(vkCreateSampler(device,&sample,nullptr,&sampler),"create temporal sampler");
        std::array<VkDescriptorSetLayoutBinding,8> bindings{};
        for(unsigned i=0;i<bindings.size();++i) {
            bindings[i].binding=i;bindings[i].descriptorCount=1;bindings[i].stageFlags=VK_SHADER_STAGE_COMPUTE_BIT|VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT;
            bindings[i].descriptorType=i==6?VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                i==0 || i==1 || i==7?VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        }
        VkDescriptorSetLayoutCreateInfo desc{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};desc.bindingCount=bindings.size();desc.pBindings=bindings.data();
        checked(vkCreateDescriptorSetLayout(device,&desc,nullptr,&descriptors),"create temporal descriptors");
        const VkDescriptorPoolSize sizes[]{{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,96},{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,128},{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,32}};
        VkDescriptorPoolCreateInfo create_pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};create_pool.maxSets=32;create_pool.poolSizeCount=3;create_pool.pPoolSizes=sizes;
        checked(vkCreateDescriptorPool(device,&create_pool,nullptr,&pool),"create temporal descriptor pool");
        for(auto& slot:slots) {
            make_buffer(slot.uniform,sizeof(Uniform),VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
            VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};allocate.descriptorPool=pool;allocate.descriptorSetCount=1;allocate.pSetLayouts=&descriptors;
            checked(vkAllocateDescriptorSets(device,&allocate,&slot.descriptor),"allocate temporal descriptors");
            write_images(slot,history.view);
        }
        const VkPushConstantRange push{VK_SHADER_STAGE_COMPUTE_BIT|VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(MotionPush)};
        VkPipelineLayoutCreateInfo create_layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};create_layout.setLayoutCount=1;create_layout.pSetLayouts=&descriptors;
        create_layout.pushConstantRangeCount=1;create_layout.pPushConstantRanges=&push;
        checked(vkCreatePipelineLayout(device,&create_layout,nullptr,&layout),"create temporal layout");
        VkComputePipelineCreateInfo compute{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};compute.layout=layout;
        compute.stage={VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_COMPUTE_BIT,shader(shaders/"upscale_prepare.comp.spv"),"main",nullptr};
        const auto compute_status=vkCreateComputePipelines(device,{},1,&compute,nullptr,&prepare);vkDestroyShaderModule(device,compute.stage.module,nullptr);checked(compute_status,"create temporal preparation");
        std::array<VkAttachmentDescription,3> attachments{};
        const std::array<VkFormat,3> formats{motion.format,linear_depth.format,reactive.format};
        std::array<VkAttachmentReference,3> refs{};
        for(unsigned i=0;i<3;++i) {
            attachments[i].format=formats[i];attachments[i].samples=VK_SAMPLE_COUNT_1_BIT;attachments[i].loadOp=VK_ATTACHMENT_LOAD_OP_LOAD;
            attachments[i].storeOp=VK_ATTACHMENT_STORE_OP_STORE;attachments[i].initialLayout=attachments[i].finalLayout=VK_IMAGE_LAYOUT_GENERAL;
            refs[i]={i,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        }
        VkSubpassDescription subpass{};subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;subpass.colorAttachmentCount=3;subpass.pColorAttachments=refs.data();
        VkRenderPassCreateInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};pass.attachmentCount=3;pass.pAttachments=attachments.data();pass.subpassCount=1;pass.pSubpasses=&subpass;
        checked(vkCreateRenderPass(device,&pass,nullptr,&motion_pass),"create motion pass");
        const VkImageView views[]{motion.view,linear_depth.view,reactive.view};
        VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};fb.renderPass=motion_pass;fb.attachmentCount=3;fb.pAttachments=views;fb.width=input.width;fb.height=input.height;fb.layers=1;
        checked(vkCreateFramebuffer(device,&fb,nullptr,&motion_framebuffer),"create motion framebuffer");
        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0]={VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_VERTEX_BIT,shader(shaders/"upscale_motion.vert.spv"),"main",nullptr};
        stages[1]={VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_FRAGMENT_BIT,shader(shaders/"upscale_motion.frag.spv"),"main",nullptr};
        VkVertexInputBindingDescription vb{0,sizeof(MotionVertex),VK_VERTEX_INPUT_RATE_VERTEX};
        VkVertexInputAttributeDescription attributes[]{{0,0,VK_FORMAT_R32G32B32_SFLOAT,0},{1,0,VK_FORMAT_R32G32B32_SFLOAT,sizeof(Vec3)},{2,0,VK_FORMAT_R32_SFLOAT,2*sizeof(Vec3)}};
        VkPipelineVertexInputStateCreateInfo vertex{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};vertex.vertexBindingDescriptionCount=1;vertex.pVertexBindingDescriptions=&vb;
        vertex.vertexAttributeDescriptionCount=3;vertex.pVertexAttributeDescriptions=attributes;
        VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};assembly.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};viewport.viewportCount=viewport.scissorCount=1;
        VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};raster.polygonMode=VK_POLYGON_MODE_FILL;raster.lineWidth=1;
        VkPipelineMultisampleStateCreateInfo samples{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};samples.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
        std::array<VkPipelineColorBlendAttachmentState,3> blend{};for(auto& b:blend)b.colorWriteMask=15;
        VkPipelineColorBlendStateCreateInfo blends{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};blends.attachmentCount=3;blends.pAttachments=blend.data();
        const VkDynamicState dynamic[]{VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamics{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};dynamics.dynamicStateCount=2;dynamics.pDynamicStates=dynamic;
        VkGraphicsPipelineCreateInfo graphics{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};graphics.stageCount=2;graphics.pStages=stages;
        graphics.pVertexInputState=&vertex;graphics.pInputAssemblyState=&assembly;graphics.pViewportState=&viewport;graphics.pRasterizationState=&raster;
        graphics.pMultisampleState=&samples;graphics.pColorBlendState=&blends;graphics.pDynamicState=&dynamics;graphics.layout=layout;graphics.renderPass=motion_pass;
        const auto graphics_status=vkCreateGraphicsPipelines(device,{},1,&graphics,nullptr,&motion_pipeline);
        for(auto& stage:stages)vkDestroyShaderModule(device,stage.module,nullptr);checked(graphics_status,"create object motion pipeline");
    }
    ~Impl() {
        native.reset();
        for(auto& slot:slots){free_buffer(slot.vertices);free_buffer(slot.uniform);}
        free_buffer(depth_transfer);
        if(motion_pipeline)vkDestroyPipeline(device,motion_pipeline,nullptr);
        if(prepare)vkDestroyPipeline(device,prepare,nullptr);
        if(motion_framebuffer)vkDestroyFramebuffer(device,motion_framebuffer,nullptr);
        if(motion_pass)vkDestroyRenderPass(device,motion_pass,nullptr);
        if(layout)vkDestroyPipelineLayout(device,layout,nullptr);
        if(pool)vkDestroyDescriptorPool(device,pool,nullptr);
        if(descriptors)vkDestroyDescriptorSetLayout(device,descriptors,nullptr);
        if(sampler)vkDestroySampler(device,sampler,nullptr);
        for(auto* img:{&color,&motion,&linear_depth,&native_depth,&reactive,&result,&history}) {
            if(img->view)vkDestroyImageView(device,img->view,nullptr);
            if(img->image)vkDestroyImage(device,img->image,nullptr);
            if(img->memory)vkFreeMemory(device,img->memory,nullptr);
        }
    }
};
TemporalUpscaler::TemporalUpscaler(VkInstance instance,VkPhysicalDevice physical,VkDevice device,VkQueue queue,unsigned family,
    VkExtent2D input,VkExtent2D output,Upscaler method,UpscalingQuality quality,const std::filesystem::path& shaders,VkImage depth,VkImageView view,std::unique_ptr<NativeUpscaler> prepared):impl_(std::make_unique<Impl>()) {
    auto& p=*impl_;p.device=device;p.physical=physical;p.input=input;p.output=output;p.raw_depth=depth;p.raw_depth_view=view;
    p.native=prepared?std::move(prepared):std::make_unique<NativeUpscaler>(instance,physical,device,queue,family,input,output,method,quality);
    p.initialize(shaders);
}
TemporalUpscaler::~TemporalUpscaler()=default;
glm::vec2 TemporalUpscaler::jitter() const {
    const auto& p=*impl_;const auto phases=std::max(8u,unsigned(8.f*p.output.width*p.output.width/(float(p.input.width)*p.input.width)));
    const auto index=unsigned(p.frame%phases)+1;return {halton(index,2),halton(index,3)};
}
std::uint64_t TemporalUpscaler::frames() const{return impl_->frame;}
void TemporalUpscaler::reset_history(){impl_->force_reset=true;}
void TemporalUpscaler::begin(VkCommandBuffer cmd) {
    auto& p=*impl_;const auto now=std::chrono::steady_clock::now();
    p.delta=p.frame?std::chrono::duration<float>(now-p.previous_time).count():1.f/60;p.previous_time=now;
    p.reset=p.frame==0 || p.delta>.5f || p.force_reset;p.force_reset=false;p.views=0;p.delta=std::clamp(p.delta,.001f,.5f);
    const auto old=p.images_initialized?VK_IMAGE_LAYOUT_GENERAL:VK_IMAGE_LAYOUT_UNDEFINED;
    for(auto* image:{&p.color,&p.motion,&p.linear_depth,&p.reactive,&p.result})p.barrier(cmd,image->image,old,VK_IMAGE_LAYOUT_GENERAL);
    const VkImageSubresourceRange range{VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
    const VkClearColorValue zero{},one{{1,1,1,1}};
    vkCmdClearColorImage(cmd,p.motion.image,VK_IMAGE_LAYOUT_GENERAL,&zero,1,&range);
    vkCmdClearColorImage(cmd,p.linear_depth.image,VK_IMAGE_LAYOUT_GENERAL,&one,1,&range);
    vkCmdClearColorImage(cmd,p.reactive.image,VK_IMAGE_LAYOUT_GENERAL,&one,1,&range);
    if(!p.images_initialized) {
        p.barrier(cmd,p.history.image,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        vkCmdClearColorImage(cmd,p.history.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,&zero,1,&range);
        p.barrier(cmd,p.history.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
    auto& final_slot=p.slots.back();final_slot.data.size={p.input.width,p.input.height,0,0};
    std::memcpy(final_slot.uniform.mapped,&final_slot.data,sizeof(Uniform));
    p.images_initialized=true;
}
void TemporalUpscaler::view(VkCommandBuffer cmd,unsigned index,const TemporalView& view) {
    auto& p=*impl_;if(index>=p.slots.size()-1)throw std::runtime_error("Too many temporal views");
    p.views|=1u<<index;
    auto& slot=p.slots[index];
    float jump{};for(unsigned c=0;c<4;++c)for(unsigned r=0;r<4;++r)jump=std::max(jump,std::abs(view.matrix[c][r]-slot.previous[c][r]));
    const bool discontinuity=slot.last+1!=p.frame || p.reset || jump>8.f;
    if(jump>8.f)p.reset=true;
    const auto j=jitter();slot.data={glm::inverse(view.matrix),discontinuity?view.matrix:slot.previous,discontinuity?view.foreground_matrix:slot.previous_foreground,
        {view.rectangle.offset.x,view.rectangle.offset.y,view.rectangle.extent.width,view.rectangle.extent.height},
        {view.near_plane,view.far_plane,view.w_depth?1.f:0.f,discontinuity?1.f:0.f},
        {j.x,j.y,0,0},{p.input.width,p.input.height,0,0}};
    std::memcpy(slot.uniform.mapped,&slot.data,sizeof(Uniform));
    constexpr auto depth_aspect=VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT;
    p.barrier(cmd,p.raw_depth,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,depth_aspect);
    for(auto* image:{&p.motion,&p.linear_depth,&p.reactive})p.barrier(cmd,image->image,VK_IMAGE_LAYOUT_GENERAL,VK_IMAGE_LAYOUT_GENERAL);
    vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_COMPUTE,p.prepare);
    vkCmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_COMPUTE,p.layout,0,1,&slot.descriptor,0,nullptr);
    const unsigned phase=0;vkCmdPushConstants(cmd,p.layout,VK_SHADER_STAGE_COMPUTE_BIT|VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,4,&phase);
    vkCmdDispatch(cmd,(view.rectangle.extent.width+7)/8,(view.rectangle.extent.height+7)/8,1);
    const auto count=view.vertices.size()+view.foreground.size();
    if(count) {
        p.make_buffer(slot.vertices,count*sizeof(MotionVertex),VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        auto* out=static_cast<MotionVertex*>(slot.vertices.mapped);
        const auto append=[&](std::span<const Vertex> vertices,const std::vector<HistoryVertex>& previous,std::size_t first) {
            const bool continuous=!discontinuity && previous.size()==vertices.size();
            for(std::size_t i=0;i<vertices.size();++i) {
                const bool valid=continuous && previous[i].material==vertices[i].material && previous[i].source_section==vertices[i].source_section && previous[i].source_part==vertices[i].source_part;
                out[first+i]={vertices[i].position,valid?previous[i].position:vertices[i].position,valid?1.f:0.f};
            }
        };
        append(view.vertices,slot.positions,0);append(view.foreground,slot.foreground,view.vertices.size());
        for(auto* image:{&p.motion,&p.linear_depth,&p.reactive})p.barrier(cmd,image->image,VK_IMAGE_LAYOUT_GENERAL,VK_IMAGE_LAYOUT_GENERAL);
        VkRenderPassBeginInfo begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};begin.renderPass=p.motion_pass;begin.framebuffer=p.motion_framebuffer;begin.renderArea=view.rectangle;
        vkCmdBeginRenderPass(cmd,&begin,VK_SUBPASS_CONTENTS_INLINE);
        VkViewport viewport{float(view.rectangle.offset.x),float(view.rectangle.offset.y),float(view.rectangle.extent.width),float(view.rectangle.extent.height),0,1};
        vkCmdSetViewport(cmd,0,1,&viewport);vkCmdSetScissor(cmd,0,1,&view.rectangle);
        vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,p.motion_pipeline);
        vkCmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,p.layout,0,1,&slot.descriptor,0,nullptr);
        VkDeviceSize offset=0;vkCmdBindVertexBuffers(cmd,0,1,&slot.vertices.buffer,&offset);
        MotionPush push{view.matrix,1,0};
        vkCmdPushConstants(cmd,p.layout,VK_SHADER_STAGE_COMPUTE_BIT|VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(push),&push);
        if(!view.vertices.empty())vkCmdDraw(cmd,view.vertices.size(),1,0,0);
        if(!view.foreground.empty()) {
            push={view.foreground_matrix,view.w_depth?.15f:1.f,1};
            vkCmdPushConstants(cmd,p.layout,VK_SHADER_STAGE_COMPUTE_BIT|VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(push),&push);
            vkCmdDraw(cmd,view.foreground.size(),1,view.vertices.size(),0);
        }
        vkCmdEndRenderPass(cmd);
    }
    p.barrier(cmd,p.raw_depth,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,depth_aspect);
    const auto retain=[](std::vector<HistoryVertex>& history,std::span<const Vertex> vertices) {
        history.clear();history.reserve(vertices.size());
        for(const auto& v:vertices)history.push_back({v.position,v.material,v.source_section,v.source_part});
    };
    retain(slot.positions,view.vertices);retain(slot.foreground,view.foreground);
    slot.previous=view.matrix;slot.previous_foreground=view.foreground_matrix;slot.last=p.frame;
    p.last_view=view;p.last_view.vertices={};p.last_view.foreground={};
}
void TemporalUpscaler::present(VkCommandBuffer cmd,VkImage scene,VkImageView scene_view,VkImage destination,VkImageLayout scene_layout) {
    auto& p=*impl_;auto& slot=p.slots.back();
    // The final phase owns its descriptor set; view sets remain immutable
    // from their first recorded bind until the frame has completed.
    p.write_images(slot,scene_view);
    p.barrier(cmd,scene,scene_layout,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    p.barrier(cmd,p.raw_depth,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT);
    for(auto* image:{&p.motion,&p.linear_depth,&p.reactive})p.barrier(cmd,image->image,VK_IMAGE_LAYOUT_GENERAL,VK_IMAGE_LAYOUT_GENERAL);
    vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_COMPUTE,p.prepare);
    vkCmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_COMPUTE,p.layout,0,1,&slot.descriptor,0,nullptr);
    const unsigned phase=1;vkCmdPushConstants(cmd,p.layout,VK_SHADER_STAGE_COMPUTE_BIT|VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,4,&phase);
    vkCmdDispatch(cmd,(p.input.width+7)/8,(p.input.height+7)/8,1);
    // The unchanged donor FSR bridge treats its depth input as a depth
    // target. Preserve the exact IEEE floats while transferring the prepared
    // R32 samples into a real D32 image; no W-depth or stencil alias is exposed
    // to either temporal SDK.
    p.barrier(cmd,p.linear_depth.image,VK_IMAGE_LAYOUT_GENERAL,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    VkBufferImageCopy depth_copy{};depth_copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};depth_copy.imageExtent={p.input.width,p.input.height,1};
    vkCmdCopyImageToBuffer(cmd,p.linear_depth.image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,p.depth_transfer.buffer,1,&depth_copy);
    VkBufferMemoryBarrier depth_ready{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};depth_ready.buffer=p.depth_transfer.buffer;depth_ready.size=VK_WHOLE_SIZE;
    depth_ready.srcQueueFamilyIndex=depth_ready.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
    depth_ready.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;depth_ready.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
    vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,1,&depth_ready,0,nullptr);
    p.barrier(cmd,p.native_depth.image,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_ASPECT_DEPTH_BIT);
    depth_copy.imageSubresource.aspectMask=VK_IMAGE_ASPECT_DEPTH_BIT;
    vkCmdCopyBufferToImage(cmd,p.depth_transfer.buffer,p.native_depth.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&depth_copy);
    p.barrier(cmd,p.native_depth.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_IMAGE_ASPECT_DEPTH_BIT);
    p.barrier(cmd,p.linear_depth.image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_IMAGE_LAYOUT_GENERAL);
    for(auto* image:{&p.color,&p.motion,&p.reactive})p.barrier(cmd,image->image,VK_IMAGE_LAYOUT_GENERAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    auto resource=[](Impl::Image& image){return UpscalingImage{image.image,image.view,image.format};};
    p.reset|=p.views!=p.previous_views;p.previous_views=p.views;
    const auto j=jitter();UpscalingDispatch dispatch{cmd,resource(p.color),resource(p.native_depth),resource(p.motion),resource(p.reactive),resource(p.result),
        j.x,-j.y,p.delta,p.last_view.near_plane,p.last_view.far_plane,p.last_view.vertical_fov,p.reset};
    p.native->dispatch(dispatch);
    p.barrier(cmd,p.result.image,VK_IMAGE_LAYOUT_GENERAL,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    p.barrier(cmd,destination,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    VkImageBlit blit{};blit.srcSubresource=blit.dstSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};
    blit.srcOffsets[1]={int(p.output.width),int(p.output.height),1};blit.dstOffsets[1]=blit.srcOffsets[1];
    vkCmdBlitImage(cmd,p.result.image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,destination,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&blit,VK_FILTER_NEAREST);
    p.barrier(cmd,p.result.image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_IMAGE_LAYOUT_GENERAL);
    p.barrier(cmd,p.color.image,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    p.barrier(cmd,p.history.image,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    VkImageCopy copy{};copy.srcSubresource=copy.dstSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.extent={p.input.width,p.input.height,1};
    vkCmdCopyImage(cmd,p.color.image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,p.history.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy);
    p.barrier(cmd,p.history.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    p.barrier(cmd,p.color.image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_IMAGE_LAYOUT_GENERAL);
    for(auto* image:{&p.motion,&p.reactive})p.barrier(cmd,image->image,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_IMAGE_LAYOUT_GENERAL);
    p.barrier(cmd,p.raw_depth,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT);
    p.barrier(cmd,scene,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    ++p.frame;
}
}
