#include "halo2/source_lens_draw_gpu.hpp"
#include <fstream>
#include <map>
#include <cstring>
#include <cmath>
#include <limits>
namespace halo2 {
namespace {
void check(VkResult r){if(r!=VK_SUCCESS)throw FormatError("Lens draw Vulkan failure: "+std::to_string(r));}
VkShaderModule shader(VkDevice device,const std::filesystem::path &path) {
    std::ifstream f(path,std::ios::binary|std::ios::ate);
    if(!f || f.tellg()<=0 || f.tellg()%4)throw FormatError("Original lens draw shader is missing: "+path.string());
    std::vector<std::uint32_t> code(std::size_t(f.tellg())/4);f.seekg(0);
    if(!f.read(reinterpret_cast<char*>(code.data()),code.size()*4))throw FormatError("Original lens shader read failed");
    VkShaderModuleCreateInfo i{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};i.codeSize=code.size()*4;i.pCode=code.data();
    VkShaderModule result{};check(vkCreateShaderModule(device,&i,nullptr,&result));return result;
}
}
struct SourceLensDrawGpu::Impl {
    SourceLensDrawGpuContext context;
    VkDescriptorSetLayout descriptors{};VkDescriptorPool pool{};VkPipelineLayout layout{};
    std::array<VkPipeline,2> pipelines{};VkBuffer buffer{};VkDeviceMemory memory{};std::size_t capacity{};
    struct Image {SourceLensImageBinding binding;VkDescriptorSet set{};};
    std::map<unsigned,Image> images;std::vector<SourceLensDraw> prepared;
    explicit Impl(SourceLensDrawGpuContext c):context(std::move(c)){}
    void initialize() {
        if(!context.physical || !context.device || !context.render_pass)throw FormatError("Lens draws require an actual renderer device/pass");
        VkDescriptorSetLayoutBinding binding{0,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr};
        VkDescriptorSetLayoutCreateInfo di{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};di.bindingCount=1;di.pBindings=&binding;
        check(vkCreateDescriptorSetLayout(context.device,&di,nullptr,&descriptors));
        VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1024};
        VkDescriptorPoolCreateInfo pi{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pi.maxSets=1024;pi.poolSizeCount=1;pi.pPoolSizes=&size;
        check(vkCreateDescriptorPool(context.device,&pi,nullptr,&pool));
        VkPushConstantRange push{VK_SHADER_STAGE_VERTEX_BIT,0,8};
        VkPipelineLayoutCreateInfo li{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};li.setLayoutCount=1;li.pSetLayouts=&descriptors;li.pushConstantRangeCount=1;li.pPushConstantRanges=&push;
        check(vkCreatePipelineLayout(context.device,&li,nullptr,&layout));
        const auto vertex=shader(context.device,context.shader_directory/"source_lens_draw.vert.spv");VkShaderModule fragment{};
        try {
            fragment=shader(context.device,context.shader_directory/"source_lens_draw.frag.spv");
            std::array<VkPipelineShaderStageCreateInfo,2> stages{};
            for(auto &s:stages){s.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;s.pName="main";}
            stages[0].stage=VK_SHADER_STAGE_VERTEX_BIT;stages[0].module=vertex;
            stages[1].stage=VK_SHADER_STAGE_FRAGMENT_BIT;stages[1].module=fragment;
            VkVertexInputBindingDescription stream{0,sizeof(SourceLensVertex),VK_VERTEX_INPUT_RATE_VERTEX};
            const std::array<VkVertexInputAttributeDescription,4> attrs{{
                {0,0,VK_FORMAT_R32G32B32A32_SFLOAT,0},{1,0,VK_FORMAT_R32G32_SFLOAT,16},
                {2,0,VK_FORMAT_R8G8B8A8_UNORM,24},{3,0,VK_FORMAT_R32_SFLOAT,28}}};
            VkPipelineVertexInputStateCreateInfo input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
            input.vertexBindingDescriptionCount=1;input.pVertexBindingDescriptions=&stream;input.vertexAttributeDescriptionCount=attrs.size();input.pVertexAttributeDescriptions=attrs.data();
            VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};assembly.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
            VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};viewport.viewportCount=1;viewport.scissorCount=1;
            VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};raster.polygonMode=VK_POLYGON_MODE_FILL;raster.cullMode=VK_CULL_MODE_NONE;raster.lineWidth=1;
            VkPipelineMultisampleStateCreateInfo multi{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};multi.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
            VkPipelineDepthStencilStateCreateInfo depth{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};depth.depthCompareOp=VK_COMPARE_OP_LESS_OR_EQUAL;
            // Literal2DEE0: SRC_ALPHA, ONE, ADD, RGB writes, no depth writes.
            VkPipelineColorBlendAttachmentState attachment{};attachment.blendEnable=VK_TRUE;
            attachment.srcColorBlendFactor=attachment.srcAlphaBlendFactor=VK_BLEND_FACTOR_SRC_ALPHA;
            attachment.dstColorBlendFactor=attachment.dstAlphaBlendFactor=VK_BLEND_FACTOR_ONE;
            attachment.colorBlendOp=attachment.alphaBlendOp=VK_BLEND_OP_ADD;
            attachment.colorWriteMask=VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT;
            VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};blend.attachmentCount=1;blend.pAttachments=&attachment;
            const std::array<VkDynamicState,2> states{VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};
            VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};dynamic.dynamicStateCount=states.size();dynamic.pDynamicStates=states.data();
            VkGraphicsPipelineCreateInfo p{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};p.stageCount=stages.size();p.pStages=stages.data();p.pVertexInputState=&input;p.pInputAssemblyState=&assembly;p.pViewportState=&viewport;p.pRasterizationState=&raster;p.pMultisampleState=&multi;p.pDepthStencilState=&depth;p.pColorBlendState=&blend;p.pDynamicState=&dynamic;p.layout=layout;p.renderPass=context.render_pass;
            for(unsigned n=0;n<2;++n){depth.depthTestEnable=n;check(vkCreateGraphicsPipelines(context.device,VK_NULL_HANDLE,1,&p,nullptr,&pipelines[n]));}
        }catch(...){if(fragment)vkDestroyShaderModule(context.device,fragment,nullptr);vkDestroyShaderModule(context.device,vertex,nullptr);throw;}
        vkDestroyShaderModule(context.device,fragment,nullptr);vkDestroyShaderModule(context.device,vertex,nullptr);
    }
    void free_buffer(){if(buffer)vkDestroyBuffer(context.device,buffer,nullptr);if(memory)vkFreeMemory(context.device,memory,nullptr);buffer={};memory={};capacity=0;}
    void reserve(std::size_t bytes) {
        if(bytes<=capacity)return;
        free_buffer();
        VkBufferCreateInfo b{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};b.size=bytes;b.usage=VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;check(vkCreateBuffer(context.device,&b,nullptr,&buffer));
        VkMemoryRequirements needs{};vkGetBufferMemoryRequirements(context.device,buffer,&needs);
        VkPhysicalDeviceMemoryProperties props{};vkGetPhysicalDeviceMemoryProperties(context.physical,&props);unsigned type=props.memoryTypeCount;
        for(unsigned n=0;n<props.memoryTypeCount;++n)if((needs.memoryTypeBits&(1u<<n)) && (props.memoryTypes[n].propertyFlags&(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))==(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)){type=n;break;}
        if(type==props.memoryTypeCount)throw FormatError("Lens geometry lacks coherent GPU storage");
        VkMemoryAllocateInfo a{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};a.allocationSize=needs.size;a.memoryTypeIndex=type;check(vkAllocateMemory(context.device,&a,nullptr,&memory));check(vkBindBufferMemory(context.device,buffer,memory,0));capacity=bytes;
    }
    ~Impl(){if(context.device)vkDeviceWaitIdle(context.device);free_buffer();for(auto p:pipelines)if(p)vkDestroyPipeline(context.device,p,nullptr);if(pool)vkDestroyDescriptorPool(context.device,pool,nullptr);if(layout)vkDestroyPipelineLayout(context.device,layout,nullptr);if(descriptors)vkDestroyDescriptorSetLayout(context.device,descriptors,nullptr);}
};
SourceLensDrawGpu::SourceLensDrawGpu(SourceLensDrawGpuContext c):impl_(std::make_unique<Impl>(std::move(c))){impl_->initialize();}
SourceLensDrawGpu::~SourceLensDrawGpu()=default;
void SourceLensDrawGpu::prepare(std::span<const SourceLensDraw> draws,std::span<const SourceLensImageBinding> bindings) {
    auto &s=*impl_;
    for(const auto &b:bindings) {
        if(!b.view || !b.sampler)throw FormatError("Lens bitmap lacks a real resident image/sampler");
        if(const auto found=s.images.find(b.material);found!=s.images.end()) {
            if(found->second.binding.view!=b.view || found->second.binding.sampler!=b.sampler)throw FormatError("Resident lens bitmap changed without retiring its consumer");
            continue;
        }
        Impl::Image image;image.binding=b;VkDescriptorSetAllocateInfo a{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};a.descriptorPool=s.pool;a.descriptorSetCount=1;a.pSetLayouts=&s.descriptors;check(vkAllocateDescriptorSets(s.context.device,&a,&image.set));
        VkDescriptorImageInfo info{b.sampler,b.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkWriteDescriptorSet w{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};w.dstSet=image.set;w.dstBinding=0;w.descriptorCount=1;w.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;w.pImageInfo=&info;vkUpdateDescriptorSets(s.context.device,1,&w,0,nullptr);s.images.emplace(b.material,image);
    }
    if(draws.size()>std::numeric_limits<std::uint32_t>::max()/4)
        throw FormatError("Lens geometry exceeds native vertex addressing");
    for(const auto &d:draws) {
        if(!s.images.contains(d.material))throw FormatError("Lens draw has no admitted resident bitmap");
        for(const auto &v:d.vertices) {
            for(float value:v.position)if(!std::isfinite(value))throw FormatError("Lens GPU position is not finite");
            if(v.position[3]<=0 || v.position[3]!=d.vertices[0].position[3] ||
               !std::isfinite(v.uv.x) || !std::isfinite(v.uv.y) ||
               !std::isfinite(v.secondary_alpha) || v.secondary_alpha<0 || v.secondary_alpha>1)
                throw FormatError("Lens GPU geometry lost its original W/UV/attribute8 inputs");
        }
    }
    s.prepared.assign(draws.begin(),draws.end());if(draws.empty())return;
    s.reserve(draws.size()*128);void *mapped{};check(vkMapMemory(s.context.device,s.memory,0,VK_WHOLE_SIZE,0,&mapped));
    for(unsigned n=0;n<draws.size();++n)std::memcpy(static_cast<std::byte*>(mapped)+n*128,draws[n].vertices.data(),128);
    vkUnmapMemory(s.context.device,s.memory);
}
void SourceLensDrawGpu::record(VkCommandBuffer command,std::span<const SourceLensDraw> draws,unsigned first,VkExtent2D extent,VkRect2D clip) {
    auto &s=*impl_;if(!command || first>s.prepared.size() || draws.size()>s.prepared.size()-first || !extent.width || !extent.height)throw FormatError("Lens draw escaped its prepared view");
    const VkViewport viewport{0,0,float(extent.width),float(extent.height),0,1};vkCmdSetViewport(command,0,1,&viewport);vkCmdSetScissor(command,0,1,&clip);
    const std::array<float,2> size{float(extent.width),float(extent.height)};vkCmdPushConstants(command,s.layout,VK_SHADER_STAGE_VERTEX_BIT,0,8,size.data());
    VkDeviceSize offset{};vkCmdBindVertexBuffers(command,0,1,&s.buffer,&offset);
    for(unsigned n=0;n<draws.size();++n) {
        const auto &d=draws[n],&saved=s.prepared[first+n];
        if(d.material!=saved.material || d.depth_test!=saved.depth_test || std::memcmp(d.vertices.data(),saved.vertices.data(),128))throw FormatError("Lens draw changed after its original packet upload");
        vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,s.pipelines[d.depth_test]);const auto set=s.images.at(d.material).set;
        vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,s.layout,0,1,&set,0,nullptr);vkCmdDraw(command,4,1,(first+n)*4,0);
    }
}
}
