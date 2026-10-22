#include "halo2/menu_world_vertex_catalog.hpp"
#include "../depth_storage/gpu.hpp"
#include <array>
#include <bit>
#include <cstring>
#include <cstdlib>
#include <nlohmann/json.hpp>
using nlohmann::json;
namespace {
constexpr unsigned vertices=8;
const std::vector<unsigned> ordered{0,1,2,1,3,2,2,3,4,3,5,4,4,5,6,5,7,6,0,0,1,1,2,2,7,3,0,7,0,4};
std::vector<std::uint32_t> read_words(const std::filesystem::path &path){std::ifstream file(path,std::ios::binary|std::ios::ate);require(file&&file.tellg()>0&&file.tellg()%4==0,"Missing reference words: "+path.string());std::vector<std::uint32_t> data(std::size_t(file.tellg())/4);file.seekg(0);require(bool(file.read(reinterpret_cast<char*>(data.data()),data.size()*4)),"Truncated reference words");return data;}
std::uint64_t digest(std::span<const std::byte> bytes){std::uint64_t h=14695981039346656037ull;for(auto b:bytes){h^=std::to_integer<unsigned char>(b);h*=1099511628211ull;}return h;}
struct Stream {VkFormat format{};unsigned offset{},stride{},code{};};
struct Input {std::vector<std::uint8_t> raw;std::vector<std::uint32_t> metadata;std::vector<Stream> streams;halo2::MenuWorldVertexAttributeUniforms uniform{};};
struct ScopedBuffers {
    Gpu &g;std::size_t first;
    explicit ScopedBuffers(Gpu &gpu):g(gpu),first(gpu.buffers.size()){}
    ~ScopedBuffers(){vkDeviceWaitIdle(g.device);while(g.buffers.size()>first){auto b=g.buffers.back();g.buffers.pop_back();if(b.mapped)vkUnmapMemory(g.device,b.memory);if(b.handle)vkDestroyBuffer(g.device,b.handle,nullptr);if(b.memory)vkFreeMemory(g.device,b.memory,nullptr);}}
};
void put(std::vector<std::uint8_t> &bytes,unsigned at,unsigned n,unsigned word){for(unsigned i=0;i<n;++i)bytes.at(at+i)=word>>(i*8);}
Input make_input(const json &entry,bool compressed){
    Input input;input.metadata.resize(64+ordered.size());std::copy(ordered.begin(),ordered.end(),input.metadata.begin()+64);
    const auto semantics=entry["semantics"].get<std::vector<unsigned>>();const unsigned classification=entry["class_index"];
    for(unsigned reg=0;reg<semantics.size();++reg){const auto semantic=semantics[reg];Stream stream;unsigned components=1,bytes=4;
        if(semantic==0){components=3;stream.code=compressed?2:0;bytes=compressed?2:4;stream.format=compressed?VK_FORMAT_R16G16B16_SNORM:VK_FORMAT_R32G32B32_SFLOAT;}
        else if(semantic==3){components=2;stream.code=compressed?3:1;bytes=compressed?2:4;stream.format=compressed?VK_FORMAT_R16G16_SNORM:VK_FORMAT_R32G32_SFLOAT;}
        else if(semantic==1||semantic==2){components=classification==2?1:classification==3?2:classification==4?2:classification==5?3:4;bytes=1;stream.code=4+components;const VkFormat formats[]{VK_FORMAT_R8_UNORM,VK_FORMAT_R8G8_UNORM,VK_FORMAT_R8G8B8_UNORM,VK_FORMAT_R8G8B8A8_UNORM};stream.format=formats[components-1];}
        else {require(semantic>=4&&semantic<=6,"Unknown declared semantic");stream.code=4;stream.format=VK_FORMAT_R32_SINT;}
        stream.stride=(components*bytes+3)&~3u;stream.offset=input.raw.size();input.raw.resize(input.raw.size()+vertices*stream.stride);
        const std::array<std::uint32_t,8> finite{0xbf400000,0xbe800000,0x3e000000,0x3f000000,0x3f400000,0x3f800000,0x40000000,0x3fc00000};
        for(unsigned vertex=0;vertex<vertices;++vertex)for(unsigned lane=0;lane<components;++lane){unsigned value;
            if(semantic==1)value=(vertex+lane)%4;
            else if(semantic==2)value=lane==0?192:lane==1?63:0;
            else if(semantic>=4)value=0x1ff803ffu^(vertex*0x0801001u)^(semantic*0x1234567u);
            else if(compressed)value=(vertex*7919u+lane*4337u)&65535u;
            else value=finite[(vertex+lane+reg)%finite.size()];
            put(input.raw,stream.offset+vertex*stream.stride+lane*bytes,bytes,value);
        }
        const auto at=reg*4;input.metadata[at]=stream.offset;input.metadata[at+1]=stream.stride;input.metadata[at+2]=stream.code;input.streams.push_back(stream);
    }
    // Explicit arithmetic fixtures. These values select no runtime camera or
    // animation state. Palette indices remain within the admitted source bank.
    const std::array<float,8> values{0.0f,0.125f,0.25f,0.5f,0.75f,1.0f,-0.25f,-0.5f};
    for(unsigned i=0;i<192;++i)for(unsigned lane=0;lane<4;++lane)input.uniform.view.constants[i][lane]=values[(i+lane)%8];
    for(unsigned i=0;i<4;++i){input.uniform.view.constants[i]={0,0,0,0};input.uniform.view.constants[i][i]=1;}
    input.uniform.view.constants[7]={0,0,0,765};
    input.uniform.view.constants[8]={320,0,0,0};input.uniform.view.constants[9]={0,-240,0,0};input.uniform.view.constants[10]={320,-240,16777215,1};input.uniform.view.constants[11]={319.5f,239.5f,0,0};
    for(unsigned node=0;node<40;++node)for(unsigned row=0;row<3;++row){input.uniform.view.constants[50+node*3+row]={0,0,0,0};input.uniform.view.constants[50+node*3+row][row]=1;}
    input.uniform.view.constants[170]={1,0,0,0};input.uniform.view.constants[171]={0,1,0,0};input.uniform.view.constants[172]={0,0,1,0};input.uniform.view.clip_range={0.015625f,16777215,0,16777215};input.uniform.view.fog_parameters={1,0.5f};input.uniform.view.surface_size={640,480};
    for(unsigned i=0;i<16;++i)input.uniform.attributes[i]={0.25f,0.5f,0.75f,1};
    return input;
}
struct Runner {
    Gpu &g;bool geometry{};VkRenderPass pass{};VkFramebuffer frame{};std::array<VkDescriptorSetLayout,3> sets{};VkPipelineLayout layout{};VkDescriptorPool pool{};VkShaderModule capture{};
    explicit Runner(Gpu &gpu,bool use_geometry):g(gpu),geometry(use_geometry){}
    VkShaderModule module(const std::filesystem::path &path){auto words=read_words(path);VkShaderModuleCreateInfo sc{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};sc.codeSize=words.size()*4;sc.pCode=words.data();VkShaderModule module{};check(vkCreateShaderModule(g.device,&sc,nullptr,&module));return module;}
    void initialize(){
        for(unsigned i=0;i<3;++i){std::array<VkDescriptorSetLayoutBinding,3> bindings{};unsigned n=0;if(i==1){bindings[0]={0,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_VERTEX_BIT,nullptr};bindings[1]={2,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_VERTEX_BIT,nullptr};bindings[2]={3,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_VERTEX_BIT,nullptr};n=3;}else if(i==2){bindings[0]={0,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_VERTEX_BIT|(geometry?VK_SHADER_STAGE_GEOMETRY_BIT:0u),nullptr};n=1;}VkDescriptorSetLayoutCreateInfo dc{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};dc.bindingCount=n;dc.pBindings=bindings.data();check(vkCreateDescriptorSetLayout(g.device,&dc,nullptr,&sets[i]));}
        VkPipelineLayoutCreateInfo lc{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};lc.setLayoutCount=3;lc.pSetLayouts=sets.data();check(vkCreatePipelineLayout(g.device,&lc,nullptr,&layout));const VkDescriptorPoolSize sizes[]{{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,8},{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,24}};VkDescriptorPoolCreateInfo pc{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pc.maxSets=16;pc.poolSizeCount=2;pc.pPoolSizes=sizes;check(vkCreateDescriptorPool(g.device,&pc,nullptr,&pool));VkSubpassDescription sub{};sub.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;VkRenderPassCreateInfo rc{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};rc.subpassCount=1;rc.pSubpasses=&sub;check(vkCreateRenderPass(g.device,&rc,nullptr,&pass));VkFramebufferCreateInfo fc{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};fc.renderPass=pass;fc.width=640;fc.height=480;fc.layers=1;check(vkCreateFramebuffer(g.device,&fc,nullptr,&frame));if(geometry)capture=module(std::filesystem::path(DEPTH_SHADER_DIR)/"capture.geom.spv");
    }
    std::vector<std::uint32_t> run(const json &entry,const Input &input,bool triangle,bool native=false){
        ScopedBuffers owned(g);
        const auto count=native?vertices:ordered.size();
        const auto uniform=g.buffer(sizeof(input.uniform),VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT),raw=g.buffer(input.raw.size(),VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_VERTEX_BUFFER_BIT),metadata=g.buffer(input.metadata.size()*4,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT),indices=g.buffer(ordered.size()*2,VK_BUFFER_USAGE_INDEX_BUFFER_BIT),output=g.buffer(count*204,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        std::memcpy(uniform.mapped,&input.uniform,sizeof(input.uniform));std::memcpy(raw.mapped,input.raw.data(),input.raw.size());std::memcpy(metadata.mapped,input.metadata.data(),input.metadata.size()*4);for(unsigned i=0;i<ordered.size();++i)static_cast<std::uint16_t*>(indices.mapped)[i]=ordered[i];std::memset(output.mapped,0xcd,count*204);
        check(vkResetDescriptorPool(g.device,pool,0));std::array<VkDescriptorSet,2> descriptors{};VkDescriptorSetAllocateInfo da{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};da.descriptorPool=pool;da.descriptorSetCount=2;da.pSetLayouts=sets.data()+1;check(vkAllocateDescriptorSets(g.device,&da,descriptors.data()));std::array<VkDescriptorBufferInfo,4> infos{{{uniform.handle,0,entry["abi"]==3?sizeof(input.uniform):sizeof(input.uniform.view)},{raw.handle,0,input.raw.size()},{metadata.handle,0,input.metadata.size()*4},{output.handle,0,count*204}}};std::array<VkWriteDescriptorSet,4> writes{};const unsigned bindings[]{0,2,3,0};for(unsigned i=0;i<4;++i){auto &w=writes[i];w={VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};w.dstSet=descriptors[i==3?1:0];w.dstBinding=bindings[i];w.descriptorCount=1;w.descriptorType=i==0?VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;w.pBufferInfo=&infos[i];}vkUpdateDescriptorSets(g.device,4,writes.data(),0,nullptr);
        const auto vertex=module(std::filesystem::path(DEPTH_SHADER_DIR)/entry[native?"native":triangle?"triangle":"original"].get<std::string>());VkPipeline pipeline{};
        try{
            const VkPipelineShaderStageCreateInfo stages[]{{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_VERTEX_BIT,vertex,"main",nullptr},{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_GEOMETRY_BIT,capture,"main",nullptr}};
            std::vector<VkVertexInputBindingDescription> native_bindings;std::vector<VkVertexInputAttributeDescription> attributes;for(unsigned i=0;i<input.streams.size();++i){native_bindings.push_back({i,input.streams[i].stride,VK_VERTEX_INPUT_RATE_VERTEX});attributes.push_back({i,i,input.streams[i].format,0});}
            VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};vi.vertexBindingDescriptionCount=triangle?0:native_bindings.size();vi.pVertexBindingDescriptions=native_bindings.data();vi.vertexAttributeDescriptionCount=triangle?0:attributes.size();vi.pVertexAttributeDescriptions=attributes.data();VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};ia.topology=native?VK_PRIMITIVE_TOPOLOGY_POINT_LIST:VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;const VkViewport viewport{0,0,640,480,0,1};const VkRect2D scissor{{0,0},{640,480}};VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};vp.viewportCount=vp.scissorCount=1;vp.pViewports=&viewport;vp.pScissors=&scissor;VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};raster.rasterizerDiscardEnable=VK_TRUE;raster.polygonMode=VK_POLYGON_MODE_FILL;raster.lineWidth=1;VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};ms.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;VkGraphicsPipelineCreateInfo pc{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};pc.stageCount=(triangle||native)?1:2;pc.pStages=stages;pc.pVertexInputState=&vi;pc.pInputAssemblyState=&ia;pc.pViewportState=&vp;pc.pRasterizationState=&raster;pc.pMultisampleState=&ms;pc.layout=layout;pc.renderPass=pass;check(vkCreateGraphicsPipelines(g.device,{},1,&pc,nullptr,&pipeline));
            g.begin();VkMemoryBarrier memory{VK_STRUCTURE_TYPE_MEMORY_BARRIER};memory.srcAccessMask=VK_ACCESS_HOST_WRITE_BIT;memory.dstAccessMask=VK_ACCESS_UNIFORM_READ_BIT|VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT|VK_ACCESS_INDEX_READ_BIT;const auto shader_stages=VK_PIPELINE_STAGE_VERTEX_SHADER_BIT|(geometry?VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT:0u);vkCmdPipelineBarrier(g.command,VK_PIPELINE_STAGE_HOST_BIT,VK_PIPELINE_STAGE_VERTEX_INPUT_BIT|shader_stages,0,1,&memory,0,nullptr,0,nullptr);VkRenderPassBeginInfo bc{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};bc.renderPass=pass;bc.framebuffer=frame;bc.renderArea={{0,0},{640,480}};vkCmdBeginRenderPass(g.command,&bc,VK_SUBPASS_CONTENTS_INLINE);vkCmdBindPipeline(g.command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline);vkCmdBindDescriptorSets(g.command,VK_PIPELINE_BIND_POINT_GRAPHICS,layout,1,2,descriptors.data(),0,nullptr);
            if(triangle)vkCmdDraw(g.command,count,1,0,0);else{std::vector<VkBuffer> buffers(input.streams.size(),raw.handle);std::vector<VkDeviceSize> offsets;for(auto stream:input.streams)offsets.push_back(stream.offset);vkCmdBindVertexBuffers(g.command,0,buffers.size(),buffers.data(),offsets.data());if(native)vkCmdDraw(g.command,count,1,0,0);else{vkCmdBindIndexBuffer(g.command,indices.handle,0,VK_INDEX_TYPE_UINT16);vkCmdDrawIndexed(g.command,ordered.size(),1,0,0,0);}}vkCmdEndRenderPass(g.command);memory.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;memory.dstAccessMask=VK_ACCESS_HOST_READ_BIT;vkCmdPipelineBarrier(g.command,shader_stages,VK_PIPELINE_STAGE_HOST_BIT,0,1,&memory,0,nullptr,0,nullptr);check(vkEndCommandBuffer(g.command));VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&g.command;check(vkQueueSubmit(g.queue,1,&submit,{}));check(vkQueueWaitIdle(g.queue));
        }catch(...){vkDeviceWaitIdle(g.device);if(pipeline)vkDestroyPipeline(g.device,pipeline,nullptr);vkDestroyShaderModule(g.device,vertex,nullptr);throw;}
        vkDestroyPipeline(g.device,pipeline,nullptr);vkDestroyShaderModule(g.device,vertex,nullptr);std::vector<std::uint32_t> result(count*51);std::memcpy(result.data(),output.mapped,result.size()*4);return result;
    }
    ~Runner(){if(frame)vkDestroyFramebuffer(g.device,frame,nullptr);if(pass)vkDestroyRenderPass(g.device,pass,nullptr);if(pool)vkDestroyDescriptorPool(g.device,pool,nullptr);if(layout)vkDestroyPipelineLayout(g.device,layout,nullptr);for(auto set:sets)if(set)vkDestroyDescriptorSetLayout(g.device,set,nullptr);if(capture)vkDestroyShaderModule(g.device,capture,nullptr);}
};
// Independent same-device control: execute the unchanged original VS once per
// native hardware-fetched vertex, then assemble its outputs in a compute pass.
// Linux additionally compares this control with the original geometry shader.
// Mac cross-device arithmetic differences remain separately reported.
struct Assembler {
    Gpu &g;VkDescriptorSetLayout set{};VkPipelineLayout layout{};VkDescriptorPool pool{};VkPipeline pipeline{};VkShaderModule shader{};
    explicit Assembler(Gpu &gpu):g(gpu){}
    void initialize() {
        std::array<VkDescriptorSetLayoutBinding,3> bindings{};
        for(unsigned i=0;i<3;++i)bindings[i]={i,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr};
        VkDescriptorSetLayoutCreateInfo sc{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};sc.bindingCount=3;sc.pBindings=bindings.data();check(vkCreateDescriptorSetLayout(g.device,&sc,nullptr,&set));
        VkPipelineLayoutCreateInfo lc{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};lc.setLayoutCount=1;lc.pSetLayouts=&set;check(vkCreatePipelineLayout(g.device,&lc,nullptr,&layout));
        VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,3};VkDescriptorPoolCreateInfo pc{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pc.maxSets=1;pc.poolSizeCount=1;pc.pPoolSizes=&size;check(vkCreateDescriptorPool(g.device,&pc,nullptr,&pool));
        shader=g.shader("assemble.comp.spv");VkComputePipelineCreateInfo compute{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};compute.stage={VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_COMPUTE_BIT,shader,"main",nullptr};compute.layout=layout;check(vkCreateComputePipelines(g.device,{},1,&compute,nullptr,&pipeline));
    }
    std::vector<std::uint32_t> run(std::span<const std::uint32_t> native) {
        ScopedBuffers owned(g);
        const auto input=g.buffer(native.size_bytes(),VK_BUFFER_USAGE_STORAGE_BUFFER_BIT),indices=g.buffer(ordered.size()*4,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT),output=g.buffer(ordered.size()*204,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        std::memcpy(input.mapped,native.data(),native.size_bytes());std::memcpy(indices.mapped,ordered.data(),ordered.size()*4);std::memset(output.mapped,0xcd,ordered.size()*204);
        check(vkResetDescriptorPool(g.device,pool,0));VkDescriptorSet descriptor{};VkDescriptorSetAllocateInfo da{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};da.descriptorPool=pool;da.descriptorSetCount=1;da.pSetLayouts=&set;check(vkAllocateDescriptorSets(g.device,&da,&descriptor));
        std::array<VkDescriptorBufferInfo,3> infos{{{input.handle,0,native.size_bytes()},{indices.handle,0,ordered.size()*4},{output.handle,0,ordered.size()*204}}};std::array<VkWriteDescriptorSet,3> writes{};
        for(unsigned i=0;i<3;++i){writes[i]={VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};writes[i].dstSet=descriptor;writes[i].dstBinding=i;writes[i].descriptorCount=1;writes[i].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;writes[i].pBufferInfo=&infos[i];}vkUpdateDescriptorSets(g.device,3,writes.data(),0,nullptr);
        g.begin();VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};barrier.srcAccessMask=VK_ACCESS_HOST_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT;vkCmdPipelineBarrier(g.command,VK_PIPELINE_STAGE_HOST_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&barrier,0,nullptr,0,nullptr);
        vkCmdBindPipeline(g.command,VK_PIPELINE_BIND_POINT_COMPUTE,pipeline);vkCmdBindDescriptorSets(g.command,VK_PIPELINE_BIND_POINT_COMPUTE,layout,0,1,&descriptor,0,nullptr);vkCmdDispatch(g.command,ordered.size(),1,1);
        barrier.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_HOST_READ_BIT;vkCmdPipelineBarrier(g.command,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&barrier,0,nullptr,0,nullptr);check(vkEndCommandBuffer(g.command));VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&g.command;check(vkQueueSubmit(g.queue,1,&submit,{}));check(vkQueueWaitIdle(g.queue));
        std::vector<std::uint32_t> result(ordered.size()*51);std::memcpy(result.data(),output.mapped,result.size()*4);return result;
    }
    ~Assembler(){if(pipeline)vkDestroyPipeline(g.device,pipeline,nullptr);if(shader)vkDestroyShaderModule(g.device,shader,nullptr);if(pool)vkDestroyDescriptorPool(g.device,pool,nullptr);if(layout)vkDestroyPipelineLayout(g.device,layout,nullptr);if(set)vkDestroyDescriptorSetLayout(g.device,set,nullptr);}
};
json compare(std::span<const std::uint32_t> actual,std::span<const std::uint32_t> expected) {
    require(actual.size()==expected.size(),"World triangle output extent mismatch");
    unsigned differences=0;json first=json::array(),lanes=json::object();
    for(unsigned i=0;i<actual.size();++i)if(actual[i]!=expected[i]) {
        ++differences;const auto key=std::to_string(i%51);lanes[key]=lanes.value(key,0u)+1;
        if(first.size()<8)first.push_back({{"record",i/51},{"word",i%51},{"actual",actual[i]},{"expected",expected[i]}});
    }
    return {{"different_words",differences},{"first_differences",first},{"different_lanes",lanes}};
}
}
int main(int argc,char **argv){json report;report["schema"]="H2_WORLD_TRIANGLE_VERTEX_V2";report["cases"]=json::array();int status=0;try{require(argc==2||argc==3,"Expected report path and optional owned oracle directory");std::ifstream manifest_file(std::filesystem::path(DEPTH_SHADER_DIR)/"manifest.json");json manifest;manifest_file>>manifest;require(manifest["schema"]=="H2_WORLD_TRIANGLE_SHADER_CAPTURE_V1","Shader manifest identity");
#ifdef __APPLE__
    constexpr bool geometry=false;require(argc==3,"Mac comparison requires original-path GPU oracle files from the owned Linux run");
#else
    constexpr bool geometry=true;
#endif
    std::filesystem::path oracle=argc==3?argv[2]:"";if(geometry&&!oracle.empty())std::filesystem::create_directories(oracle);
    report["reference"]="unchanged original VS with native hardware fetch, followed by independent compute assembly";
    if(const auto policy=std::getenv("MVK_CONFIG_FAST_MATH_ENABLED"))report["moltenvk_fast_math_environment"]=policy;
    else report["moltenvk_fast_math_environment"]=nullptr;
    unsigned differences=0,assembly_differences=0,cross_device_differences=0;
    {
        Gpu g;g.initialize(false,geometry,true);report["device"]=g.properties.deviceName;report["geometry_shader_enabled"]=geometry;report["validation_layer_active"]=g.validation;report["synchronization_validation_active"]=g.synchronization_validation;report["strict_shader_math_instance"]=g.strict_shader_math;
        Runner runner(g,geometry);runner.initialize();Assembler assembler(g);assembler.initialize();unsigned entry_index=0;
        for(const auto &entry:manifest["entries"]) {
            for(bool compressed:{false,true}) {
                const auto input=make_input(entry,compressed);
                const auto result=runner.run(entry,input,true);
                const auto native=runner.run(entry,input,false,true);
                const auto expected=assembler.run(native);
                const auto local=compare(result,expected);differences+=local["different_words"].get<unsigned>();
                const auto key=std::to_string(entry_index)+"-"+(compressed?"compressed":"float");const auto path=oracle/(key+".bin");
                std::vector<std::uint32_t> original;
                json assembly=nullptr;
                if(geometry) {
                    original=runner.run(entry,input,false);
                    assembly=compare(expected,original);assembly_differences+=assembly["different_words"].get<unsigned>();
                    if(!oracle.empty()){std::ofstream file(path,std::ios::binary);file.write(reinterpret_cast<const char*>(original.data()),original.size()*4);require(bool(file),"Oracle write failed");}
                }else original=read_words(path);
                const auto cross=compare(result,original);cross_device_differences+=cross["different_words"].get<unsigned>();
                std::vector<std::byte> input_bytes(input.raw.size()+input.metadata.size()*4+sizeof(input.uniform));auto at=input_bytes.begin();at=std::copy(reinterpret_cast<const std::byte*>(input.raw.data()),reinterpret_cast<const std::byte*>(input.raw.data()+input.raw.size()),at);at=std::copy(reinterpret_cast<const std::byte*>(input.metadata.data()),reinterpret_cast<const std::byte*>(input.metadata.data()+input.metadata.size()),at);std::memcpy(&*at,&input.uniform,sizeof(input.uniform));
                report["cases"].push_back({{"entry",entry_index},{"class",entry["class_index"]},{"abi",entry["abi"]},{"compressed",compressed},{"program_sha256",entry["program_sha256"]},{"input_fnv64",digest(input_bytes)},{"output_fnv64",digest(std::as_bytes(std::span(result)))},{"expected_fnv64",digest(std::as_bytes(std::span(expected)))},{"original_linux_fnv64",digest(std::as_bytes(std::span(original)))},{"words",result.size()},{"same_device",local},{"assembly_vs_original_geometry",assembly},{"vs_original_linux",cross}});
            }
            ++entry_index;
        }
    }
    report["different_words"]=differences;report["assembly_different_words"]=assembly_differences;report["cross_device_different_words"]=cross_device_differences;report["cross_device_bit_exact"]=!cross_device_differences;
    require(report["cases"].size()==322,"Incomplete world program coverage");require(!differences,"Triangle vertex program differs from unchanged same-device native VS");require(!assembly_differences,"Independent compute assembly differs from original geometry shader");require(!validation_errors,"Vulkan validation errors");report["passed"]=true;
}catch(const std::exception &e){report["passed"]=false;report["error"]=e.what();std::cerr<<e.what()<<'\n';status=1;}report["validation_errors"]=validation_errors;if(argc>=2){std::ofstream file(argv[1]);file<<report.dump(2)<<'\n';if(!file)status=1;}std::cout<<"World triangle GPU cases: "<<report["cases"].size()<<", passed: "<<report["passed"]<<'\n';return status;}
