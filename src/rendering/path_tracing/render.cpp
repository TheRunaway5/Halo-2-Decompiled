#include "halo2/path_tracing/render.hpp"
#include "halo2/path_tracing/scene.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <utility>
#include <vector>

namespace halo2::path_tracing {
namespace {
void check(VkResult result,const char* operation) {
    if(result!=VK_SUCCESS)
        throw FormatError(std::string("Path Tracing ")+operation+": "+std::to_string(result));
}
struct Buffer {
    VkDevice device{};
    VkBuffer buffer{};
    VkDeviceMemory memory{};
    void* mapped{};
    VkDeviceSize capacity{};
    VkDeviceSize populated{};
    VkDeviceSize allocation_bytes{};
    VkMemoryPropertyFlags memory_flags{};
    Buffer()=default;
    Buffer(const Buffer&)=delete;
    Buffer& operator=(const Buffer&)=delete;
    ~Buffer(){clear();}
    void clear()noexcept {
        if(mapped)vkUnmapMemory(device,memory);
        if(buffer)vkDestroyBuffer(device,buffer,nullptr);
        if(memory)vkFreeMemory(device,memory,nullptr);
        buffer=VK_NULL_HANDLE;memory=VK_NULL_HANDLE;mapped=nullptr;capacity=populated=allocation_bytes=0;memory_flags=0;
    }
    void swap(Buffer& other)noexcept {
        std::swap(device,other.device);std::swap(buffer,other.buffer);std::swap(memory,other.memory);
        std::swap(mapped,other.mapped);std::swap(capacity,other.capacity);std::swap(populated,other.populated);
        std::swap(allocation_bytes,other.allocation_bytes);
        std::swap(memory_flags,other.memory_flags);
    }
};
struct ShaderModules {
    VkDevice device{};
    std::array<VkShaderModule,2> modules{};
    ~ShaderModules(){for(auto shader:modules)if(shader)vkDestroyShaderModule(device,shader,nullptr);}
};
template<class T> std::span<const std::byte> bytes(const std::vector<T>& value) {
    return std::as_bytes(std::span<const T>(value));
}
std::uint64_t temporal_lighting_key(const Scene& scene,const GpuSceneUniform& uniform) {
    std::uint64_t key=14695981039346656037ull;
    const auto add=[&](std::span<const std::byte> value) {
        for(const auto byte:value){key^=std::to_integer<unsigned>(byte);key*=1099511628211ull;}
    };
    add(bytes(scene.lights));
    add(bytes(scene.color_guide_image_keys));
    for(const auto& material:scene.materials)add(std::as_bytes(std::span(material.color_guide)));
    add(std::as_bytes(std::span(uniform.counts).subspan(2,2)));
    add(std::as_bytes(std::span(uniform.environment).first(3)));
    // Only actual admitted emitters participate. Unused UI/transparent roots
    // can have emissive constants without supplying any ray-scene triangles.
    std::vector<std::uint32_t> emitter_roots(scene.materials.size());
    for(std::uint32_t n=0;n<scene.emissive_count;++n) {
        const auto at=std::size_t(scene.texture_texel_count)+n;
        if(at>=scene.pixels.size() || scene.pixels[at]>=scene.triangles.size())
            throw FormatError("Path Tracing temporal emitter escaped its scene bank");
        const auto material=scene.triangles[scene.pixels[at]].metadata[0];
        if(material>=emitter_roots.size())throw FormatError("Path Tracing temporal emitter lost its material");
        ++emitter_roots[material];
    }
    for(std::size_t n=0;n<emitter_roots.size();++n)if(emitter_roots[n]) {
        const auto& material=scene.materials[n];const auto& emission=material.emission;
        add(std::as_bytes(std::span(&n,1)));add(std::as_bytes(std::span(&emitter_roots[n],1)));
        add(std::as_bytes(std::span(&material.texture[3],1)));
        const auto pointer=std::bit_cast<std::uint32_t>(emission[3]);
        if(!pointer) {
            // Constant-only callers have no authored animation interface;
            // an energy/image replacement is a real source discontinuity.
            add(std::as_bytes(std::span(emission).first(3)));
            if(n<scene.material_image_keys.size())add(std::as_bytes(std::span(scene.material_image_keys[n])));
            continue;
        }
        const auto header=std::size_t(pointer-1);
        constexpr std::size_t record_words=48;
        if(header>=scene.texture_texel_count || scene.texture_texel_count-header<2 ||
           scene.pixels[header]!=0x45525448u || scene.pixels[header+1]>64 ||
           scene.pixels[header+1]>(scene.texture_texel_count-header-2)/record_words)
            throw FormatError("Path Tracing temporal emission lost its source payload");
        const auto passes=scene.pixels[header+1];add(std::as_bytes(std::span(&passes,1)));
        for(std::uint32_t pass=0;pass<passes;++pass) {
            const auto record=header+2+pass*record_words;
            add(std::as_bytes(std::span(scene.pixels).subspan(record,2))); // Native family and active stages.
            for(unsigned stage=0;stage<4;++stage)if((scene.pixels[record+1]>>(stage*5))&31u) {
                const auto image=record+8+stage*10;
                add(std::as_bytes(std::span(scene.pixels).subspan(image+1,3))); // Dimensions and sampler.
                add(std::as_bytes(std::span(scene.pixels).subspan(image+8,2))); // Border and alpha kill.
            }
        }
        // Qualified native RGB constants, UVs and bitmap frames are sampled
        // afresh by every ray. They are time-varying radiance, not a new light
        // identity: clearing the entire history for each packed-color step
        // prevents real animated maps from ever accumulating a second sample.
    }
    return key;
}
}
struct Render::Impl {
    RenderContext context;
    VkPhysicalDeviceProperties properties{};
    VkPhysicalDeviceMemoryProperties memory_properties{};
    VkDescriptorSetLayout scene_layout{};
    VkDescriptorPool pool{};
    VkDescriptorSet descriptor{};
    VkPipelineLayout pipeline_layout{};
    VkPipeline graphics_pipeline{};
    VkImage environment_image{},environment_depth{};
    VkDeviceMemory environment_memory{},environment_depth_memory{};
    VkImageView environment_view{},environment_depth_view{};
    std::array<VkImageView,6> environment_faces{};
    std::array<VkFramebuffer,6> environment_framebuffers{};
    VkSampler environment_sampler{};
    VkRenderPass environment_pass{};
    VkFormat environment_color_format{},environment_depth_format{};
    unsigned environment_side{};
    VkDeviceSize environment_allocation_bytes{};
    bool environment_recording{},environment_source_available{},environment_fallback_available{};
    VkRenderPass visibility_pass{};
    VkPipeline visibility_pipeline{};
    VkRenderPass lighting_pass{};
    VkPipeline lighting_pipeline{};
    VkImage visibility_depth{};
    VkImageView visibility_view{};
    VkDeviceMemory visibility_memory{};
    VkDeviceSize visibility_allocation_bytes{};
    VkFramebuffer visibility_framebuffer{};
    VkFramebuffer lighting_framebuffer{};
    VkExtent2D visibility_extent{};
    std::array<Buffer,9> buffers;
    // Binding9 is an immutable previous-frame snapshot. Keeping its read bank
    // separate avoids races when several current pixels reproject to the same
    // old cell. Only the two bounded16-word histories are copied on the GPU.
    struct alignas(16) HistoryCamera {
        std::array<std::array<float,16>,2> matrix{};
        std::array<std::array<float,4>,2> viewport{};
    };
    static_assert(sizeof(HistoryCamera)==160);
    HistoryCamera current_history_camera{},previous_history_camera{};
    std::array<float,2> current_history_eye{},previous_history_eye{};
    std::uint32_t current_history_camera_mask{},previous_history_camera_mask{};
    VkViewport visibility_viewport{};
    bool previous_history_prepared{};
    const Scene* uploaded_color_guide_source{};
    std::vector<std::array<std::uint64_t,2>> uploaded_color_guide_keys;
    bool uploaded{},visibility_ready{},visibility_recording{},lighting_ready{},lighting_recording{};
    bool history_reset_pending=true,lighting_key_valid{};
    std::uint32_t frame_serial{};
    std::uint64_t lighting_key{};
    const Scene* uploaded_source{};
    unsigned lighting_scale=2;
    unsigned samples=1,bounces=1;
    void clear_environment()noexcept {
        for(auto& framebuffer:environment_framebuffers) {
            if(framebuffer)vkDestroyFramebuffer(context.device,framebuffer,nullptr);
            framebuffer=VK_NULL_HANDLE;
        }
        for(auto& view:environment_faces) {
            if(view)vkDestroyImageView(context.device,view,nullptr);
            view=VK_NULL_HANDLE;
        }
        if(environment_sampler)vkDestroySampler(context.device,environment_sampler,nullptr);
        if(environment_view)vkDestroyImageView(context.device,environment_view,nullptr);
        if(environment_depth_view)vkDestroyImageView(context.device,environment_depth_view,nullptr);
        if(environment_image)vkDestroyImage(context.device,environment_image,nullptr);
        if(environment_depth)vkDestroyImage(context.device,environment_depth,nullptr);
        if(environment_memory)vkFreeMemory(context.device,environment_memory,nullptr);
        if(environment_depth_memory)vkFreeMemory(context.device,environment_depth_memory,nullptr);
        if(environment_pass)vkDestroyRenderPass(context.device,environment_pass,nullptr);
        environment_image=environment_depth=VK_NULL_HANDLE;
        environment_memory=environment_depth_memory=VK_NULL_HANDLE;
        environment_view=environment_depth_view=VK_NULL_HANDLE;
        environment_sampler=VK_NULL_HANDLE;environment_pass=VK_NULL_HANDLE;
        environment_allocation_bytes=0;environment_side=0;environment_recording=false;
    }
    void clear_visibility_target()noexcept {
        if(lighting_framebuffer)vkDestroyFramebuffer(context.device,lighting_framebuffer,nullptr);
        if(visibility_framebuffer)vkDestroyFramebuffer(context.device,visibility_framebuffer,nullptr);
        if(visibility_view)vkDestroyImageView(context.device,visibility_view,nullptr);
        if(visibility_depth)vkDestroyImage(context.device,visibility_depth,nullptr);
        if(visibility_memory)vkFreeMemory(context.device,visibility_memory,nullptr);
        visibility_framebuffer=VK_NULL_HANDLE;visibility_view=VK_NULL_HANDLE;
        lighting_framebuffer=VK_NULL_HANDLE;
        visibility_depth=VK_NULL_HANDLE;visibility_memory=VK_NULL_HANDLE;
        visibility_allocation_bytes=0;visibility_extent={};visibility_ready=lighting_ready=false;
        history_reset_pending=true;
        current_history_camera_mask=previous_history_camera_mask=0;
        previous_history_prepared=false;
    }
    ~Impl() {
        clear_environment();
        clear_visibility_target();
        for(auto& buffer:buffers)buffer.clear();
        if(visibility_pipeline)vkDestroyPipeline(context.device,visibility_pipeline,nullptr);
        if(visibility_pass)vkDestroyRenderPass(context.device,visibility_pass,nullptr);
        if(lighting_pipeline)vkDestroyPipeline(context.device,lighting_pipeline,nullptr);
        if(lighting_pass)vkDestroyRenderPass(context.device,lighting_pass,nullptr);
        if(graphics_pipeline)vkDestroyPipeline(context.device,graphics_pipeline,nullptr);
        if(pipeline_layout)vkDestroyPipelineLayout(context.device,pipeline_layout,nullptr);
        if(pool)vkDestroyDescriptorPool(context.device,pool,nullptr);
        if(scene_layout)vkDestroyDescriptorSetLayout(context.device,scene_layout,nullptr);
    }
    unsigned memory_type(std::uint32_t mask,VkMemoryPropertyFlags required=
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)const {
        // Random BVH and texture reads must stay in GPU memory when a mapped
        // device-local type is available. The first compatible host type on
        // discrete devices can instead live in system RAM across PCIe.
        for(const auto preferred:{required|VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                  required|VK_MEMORY_PROPERTY_HOST_CACHED_BIT,required})
            for(unsigned i=0;i<memory_properties.memoryTypeCount;++i)
                if((mask&(1u<<i)) && (memory_properties.memoryTypes[i].propertyFlags&preferred)==preferred)return i;
        throw FormatError("Path Tracing has no memory type for the requested resource");
    }
    void allocate(Buffer& destination,VkDeviceSize required,VkBufferUsageFlags usage,VkDeviceSize limit) {
        if(required<=destination.capacity)return;
        Buffer replacement;replacement.device=context.device;
        replacement.capacity=std::min(limit,std::max(required,destination.capacity+destination.capacity/2));
        VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        info.size=replacement.capacity;info.usage=usage;info.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
        check(vkCreateBuffer(context.device,&info,nullptr,&replacement.buffer),"create scene buffer");
        VkMemoryRequirements requirements{};vkGetBufferMemoryRequirements(context.device,replacement.buffer,&requirements);
        VkMemoryAllocateInfo memory{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        memory.allocationSize=requirements.size;memory.memoryTypeIndex=memory_type(requirements.memoryTypeBits);
        replacement.memory_flags=memory_properties.memoryTypes[memory.memoryTypeIndex].propertyFlags;
        check(vkAllocateMemory(context.device,&memory,nullptr,&replacement.memory),"allocate scene buffer");
        replacement.allocation_bytes=memory.allocationSize;
        check(vkBindBufferMemory(context.device,replacement.buffer,replacement.memory,0),"bind scene buffer");
        check(vkMapMemory(context.device,replacement.memory,0,replacement.capacity,0,&replacement.mapped),"map scene buffer");
        destination.swap(replacement);
    }
    VkShaderModule shader(const char* name)const {
        const auto path=context.shaders/"path_tracing"/name;
        std::ifstream file(path,std::ios::binary|std::ios::ate);
        const auto length=file.tellg();
        if(length<=0 || length%4 || std::uint64_t(length)>64u*1024u*1024u)
            throw FormatError("Path Tracing shader unavailable or invalid: "+path.string());
        std::vector<std::uint32_t> words(std::size_t(length)/4);file.seekg(0);
        if(!file.read(reinterpret_cast<char*>(words.data()),length))
            throw FormatError("Path Tracing shader truncated: "+path.string());
        VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        info.codeSize=words.size()*sizeof(std::uint32_t);info.pCode=words.data();
        VkShaderModule result{};check(vkCreateShaderModule(context.device,&info,nullptr,&result),"create shader module");return result;
    }
    void create() {
        if(!context.device || !context.physical || !context.render_pass || !context.material_layout)
            throw FormatError("Path Tracing requires a device, render pass, and material descriptor layout");
        vkGetPhysicalDeviceProperties(context.physical,&properties);
        vkGetPhysicalDeviceMemoryProperties(context.physical,&memory_properties);
        if(properties.limits.maxPerStageDescriptorStorageBuffers<8 || properties.limits.maxDescriptorSetStorageBuffers<8 ||
           properties.limits.maxPushConstantsSize<camera_bytes)
            throw FormatError("Path Tracing scene resources exceed this Vulkan device's descriptor or push-constant limits");
        std::array<VkDescriptorSetLayoutBinding,10> bindings{};
        for(unsigned i=0;i<bindings.size();++i) {
            bindings[i].binding=i;bindings[i].descriptorCount=1;bindings[i].stageFlags=VK_SHADER_STAGE_FRAGMENT_BIT;
            bindings[i].descriptorType=i==7?VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                i==5?VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        }
        VkDescriptorSetLayoutCreateInfo scene{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        scene.bindingCount=bindings.size();scene.pBindings=bindings.data();
        check(vkCreateDescriptorSetLayout(context.device,&scene,nullptr,&scene_layout),"create scene descriptor layout");
        const std::array layouts{context.material_layout,scene_layout};
        const VkPushConstantRange push{VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,camera_bytes};
        VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        layout.setLayoutCount=layouts.size();layout.pSetLayouts=layouts.data();layout.pushConstantRangeCount=1;layout.pPushConstantRanges=&push;
        check(vkCreatePipelineLayout(context.device,&layout,nullptr,&pipeline_layout),"create pipeline layout");
        const VkDescriptorPoolSize sizes[]{{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,8},{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1}};
        VkDescriptorPoolCreateInfo descriptor_pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        descriptor_pool.maxSets=1;descriptor_pool.poolSizeCount=3;descriptor_pool.pPoolSizes=sizes;
        check(vkCreateDescriptorPool(context.device,&descriptor_pool,nullptr,&pool),"create descriptor pool");
        VkDescriptorSetAllocateInfo allocation{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocation.descriptorPool=pool;allocation.descriptorSetCount=1;allocation.pSetLayouts=&scene_layout;
        check(vkAllocateDescriptorSets(context.device,&allocation,&descriptor),"allocate scene descriptor");
        graphics_pipeline=make_pipeline(context.render_pass,false);
    }
    VkPipeline make_pipeline(VkRenderPass pass,bool visibility,bool lighting=false) {
        ShaderModules modules;modules.device=context.device;
        modules.modules[0]=shader("world.vert.spv");
        modules.modules[1]=shader(visibility?"depth.frag.spv":lighting?"lighting.frag.spv":"world.frag.spv");
        const VkPipelineShaderStageCreateInfo stages[]{
            {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_VERTEX_BIT,modules.modules[0],"main",nullptr},
            {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_FRAGMENT_BIT,modules.modules[1],"main",nullptr}};
        const VkVertexInputBindingDescription binding{0,sizeof(Vertex),VK_VERTEX_INPUT_RATE_VERTEX};
        const VkVertexInputAttributeDescription attributes[]{
            {0,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex,position)},
            {1,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex,color)},
            {2,0,VK_FORMAT_R32G32_SFLOAT,offsetof(Vertex,uv)},
            {3,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex,normal)},
            {4,0,VK_FORMAT_R32_UINT,offsetof(Vertex,material)},
            {5,0,VK_FORMAT_R32_UINT,offsetof(Vertex,source_stipple_level)},
            {6,0,VK_FORMAT_R32_UINT,offsetof(Vertex,lightmap)},
            {7,0,VK_FORMAT_R32_SINT,offsetof(Vertex,bsp_cluster)},
            {8,0,VK_FORMAT_R32G32_SFLOAT,offsetof(Vertex,lightmap_uv)}};
        VkPipelineVertexInputStateCreateInfo input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        input.vertexBindingDescriptionCount=1;input.pVertexBindingDescriptions=&binding;
        input.vertexAttributeDescriptionCount=std::size(attributes);input.pVertexAttributeDescriptions=attributes;
        VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        assembly.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        viewport.viewportCount=viewport.scissorCount=1;
        VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        raster.polygonMode=VK_POLYGON_MODE_FILL;raster.cullMode=context.cull_mode;
        raster.frontFace=VK_FRONT_FACE_COUNTER_CLOCKWISE;raster.lineWidth=1;
        VkPipelineMultisampleStateCreateInfo samples{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        samples.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
        VkPipelineDepthStencilStateCreateInfo depth{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
        depth.depthTestEnable=VK_TRUE;depth.depthWriteEnable=lighting?VK_FALSE:VK_TRUE;
        depth.depthCompareOp=VK_COMPARE_OP_LESS_OR_EQUAL;
        VkPipelineColorBlendAttachmentState attachment{};
        attachment.colorWriteMask=VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT;
        VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        blend.attachmentCount=visibility || lighting?0u:1u;
        blend.pAttachments=visibility || lighting?nullptr:&attachment;
        const VkDynamicState states[]{VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
        dynamic.dynamicStateCount=std::size(states);dynamic.pDynamicStates=states;
        VkGraphicsPipelineCreateInfo pipeline{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        pipeline.stageCount=std::size(stages);pipeline.pStages=stages;pipeline.pVertexInputState=&input;
        pipeline.pInputAssemblyState=&assembly;pipeline.pViewportState=&viewport;pipeline.pRasterizationState=&raster;
        pipeline.pMultisampleState=&samples;pipeline.pDepthStencilState=&depth;pipeline.pColorBlendState=&blend;
        pipeline.pDynamicState=&dynamic;pipeline.layout=pipeline_layout;pipeline.renderPass=pass;
        VkPipeline result{};
        check(vkCreateGraphicsPipelines(context.device,VK_NULL_HANDLE,1,&pipeline,nullptr,&result),"create graphics pipeline");
        return result;
    }
    void prepare_environment(VkFormat color,VkFormat depth,unsigned side) {
        if(environment_recording)throw FormatError("Path Tracing environment resized while recording");
        if(environment_side==side && environment_color_format==color && environment_depth_format==depth)return;
        if(!side || side>properties.limits.maxImageDimensionCube || side>properties.limits.maxFramebufferWidth ||
            side>properties.limits.maxFramebufferHeight)
            throw FormatError("Path Tracing environment has an unsupported extent");
        const auto supported=[&](VkFormat format,VkImageUsageFlags usage,VkImageCreateFlags flags,unsigned layers) {
            VkImageFormatProperties image{};
            return vkGetPhysicalDeviceImageFormatProperties(context.physical,format,VK_IMAGE_TYPE_2D,
                VK_IMAGE_TILING_OPTIMAL,usage,flags,&image)==VK_SUCCESS && image.maxArrayLayers>=layers &&
                image.maxExtent.width>=side && image.maxExtent.height>=side;
        };
        if(!supported(color,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,6) ||
            !supported(depth,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,0,1))
            throw FormatError("Path Tracing environment requires compatible native color/depth attachments");
        VkFormatProperties filtering{};vkGetPhysicalDeviceFormatProperties(context.physical,color,&filtering);
        if(!(filtering.optimalTilingFeatures&VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
            throw FormatError("Path Tracing environment requires filtered native sky samples");
        clear_environment();environment_color_format=color;environment_depth_format=depth;
        const auto image=[&](VkFormat format,VkImageUsageFlags usage,VkImageCreateFlags flags,unsigned layers,
                             VkImage& target,VkDeviceMemory& memory) {
            VkImageCreateInfo create{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};create.flags=flags;
            create.imageType=VK_IMAGE_TYPE_2D;create.format=format;create.extent={side,side,1};
            create.mipLevels=1;create.arrayLayers=layers;create.samples=VK_SAMPLE_COUNT_1_BIT;
            create.tiling=VK_IMAGE_TILING_OPTIMAL;create.usage=usage;create.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
            check(vkCreateImage(context.device,&create,nullptr,&target),"create environment image");
            VkMemoryRequirements requirements{};vkGetImageMemoryRequirements(context.device,target,&requirements);
            VkMemoryAllocateInfo allocate{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocate.allocationSize=requirements.size;
            allocate.memoryTypeIndex=memory_type(requirements.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            check(vkAllocateMemory(context.device,&allocate,nullptr,&memory),"allocate environment image");
            environment_allocation_bytes+=requirements.size;
            check(vkBindImageMemory(context.device,target,memory,0),"bind environment image");
        };
        image(color,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,6,
            environment_image,environment_memory);
        image(depth,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,0,1,environment_depth,environment_depth_memory);
        const auto view=[&](VkImage target,VkFormat format,VkImageViewType type,VkImageAspectFlags aspect,
                            unsigned first,unsigned count,VkImageView& result) {
            VkImageViewCreateInfo create{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};create.image=target;
            create.viewType=type;create.format=format;create.subresourceRange={aspect,0,1,first,count};
            check(vkCreateImageView(context.device,&create,nullptr,&result),"create environment image view");
        };
        view(environment_image,color,VK_IMAGE_VIEW_TYPE_CUBE,VK_IMAGE_ASPECT_COLOR_BIT,0,6,environment_view);
        const bool stencil=depth==VK_FORMAT_D24_UNORM_S8_UINT || depth==VK_FORMAT_D32_SFLOAT_S8_UINT || depth==VK_FORMAT_D16_UNORM_S8_UINT;
        view(environment_depth,depth,VK_IMAGE_VIEW_TYPE_2D,VK_IMAGE_ASPECT_DEPTH_BIT|(stencil?VK_IMAGE_ASPECT_STENCIL_BIT:0),0,1,environment_depth_view);
        for(unsigned face=0;face<6;++face)
            view(environment_image,color,VK_IMAGE_VIEW_TYPE_2D,VK_IMAGE_ASPECT_COLOR_BIT,face,1,environment_faces[face]);
        std::array<VkAttachmentDescription,2> attachments{};
        attachments[0].format=color;attachments[0].samples=VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;attachments[0].storeOp=VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;attachments[0].finalLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        attachments[1].format=depth;attachments[1].samples=VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;attachments[1].storeOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].stencilLoadOp=stencil?VK_ATTACHMENT_LOAD_OP_CLEAR:VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[1].stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;attachments[1].finalLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        const VkAttachmentReference color_reference{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        const VkAttachmentReference depth_reference{1,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        VkSubpassDescription subpass{};subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount=1;subpass.pColorAttachments=&color_reference;subpass.pDepthStencilAttachment=&depth_reference;
        std::array<VkSubpassDependency,2> dependencies{};
        dependencies[0].srcSubpass=VK_SUBPASS_EXTERNAL;dependencies[0].dstSubpass=0;
        dependencies[0].srcStageMask=VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependencies[0].dstStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependencies[0].srcAccessMask=VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependencies[0].dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT|
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependencies[1].srcSubpass=0;dependencies[1].dstSubpass=VK_SUBPASS_EXTERNAL;
        // Match the ordinary renderer's two dependencies as well as its
        // attachment formats/subpass. Native sky pipelines remain compatible.
        dependencies[1].srcStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask=VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[1].srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
        VkRenderPassCreateInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};pass.attachmentCount=attachments.size();
        pass.pAttachments=attachments.data();pass.subpassCount=1;pass.pSubpasses=&subpass;
        pass.dependencyCount=dependencies.size();pass.pDependencies=dependencies.data();
        check(vkCreateRenderPass(context.device,&pass,nullptr,&environment_pass),"create environment render pass");
        for(unsigned face=0;face<6;++face) {
            const std::array views{environment_faces[face],environment_depth_view};
            VkFramebufferCreateInfo framebuffer{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};framebuffer.renderPass=environment_pass;
            framebuffer.attachmentCount=views.size();framebuffer.pAttachments=views.data();
            framebuffer.width=framebuffer.height=side;framebuffer.layers=1;
            check(vkCreateFramebuffer(context.device,&framebuffer,nullptr,&environment_framebuffers[face]),"create environment framebuffer");
        }
        VkSamplerCreateInfo sampler{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};sampler.magFilter=sampler.minFilter=VK_FILTER_LINEAR;
        sampler.mipmapMode=VK_SAMPLER_MIPMAP_MODE_NEAREST;sampler.addressModeU=sampler.addressModeV=sampler.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        check(vkCreateSampler(context.device,&sampler,nullptr,&environment_sampler),"create environment sampler");
        const VkDescriptorImageInfo info{environment_sampler,environment_view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};write.dstSet=descriptor;write.dstBinding=7;
        write.descriptorCount=1;write.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;write.pImageInfo=&info;
        vkUpdateDescriptorSets(context.device,1,&write,0,nullptr);environment_side=side;
    }
    static VkDeviceSize lighting_cells(VkExtent2D extent,unsigned scale) {
        return VkDeviceSize((extent.width+scale-1)/scale)*((extent.height+scale-1)/scale);
    }
    VkDeviceSize lighting_offset()const {
        return 32+VkDeviceSize(visibility_extent.width)*visibility_extent.height*sizeof(float);
    }
    VkDeviceSize lighting_bytes()const {
        return lighting_cells(visibility_extent,lighting_scale)*12*sizeof(std::uint32_t);
    }
    VkDeviceSize visibility_bytes()const {
        //32-byte immutable frame header, full-resolution D32, current12-word
        // records and separate16-word world/foreground history records.
        return lighting_offset()+lighting_cells(visibility_extent,lighting_scale)*(12+2*16)*sizeof(std::uint32_t);
    }
    VkDeviceSize history_offset()const {return lighting_offset()+lighting_bytes();}
    VkDeviceSize history_bytes()const {
        return lighting_cells(visibility_extent,lighting_scale)*2*16*sizeof(std::uint32_t);
    }
    void write_previous_history_mask() {
        const auto mask=history_reset_pending?0u:previous_history_camera_mask;
        std::memcpy(static_cast<std::byte*>(buffers[6].mapped)+6*sizeof(std::uint32_t),&mask,sizeof(mask));
    }
    bool environment_nee_available()const {
        return environment_source_available || environment_fallback_available;
    }
    void update_visibility_binding() {
        const auto size=visibility_bytes();
        if(size>properties.limits.maxStorageBufferRange)
            throw FormatError("Path Tracing visibility exceeds maxStorageBufferRange");
        auto& buffer=buffers[6];
        allocate(buffer,size,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 properties.limits.maxStorageBufferRange);
        const std::array<std::uint32_t,8> dimensions{visibility_extent.width,visibility_extent.height,
            visibility_extent.width*visibility_extent.height,lighting_scale,frame_serial,
            environment_nee_available()?1u:0u,history_reset_pending?0u:previous_history_camera_mask,0};
        std::memcpy(buffer.mapped,dimensions.data(),sizeof(dimensions));buffer.populated=sizeof(dimensions);
        const auto previous_size=sizeof(HistoryCamera)+history_bytes();
        if(previous_size>properties.limits.maxStorageBufferRange)
            throw FormatError("Path Tracing previous history exceeds maxStorageBufferRange");
        auto& previous=buffers[8];
        allocate(previous,previous_size,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                 properties.limits.maxStorageBufferRange);
        std::memcpy(previous.mapped,&previous_history_camera,sizeof(previous_history_camera));
        previous.populated=previous_size;
        const VkDescriptorBufferInfo infos[]{{buffer.buffer,0,size},{previous.buffer,0,previous_size}};
        std::array<VkWriteDescriptorSet,2> writes{};
        for(unsigned i=0;i<writes.size();++i) {
            auto& write=writes[i];write.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet=descriptor;write.dstBinding=i?9:6;write.descriptorCount=1;
            write.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;write.pBufferInfo=&infos[i];
        }
        vkUpdateDescriptorSets(context.device,writes.size(),writes.data(),0,nullptr);
    }
    void snapshot_previous_history(VkCommandBuffer command) {
        if(previous_history_prepared)return;
        previous_history_prepared=true;
        if(history_reset_pending)previous_history_camera_mask=0;
        write_previous_history_mask();
        if(!previous_history_camera_mask || !history_bytes())return;
        std::array<VkBufferMemoryBarrier,2> barriers{};
        for(auto& barrier:barriers) {
            barrier.sType=VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
            barrier.size=history_bytes();
        }
        barriers[0].buffer=buffers[6].buffer;barriers[0].offset=history_offset();
        barriers[0].srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_TRANSFER_WRITE_BIT;
        barriers[0].dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
        barriers[1].buffer=buffers[8].buffer;barriers[1].offset=sizeof(HistoryCamera);
        barriers[1].srcAccessMask=VK_ACCESS_SHADER_READ_BIT;barriers[1].dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,barriers.size(),barriers.data(),0,nullptr);
        const VkBufferCopy copy{history_offset(),sizeof(HistoryCamera),history_bytes()};
        vkCmdCopyBuffer(command,buffers[6].buffer,buffers[8].buffer,1,&copy);
        barriers[0].srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
        barriers[0].dstAccessMask=VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_TRANSFER_WRITE_BIT;
        barriers[1].srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;barriers[1].dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,0,nullptr,barriers.size(),barriers.data(),0,nullptr);
    }
    void capture_history_camera(std::span<const std::byte> camera) {
        float eye;std::memcpy(&eye,camera.data()+23*sizeof(float),sizeof(eye));
        const unsigned role=eye==2.f?1:0;const auto bit=1u<<role;
        if((previous_history_camera_mask&bit) &&
           std::bit_cast<std::uint32_t>(eye)!=std::bit_cast<std::uint32_t>(previous_history_eye[role])) {
            previous_history_camera_mask&=~bit;
            write_previous_history_mask();
        }
        std::memcpy(current_history_camera.matrix[role].data(),camera.data(),16*sizeof(float));
        current_history_camera.viewport[role]={visibility_viewport.x,visibility_viewport.y,
            visibility_viewport.width,visibility_viewport.height};
        current_history_eye[role]=eye;current_history_camera_mask|=bit;
    }
    void prepare_visibility(VkExtent2D extent) {
        if(visibility_recording || lighting_recording)throw FormatError("Path Tracing visibility cannot resize while recording");
        visibility_ready=lighting_ready=false;
        if(!extent.width || !extent.height ||
           extent.width>properties.limits.maxFramebufferWidth || extent.height>properties.limits.maxFramebufferHeight ||
           extent.width>properties.limits.maxImageDimension2D || extent.height>properties.limits.maxImageDimension2D ||
           std::uint64_t(extent.width)*extent.height>std::numeric_limits<std::uint32_t>::max() ||
           32+std::uint64_t(extent.width)*extent.height*sizeof(float)+(12+2*16)*sizeof(std::uint32_t)>properties.limits.maxStorageBufferRange)
            throw FormatError("Path Tracing visibility has an unsupported extent");
        if(extent.width==visibility_extent.width && extent.height==visibility_extent.height)return;
        lighting_scale=2;
        const auto depth_bytes=32+VkDeviceSize(extent.width)*extent.height*sizeof(float);
        while(depth_bytes+lighting_cells(extent,lighting_scale)*(12+2*16)*sizeof(std::uint32_t)>
              properties.limits.maxStorageBufferRange)
            lighting_scale=std::min(std::max(extent.width,extent.height),lighting_scale*2);
        constexpr auto usage=VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        VkImageFormatProperties support{};
        if(vkGetPhysicalDeviceImageFormatProperties(context.physical,VK_FORMAT_D32_SFLOAT,VK_IMAGE_TYPE_2D,
            VK_IMAGE_TILING_OPTIMAL,usage,0,&support)!=VK_SUCCESS ||
           extent.width>support.maxExtent.width || extent.height>support.maxExtent.height)
            throw FormatError("Path Tracing visibility requires a transferable D32 depth attachment");
        if(!visibility_pass) {
            VkAttachmentDescription attachment{};attachment.format=VK_FORMAT_D32_SFLOAT;
            attachment.samples=VK_SAMPLE_COUNT_1_BIT;attachment.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachment.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
            attachment.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;attachment.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;attachment.finalLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            const VkAttachmentReference depth{0,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
            VkSubpassDescription subpass{};subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.pDepthStencilAttachment=&depth;
            std::array<VkSubpassDependency,2> dependencies{};
            dependencies[0].srcSubpass=VK_SUBPASS_EXTERNAL;dependencies[0].dstSubpass=0;
            dependencies[0].srcStageMask=VK_PIPELINE_STAGE_TRANSFER_BIT|
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            dependencies[0].dstStageMask=VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            dependencies[0].srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT|
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            dependencies[0].dstAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            dependencies[1].srcSubpass=0;dependencies[1].dstSubpass=VK_SUBPASS_EXTERNAL;
            dependencies[1].srcStageMask=VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            dependencies[1].dstStageMask=VK_PIPELINE_STAGE_TRANSFER_BIT;
            dependencies[1].srcAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            dependencies[1].dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
            VkRenderPassCreateInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
            pass.attachmentCount=1;pass.pAttachments=&attachment;pass.subpassCount=1;pass.pSubpasses=&subpass;
            pass.dependencyCount=dependencies.size();pass.pDependencies=dependencies.data();
            check(vkCreateRenderPass(context.device,&pass,nullptr,&visibility_pass),"create visibility render pass");
        }
        if(!visibility_pipeline)visibility_pipeline=make_pipeline(visibility_pass,true);
        if(!lighting_pass) {
            VkAttachmentDescription attachment{};attachment.format=VK_FORMAT_D32_SFLOAT;
            attachment.samples=VK_SAMPLE_COUNT_1_BIT;attachment.loadOp=VK_ATTACHMENT_LOAD_OP_LOAD;
            attachment.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
            attachment.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;attachment.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment.initialLayout=attachment.finalLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            const VkAttachmentReference depth{0,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
            VkSubpassDescription subpass{};subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.pDepthStencilAttachment=&depth;
            std::array<VkSubpassDependency,2> dependencies{};
            dependencies[0].srcSubpass=VK_SUBPASS_EXTERNAL;dependencies[0].dstSubpass=0;
            dependencies[0].srcStageMask=VK_PIPELINE_STAGE_TRANSFER_BIT;
            dependencies[0].dstStageMask=VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT|
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dependencies[0].srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT|VK_ACCESS_TRANSFER_WRITE_BIT;
            // STORE still writes the attachment when the lighting pipeline
            // disables depth writes. Order that store and its final layout
            // transition as well as the depth tests and HDR storage writes.
            dependencies[0].dstAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT|
                VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT;
            dependencies[1].srcSubpass=0;dependencies[1].dstSubpass=VK_SUBPASS_EXTERNAL;
            dependencies[1].srcStageMask=VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            dependencies[1].dstStageMask=VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_TRANSFER_BIT;
            dependencies[1].srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            dependencies[1].dstAccessMask=VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_TRANSFER_READ_BIT;
            VkRenderPassCreateInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
            pass.attachmentCount=1;pass.pAttachments=&attachment;pass.subpassCount=1;pass.pSubpasses=&subpass;
            pass.dependencyCount=dependencies.size();pass.pDependencies=dependencies.data();
            check(vkCreateRenderPass(context.device,&pass,nullptr,&lighting_pass),"create HDR lighting render pass");
        }
        if(!lighting_pipeline)lighting_pipeline=make_pipeline(lighting_pass,false,true);
        clear_visibility_target();
        VkImageCreateInfo image{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        image.imageType=VK_IMAGE_TYPE_2D;image.format=VK_FORMAT_D32_SFLOAT;
        image.extent={extent.width,extent.height,1};image.mipLevels=image.arrayLayers=1;
        image.samples=VK_SAMPLE_COUNT_1_BIT;image.tiling=VK_IMAGE_TILING_OPTIMAL;
        image.usage=usage;image.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
        check(vkCreateImage(context.device,&image,nullptr,&visibility_depth),"create visibility depth image");
        VkMemoryRequirements requirements{};vkGetImageMemoryRequirements(context.device,visibility_depth,&requirements);
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocation.allocationSize=requirements.size;allocation.memoryTypeIndex=memory_type(requirements.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        check(vkAllocateMemory(context.device,&allocation,nullptr,&visibility_memory),"allocate visibility depth memory");
        visibility_allocation_bytes=requirements.size;
        check(vkBindImageMemory(context.device,visibility_depth,visibility_memory,0),"bind visibility depth memory");
        VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        view.image=visibility_depth;view.viewType=VK_IMAGE_VIEW_TYPE_2D;view.format=VK_FORMAT_D32_SFLOAT;
        view.subresourceRange={VK_IMAGE_ASPECT_DEPTH_BIT,0,1,0,1};
        check(vkCreateImageView(context.device,&view,nullptr,&visibility_view),"create visibility depth view");
        VkFramebufferCreateInfo framebuffer{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebuffer.renderPass=visibility_pass;framebuffer.attachmentCount=1;framebuffer.pAttachments=&visibility_view;
        framebuffer.width=extent.width;framebuffer.height=extent.height;framebuffer.layers=1;
        check(vkCreateFramebuffer(context.device,&framebuffer,nullptr,&visibility_framebuffer),"create visibility framebuffer");
        framebuffer.renderPass=lighting_pass;
        check(vkCreateFramebuffer(context.device,&framebuffer,nullptr,&lighting_framebuffer),"create HDR lighting framebuffer");
        visibility_extent=extent;update_visibility_binding();
    }
    void bind(VkCommandBuffer command,VkDescriptorSet material,std::uint32_t dynamic_bank_offset,
              std::span<const std::byte> camera,std::uint32_t material_index,bool visibility,bool lighting=false) {
        if(!uploaded)throw FormatError("Path Tracing scene must be uploaded before binding");
        if(camera.size()!=camera_bytes || (material_index!=~0u && material_index>=0x80000000u))
            throw FormatError("Path Tracing draw lost its camera or material identity");
        if(visibility!=visibility_recording || lighting!=lighting_recording || (visibility && lighting))
            throw FormatError("Path Tracing material pipeline does not match the visibility phase");
        if(visibility || lighting)capture_history_camera(camera);
        std::array<std::byte,camera_bytes> push;std::copy(camera.begin(),camera.end(),push.begin());
        const std::array<float,2> quality{float(samples),float(bounces)};
        std::memcpy(push.data()+camera_bytes-4*sizeof(float),quality.data(),sizeof(quality));
        // RT does not otherwise consume viewport.z. A push-constant flag is
        // immutable for each recorded draw, even when another camera phase
        // overwrites the visibility pixels later in this command buffer.
        const float guarded=lighting?2.f:!visibility && lighting_ready?3.f:!visibility && visibility_ready?1.f:0.f;
        std::memcpy(push.data()+camera_bytes-2*sizeof(float),&guarded,sizeof(guarded));
        if(material_index!=~0u) {
            const auto binding=material_index|0x80000000u;
            std::memcpy(push.data()+camera_bytes-sizeof(binding),&binding,sizeof(binding));
        }
        const VkDescriptorSet sets[]{material,descriptor};
        vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,
            visibility?visibility_pipeline:lighting?lighting_pipeline:graphics_pipeline);
        vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline_layout,0,2,sets,1,&dynamic_bank_offset);
        vkCmdPushConstants(command,pipeline_layout,VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,
                           0,camera_bytes,push.data());
    }
};
Render::Render(RenderContext context):impl_(std::make_unique<Impl>()) {
    impl_->context=std::move(context);impl_->create();
}
Render::~Render()=default;
void Render::set_quality(unsigned samples,unsigned bounces) {
    if(samples<1 || samples>2 || bounces<1 || bounces>2)
        throw FormatError("Path Tracing sample and bounce budgets must be 1 or 2");
    if(impl_->samples!=samples || impl_->bounces!=bounces)impl_->history_reset_pending=true;
    impl_->samples=samples;impl_->bounces=bounces;
}
VkPipeline Render::pipeline()const noexcept{return impl_->graphics_pipeline;}
VkPipelineLayout Render::layout()const noexcept{return impl_->pipeline_layout;}
std::size_t Render::allocated_bytes()const noexcept {
    std::size_t total=std::size_t(impl_->visibility_allocation_bytes+impl_->environment_allocation_bytes);
    for(const auto& buffer:impl_->buffers)total+=std::size_t(buffer.allocation_bytes);
    return total;
}
void Render::record_temporal_stats(VkCommandBuffer command)const {
    const auto& p=*impl_;if(!p.buffers[6].buffer || !p.visibility_extent.width)return;
    VkBufferMemoryBarrier barrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
    barrier.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_TRANSFER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
    barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
    barrier.buffer=p.buffers[6].buffer;barrier.offset=p.lighting_offset()+p.lighting_bytes();
    barrier.size=p.visibility_bytes()-barrier.offset;
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,
        0,0,nullptr,1,&barrier,0,nullptr);
}
Render::TemporalStats Render::temporal_stats()const {
    const auto& p=*impl_;TemporalStats result{};result.frame_serial=p.frame_serial;
    const auto& buffer=p.buffers[6];
    if(!buffer.mapped || !p.visibility_extent.width || !p.visibility_extent.height || p.history_reset_pending)return result;
    if(!(buffer.memory_flags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
        throw FormatError("Path Tracing temporal diagnostics require coherent mapped history");
    const auto cells=p.lighting_cells(p.visibility_extent,p.lighting_scale);
    const auto samples=std::uint32_t(std::min<VkDeviceSize>(64,cells));
    const auto materials=p.buffers[2].populated/sizeof(GpuMaterial);
    for(unsigned bank=0;bank<result.banks.size();++bank) {
        auto& stats=result.banks[bank];stats.sampled_cells=samples;std::uint32_t sum{};
        const auto base=p.lighting_offset()+p.lighting_bytes()+bank*cells*16*sizeof(std::uint32_t);
        for(std::uint32_t sample=0;sample<samples;++sample) {
            const auto at=cells*sample/samples;
            std::array<std::uint32_t,16> record;
            std::memcpy(record.data(),static_cast<const std::byte*>(buffer.mapped)+base+at*sizeof(record),sizeof(record));
            if(record[7]==0xffffffffu || record[13]!=p.frame_serial)continue;
            if(record[7]>=materials || record[12]<1 || record[12]>16)
                throw FormatError("Path Tracing temporal diagnostics found an invalid completed history record");
            ++stats.sampled_valid_count;stats.maximum_count=std::max(stats.maximum_count,record[12]);sum+=record[12];
        }
        if(stats.sampled_valid_count)stats.mean_count=double(sum)/stats.sampled_valid_count;
    }
    return result;
}
void Render::prepare_environment(VkFormat color,VkFormat depth,unsigned side,bool source_available) {
    auto& p=*impl_;const bool previous=p.environment_nee_available();
    p.prepare_environment(color,depth,side);p.environment_source_available=source_available;
    if(previous!=p.environment_nee_available())p.history_reset_pending=true;
    // Production prepares the cube after upload. Keep the frame header final
    // before recording any world/foreground/environment command buffers.
    if(p.uploaded)p.update_visibility_binding();
}
void Render::begin_environment(VkCommandBuffer command,unsigned face,std::array<float,4> color) {
    auto& p=*impl_;
    if(!p.environment_side || p.environment_recording || face>=6)
        throw FormatError("Path Tracing environment needs a prepared face target");
    std::array<VkClearValue,2> clear{};std::copy(color.begin(),color.end(),clear[0].color.float32);
    clear[1].depthStencil={1,0};
    VkRenderPassBeginInfo begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};begin.renderPass=p.environment_pass;
    begin.framebuffer=p.environment_framebuffers[face];begin.renderArea={{0,0},{p.environment_side,p.environment_side}};
    begin.clearValueCount=clear.size();begin.pClearValues=clear.data();
    vkCmdBeginRenderPass(command,&begin,VK_SUBPASS_CONTENTS_INLINE);p.environment_recording=true;
}
void Render::end_environment(VkCommandBuffer command) {
    if(!impl_->environment_recording)throw FormatError("Path Tracing environment has no recording phase");
    vkCmdEndRenderPass(command);impl_->environment_recording=false;
}
void Render::upload(const Scene& scene) {
    upload(scene,scene.uniform());
}
void Render::upload(const Scene& scene,const GpuSceneUniform& uniform) {
    auto& p=*impl_;
    if(p.visibility_recording || p.lighting_recording)throw FormatError("Path Tracing scene cannot upload during screen-pass recording");
    p.previous_history_camera=p.current_history_camera;
    p.previous_history_eye=p.current_history_eye;
    p.previous_history_camera_mask=p.current_history_camera_mask;
    p.current_history_camera_mask=0;p.previous_history_prepared=false;
    p.visibility_ready=p.lighting_ready=false;
    const std::array<std::span<const std::byte>,7> input{
        bytes(scene.nodes),bytes(scene.triangles),bytes(scene.materials),bytes(scene.lights),bytes(scene.pixels),
        std::as_bytes(std::span(&uniform,1)),bytes(scene.color_guide_pixels)};
    // Validate the complete upload before replacing any resource. Vulkan never
    // receives an empty descriptor, including a scene with no lights or rays.
    for(unsigned i=0;i<input.size();++i) {
        const auto limit=i==5?p.properties.limits.maxUniformBufferRange:p.properties.limits.maxStorageBufferRange;
        if(std::max<std::size_t>(16,input[i].size())>limit)
            throw FormatError("Path Tracing scene bank "+std::to_string(i)+" exceeds maxStorageBufferRange or maxUniformBufferRange");
    }
    p.uploaded=false;
    std::array<VkDescriptorBufferInfo,7> infos{};
    std::array<VkWriteDescriptorSet,7> writes{};
    for(unsigned i=0;i<input.size();++i) {
        const auto usage=i==5?VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT:VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        const auto limit=i==5?p.properties.limits.maxUniformBufferRange:p.properties.limits.maxStorageBufferRange;
        auto& buffer=p.buffers[i==6?7:i];
        p.allocate(buffer,std::max<std::size_t>(16,input[i].size()),usage,limit);
        // Only cached host memory is suitable for a CPU read/compare. Reading
        // an uncached mapped VRAM bank back over PCIe can cost more than
        // uploading the CPU-owned scene again, so use write-only copies there.
        const bool immutable_guide=i==6 && !scene.color_guide_image_keys.empty() &&
            p.uploaded_color_guide_source==&scene && p.uploaded_color_guide_keys==scene.color_guide_image_keys &&
            buffer.populated==input[i].size();
        if(immutable_guide) {
            // The source fingerprint proves this atlas is unchanged. Do not
            // read uncached VRAM or resend megabytes of immutable map data.
        } else if(input[i].empty()) {
            std::memset(buffer.mapped,0,16);
            buffer.populated=16;
        } else {
            if(!(buffer.memory_flags&VK_MEMORY_PROPERTY_HOST_CACHED_BIT) ||
               buffer.populated!=input[i].size() || std::memcmp(buffer.mapped,input[i].data(),input[i].size())!=0)
                std::memcpy(buffer.mapped,input[i].data(),input[i].size());
            buffer.populated=input[i].size();
        }
        infos[i]={buffer.buffer,0,std::max<std::size_t>(16,input[i].size())};
        auto& write=writes[i];write.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet=p.descriptor;write.dstBinding=i==6?8:i;write.descriptorCount=1;
        write.descriptorType=i==5?VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.pBufferInfo=&infos[i];
    }
    vkUpdateDescriptorSets(p.context.device,writes.size(),writes.data(),0,nullptr);
    p.uploaded_color_guide_source=&scene;p.uploaded_color_guide_keys=scene.color_guide_image_keys;
    p.environment_fallback_available=uniform.environment[0]>0 || uniform.environment[1]>0 || uniform.environment[2]>0;
    const auto key=temporal_lighting_key(scene,uniform);
    if(p.uploaded_source!=&scene || !p.lighting_key_valid || p.lighting_key!=key)p.history_reset_pending=true;
    p.uploaded_source=&scene;p.lighting_key=key;p.lighting_key_valid=true;
    if(++p.frame_serial==0)p.history_reset_pending=true;
    p.update_visibility_binding();p.uploaded=true;
}
void Render::prepare_visibility(VkExtent2D extent) {
    prepare_visibility(extent,VkViewport{0,0,float(extent.width),float(extent.height),0,1});
}
void Render::prepare_visibility(VkExtent2D extent,const VkViewport& viewport) {
    const std::array values{viewport.x,viewport.y,viewport.width,viewport.height,viewport.minDepth,viewport.maxDepth};
    if(!std::all_of(values.begin(),values.end(),[](float value){return std::isfinite(value);}) ||
       viewport.width<=0 || viewport.height==0)
        throw FormatError("Path Tracing history requires its actual finite nonempty viewport");
    impl_->prepare_visibility(extent);impl_->visibility_viewport=viewport;
}
void Render::disable_visibility()noexcept {impl_->visibility_ready=impl_->lighting_ready=false;}
void Render::begin_visibility(VkCommandBuffer command) {
    auto& p=*impl_;
    if(!p.uploaded || !p.visibility_framebuffer || p.visibility_recording || p.lighting_recording)
        throw FormatError("Path Tracing visibility needs an uploaded scene and prepared target");
    p.snapshot_previous_history(command);
    p.visibility_ready=p.lighting_ready=false;
    VkClearValue clear{};clear.depthStencil={1.f,0};
    VkRenderPassBeginInfo begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    begin.renderPass=p.visibility_pass;begin.framebuffer=p.visibility_framebuffer;
    begin.renderArea={{0,0},p.visibility_extent};begin.clearValueCount=1;begin.pClearValues=&clear;
    vkCmdBeginRenderPass(command,&begin,VK_SUBPASS_CONTENTS_INLINE);p.visibility_recording=true;
}
void Render::bind_visibility(VkCommandBuffer command,VkDescriptorSet material,std::uint32_t dynamic_bank_offset,
                            std::span<const std::byte> camera,std::uint32_t material_index)const {
    if(material_index>=0x80000000u)throw FormatError("Path Tracing visibility lost its material identity");
    impl_->bind(command,material,dynamic_bank_offset,camera,material_index,true);
}
void Render::end_visibility(VkCommandBuffer command) {
    auto& p=*impl_;
    if(!p.visibility_recording)throw FormatError("Path Tracing visibility has no recording phase");
    vkCmdEndRenderPass(command);p.visibility_recording=false;
    VkBufferMemoryBarrier barrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
    barrier.srcAccessMask=VK_ACCESS_SHADER_READ_BIT;barrier.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
    barrier.buffer=p.buffers[6].buffer;barrier.offset=32;barrier.size=p.lighting_offset()-32;
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0,0,nullptr,1,&barrier,0,nullptr);
    VkBufferImageCopy copy{};copy.bufferOffset=32;
    copy.imageSubresource={VK_IMAGE_ASPECT_DEPTH_BIT,0,0,1};
    copy.imageExtent={p.visibility_extent.width,p.visibility_extent.height,1};
    vkCmdCopyImageToBuffer(command,p.visibility_depth,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,p.buffers[6].buffer,1,&copy);
    barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0,0,nullptr,1,&barrier,0,nullptr);
    p.visibility_ready=true;
}
void Render::begin_lighting(VkCommandBuffer command) {
    auto& p=*impl_;
    if(!p.uploaded || !p.visibility_ready || !p.lighting_framebuffer || p.visibility_recording || p.lighting_recording)
        throw FormatError("Path Tracing HDR lighting requires a completed visibility pass");
    p.lighting_ready=false;
    // The material word is also the atomic ownership token. Clear every
    // record to invalid before a new camera phase reuses this view's storage.
    VkBufferMemoryBarrier barrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
    barrier.srcAccessMask=VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
    barrier.buffer=p.buffers[6].buffer;barrier.offset=p.lighting_offset();
    barrier.size=p.visibility_bytes()-barrier.offset;
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0,0,nullptr,1,&barrier,0,nullptr);
    vkCmdFillBuffer(command,barrier.buffer,barrier.offset,
                    p.history_reset_pending?barrier.size:p.lighting_bytes(),0xffffffffu);
    p.history_reset_pending=false;
    barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT;
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0,0,nullptr,1,&barrier,0,nullptr);
    VkRenderPassBeginInfo begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    begin.renderPass=p.lighting_pass;begin.framebuffer=p.lighting_framebuffer;
    begin.renderArea={{0,0},p.visibility_extent};
    vkCmdBeginRenderPass(command,&begin,VK_SUBPASS_CONTENTS_INLINE);p.lighting_recording=true;
}
void Render::bind_lighting(VkCommandBuffer command,VkDescriptorSet material,std::uint32_t dynamic_bank_offset,
                          std::span<const std::byte> camera,std::uint32_t material_index)const {
    if(material_index>=0x80000000u)throw FormatError("Path Tracing HDR lighting lost its material identity");
    impl_->bind(command,material,dynamic_bank_offset,camera,material_index,false,true);
}
void Render::end_lighting(VkCommandBuffer command) {
    auto& p=*impl_;
    if(!p.lighting_recording)throw FormatError("Path Tracing HDR lighting has no recording phase");
    vkCmdEndRenderPass(command);p.lighting_recording=false;
    VkBufferMemoryBarrier barrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
    barrier.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
    barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
    barrier.buffer=p.buffers[6].buffer;barrier.offset=p.lighting_offset();
    barrier.size=p.visibility_bytes()-barrier.offset;
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0,0,nullptr,1,&barrier,0,nullptr);
    p.lighting_ready=true;
}
void Render::bind(VkCommandBuffer command,VkDescriptorSet material,std::uint32_t dynamic_bank_offset,
                  std::span<const std::byte> camera)const {
    impl_->bind(command,material,dynamic_bank_offset,camera,~0u,false);
}
void Render::bind(VkCommandBuffer command,VkDescriptorSet material,std::uint32_t dynamic_bank_offset,
                  std::span<const std::byte> camera,std::uint32_t material_index)const {
    if(camera.size()!=camera_bytes || material_index>=0x80000000u)
        throw FormatError("Path Tracing draw lost its camera or material identity");
    // The RT camera does not consume viewport.w for projection. Reserve its
    // bits for a per-draw material override, leaving the source camera bank
    // and the ordinary renderer's push constants unchanged.
    impl_->bind(command,material,dynamic_bank_offset,camera,material_index,false);
}
} // namespace halo2::path_tracing
