#include "halo2/source_weather_depth_alias.hpp"
#include "source_weather_plate_gpu_fixture.hpp"
#include <functional>
#include <limits>

using namespace halo2;
namespace {
using namespace halo2_weather_plate_fixture;
struct Input { VkExtent2D extent;std::vector<std::byte> depth,stencil,expected; };
std::vector<std::byte> read(const std::filesystem::path &path) {
    std::ifstream f(path,std::ios::binary|std::ios::ate);require(bool(f)&&f.tellg()>0,"weather depth alias source artifact missing");
    std::vector<std::byte> bytes(std::size_t(f.tellg()));f.seekg(0);require(bool(f.read(reinterpret_cast<char*>(bytes.data()),bytes.size())),"weather depth alias artifact incomplete");return bytes;
}
void save(const std::filesystem::path &path,std::span<const std::byte> bytes) {
    std::ofstream f(path,std::ios::binary);require(bool(f.write(reinterpret_cast<const char*>(bytes.data()),bytes.size())),"weather depth alias artifact write failed");
}
template<class F>void reject(F f,unsigned &count) {try{f();}catch(const FormatError&){++count;return;}throw FormatError("weather depth alias invalid input was admitted");}
Input original_input() {
    const auto base=std::filesystem::path(HALO2_SOURCE_DIR)/"analysis/multiplayer-shading/weather-plate-original-reference/event-215778";
    Input input{{640,480},read(base/"before-depth-d24-low24.bin"),{},read(base/"stage-0-mip-0.rgba")};
    const auto source=read(base/"before-scene-depth.raw");
    require(input.depth.size()==640*480*4&&source.size()==input.depth.size()&&input.expected.size()==source.size(),"weather depth alias original dimensions changed");
    input.stencil.resize(640*480);
    for(unsigned i=0;i<640*480;++i) {
        std::uint32_t original,native;std::memcpy(&original,source.data()+i*4,4);std::memcpy(&native,input.depth.data()+i*4,4);
        require(native==original>>8,"original integer depth staging changed");input.stencil[i]=std::byte(original&255);
        const auto *a=input.expected.data()+i*4,*s=source.data()+i*4;
        require(a[0]==s[2]&&a[1]==s[1]&&a[2]==s[0]&&a[3]==s[3],"original alias is not the exact D24S8 byte permutation");
    }
    return input;
}
Input control(VkExtent2D extent,unsigned seed) {
    Input input{extent,{},{},{}};const auto n=source_weather_depth_alias_storage(extent).pixels;
    input.depth.resize(n*4);input.stencil.resize(n);input.expected.resize(n*4);
    for(unsigned i=0;i<n;++i) {
        const unsigned a=(i+seed)&255,r=(i*17+seed)&255,g=(i*29+seed)&255,s=(i*37+seed)&255;
        // Deliberately nonzero high padding on upload. It is not depth.
        const std::uint32_t word=(a<<16)|(r<<8)|g|0x5a000000;
        std::memcpy(input.depth.data()+i*4,&word,4);input.stencil[i]=std::byte(s);
        input.expected[i*4]=std::byte(r);input.expected[i*4+1]=std::byte(g);input.expected[i*4+2]=std::byte(s);input.expected[i*4+3]=std::byte(a);
    }
    return input;
}
struct Scope { VkPipelineStageFlags stages;VkAccessFlags access; };
Scope scope(VkImageLayout layout) {
    switch(layout){
    case VK_IMAGE_LAYOUT_UNDEFINED:return {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,0};
    case VK_IMAGE_LAYOUT_GENERAL:return {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT};
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:return {VK_PIPELINE_STAGE_TRANSFER_BIT,VK_ACCESS_TRANSFER_READ_BIT};
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:return {VK_PIPELINE_STAGE_TRANSFER_BIT,VK_ACCESS_TRANSFER_WRITE_BIT};
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:return {VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_SHADER_READ_BIT};
    default:return {VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT};
    }
}
void barrier(Gpu &g,Image image,VkImageLayout from,VkImageLayout to) {
    const auto before=scope(from),after=scope(to);VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};b.image=image.handle;b.subresourceRange={VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT,0,1,0,1};b.oldLayout=from;b.newLayout=to;b.srcAccessMask=before.access;b.dstAccessMask=after.access;b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
    vkCmdPipelineBarrier(g.command,before.stages,after.stages,0,0,nullptr,0,nullptr,1,&b);
}
void upload(Gpu &g,Image image,const Input &input,VkImageLayout from,VkImageLayout to) {
    const auto size=source_weather_depth_alias_storage(input.extent);auto buffer=g.buffer(size.depth_bytes+size.stencil_bytes,VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    std::memcpy(buffer.mapped,input.depth.data(),input.depth.size());std::memcpy(static_cast<std::byte*>(buffer.mapped)+size.depth_bytes,input.stencil.data(),input.stencil.size());
    g.begin();barrier(g,image,from,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    VkBufferImageCopy regions[2]{};regions[0].imageSubresource={VK_IMAGE_ASPECT_DEPTH_BIT,0,0,1};regions[1].imageSubresource={VK_IMAGE_ASPECT_STENCIL_BIT,0,0,1};regions[1].bufferOffset=size.depth_bytes;
    for(auto &r:regions)r.imageExtent={input.extent.width,input.extent.height,1};vkCmdCopyBufferToImage(g.command,buffer.handle,image.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,2,regions);
    barrier(g,image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,to);g.end();
}
void verify_source(Gpu &g,Image image,const Input &input,VkImageLayout layout) {
    const auto size=source_weather_depth_alias_storage(input.extent);auto buffer=g.buffer(size.depth_bytes+size.stencil_bytes,VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    g.begin();barrier(g,image,layout,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    VkBufferImageCopy regions[2]{};regions[0].imageSubresource={VK_IMAGE_ASPECT_DEPTH_BIT,0,0,1};regions[1].imageSubresource={VK_IMAGE_ASPECT_STENCIL_BIT,0,0,1};regions[1].bufferOffset=size.depth_bytes;
    for(auto &r:regions)r.imageExtent={input.extent.width,input.extent.height,1};vkCmdCopyImageToBuffer(g.command,image.handle,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,buffer.handle,2,regions);
    barrier(g,image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,layout);
    VkBufferMemoryBarrier host{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};host.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;host.dstAccessMask=VK_ACCESS_HOST_READ_BIT;host.srcQueueFamilyIndex=host.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;host.buffer=buffer.handle;host.size=buffer.size;vkCmdPipelineBarrier(g.command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,0,nullptr,1,&host,0,nullptr);g.end();
    const auto *raw=static_cast<const std::byte*>(buffer.mapped);
    for(unsigned i=0;i<size.pixels;++i){std::uint32_t want,actual;std::memcpy(&want,input.depth.data()+i*4,4);std::memcpy(&actual,raw+i*4,4);require((actual&0xffffff)==(want&0xffffff),"weather depth alias producer changed source depth");require(raw[size.depth_bytes+i]==input.stencil[i],"weather depth alias producer changed source stencil");}
}
struct Fence {
    VkDevice device;VkFence value{};
    explicit Fence(Gpu &g):device(g.device){VkFenceCreateInfo ci{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};check(vkCreateFence(device,&ci,nullptr,&value));}
    ~Fence(){vkDestroyFence(device,value,nullptr);}
};
struct SampleProbe {
    Gpu &g;VkDescriptorSetLayout descriptors{};VkDescriptorPool pool{};VkDescriptorSet descriptor{};VkPipelineLayout layout{};VkShaderModule module{};VkPipeline pipeline{};
    explicit SampleProbe(Gpu &gpu):g(gpu) {
        const auto raw=read(std::filesystem::path(HALO2_WEATHER_DEPTH_ALIAS_SHADER_DIR)/"source_weather_depth_alias_sample.comp.spv");require(raw.size()%4==0,"sample probe SPIR-V incomplete");std::vector<std::uint32_t> words(raw.size()/4);std::memcpy(words.data(),raw.data(),raw.size());
        VkShaderModuleCreateInfo sm{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};sm.codeSize=raw.size();sm.pCode=words.data();check(vkCreateShaderModule(g.device,&sm,nullptr,&module));
        const VkDescriptorSetLayoutBinding bindings[]{{0,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},{1,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr}};
        VkDescriptorSetLayoutCreateInfo dc{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};dc.bindingCount=2;dc.pBindings=bindings;check(vkCreateDescriptorSetLayout(g.device,&dc,nullptr,&descriptors));
        const VkPushConstantRange push{VK_SHADER_STAGE_COMPUTE_BIT,0,8};VkPipelineLayoutCreateInfo lc{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};lc.setLayoutCount=1;lc.pSetLayouts=&descriptors;lc.pushConstantRangeCount=1;lc.pPushConstantRanges=&push;check(vkCreatePipelineLayout(g.device,&lc,nullptr,&layout));
        VkComputePipelineCreateInfo pc{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};pc.layout=layout;pc.stage={VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};pc.stage.stage=VK_SHADER_STAGE_COMPUTE_BIT;pc.stage.module=module;pc.stage.pName="main";check(vkCreateComputePipelines(g.device,{},1,&pc,nullptr,&pipeline));
        const VkDescriptorPoolSize sizes[]{{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1},{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1}};VkDescriptorPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pool_info.maxSets=1;pool_info.poolSizeCount=2;pool_info.pPoolSizes=sizes;check(vkCreateDescriptorPool(g.device,&pool_info,nullptr,&pool));VkDescriptorSetAllocateInfo da{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};da.descriptorPool=pool;da.descriptorSetCount=1;da.pSetLayouts=&descriptors;check(vkAllocateDescriptorSets(g.device,&da,&descriptor));
    }
    ~SampleProbe(){vkDestroyDescriptorPool(g.device,pool,nullptr);vkDestroyPipeline(g.device,pipeline,nullptr);vkDestroyPipelineLayout(g.device,layout,nullptr);vkDestroyDescriptorSetLayout(g.device,descriptors,nullptr);vkDestroyShaderModule(g.device,module,nullptr);}
    void record(SourceWeatherDepthAliasBinding binding,Buffer output) {
        require(binding.format==VK_FORMAT_R8G8B8A8_UNORM&&binding.mips==1&&binding.layout==VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL&&binding.view&&binding.sampler,"alias sampled binding contract");
        VkDescriptorImageInfo image{binding.sampler,binding.view,binding.layout};VkDescriptorBufferInfo buffer{output.handle,0,output.size};VkWriteDescriptorSet writes[2]{};
        for(unsigned i=0;i<2;++i){writes[i]={VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};writes[i].dstSet=descriptor;writes[i].dstBinding=i;writes[i].descriptorCount=1;writes[i].descriptorType=i?VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;}writes[0].pImageInfo=&image;writes[1].pBufferInfo=&buffer;vkUpdateDescriptorSets(g.device,2,writes,0,nullptr);
        vkCmdBindPipeline(g.command,VK_PIPELINE_BIND_POINT_COMPUTE,pipeline);vkCmdBindDescriptorSets(g.command,VK_PIPELINE_BIND_POINT_COMPUTE,layout,0,1,&descriptor,0,nullptr);
        const std::array<std::uint32_t,2> extent{binding.extent.width,binding.extent.height};vkCmdPushConstants(g.command,layout,VK_SHADER_STAGE_COMPUTE_BIT,0,sizeof(extent),extent.data());vkCmdDispatch(g.command,(extent[0]*extent[1]+2+63)/64,1,1);
        VkBufferMemoryBarrier b{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};b.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;b.dstAccessMask=VK_ACCESS_HOST_READ_BIT;b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;b.buffer=output.handle;b.size=output.size;vkCmdPipelineBarrier(g.command,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,0,nullptr,1,&b,0,nullptr);
    }
};
SourceWeatherDepthAliasRecord packet(Gpu &g,Image depth,Input input,VkImageLayout layout,VkFence fence) {
    SourceWeatherDepthAliasRecord p;p.source={g.device,depth.handle,VK_FORMAT_D24_UNORM_S8_UINT,input.extent,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT,layout,VK_SAMPLE_COUNT_1_BIT,1,1,g.family,VK_SHARING_MODE_EXCLUSIVE};p.command_owner=g.device;p.command=g.command;p.completion=fence;p.command_queue_family=g.family;p.outside_render_pass=p.exclusive_access=true;return p;
}
void read_alias(Gpu &g,SourceWeatherDepthAliasBinding binding,Buffer output) {
    VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};b.image=binding.image;b.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;b.oldLayout=binding.layout;b.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;b.srcAccessMask=VK_ACCESS_SHADER_READ_BIT;b.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
    // The immediately preceding sampler probe is a COMPUTE consumer.
    vkCmdPipelineBarrier(g.command,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&b);
    VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={binding.extent.width,binding.extent.height,1};vkCmdCopyImageToBuffer(g.command,binding.image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,output.handle,1,&copy);
    b.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;b.newLayout=binding.layout;b.srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;b.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;vkCmdPipelineBarrier(g.command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,0,nullptr,0,nullptr,1,&b);
    VkBufferMemoryBarrier host{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};host.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;host.dstAccessMask=VK_ACCESS_HOST_READ_BIT;host.srcQueueFamilyIndex=host.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;host.buffer=output.handle;host.size=output.size;vkCmdPipelineBarrier(g.command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,0,nullptr,1,&host,0,nullptr);
}
unsigned gpu_rejections{},busy_rejections{};
std::vector<std::byte> run(Gpu &g,SourceWeatherDepthAlias &producer,SampleProbe &probe,Image depth,const Input &input,VkImageLayout layout,bool controls) {
    producer.prepare(input.extent);Fence fence(g),different(g);const auto p=packet(g,depth,input,layout,fence.value);
    const auto pixels=source_weather_depth_alias_storage(input.extent).pixels;
    const auto out=g.buffer(std::uint64_t(pixels)*4,VK_BUFFER_USAGE_TRANSFER_DST_BIT),sampled=g.buffer(std::uint64_t(pixels+2)*4,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    g.begin();
    if(controls){const auto reject_packet=[&](SourceWeatherDepthAliasRecord bad){reject([&]{producer.record(bad);},gpu_rejections);};auto bad=p;
        bad.source.type=VK_IMAGE_TYPE_3D;reject_packet(bad);bad=p;bad.source.tiling=VK_IMAGE_TILING_LINEAR;reject_packet(bad);bad=p;bad.source.create_flags=VK_IMAGE_CREATE_SPARSE_BINDING_BIT;reject_packet(bad);bad=p;
        bad.source.format=VK_FORMAT_D32_SFLOAT;reject_packet(bad);bad=p;bad.source.samples=VK_SAMPLE_COUNT_4_BIT;reject_packet(bad);bad=p;bad.source.extent.width-=1;reject_packet(bad);bad=p;bad.source.usage=VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;reject_packet(bad);bad=p;bad.source.mip_levels=2;reject_packet(bad);bad=p;bad.source.array_layers=2;reject_packet(bad);bad=p;bad.source.layout=VK_IMAGE_LAYOUT_UNDEFINED;reject_packet(bad);bad=p;bad.source.layout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;reject_packet(bad);bad=p;bad.source.owner={};reject_packet(bad);bad=p;bad.command_owner={};reject_packet(bad);bad=p;bad.source.queue_family=std::numeric_limits<unsigned>::max();reject_packet(bad);bad=p;bad.command_queue_family=std::numeric_limits<unsigned>::max();reject_packet(bad);bad=p;bad.source.sharing=VK_SHARING_MODE_CONCURRENT;reject_packet(bad);bad=p;bad.outside_render_pass=false;reject_packet(bad);bad=p;bad.exclusive_access=false;reject_packet(bad);bad=p;bad.completion={};reject_packet(bad);bad=p;bad.command={};reject_packet(bad);bad=p;bad.source.image={};reject_packet(bad);
        VkFenceCreateInfo ci{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};ci.flags=VK_FENCE_CREATE_SIGNALED_BIT;VkFence signaled{};check(vkCreateFence(g.device,&ci,nullptr,&signaled));bad=p;bad.completion=signaled;reject_packet(bad);vkDestroyFence(g.device,signaled,nullptr);
        reject([&]{producer.prepare({0,input.extent.height});},gpu_rejections);reject([&]{producer.prepare({std::numeric_limits<unsigned>::max(),2});},gpu_rejections);reject([&]{producer.retire(different.value);},gpu_rejections);
    }
    producer.record(p);
    bool submitted=false,retired=false;
    const auto submit=[&]{check(vkEndCommandBuffer(g.command));VkSubmitInfo info{VK_STRUCTURE_TYPE_SUBMIT_INFO};info.commandBufferCount=1;info.pCommandBuffers=&g.command;check(vkQueueSubmit(g.queue,1,&info,fence.value));submitted=true;};
    const auto retire=[&]{check(vkWaitForFences(g.device,1,&fence.value,VK_TRUE,std::numeric_limits<std::uint64_t>::max()));producer.retire(fence.value);retired=true;};
    try {
        if(controls){reject([&]{producer.record(p);},busy_rejections);reject([&]{producer.prepare(input.extent);},busy_rejections);reject([&]{producer.retire(fence.value);},busy_rejections);reject([&]{producer.retire(different.value);},busy_rejections);}
        const auto binding=producer.binding();probe.record(binding,sampled);read_alias(g,binding,out);submit();retire();
    }catch(...){
        // A failing assertion after record still honors its submission/fence
        // contract before local fence destruction and producer teardown.
        if(!submitted)submit();if(!retired)retire();throw;
    }
    std::vector<std::byte> result(input.expected.size());std::memcpy(result.data(),out.mapped,result.size());require(result==input.expected,"GPU packed alias differs from exact expected RGBA bytes");require(std::memcmp(sampled.mapped,input.expected.data(),input.expected.size())==0,"owned nearest sampled UNORM alias differs from exact RGBA bytes");
    const auto *samples=static_cast<std::byte*>(sampled.mapped);require(std::memcmp(samples+pixels*4,input.expected.data(),4)==0&&std::memcmp(samples+(pixels+1)*4,input.expected.data()+(pixels-1)*4,4)==0,"owned alias clamp sampler did not preserve edge pixels");verify_source(g,depth,input,layout);return result;
}
}
int main(int argc,char **argv){try{
    require(argc==1||(argc==2&&(std::string(argv[1])=="--gpu"||std::string(argv[1])=="--gpu-baseline")),"weather depth alias optional --gpu or --gpu-baseline expected");
    const auto original=original_input();unsigned rejected=0;
    reject([]{source_weather_depth_alias_storage({0,1});},rejected);reject([]{source_weather_depth_alias_storage({1,0});},rejected);reject([]{source_weather_depth_alias_storage({std::numeric_limits<unsigned>::max(),2});},rejected);reject([]{SourceWeatherDepthAlias p({});},rejected);
    for(unsigned n=1;n<=17;++n){const auto size=source_weather_depth_alias_storage({n,1});require(size.pixels==n&&size.depth_bytes==n*4&&size.stencil_bytes==((n+3)&~3u),"odd stencil allocation was not rounded to full uint words");}
    const auto full=control({17,17},0);for(unsigned c=0;c<4;++c){std::array<bool,256> seen{};for(unsigned i=0;i<289;++i)seen[std::to_integer<unsigned>(full.expected[i*4+c])]=true;require(std::all_of(seen.begin(),seen.end(),[](bool b){return b;}),"all-byte packing controls incomplete");}
    require(rejected==4,"CPU input guards incomplete");std::cout<<"Original event215778: all 307200 alias pixels match raw D24S8 byte permutation; checked odd stencil storage, all byte values, four CPU input guards\n";
    if(argc==1)return 0;Gpu g;if(!g.initialize())return 77;if(std::string(argv[1])=="--gpu-baseline")return 0;
    SourceWeatherDepthAlias producer({g.physical,g.device,g.family,HALO2_WEATHER_DEPTH_ALIAS_SHADER_DIR});SampleProbe probe(g);
    reject([&]{producer.binding();},gpu_rejections);reject([&]{producer.retire({});},gpu_rejections);
    reject([&]{SourceWeatherDepthAlias p({g.physical,g.device,std::numeric_limits<unsigned>::max(),HALO2_WEATHER_DEPTH_ALIAS_SHADER_DIR});},gpu_rejections);
    const auto depth=g.image(original.extent,VK_FORMAT_D24_UNORM_S8_UINT,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    upload(g,depth,original,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);verify_source(g,depth,original,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    const auto exact=run(g,producer,probe,depth,original,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,true);const auto binding=producer.binding();
    auto changed=original;for(auto &byte:changed.expected)byte^=std::byte{255};for(auto &byte:changed.depth)byte^=std::byte{255};for(auto &byte:changed.stencil)byte^=std::byte{255};
    upload(g,depth,changed,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);run(g,producer,probe,depth,changed,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,false);require(binding.image==producer.binding().image&&binding.view==producer.binding().view&&binding.sampler==producer.binding().sampler,"same extent did not reuse owned alias bindings");
    upload(g,depth,original,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);require(run(g,producer,probe,depth,original,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,false)==exact,"restored original alias reuse was not exact");
    const VkImageLayout layouts[]{VK_IMAGE_LAYOUT_GENERAL,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL};
    const Input controls[]{control({1,1},0),control({1,1},255),control({3,5},1),full};
    for(unsigned i=0;i<4;++i){const auto image=g.image(controls[i].extent,VK_FORMAT_D24_UNORM_S8_UINT,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT);upload(g,image,controls[i],VK_IMAGE_LAYOUT_UNDEFINED,layouts[i]);run(g,producer,probe,image,controls[i],layouts[i],false);}
    require(gpu_rejections==28&&busy_rejections==4,"GPU ownership/admission controls incomplete");require(validation_errors==0,"depth alias SDK validation diagnostics");
    const auto output=std::filesystem::path(HALO2_SOURCE_DIR)/"analysis/multiplayer-shading/weather-depth-alias-prototype";std::filesystem::create_directories(output);save(output/"native-event215778-alias.rgba",exact);
    std::cout<<"GPU: all 307200 original alias pixels byte-exact; source depth/stencil unchanged; current-image changes and binding reuse exact; 1x1 zero/max, 3x5 tail and 17x17 all-byte controls exact; all five source layouts restored; sampled nearest/clamp exact; 28 admission and four pending-ownership controls rejected; zero validation errors\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
