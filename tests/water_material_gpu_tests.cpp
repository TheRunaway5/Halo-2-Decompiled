#include "halo2/water.hpp"
#include "halo2/original_data.hpp"
#include "halo2/world_material.hpp"
#include "halo2/world_material_uniform.hpp"
#include "halo2/native_world_kernel.hpp"
#include "halo2/source_vertex_inputs.hpp"
#include "halo2/source_frustum_camera.hpp"
#include "halo2/source_view_projection.hpp"
#include "halo2/math.hpp"
#include "native_world_kernels.generated.hpp"
#include <vulkan/vulkan.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>

// Numerical component test: the production precompiled native VS runs with
// its real vertex/UBO/push ABI. Only the following capture GS is test-specific.
// CPU instruction execution is an offline arithmetic oracle, never runtime.
namespace {
using namespace halo2;
using json=nlohmann::json;
namespace fs=std::filesystem;
void require(bool value,const std::string& why){if(!value)throw FormatError(why);}
void check(VkResult value){require(value==VK_SUCCESS,"water capture Vulkan result "+std::to_string(value));}
std::vector<std::uint32_t> words(const fs::path& path) {
    std::ifstream file(path,std::ios::binary|std::ios::ate);
    require(bool(file),"missing compiled shader: "+path.string());const auto size=file.tellg();
    require(size>=20 && size<16*1024*1024 && size%4==0,"invalid SPIR-V extent");
    std::vector<std::uint32_t> out(std::size_t(size)/4);file.seekg(0);
    require(bool(file.read(reinterpret_cast<char*>(out.data()),size)),"truncated SPIR-V");
    require(out[0]==0x07230203u,"shader lost SPIR-V identity");return out;
}
struct Push {
    std::array<ShaderVector,4> matrix{};
    ShaderVector forward{},eye{},viewport{};
};
static_assert(sizeof(Push)==112 && offsetof(Push,forward)==64 && offsetof(Push,viewport)==96);
static_assert(sizeof(WorldMaterialUniform)==3712 && offsetof(WorldMaterialUniform,vertex_instructions)==464);
static_assert(sizeof(PackedVertexProgramBank)==2544);
class Gpu {
    struct Buffer {VkBuffer value{};VkDeviceMemory memory{};void* mapped{};VkDeviceSize bytes{};};
    VkInstance instance{};VkPhysicalDevice physical{};VkDevice device{};VkQueue queue{};
    VkCommandPool commands{};VkCommandBuffer command{};VkDescriptorPool pool{};
    std::array<VkDescriptorSetLayout,2> sets{};VkPipelineLayout layout{};
    VkRenderPass pass{};VkFramebuffer frame{};VkShaderModule capture{};
    std::vector<Buffer> buffers;std::vector<VkShaderModule> modules;
    std::map<unsigned,VkPipeline> pipelines;
    fs::path shaders;
    Buffer allocate(VkDeviceSize bytes,VkBufferUsageFlags usage) {
        Buffer out;out.bytes=bytes;VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        info.size=bytes;info.usage=usage;check(vkCreateBuffer(device,&info,nullptr,&out.value));
        VkMemoryRequirements need;vkGetBufferMemoryRequirements(device,out.value,&need);
        VkPhysicalDeviceMemoryProperties properties;vkGetPhysicalDeviceMemoryProperties(physical,&properties);
        unsigned type=properties.memoryTypeCount;
        for(unsigned i=0;i<properties.memoryTypeCount;++i)if((need.memoryTypeBits&(1u<<i)) &&
            (properties.memoryTypes[i].propertyFlags&(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))==
            (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)){type=i;break;}
        require(type<properties.memoryTypeCount,"no coherent water capture memory");
        VkMemoryAllocateInfo memory{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};memory.allocationSize=need.size;memory.memoryTypeIndex=type;
        check(vkAllocateMemory(device,&memory,nullptr,&out.memory));check(vkBindBufferMemory(device,out.value,out.memory,0));
        check(vkMapMemory(device,out.memory,0,bytes,0,&out.mapped));buffers.push_back(out);return out;
    }
    VkShaderModule module(const fs::path& path) {
        const auto code=words(path);VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        info.codeSize=code.size()*4;info.pCode=code.data();VkShaderModule out{};
        check(vkCreateShaderModule(device,&info,nullptr,&out));modules.push_back(out);return out;
    }
    VkPipeline pipeline(unsigned kernel) {
        if(const auto found=pipelines.find(kernel);found!=pipelines.end())return found->second;
        const auto& entry=generated_native_world_kernels::entries.at(kernel);
        const VkShaderModule vertex=module(shaders/std::string(entry.vertex_path));
        VkPipelineShaderStageCreateInfo stages[2]{};
        for(auto& stage:stages){stage.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;stage.pName="main";}
        stages[0].stage=VK_SHADER_STAGE_VERTEX_BIT;stages[0].module=vertex;
        stages[1].stage=VK_SHADER_STAGE_GEOMETRY_BIT;stages[1].module=capture;
        const VkVertexInputBindingDescription binding{0,sizeof(Vertex),VK_VERTEX_INPUT_RATE_VERTEX};
        const VkVertexInputAttributeDescription attributes[]{
            {0,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex,position)},
            {1,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex,color)},
            {2,0,VK_FORMAT_R32G32_SFLOAT,offsetof(Vertex,uv)},
            {3,0,VK_FORMAT_R32G32_SFLOAT,offsetof(Vertex,lightmap_uv)},
            {4,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex,normal)},
            {5,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex,tangent)},
            {6,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex,binormal)},
            {7,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex,incident)},
            {8,0,VK_FORMAT_R32_UINT,offsetof(Vertex,lightmap)},
            {11,0,VK_FORMAT_R32_UINT,offsetof(Vertex,source_stipple_level)},
            {12,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex,source_local_position)},
            {13,0,VK_FORMAT_R32_SFLOAT,offsetof(Vertex,source_color_alpha)},
            {9,0,VK_FORMAT_R32_SFLOAT,offsetof(Vertex,prt_cluster)},
            {10,0,VK_FORMAT_R32G32B32A32_SFLOAT,offsetof(Vertex,prt_weights)}};
        VkPipelineVertexInputStateCreateInfo input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        input.vertexBindingDescriptionCount=1;input.pVertexBindingDescriptions=&binding;
        input.vertexAttributeDescriptionCount=entry.extended_vertex_bank?14:12;input.pVertexAttributeDescriptions=attributes;
        VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};assembly.topology=VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        const VkViewport viewport{0,0,640,480,0,1};const VkRect2D scissor{{0,0},{640,480}};
        VkPipelineViewportStateCreateInfo view{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};view.viewportCount=view.scissorCount=1;
        view.pViewports=&viewport;view.pScissors=&scissor;
        VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        raster.rasterizerDiscardEnable=VK_TRUE;raster.polygonMode=VK_POLYGON_MODE_FILL;raster.lineWidth=1;
        VkPipelineMultisampleStateCreateInfo samples{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};samples.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
        VkGraphicsPipelineCreateInfo info{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};info.stageCount=2;info.pStages=stages;
        info.pVertexInputState=&input;info.pInputAssemblyState=&assembly;info.pViewportState=&view;
        info.pRasterizationState=&raster;info.pMultisampleState=&samples;info.layout=layout;info.renderPass=pass;
        VkPipeline out{};check(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&info,nullptr,&out));pipelines.emplace(kernel,out);return out;
    }
public:
    std::string device_name;
    explicit Gpu(fs::path shader_root,const fs::path& geometry):shaders(std::move(shader_root)) {
        VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.apiVersion=VK_API_VERSION_1_3;
        VkInstanceCreateInfo create{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};create.pApplicationInfo=&app;
        check(vkCreateInstance(&create,nullptr,&instance));unsigned count{};check(vkEnumeratePhysicalDevices(instance,&count,nullptr));
        std::vector<VkPhysicalDevice> devices(count);check(vkEnumeratePhysicalDevices(instance,&count,devices.data()));unsigned family{};
        for(auto candidate:devices) {
            VkPhysicalDeviceProperties candidate_properties;vkGetPhysicalDeviceProperties(candidate,&candidate_properties);
            if(candidate_properties.deviceType==VK_PHYSICAL_DEVICE_TYPE_CPU)continue;
            VkPhysicalDeviceFeatures features;vkGetPhysicalDeviceFeatures(candidate,&features);
            if(!features.geometryShader || !features.vertexPipelineStoresAndAtomics)continue;
            vkGetPhysicalDeviceQueueFamilyProperties(candidate,&count,nullptr);std::vector<VkQueueFamilyProperties> families(count);
            vkGetPhysicalDeviceQueueFamilyProperties(candidate,&count,families.data());
            for(unsigned i=0;i<count;++i)if(families[i].queueFlags&VK_QUEUE_GRAPHICS_BIT){physical=candidate;family=i;break;}
            if(physical)break;
        }
        require(physical,"water capture needs a graphics/geometry-store GPU");
        VkPhysicalDeviceProperties properties;vkGetPhysicalDeviceProperties(physical,&properties);device_name=properties.deviceName;
        const float priority=1;VkDeviceQueueCreateInfo q{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};q.queueFamilyIndex=family;q.queueCount=1;q.pQueuePriorities=&priority;
        VkPhysicalDeviceFeatures features{};features.geometryShader=features.vertexPipelineStoresAndAtomics=VK_TRUE;
        VkDeviceCreateInfo info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};info.queueCreateInfoCount=1;info.pQueueCreateInfos=&q;info.pEnabledFeatures=&features;
        check(vkCreateDevice(physical,&info,nullptr,&device));vkGetDeviceQueue(device,family,0,&queue);
        VkCommandPoolCreateInfo cp{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};cp.queueFamilyIndex=family;cp.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        check(vkCreateCommandPool(device,&cp,nullptr,&commands));VkCommandBufferAllocateInfo ca{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        ca.commandPool=commands;ca.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ca.commandBufferCount=1;check(vkAllocateCommandBuffers(device,&ca,&command));
        const std::array<VkDescriptorSetLayoutBinding,2> material{{{4,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_VERTEX_BIT,nullptr},
            {7,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_VERTEX_BIT,nullptr}}};
        VkDescriptorSetLayoutCreateInfo set{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};set.bindingCount=2;set.pBindings=material.data();
        check(vkCreateDescriptorSetLayout(device,&set,nullptr,&sets[0]));
        const VkDescriptorSetLayoutBinding output{0,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_GEOMETRY_BIT,nullptr};set.bindingCount=1;set.pBindings=&output;
        check(vkCreateDescriptorSetLayout(device,&set,nullptr,&sets[1]));
        const VkPushConstantRange push{VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(Push)};
        VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};pl.setLayoutCount=2;pl.pSetLayouts=sets.data();pl.pushConstantRangeCount=1;pl.pPushConstantRanges=&push;
        check(vkCreatePipelineLayout(device,&pl,nullptr,&layout));
        const VkDescriptorPoolSize sizes[]{{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,512},{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,256}};
        VkDescriptorPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pool_info.maxSets=512;pool_info.poolSizeCount=2;pool_info.pPoolSizes=sizes;
        check(vkCreateDescriptorPool(device,&pool_info,nullptr,&pool));
        VkSubpassDescription subpass{};subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;
        VkRenderPassCreateInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};rp.subpassCount=1;rp.pSubpasses=&subpass;check(vkCreateRenderPass(device,&rp,nullptr,&pass));
        VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};fb.renderPass=pass;fb.width=640;fb.height=480;fb.layers=1;check(vkCreateFramebuffer(device,&fb,nullptr,&frame));
        capture=module(geometry);
    }
    ~Gpu() {
        if(device){vkDeviceWaitIdle(device);for(auto [key,p]:pipelines)vkDestroyPipeline(device,p,nullptr);
            for(auto m:modules)vkDestroyShaderModule(device,m,nullptr);if(frame)vkDestroyFramebuffer(device,frame,nullptr);if(pass)vkDestroyRenderPass(device,pass,nullptr);
            if(layout)vkDestroyPipelineLayout(device,layout,nullptr);if(pool)vkDestroyDescriptorPool(device,pool,nullptr);
            for(auto s:sets)if(s)vkDestroyDescriptorSetLayout(device,s,nullptr);
            for(auto b:buffers){if(b.mapped)vkUnmapMemory(device,b.memory);if(b.value)vkDestroyBuffer(device,b.value,nullptr);if(b.memory)vkFreeMemory(device,b.memory,nullptr);}
            if(commands)vkDestroyCommandPool(device,commands,nullptr);vkDestroyDevice(device,nullptr);}
        if(instance)vkDestroyInstance(instance,nullptr);
    }
    std::vector<ShaderVector> run(unsigned kernel,const WorldMaterialUniform& material,
        const PackedVertexProgramBank& bank,const Push& push,std::span<const Vertex> vertices) {
        require(!vertices.empty(),"empty native water vertex case");
        const auto vb=allocate(vertices.size_bytes(),VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        const auto ub=allocate(sizeof(material),VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT),eb=allocate(sizeof(bank),VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
        const auto output=allocate(vertices.size()*8*sizeof(ShaderVector),VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        std::memcpy(vb.mapped,vertices.data(),vertices.size_bytes());std::memcpy(ub.mapped,&material,sizeof(material));std::memcpy(eb.mapped,&bank,sizeof(bank));
        std::memset(output.mapped,0xcd,output.bytes);
        std::array<VkDescriptorSet,2> ds{};VkDescriptorSetAllocateInfo da{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};da.descriptorPool=pool;da.descriptorSetCount=2;da.pSetLayouts=sets.data();
        check(vkAllocateDescriptorSets(device,&da,ds.data()));const VkDescriptorBufferInfo descriptors[]{{ub.value,0,ub.bytes},{eb.value,0,eb.bytes},{output.value,0,output.bytes}};
        std::array<VkWriteDescriptorSet,3> writes{};for(unsigned i=0;i<3;++i){auto& w=writes[i];w.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            w.dstSet=ds[i==2?1:0];w.dstBinding=i==0?4:i==1?7:0;w.descriptorCount=1;w.descriptorType=i==2?VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;w.pBufferInfo=&descriptors[i];}
        vkUpdateDescriptorSets(device,writes.size(),writes.data(),0,nullptr);
        const auto p=pipeline(kernel);check(vkResetCommandBuffer(command,0));VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};check(vkBeginCommandBuffer(command,&begin));
        VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER};host.srcAccessMask=VK_ACCESS_HOST_WRITE_BIT;host.dstAccessMask=VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT|VK_ACCESS_UNIFORM_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT;
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_HOST_BIT,VK_PIPELINE_STAGE_VERTEX_INPUT_BIT|VK_PIPELINE_STAGE_VERTEX_SHADER_BIT|VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT,0,1,&host,0,nullptr,0,nullptr);
        VkRenderPassBeginInfo render{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};render.renderPass=pass;render.framebuffer=frame;render.renderArea={{0,0},{640,480}};
        vkCmdBeginRenderPass(command,&render,VK_SUBPASS_CONTENTS_INLINE);vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,p);
        vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,layout,0,ds.size(),ds.data(),0,nullptr);
        vkCmdPushConstants(command,layout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(push),&push);const VkDeviceSize offset{};
        vkCmdBindVertexBuffers(command,0,1,&vb.value,&offset);vkCmdDraw(command,vertices.size(),1,0,0);vkCmdEndRenderPass(command);
        VkMemoryBarrier readback{VK_STRUCTURE_TYPE_MEMORY_BARRIER};readback.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;readback.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&readback,0,nullptr,0,nullptr);
        check(vkEndCommandBuffer(command));VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;
        check(vkQueueSubmit(queue,1,&submit,VK_NULL_HANDLE));check(vkQueueWaitIdle(queue));
        std::vector<ShaderVector> result(vertices.size()*8);std::memcpy(result.data(),output.mapped,output.bytes);return result;
    }
};

unsigned kernel_for(const Material& material) {
    auto controls=material.forward_program.words;std::fill(controls.begin()+10,controls.begin()+26,0);controls[43]=controls[44]=0;
    const auto declaration=pack_source_vertex_declaration(material.vertex_declaration,material.world_vertex_input_transform);
    for(unsigned i=0;i<generated_native_world_kernels::entries.size();++i) {
        const auto& entry=generated_native_world_kernels::entries[i];const auto raw=std::as_bytes(entry.vertex_program);
        if(entry.controls==controls && entry.declaration==declaration && entry.extended_vertex_bank==bool(material.vertex_bank) &&
            raw.size()==material.vertex_program.size() && std::equal(raw.begin(),raw.end(),material.vertex_program.begin()))return i;
    }
    throw FormatError("actual water material lacks complete native identity: "+material.name);
}
Material bind(const Cache& cache,const Material& authored) {
    Material result;const auto attempt=[&](const auto& prepare){auto candidate=authored;if(!prepare(candidate))return false;result=std::move(candidate);return true;};
    require(attempt([&](auto& m){return prepare_water_material(cache,m,false);}) || attempt([&](auto& m){return prepare_water_material(cache,m,true);}) ||
        attempt([&](auto& m){return prepare_world_material(cache,m,true);}) || attempt([&](auto& m){return prepare_world_material(cache,m,false);}),"unbound original water program: "+authored.name);
    return result;
}
std::array<ShaderVector,8> expected(const Material& material,const WorldMaterialUniform& uniform,
    const PackedVertexProgramBank& bank,const Push& push,const Vertex& vertex) {
    VertexProgramState cpu;
    for(unsigned i=0;i<7;++i)cpu.constants[i]=uniform.source_camera_world[i];
    cpu.constants[7]=push.eye;cpu.constants[7][3]=767.8125f;
    const auto viewport=viewport_constants({push.viewport[0],push.viewport[1],push.viewport[2],push.viewport[3]});
    for(unsigned i=0;i<4;++i)cpu.constants[8+i]=viewport[i];
    for(unsigned i=0;i<6;++i)cpu.constants[12+i]=uniform.fog_constants[i];
    for(unsigned i=0;i<16;++i)cpu.constants[18+i]=uniform.vertex_constants[i];
    cpu.constants[172]={1,1,0,0};
    if(material.vertex_bank) {
        for(unsigned i=0;i<bank.range[1];++i) {
            const unsigned index=bank.range[0]+i;
            if(index>=34 && index<192 && index!=172)cpu.constants[index]=bank.constants[i];
        }
        if(bank.range[3])for(unsigned i=0;i<12;++i)cpu.constants[i]=bank.constants[i];
    }
    if(material.vertex_declaration)cpu.attributes=source_world_vertex_inputs(*material.vertex_declaration,vertex,uniform.vertex_attributes,material.world_vertex_input_transform);
    else {
        cpu.attributes=uniform.vertex_attributes;
        cpu.attributes[0]={vertex.position.x,vertex.position.y,vertex.position.z,1};cpu.attributes[1]={vertex.uv.x,vertex.uv.y,0,1};
        for(auto [reg,v]:{std::pair{2u,vertex.normal},std::pair{3u,vertex.tangent},std::pair{4u,vertex.binormal},std::pair{5u,vertex.incident}})cpu.attributes[reg]={v.x,v.y,v.z,1};
        cpu.attributes[6]={vertex.lightmap_uv.x,vertex.lightmap_uv.y,0,1};
        if(bank.range[2]){cpu.attributes[5]={vertex.prt_cluster,0,0,1};cpu.attributes[6]=vertex.prt_weights;}
        if(material.forward_vertex==4)cpu.attributes[6]={vertex.color.x,vertex.color.y,vertex.color.z,1};
    }
    if(material.forward_vertex==12)cpu.attributes[11]={vertex.color.x,vertex.color.y,vertex.color.z,vertex.source_color_alpha};
    execute_vertex_program(decode_vertex_program(material.vertex_program),cpu);
    if(material.world_vertex_input_transform==SourceWorldVertexInputTransform::OffsetLocalPosition)
        for(unsigned stage=0;stage<2;++stage)for(unsigned axis=0;axis<3;++axis) {
            const auto& row=cpu.constants[18+stage*3+axis];
            const auto product=[](float a,float b){return a==0 || b==0?0.f:a*b;};
            const float xy=product(vertex.source_local_position.x,row[0])+product(vertex.source_local_position.y,row[1]);
            cpu.outputs[9+stage][axis]=(xy+product(vertex.source_local_position.z,row[2]))+row[3];
        }
    auto screen=cpu.outputs[0];screen[0]=std::trunc(screen[0]*16.f)/16.f;screen[1]=std::trunc(screen[1]*16.f)/16.f;
    screen[3]=(screen[3]<0?-1.f:1.f)*std::clamp(std::abs(screen[3]),5.421010862427522e-20f,1.8446744073709552e19f);
    std::array<ShaderVector,8> out{};out[0]={((screen[0]-push.viewport[0])*2.f/push.viewport[2]-1.f)*screen[3],
        ((screen[1]-push.viewport[1])*2.f/push.viewport[3]-1.f)*screen[3],(screen[2]/16777215.f)*screen[3],screen[3]};
    for(unsigned axis=0;axis<4;++axis){out[1][axis]=std::clamp(cpu.outputs[3][axis],0.f,1.f);out[2][axis]=std::clamp(cpu.outputs[4][axis],0.f,1.f);}
    for(unsigned stage=0;stage<4;++stage)out[3+stage]=cpu.outputs[9+stage];out[7]=screen;return out;
}
}

int main(int argc,char** argv)try {
    require(argc>=7,"usage: water_material_gpu_tests SHADERS CAPTURE_GEOM TU15_XBE OUTPUT_JSON MAP...");
    if(std::string_view(argv[3]).empty())return 77;
    const auto original=OriginalData::load_xbe(argv[3]);require(original->image==OriginalData::Image::TitleUpdate15,"water GPU cohort needs authenticated TU1.5");
    const char* negative=std::getenv("HALO2_WATER_VERTEX_WRONG_KERNEL");
    require(!negative || std::string_view(negative)=="0" || std::string_view(negative)=="1","invalid water GPU wrong-kernel control");
    const bool wrong_kernel=negative && std::string_view(negative)=="1";
    Gpu gpu(argv[1],argv[2]);json report={{"schema","H2_NATIVE_WATER_VERTEX_GPU_V1"},{"device",gpu.device_name},
        {"scope","Actual complete water programs through unchanged production native vertex SPIR-V, test-only GS capture and offline CPU arithmetic reference"},
        {"original_framebuffer_parity",false},{"physical_nv2a_precision",false},{"fragment_filter_reflection_scene_proof",false},
        {"runtime_vertex_instruction_interpreter",false},{"materials",json::array()}};
    unsigned materials{},cases{},checks{},finite{},nonfinite{},bit_exact{},signed_zeros{};double maximum_absolute{},maximum_relative{};
    std::array<double,8> absolute_by_output{},relative_by_output{};
    std::array<std::array<double,4>,8> absolute_by_axis{},relative_by_axis{};
    for(int argument=5;argument<argc;++argument) {
        const fs::path path(argv[argument]);Cache cache(path,path.parent_path(),original);
        require(!cache.bsps().empty(),"water input map has no BSP");
        // These are authored cache vertices, sampled as arithmetic inputs.
        // Matching material ownership is reported independently below.
        std::vector<Mesh> meshes;std::vector<BspWater> water;
        for(unsigned bsp=0;bsp<cache.bsps().size();++bsp){meshes.push_back(cache.load_bsp(bsp,false));water.push_back(cache.load_bsp_water(bsp));}
        for(const auto& tag:cache.tags()) {
            if(tag.group!="shad" || !tag.address || tag.size<8)continue;
            const auto* stem=cache.find_tag({Bytes(cache.tag_payload(tag.handle())).u32(4)},TagGroup{"stem"});
            if(!stem || (!stem->name.starts_with("shaders\\shader_templates\\water\\") && stem->name!="shaders\\shader_templates\\transparent\\waves"))continue;
            auto material=bind(cache,cache.load_material(tag.datum));const auto kernel=kernel_for(material);
            unsigned selected_bsp{};int definition=-1;bool authored_owner=false,infinite=false;
            std::vector<Vertex> vertices;
            for(unsigned bsp=0;bsp<water.size() && vertices.empty();++bsp)for(unsigned d=0;d<water[bsp].definitions.size();++d) {
                const auto& source=water[bsp].definitions[d];if(source.shader!=tag.datum || source.geometry.vertices.empty())continue;
                selected_bsp=bsp;definition=int(d);infinite=true;authored_owner=true;
                const auto stride=std::max<std::size_t>(1,source.geometry.vertices.size()/16);
                for(std::size_t i=0;i<source.geometry.vertices.size() && vertices.size()<16;i+=stride)vertices.push_back(source.geometry.vertices[i]);break;
            }
            if(vertices.empty())for(unsigned bsp=0;bsp<meshes.size() && vertices.empty();++bsp) {
                const auto& info=cache.bsps()[bsp];const Bytes data(Bytes(cache.source_runtime_cache_bytes()).slice(info.file_offset,info.size));
                const auto slots=TagAddressSpace(data,info.memory_address).block(16+164,32);
                for(const auto& v:meshes[bsp].vertices)if(v.material<slots.count && data.u32(slots.offset+v.material*32+12)==tag.datum) {
                    selected_bsp=bsp;authored_owner=true;vertices.push_back(v);if(vertices.size()==16)break;
                }
            }
            if(vertices.empty()) {
                require(!meshes.front().vertices.empty(),"water host-case input has no authored vertex bank");
                const auto& source=meshes.front().vertices;const auto stride=std::max<std::size_t>(1,source.size()/16);
                for(std::size_t i=0;i<source.size() && vertices.size()<16;i+=stride)vertices.push_back(source[i]);
            }
            if(material.water_pass && material.forward_vertex==7 && !water[selected_bsp].definitions.empty())definition=0;
            const auto& mesh=meshes[selected_bsp];Vec3 eye=mesh.maximum;std::string eye_owner="authored-BSP-bounds-maximum";
            for(const auto& spawn:cache.spawns())if(spawn.bsp==selected_bsp){eye=spawn.position;eye_owner="authored-player-start";break;}
            auto forward=normalized((mesh.minimum+mesh.maximum)*.5f-eye);if(length(forward)<.001f)forward={1,0,0};
            const Vec3 up=std::abs(forward.z)>.99f?Vec3{0,1,0}:Vec3{0,0,1};
            json row={{"map",path.filename().string()},{"name",material.name},{"datum",tag.datum},{"native_kernel",kernel},
                {"authored_material_vertex_owner",authored_owner},{"input_scope",authored_owner?"actual material-owned vertices":"declared host cases using another authored BSP vertex stream"},
                {"vertices",vertices.size()},{"bsp",selected_bsp},{"water_definition",definition},{"camera_eye_owner",eye_owner},
                {"camera_eye",{eye.x,eye.y,eye.z}},{"camera_forward",{forward.x,forward.y,forward.z}},{"cases",json::array()}};
            for(const auto extent:std::array{std::array{640,480},std::array{1280,720}}) {
                constexpr float angle=1.2217304764f;SourceFrustumProjectionInput input;input.projection_angle=angle;
                input.outer={0,0,std::int16_t(extent[1]),std::int16_t(extent[0])};input.safe=input.outer;input.scenario_present=true;input.scenario_type=cache.load_source_geometry_scenario_type();
                const auto camera=source_view_projection({eye,forward,up,.015625f,1024.f,{},source_frustum_projection(input).projection_program});
                const auto constants=source_view_shader_bank(camera,1024.f,eye,{0,0,float(extent[0]),float(extent[1])},false);
                Push push;std::copy_n(constants.begin(),4,push.matrix.begin());push.forward={forward.x,forward.y,forward.z,1};push.eye={eye.x,eye.y,eye.z,0};push.viewport={0,0,float(extent[0]),float(extent[1])};
                WorldMaterialUniform uniform;uniform.program=material.forward_program.words;uniform.flags={0,1,material.forward_vertex,decode_vertex_program(material.vertex_program).count};
                uniform.vertex_attributes=material.program_constants[0];uniform.vertex_constants=material.program_constants[1];uniform.vertex_declaration=pack_source_vertex_declaration(material.vertex_declaration,material.world_vertex_input_transform);
                uniform.source_camera_control[0]=1;std::copy_n(constants.begin(),7,uniform.source_camera_world.begin());
                auto submitted=vertices;
                if(definition>=0) {
                    const auto state=water_frame(water[selected_bsp].definitions.at(unsigned(definition)),eye,forward,angle,float(extent[0])/extent[1]);
                    std::copy(state.constants.begin(),state.constants.end(),uniform.vertex_constants.begin()+3);uniform.bump_matrices=state.bump_matrices;
                    if(infinite)for(auto& v:submitted)v.position=water_world_position(state,v.position);
                }
                const auto bank=material.vertex_bank?pack_vertex_program_bank(*material.vertex_bank):PackedVertexProgramBank{};
                for(const auto& instruction:uniform.vertex_instructions)for(auto word:instruction)require(word==0,"native input retained runtime instructions");
                auto selected_kernel=kernel;
                if(wrong_kernel) {
                    const auto& entries=generated_native_world_kernels::entries;
                    for(unsigned i=0;i<entries.size();++i)if(entries[i].vertex_path!=entries[kernel].vertex_path){selected_kernel=i;break;}
                    require(selected_kernel!=kernel,"wrong-kernel control has no distinct production vertex module");
                }
                const auto actual=gpu.run(selected_kernel,uniform,bank,push,submitted),repeat=gpu.run(selected_kernel,uniform,bank,push,submitted);
                require(actual.size()==repeat.size() && std::memcmp(actual.data(),repeat.data(),actual.size()*sizeof(ShaderVector))==0,"native water capture changed on identical repeat");
                bool varying=false;
                for(unsigned vertex=1;vertex<submitted.size();++vertex)for(unsigned output=0;output<8;++output)
                    varying|=actual[vertex*8+output]!=actual[output];
                require(varying,"water capture has no vertex-dependent output: "+material.name);
                double local_absolute{},local_relative{};
                for(unsigned vertex=0;vertex<submitted.size();++vertex) {
                    const auto reference=expected(material,uniform,bank,push,submitted[vertex]);
                    for(unsigned output=0;output<8;++output)for(unsigned axis=0;axis<4;++axis) {
                        const float a=actual[vertex*8+output][axis],b=reference[output][axis];++checks;
                        bit_exact+=std::bit_cast<std::uint32_t>(a)==std::bit_cast<std::uint32_t>(b);
                        signed_zeros+=a==0.f && b==0.f && std::signbit(a)!=std::signbit(b);
                        if(!std::isfinite(a) || !std::isfinite(b)) {
                            require((std::isnan(a)&&std::isnan(b)) || (std::isinf(a)&&std::isinf(b)&&std::signbit(a)==std::signbit(b)),"native water nonfinite classification differs: "+material.name);++nonfinite;continue;
                        }
                        ++finite;const double absolute=std::abs(double(a)-b),relative=absolute/std::max(1.,std::abs(double(b)));
                        local_absolute=std::max(local_absolute,absolute);local_relative=std::max(local_relative,relative);
                        absolute_by_output[output]=std::max(absolute_by_output[output],absolute);
                        relative_by_output[output]=std::max(relative_by_output[output],relative);
                        absolute_by_axis[output][axis]=std::max(absolute_by_axis[output][axis],absolute);
                        relative_by_axis[output][axis]=std::max(relative_by_axis[output][axis],relative);
                        // A CPU/GPU result on opposite sides of trunc's 1/16
                        // pixel boundary can differ by one source quantum.
                        // Propagate the declared window allowance into clip XY.
                        const double quantization=output==7 && axis<2?.125:
                            output==0 && axis<2?.125*2./push.viewport[axis+2]*std::abs(double(reference[0][3])):0.;
                        const double allowance=1e-4+quantization+std::abs(double(b))*2e-5;
                        require(absolute<=allowance,"native water VS output differs: "+material.name+" vertex="+std::to_string(vertex)+" output="+std::to_string(output)+" axis="+std::to_string(axis)+" actual="+std::to_string(a)+" expected="+std::to_string(b));
                    }
                }
                maximum_absolute=std::max(maximum_absolute,local_absolute);maximum_relative=std::max(maximum_relative,local_relative);
                row["cases"].push_back({{"viewport",extent},{"declared_host_vertical_fov",angle},{"maximum_absolute_error",local_absolute},
                    {"maximum_relative_error",local_relative},{"repeat_exact",true},{"uploaded_vertex_instruction_words",0}});++cases;
            }
            report["materials"].push_back(std::move(row));++materials;
        }
    }
    require(materials==13 && cases==26 && finite>0,"native water GPU cohort lost one of thirteen original material programs");
    report.update({{"passed",true},{"material_count",materials},{"case_count",cases},{"scalar_checks",checks},{"finite_scalar_checks",finite},
        {"matching_nonfinite_scalars",nonfinite},{"maximum_absolute_error",maximum_absolute},{"maximum_relative_error",maximum_relative},
        {"bit_exact_scalar_checks",bit_exact},{"signed_zero_differences",signed_zeros},
        {"output_labels",{"clip_position","clamped_oD0","clamped_oD1","oT0","oT1","oT2","oT3","quantized_source_window"}},
        {"maximum_absolute_error_by_output",absolute_by_output},{"maximum_relative_error_by_output",relative_by_output},
        {"maximum_absolute_error_by_output_xyzw",absolute_by_axis},{"maximum_relative_error_by_output_xyzw",relative_by_axis},
        {"absolute_tolerance",1e-4},{"relative_tolerance",2e-5},{"quantized_window_xy_absolute_tolerance",.125},
        {"clip_xy_tolerance_includes_window_allowance_times_two_over_extent_times_abs_w",true}});
    std::ofstream(argv[4])<<report.dump(2)<<'\n';
    std::cout<<materials<<" complete native water material programs, "<<cases<<" viewport cases, "<<checks<<" scalar comparisons; numerical component only\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
