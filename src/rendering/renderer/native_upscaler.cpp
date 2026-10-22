#include "halo2/native_upscaler.hpp"
#include <SDL.h>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <mutex>
#include <unordered_map>
#include <stdexcept>
#include <nlohmann/json.hpp>
#ifdef HALO2_HAS_NGX
#include <nvsdk_ngx_helpers_vk.h>
#include <nvsdk_ngx_helpers.h>
#ifndef _WIN32
#include "rendering/renderer/dlss_vulkan_initialization.hpp"
#endif
#endif
namespace halo2 {
namespace {
#ifdef _WIN32
constexpr const char* platform="windows/x64";
constexpr const char* fsr_library="fsrbridge.dll";
constexpr const char* dlss_library="nvngx_dlss.dll";
#else
constexpr const char* platform="linux/x64";
constexpr const char* fsr_library="libfsrbridge.so";
constexpr const char* dlss_library="libnvidia-ngx-dlss.so.310.5.3";
#endif
std::filesystem::path native_directory() {
    char* base=SDL_GetBasePath();
    const auto beside=std::filesystem::path(base?base:"")/"upscalers"/platform;
    SDL_free(base);
    if(std::filesystem::exists(beside/fsr_library))return beside;
    return std::filesystem::path(HALO2_UPSCALER_DIR)/platform;
}
void checked(VkResult result,const char* operation) {
    if(result!=VK_SUCCESS)throw std::runtime_error(std::string(operation)+": Vulkan "+std::to_string(result));
}
template<class T> std::int64_t handle(T value) {return std::int64_t(reinterpret_cast<std::uintptr_t>(value));}
#ifdef HALO2_HAS_NGX
std::mutex ngx_mutex;
std::unordered_map<VkDevice,unsigned> ngx_devices;
void ngx_checked(NVSDK_NGX_Result result,const char* operation) {
    if(NVSDK_NGX_FAILED(result))throw std::runtime_error(std::string("DLSS ")+operation+": NGX "+std::to_string(unsigned(result)));
}
NVSDK_NGX_PerfQuality_Value ngx_quality(UpscalingQuality q) {
    switch(q) {
    case UpscalingQuality::NativeAA:return NVSDK_NGX_PerfQuality_Value_DLAA;
    case UpscalingQuality::Performance:return NVSDK_NGX_PerfQuality_Value_MaxPerf;
    case UpscalingQuality::UltraPerformance:return NVSDK_NGX_PerfQuality_Value_UltraPerformance;
    case UpscalingQuality::Balanced:return NVSDK_NGX_PerfQuality_Value_Balanced;
    default:return NVSDK_NGX_PerfQuality_Value_MaxQuality;
    }
}
std::filesystem::path ngx_cache_directory() {
    if(const char* scratch=std::getenv("HALO2_TEST_SCRATCH");scratch && *scratch) {
        auto cache=std::filesystem::path(scratch)/"ngx";std::filesystem::create_directories(cache);return cache;
    }
    char* pref=SDL_GetPrefPath("Halo2","upscaling");
    if(!pref)throw std::runtime_error(SDL_GetError());
    const std::filesystem::path cache=pref;SDL_free(pref);return cache;
}
struct NgxDiscovery {
    std::wstring directory=native_directory().wstring(),cache=ngx_cache_directory().wstring();
    const wchar_t* search=directory.c_str();
    NVSDK_NGX_FeatureCommonInfo common{};
    NVSDK_NGX_FeatureDiscoveryInfo info{};
    NgxDiscovery() {
        common.PathListInfo.Path=&search;common.PathListInfo.Length=1;
        info.SDKVersion=NVSDK_NGX_Version_API;info.FeatureID=NVSDK_NGX_Feature_SuperSampling;
        info.Identifier.IdentifierType=NVSDK_NGX_Application_Identifier_Type_Project_Id;
        info.Identifier.v.ProjectDesc={"a16922a3-7083-4e61-a370-724d69fcff17",NVSDK_NGX_ENGINE_TYPE_CUSTOM,"Halo2Native"};
        info.ApplicationDataPath=cache.c_str();info.FeatureInfo=&common;
    }
};
#endif
}
struct NativeUpscaler::Impl {
    VkDevice device{};VkExtent2D input{},output{};Upscaler method{};
    void* library{};std::int64_t fsr{};
    // The donor's portable entry points do not access JNIEnv in these three
    // functions. Preserve the exported JNI scalar ABI, without loading Java.
    using FsrCreate=std::int64_t(*)(void*,void*,std::int64_t,std::int64_t,std::int64_t,int,int,int,int,int);
    using FsrDestroy=void(*)(void*,void*,std::int64_t);
    using FsrDispatch=int(*)(void*,void*,std::int64_t,std::int64_t,std::int64_t,std::int64_t,std::int64_t,std::int64_t,std::int64_t,
        int,int,int,int,int,int,int,int,int,float,float,float,float,float,float,float,float,float,float,unsigned char);
    FsrDestroy fsr_destroy{};FsrDispatch fsr_dispatch{};
#ifdef HALO2_HAS_NGX
    bool ngx_initialized{};NVSDK_NGX_Parameter* parameters{};NVSDK_NGX_Handle* feature{};
#ifndef _WIN32
    std::unique_ptr<detail::DlssVulkanInitialization> initialization;
#endif
#endif
    ~Impl() {
        if(fsr && fsr_destroy)fsr_destroy(nullptr,nullptr,fsr);
        if(library)SDL_UnloadObject(library);
#ifdef HALO2_HAS_NGX
        if(feature)NVSDK_NGX_VULKAN_ReleaseFeature(feature);
        if(parameters)NVSDK_NGX_VULKAN_DestroyParameters(parameters);
        if(ngx_initialized) {
            std::lock_guard lock(ngx_mutex);
            if(--ngx_devices.at(device)==0){NVSDK_NGX_VULKAN_Shutdown1(device);ngx_devices.erase(device);}
        }
#endif
    }
};
std::string NativeUpscaler::availability(VkPhysicalDevice physical,Upscaler method) {
    if(method==Upscaler::Off)return {};
    if(!std::filesystem::is_regular_file(native_directory()/(method==Upscaler::FSR31?fsr_library:dlss_library)))
        return std::string(upscaler_name(method))+" runtime is missing.";
    if(method==Upscaler::DLSS45) {
#ifndef HALO2_HAS_NGX
        return "DLSS is unavailable on this platform.";
#else
        VkPhysicalDeviceProperties props{};vkGetPhysicalDeviceProperties(physical,&props);
        if(props.vendorID!=0x10de)return "DLSS requires an NVIDIA RTX graphics card. Select FSR 3.1 on this GPU.";
#endif
    }
    return {};
}
std::vector<std::string> NativeUpscaler::instance_extensions() {
    std::vector<std::string> result;
#ifdef HALO2_HAS_NGX
    NgxDiscovery discovery;unsigned count{};VkExtensionProperties* properties{};
    if(NVSDK_NGX_SUCCEED(NVSDK_NGX_VULKAN_GetFeatureInstanceExtensionRequirements(&discovery.info,&count,&properties))) {
        for(unsigned i=0;i<count;++i)result.emplace_back(properties[i].extensionName);
        return result;
    }
    unsigned ic{},dc{};const char **inames{},**dnames{};
    if(NVSDK_NGX_SUCCEED(NVSDK_NGX_VULKAN_RequiredExtensions(&ic,&inames,&dc,&dnames)))
        for(unsigned i=0;i<ic;++i)result.emplace_back(inames[i]);
#endif
    return result;
}
std::vector<std::string> NativeUpscaler::device_extensions(VkInstance instance,VkPhysicalDevice physical) {
    std::vector<std::string> result;
#ifdef HALO2_HAS_NGX
    NgxDiscovery discovery;unsigned count{};VkExtensionProperties* properties{};
    if(NVSDK_NGX_SUCCEED(NVSDK_NGX_VULKAN_GetFeatureDeviceExtensionRequirements(instance,physical,&discovery.info,&count,&properties))) {
        for(unsigned i=0;i<count;++i)result.emplace_back(properties[i].extensionName);
        return result;
    }
    unsigned ic{},dc{};const char **inames{},**dnames{};
    if(NVSDK_NGX_SUCCEED(NVSDK_NGX_VULKAN_RequiredExtensions(&ic,&inames,&dc,&dnames)))
        for(unsigned i=0;i<dc;++i)result.emplace_back(dnames[i]);
#endif
    return result;
}
NativeUpscaler::NativeUpscaler(VkInstance instance,VkPhysicalDevice physical,VkDevice device,VkQueue queue,unsigned family,
    VkExtent2D input,VkExtent2D output,Upscaler method,UpscalingQuality quality):impl_(std::make_unique<Impl>()) {
    auto& p=*impl_;p.device=device;p.input=input;p.output=output;p.method=method;
    if(const auto error=availability(physical,method);!error.empty())throw std::runtime_error(error);
    if(method==Upscaler::FSR31) {
        p.library=SDL_LoadObject((native_directory()/fsr_library).string().c_str());
        if(!p.library)throw std::runtime_error(SDL_GetError());
        auto symbol=[&](const char* name) {auto f=SDL_LoadFunction(p.library,name);if(!f)throw std::runtime_error(SDL_GetError());return f;};
        const auto create=reinterpret_cast<Impl::FsrCreate>(symbol("Java_net_minecraft_src_graphics_fsr_FsrNativeBridge_nCreateContext"));
        p.fsr_destroy=reinterpret_cast<Impl::FsrDestroy>(symbol("Java_net_minecraft_src_graphics_fsr_FsrNativeBridge_nDestroyContext"));
        p.fsr_dispatch=reinterpret_cast<Impl::FsrDispatch>(symbol("Java_net_minecraft_src_graphics_fsr_FsrNativeBridge_nDispatchUpscale"));
        p.fsr=create(nullptr,nullptr,handle(instance),handle(physical),handle(device),input.width,input.height,output.width,output.height,0);
        if(!p.fsr)throw std::runtime_error("FSR 3.1 could not create its Vulkan context.");
        return;
    }
#ifdef HALO2_HAS_NGX
    if(method!=Upscaler::DLSS45)throw std::runtime_error("Invalid upscaler");
    const auto cache=ngx_cache_directory();
#ifndef _WIN32
    if(!std::getenv("__NGX_CONF_FILE")) {
        const auto config=cache/"nvidia-ngx-conf.json";
        std::ofstream(config)<<nlohmann::json{{"file_format_version","1.0.0"},{"ngx_models_path",cache.string()},{"allow_ngx_updater",false}}.dump(2);
        SDL_setenv("__NGX_CONF_FILE",config.string().c_str(),0);
    }
#endif
    const auto directory=native_directory().wstring(),appdata=cache.wstring();
    const wchar_t* search=directory.c_str();NVSDK_NGX_FeatureCommonInfo common{};
    common.PathListInfo.Path=&search;common.PathListInfo.Length=1;
    {
        std::lock_guard lock(ngx_mutex);
        if(!ngx_devices.contains(device)) {
            const auto instance_proc=
#ifndef _WIN32
                detail::DlssVulkanInitialization::instance_proc;
            const auto device_proc=detail::DlssVulkanInitialization::device_proc;
#else
                vkGetInstanceProcAddr;
            const auto device_proc=vkGetDeviceProcAddr;
#endif
            ngx_checked(NVSDK_NGX_VULKAN_Init_with_ProjectID("a16922a3-7083-4e61-a370-724d69fcff17",NVSDK_NGX_ENGINE_TYPE_CUSTOM,
                "Halo2Native",appdata.c_str(),instance,physical,device,instance_proc,device_proc,&common),"initialization");
        }
        ++ngx_devices[device];p.ngx_initialized=true;
    }
    ngx_checked(NVSDK_NGX_VULKAN_GetCapabilityParameters(&p.parameters),"capabilities");
    int available{};NVSDK_NGX_Parameter_GetI(p.parameters,NVSDK_NGX_Parameter_SuperSampling_Available,&available);
    if(!available)throw std::runtime_error("DLSS Super Resolution is unavailable on this GPU/driver.");
    const unsigned preset=quality==UpscalingQuality::Performance?NVSDK_NGX_DLSS_Hint_Render_Preset_M:
        quality==UpscalingQuality::UltraPerformance?NVSDK_NGX_DLSS_Hint_Render_Preset_L:NVSDK_NGX_DLSS_Hint_Render_Preset_K;
    for(auto key:{NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_DLAA,NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Quality,
        NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Balanced,NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Performance,
        NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraPerformance})NVSDK_NGX_Parameter_SetUI(p.parameters,key,preset);
    NVSDK_NGX_DLSS_Create_Params create{};
    create.Feature={input.width,input.height,output.width,output.height,ngx_quality(quality)};
    if(quality==UpscalingQuality::UltraQuality) {
        unsigned w{},h{},maxw{},maxh{},minw{},minh{};float sharp{};
        ngx_checked(NGX_DLSS_GET_OPTIMAL_SETTINGS(p.parameters,output.width,output.height,ngx_quality(quality),
            &w,&h,&maxw,&maxh,&minw,&minh,&sharp),"Ultra Quality sizing");
        if(input.width<minw || input.width>maxw || input.height<minh || input.height>maxh)
            throw std::runtime_error("DLSS Ultra Quality is unavailable at this output resolution.");
        create.Feature.InWidth=w;create.Feature.InHeight=h;
    }
    create.InFeatureCreateFlags=NVSDK_NGX_DLSS_Feature_Flags_AutoExposure|NVSDK_NGX_DLSS_Feature_Flags_MVLowRes;
    VkCommandPool pool{};VkCommandBuffer command{};
    struct Cleanup {VkDevice device;VkCommandPool& pool;~Cleanup(){if(pool)vkDestroyCommandPool(device,pool,nullptr);}} cleanup{device,pool};
    VkCommandPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};pool_info.queueFamilyIndex=family;
    checked(vkCreateCommandPool(device,&pool_info,nullptr,&pool),"create DLSS initialization pool");
    VkCommandBufferAllocateInfo allocate{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};allocate.commandPool=pool;allocate.commandBufferCount=1;
    checked(vkAllocateCommandBuffers(device,&allocate,&command),"allocate DLSS initialization");
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    checked(vkBeginCommandBuffer(command,&begin),"begin DLSS initialization");
    {
#ifndef _WIN32
        if(quality==UpscalingQuality::Performance || quality==UpscalingQuality::UltraPerformance)
            p.initialization=std::make_unique<detail::DlssVulkanInitialization>(device,input);
        detail::DlssVulkanInitialization::Capture capture{p.initialization.get()};
#endif
        ngx_checked(NGX_VULKAN_CREATE_DLSS_EXT(command,1,1,&p.feature,p.parameters,&create),"feature creation");
    }
    checked(vkEndCommandBuffer(command),"end DLSS initialization");
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;
    checked(vkQueueSubmit(queue,1,&submit,VK_NULL_HANDLE),"submit DLSS initialization");
    checked(vkQueueWaitIdle(queue),"wait DLSS initialization");
#else
    throw std::runtime_error("DLSS is unavailable in this build.");
#endif
}
NativeUpscaler::~NativeUpscaler()=default;
void NativeUpscaler::dispatch(const UpscalingDispatch& d) {
    auto& p=*impl_;
    if(p.method==Upscaler::FSR31) {
        const int result=p.fsr_dispatch(nullptr,nullptr,p.fsr,handle(d.command),handle(d.color.image),handle(d.depth.image),
            handle(d.motion.image),handle(d.reactive.image),handle(d.output.image),d.color.format,d.depth.format,d.motion.format,
            d.reactive.format,d.output.format,p.input.width,p.input.height,p.output.width,p.output.height,
            d.jitter_x,d.jitter_y,1.f,1.f,d.delta_seconds,0.f,d.near_plane,d.far_plane,d.vertical_fov,1.f,d.reset);
        if(result)throw std::runtime_error("FSR 3.1 dispatch failed: "+std::to_string(result));
        return;
    }
#ifdef HALO2_HAS_NGX
    auto resource=[](UpscalingImage image,VkExtent2D extent,bool write) {
        const VkImageSubresourceRange range{image.format==VK_FORMAT_D32_SFLOAT?VK_IMAGE_ASPECT_DEPTH_BIT:VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
        return NVSDK_NGX_Create_ImageView_Resource_VK(image.view,image.image,range,image.format,extent.width,extent.height,write);
    };
    auto color=resource(d.color,p.input,false),depth=resource(d.depth,p.input,false),motion=resource(d.motion,p.input,false),
        reactive=resource(d.reactive,p.input,false),output=resource(d.output,p.output,true);
    NVSDK_NGX_VK_DLSS_Eval_Params eval{};eval.Feature.pInColor=&color;eval.Feature.pInOutput=&output;
    eval.pInDepth=&depth;eval.pInMotionVectors=&motion;eval.pInBiasCurrentColorMask=d.reactive.image?&reactive:nullptr;
    eval.InJitterOffsetX=d.jitter_x;eval.InJitterOffsetY=d.jitter_y;eval.InReset=d.reset;
    eval.InMVScaleX=eval.InMVScaleY=1.f;eval.InPreExposure=eval.InExposureScale=1.f;
    eval.InRenderSubrectDimensions={p.input.width,p.input.height};eval.InFrameTimeDeltaInMsec=d.delta_seconds*1000;
#ifndef _WIN32
    if(p.initialization)p.initialization->prepare(d.command);
    detail::DlssVulkanInitialization::Capture capture{p.initialization.get()};
#endif
    ngx_checked(NGX_VULKAN_EVALUATE_DLSS_EXT(d.command,p.feature,p.parameters,&eval),"dispatch");
#endif
}
}
