#include "halo2/particle_renderer.hpp"
#include "halo2/native_fragment_kernel.hpp"
#include "halo2/particle_material_validation.hpp"
#include "helpers/particle_admission_material.hpp"
#ifndef HALO2_PARTICLE_ADMISSION_BASELINE
#include "halo2/particle_resource_error.hpp"
#endif
#include <cstdlib>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>
#include <nlohmann/json.hpp>

namespace {
using namespace halo2;
unsigned validation_errors{};
void require(bool value,const char* message){if(!value)throw FormatError(message);}
void vkcheck(VkResult value){if(value!=VK_SUCCESS)throw FormatError("Particle admission fixture Vulkan failure: "+std::to_string(value));}
VKAPI_ATTR VkBool32 VKAPI_CALL debug(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT,const VkDebugUtilsMessengerCallbackDataEXT* message,void*) {
    if(severity&VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT){++validation_errors;std::cerr<<message->pMessage<<'\n';}
    return VK_FALSE;
}
class Device {
    VkInstance instance{};VkDebugUtilsMessengerEXT messenger{};
    VkDevice device{};VkRenderPass pass{};VkPhysicalDevice physical{};VkQueue queue{};unsigned family{};
    bool mirror_clamp_enabled{};
    struct Image {VkImage image{};VkDeviceMemory memory{};VkImageView view{};};
    std::array<Image,2> images{};VkFramebuffer framebuffer{};VkCommandPool commands{};
    void release() {
        if(device)vkDeviceWaitIdle(device);
        if(commands)vkDestroyCommandPool(device,commands,nullptr);
        if(framebuffer)vkDestroyFramebuffer(device,framebuffer,nullptr);
        for(const auto& image:images){if(image.view)vkDestroyImageView(device,image.view,nullptr);if(image.image)vkDestroyImage(device,image.image,nullptr);if(image.memory)vkFreeMemory(device,image.memory,nullptr);}
        if(pass)vkDestroyRenderPass(device,pass,nullptr);
        if(device)vkDestroyDevice(device,nullptr);
        if(messenger)reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,"vkDestroyDebugUtilsMessengerEXT"))(instance,messenger,nullptr);
        if(instance)vkDestroyInstance(instance,nullptr);
    }
public:
    Device()try {
        std::uint32_t count{};vkcheck(vkEnumerateInstanceLayerProperties(&count,nullptr));
        std::vector<VkLayerProperties> layers(count);vkcheck(vkEnumerateInstanceLayerProperties(&count,layers.data()));
        const char* validation="VK_LAYER_KHRONOS_validation";bool available=false;
        for(const auto& layer:layers)available|=std::string_view(layer.layerName)==validation;
        const char* extension=VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.apiVersion=VK_API_VERSION_1_3;
        VkInstanceCreateInfo info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};info.pApplicationInfo=&app;
        if(available){info.enabledLayerCount=1;info.ppEnabledLayerNames=&validation;info.enabledExtensionCount=1;info.ppEnabledExtensionNames=&extension;}
        vkcheck(vkCreateInstance(&info,nullptr,&instance));
        if(available) {
            VkDebugUtilsMessengerCreateInfoEXT create{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
            create.messageSeverity=VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            create.messageType=VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            create.pfnUserCallback=debug;
            vkcheck(reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,"vkCreateDebugUtilsMessengerEXT"))(instance,&create,nullptr,&messenger));
        }
        vkcheck(vkEnumeratePhysicalDevices(instance,&count,nullptr));std::vector<VkPhysicalDevice> devices(count);
        vkcheck(vkEnumeratePhysicalDevices(instance,&count,devices.data()));
        for(auto candidate:devices) {
            VkPhysicalDeviceProperties properties{};vkGetPhysicalDeviceProperties(candidate,&properties);
            VkPhysicalDeviceVulkan13Features features13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
            VkPhysicalDeviceVulkan12Features features12{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};features13.pNext=&features12;
            VkPhysicalDeviceFeatures2 features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};features.pNext=&features13;
            vkGetPhysicalDeviceFeatures2(candidate,&features);
            if(properties.apiVersion<VK_API_VERSION_1_3 || !features.features.geometryShader ||
               !features.features.shaderTessellationAndGeometryPointSize || !features.features.depthClamp || !features13.shaderDemoteToHelperInvocation)continue;
            vkGetPhysicalDeviceQueueFamilyProperties(candidate,&count,nullptr);std::vector<VkQueueFamilyProperties> queues(count);
            vkGetPhysicalDeviceQueueFamilyProperties(candidate,&count,queues.data());
            for(unsigned n=0;n<count;++n)if(queues[n].queueFlags&VK_QUEUE_GRAPHICS_BIT){physical=candidate;family=n;break;}
            if(physical){mirror_clamp_enabled=features12.samplerMirrorClampToEdge;break;}
        }
        require(physical,"Particle admission fixture has no Vulkan1.3 geometry/demote device");
        float priority=1;VkDeviceQueueCreateInfo qi{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};qi.queueFamilyIndex=family;qi.queueCount=1;qi.pQueuePriorities=&priority;
        VkPhysicalDeviceFeatures enabled{};enabled.geometryShader=enabled.shaderTessellationAndGeometryPointSize=enabled.depthClamp=VK_TRUE;
        VkPhysicalDeviceVulkan13Features enabled13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};enabled13.shaderDemoteToHelperInvocation=VK_TRUE;
        VkPhysicalDeviceVulkan12Features enabled12{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
        enabled12.samplerMirrorClampToEdge=mirror_clamp_enabled;enabled13.pNext=&enabled12;
        VkDeviceCreateInfo di{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};di.pNext=&enabled13;di.pEnabledFeatures=&enabled;di.queueCreateInfoCount=1;di.pQueueCreateInfos=&qi;
        vkcheck(vkCreateDevice(physical,&di,nullptr,&device));vkGetDeviceQueue(device,family,0,&queue);
        std::array<VkAttachmentDescription,2> attachments{};
        attachments[0].format=VK_FORMAT_R8G8B8A8_UNORM;attachments[1].format=VK_FORMAT_D32_SFLOAT;
        for(auto& attachment:attachments){attachment.samples=VK_SAMPLE_COUNT_1_BIT;attachment.loadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;attachment.storeOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;attachment.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;attachment.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;attachment.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;}
        attachments[0].finalLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;attachments[1].finalLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        const VkAttachmentReference color{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},depth{1,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        VkSubpassDescription subpass{};subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;subpass.colorAttachmentCount=1;subpass.pColorAttachments=&color;subpass.pDepthStencilAttachment=&depth;
        VkRenderPassCreateInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};rp.attachmentCount=attachments.size();rp.pAttachments=attachments.data();rp.subpassCount=1;rp.pSubpasses=&subpass;
        vkcheck(vkCreateRenderPass(device,&rp,nullptr,&pass));
        VkPhysicalDeviceMemoryProperties memory{};vkGetPhysicalDeviceMemoryProperties(physical,&memory);
        for(unsigned n=0;n<images.size();++n) {
            auto& image=images[n];VkImageCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};ci.imageType=VK_IMAGE_TYPE_2D;
            ci.format=attachments[n].format;ci.extent={1,1,1};ci.mipLevels=ci.arrayLayers=1;ci.samples=VK_SAMPLE_COUNT_1_BIT;
            ci.tiling=VK_IMAGE_TILING_OPTIMAL;ci.usage=n?VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT:(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT);
            vkcheck(vkCreateImage(device,&ci,nullptr,&image.image));VkMemoryRequirements requirements{};vkGetImageMemoryRequirements(device,image.image,&requirements);
            VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};ai.allocationSize=requirements.size;ai.memoryTypeIndex=memory.memoryTypeCount;
            for(unsigned type=0;type<memory.memoryTypeCount;++type)if((requirements.memoryTypeBits&(1u<<type)) &&
                (memory.memoryTypes[type].propertyFlags&VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)){ai.memoryTypeIndex=type;break;}
            require(ai.memoryTypeIndex<memory.memoryTypeCount,"Particle fixture image memory unavailable");
            vkcheck(vkAllocateMemory(device,&ai,nullptr,&image.memory));vkcheck(vkBindImageMemory(device,image.image,image.memory,0));
            VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};vi.image=image.image;vi.viewType=VK_IMAGE_VIEW_TYPE_2D;vi.format=ci.format;
            vi.subresourceRange={n?VK_IMAGE_ASPECT_DEPTH_BIT:VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};vkcheck(vkCreateImageView(device,&vi,nullptr,&image.view));
        }
        const VkImageView views[]{images[0].view,images[1].view};VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fb.renderPass=pass;fb.attachmentCount=2;fb.pAttachments=views;fb.width=fb.height=fb.layers=1;vkcheck(vkCreateFramebuffer(device,&fb,nullptr,&framebuffer));
        VkCommandPoolCreateInfo pool{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};pool.queueFamilyIndex=family;vkcheck(vkCreateCommandPool(device,&pool,nullptr,&commands));
    }catch(...){release();throw;}
    ~Device(){release();}
    std::unique_ptr<ParticleRenderer> renderer(const std::filesystem::path& directory,bool allow_mirror=true) {
#ifdef HALO2_PARTICLE_ADMISSION_BASELINE
        (void)allow_mirror;return std::make_unique<ParticleRenderer>(physical,device,queue,family,pass,directory);
#else
        return std::make_unique<ParticleRenderer>(physical,device,queue,family,pass,directory,allow_mirror && mirror_clamp_enabled);
#endif
    }
    bool supports_mirror_clamp()const{return mirror_clamp_enabled;}
    ParticleSceneTexture scene()const{return {images[0].view};}
    void record(ParticleRenderer& renderer,std::size_t count) {
        VkCommandBuffer command{};VkCommandBufferAllocateInfo allocate{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        allocate.commandPool=commands;allocate.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;allocate.commandBufferCount=1;vkcheck(vkAllocateCommandBuffers(device,&allocate,&command));
        VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};vkcheck(vkBeginCommandBuffer(command,&begin));
        VkRenderPassBeginInfo render{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};render.renderPass=pass;render.framebuffer=framebuffer;render.renderArea.extent={1,1};
        vkCmdBeginRenderPass(command,&render,VK_SUBPASS_CONTENTS_INLINE);renderer.record(command,{0,0,1,1,0,1},{{0,0},{1,1}},0,count);
        vkCmdEndRenderPass(command);vkcheck(vkEndCommandBuffer(command));vkFreeCommandBuffers(device,commands,1,&command);
    }
};
}
#ifdef HALO2_PARTICLE_TEST_LINK_WRAP
VkResult next_sampler_failure=VK_SUCCESS;
unsigned sampler_creations{};
unsigned fail_buffer_creation{};
extern "C" VKAPI_ATTR VkResult VKAPI_CALL __real_vkCreateSampler(VkDevice,const VkSamplerCreateInfo*,const VkAllocationCallbacks*,VkSampler*);
extern "C" VKAPI_ATTR VkResult VKAPI_CALL __wrap_vkCreateSampler(VkDevice device,const VkSamplerCreateInfo* info,const VkAllocationCallbacks* allocator,VkSampler* sampler) {
    ++sampler_creations;
    if(next_sampler_failure!=VK_SUCCESS){const auto failure=next_sampler_failure;next_sampler_failure=VK_SUCCESS;return failure;}
    return __real_vkCreateSampler(device,info,allocator,sampler);
}
extern "C" VKAPI_ATTR VkResult VKAPI_CALL __real_vkCreateBuffer(VkDevice,const VkBufferCreateInfo*,const VkAllocationCallbacks*,VkBuffer*);
extern "C" VKAPI_ATTR VkResult VKAPI_CALL __wrap_vkCreateBuffer(VkDevice device,const VkBufferCreateInfo* info,const VkAllocationCallbacks* allocator,VkBuffer* buffer) {
    if(fail_buffer_creation && !--fail_buffer_creation)return VK_ERROR_OUT_OF_DEVICE_MEMORY;
    return __real_vkCreateBuffer(device,info,allocator,buffer);
}
#endif
int main(int argc,char** argv) {
    nlohmann::json report{{"passed",false},{"scope","Actual production particle GPU resource admission and failure recovery; no simulation or framebuffer claim"}};
    const auto save=[&]{if(argc>2){std::ofstream output(argv[2]);output<<report.dump(2)<<'\n';}};
    try {
        require(argc>=3,"Supply compiled shader directory, owned JSON report and optional prepared MCC maps");
        const auto* owner=std::getenv("HALO2_TEST_SCRATCH");require(owner,"Particle admission fixture needs its scratch owner");
        const std::filesystem::path original=argv[1],staged=std::filesystem::path(owner)/"particle-admission-shaders";
        std::filesystem::create_directories(staged);
        for(const char* name:{"particle.vert.spv","particle.geom.spv","particle_directional.vert.spv","particle_perpendicular.vert.spv","particle_model.vert.spv","particle_model.geom.spv","particle_distortion.vert.spv"})
            std::filesystem::copy_file(original/name,staged/name);
        auto source=std::make_shared<ParticleMaterial>(tests::particle_admission_material());
        const auto fragment=std::filesystem::path(native_fragment_kernel_path("particle",source->pass.program.words));
        {
            Device device;auto renderer=device.renderer(staged);
            bool rejected=false;
            try{(void)renderer->program(source);}catch(const FormatError& error){rejected=std::string_view(error.what())=="compiled native particle fragment kernel missing";}
            require(rejected,"Missing compiled particle fragment was not rejected at actual GPU admission");
            std::filesystem::create_directories((staged/fragment).parent_path());
            std::filesystem::copy_file(original/fragment,staged/fragment);
            const auto recovered=renderer->program(source);
            require(recovered.pipeline && recovered.vertex_module && recovered.instructions==51,
                "Corrected particle shader could not recover its actual GPU pipeline");
            report["missing_fragment_recovery"]=true;
#ifndef HALO2_PARTICLE_ADMISSION_BASELINE
            auto bad=std::make_shared<ParticleMaterial>(*source);bad->textures[0].reset();std::string diagnostic;
            auto mirrored=std::make_shared<ParticleMaterial>(*source);mirrored->pass.texture_states[0].address_u=5;
            {
                auto disabled=device.renderer(staged,false);
                require(!disabled->try_program(mirrored,diagnostic) && diagnostic.find("mirror-once")!=std::string::npos,
                    "Particle mirror-once addressing bypassed the enabled-capability boundary");
            }
            report["missing_mirror_capability_isolated"]=true;
            if(device.supports_mirror_clamp()) {
                require(renderer->try_program(mirrored,diagnostic).has_value(),"Enabled particle mirror-once sampler was rejected");
                report["enabled_mirror_capability_admitted"]=true;
            }
            require(!renderer->try_program(bad,diagnostic) && !diagnostic.empty(),"Missing particle bitmap was not isolated");
            require(renderer->take_resource_diagnostics().size()==1,"Rejected particle material did not report once");
            require(!renderer->try_program(bad,diagnostic) && renderer->take_resource_diagnostics().empty(),"Rejected particle material spammed diagnostics");
            report["content_failure_isolated"]=true;
            std::array<ParticleDraw,3> draws{};draws[0].material=source;draws[1].material=bad;draws[2].material=source;
            renderer->prepare(draws);device.record(*renderer,draws.size());
            require(renderer->view_inputs(0).draw==0 && renderer->view_inputs(2).draw==2,
                "Unavailable particle content changed valid source draw order");
            bool inactive=false;try{(void)renderer->view_inputs(1);}catch(const FormatError&){inactive=true;}
            require(inactive && !renderer->reads_scene_texture(1),"Unavailable draw retained a live view or scene lookup slot");
            require(renderer->take_resource_diagnostics().empty(),"Inactive prepared draw repeated its resource diagnostic");
            report["mixed_draw_order_preserved"]=true;
#ifdef HALO2_PARTICLE_TEST_LINK_WRAP
            auto unfinished=std::make_shared<ParticleMaterial>(*source);std::filesystem::remove(staged/fragment);
            const auto before=sampler_creations;require(!renderer->try_program(unfinished,diagnostic),"Missing fragment was admitted");
            const auto failed=sampler_creations;require(failed>before,"Failed fragment fixture did not reach actual partial GPU work");
            std::filesystem::copy_file(original/fragment,staged/fragment);
            require(!renderer->try_program(unfinished,diagnostic) && sampler_creations==failed,
                "Immutable rejected particle retried partial uploads during ordinary admission");
            draws[0].material=draws[2].material=unfinished;renderer->prepare(draws);
            require(sampler_creations==failed,"Inactive immutable material retried GPU work during frame preparation");
            require(renderer->program(unfinished).pipeline && renderer->try_program(unfinished,diagnostic),
                "Explicit repaired material retry did not clear its unavailable admission");
            require(renderer->take_resource_diagnostics().size()==1,"Missing fragment admission did not report once");
            report["immutable_failed_admission_cached"]=true;
            auto memory=std::make_shared<ParticleMaterial>(*source);next_sampler_failure=VK_ERROR_OUT_OF_DEVICE_MEMORY;
            bool critical=false;
            try{(void)renderer->try_program(memory,diagnostic);}catch(const ParticleResourceError&){throw FormatError("Device OOM was misclassified as particle content");}
            catch(const FormatError&){critical=true;}
            require(critical,"Device OOM was swallowed by particle content isolation");
            require(renderer->program(memory).pipeline,"GPU allocation failure poisoned the particle material retry");
            report["device_failure_propagated"]=true;
#endif
            std::set<unsigned> classifications;std::set<std::string> fragments;std::set<std::uint64_t> vertex_programs;
            unsigned admitted{},feedback{},catalog_rejected{};bool missing_scene_checked=false,missing_model_checked=false;
            report["resident_gpu_maps"]=nlohmann::json::array();
            report["resident_gpu_rejections"]=nlohmann::json::array();
            std::vector<std::filesystem::path> maps;
            for(int argument=3;argument<argc;++argument) {
                if(!*argv[argument])continue;
                const std::filesystem::path path=argv[argument];
                if(std::filesystem::is_directory(path)) {
                    for(const auto& entry:std::filesystem::directory_iterator(path)) {
                        const auto name=entry.path().stem().string();
                        if(entry.is_regular_file() && entry.path().extension()==".map" &&
                           name!="mainmenu" && name!="shared" && name!="single_player_shared")maps.push_back(entry.path());
                    }
                }else maps.push_back(path);
            }
            std::sort(maps.begin(),maps.end());maps.erase(std::unique(maps.begin(),maps.end()),maps.end());
            for(const auto& path:maps) {
                Cache cache(path,path.parent_path());
                require(cache.is_mcc(),"Particle GPU census requires actual prepared imported MCC content");
                // Each map owns a fresh material cache. The census cannot retain
                // every campaign's textures and pipelines in device memory.
                auto catalog=device.renderer(staged);unsigned map_admitted{},map_rejected{};
                const auto reject=[&](const Tag& tag,std::string_view stage,std::string_view message) {
                    ++catalog_rejected;++map_rejected;
                    if(report["resident_gpu_rejections"].size()<128)report["resident_gpu_rejections"].push_back({
                        {"map",cache.name()},{"particle",tag.name.substr(0,240)},{"stage",stage},{"diagnostic",message.substr(0,512)}});
                };
                for(const auto& tag:cache.tags()) {
                    if(!tag.address || (tag.group!="prt3" && tag.group!="PRTM"))continue;
                    std::shared_ptr<ParticleMaterial> resident;ParticleMaterialValidation contract;
                    // Loader errors at this CPU-only content boundary are
                    // recorded. GPU allocation/device exceptions below remain fatal.
                    try{resident=std::make_shared<ParticleMaterial>(cache.load_particle_material(tag.datum,15,0,tag.group=="PRTM"?1:0));}
                    catch(const FormatError& error){reject(tag,"source_material",error.what());continue;}
                    try{contract=validate_particle_material(*resident);}
                    catch(const ParticleResourceError& error){reject(tag,"compiled_contract",error.what());continue;}
                    const auto compiled=std::filesystem::path(contract.fragment_kernel);
                    if(!std::filesystem::exists(staged/compiled)) {
                        if(!std::filesystem::exists(original/compiled)){reject(tag,"compiled_fragment","Compiled custom Vulkan particle fragment is missing");continue;}
                        std::filesystem::create_directories((staged/compiled).parent_path());
                        std::filesystem::copy_file(original/compiled,staged/compiled);
                    }
                    const auto program=catalog->try_program(resident,diagnostic);
                    if(!program){reject(tag,"gpu_material",diagnostic);continue;}
                    ++admitted;++map_admitted;classifications.insert(contract.classification);vertex_programs.insert(contract.program_hash);
                    fragments.insert(std::string(contract.fragment_kernel));
                    if(contract.external_scene_texture)++feedback;
                    if(contract.external_scene_texture && !missing_scene_checked) {
                        ParticleDraw draw;draw.material=resident;draw.vertex.surface_fog={1,1,0,0};
                        catalog->prepare(std::span(&draw,1));
                        require(!catalog->reads_scene_texture(0),"Framebuffer particle became live without its current frame owner");
                        catalog->prepare(std::span(&draw,1),device.scene());
                        require(catalog->reads_scene_texture(0),"Draw-local missing framebuffer owner poisoned a valid material");
                        draw.vertex.surface_fog[0]=std::numeric_limits<float>::infinity();catalog->prepare(std::span(&draw,1),device.scene());
                        require(!catalog->reads_scene_texture(0),"Framebuffer particle admitted nonfinite surface ownership");
                        catalog->take_resource_diagnostics();missing_scene_checked=true;
                    }
                    if(contract.classification==1 && !missing_model_checked) {
                        ParticleDraw draw;draw.material=resident;catalog->prepare(std::span(&draw,1));
                        require(!catalog->reads_scene_texture(0) && catalog->try_program(resident,diagnostic),
                            "Draw-local missing model owner poisoned an admitted material");
                        device.record(*catalog,1);bool skipped=false;try{(void)catalog->view_inputs(0);}catch(const FormatError&){skipped=true;}
                        require(skipped,"Particle model with absent geometry recorded a live draw");
#ifdef HALO2_PARTICLE_TEST_LINK_WRAP
                        auto model=std::make_shared<ParticleModelDefinition>();model->vertices.resize(1);model->indices={0,0,0};
                        draw.model=model;draw.model_count=3;fail_buffer_creation=2;
                        bool critical_model=false;
                        try{catalog->prepare(std::span(&draw,1));}catch(const ParticleResourceError&){throw FormatError("Model device OOM was misclassified as content");}
                        catch(const FormatError&){critical_model=true;}
                        require(critical_model && !fail_buffer_creation,"Model fixture did not fail its second real GPU buffer allocation");
                        catalog->prepare(std::span(&draw,1));device.record(*catalog,1);
                        require(catalog->view_inputs(0).recorded,"Failed second model allocation poisoned its same-owner retry");
                        report["model_partial_allocation_recovered"]=true;
#endif
                        catalog->take_resource_diagnostics();missing_model_checked=true;
                    }
                }
                report["resident_gpu_maps"].push_back({{"map",cache.name()},{"admitted",map_admitted},{"rejected",map_rejected}});
                std::cout<<cache.name()<<": GPU-admitted "<<map_admitted<<", rejected "<<map_rejected<<'\n'<<std::flush;
            }
            report["resident_gpu_materials"]=admitted;report["classifications"]=classifications;
            report["compiled_fragments"]=fragments;report["vertex_programs"]=vertex_programs;
            report["scene_feedback_materials"]=feedback;report["missing_scene_owner_isolated"]=missing_scene_checked;
            report["missing_model_owner_isolated"]=missing_model_checked;
            report["resident_gpu_rejected"]=catalog_rejected;
            require(!catalog_rejected,"Resident particle GPU census found unsupported or missing content");
#endif
        }
        require(!validation_errors,"Particle admission generated Vulkan validation errors");
        report["validation_errors"]=validation_errors;report["passed"]=true;save();
        std::cout<<"PASS actual particle GPU admission, corrected-fragment retry and isolated content failures\n";return 0;
    }catch(const std::exception& error){report["error"]=error.what();report["validation_errors"]=validation_errors;save();std::cerr<<error.what()<<'\n';return 1;}
}
