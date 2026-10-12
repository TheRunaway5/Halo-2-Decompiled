#include "halo2/source_weather_depth_alias.hpp"
// Reuse the already proved original scene fixture without editing it. Its
// standalone entry point is renamed and is never executed by this fixture.
#define main halo2_weather_plate_reference_fixture_entry
#include "source_weather_plate_scene_tests.cpp"
#undef main
#include <limits>

namespace {
struct CompositionFence {
    VkDevice device;VkFence fence{};
    explicit CompositionFence(Gpu &g):device(g.device){VkFenceCreateInfo ci{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};check(vkCreateFence(device,&ci,nullptr,&fence));}
    ~CompositionFence(){vkDestroyFence(device,fence,nullptr);}
};
void host_read(Gpu &g,Buffer buffer) {
    VkBufferMemoryBarrier b{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};b.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;b.dstAccessMask=VK_ACCESS_HOST_READ_BIT;b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;b.buffer=buffer.handle;b.size=buffer.size;vkCmdPipelineBarrier(g.command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,0,nullptr,1,&b,0,nullptr);
}
void alias_read(Gpu &g,SourceWeatherDepthAliasBinding binding,Buffer out) {
    VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};b.image=binding.image;b.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;b.oldLayout=binding.layout;b.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;b.srcAccessMask=VK_ACCESS_SHADER_READ_BIT;b.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
    vkCmdPipelineBarrier(g.command,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&b);
    VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={binding.extent.width,binding.extent.height,1};vkCmdCopyImageToBuffer(g.command,binding.image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,out.handle,1,&copy);
    b.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;b.newLayout=binding.layout;b.srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;b.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;vkCmdPipelineBarrier(g.command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,0,nullptr,0,nullptr,1,&b);host_read(g,out);
}
}
int main(int argc,char **argv){try{
    require(argc==1||(argc==2&&(std::string(argv[1])=="--gpu"||std::string(argv[1])=="--gpu-baseline")),"depth alias composition optional --gpu or --gpu-baseline expected");
    const auto root=std::filesystem::path(HALO2_SOURCE_DIR),inputs=root/"analysis/multiplayer-shading/weather-plate-original-reference/event-215778",output=root/"analysis/multiplayer-shading/weather-depth-alias-composition";
    const auto before=read(inputs/"before-scene-color.rgba"),original_after=read(inputs/"after-scene-color.rgba"),low24=read(inputs/"before-depth-d24-low24.bin"),source_depth=read(inputs/"before-scene-depth.raw"),original_alias=read(inputs/"stage-0-mip-0.rgba");
    const auto prior_native=read(root/"analysis/multiplayer-shading/weather-plate-scene-prototype/native-after-scene-color.rgba");
    require(before.size()==640*480*4&&original_after.size()==before.size()&&prior_native.size()==before.size()&&original_alias.size()==before.size()&&low24.size()==before.size()&&source_depth.size()==before.size(),"composition source surface dimensions changed");
    for(unsigned i=0;i<640*480;++i){require(original_alias[i*4]==source_depth[i*4+2]&&original_alias[i*4+1]==source_depth[i*4+1]&&original_alias[i*4+2]==source_depth[i*4]&&original_alias[i*4+3]==source_depth[i*4+3],"composition source depth alias byte permutation changed");require(prior_native[i*4+3]==before[i*4+3],"prior native plate alpha changed");}
    const auto code=read(root/"analysis/multiplayer-shading/weather-gpu-assets/lockout-global21-class0.vp");
    const auto vertices=decode<SourceWeatherPlateVertex>(read(inputs/"actual-four-vertices.bin"));const auto scroll=decode<ShaderVector>(read(inputs/"actual-scroll-c21-c23.bin")),clip=decode<ShaderVector>(read(inputs/"actual-clip-range.bin"));
    const auto regions=decode<std::array<std::int32_t,4>>(read(inputs/"actual-clip-regions.bin"));const auto ps=decode<std::uint32_t>(read(inputs/"source-mode3-pixel-definition.bin"));
    require(vertices.size()==4&&scroll.size()==3&&clip.size()==1&&regions.size()==8&&ps.size()==60,"composition original plate packet incomplete");
    std::cout<<"Original event215778 composition inputs: source D24S8/depth-alias relationship and prior native alpha verified for all 307200 pixels\n";
    if(argc==1)return 0;Gpu g;if(!g.initialize())return 77;if(std::string(argv[1])=="--gpu-baseline")return 0;
    const VkExtent2D extent{640,480};const auto color=g.image(extent,VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT),depth=g.image(extent,VK_FORMAT_D24_UNORM_S8_UINT,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    upload_color(g,color,before,VK_IMAGE_LAYOUT_UNDEFINED);upload_depth(g,depth,low24,source_depth);verify_depth(g,depth,low24,source_depth);
    const auto pass=g.scene_pass(false,VK_FORMAT_D24_UNORM_S8_UINT);const auto frame=g.framebuffer(pass,extent,color,depth);
    SourceWeatherPlateRender renderer({g.physical,g.device,pass,HALO2_WEATHER_PLATE_SHADER_DIR,true});SourceWeatherDepthAlias producer({g.physical,g.device,g.family,HALO2_WEATHER_DEPTH_ALIAS_SHADER_DIR});producer.prepare(extent);
    SourceWeatherPlatePacket plate;plate.vertex_program=code;std::copy(vertices.begin(),vertices.end(),plate.vertices.begin());std::copy(scroll.begin(),scroll.end(),plate.scroll.begin());std::copy(ps.begin(),ps.end(),plate.pixel.words.begin());std::copy(regions.begin(),regions.end(),plate.clip_regions.begin());plate.clip_range=clip[0];plate.surface={640,480,0,0};plate.depth_clamp=plate.depth_clipping=true;
    // Only stages1..3 are uploaded. Stage0 MUST come from current native D24.
    for(unsigned i=1;i<4;++i){const auto texture=g.image({256,256},VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT,7);upload_texture(g,texture,inputs,i,256);VkSamplerCreateInfo ci{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};ci.magFilter=ci.minFilter=VK_FILTER_LINEAR;ci.mipmapMode=VK_SAMPLER_MIPMAP_MODE_LINEAR;ci.addressModeU=ci.addressModeV=ci.addressModeW=VK_SAMPLER_ADDRESS_MODE_REPEAT;ci.minLod=-1000;ci.maxLod=1000;VkSampler sampler;check(vkCreateSampler(g.device,&ci,nullptr,&sampler));g.samplers.push_back(sampler);plate.textures[i]={texture.view,sampler,VK_FORMAT_R8G8B8A8_UNORM,{256,256},7};}
    const auto compose=[&] {
        const auto out=g.buffer(before.size(),VK_BUFFER_USAGE_TRANSFER_DST_BIT),alias_out=g.buffer(before.size(),VK_BUFFER_USAGE_TRANSFER_DST_BIT);CompositionFence fence(g);SourceWeatherDepthAliasRecord record;
        record.source={g.device,depth.handle,VK_FORMAT_D24_UNORM_S8_UINT,extent,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,VK_SAMPLE_COUNT_1_BIT,1,1,g.family,VK_SHARING_MODE_EXCLUSIVE};record.command_owner=g.device;record.command=g.command;record.completion=fence.fence;record.command_queue_family=g.family;record.outside_render_pass=record.exclusive_access=true;
        g.begin();producer.record(record);bool submitted=false,retired=false;
        const auto submit=[&]{check(vkEndCommandBuffer(g.command));VkSubmitInfo info{VK_STRUCTURE_TYPE_SUBMIT_INFO};info.commandBufferCount=1;info.pCommandBuffers=&g.command;check(vkQueueSubmit(g.queue,1,&info,fence.fence));submitted=true;};
        const auto retire=[&]{check(vkWaitForFences(g.device,1,&fence.fence,VK_TRUE,std::numeric_limits<std::uint64_t>::max()));producer.retire(fence.fence);retired=true;};
        try {
            const auto binding=producer.binding();plate.textures[0]={binding.view,binding.sampler,binding.format,binding.extent,binding.mips};renderer.prepare(plate);
            VkRenderPassBeginInfo bi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};bi.renderPass=pass;bi.framebuffer=frame;bi.renderArea={{0,0},extent};vkCmdBeginRenderPass(g.command,&bi,VK_SUBPASS_CONTENTS_INLINE);renderer.record(g.command,{0,0,640,480,0,1},{{0,0},extent});vkCmdEndRenderPass(g.command);
            g.readback(color.handle,extent,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,out);host_read(g,out);alias_read(g,binding,alias_out);submit();retire();
        }catch(...){if(!submitted)submit();if(!retired)retire();throw;}
        std::vector<std::byte> native(before.size()),alias(before.size());std::memcpy(native.data(),out.mapped,native.size());std::memcpy(alias.data(),alias_out.mapped,alias.size());require(alias==original_alias,"composition producer stage0 differs from all original alias bytes");require(native==prior_native,"live-produced alias changed previously proved native weather plate output");report(before,original_after,native);verify_depth(g,depth,low24,source_depth);return std::pair{native,alias};
    };
    const auto result=compose();const auto binding=producer.binding();upload_color(g,color,before,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);require(compose()==result,"reused live depth alias/plate composition changed");require(binding.image==producer.binding().image&&binding.view==producer.binding().view&&binding.sampler==producer.binding().sampler,"composition binding reuse changed handles");require(validation_errors==0,"live depth alias/plate SDK synchronization diagnostics");
    std::filesystem::create_directories(output);save(output/"native-after-scene-color.rgba",result.first);save(output/"native-generated-depth-alias.rgba",result.second);
    std::cout<<"Live current VkImage D24S8 -> exact generated stage0 -> original plate consumer: all 307200 alias/depth/stencil/target-alpha pixels preserved; scene bytes identical to prior native plate result; reuse exact; one fence covers producer and all consumers; zero SDK synchronization errors\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
