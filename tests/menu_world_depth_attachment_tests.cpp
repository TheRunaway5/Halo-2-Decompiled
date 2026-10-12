#include <vulkan/vulkan.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// Isolated ordinary-world attachment proof. Retained GS/FS inputs are fixtures,
// never runtime constants. Canonical D32 prior depth is controlled test input,
// not proof of previous native draw history. Root owns compilation/execution.
namespace {
using Bytes=std::vector<std::uint8_t>;
namespace fs=std::filesystem;
void require(bool value,const std::string &message) {if(!value)throw std::runtime_error(message);}
void check(VkResult result) {require(result==VK_SUCCESS,"attachment Vulkan failure "+std::to_string(result));}
Bytes read(const fs::path &path,std::size_t expected=0) {
    std::ifstream file(path,std::ios::binary|std::ios::ate);
    require(bool(file),"missing attachment fixture "+path.string());
    const auto length=file.tellg();require(length>=0,"invalid attachment fixture size");
    Bytes data(static_cast<std::size_t>(length));
    require(!expected||data.size()==expected,"attachment fixture size differs: "+path.string());
    file.seekg(0);require(bool(file.read(reinterpret_cast<char*>(data.data()),data.size())),"short fixture");return data;
}
void save(const fs::path &path,const Bytes &data) {
    std::ofstream file(path,std::ios::binary);require(bool(file.write(reinterpret_cast<const char*>(data.data()),data.size())),"cannot save attachment result");
}
std::uint32_t word(const Bytes &data,std::size_t offset) {
    require(offset<=data.size()&&4<=data.size()-offset,"word outside attachment fixture");
    std::uint32_t value;std::memcpy(&value,data.data()+offset,4);return value;
}
std::size_t different(const Bytes &a,const Bytes &b) {
    require(a.size()==b.size(),"attachment comparison size differs");
    std::size_t count=0;for(std::size_t i=0;i<a.size();++i)count+=a[i]!=b[i];return count;
}
std::size_t depth_words(const Bytes &a,const Bytes &b,bool d24) {
    require(a.size()==b.size()&&a.size()%4==0,"depth comparison size differs");
    std::size_t count=0;for(std::size_t i=0;i<a.size();i+=4)
        count+=(word(a,i)&(d24?0xffffffu:0xffffffffu))!=(word(b,i)&(d24?0xffffffu:0xffffffffu));return count;
}
struct State {
    std::array<std::uint32_t,44> w{};
    float f(unsigned i)const{return std::bit_cast<float>(w.at(i));}
    VkViewport viewport()const{return {f(31),f(32),f(33),f(34),f(35),f(36)};}
    VkRect2D scissor()const{return {{std::bit_cast<std::int32_t>(w[37]),std::bit_cast<std::int32_t>(w[38])},{w[39],w[40]}};}
    VkStencilOpState stencil(unsigned first)const {
        return {VkStencilOp(w[first]),VkStencilOp(w[first+1]),VkStencilOp(w[first+2]),
            VkCompareOp(w[first+3]),w[first+4],w[first+5],w[first+6]};
    }
};
static_assert(sizeof(State)==44*4);
struct Fixture {
    unsigned event{};std::string role;State state;
    Bytes original,mask,records,uniform,before_depth,before_stencil,after_depth,after_stencil;
};
struct FullFogFixture {
    Bytes eye,hilo,gradient,depth,stencil,before,after,visibility;
    std::array<std::uint32_t,12> blend{};
    unsigned covered{};
};
FullFogFixture full_fog_fixture(const fs::path &directory) {
    require(word(read(directory/"color-authority.bin",4),0)==0x46474331,"full fog captured RGBA authority is unverified");
    std::ifstream file(directory/"manifest.tsv");std::string line;
    require(bool(std::getline(file,line))&&line=="H2_WORLD_DEPTH_FULL_FOG_V1","full fog manifest version differs");
    require(bool(std::getline(file,line)),"full fog fixture row missing");
    std::istringstream row(line);unsigned width{},height{};FullFogFixture f;std::array<std::string,9> paths;
    require(bool(row>>width>>height>>f.covered)&&width==640&&height==480&&f.covered==118320,"full fog dimensions/coverage differ");
    for(auto &name:paths){require(bool(row>>name),"full fog path missing");require(fs::path(name).is_relative()&&name.find("..") == std::string::npos,"full fog path escapes directory");}
    std::string extra;require(!(row>>extra)&&!std::getline(file,line),"unexpected full fog fixture data");
    constexpr auto pixels=640u*480u;
    f.eye=read(directory/paths[0],272);f.hilo=read(directory/paths[1],272);f.gradient=read(directory/paths[2],1024);
    f.depth=read(directory/paths[3],pixels*4);f.stencil=read(directory/paths[4],pixels);
    f.before=read(directory/paths[5],pixels*4);f.after=read(directory/paths[6],pixels*4);f.visibility=read(directory/paths[7],pixels);
    const auto blend=read(directory/paths[8],48);std::memcpy(f.blend.data(),blend.data(),48);
    require(f.blend==std::array<std::uint32_t,12>{0x46424731,VK_BLEND_FACTOR_CONSTANT_COLOR,VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,VK_BLEND_OP_ADD,
        VK_BLEND_FACTOR_CONSTANT_COLOR,VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,VK_BLEND_OP_ADD,15,0x3f800000,0x3f800000,0x3f800000,0},"captured fog blend state differs");
    for(unsigned i=0;i<68;++i){const auto expected=i==8?0x01200000u:i==9?0x1100u:i==43?0x41201b16u:i==53?0x11001u:0u;require(word(f.eye,i*4)==expected,"captured eye program/lookup differs");}
    require(word(f.hilo,54*4)==0x2621,"selected source HILO bank differs");
    for(unsigned i=0;i<pixels;++i){const auto n=word(f.depth,i*4);require(n>=97960&&n<=0xffffffu&&f.visibility[i]==(n==0xffffffu?255:0),"captured fog depth/mask differs");}
    require(std::count(f.visibility.begin(),f.visibility.end(),std::uint8_t(255))==f.covered,"captured eye coverage differs");return f;
}
std::vector<Fixture> fixtures(const fs::path &directory) {
    std::ifstream file(directory/"manifest.tsv");std::string line;
    require(bool(std::getline(file,line))&&line=="H2_WORLD_DEPTH_ATTACHMENT_V1","attachment manifest version differs");
    std::vector<Fixture> result;
    while(std::getline(file,line)) {
        if(line.empty()||line[0]=='#')continue;
        std::istringstream row(line);Fixture f;std::array<std::string,9> paths;
        require(bool(row>>f.event>>f.role),"invalid attachment fixture row");
        for(auto &name:paths)require(bool(row>>name),"incomplete attachment fixture row");
        std::string extra;require(!(row>>extra),"unexpected attachment fixture field");
        for(const auto &name:paths)require(fs::path(name).is_relative()&&name.find("..") == std::string::npos,"fixture path escapes directory");
        auto state=read(directory/paths[0],sizeof(State));std::memcpy(f.state.w.data(),state.data(),state.size());
        const auto &s=f.state.w;require(s[0]==0x57444731&&s[1]==640&&s[2]==480&&s[3]>0&&s[3]%3==0&&s[3]<=13131,"unsupported retained attachment shape");
        require(s[30]==VK_SAMPLE_COUNT_1_BIT&&s[6]&&s[7]&&!s[26]&&!s[41],"unsupported retained raster state");
        require(s[9]==VK_COMPARE_OP_LESS_OR_EQUAL||s[9]==VK_COMPARE_OP_EQUAL,"unsupported retained comparison");
        if(s[10])require(s[14]==VK_COMPARE_OP_ALWAYS&&s[21]==VK_COMPARE_OP_ALWAYS&&s[11]==VK_STENCIL_OP_KEEP&&s[13]==VK_STENCIL_OP_KEEP&&
            s[18]==VK_STENCIL_OP_KEEP&&s[20]==VK_STENCIL_OP_KEEP,"D32 cannot omit inhibiting original stencil tests");
        f.original=read(directory/paths[1]);f.mask=read(directory/paths[2]);
        f.records=read(directory/paths[3],std::size_t(s[3])*204);f.uniform=read(directory/paths[4],944);
        const std::size_t pixels=std::size_t(s[1])*s[2];
        f.before_depth=read(directory/paths[5],pixels*4);f.before_stencil=read(directory/paths[6],pixels);
        f.after_depth=read(directory/paths[7],pixels*4);f.after_stencil=read(directory/paths[8],pixels);
        for(std::size_t i=0;i<pixels;++i)require((word(f.before_depth,i*4)&0xffffffu)>=256,"canonical prior seeding excludes zero/sub-near values");
        result.push_back(std::move(f));
    }
    require(result.size()==4,"attachment manifest must retain all four draws");
    std::array<unsigned,4> expected{1335,1448,1880,2409};
    for(std::size_t i=0;i<4;++i)require(result[i].event==expected[i],"attachment event ordering differs");return result;
}
struct Buffer {VkBuffer handle{};VkDeviceMemory memory{};void *mapped{};VkDeviceSize size{};};
struct Image {VkImage handle{};VkDeviceMemory memory{};VkImageView view{};VkImageLayout layout=VK_IMAGE_LAYOUT_UNDEFINED;VkImageAspectFlags aspect{};unsigned layers=1;};
std::atomic<unsigned> validation_errors{};
VKAPI_ATTR VkBool32 VKAPI_CALL diagnostic(VkDebugUtilsMessageSeverityFlagBitsEXT severity,VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT *data,void*) {
    if(severity&VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT){++validation_errors;std::cerr<<data->pMessage<<'\n';}return VK_FALSE;
}
struct Result {Bytes depth,stencil,initial_depth,mask,alias;};
struct FogResult {Bytes raw,blended,visibility,depth,stencil;};
class Gpu {
    VkInstance instance{};VkDebugUtilsMessengerEXT messenger{};VkPhysicalDevice physical{};VkDevice device{};VkQueue queue{};unsigned family{};
    VkCommandPool pool{};VkCommandBuffer command{};VkFormat format{};unsigned width=640,height=480;
    std::vector<Buffer> buffers;std::vector<Image> images;std::vector<VkImageView> extra_views;
    std::vector<VkPipeline> pipelines;std::vector<VkShaderModule> modules;std::vector<VkRenderPass> passes;std::vector<VkFramebuffer> frames;
    std::vector<VkDescriptorSetLayout> set_layouts;std::vector<VkPipelineLayout> pipeline_layouts;
    VkDescriptorPool descriptors{};VkSampler sampler{},gradient_sampler{};
    Buffer records,uniform,prior,stencil_input,depth_output,stencil_output,initial_output,mask_output,alias_output;
    Image depth,mask_image,alias_image,dummy2d,dummycube;VkImageView sampled_depth{};
    VkRenderPass draw_pass{},seed_pass{},alias_pass{};VkFramebuffer draw_frame{},seed_frame{},alias_frame{};
    VkPipelineLayout draw_layout{},seed_layout{},alias_layout{};VkDescriptorSet source_set{},record_set{},seed_set{},alias_set{};
    VkPipeline seed_pipeline{},alias_pipeline{};VkShaderModule vertex{},fullscreen{};fs::path shader_directory;
    Buffer fog_uniform,fog_background,fog_raw_output,fog_blend_output,fog_visibility_output;
    Image gradient,fog_image,fog_visibility_image;
    VkDescriptorSet fog_set{};VkPipelineLayout fog_layout{};
    VkRenderPass fog_pass{},fog_load_pass{},fog_visibility_pass{};
    VkFramebuffer fog_frame{},fog_load_frame{},fog_visibility_frame{};
    VkPipeline fog_pipeline{},fog_blend_pipeline{},fog_visibility_pipeline{};
    unsigned memory_type(unsigned bits,VkMemoryPropertyFlags flags) {
        VkPhysicalDeviceMemoryProperties p;vkGetPhysicalDeviceMemoryProperties(physical,&p);
        for(unsigned i=0;i<p.memoryTypeCount;++i)if((bits&(1u<<i))&&(p.memoryTypes[i].propertyFlags&flags)==flags)return i;
        throw std::runtime_error("attachment memory type unavailable");
    }
    Buffer buffer(VkDeviceSize size,VkBufferUsageFlags usage) {
        Buffer b;b.size=size;VkBufferCreateInfo ci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};ci.size=size;ci.usage=usage;
        check(vkCreateBuffer(device,&ci,nullptr,&b.handle));VkMemoryRequirements req;vkGetBufferMemoryRequirements(device,b.handle,&req);
        VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};ai.allocationSize=req.size;ai.memoryTypeIndex=memory_type(req.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        check(vkAllocateMemory(device,&ai,nullptr,&b.memory));check(vkBindBufferMemory(device,b.handle,b.memory,0));check(vkMapMemory(device,b.memory,0,size,0,&b.mapped));buffers.push_back(b);return b;
    }
    Image image(VkFormat fmt,VkImageUsageFlags usage,VkImageAspectFlags aspect,unsigned w,unsigned h,bool cube=false) {
        Image i;i.aspect=aspect;i.layers=cube?6:1;VkImageCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};ci.flags=cube?VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT:0;
        ci.imageType=VK_IMAGE_TYPE_2D;ci.format=fmt;ci.extent={w,h,1};ci.mipLevels=1;ci.arrayLayers=i.layers;ci.samples=VK_SAMPLE_COUNT_1_BIT;ci.tiling=VK_IMAGE_TILING_OPTIMAL;ci.usage=usage;
        check(vkCreateImage(device,&ci,nullptr,&i.handle));VkMemoryRequirements req;vkGetImageMemoryRequirements(device,i.handle,&req);
        VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};ai.allocationSize=req.size;ai.memoryTypeIndex=memory_type(req.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        check(vkAllocateMemory(device,&ai,nullptr,&i.memory));check(vkBindImageMemory(device,i.handle,i.memory,0));
        VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};vi.image=i.handle;vi.format=fmt;vi.viewType=cube?VK_IMAGE_VIEW_TYPE_CUBE:VK_IMAGE_VIEW_TYPE_2D;vi.subresourceRange={aspect,0,1,0,i.layers};
        check(vkCreateImageView(device,&vi,nullptr,&i.view));images.push_back(i);return i;
    }
    VkShaderModule module(const Bytes &raw) {
        require(raw.size()%4==0&&raw.size()>=20&&word(raw,0)==0x07230203,"invalid fixture SPIR-V");
        std::vector<std::uint32_t> words(raw.size()/4);std::memcpy(words.data(),raw.data(),raw.size());
        VkShaderModuleCreateInfo ci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};ci.codeSize=raw.size();ci.pCode=words.data();VkShaderModule m;check(vkCreateShaderModule(device,&ci,nullptr,&m));modules.push_back(m);return m;
    }
    VkDescriptorSetLayout set_layout(std::vector<VkDescriptorSetLayoutBinding> bindings) {
        VkDescriptorSetLayoutCreateInfo ci{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};ci.bindingCount=bindings.size();ci.pBindings=bindings.data();VkDescriptorSetLayout set;
        check(vkCreateDescriptorSetLayout(device,&ci,nullptr,&set));set_layouts.push_back(set);return set;
    }
    VkPipelineLayout layout(std::vector<VkDescriptorSetLayout> sets,bool push=false) {
        VkPushConstantRange range{VK_SHADER_STAGE_FRAGMENT_BIT,0,4};VkPipelineLayoutCreateInfo ci{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};ci.setLayoutCount=sets.size();ci.pSetLayouts=sets.data();
        if(push){ci.pushConstantRangeCount=1;ci.pPushConstantRanges=&range;}VkPipelineLayout l;check(vkCreatePipelineLayout(device,&ci,nullptr,&l));pipeline_layouts.push_back(l);return l;
    }
    VkDescriptorSet allocate(VkDescriptorSetLayout layout) {
        VkDescriptorSetAllocateInfo ai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};ai.descriptorPool=descriptors;ai.descriptorSetCount=1;ai.pSetLayouts=&layout;VkDescriptorSet set;check(vkAllocateDescriptorSets(device,&ai,&set));return set;
    }
    void bind_buffer(VkDescriptorSet set,unsigned binding,Buffer b,VkDescriptorType type,VkDeviceSize size=0) {
        VkDescriptorBufferInfo info{b.handle,0,size?size:b.size};VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};write.dstSet=set;write.dstBinding=binding;write.descriptorCount=1;write.descriptorType=type;write.pBufferInfo=&info;vkUpdateDescriptorSets(device,1,&write,0,nullptr);
    }
    void bind_image(VkDescriptorSet set,unsigned binding,VkImageView view,VkSampler selected=VK_NULL_HANDLE) {
        VkDescriptorImageInfo info{selected?selected:sampler,view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};write.dstSet=set;write.dstBinding=binding;write.descriptorCount=1;write.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;write.pImageInfo=&info;vkUpdateDescriptorSets(device,1,&write,0,nullptr);
    }
    void barrier(Image &i,VkImageLayout next,VkAccessFlags source,VkAccessFlags destination,VkPipelineStageFlags first,VkPipelineStageFlags last) {
        VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};b.image=i.handle;b.subresourceRange={i.aspect,0,1,0,i.layers};b.oldLayout=i.layout;b.newLayout=next;b.srcAccessMask=source;b.dstAccessMask=destination;b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
        vkCmdPipelineBarrier(command,first,last,0,0,nullptr,0,nullptr,1,&b);i.layout=next;
    }
    void begin() {check(vkResetCommandBuffer(command,0));VkCommandBufferBeginInfo ci{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};check(vkBeginCommandBuffer(command,&ci));}
    void end() {check(vkEndCommandBuffer(command));VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;check(vkQueueSubmit(queue,1,&submit,VK_NULL_HANDLE));check(vkQueueWaitIdle(queue));}
    VkRenderPass render_pass(bool color,bool depth_attachment,bool clear_color,VkFormat color_format=VK_FORMAT_UNDEFINED) {
        std::vector<VkAttachmentDescription> attachments;VkAttachmentReference cr{},dr{};
        if(color){VkAttachmentDescription a{};a.format=color_format!=VK_FORMAT_UNDEFINED?color_format:depth_attachment?VK_FORMAT_R8_UNORM:VK_FORMAT_R8G8B8A8_UNORM;a.samples=VK_SAMPLE_COUNT_1_BIT;a.loadOp=clear_color?VK_ATTACHMENT_LOAD_OP_CLEAR:VK_ATTACHMENT_LOAD_OP_LOAD;a.storeOp=VK_ATTACHMENT_STORE_OP_STORE;a.initialLayout=a.finalLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;cr={unsigned(attachments.size()),a.initialLayout};attachments.push_back(a);}
        if(depth_attachment){VkAttachmentDescription a{};a.format=format;a.samples=VK_SAMPLE_COUNT_1_BIT;a.loadOp=color?VK_ATTACHMENT_LOAD_OP_LOAD:VK_ATTACHMENT_LOAD_OP_DONT_CARE;a.storeOp=VK_ATTACHMENT_STORE_OP_STORE;a.stencilLoadOp=format==VK_FORMAT_D24_UNORM_S8_UINT?VK_ATTACHMENT_LOAD_OP_LOAD:VK_ATTACHMENT_LOAD_OP_DONT_CARE;a.stencilStoreOp=format==VK_FORMAT_D24_UNORM_S8_UINT?VK_ATTACHMENT_STORE_OP_STORE:VK_ATTACHMENT_STORE_OP_DONT_CARE;a.initialLayout=a.finalLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;dr={unsigned(attachments.size()),a.initialLayout};attachments.push_back(a);}
        VkSubpassDescription sub{};sub.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;if(color){sub.colorAttachmentCount=1;sub.pColorAttachments=&cr;}if(depth_attachment)sub.pDepthStencilAttachment=&dr;
        VkRenderPassCreateInfo ci{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};ci.attachmentCount=attachments.size();ci.pAttachments=attachments.data();ci.subpassCount=1;ci.pSubpasses=&sub;VkRenderPass p;check(vkCreateRenderPass(device,&ci,nullptr,&p));passes.push_back(p);return p;
    }
    VkFramebuffer framebuffer(VkRenderPass pass,std::vector<VkImageView> views) {
        VkFramebufferCreateInfo ci{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};ci.renderPass=pass;ci.attachmentCount=views.size();ci.pAttachments=views.data();ci.width=width;ci.height=height;ci.layers=1;VkFramebuffer f;check(vkCreateFramebuffer(device,&ci,nullptr,&f));frames.push_back(f);return f;
    }
    VkPipeline pipeline(VkShaderModule vs,VkShaderModule fragment,VkPipelineLayout l,VkRenderPass pass,const State *source,bool seed=false,
                        const std::array<std::uint32_t,12> *captured_blend=nullptr,bool red_only=false) {
        VkPipelineShaderStageCreateInfo stages[2]{};for(auto &stage:stages){stage.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;stage.pName="main";}stages[0].stage=VK_SHADER_STAGE_VERTEX_BIT;stages[0].module=vs;stages[1].stage=VK_SHADER_STAGE_FRAGMENT_BIT;stages[1].module=fragment;
        VkPipelineVertexInputStateCreateInfo input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};assembly.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkViewport viewport{0,0,float(width),float(height),0,1};VkRect2D scissor{{0,0},{width,height}};if(source){viewport=source->viewport();scissor=source->scissor();}
        VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};vp.viewportCount=vp.scissorCount=1;vp.pViewports=&viewport;vp.pScissors=&scissor;
        VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};raster.polygonMode=VK_POLYGON_MODE_FILL;raster.lineWidth=1;
        if(source){const auto &w=source->w;raster.cullMode=w[4];raster.frontFace=VkFrontFace(w[5]);raster.depthClampEnable=w[6];raster.depthBiasEnable=w[26];raster.depthBiasConstantFactor=source->f(27);raster.depthBiasClamp=source->f(28);raster.depthBiasSlopeFactor=source->f(29);}
        VkPipelineMultisampleStateCreateInfo samples{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};samples.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
        VkPipelineDepthStencilStateCreateInfo ds{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};if(source){const auto &w=source->w;ds.depthTestEnable=w[7];ds.depthWriteEnable=w[8];ds.depthCompareOp=VkCompareOp(w[9]);ds.stencilTestEnable=format==VK_FORMAT_D24_UNORM_S8_UINT?w[10]:VK_FALSE;ds.front=source->stencil(11);ds.back=source->stencil(18);ds.depthBoundsTestEnable=w[41];ds.minDepthBounds=source->f(42);ds.maxDepthBounds=source->f(43);}else if(seed){ds.depthTestEnable=ds.depthWriteEnable=VK_TRUE;ds.depthCompareOp=VK_COMPARE_OP_ALWAYS;}
        VkPipelineColorBlendAttachmentState attachment{};attachment.colorWriteMask=source||red_only?VK_COLOR_COMPONENT_R_BIT:VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT;
        VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};if(!seed){blend.attachmentCount=1;blend.pAttachments=&attachment;}
        if(captured_blend){const auto &b=*captured_blend;attachment.blendEnable=VK_TRUE;attachment.srcColorBlendFactor=VkBlendFactor(b[1]);attachment.dstColorBlendFactor=VkBlendFactor(b[2]);attachment.colorBlendOp=VkBlendOp(b[3]);attachment.srcAlphaBlendFactor=VkBlendFactor(b[4]);attachment.dstAlphaBlendFactor=VkBlendFactor(b[5]);attachment.alphaBlendOp=VkBlendOp(b[6]);attachment.colorWriteMask=b[7];for(unsigned i=0;i<4;++i)blend.blendConstants[i]=std::bit_cast<float>(b[8+i]);}
        VkGraphicsPipelineCreateInfo ci{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};ci.stageCount=2;ci.pStages=stages;ci.pVertexInputState=&input;ci.pInputAssemblyState=&assembly;ci.pViewportState=&vp;ci.pRasterizationState=&raster;ci.pMultisampleState=&samples;ci.pDepthStencilState=&ds;ci.pColorBlendState=&blend;ci.layout=l;ci.renderPass=pass;
        VkPipeline p;check(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&ci,nullptr,&p));pipelines.push_back(p);return p;
    }
    void start_pass(VkRenderPass pass,VkFramebuffer frame,bool color_clear=false) {
        VkClearValue clear{};VkRenderPassBeginInfo begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};begin.renderPass=pass;begin.framebuffer=frame;begin.renderArea={{0,0},{width,height}};if(color_clear){begin.clearValueCount=1;begin.pClearValues=&clear;}vkCmdBeginRenderPass(command,&begin,VK_SUBPASS_CONTENTS_INLINE);
    }
    void copy_image(Image &i,Buffer output,VkImageAspectFlags aspect) {
        VkBufferImageCopy copy{};copy.imageSubresource={aspect,0,0,1};copy.imageExtent={width,height,1};vkCmdCopyImageToBuffer(command,i.handle,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,output.handle,1,&copy);
    }
    Bytes bytes(Buffer b)const {return Bytes(static_cast<const std::uint8_t*>(b.mapped),static_cast<const std::uint8_t*>(b.mapped)+b.size);}
public:
    explicit Gpu(VkFormat fmt,const fs::path &shaders):format(fmt),shader_directory(shaders){}
    ~Gpu() {
        if(device){vkDeviceWaitIdle(device);for(auto p:pipelines)vkDestroyPipeline(device,p,nullptr);for(auto m:modules)vkDestroyShaderModule(device,m,nullptr);for(auto f:frames)vkDestroyFramebuffer(device,f,nullptr);for(auto p:passes)vkDestroyRenderPass(device,p,nullptr);for(auto l:pipeline_layouts)vkDestroyPipelineLayout(device,l,nullptr);if(descriptors)vkDestroyDescriptorPool(device,descriptors,nullptr);for(auto s:set_layouts)vkDestroyDescriptorSetLayout(device,s,nullptr);if(sampler)vkDestroySampler(device,sampler,nullptr);if(gradient_sampler)vkDestroySampler(device,gradient_sampler,nullptr);for(auto v:extra_views)vkDestroyImageView(device,v,nullptr);for(auto i:images){vkDestroyImageView(device,i.view,nullptr);vkDestroyImage(device,i.handle,nullptr);vkFreeMemory(device,i.memory,nullptr);}for(auto b:buffers){if(b.mapped)vkUnmapMemory(device,b.memory);vkDestroyBuffer(device,b.handle,nullptr);vkFreeMemory(device,b.memory,nullptr);}if(pool)vkDestroyCommandPool(device,pool,nullptr);vkDestroyDevice(device,nullptr);}
        if(messenger)reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,"vkDestroyDebugUtilsMessengerEXT"))(instance,messenger,nullptr);if(instance)vkDestroyInstance(instance,nullptr);
    }
    bool initialize();
    Result run(const Fixture &,const Bytes &fragment);
    void prepare_full_fog(const FullFogFixture &fixture);
    Result seed_fog_depth(const FullFogFixture &fixture);
    FogResult sample_fog(const FullFogFixture &fixture,const Bytes &program);
};

bool Gpu::initialize() {
    unsigned count=0;check(vkEnumerateInstanceLayerProperties(&count,nullptr));std::vector<VkLayerProperties> layers(count);check(vkEnumerateInstanceLayerProperties(&count,layers.data()));
    if(std::none_of(layers.begin(),layers.end(),[](const auto &p){return std::string(p.layerName)=="VK_LAYER_KHRONOS_validation";}))return false;
    const char *layer="VK_LAYER_KHRONOS_validation";const char *extensions[]{VK_EXT_DEBUG_UTILS_EXTENSION_NAME,VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME};
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.apiVersion=VK_API_VERSION_1_3;
    VkDebugUtilsMessengerCreateInfoEXT debug{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};debug.messageSeverity=VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;debug.messageType=VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;debug.pfnUserCallback=diagnostic;
    VkValidationFeatureEnableEXT synchronization=VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT;
    VkValidationFeaturesEXT validation{VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT};validation.enabledValidationFeatureCount=1;validation.pEnabledValidationFeatures=&synchronization;validation.pNext=&debug;
    VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};ci.pApplicationInfo=&app;ci.enabledLayerCount=1;ci.ppEnabledLayerNames=&layer;ci.enabledExtensionCount=2;ci.ppEnabledExtensionNames=extensions;ci.pNext=&validation;
    check(vkCreateInstance(&ci,nullptr,&instance));check(reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,"vkCreateDebugUtilsMessengerEXT"))(instance,&debug,nullptr,&messenger));
    check(vkEnumeratePhysicalDevices(instance,&count,nullptr));std::vector<VkPhysicalDevice> devices(count);check(vkEnumeratePhysicalDevices(instance,&count,devices.data()));
    for(auto candidate:devices) {
        VkPhysicalDeviceProperties properties;vkGetPhysicalDeviceProperties(candidate,&properties);if(properties.apiVersion<VK_API_VERSION_1_3)continue;
        VkPhysicalDeviceVulkan13Features v13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};VkPhysicalDeviceFeatures2 features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};features.pNext=&v13;vkGetPhysicalDeviceFeatures2(candidate,&features);
        if(!features.features.depthClamp||!v13.shaderDemoteToHelperInvocation)continue;
        bool formats=true;for(auto fmt:{VK_FORMAT_D24_UNORM_S8_UINT,VK_FORMAT_D32_SFLOAT}) {
            VkFormatProperties p;vkGetPhysicalDeviceFormatProperties(candidate,fmt,&p);
            const auto required=VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT|VK_FORMAT_FEATURE_TRANSFER_SRC_BIT|VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
            formats&=(p.optimalTilingFeatures&required)==required;
        }
        if(!formats)continue;vkGetPhysicalDeviceQueueFamilyProperties(candidate,&count,nullptr);std::vector<VkQueueFamilyProperties> families(count);vkGetPhysicalDeviceQueueFamilyProperties(candidate,&count,families.data());
        for(unsigned i=0;i<count;++i)if(families[i].queueFlags&VK_QUEUE_GRAPHICS_BIT){physical=candidate;family=i;break;}if(physical)break;
    }
    if(!physical)return false;
    VkPhysicalDeviceVulkan13Features v13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};v13.shaderDemoteToHelperInvocation=VK_TRUE;
    VkPhysicalDeviceFeatures features{};features.depthClamp=VK_TRUE;float priority=1;
    VkDeviceQueueCreateInfo qi{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};qi.queueFamilyIndex=family;qi.queueCount=1;qi.pQueuePriorities=&priority;
    VkDeviceCreateInfo di{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};di.pNext=&v13;di.pEnabledFeatures=&features;di.queueCreateInfoCount=1;di.pQueueCreateInfos=&qi;
    check(vkCreateDevice(physical,&di,nullptr,&device));vkGetDeviceQueue(device,family,0,&queue);
    VkPhysicalDeviceProperties properties;vkGetPhysicalDeviceProperties(physical,&properties);std::cout<<"World attachment GPU: "<<properties.deviceName<<'\n';
    VkCommandPoolCreateInfo pc{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};pc.queueFamilyIndex=family;pc.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;check(vkCreateCommandPool(device,&pc,nullptr,&pool));
    VkCommandBufferAllocateInfo ca{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};ca.commandPool=pool;ca.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ca.commandBufferCount=1;check(vkAllocateCommandBuffers(device,&ca,&command));
    const auto pixels=std::size_t(width)*height;
    records=buffer(13131*204,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);uniform=buffer(944,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    prior=buffer(pixels*4,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_SRC_BIT);stencil_input=buffer(pixels,VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    depth_output=buffer(pixels*4,VK_BUFFER_USAGE_TRANSFER_DST_BIT);initial_output=buffer(pixels*4,VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    stencil_output=buffer(pixels,VK_BUFFER_USAGE_TRANSFER_DST_BIT);mask_output=buffer(pixels,VK_BUFFER_USAGE_TRANSFER_DST_BIT);alias_output=buffer(pixels*4,VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    const VkImageAspectFlags aspects=VK_IMAGE_ASPECT_DEPTH_BIT|(format==VK_FORMAT_D24_UNORM_S8_UINT?VK_IMAGE_ASPECT_STENCIL_BIT:0);
    depth=image(format,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT,aspects,width,height);
    mask_image=image(VK_FORMAT_R8_UNORM,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT,VK_IMAGE_ASPECT_COLOR_BIT,width,height);
    alias_image=image(VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT,VK_IMAGE_ASPECT_COLOR_BIT,width,height);
    dummy2d=image(VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT,VK_IMAGE_ASPECT_COLOR_BIT,1,1);
    dummycube=image(VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT,VK_IMAGE_ASPECT_COLOR_BIT,1,1,true);
    VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};view.image=depth.handle;view.viewType=VK_IMAGE_VIEW_TYPE_2D;view.format=format;view.subresourceRange={VK_IMAGE_ASPECT_DEPTH_BIT,0,1,0,1};check(vkCreateImageView(device,&view,nullptr,&sampled_depth));extra_views.push_back(sampled_depth);
    VkSamplerCreateInfo si{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};si.magFilter=si.minFilter=VK_FILTER_NEAREST;si.mipmapMode=VK_SAMPLER_MIPMAP_MODE_NEAREST;si.addressModeU=si.addressModeV=si.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;check(vkCreateSampler(device,&si,nullptr,&sampler));
    VkDescriptorPoolSize sizes[]{{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,2},{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,2},{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,7}};
    VkDescriptorPoolCreateInfo dp{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};dp.maxSets=5;dp.poolSizeCount=3;dp.pPoolSizes=sizes;check(vkCreateDescriptorPool(device,&dp,nullptr,&descriptors));
    std::vector<VkDescriptorSetLayoutBinding> source_bindings{{1,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr}};
    for(unsigned i=2;i<=5;++i)source_bindings.push_back({i,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr});
    const auto source_layout=set_layout(source_bindings),record_layout=set_layout({{0,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_VERTEX_BIT,nullptr}}),
        prior_layout=set_layout({{0,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr}}),sample_layout=set_layout({{0,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr}});
    source_set=allocate(source_layout);record_set=allocate(record_layout);seed_set=allocate(prior_layout);alias_set=allocate(sample_layout);
    draw_layout=layout({source_layout,record_layout});seed_layout=layout({prior_layout},true);alias_layout=layout({sample_layout});
    bind_buffer(source_set,1,uniform,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);bind_buffer(record_set,0,records,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);bind_buffer(seed_set,0,prior,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);bind_image(alias_set,0,sampled_depth);
    draw_pass=render_pass(true,true,true);seed_pass=render_pass(false,true,false);alias_pass=render_pass(true,false,true);
    draw_frame=framebuffer(draw_pass,{mask_image.view,depth.view});seed_frame=framebuffer(seed_pass,{depth.view});alias_frame=framebuffer(alias_pass,{alias_image.view});
    vertex=module(read(shader_directory/"menu_world_depth_attachment.vert.spv"));fullscreen=module(read(shader_directory/"menu_world_depth_attachment_fullscreen.vert.spv"));
    seed_pipeline=pipeline(fullscreen,module(read(shader_directory/"menu_world_depth_attachment_seed.frag.spv")),seed_layout,seed_pass,nullptr,true);
    alias_pipeline=pipeline(fullscreen,module(read(shader_directory/"menu_world_depth_attachment_alias.frag.spv")),alias_layout,alias_pass,nullptr);
    begin();for(auto *i:{&dummy2d,&dummycube}) {
        barrier(*i,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,0,VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
        VkClearColorValue white{{1,1,1,1}};VkImageSubresourceRange range{VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,i->layers};vkCmdClearColorImage(command,i->handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,&white,1,&range);
        barrier(*i,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_SHADER_READ_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    }end();return true;
}

Result Gpu::run(const Fixture &fixture,const Bytes &fragment) {
    const auto &s=fixture.state.w;std::memcpy(records.mapped,fixture.records.data(),fixture.records.size());std::memcpy(uniform.mapped,fixture.uniform.data(),fixture.uniform.size());std::memcpy(prior.mapped,fixture.before_depth.data(),fixture.before_depth.size());std::memcpy(stencil_input.mapped,fixture.before_stencil.data(),fixture.before_stencil.size());
    bind_buffer(record_set,0,records,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,fixture.records.size());
    for(unsigned i=0;i<4;++i)bind_image(source_set,i+2,(s[25]&(1u<<i))?dummycube.view:dummy2d.view);
    const auto draw_pipeline=pipeline(vertex,module(fragment),draw_layout,draw_pass,&fixture.state);
    begin();VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER};host.srcAccessMask=VK_ACCESS_HOST_WRITE_BIT;host.dstAccessMask=VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_UNIFORM_READ_BIT|VK_ACCESS_TRANSFER_READ_BIT;
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_HOST_BIT,VK_PIPELINE_STAGE_VERTEX_SHADER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_TRANSFER_BIT,0,1,&host,0,nullptr,0,nullptr);
    const auto old_stage=depth.layout==VK_IMAGE_LAYOUT_UNDEFINED?VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT:VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    const auto old_access=depth.layout==VK_IMAGE_LAYOUT_UNDEFINED?0u:VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT;
    if(format==VK_FORMAT_D24_UNORM_S8_UINT) {
        barrier(depth,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,old_access,VK_ACCESS_TRANSFER_WRITE_BIT,old_stage,VK_PIPELINE_STAGE_TRANSFER_BIT);
        VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_DEPTH_BIT,0,0,1};copy.imageExtent={width,height,1};vkCmdCopyBufferToImage(command,prior.handle,depth.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy);
        copy.imageSubresource.aspectMask=VK_IMAGE_ASPECT_STENCIL_BIT;vkCmdCopyBufferToImage(command,stencil_input.handle,depth.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy);
    } else {
        barrier(depth,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,old_access,VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,old_stage,VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
        start_pass(seed_pass,seed_frame);vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,seed_pipeline);vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,seed_layout,0,1,&seed_set,0,nullptr);vkCmdPushConstants(command,seed_layout,VK_SHADER_STAGE_FRAGMENT_BIT,0,4,&width);vkCmdDraw(command,3,1,0,0);vkCmdEndRenderPass(command);
    }
    // Observe actual seeded storage before the tested draw, not a CPU float proxy.
    if(format==VK_FORMAT_D24_UNORM_S8_UINT)barrier(depth,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_TRANSFER_READ_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
    else barrier(depth,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,VK_ACCESS_TRANSFER_READ_BIT,VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
    copy_image(depth,initial_output,VK_IMAGE_ASPECT_DEPTH_BIT);
    barrier(depth,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,VK_ACCESS_TRANSFER_READ_BIT,VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
    barrier(mask_image,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,mask_image.layout==VK_IMAGE_LAYOUT_UNDEFINED?0:VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT,VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,mask_image.layout==VK_IMAGE_LAYOUT_UNDEFINED?VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT:VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    start_pass(draw_pass,draw_frame,true);vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,draw_pipeline);VkDescriptorSet sets[]{source_set,record_set};vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,draw_layout,0,2,sets,0,nullptr);vkCmdDraw(command,s[3],1,0,0);vkCmdEndRenderPass(command);
    barrier(mask_image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_ACCESS_TRANSFER_READ_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);copy_image(mask_image,mask_output,VK_IMAGE_ASPECT_COLOR_BIT);
    barrier(mask_image,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,VK_ACCESS_TRANSFER_READ_BIT,VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    barrier(depth,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,VK_ACCESS_SHADER_READ_BIT,VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    barrier(alias_image,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,alias_image.layout==VK_IMAGE_LAYOUT_UNDEFINED?0:VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT,VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,alias_image.layout==VK_IMAGE_LAYOUT_UNDEFINED?VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT:VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    start_pass(alias_pass,alias_frame,true);vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,alias_pipeline);vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,alias_layout,0,1,&alias_set,0,nullptr);vkCmdDraw(command,3,1,0,0);vkCmdEndRenderPass(command);
    barrier(alias_image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_ACCESS_TRANSFER_READ_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);copy_image(alias_image,alias_output,VK_IMAGE_ASPECT_COLOR_BIT);
    barrier(alias_image,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,VK_ACCESS_TRANSFER_READ_BIT,VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    barrier(depth,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_ACCESS_SHADER_READ_BIT,VK_ACCESS_TRANSFER_READ_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);copy_image(depth,depth_output,VK_IMAGE_ASPECT_DEPTH_BIT);if(format==VK_FORMAT_D24_UNORM_S8_UINT)copy_image(depth,stencil_output,VK_IMAGE_ASPECT_STENCIL_BIT);
    barrier(depth,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_ACCESS_TRANSFER_READ_BIT,VK_ACCESS_SHADER_READ_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    VkMemoryBarrier transfer{VK_STRUCTURE_TYPE_MEMORY_BARRIER};transfer.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;transfer.dstAccessMask=VK_ACCESS_HOST_READ_BIT;vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&transfer,0,nullptr,0,nullptr);end();
    return {bytes(depth_output),format==VK_FORMAT_D24_UNORM_S8_UINT?bytes(stencil_output):Bytes{},bytes(initial_output),bytes(mask_output),bytes(alias_output)};
}

void Gpu::prepare_full_fog(const FullFogFixture &fixture) {
    require(!fog_set,"full fog resources already prepared");
    const auto pixels=std::size_t(width)*height;
    fog_uniform=buffer(272,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    fog_background=buffer(pixels*4,VK_BUFFER_USAGE_TRANSFER_SRC_BIT);std::memcpy(fog_background.mapped,fixture.before.data(),fixture.before.size());
    fog_raw_output=buffer(pixels*4,VK_BUFFER_USAGE_TRANSFER_DST_BIT);fog_blend_output=buffer(pixels*4,VK_BUFFER_USAGE_TRANSFER_DST_BIT);fog_visibility_output=buffer(pixels,VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    gradient=image(VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,VK_IMAGE_ASPECT_COLOR_BIT,16,16);
    const auto gradient_input=buffer(1024,VK_BUFFER_USAGE_TRANSFER_SRC_BIT);std::memcpy(gradient_input.mapped,fixture.gradient.data(),1024);
    fog_image=image(VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT,VK_IMAGE_ASPECT_COLOR_BIT,width,height);
    fog_visibility_image=image(VK_FORMAT_R8_UNORM,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT,VK_IMAGE_ASPECT_COLOR_BIT,width,height);
    VkSamplerCreateInfo si{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};si.magFilter=si.minFilter=VK_FILTER_LINEAR;si.addressModeU=si.addressModeV=si.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    check(vkCreateSampler(device,&si,nullptr,&gradient_sampler));
    const auto descriptor_layout=set_layout({{0,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
        {1,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},{2,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr}});
    fog_set=allocate(descriptor_layout);fog_layout=layout({descriptor_layout});bind_image(fog_set,0,sampled_depth);bind_image(fog_set,1,gradient.view,gradient_sampler);bind_buffer(fog_set,2,fog_uniform,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    fog_pass=render_pass(true,false,true);fog_load_pass=render_pass(true,false,false);fog_visibility_pass=render_pass(true,false,true,VK_FORMAT_R8_UNORM);
    fog_frame=framebuffer(fog_pass,{fog_image.view});fog_load_frame=framebuffer(fog_load_pass,{fog_image.view});fog_visibility_frame=framebuffer(fog_visibility_pass,{fog_visibility_image.view});
    const auto fog_vertex=module(read(shader_directory/"fog.vert.spv")),fog_fragment=module(read(shader_directory/"fog.frag.spv"));
    fog_pipeline=pipeline(fog_vertex,fog_fragment,fog_layout,fog_pass,nullptr);
    fog_blend_pipeline=pipeline(fog_vertex,fog_fragment,fog_layout,fog_load_pass,nullptr,false,&fixture.blend);
    fog_visibility_pipeline=pipeline(fog_vertex,module(read(shader_directory/"menu_world_depth_attachment_fog_visibility.frag.spv")),fog_layout,fog_visibility_pass,nullptr,false,nullptr,true);
    begin();VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER};host.srcAccessMask=VK_ACCESS_HOST_WRITE_BIT;host.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_HOST_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,1,&host,0,nullptr,0,nullptr);
    barrier(gradient,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,0,VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
    VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={16,16,1};vkCmdCopyBufferToImage(command,gradient_input.handle,gradient.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy);
    barrier(gradient,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_SHADER_READ_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);end();
}

Result Gpu::seed_fog_depth(const FullFogFixture &fixture) {
    // Source event1872 has no depth writes. Use its own actual prior bank,
    // rather than the after bank from any earlier isolated base draw.
    std::memcpy(prior.mapped,fixture.depth.data(),fixture.depth.size());std::memcpy(stencil_input.mapped,fixture.stencil.data(),fixture.stencil.size());
    begin();VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER};host.srcAccessMask=VK_ACCESS_HOST_WRITE_BIT;host.dstAccessMask=VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_TRANSFER_READ_BIT;
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_HOST_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_TRANSFER_BIT,0,1,&host,0,nullptr,0,nullptr);
    const auto old_stage=depth.layout==VK_IMAGE_LAYOUT_UNDEFINED?VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT:VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    const auto old_access=depth.layout==VK_IMAGE_LAYOUT_UNDEFINED?0u:VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT;
    if(format==VK_FORMAT_D24_UNORM_S8_UINT) {
        barrier(depth,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,old_access,VK_ACCESS_TRANSFER_WRITE_BIT,old_stage,VK_PIPELINE_STAGE_TRANSFER_BIT);
        VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_DEPTH_BIT,0,0,1};copy.imageExtent={width,height,1};vkCmdCopyBufferToImage(command,prior.handle,depth.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy);
        copy.imageSubresource.aspectMask=VK_IMAGE_ASPECT_STENCIL_BIT;vkCmdCopyBufferToImage(command,stencil_input.handle,depth.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy);
        barrier(depth,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_TRANSFER_READ_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
    } else {
        barrier(depth,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,old_access,VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,old_stage,VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
        start_pass(seed_pass,seed_frame);vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,seed_pipeline);vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,seed_layout,0,1,&seed_set,0,nullptr);vkCmdPushConstants(command,seed_layout,VK_SHADER_STAGE_FRAGMENT_BIT,0,4,&width);vkCmdDraw(command,3,1,0,0);vkCmdEndRenderPass(command);
        barrier(depth,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,VK_ACCESS_TRANSFER_READ_BIT,VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
    }
    copy_image(depth,depth_output,VK_IMAGE_ASPECT_DEPTH_BIT);if(format==VK_FORMAT_D24_UNORM_S8_UINT)copy_image(depth,stencil_output,VK_IMAGE_ASPECT_STENCIL_BIT);
    barrier(depth,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_ACCESS_TRANSFER_READ_BIT,VK_ACCESS_SHADER_READ_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    VkMemoryBarrier transfer{VK_STRUCTURE_TYPE_MEMORY_BARRIER};transfer.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;transfer.dstAccessMask=VK_ACCESS_HOST_READ_BIT;vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&transfer,0,nullptr,0,nullptr);end();
    Result r;r.depth=bytes(depth_output);if(format==VK_FORMAT_D24_UNORM_S8_UINT)r.stencil=bytes(stencil_output);return r;
}

FogResult Gpu::sample_fog(const FullFogFixture &,const Bytes &program) {
    require(fog_set&&program.size()==272&&depth.layout==VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,"full fog resources/depth unready");
    std::memcpy(fog_uniform.mapped,program.data(),272);
    begin();VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER};host.srcAccessMask=VK_ACCESS_HOST_WRITE_BIT;host.dstAccessMask=VK_ACCESS_UNIFORM_READ_BIT|VK_ACCESS_TRANSFER_READ_BIT;
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_HOST_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_TRANSFER_BIT,0,1,&host,0,nullptr,0,nullptr);
    barrier(fog_image,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,fog_image.layout==VK_IMAGE_LAYOUT_UNDEFINED?0:VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT,VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        fog_image.layout==VK_IMAGE_LAYOUT_UNDEFINED?VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT:VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    auto draw=[&](VkRenderPass pass,VkFramebuffer frame,VkPipeline selected,bool clear){start_pass(pass,frame,clear);vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,selected);vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,fog_layout,0,1,&fog_set,0,nullptr);vkCmdDraw(command,3,1,0,0);vkCmdEndRenderPass(command);};
    draw(fog_pass,fog_frame,fog_pipeline,true);
    barrier(fog_image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_ACCESS_TRANSFER_READ_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);copy_image(fog_image,fog_raw_output,VK_IMAGE_ASPECT_COLOR_BIT);
    // Captured blending uses an actual pre1872 background. For earlier draw
    // banks and HILO this is explicitly a controlled compositor input.
    barrier(fog_image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_ACCESS_TRANSFER_READ_BIT,VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
    VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={width,height,1};vkCmdCopyBufferToImage(command,fog_background.handle,fog_image.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy);
    barrier(fog_image,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    draw(fog_load_pass,fog_load_frame,fog_blend_pipeline,false);
    barrier(fog_image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_ACCESS_TRANSFER_READ_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);copy_image(fog_image,fog_blend_output,VK_IMAGE_ASPECT_COLOR_BIT);
    barrier(fog_visibility_image,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,fog_visibility_image.layout==VK_IMAGE_LAYOUT_UNDEFINED?0:VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT,VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        fog_visibility_image.layout==VK_IMAGE_LAYOUT_UNDEFINED?VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT:VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    draw(fog_visibility_pass,fog_visibility_frame,fog_visibility_pipeline,true);
    barrier(fog_visibility_image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_ACCESS_TRANSFER_READ_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);copy_image(fog_visibility_image,fog_visibility_output,VK_IMAGE_ASPECT_COLOR_BIT);
    barrier(depth,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_ACCESS_SHADER_READ_BIT,VK_ACCESS_TRANSFER_READ_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);copy_image(depth,depth_output,VK_IMAGE_ASPECT_DEPTH_BIT);if(format==VK_FORMAT_D24_UNORM_S8_UINT)copy_image(depth,stencil_output,VK_IMAGE_ASPECT_STENCIL_BIT);
    barrier(depth,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_ACCESS_TRANSFER_READ_BIT,VK_ACCESS_SHADER_READ_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    VkMemoryBarrier transfer{VK_STRUCTURE_TYPE_MEMORY_BARRIER};transfer.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;transfer.dstAccessMask=VK_ACCESS_HOST_READ_BIT;vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&transfer,0,nullptr,0,nullptr);end();
    return {bytes(fog_raw_output),bytes(fog_blend_output),bytes(fog_visibility_output),bytes(depth_output),format==VK_FORMAT_D24_UNORM_S8_UINT?bytes(stencil_output):Bytes{}};
}

struct Measurement {
    unsigned event{};std::string producer,format,depth_reference;std::size_t depth_differences{},stencil_differences{},alias_differences{},mask_differences{},accepted_pixels{},initial_changed_words{};
};
struct FogMeasurement {
    unsigned event{};std::string producer,format,bank,reference;
    std::size_t raw_differences{},blend_differences{},visibility_differences{},visibility_expected_differences{},
        preserved_depth_differences{},preserved_stencil_differences{},accepted_pixels{};
};
Bytes expected_fog_visibility(const Bytes &depth,bool eye) {
    Bytes mask(depth.size()/4,255);if(eye)for(std::size_t i=0;i<mask.size();++i)mask[i]=(word(depth,i*4)&0xffffffu)==0xffffffu?255:0;return mask;
}
Bytes expected_eye_raw(const Bytes &mask) {
    Bytes rgba(mask.size()*4);constexpr std::array<std::uint8_t,4> color{32,27,22,65};
    for(std::size_t i=0;i<mask.size();++i)if(mask[i])std::copy(color.begin(),color.end(),rgba.begin()+i*4);return rgba;
}
void retain_fog(const fs::path &output,unsigned event,const std::string &format,const std::string &producer,const std::string &bank,const FogResult &r) {
    const auto prefix=std::to_string(event)+"-"+format+"-"+producer+"-"+bank+"-full-fog";
    save(output/(prefix+"-raw.rgba"),r.raw);save(output/(prefix+"-blended.rgba"),r.blended);save(output/(prefix+"-visibility.r8"),r.visibility);
}
void write_full_fog_report(const fs::path &output,const std::vector<FogMeasurement> &rows,bool captured,bool cross_format,bool precise,const std::string &failure) {
    const bool complete=rows.size()==50&&failure.empty()&&validation_errors==0;
    std::ofstream file(output/"world-depth-full-fog.json");
    file<<"{\n  \"status\": \""<<(complete&&captured&&cross_format&&precise?"passed":"failed")<<"\",\n"
        <<"  \"measurement_contract_complete\": "<<(complete?"true":"false")<<",\n"
        <<"  \"captured_eye_baseline_passed\": "<<(complete&&captured?"true":"false")<<",\n"
        <<"  \"sampled_full_fog_cross_format_contract_passed\": "<<(complete&&cross_format?"true":"false")<<",\n"
        <<"  \"precise_helper_full_fog_contract_passed\": "<<(complete&&precise?"true":"false")<<",\n"
        <<"  \"validation_errors\": "<<validation_errors.load()<<",\n"
        <<"  \"underwater_original_GPU_framebuffer_verified\": false,\n  \"actual_prior_native_D32_history_verified\": false,\n  \"production_pipeline_verified\": false,\n"
        <<"  \"eye_authority\": \"Captured1872 own depth, pre1865 RGBA and post1872 RGBA; actual selected fog bank.\",\n"
        <<"  \"HILO_authority\": \"Unchanged selected-XBE mode5 program and26710 lookup; actual gradient; other depth/background combinations controlled.\",\n"
        <<"  \"cross_format_reference\": \"Same producer D24 actual attachment sampled by unchanged production fog.frag.\",\n"
        <<"  \"failure\": \"";
    for(char c:failure){if(c=='\\'||c=='\"')file<<'\\';if(c=='\n')file<<"\\n";else file<<c;}file<<"\",\n  \"measurements\": [\n";
    for(std::size_t i=0;i<rows.size();++i){const auto &r=rows[i];file<<"    {\"event\": "<<r.event<<", \"format\": \""<<r.format<<"\", \"producer\": \""<<r.producer<<"\", \"fog_bank\": \""<<r.bank
        <<"\", \"reference\": \""<<r.reference<<"\", \"raw_color_different_bytes\": "<<r.raw_differences<<", \"blended_color_different_bytes\": "<<r.blend_differences
        <<", \"visibility_reference_different_bytes\": "<<r.visibility_differences<<", \"source_visibility_different_bytes\": "<<r.visibility_expected_differences
        <<", \"sample_pass_depth_changed_words\": "<<r.preserved_depth_differences<<", \"sample_pass_stencil_changed_bytes\": "<<r.preserved_stencil_differences
        <<", \"visible_pixels\": "<<r.accepted_pixels<<"}"<<(i+1<rows.size()?",":"")<<'\n';}file<<"  ]\n}\n";
}
Bytes expected_alias(const Bytes &depth) {
    Bytes result(depth.size());for(std::size_t i=0;i<depth.size();i+=4){const auto n=word(depth,i)&0xffffffu;result[i]=(n>>8)&255;result[i+1]=n&255;result[i+2]=0;result[i+3]=n>>16;}return result;
}
void retain(const fs::path &output,unsigned event,const std::string &format,const std::string &producer,const Result &r) {
    const auto prefix=std::to_string(event)+"-"+format+"-"+producer;
    save(output/(prefix+"-depth.bin"),r.depth);save(output/(prefix+"-initial-depth.bin"),r.initial_depth);save(output/(prefix+"-mask.r8"),r.mask);save(output/(prefix+"-sampled-alias.rgba"),r.alias);if(!r.stencil.empty())save(output/(prefix+"-stencil.bin"),r.stencil);
}
void write_report(const fs::path &output,const std::vector<Measurement> &rows,bool baseline,bool source_contract,bool precise_contract,const std::string &failure) {
    std::ofstream file(output/"world-depth-attachment.json");
    const bool complete=rows.size()==32&&failure.empty()&&validation_errors==0;
    file<<"{\n  \"status\": \""<<(complete&&baseline&&source_contract&&precise_contract?"passed":"failed")<<"\",\n"
        <<"  \"original_D24_baseline_passed\": "<<(baseline?"true":"false")<<",\n"
        <<"  \"measurement_contract_complete\": "<<(complete?"true":"false")<<",\n"
        <<"  \"original_D32_controlled_input_contract_passed\": "<<(complete&&source_contract?"true":"false")<<",\n"
        <<"  \"precise_helper_attachment_contract_passed\": "<<(complete&&precise_contract?"true":"false")<<",\n"
        <<"  \"synchronization_validation_enabled\": true,\n  \"validation_errors\": "<<validation_errors.load()<<",\n"
        <<"  \"actual_prior_native_D32_history_verified\": false,\n  \"production_world_pipeline_verified\": false,\n  \"full_fog_combiners_and_gradient_verified\": false,\n"
        <<"  \"alias_scope\": \"Actual depth attachment texture sample through fog_color.glsl; diagnostic samples0 output, omitted stencil byte0.\",\n"
        <<"  \"prior_D32_scope\": \"Controlled canonical GPU quantizer seed from retained prior integer depth; no proof of preceding native draws or formats.\",\n"
        <<"  \"failure\": \"";
    for(char c:failure){if(c=='\\'||c=='\"')file<<'\\';if(c=='\n')file<<"\\n";else file<<c;}file<<"\",\n  \"measurements\": [\n";
    for(std::size_t i=0;i<rows.size();++i){const auto &r=rows[i];file<<"    {\"event\": "<<r.event<<", \"format\": \""<<r.format<<"\", \"producer\": \""<<r.producer<<"\", \"depth_comparison_reference\": \""<<r.depth_reference<<"\", \"depth_comparison_different_words\": "<<r.depth_differences<<", \"source_after_stencil_different_bytes\": "<<r.stencil_differences<<", \"sampled_alias_different_bytes\": "<<r.alias_differences<<", \"source_D24_mask_different_bytes\": "<<r.mask_differences<<", \"accepted_mask_pixels\": "<<r.accepted_pixels<<", \"changed_words_from_actual_seed\": "<<r.initial_changed_words<<"}"<<(i+1<rows.size()?",":"")<<'\n';}file<<"  ]\n}\n";
}
}

int main(int argc,char **argv) {
    std::vector<Measurement> measurements;bool baseline=false,source_contract=false,precise_contract=false,output_ready=false;std::string failure;fs::path output;
    std::vector<FogMeasurement> fog_measurements;std::optional<FullFogFixture> fog;
    bool full_mode=false,captured_contract=false,fog_cross_format=false,fog_precise=false;
    try {
        require(argc==3||(argc==5&&std::string(argv[3])=="--full-fog"),"attachment fixture/output directories [--full-fog fog-fixtures] required");const fs::path directory=argv[1];output=argv[2];full_mode=argc==5;
        if(!fs::is_regular_file(directory/"manifest.tsv"))return 77;
        if(full_mode&&!fs::is_regular_file(fs::path(argv[4])/"manifest.tsv"))return 77;
        require(!fs::exists(output/"world-depth-attachment.json")&&!fs::exists(output/"world-depth-full-fog.json"),"refusing to overwrite completed attachment/fog proof; select fresh output");fs::create_directories(output);output_ready=true;
        if(full_mode)fog=full_fog_fixture(argv[4]);
        const auto bank=fixtures(directory);const fs::path shaders=HALO2_MENU_WORLD_DEPTH_ATTACHMENT_SHADER_DIR;
        const auto current=read(shaders/"menu_world_depth_attachment_helper.frag.spv"),precise=read(shaders/"menu_world_depth_attachment_helper_precise.frag.spv");
        std::vector<Result> source_d24,mask_d24;
        std::array<std::array<std::array<FogResult,2>,3>,4> fog_d24;
        source_contract=precise_contract=true;
        fog_cross_format=fog_precise=captured_contract=true;
        for(auto format:{VK_FORMAT_D24_UNORM_S8_UINT,VK_FORMAT_D32_SFLOAT}) {
            const bool d24=format==VK_FORMAT_D24_UNORM_S8_UINT;const std::string label=d24?"d24":"d32";Gpu gpu(format,shaders);
            if(!gpu.initialize()){std::cout<<"Vulkan1.3, demotion, both sampled depth formats and validation layer required\n";return 77;}
            if(fog)gpu.prepare_full_fog(*fog);
            for(std::size_t index=0;index<bank.size();++index) {
                const auto &f=bank[index];const auto expected=expected_alias(f.after_depth);
                const auto original=gpu.run(f,f.original);retain(output,f.event,label,"original",original);
                std::array<FogResult,2> original_fog;
                auto observe_fog=[&](unsigned producer,const std::string &name,const Result &depth_result) {
                    if(!fog)return;
                    for(unsigned b=0;b<2;++b) {
                        const auto result=gpu.sample_fog(*fog,b?fog->hilo:fog->eye);const std::string bank_name=b?"selected-HILO":"captured-eye";
                        retain_fog(output,f.event,label,name,bank_name,result);
                        if(producer==0)original_fog[b]=result;
                        const auto &reference=d24?original_fog[b]:fog_d24[index][producer][b];
                        FogMeasurement m;m.event=f.event;m.format=label;m.producer=name;m.bank=bank_name;
                        m.reference=d24?"Same-format full fog original-W control; HILO not captured GPU framebuffer":"Same-producer D24 full fog result; controlled D32 prior input";
                        m.raw_differences=different(result.raw,reference.raw);m.blend_differences=different(result.blended,reference.blended);m.visibility_differences=different(result.visibility,reference.visibility);
                        m.visibility_expected_differences=different(result.visibility,expected_fog_visibility(f.after_depth,!b));
                        m.preserved_depth_differences=depth_words(result.depth,depth_result.depth,d24);m.preserved_stencil_differences=different(result.stencil,depth_result.stencil);
                        m.accepted_pixels=std::count(result.visibility.begin(),result.visibility.end(),std::uint8_t(255));fog_measurements.push_back(m);
                        require(!m.preserved_depth_differences&&!m.preserved_stencil_differences,"full fog sampling changed attachment depth/stencil");
                        if(producer==0||producer==2)require(!m.visibility_expected_differences,"full fog source/precise visibility differs from original integers");
                        if(!d24)fog_cross_format&=m.raw_differences==0&&m.blend_differences==0&&m.visibility_differences==0;
                        if(producer==2){const auto &gold=original_fog[b];fog_precise&=different(result.raw,gold.raw)==0&&different(result.blended,gold.blended)==0&&different(result.visibility,gold.visibility)==0;}
                        if(d24)fog_d24[index][producer][b]=result;
                    }
                };
                observe_fog(0,"original",original);
                const auto mask=gpu.run(f,f.mask);retain(output,f.event,label,"source-mask",mask);
                require(depth_words(original.depth,mask.depth,d24)==0&&depth_words(original.initial_depth,mask.initial_depth,d24)==0&&original.stencil==mask.stencil,"source color-mask derivative changed depth/stencil or controlled seed");
                if(d24){require(depth_words(original.depth,f.after_depth,true)==0&&original.stencil==f.after_stencil&&depth_words(original.initial_depth,f.before_depth,true)==0,"unchanged original FS D24 baseline differs from retained draw");source_d24.push_back(original);mask_d24.push_back(mask);}
                const auto &gold=mask_d24[index];
                auto measure=[&](const std::string &producer,const Result &r,bool is_mask) {
                    Measurement m;m.event=f.event;m.producer=producer;m.format=label;
                    m.depth_reference=d24?"retained original encoded D24 low24":"same-format unchanged original FS D32 storage; sampled alias independently checks retained D24 integers";
                    m.depth_differences=d24?depth_words(r.depth,f.after_depth,true):depth_words(r.depth,original.depth,false);
                    m.stencil_differences=d24?different(r.stencil,f.after_stencil):0;m.alias_differences=different(r.alias,expected);
                    m.mask_differences=is_mask?different(r.mask,gold.mask):0;m.accepted_pixels=is_mask?std::count(r.mask.begin(),r.mask.end(),std::uint8_t(255)):0;
                    m.initial_changed_words=depth_words(r.depth,r.initial_depth,d24);
                    measurements.push_back(m);std::cout<<f.event<<' '<<label<<' '<<producer<<": "<<m.depth_differences<<" depth words, "<<m.alias_differences<<" sampled alias bytes, "<<m.mask_differences<<" acceptance mask bytes differ\n";return m;
                };
                const auto a=measure("original",original,false),b=measure("source-mask",mask,true);
                source_contract&=a.alias_differences==0&&b.alias_differences==0&&b.mask_differences==0;
                require(b.accepted_pixels>0,"retained source acceptance mask has no covered pixels");
                unsigned producer=1;
                for(const auto &entry:std::array<std::pair<const char*,const Bytes*>,2>{{{"current-helper",&current},{"precise-helper",&precise}}}) {
                    const auto result=gpu.run(f,*entry.second);retain(output,f.event,label,entry.first,result);
                    require(depth_words(result.initial_depth,original.initial_depth,d24)==0,"producer changed controlled prior-depth input");
                    const auto m=measure(entry.first,result,true);
                    if(std::string(entry.first)=="precise-helper")precise_contract&=m.depth_differences==0&&m.stencil_differences==0&&m.alias_differences==0&&m.mask_differences==0;
                    observe_fog(producer++,entry.first,result);
                }
            }
            if(d24)baseline=true;
            if(fog) {
                const auto captured_depth=gpu.seed_fog_depth(*fog);const auto result=gpu.sample_fog(*fog,fog->eye);retain_fog(output,1872,label,"captured-input","captured-eye",result);
                FogMeasurement m;m.event=1872;m.format=label;m.producer="captured-input";m.bank="captured-eye";m.reference="Original own-depth pre1865/post1872 capture; raw exact selected eye constant";
                m.raw_differences=different(result.raw,expected_eye_raw(fog->visibility));m.blend_differences=different(result.blended,fog->after);
                m.visibility_differences=m.visibility_expected_differences=different(result.visibility,fog->visibility);m.accepted_pixels=std::count(result.visibility.begin(),result.visibility.end(),std::uint8_t(255));
                m.preserved_depth_differences=depth_words(result.depth,captured_depth.depth,d24);m.preserved_stencil_differences=different(result.stencil,captured_depth.stencil);fog_measurements.push_back(m);
                captured_contract&=m.raw_differences==0&&m.blend_differences==0&&m.visibility_differences==0&&m.preserved_depth_differences==0&&m.preserved_stencil_differences==0;
                require(captured_contract,"captured original eye fog baseline differs; no tolerance admitted");
            }
        }
        require(validation_errors==0,"attachment core/synchronization validation errors");
    }catch(const std::exception &e){failure=e.what();std::cerr<<failure<<'\n';}
    if(output_ready){write_report(output,measurements,baseline,source_contract,precise_contract,measurements.size()==32?"":failure);if(full_mode)write_full_fog_report(output,fog_measurements,captured_contract,fog_cross_format,fog_precise,failure);}
    if(!failure.empty()||measurements.size()!=32||!baseline||!source_contract||!precise_contract||validation_errors)return 1;
    if(full_mode&&(fog_measurements.size()!=50||!captured_contract||!fog_cross_format||!fog_precise))return 1;
    std::cout<<"Four original ordinary draws retain exact source depth/stencil, sampled alias and EQUAL masks across controlled D24/D32 inputs. Current-helper differences are diagnostic; actual prior native history remains unverified.\n";return 0;
}
