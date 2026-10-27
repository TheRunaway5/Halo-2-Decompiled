#include "halo2/bsp_postprocess.hpp"
#include "halo2/math.hpp"
#include "halo2/source_bloom_sampling.hpp"
#include "halo2/native_fragment_kernel.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <map>
#include <stdexcept>
namespace halo2 {
namespace {
void check(VkResult result,const char *operation) {
    if(result!=VK_SUCCESS)throw std::runtime_error(std::string(operation)+": Vulkan "+std::to_string(result));
}
struct Image {VkImage image{};VkDeviceMemory memory{};VkImageView view{};};
struct Uniform {VkBuffer buffer{};VkDeviceMemory memory{};};
struct PatchyUniform {std::array<std::uint32_t,60> program;std::array<ShaderVector,8> rows;ShaderVector controls,lookup;};
struct BloomUniform {std::array<std::uint32_t,60> program;ShaderVector controls,rectangle;};
static_assert(sizeof(PatchyUniform)==400 && sizeof(BloomUniform)==272);
}
struct BspPostprocess::Impl {
    BspPostprocessContext context;
    const BspAtmosphere &bank;
    bool cinematic{};
    CinematicBloomOverride cinematic_override;
    VkRenderPass target_pass{};
    VkDescriptorSetLayout patchy_set_layout{},bloom_set_layout{};
    VkPipelineLayout patchy_layout{},bloom_layout{};
    std::map<std::pair<std::string,unsigned>,VkPipeline> native_pipelines;
    VkDescriptorPool pool{};
    VkSampler repeat_sampler{},clamp_sampler{},depth_sampler{};
    VkImageView gradient{};
    std::map<std::uint32_t,std::vector<VkImageView>> noise_frames;
    std::uint32_t random{};
    struct View {
        BspPostprocessView input;
        PatchyFogState state;
        BspPatchyFrame frame;
        BspBloomFrame bloom;
        bool patchy{},scene_initialized{};
        VkExtent2D size{},half{},quarter{};
        Image noise,scene;
        std::array<Image,2> blur;
        VkFramebuffer noise_framebuffer{};
        std::array<VkFramebuffer,2> blur_framebuffers{};
        std::array<Uniform,8> uniforms;
        std::array<VkDescriptorSet,8> descriptors{};
        std::array<VkPipeline,8> pipelines{};
    };
    std::array<View,4> views;
    std::size_t view_count{};
    Impl(BspPostprocessContext c,const BspAtmosphere &b,
        const std::function<VkImageView(const Texture &)> &upload):context(std::move(c)),bank(b),random(context.random_seed) {
        gradient=upload(bank.gradient);
        for(const auto &definition:bank.patchy_definitions) {
            auto &frames=noise_frames[definition.datum];
            for(const auto &texture:definition.fog.noise)frames.push_back(upload(texture));
        }
        make_sampler(VK_SAMPLER_ADDRESS_MODE_REPEAT,VK_FILTER_LINEAR,repeat_sampler);
        make_sampler(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,VK_FILTER_LINEAR,clamp_sampler);
        make_sampler(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,VK_FILTER_NEAREST,depth_sampler);
        std::array<VkDescriptorSetLayoutBinding,4> patchy_bindings{};
        for(unsigned i=0;i<4;++i)patchy_bindings[i]={i,i==3?VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,i==2?4u:1u,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr};
        make_layout(patchy_bindings,patchy_set_layout,patchy_layout);
        const std::array<VkDescriptorSetLayoutBinding,2> bloom_bindings{{
            {0,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
            {1,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr}}};
        make_layout(bloom_bindings,bloom_set_layout,bloom_layout);
        const VkDescriptorPoolSize sizes[]{{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,72},{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,32}};
        VkDescriptorPoolCreateInfo pi{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        pi.maxSets=32;pi.poolSizeCount=2;pi.pPoolSizes=sizes;
        check(vkCreateDescriptorPool(context.device,&pi,nullptr,&pool),"BSP postprocess descriptor pool");
        make_target_pass();
        for(unsigned index=0;index<views.size();++index) {
            auto &view=views[index];
            std::array<VkDescriptorSetLayout,8> layouts;layouts.fill(bloom_set_layout);
            layouts[0]=layouts[1]=patchy_set_layout;
            VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
            allocate.descriptorPool=pool;allocate.descriptorSetCount=8;allocate.pSetLayouts=layouts.data();
            check(vkAllocateDescriptorSets(context.device,&allocate,view.descriptors.data()),"BSP postprocess descriptor sets");
            for(unsigned i=0;i<8;++i) {
                make_uniform(i<2?400:272,view.uniforms[i]);
                VkDescriptorBufferInfo info{view.uniforms[i].buffer,0,i<2?400u:272u};
                VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
                write.dstSet=view.descriptors[i];write.dstBinding=i<2?3:1;write.descriptorCount=1;
                write.descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;write.pBufferInfo=&info;
                vkUpdateDescriptorSets(context.device,1,&write,0,nullptr);
            }
        }
    }
    ~Impl() {
        for(auto &view:views) {
            release_targets(view);
            for(auto &uniform:view.uniforms) {
                if(uniform.buffer)vkDestroyBuffer(context.device,uniform.buffer,nullptr);
                if(uniform.memory)vkFreeMemory(context.device,uniform.memory,nullptr);
            }
        }
        for(const auto& [key,pipeline]:native_pipelines)if(pipeline)vkDestroyPipeline(context.device,pipeline,nullptr);
        if(pool)vkDestroyDescriptorPool(context.device,pool,nullptr);
        if(patchy_layout)vkDestroyPipelineLayout(context.device,patchy_layout,nullptr);
        if(bloom_layout)vkDestroyPipelineLayout(context.device,bloom_layout,nullptr);
        if(patchy_set_layout)vkDestroyDescriptorSetLayout(context.device,patchy_set_layout,nullptr);
        if(bloom_set_layout)vkDestroyDescriptorSetLayout(context.device,bloom_set_layout,nullptr);
        if(target_pass)vkDestroyRenderPass(context.device,target_pass,nullptr);
        for(auto sampler:{repeat_sampler,clamp_sampler,depth_sampler})if(sampler)vkDestroySampler(context.device,sampler,nullptr);
    }
    unsigned memory_type(unsigned bits,VkMemoryPropertyFlags flags) {
        VkPhysicalDeviceMemoryProperties properties;vkGetPhysicalDeviceMemoryProperties(context.physical,&properties);
        for(unsigned i=0;i<properties.memoryTypeCount;++i)
            if((bits&(1u<<i)) && (properties.memoryTypes[i].propertyFlags&flags)==flags)return i;
        throw FormatError("BSP postprocess memory type unavailable");
    }
    void make_sampler(VkSamplerAddressMode address,VkFilter filter,VkSampler &sampler) {
        VkSamplerCreateInfo info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        info.magFilter=info.minFilter=filter;info.mipmapMode=VK_SAMPLER_MIPMAP_MODE_LINEAR;
        info.addressModeU=info.addressModeV=info.addressModeW=address;info.maxLod=VK_LOD_CLAMP_NONE;
        check(vkCreateSampler(context.device,&info,nullptr,&sampler),"BSP postprocess sampler");
    }
    void make_layout(std::span<const VkDescriptorSetLayoutBinding> bindings,
        VkDescriptorSetLayout &set,VkPipelineLayout &pipeline) {
        VkDescriptorSetLayoutCreateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        info.bindingCount=unsigned(bindings.size());info.pBindings=bindings.data();
        check(vkCreateDescriptorSetLayout(context.device,&info,nullptr,&set),"BSP postprocess descriptor layout");
        VkPipelineLayoutCreateInfo pi{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};pi.setLayoutCount=1;pi.pSetLayouts=&set;
        check(vkCreatePipelineLayout(context.device,&pi,nullptr,&pipeline),"BSP postprocess pipeline layout");
    }
    void make_uniform(unsigned size,Uniform &uniform) {
        VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};info.size=size;info.usage=VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        check(vkCreateBuffer(context.device,&info,nullptr,&uniform.buffer),"BSP postprocess uniform buffer");
        VkMemoryRequirements requirements;vkGetBufferMemoryRequirements(context.device,uniform.buffer,&requirements);
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocation.allocationSize=requirements.size;
        allocation.memoryTypeIndex=memory_type(requirements.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        check(vkAllocateMemory(context.device,&allocation,nullptr,&uniform.memory),"BSP postprocess uniform memory");
        check(vkBindBufferMemory(context.device,uniform.buffer,uniform.memory,0),"BSP postprocess uniform binding");
    }
    template<class T>void write_uniform(const Uniform &uniform,const T &value) {
        void *mapped{};check(vkMapMemory(context.device,uniform.memory,0,sizeof(value),0,&mapped),"BSP postprocess uniform map");
        std::memcpy(mapped,&value,sizeof(value));vkUnmapMemory(context.device,uniform.memory);
    }
    void make_target_pass() {
        VkAttachmentDescription attachment{};attachment.format=VK_FORMAT_R8G8B8A8_UNORM;
        attachment.samples=VK_SAMPLE_COUNT_1_BIT;attachment.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;attachment.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;attachment.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;attachment.finalLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        VkAttachmentReference color{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        VkSubpassDescription subpass{};subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount=1;subpass.pColorAttachments=&color;
        std::array<VkSubpassDependency,2> dependencies{};
        dependencies[0]={VK_SUBPASS_EXTERNAL,0,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_ACCESS_SHADER_READ_BIT,VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_DEPENDENCY_BY_REGION_BIT};
        dependencies[1]={0,VK_SUBPASS_EXTERNAL,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_ACCESS_SHADER_READ_BIT,VK_DEPENDENCY_BY_REGION_BIT};
        VkRenderPassCreateInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};pass.attachmentCount=1;pass.pAttachments=&attachment;
        pass.subpassCount=1;pass.pSubpasses=&subpass;pass.dependencyCount=2;pass.pDependencies=dependencies.data();
        check(vkCreateRenderPass(context.device,&pass,nullptr,&target_pass),"BSP postprocess target pass");
    }
    VkShaderModule shader(const char *name) {
        const auto path=context.shader_directory/name;std::ifstream file(path,std::ios::binary|std::ios::ate);
        if(!file)throw FormatError("missing BSP postprocess shader "+path.string());
        const auto bytes=file.tellg();if(bytes<=0 || bytes%4)throw FormatError("invalid BSP postprocess shader");
        std::vector<std::uint32_t> words(std::size_t(bytes)/4);file.seekg(0);
        if(!file.read(reinterpret_cast<char*>(words.data()),bytes))throw FormatError("short BSP postprocess shader");
        VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};info.codeSize=std::size_t(bytes);info.pCode=words.data();
        VkShaderModule module{};check(vkCreateShaderModule(context.device,&info,nullptr,&module),"BSP postprocess shader module");return module;
    }
    VkPipeline make_pipeline(const char *fragment,VkPipelineLayout layout,VkRenderPass pass,unsigned blending) {
        const auto vertex=shader("bloom.vert.spv"),pixel=shader(fragment);
        const std::array<VkPipelineShaderStageCreateInfo,2> stages{{
            {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_VERTEX_BIT,vertex,"main",nullptr},
            {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_FRAGMENT_BIT,pixel,"main",nullptr}}};
        VkPipelineVertexInputStateCreateInfo input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};assembly.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};viewport.viewportCount=viewport.scissorCount=1;
        VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};raster.polygonMode=VK_POLYGON_MODE_FILL;raster.lineWidth=1;
        VkPipelineMultisampleStateCreateInfo samples{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};samples.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
        VkPipelineDepthStencilStateCreateInfo depth{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
        VkPipelineColorBlendAttachmentState attachment{};attachment.colorWriteMask=0xf;attachment.blendEnable=blending!=0;
        attachment.srcColorBlendFactor=blending==2?VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR:VK_BLEND_FACTOR_ONE;
        attachment.dstColorBlendFactor=blending==2?VK_BLEND_FACTOR_ONE:VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        attachment.srcAlphaBlendFactor=blending==2?VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA:VK_BLEND_FACTOR_ZERO;
        attachment.dstAlphaBlendFactor=blending==2?VK_BLEND_FACTOR_ONE:VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        attachment.colorBlendOp=attachment.alphaBlendOp=VK_BLEND_OP_ADD;
        VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};blend.attachmentCount=1;blend.pAttachments=&attachment;
        const VkDynamicState dynamic_states[]{VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};dynamic.dynamicStateCount=2;dynamic.pDynamicStates=dynamic_states;
        VkGraphicsPipelineCreateInfo info{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};info.stageCount=2;info.pStages=stages.data();
        info.pVertexInputState=&input;info.pInputAssemblyState=&assembly;info.pViewportState=&viewport;info.pRasterizationState=&raster;
        info.pMultisampleState=&samples;info.pDepthStencilState=&depth;info.pColorBlendState=&blend;info.pDynamicState=&dynamic;info.layout=layout;info.renderPass=pass;
        VkPipeline pipeline{};const auto result=vkCreateGraphicsPipelines(context.device,VK_NULL_HANDLE,1,&info,nullptr,&pipeline);
        vkDestroyShaderModule(context.device,vertex,nullptr);vkDestroyShaderModule(context.device,pixel,nullptr);
        check(result,"BSP postprocess graphics pipeline");return pipeline;
    }
    VkPipeline native_pipeline(std::string_view family,const PixelProgram& program,
        VkPipelineLayout layout,VkRenderPass pass,unsigned blending) {
        const std::string path(native_fragment_kernel_path(family,program.words));
        const auto key=std::pair(path,blending);
        if(auto found=native_pipelines.find(key);found!=native_pipelines.end())return found->second;
        const auto pipeline=make_pipeline(path.c_str(),layout,pass,blending);
        native_pipelines.emplace(key,pipeline);return pipeline;
    }
    void make_image(Image &target,VkExtent2D size,VkFormat format,VkImageUsageFlags usage) {
        VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};info.imageType=VK_IMAGE_TYPE_2D;info.format=format;info.extent={size.width,size.height,1};
        info.mipLevels=info.arrayLayers=1;info.samples=VK_SAMPLE_COUNT_1_BIT;info.tiling=VK_IMAGE_TILING_OPTIMAL;info.usage=usage|VK_IMAGE_USAGE_SAMPLED_BIT;
        check(vkCreateImage(context.device,&info,nullptr,&target.image),"BSP postprocess image");
        VkMemoryRequirements requirements;vkGetImageMemoryRequirements(context.device,target.image,&requirements);
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocation.allocationSize=requirements.size;
        allocation.memoryTypeIndex=memory_type(requirements.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        check(vkAllocateMemory(context.device,&allocation,nullptr,&target.memory),"BSP postprocess image memory");
        check(vkBindImageMemory(context.device,target.image,target.memory,0),"BSP postprocess image binding");
        VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};view.image=target.image;view.viewType=VK_IMAGE_VIEW_TYPE_2D;view.format=format;
        view.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};check(vkCreateImageView(context.device,&view,nullptr,&target.view),"BSP postprocess image view");
    }
    void release_image(Image &image) {
        if(image.view)vkDestroyImageView(context.device,image.view,nullptr);
        if(image.image)vkDestroyImage(context.device,image.image,nullptr);
        if(image.memory)vkFreeMemory(context.device,image.memory,nullptr);
        image={};
    }
    void release_targets(View &view) {
        if(view.noise_framebuffer)vkDestroyFramebuffer(context.device,view.noise_framebuffer,nullptr);
        for(auto fb:view.blur_framebuffers)if(fb)vkDestroyFramebuffer(context.device,fb,nullptr);
        release_image(view.noise);release_image(view.scene);for(auto &image:view.blur)release_image(image);
        view.noise_framebuffer={};view.blur_framebuffers={};view.scene_initialized=false;
    }
    void image_binding(VkDescriptorSet descriptor,unsigned binding,std::span<const VkImageView> images,VkSampler sampler) {
        std::vector<VkDescriptorImageInfo> information;
        for(auto view:images)information.push_back({sampler,view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};write.dstSet=descriptor;write.dstBinding=binding;
        write.descriptorCount=unsigned(information.size());write.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;write.pImageInfo=information.data();
        vkUpdateDescriptorSets(context.device,1,&write,0,nullptr);
    }
    void make_targets(View &view,VkExtent2D size) {
        release_targets(view);view.size=size;view.half={std::max(1u,(size.width+1)/2),std::max(1u,(size.height+1)/2)};
        view.quarter={std::max(1u,(size.width+3)/4),std::max(1u,(size.height+3)/4)};
        make_image(view.noise,view.half,VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
        make_image(view.scene,size,context.color_format,VK_IMAGE_USAGE_TRANSFER_DST_BIT);
        for(auto &image:view.blur)make_image(image,view.quarter,VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
        const auto framebuffer=[&](VkImageView image,VkExtent2D extent,VkFramebuffer &fb) {
            VkFramebufferCreateInfo info{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};info.renderPass=target_pass;info.attachmentCount=1;info.pAttachments=&image;
            info.width=extent.width;info.height=extent.height;info.layers=1;check(vkCreateFramebuffer(context.device,&info,nullptr,&fb),"BSP postprocess framebuffer");
        };
        framebuffer(view.noise.view,view.half,view.noise_framebuffer);
        for(unsigned i=0;i<2;++i)framebuffer(view.blur[i].view,view.quarter,view.blur_framebuffers[i]);
        for(unsigned i=0;i<2;++i) {
            image_binding(view.descriptors[i],0,{&context.depth_view,1},depth_sampler);
            image_binding(view.descriptors[i],1,{&gradient,1},clamp_sampler);
        }
        const std::array<VkImageView,4> target{view.noise.view,view.noise.view,view.noise.view,view.noise.view};
        image_binding(view.descriptors[1],2,target,clamp_sampler);
        for(unsigned i=0;i<6;++i) {
            const auto image=i?view.blur[(i-1)%2].view:view.scene.view;
            image_binding(view.descriptors[i+2],0,{&image,1},clamp_sampler);
        }
    }
    void prepare(std::span<const BspPostprocessView> inputs,float time) {
        if(inputs.size()>views.size())throw FormatError("too many BSP postprocess views");
        view_count=inputs.size();
        for(unsigned i=0;i<inputs.size();++i) {
            auto &view=views[i];view.input=inputs[i];const auto &rectangle=view.input.rectangle;
            if(!rectangle.extent.width || !rectangle.extent.height || rectangle.offset.x<0 || rectangle.offset.y<0 ||
                std::uint64_t(rectangle.offset.x)+rectangle.extent.width>context.extent.width || std::uint64_t(rectangle.offset.y)+rectangle.extent.height>context.extent.height)
                throw FormatError("BSP postprocess rectangle outside surface");
            if(view.size.width!=rectangle.extent.width || view.size.height!=rectangle.extent.height)make_targets(view,rectangle.extent);
            const auto &sample=view.input.atmosphere;view.patchy=bool(sample.patchy);
            if(view.patchy) {
                const auto definition=bank.patchy_definition(sample.patchy_datum);
                if(!definition)throw FormatError("missing BSP patchy definition");
                const auto &camera=view.input.camera;
                const auto f=camera.oriented?camera.forward:direction(camera.yaw,camera.pitch);
                const auto up=camera.oriented?camera.up:Vec3{0,0,1};const auto right=normalized(cross(f,up));
                const Transform to_world{1,{right,normalized(cross(right,f)),normalized(f)*-1.f},camera.position};
                const float aspect=float(rectangle.extent.width)/float(rectangle.extent.height);
                view.frame=bsp_patchy_fog_frame(*definition,*sample.patchy,view.state,random,
                    inverse_transform(to_world),f.z,camera_vertical_field_of_view(camera,aspect),
                    float(rectangle.extent.height)/float(rectangle.extent.width),time,
                    view.input.clip_planes.y,view.input.clip_planes.x);
                std::array<VkImageView,4> selected;const auto &frames=noise_frames.at(sample.patchy_datum);
                for(unsigned layer=0;layer<4;++layer)selected[layer]=frames.at(view.frame.bitmaps[layer]);
                image_binding(view.descriptors[0],2,selected,repeat_sampler);
                const float inverse=1.f/std::max(.0001f,sample.patchy->distance[1]-sample.patchy->distance[0]);
                for(unsigned pass=0;pass<2;++pass) {
                    const auto& program=pass?view.frame.shading.direct:view.frame.shading.noise;
                    view.pipelines[pass]=native_pipeline(pass?"bsp-patchy-composite":"bsp-patchy-noise",program,
                        patchy_layout,pass?context.color_pass:target_pass,pass?1:0);
                    const PatchyUniform uniform{native_fragment_uniform_words(program.words),view.frame.shading.rows,
                        {view.frame.shading.vertical,float(pass?view.size.width:view.half.width),float(pass?view.size.height:view.half.height),
                            pass?view.frame.composite_minimum_depth:0.f},
                        {1024.f*inverse,0,0.f-sample.patchy->distance[0]*inverse,0}};
                    write_uniform(view.uniforms[pass],uniform);
                }
            }
            view.bloom=bsp_bloom_frame(sample,.5f,.5f,source_bloom_screen_threshold_factor(
                view.input.bloom_screen_context?&*view.input.bloom_screen_context:nullptr));
            const auto parameters=cinematic_bloom_parameters(view.bloom.threshold,view.bloom.strength,
                cinematic,cinematic_override);
            view.bloom.threshold=parameters.threshold;view.bloom.strength=parameters.strength;
            view.bloom.programs=menu_bloom(parameters.threshold,parameters.strength);
            for(unsigned pass=0;pass<6;++pass) {
                const auto physical=pass==5?view.size:view.quarter;
                const auto controls=source_bloom_filter_controls(pass<5?view.bloom.programs.radii[pass]:0,
                    {float(physical.width),float(physical.height)},
                    pass<5?view.input.source_bloom_filter_extent:std::nullopt);
                const auto& program=pass==0?view.bloom.programs.threshold:pass==5?view.bloom.programs.composite:view.bloom.programs.blur;
                view.pipelines[pass+2]=native_pipeline("bloom",program,bloom_layout,
                    pass==5?context.color_pass:target_pass,pass==5?2:0);
                const BloomUniform uniform{native_fragment_uniform_words(program.words),
                    controls,{0,0,1,1}};
                write_uniform(view.uniforms[pass+2],uniform);
            }
        }
    }
    void viewport(VkCommandBuffer command,VkRect2D rect) {
        const VkViewport viewport{float(rect.offset.x),float(rect.offset.y),float(rect.extent.width),float(rect.extent.height),0,1};
        vkCmdSetViewport(command,0,1,&viewport);vkCmdSetScissor(command,0,1,&rect);
    }
    void pass(VkCommandBuffer command,VkRenderPass render_pass,VkFramebuffer framebuffer,VkExtent2D extent,
        VkRect2D rectangle,VkPipeline pipeline,VkPipelineLayout layout,VkDescriptorSet descriptor,bool clear) {
        VkClearValue zero{};VkRenderPassBeginInfo begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};begin.renderPass=render_pass;begin.framebuffer=framebuffer;
        begin.renderArea={{0,0},extent};begin.clearValueCount=clear?1:0;begin.pClearValues=clear?&zero:nullptr;
        vkCmdBeginRenderPass(command,&begin,VK_SUBPASS_CONTENTS_INLINE);viewport(command,rectangle);
        vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline);
        vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,layout,0,1,&descriptor,0,nullptr);
        vkCmdDraw(command,3,1,0,0);vkCmdEndRenderPass(command);
    }
    void record(unsigned index,const BspPostprocessTarget &target,bool patchy,bool bloom) {
        if(index>=view_count)throw FormatError("BSP postprocess view outside prepared bank");
        auto &view=views[index];
        patchy=patchy&&view.patchy;
        if(!patchy&&!bloom)return;
        const auto command=target.command;vkCmdEndRenderPass(command);
        VkImageMemoryBarrier depth{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};depth.image=context.depth;
        depth.srcQueueFamilyIndex=depth.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
        depth.subresourceRange={VK_IMAGE_ASPECT_DEPTH_BIT,0,1,0,1};
        if(context.depth_format==VK_FORMAT_D24_UNORM_S8_UINT || context.depth_format==VK_FORMAT_D32_SFLOAT_S8_UINT)depth.subresourceRange.aspectMask|=VK_IMAGE_ASPECT_STENCIL_BIT;
        if(patchy) {
            depth.srcAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;depth.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
            depth.oldLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;depth.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,0,nullptr,0,nullptr,1,&depth);
            pass(command,target_pass,view.noise_framebuffer,view.half,{{0,0},view.half},view.pipelines[0],patchy_layout,view.descriptors[0],true);
            pass(command,context.color_pass,target.color_framebuffer,context.extent,view.input.rectangle,view.pipelines[1],patchy_layout,view.descriptors[1],false);
            depth.srcAccessMask=VK_ACCESS_SHADER_READ_BIT;depth.dstAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            depth.oldLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;depth.newLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,0,0,nullptr,0,nullptr,1,&depth);
        }
        if(bloom) {
        std::array<VkImageMemoryBarrier,2> barriers{};
        for(auto &barrier:barriers) {barrier.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};}
        barriers[0].image=target.color;barriers[0].oldLayout=context.scene_layout;barriers[0].newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barriers[0].srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;barriers[0].dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
        barriers[1].image=view.scene.image;barriers[1].oldLayout=view.scene_initialized?VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:VK_IMAGE_LAYOUT_UNDEFINED;
        barriers[1].newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;barriers[1].srcAccessMask=view.scene_initialized?VK_ACCESS_SHADER_READ_BIT:0;barriers[1].dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,2,barriers.data());
        VkImageCopy copy{};copy.srcSubresource=copy.dstSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};
        copy.srcOffset={view.input.rectangle.offset.x,view.input.rectangle.offset.y,0};copy.extent={view.size.width,view.size.height,1};
        vkCmdCopyImage(command,target.color,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,view.scene.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy);
        barriers[0].oldLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;barriers[0].newLayout=context.scene_layout;
        barriers[0].srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;barriers[0].dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barriers[1].oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;barriers[1].newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barriers[1].srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;barriers[1].dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,0,nullptr,0,nullptr,2,barriers.data());
        view.scene_initialized=true;
        for(unsigned pass_index=0;pass_index<5;++pass_index)
            pass(command,target_pass,view.blur_framebuffers[pass_index%2],view.quarter,{{0,0},view.quarter},view.pipelines[pass_index+2],bloom_layout,view.descriptors[pass_index+2],true);
        pass(command,context.color_pass,target.color_framebuffer,context.extent,view.input.rectangle,view.pipelines[7],bloom_layout,view.descriptors[7],false);
        }
        VkRenderPassBeginInfo resume{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};resume.renderPass=context.continuation_pass;resume.framebuffer=target.continuation_framebuffer;
        resume.renderArea={{0,0},context.extent};vkCmdBeginRenderPass(command,&resume,VK_SUBPASS_CONTENTS_INLINE);viewport(command,view.input.rectangle);
    }
};
BspPostprocess::BspPostprocess(BspPostprocessContext context,const BspAtmosphere &bank,
    const std::function<VkImageView(const Texture &)> &upload):impl_(std::make_unique<Impl>(std::move(context),bank,upload)) {}
BspPostprocess::~BspPostprocess()=default;
void BspPostprocess::prepare(std::span<const BspPostprocessView> views,float time) {impl_->prepare(views,time);}
void BspPostprocess::cinematic_bloom(bool cinematic,const CinematicBloomOverride &override) {
    impl_->cinematic=cinematic;impl_->cinematic_override=override;
}
BspPostprocessState BspPostprocess::persistent_state() const {
    BspPostprocessState state;state.random=impl_->random;
    for(unsigned i=0;i<state.motion.size();++i)state.motion[i]=impl_->views[i].state;
    return state;
}
void BspPostprocess::restore_state(const BspPostprocessState &state) {
    impl_->random=state.random;
    for(unsigned i=0;i<state.motion.size();++i)impl_->views[i].state=state.motion[i];
}
void BspPostprocess::record_patchy(std::size_t view,const BspPostprocessTarget &target) {impl_->record(unsigned(view),target,true,false);}
void BspPostprocess::record_bloom(std::size_t view,const BspPostprocessTarget &target) {impl_->record(unsigned(view),target,false,true);}
void BspPostprocess::record(std::size_t view,const BspPostprocessTarget &target) {impl_->record(unsigned(view),target,true,true);}
} // namespace halo2
