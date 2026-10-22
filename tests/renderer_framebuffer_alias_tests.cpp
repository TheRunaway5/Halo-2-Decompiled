#include "halo2/renderer.hpp"
#include "halo2/native_tu15_framebuffer_aliases.hpp"
#include "halo2/source_weather_depth_alias.hpp"
#include <bit>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <thread>

namespace {
using namespace halo2;
using Json=nlohmann::json;
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void check(VkResult result){if(result!=VK_SUCCESS)throw std::runtime_error("Native alias probe Vulkan "+std::to_string(result));}
template<class F>void rejects(F&& f){bool denied=false;try{f();}catch(const std::exception&){denied=true;}require(denied,"Unavailable native alias was admitted");}
struct Probe {
    SourceWeatherDepthAliasContext context;
    VkCommandPool pool{};VkCommandBuffer command{};VkFence fence{};VkBuffer buffer{};VkDeviceMemory memory{};void* mapped{};bool submitted{};
    ~Probe(){if(submitted)vkWaitForFences(context.device,1,&fence,VK_TRUE,UINT64_MAX);if(mapped)vkUnmapMemory(context.device,memory);if(buffer)vkDestroyBuffer(context.device,buffer,nullptr);if(memory)vkFreeMemory(context.device,memory,nullptr);if(fence)vkDestroyFence(context.device,fence,nullptr);if(pool)vkDestroyCommandPool(context.device,pool,nullptr);}
    void prepare(VkDeviceSize bytes) {
        VkCommandPoolCreateInfo pc{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};pc.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;pc.queueFamilyIndex=context.queue_family;check(vkCreateCommandPool(context.device,&pc,nullptr,&pool));
        VkCommandBufferAllocateInfo ac{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};ac.commandPool=pool;ac.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ac.commandBufferCount=1;check(vkAllocateCommandBuffers(context.device,&ac,&command));
        VkFenceCreateInfo fc{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};check(vkCreateFence(context.device,&fc,nullptr,&fence));
        VkBufferCreateInfo bc{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};bc.size=bytes;bc.usage=VK_BUFFER_USAGE_TRANSFER_DST_BIT;check(vkCreateBuffer(context.device,&bc,nullptr,&buffer));
        VkMemoryRequirements needed;vkGetBufferMemoryRequirements(context.device,buffer,&needed);VkPhysicalDeviceMemoryProperties physical;vkGetPhysicalDeviceMemoryProperties(context.physical,&physical);
        unsigned type=physical.memoryTypeCount;
        for(unsigned i=0;i<physical.memoryTypeCount;++i)if((needed.memoryTypeBits&(1u<<i))&&
            (physical.memoryTypes[i].propertyFlags&(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))==(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)){type=i;break;}
        require(type<physical.memoryTypeCount,"Actual native alias device lacks coherent probe storage");
        VkMemoryAllocateInfo ma{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};ma.allocationSize=needed.size;ma.memoryTypeIndex=type;check(vkAllocateMemory(context.device,&ma,nullptr,&memory));check(vkBindBufferMemory(context.device,buffer,memory,0));check(vkMapMemory(context.device,memory,0,bytes,0,&mapped));
    }
};
void barrier(VkCommandBuffer command,VkImage image,VkImageAspectFlags aspects,VkImageLayout from,VkImageLayout to,
             VkPipelineStageFlags before,VkAccessFlags prior,VkPipelineStageFlags after,VkAccessFlags next) {
    VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};b.image=image;b.subresourceRange={aspects,0,1,0,1};b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;b.oldLayout=from;b.newLayout=to;b.srcAccessMask=prior;b.dstAccessMask=next;
    vkCmdPipelineBarrier(command,before,after,0,0,nullptr,0,nullptr,1,&b);
}
std::uint64_t actual_depth_pixels(NativeTu15FramebufferAliases& aliases) {
    auto& producer=aliases.depth_producer();const auto& depth=aliases.targets().depth_stencil;
    Probe gpu{producer.context()};check(vkDeviceWaitIdle(gpu.context.device));
    const VkExtent2D extent{depth.width,depth.height};const auto sizes=source_weather_depth_alias_storage(extent);
    const auto stride=sizes.depth_bytes+sizes.stencil_bytes,total=stride*2+sizes.depth_bytes;gpu.prepare(total);
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};check(vkBeginCommandBuffer(gpu.command,&begin));
    const auto depth_image=std::bit_cast<VkImage>(depth.image);
    const auto capture=[&](VkDeviceSize base) {
        barrier(gpu.command,depth_image,VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_ACCESS_TRANSFER_READ_BIT);
        VkBufferImageCopy copies[2]{};copies[0].bufferOffset=base;copies[0].imageSubresource={VK_IMAGE_ASPECT_DEPTH_BIT,0,0,1};copies[1].bufferOffset=base+sizes.depth_bytes;copies[1].imageSubresource={VK_IMAGE_ASPECT_STENCIL_BIT,0,0,1};
        for(auto& c:copies)c.imageExtent={extent.width,extent.height,1};vkCmdCopyImageToBuffer(gpu.command,depth_image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,gpu.buffer,2,copies);
        barrier(gpu.command,depth_image,VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT,VK_ACCESS_TRANSFER_READ_BIT,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT);
    };
    capture(0);
    SourceWeatherDepthAliasRecord record{};record.source={gpu.context.device,depth_image,VkFormat(depth.format),extent,depth.usage,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,VkSampleCountFlagBits(depth.samples),depth.mips,depth.layers,gpu.context.queue_family,VK_SHARING_MODE_EXCLUSIVE};
    record.command_owner=gpu.context.device;record.command=gpu.command;record.completion=gpu.fence;record.command_queue_family=gpu.context.queue_family;record.outside_render_pass=record.exclusive_access=true;
    producer.record(record);bool pending=true;
    try {
        // Re-querying a prepared registry while this submission owns its
        // producer must preserve both the resource and fence ownership.
        require(&aliases.depth_producer()==&producer,"Native producer re-query recreated an outstanding owner");
        const auto binding=producer.binding();require(std::bit_cast<std::uint64_t>(binding.view)==aliases.resolve(0x515e48).view,"Native registry selected another produced depth view");
        barrier(gpu.command,binding.image,VK_IMAGE_ASPECT_COLOR_BIT,binding.layout,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_SHADER_READ_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_ACCESS_TRANSFER_READ_BIT);
        VkBufferImageCopy copy{};copy.bufferOffset=stride*2;copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={extent.width,extent.height,1};vkCmdCopyImageToBuffer(gpu.command,binding.image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,gpu.buffer,1,&copy);
        barrier(gpu.command,binding.image,VK_IMAGE_ASPECT_COLOR_BIT,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,binding.layout,
            VK_PIPELINE_STAGE_TRANSFER_BIT,VK_ACCESS_TRANSFER_READ_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,VK_ACCESS_SHADER_READ_BIT);
        capture(stride);
        VkBufferMemoryBarrier host{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};host.buffer=gpu.buffer;host.size=total;host.srcQueueFamilyIndex=host.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;host.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;host.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
        vkCmdPipelineBarrier(gpu.command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,0,nullptr,1,&host,0,nullptr);check(vkEndCommandBuffer(gpu.command));
        VkQueue queue{};vkGetDeviceQueue(gpu.context.device,gpu.context.queue_family,0,&queue);VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&gpu.command;check(vkQueueSubmit(queue,1,&submit,gpu.fence));gpu.submitted=true;
        check(vkWaitForFences(gpu.context.device,1,&gpu.fence,VK_TRUE,UINT64_MAX));producer.retire(gpu.fence);pending=false;
    } catch(...) {
        if(pending){if(gpu.submitted){check(vkWaitForFences(gpu.context.device,1,&gpu.fence,VK_TRUE,UINT64_MAX));producer.retire(gpu.fence);}else{check(vkResetCommandBuffer(gpu.command,0));producer.discard_unsubmitted(gpu.command);}}
        throw;
    }
    const auto* bytes=static_cast<const unsigned char*>(gpu.mapped);
    for(unsigned i=0;i<sizes.pixels;++i) {
        std::uint32_t before{},after{};std::memcpy(&before,bytes+i*4,4);std::memcpy(&after,bytes+stride+i*4,4);
        require((before&0xffffff)==(after&0xffffff)&&bytes[sizes.depth_bytes+i]==bytes[stride+sizes.depth_bytes+i],"Native alias production changed the actual renderer depth/stencil");
        const auto* rgba=bytes+stride*2+i*4;
        require(rgba[0]==((before>>8)&255)&&rgba[1]==(before&255)&&rgba[2]==bytes[sizes.depth_bytes+i]&&rgba[3]==((before>>16)&255),"Native registry alias bytes differ from the renderer's current depth producer");
    }
    return sizes.pixels;
}
Json inspect(const NativeTu15FramebufferAliases& aliases,unsigned width,unsigned height) {
    Json out=Json::array();const auto& targets=aliases.targets();
    for(const auto cell:std::array<std::uint32_t,8>{0x515e34,0x515e38,0x515e3c,0x515e40,0x515e44,0x515e48,0x515e4c,0x515e50}) {
        const auto& b=aliases.resolve(cell);require(b.image&&b.view&&b.width==width&&b.height==height&&aliases.native_address(cell).value>0xffffffffu,"Native alias identity, dimensions or wide backing differ");
        require(aliases.native_address(cell).value==reinterpret_cast<std::uintptr_t>(&b),"Native alias address did not retain its actual record owner");
        out.push_back({{"source_cell",cell},{"image",b.image},{"view",b.view},{"parent_image",b.parent_image},{"native_backing",aliases.native_address(cell).value}});
    }
    require(aliases.resolve(0x515e34).image==targets.color[0].image&&aliases.resolve(0x515e38).image==targets.color[1].image&&aliases.resolve(0x515e44).image==targets.depth_stencil.image,"Native aliases changed their original physical parent relationships");
    require(aliases.resolve(0x515e48).image==aliases.resolve(0x515e4c).image&&aliases.resolve(0x515e48).view!=aliases.resolve(0x515e4c).view&&aliases.resolve(0x515e50).view==aliases.resolve(0x515e48).view,"Depth/color alias views do not share their actual producer");
    rejects([&]{aliases.resolve(0x515e54);});return out;
}
std::vector<char> read(const std::filesystem::path& path){std::ifstream f(path,std::ios::binary);require(bool(f),"Native alias frame is missing");return {std::istreambuf_iterator<char>(f),{}};}
}
int main(int argc,char** argv)try {
    require(argc==2,"Native alias output directory required");const std::filesystem::path output=argv[1];std::filesystem::create_directories(output);
    SDL_SetMainReady();require(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
    auto* window=SDL_CreateWindow("Native framebuffer aliases",0,0,640,480,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN|SDL_WINDOW_RESIZABLE);require(window,SDL_GetError());
    std::ostringstream diagnostics;auto* previous=std::cerr.rdbuf(diagnostics.rdbuf());
    Json report{{"observations",Json::array()},{"source_initializer_complete",false}};std::uint64_t pixels=0;
    try {
        Mesh mesh;Material material;material.diffuse={1,1,11,{std::byte{32},std::byte{80},std::byte{128},std::byte{255}}};mesh.materials.push_back(material);
        mesh.vertices={{{4,-2,-1},{1,1,1},{0,0},0},{{4,2,-1},{1,1,1},{0,0},0},{{4,0,2},{1,1,1},{0,0},0}};
        const auto* shader=std::getenv("HALO2_NATIVE_GPU_SHADER_DIR");
        auto renderer=std::make_unique<Renderer>(window,mesh,true,std::span<const SourceShadowPlanMaterial>{},nullptr,std::function<void()>{},nullptr,shader?std::filesystem::path(shader):std::filesystem::path{});
        report["gpu"]=renderer->device_name();auto factory=renderer->target_resources();
        rejects([&]{NativeTu15FramebufferAliases invalid(factory.acquire());});
        auto use=factory.source_buffers(2);std::optional<NativeTu15FramebufferAliases> aliases;aliases.emplace(std::move(use));
        report["observations"].push_back(inspect(*aliases,640,480));rejects([&]{aliases->depth_producer().binding();});
        bool denied=false;std::thread foreign([&]{denied=!aliases->valid();rejects([&]{aliases->native_address(0x515e48);});rejects([&]{aliases->depth_producer();});});foreign.join();require(denied,"Native alias registry crossed its owning thread");
        Camera camera;camera.field_of_view=60;require(renderer->draw(camera,output/"before.ppm"),"Native alias baseline frame failed");
        auto captured=factory.acquire();const auto slot=*captured.bank().last_submitted_color;const auto color=captured.source_color_pixels(slot);const auto counter=captured.bank().source_frames_submitted;
        pixels+=actual_depth_pixels(*aliases);require(captured.source_color_pixels(slot)==color&&factory.acquire().bank().source_frames_submitted==counter,"Alias production changed native color pixels or frame selection");
        {NativeTu15FramebufferAliases again(factory.acquire());require(&again.depth_producer()==&aliases->depth_producer(),"Repeated native alias lookup allocated another producer");}
        require(renderer->draw(camera,output/"after.ppm")&&read(output/"before.ppm")==read(output/"after.ppm"),"Preparing/producing native aliases changed existing rendered pixels");
        for(unsigned i=0;i<6;++i) {
            const auto before=factory.acquire().bank();
            require(aliases->resolve(0x515e3c).image==before.color[before.next_source_color].image&&
                aliases->resolve(0x515e40).image==before.color[before.next_source_color^1u].image&&
                aliases->resolve(0x515e34).image==before.color[0].image&&aliases->resolve(0x515e38).image==before.color[1].image,
                "Native surface rotation changed fixed texture aliases or selected the wrong current physical bank");
            require(renderer->draw(camera),"Native rotating alias frame failed");const auto after=factory.acquire().bank();
            require(after.source_frames_submitted==before.source_frames_submitted+1&&
                aliases->resolve(0x515e3c).image==after.color[after.next_source_color].image,
                "Native alias registry retained a stale surface Data interpretation after presentation");
        }
        SDL_SetWindowSize(window,800,600);SDL_PumpEvents();renderer->resized();require(renderer->draw(camera,output/"resized.ppm"),"Native alias resized frame failed");
        require(!aliases->valid(),"Attachment resize retained a stale native alias");rejects([&]{aliases->resolve(0x515e48);});rejects([&]{aliases->depth_producer();});rejects([&]{aliases->native_address(0x515e34);});
        aliases.emplace(factory.acquire());report["observations"].push_back(inspect(*aliases,800,600));pixels+=actual_depth_pixels(*aliases);
        DisplaySettings display;display.internal_width=320;display.internal_height=240;renderer->display_settings(display);require(renderer->draw(camera,output/"internal.ppm"),"Native alias internal-size frame failed");require(!aliases->valid(),"Internal resize retained a stale alias");
        aliases.emplace(factory.acquire());report["observations"].push_back(inspect(*aliases,320,240));pixels+=actual_depth_pixels(*aliases);
        const auto retired_factory=factory;renderer->release_presentation();require(!aliases->valid()&&!factory.valid(),"Presentation retirement retained native alias access");rejects([&]{aliases->depth_producer();});
        require(renderer->draw(camera,output/"recreated.ppm"),"Native alias presentation recreation failed");factory=renderer->target_resources();require(!retired_factory.valid(),"Presentation recreation revived an old alias factory");
        aliases.emplace(factory.acquire());report["observations"].push_back(inspect(*aliases,320,240));pixels+=actual_depth_pixels(*aliases);
        renderer.reset();require(!aliases->valid(),"Renderer destruction retained native alias access");rejects([&]{aliases->resolve(0x515e48);});aliases.reset();
    }catch(...){std::cerr.rdbuf(previous);std::cerr<<diagnostics.str();SDL_DestroyWindow(window);SDL_Quit();throw;}
    std::cerr.rdbuf(previous);std::cerr<<diagnostics.str();SDL_DestroyWindow(window);SDL_Quit();
    require(diagnostics.str().find("Vulkan validation:")==std::string::npos,"Native alias ownership probe emitted Vulkan validation errors");
    report["actual_renderer_depth_pixels_checked"]=pixels;report["native_address_bits"]=64;report["frame_pixels_and_counter_unchanged"]=true;report["fixed_texture_and_rotating_surface_relationship_verified"]=true;report["retirement_resize_thread_and_unknown_id_rejected"]=true;report["vulkan_validation_messages"]=0;
    std::ofstream out(output/"report.json");out<<report.dump(2)<<'\n';require(bool(out),"Cannot retain native alias evidence");
    std::cout<<"Eight native framebuffer identities; "<<pixels<<" actual renderer depth/stencil pixels; 64-bit records, resize/retirement and unchanged frames verified\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
