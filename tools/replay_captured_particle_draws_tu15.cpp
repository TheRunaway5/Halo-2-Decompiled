#include "halo2/cache.hpp"
#include "halo2/particle_renderer.hpp"
#include "halo2/source_camera.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>

using namespace halo2;
using Json=nlohmann::json;
namespace {
constexpr unsigned width=640,height=480;
void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
void check(VkResult result){if(result!=VK_SUCCESS)throw std::runtime_error("Vulkan result "+std::to_string(result));}
template<class T>void read(std::ifstream &stream,T &value){require(bool(stream.read(reinterpret_cast<char*>(&value),sizeof(value))),"Short actual particle record");}
struct Context {
    VkInstance instance{};VkDebugUtilsMessengerEXT messenger{};VkPhysicalDevice physical{};
    VkDevice device{};VkQueue queue{};unsigned family{},errors{},warnings{};
    VkRenderPass pass{};VkFramebuffer framebuffer{};VkCommandPool commands{};
    struct Image {VkImage image{};VkDeviceMemory memory{};VkImageView view{};};
    Image color,depth;VkBuffer result{};VkDeviceMemory result_memory{};void *mapped{};
    std::string gpu;unsigned observed_draw_states{};
    static VKAPI_ATTR VkBool32 VKAPI_CALL message(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT,const VkDebugUtilsMessengerCallbackDataEXT *data,void *user) {
        auto &self=*static_cast<Context*>(user);
        if(severity&VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)++self.errors;
        else if(severity&VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)++self.warnings;
        std::cerr<<data->pMessage<<'\n';return VK_FALSE;
    }
    unsigned memory_type(unsigned mask,VkMemoryPropertyFlags flags) {
        VkPhysicalDeviceMemoryProperties properties;vkGetPhysicalDeviceMemoryProperties(physical,&properties);
        for(unsigned n=0;n<properties.memoryTypeCount;++n)
            if((mask&(1u<<n)) && (properties.memoryTypes[n].propertyFlags&flags)==flags)return n;
        throw std::runtime_error("Actual GPU memory type unavailable");
    }
    void image(Image &out,VkFormat format,VkImageUsageFlags usage,VkImageAspectFlags aspect) {
        VkFormatProperties properties;vkGetPhysicalDeviceFormatProperties(physical,format,&properties);
        const auto required=aspect==VK_IMAGE_ASPECT_COLOR_BIT?VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT:VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
        require((properties.optimalTilingFeatures&required)!=0,"Offscreen attachment format unavailable");
        VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};info.imageType=VK_IMAGE_TYPE_2D;
        info.format=format;info.extent={width,height,1};info.mipLevels=info.arrayLayers=1;
        info.samples=VK_SAMPLE_COUNT_1_BIT;info.tiling=VK_IMAGE_TILING_OPTIMAL;info.usage=usage;
        check(vkCreateImage(device,&info,nullptr,&out.image));
        VkMemoryRequirements requirement;vkGetImageMemoryRequirements(device,out.image,&requirement);
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocation.allocationSize=requirement.size;
        allocation.memoryTypeIndex=memory_type(requirement.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        check(vkAllocateMemory(device,&allocation,nullptr,&out.memory));check(vkBindImageMemory(device,out.image,out.memory,0));
        VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};view.image=out.image;
        view.viewType=VK_IMAGE_VIEW_TYPE_2D;view.format=format;view.subresourceRange={aspect,0,1,0,1};
        check(vkCreateImageView(device,&view,nullptr,&out.view));
    }
    void initialize() {
        unsigned count{};check(vkEnumerateInstanceLayerProperties(&count,nullptr));
        std::vector<VkLayerProperties> layers(count);check(vkEnumerateInstanceLayerProperties(&count,layers.data()));
        const char *layer="VK_LAYER_KHRONOS_validation",*extension=VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        require(std::ranges::any_of(layers,[&](auto &p){return std::strcmp(p.layerName,layer)==0;}),"Real Khronos validation layer unavailable");
        VkApplicationInfo application{VK_STRUCTURE_TYPE_APPLICATION_INFO};application.pApplicationName="Captured TU1.5 particle graphics";application.apiVersion=VK_API_VERSION_1_1;
        VkDebugUtilsMessengerCreateInfoEXT debug{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
        debug.messageSeverity=VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
        debug.messageType=VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug.pfnUserCallback=message;debug.pUserData=this;
        VkInstanceCreateInfo info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};info.pApplicationInfo=&application;
        info.enabledLayerCount=info.enabledExtensionCount=1;info.ppEnabledLayerNames=&layer;info.ppEnabledExtensionNames=&extension;info.pNext=&debug;
        check(vkCreateInstance(&info,nullptr,&instance));
        const auto create=reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,"vkCreateDebugUtilsMessengerEXT"));
        require(bool(create),"Actual validation callback unavailable");check(create(instance,&debug,nullptr,&messenger));
        check(vkEnumeratePhysicalDevices(instance,&count,nullptr));std::vector<VkPhysicalDevice> devices(count);
        check(vkEnumeratePhysicalDevices(instance,&count,devices.data()));
        for(auto candidate:devices) {
            unsigned families{};vkGetPhysicalDeviceQueueFamilyProperties(candidate,&families,nullptr);
            std::vector<VkQueueFamilyProperties> queues(families);vkGetPhysicalDeviceQueueFamilyProperties(candidate,&families,queues.data());
            VkPhysicalDeviceFeatures features;vkGetPhysicalDeviceFeatures(candidate,&features);
            if(!features.geometryShader || !features.shaderTessellationAndGeometryPointSize || !features.depthClamp)continue;
            for(unsigned n=0;n<families;++n)if(queues[n].queueCount && (queues[n].queueFlags&VK_QUEUE_GRAPHICS_BIT)) {physical=candidate;family=n;break;}
            if(physical)break;
        }
        require(bool(physical),"Actual compatible graphics device unavailable");
        VkPhysicalDeviceProperties properties;vkGetPhysicalDeviceProperties(physical,&properties);gpu=properties.deviceName;
        VkPhysicalDeviceFeatures features{};vkGetPhysicalDeviceFeatures(physical,&features);
        VkDeviceQueueCreateInfo queue_info{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};const float priority=1;
        queue_info.queueFamilyIndex=family;queue_info.queueCount=1;queue_info.pQueuePriorities=&priority;
        VkDeviceCreateInfo device_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};device_info.queueCreateInfoCount=1;
        device_info.pQueueCreateInfos=&queue_info;device_info.pEnabledFeatures=&features;
        check(vkCreateDevice(physical,&device_info,nullptr,&device));vkGetDeviceQueue(device,family,0,&queue);
        image(color,VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT,VK_IMAGE_ASPECT_COLOR_BIT);
        image(depth,VK_FORMAT_D32_SFLOAT,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,VK_IMAGE_ASPECT_DEPTH_BIT);
        VkAttachmentDescription attachments[2]{};
        for(auto &attachment:attachments){attachment.samples=VK_SAMPLE_COUNT_1_BIT;attachment.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;attachment.storeOp=VK_ATTACHMENT_STORE_OP_STORE;attachment.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;attachment.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;}
        attachments[0].format=VK_FORMAT_R8G8B8A8_UNORM;attachments[0].finalLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        attachments[1].format=VK_FORMAT_D32_SFLOAT;attachments[1].finalLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        VkAttachmentReference color_ref{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},depth_ref{1,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        VkSubpassDescription subpass{};subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount=1;subpass.pColorAttachments=&color_ref;subpass.pDepthStencilAttachment=&depth_ref;
        VkSubpassDependency dependencies[2]{};
        dependencies[0]={VK_SUBPASS_EXTERNAL,0,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,0,VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,0};
        dependencies[1]={0,VK_SUBPASS_EXTERNAL,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_ACCESS_TRANSFER_READ_BIT,0};
        VkRenderPassCreateInfo render{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};render.attachmentCount=2;render.pAttachments=attachments;
        render.subpassCount=1;render.pSubpasses=&subpass;render.dependencyCount=2;render.pDependencies=dependencies;
        check(vkCreateRenderPass(device,&render,nullptr,&pass));
        const VkImageView views[]{color.view,depth.view};VkFramebufferCreateInfo frame{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        frame.renderPass=pass;frame.attachmentCount=2;frame.pAttachments=views;frame.width=width;frame.height=height;frame.layers=1;
        check(vkCreateFramebuffer(device,&frame,nullptr,&framebuffer));
        VkBufferCreateInfo buffer{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};buffer.size=width*height*4;buffer.usage=VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        check(vkCreateBuffer(device,&buffer,nullptr,&result));VkMemoryRequirements requirement;vkGetBufferMemoryRequirements(device,result,&requirement);
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocation.allocationSize=requirement.size;
        allocation.memoryTypeIndex=memory_type(requirement.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        check(vkAllocateMemory(device,&allocation,nullptr,&result_memory));check(vkBindBufferMemory(device,result,result_memory,0));
        check(vkMapMemory(device,result_memory,0,width*height*4,0,&mapped));
        VkCommandPoolCreateInfo pool{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};pool.queueFamilyIndex=family;
        check(vkCreateCommandPool(device,&pool,nullptr,&commands));
    }
    std::vector<std::uint8_t> draw(ParticleRenderer &particles,std::span<const ParticleDraw> draws,VkRect2D clip) {
        check(vkQueueWaitIdle(queue));particles.prepare(draws);
        VkCommandBufferAllocateInfo allocation{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};allocation.commandPool=commands;
        allocation.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;allocation.commandBufferCount=1;VkCommandBuffer command;
        check(vkAllocateCommandBuffers(device,&allocation,&command));
        VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        check(vkBeginCommandBuffer(command,&begin));VkClearValue clear[2]{};
        clear[0].color={{.0625f,.0625f,.0625f,1}};clear[1].depthStencil={1,0};
        VkRenderPassBeginInfo render{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};render.renderPass=pass;render.framebuffer=framebuffer;
        render.renderArea={{0,0},{width,height}};render.clearValueCount=2;render.pClearValues=clear;
        vkCmdBeginRenderPass(command,&render,VK_SUBPASS_CONTENTS_INLINE);
        particles.record(command,VkViewport{0,0,float(width),float(height),0,1},clip,0,draws.size());
        for(std::size_t index=0;index<draws.size();++index) {
            const auto state=particles.view_inputs(index);const auto &draw=draws[index];
            require(state.revision==particles.view_revision() && state.draw==index && state.uniform_buffer &&
                state.descriptor && state.pipeline && !state.uniform_backing.is_null(),"Recorded native particle binding unavailable");
            const std::array<float,6> expected_view{0,0,float(width),float(height),0,1};
            const std::array<std::int32_t,4> expected_clip{clip.offset.x,clip.offset.y,
                std::int32_t(clip.extent.width),std::int32_t(clip.extent.height)};
            require(state.viewport==expected_view && state.scissor==expected_clip,"Recorded native viewport/scissor differs");
            require(state.depth_offset>=state.vertex_offset+sizeof(ParticleVertexState) &&
                state.depth_offset-state.vertex_offset<8192,"Recorded native particle arena layout differs");
            const auto *bound=reinterpret_cast<const std::byte*>(state.uniform_backing.value);
            require(!std::memcmp(bound,&draw.vertex,sizeof(draw.vertex)) &&
                !std::memcmp(bound+state.depth_offset-state.vertex_offset,&draw.depth,sizeof(draw.depth)),
                "Descriptor-backed complete vertex/depth inputs differ from actual captured source inputs");
            const auto program=particles.program(draw.material);std::uint64_t hash=14695981039346656037ull;
            for(auto byte:draw.material->vertex.program)hash=(hash^std::to_integer<unsigned>(byte))*1099511628211ull;
            require(program.pipeline==state.pipeline && program.vertex_module && program.layout &&
                program.program_hash==hash && program.classification==draw.material->vertex.classification &&
                program.instructions==(draw.material->vertex.program.size()-4)/16,
                "Recorded native pipeline/program differs from its actual authored tokens");
            ++observed_draw_states;
        }
        vkCmdEndRenderPass(command);
        VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={width,height,1};
        vkCmdCopyImageToBuffer(command,color.image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,result,1,&copy);
        VkBufferMemoryBarrier host{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};host.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
        host.dstAccessMask=VK_ACCESS_HOST_READ_BIT;host.srcQueueFamilyIndex=host.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;host.buffer=result;host.size=VK_WHOLE_SIZE;
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,0,nullptr,1,&host,0,nullptr);
        check(vkEndCommandBuffer(command));VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;
        check(vkQueueSubmit(queue,1,&submit,VK_NULL_HANDLE));check(vkQueueWaitIdle(queue));
        std::vector<std::uint8_t> bytes(width*height*4);std::memcpy(bytes.data(),mapped,bytes.size());
        vkFreeCommandBuffers(device,commands,1,&command);return bytes;
    }
    void release_image(Image &image) {
        if(image.view)vkDestroyImageView(device,image.view,nullptr);
        if(image.image)vkDestroyImage(device,image.image,nullptr);
        if(image.memory)vkFreeMemory(device,image.memory,nullptr);
    }
    ~Context() {
        if(device){vkDeviceWaitIdle(device);if(commands)vkDestroyCommandPool(device,commands,nullptr);
            if(mapped)vkUnmapMemory(device,result_memory);if(result)vkDestroyBuffer(device,result,nullptr);
            if(result_memory)vkFreeMemory(device,result_memory,nullptr);if(framebuffer)vkDestroyFramebuffer(device,framebuffer,nullptr);
            if(pass)vkDestroyRenderPass(device,pass,nullptr);release_image(color);release_image(depth);vkDestroyDevice(device,nullptr);}
        if(messenger){const auto destroy=reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,"vkDestroyDebugUtilsMessengerEXT"));if(destroy)destroy(instance,messenger,nullptr);}
        if(instance)vkDestroyInstance(instance,nullptr);
    }
};
void save(const std::filesystem::path &path,const std::vector<std::uint8_t> &bytes) {
    std::ofstream output(path,std::ios::binary);output<<"P6\n"<<width<<' '<<height<<"\n255\n";
    for(std::size_t n=0;n<bytes.size();n+=4)output.write(reinterpret_cast<const char*>(bytes.data()+n),3);
    require(bool(output),"Cannot retain GPU readback");
}
unsigned changed(const auto &first,const auto &second) {
    require(first.size()==second.size(),"GPU readback sizes differ");unsigned count{};
    for(unsigned n=0;n<first.size();n+=4)count+=!std::equal(first.begin()+n,first.begin()+n+4,second.begin()+n);
    return count;
}
using Materials=std::map<unsigned,std::shared_ptr<const ParticleMaterial>>;
std::vector<ParticleDraw> load_draws(Cache &cache,std::ifstream &records,const Json &sample,Materials &materials) {
    std::vector<ParticleDraw> draws;
    static_assert(sizeof(ParticleVertexState)==3360 && sizeof(ParticleDepthState)==32);
    for(const auto &record:sample.at("draws")) {
        const auto particle=record.at("particle_datum").get<unsigned>();
        if(!materials.contains(particle))materials.emplace(particle,std::make_shared<const ParticleMaterial>(cache.load_particle_material(particle,15,0,0)));
        ParticleDraw draw;draw.material=materials.at(particle);const auto &vertex=draw.material->vertex;
        require(vertex.template_datum==record.at("template") && vertex.pass_datum==record.at("pass") &&
            vertex.vertex_datum==record.at("vertex") && vertex.classification==record.at("classification"),"Actual authored particle material selection differs");
        records.seekg(record.at("record_offset").get<std::uint64_t>());std::array<unsigned,8> header;read(records,header);
        const std::array<unsigned,8> expected{sample.at("tick"),sample.at("view"),sample.at("first_person").get<bool>()?1u:0u,
            record.at("effect"),record.at("owner"),vertex.template_datum,vertex.pass_datum,vertex.vertex_datum};
        require(header==expected && record.at("record_bytes")==3424,"Actual particle record identity differs");
        read(records,draw.vertex);read(records,draw.depth);draw.effect_definition=header[3];draw.effect_owner=header[4];
        draw.scene_sort_depth=std::bit_cast<float>(record.at("sort_depth_bits").get<unsigned>());
        const auto &owner=record.at("sort_owner");const auto bits=owner.at("center_bits").get<std::array<unsigned,3>>();
        draw.scene_sort_owner=ParticleDraw::SortOwner{owner.at("system"),owner.at("location"),
            {std::bit_cast<float>(bits[0]),std::bit_cast<float>(bits[1]),std::bit_cast<float>(bits[2])}};
        draws.push_back(std::move(draw));
    }
    return draws;
}
VkRect2D sample_clip(unsigned view) {
    const auto grid=source_view_grid(2,view,false);
    return {{int(grid.position[0])*int(width/grid.grid[0]),int(grid.position[1])*int(height/grid.grid[1])},
        {width*unsigned(grid.span[0])/unsigned(grid.grid[0]),height*unsigned(grid.span[1])/unsigned(grid.grid[1])}};
}
int sequence(const Json &input,Cache &cache,std::ifstream &records,const std::filesystem::path &shaders,
    const std::filesystem::path &out) {
    require(input.at("source_local_players")==2 && input.at("samples").size()==810,"Unexpected native source view sequence");
    Context context;context.initialize();Materials materials;
    Json report={{"scope","Native graphics execution of every captured world/first-person particle sample across240 source ticks; isolated color/depth, without complete game composition or Xbox framebuffer comparison."},
        {"gpu",context.gpu},{"samples",Json::array()},{"source_inputs_modified",false},
        {"full_weapon_parity",false},{"physical_xbox_verified",false},{"rendered_sequence_verified",false},
        {"original_framebuffer_comparison_verified",false},{"peaks",Json::array()}};
    unsigned count{},total_draws{},empty{},world_positive{},fp_positive{};
    bool exact=true,empty_exact=true;std::map<bool,unsigned> peaks;std::map<bool,Json> peak_samples;
    std::set<unsigned> classifications;
    {
        ParticleRenderer particles(context.physical,context.device,context.queue,context.family,context.pass,shaders);
        for(const auto &sample:input.at("samples")) {
            const bool fp=sample.at("first_person");const auto draws=load_draws(cache,records,sample,materials);
            for(const auto &draw:draws)classifications.insert(draw.material->vertex.classification);
            total_draws+=draws.size();const auto clip=sample_clip(sample.at("view"));
            const auto baseline=context.draw(particles,{},clip),full=context.draw(particles,draws,clip),repeat=context.draw(particles,draws,clip);
            const auto pixels=changed(baseline,full);const bool repeated=full==repeat;
            if(!count)save(out/"baseline.ppm",baseline);
            exact&=repeated;if(draws.empty()){++empty;empty_exact&=full==baseline;}
            if(pixels){if(fp)++fp_positive;else ++world_positive;}
            Json row={{"tick",sample.at("tick")},{"view",sample.at("view")},{"first_person",fp},
                {"draws",draws.size()},{"changed_pixels",pixels},{"repeat_exact",repeated}};
            report["samples"].push_back(row);
            if(pixels>peaks[fp]) {
                peaks[fp]=pixels;const std::string stem=fp?"first-person-peak":"world-peak";
                save(out/(stem+".ppm"),full);save(out/(stem+"-repeat.ppm"),repeat);
                row["readback"]=stem+".ppm";row["repeat_readback"]=stem+"-repeat.ppm";peak_samples[fp]=row;
            }
            if(++count%100==0)std::cout<<"Completed native particle samples="<<count<<"/810\n"<<std::flush;
        }
    }
    require(total_draws==input.at("draws"),"Native graphics skipped source draw records");
    for(const auto &[fp,row]:peak_samples)report["peaks"].push_back(row);
    report["source_samples"]=count;report["draws"]=total_draws;report["authored_particle_materials"]=materials.size();
    report["classifications"]=classifications;report["empty_source_samples"]=empty;report["empty_source_samples_exact"]=empty_exact;
    report["world_positive_samples"]=world_positive;report["first_person_positive_samples"]=fp_positive;
    report["repeat_rgba_exact_all_samples"]=exact;report["observed_descriptor_backed_draw_states"]=context.observed_draw_states;
    report["complete_bound_vertex_depth_inputs_exact"]=true;report["recorded_native_program_selection_exact"]=true;
    report["validation_errors"]=context.errors;report["validation_warnings"]=context.warnings;
    report["passed"]=exact && empty_exact && world_positive && fp_positive && !context.errors;
    std::ofstream(out/"report.json")<<report.dump(2)<<'\n';
    std::cout<<Json{{"passed",report["passed"]},{"samples",count},{"draws",total_draws},
        {"world_positive_samples",world_positive},{"first_person_positive_samples",fp_positive},
        {"validation_errors",context.errors},{"validation_warnings",context.warnings}}.dump()<<'\n';
    return report.at("passed").get<bool>()?0:1;
}
}
int main(int argc,char **argv)try {
    require(argc==4,"Supply input manifest, shader directory and new output directory");
    Json input;std::ifstream(argv[1])>>input;require(input.at("passed").get<bool>(),"Unverified GPU input manifest");
    const std::filesystem::path out=argv[3];require(!std::filesystem::exists(out),"GPU output already exists");std::filesystem::create_directories(out);
    Cache cache(input.at("map").get<std::string>());std::ifstream records(input.at("source_draw_file").get<std::string>(),std::ios::binary);
    require(bool(records),"Actual source draw file unavailable");
    if(input.contains("samples"))return sequence(input,cache,records,argv[2],out);
    Materials materials;const auto draws=load_draws(cache,records,input,materials);
    std::map<unsigned,std::vector<ParticleDraw>> effects;std::set<unsigned> classifications;
    for(const auto &draw:draws){effects[draw.effect_definition].push_back(draw);classifications.insert(draw.material->vertex.classification);}
    require(!draws.empty(),"Actual queue has no GPU inputs");Context context;context.initialize();
    Json report={{"scope","Actual captured world particle draws on the native Vulkan graphics stages; isolated clear color/depth, without BSP, bodies or Xbox framebuffer comparison."},
        {"input_manifest",argv[1]},{"shader_directory",argv[2]},{"gpu",context.gpu},{"draws",draws.size()},
        {"authored_particle_materials",materials.size()},{"classifications",classifications},{"tick",input.at("tick")},{"view",input.at("view")},
        {"source_inputs_modified",false},{"physical_xbox_verified",false},{"full_weapon_parity",false},
        {"rendered_sequence_verified",false},{"original_framebuffer_comparison_verified",false},{"effects",Json::array()}};
    {
        ParticleRenderer particles(context.physical,context.device,context.queue,context.family,context.pass,argv[2]);
        const auto clip=sample_clip(input.at("view"));
        const auto baseline=context.draw(particles,{},clip),full=context.draw(particles,draws,clip),repeat=context.draw(particles,draws,clip);
        save(out/"baseline.ppm",baseline);save(out/"particles.ppm",full);save(out/"repeat.ppm",repeat);
        report["changed_pixels"]=changed(baseline,full);report["repeat_exact"]=full==repeat;
        report["scissor"]={clip.offset.x,clip.offset.y,clip.extent.width,clip.extent.height};
        for(const auto &[effect,subset]:effects) {
            const auto isolated=context.draw(particles,subset,clip);
            report["effects"].push_back({{"effect",effect},{"draws",subset.size()},{"changed_pixels",changed(baseline,isolated)}});
        }
    }
    report["validation_errors"]=context.errors;report["validation_warnings"]=context.warnings;
    report["observed_descriptor_backed_draw_states"]=context.observed_draw_states;
    report["complete_bound_vertex_depth_inputs_exact"]=true;
    report["recorded_native_program_selection_exact"]=true;
    report["passed"]=report.at("repeat_exact").get<bool>() && report.at("changed_pixels").get<unsigned>()>0 && !context.errors;
    std::ofstream(out/"report.json")<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';
    return report.at("passed").get<bool>()?0:1;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
