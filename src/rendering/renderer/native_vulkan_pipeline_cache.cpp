// Linux link wrappers keep retained renderer objects intact. Only an absent
// optional Vulkan pipeline-cache argument is replaced; pipeline descriptions,
// shader modules, allocators, results and rendering remain owned by the caller.
#include <vulkan/vulkan.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <atomic>
#include <bit>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <mutex>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>

namespace {
using Clock=std::chrono::steady_clock;
constexpr std::size_t maximum_bytes=64u*1024u*1024u;
static_assert(sizeof(VkPipelineCacheHeaderVersionOne)==32);
static_assert(std::endian::native==std::endian::little);
bool enabled(const char* name){const auto* value=std::getenv(name);return value && std::strcmp(value,"1")==0;}
std::filesystem::path directory() {
    std::filesystem::path root;
    if(const auto* p=std::getenv("HALO2_PIPELINE_CACHE_DIRECTORY");p && *p)root=p;
    else if(const auto* p=std::getenv("XDG_CACHE_HOME");p && *p)root=std::filesystem::path(p)/"halo2-native/vulkan";
    else if(const auto* p=std::getenv("HOME");p && *p)root=std::filesystem::path(p)/".cache/halo2-native/vulkan";
    return root.is_absolute()?root:std::filesystem::path{};
}
std::filesystem::path filename(const VkPhysicalDeviceProperties& properties) {
    const auto root=directory();if(root.empty())return {};
    std::ostringstream name;name<<std::hex<<std::setfill('0')<<std::setw(8)<<properties.vendorID<<'-'
        <<std::setw(8)<<properties.deviceID<<'-'<<std::setw(8)<<properties.driverVersion<<'-';
    for(auto b:properties.pipelineCacheUUID)name<<std::setw(2)<<unsigned(b);
    return root/(name.str()+".bin");
}
bool valid(std::span<const std::byte> bytes,const VkPhysicalDeviceProperties& properties) {
    if(bytes.size()<sizeof(VkPipelineCacheHeaderVersionOne) || bytes.size()>maximum_bytes)return false;
    VkPipelineCacheHeaderVersionOne header;std::memcpy(&header,bytes.data(),sizeof(header));
    return header.headerSize==sizeof(header) && header.headerVersion==VK_PIPELINE_CACHE_HEADER_VERSION_ONE &&
        header.vendorID==properties.vendorID && header.deviceID==properties.deviceID &&
        std::memcmp(header.pipelineCacheUUID,properties.pipelineCacheUUID,VK_UUID_SIZE)==0;
}
std::vector<std::byte> load(const std::filesystem::path& path,const VkPhysicalDeviceProperties& properties) {
    if(path.empty())return {};
    std::ifstream file(path,std::ios::binary|std::ios::ate);if(!file)return {};
    const auto size=file.tellg();if(size<32 || size>std::streamoff(maximum_bytes))return {};
    std::vector<std::byte> bytes(static_cast<std::size_t>(size));file.seekg(0);
    if(!file.read(reinterpret_cast<char*>(bytes.data()),size) || !valid(bytes,properties))return {};
    return bytes;
}
void save(const std::filesystem::path& path,std::span<const std::byte> bytes) {
    if(path.empty())return;
    std::filesystem::create_directories(path.parent_path());
    static std::atomic<unsigned long long> serial{};
    const auto temporary=path.string()+".tmp-"+std::to_string(::getpid())+'-'+std::to_string(serial++);
    const int descriptor=::open(temporary.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC,0600);
    if(descriptor<0)return;
    struct Cleanup {int descriptor;const std::string& path;~Cleanup(){::close(descriptor);::unlink(path.c_str());}} cleanup{descriptor,temporary};
    std::size_t at{};
    while(at<bytes.size()) {
        const auto n=::write(descriptor,bytes.data()+at,bytes.size()-at);
        if(n<0 && errno==EINTR)continue;
        if(n<=0)return;
        at+=std::size_t(n);
    }
    if(::fsync(descriptor)==0)::rename(temporary.c_str(),path.c_str());
}
struct Entry {
    std::mutex cache_mutex;
    VkDevice device{};
    VkPipelineCache cache{};
    VkPhysicalDeviceProperties properties{};
    std::filesystem::path path;
    bool trace{};
    std::size_t loaded_bytes{};
    std::atomic<unsigned> graphics_calls{},compute_calls{};
    std::atomic<long long> pipeline_nanoseconds{};
    ~Entry(){if(cache)vkDestroyPipelineCache(device,cache,nullptr);}
    void finish() noexcept {
        std::lock_guard lock(cache_mutex);
        try {
            std::size_t size{};std::size_t saved{};
            if(vkGetPipelineCacheData(device,cache,&size,nullptr)==VK_SUCCESS && size>=32 && size<=maximum_bytes) {
                std::vector<std::byte> bytes(size);
                if(vkGetPipelineCacheData(device,cache,&size,bytes.data())==VK_SUCCESS && size<=bytes.size()) {
                    bytes.resize(size);if(valid(bytes,properties)){save(path,bytes);saved=size;}
                }
            }
            if(trace)std::fprintf(stderr,"Native pipeline cache: loaded_bytes=%zu exported_bytes=%zu graphics=%u compute=%u pipeline_seconds=%.6f\n",loaded_bytes,saved,graphics_calls.load(),compute_calls.load(),double(pipeline_nanoseconds.load())/1e9);
        } catch(...) { /* Optional driver residency must never prevent device teardown. */ }
        vkDestroyPipelineCache(device,cache,nullptr);cache=VK_NULL_HANDLE;
    }
};
struct Registry {std::mutex mutex;std::unordered_map<VkDevice,std::shared_ptr<Entry>> entries;};
Registry& registry(){
    // A device can be retired by a static owner constructed before its first
    // Vulkan call. Keep lookup state alive through that owner's destructor;
    // wrapped device teardown still exports and retires each driver cache.
    static auto* const value=new Registry;
    return *value;
}
std::shared_ptr<Entry> lookup(VkDevice device) {
    auto& r=registry();std::lock_guard lock(r.mutex);
    const auto it=r.entries.find(device);return it==r.entries.end()?nullptr:it->second;
}
void attach(VkPhysicalDevice physical,VkDevice device) {
    if(enabled("HALO2_DISABLE_PIPELINE_CACHE"))return;
    auto entry=std::make_shared<Entry>();entry->device=device;entry->trace=enabled("HALO2_PIPELINE_CACHE_TRACE");
    vkGetPhysicalDeviceProperties(physical,&entry->properties);entry->path=filename(entry->properties);
    auto bytes=load(entry->path,entry->properties);entry->loaded_bytes=bytes.size();
    VkPipelineCacheCreateInfo info{VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};info.initialDataSize=bytes.size();info.pInitialData=bytes.data();
    auto result=vkCreatePipelineCache(device,&info,nullptr,&entry->cache);
    if(result!=VK_SUCCESS && !bytes.empty()) {
        entry->cache=VK_NULL_HANDLE;entry->loaded_bytes=0;info.initialDataSize=0;info.pInitialData=nullptr;
        result=vkCreatePipelineCache(device,&info,nullptr,&entry->cache);
    }
    if(result!=VK_SUCCESS){entry->cache=VK_NULL_HANDLE;return;}
    auto& r=registry();std::lock_guard lock(r.mutex);r.entries.emplace(device,std::move(entry));
}
template<class Info>std::shared_ptr<Entry> optional_cache(VkDevice device,VkPipelineCache supplied,std::uint32_t count,const Info* info) {
    if(supplied || !count || !info)return {};
    // Binary/capture-data extension chains can explicitly require a null cache.
    // Preserve every chained request verbatim through its existing path.
    for(std::uint32_t i=0;i<count;++i)if(info[i].pNext)return {};
    return lookup(device);
}
}
extern "C" {
void halo2_native_pipeline_cache_merge(VkDevice device,VkPipelineCache supplied,bool publish) noexcept {
    try {
        const auto entry=lookup(device);
        if(!entry || !supplied || supplied==entry->cache)return;
        // Merge requires external synchronization of its destination, even
        // for an otherwise internally synchronized Vulkan1.0 cache. Share the
        // lock with every intercepted creation using this device cache.
        std::lock_guard lock(entry->cache_mutex);
        const auto source=publish?supplied:entry->cache;
        const auto destination=publish?entry->cache:supplied;
        (void)vkMergePipelineCaches(device,destination,1,&source);
    }catch(...){/* Cache cooperation is optional. */}
}
VKAPI_ATTR VkResult VKAPI_CALL __real_vkCreateDevice(VkPhysicalDevice,const VkDeviceCreateInfo*,const VkAllocationCallbacks*,VkDevice*);
VKAPI_ATTR void VKAPI_CALL __real_vkDestroyDevice(VkDevice,const VkAllocationCallbacks*);
VKAPI_ATTR VkResult VKAPI_CALL __real_vkCreateGraphicsPipelines(VkDevice,VkPipelineCache,std::uint32_t,const VkGraphicsPipelineCreateInfo*,const VkAllocationCallbacks*,VkPipeline*);
VKAPI_ATTR VkResult VKAPI_CALL __real_vkCreateComputePipelines(VkDevice,VkPipelineCache,std::uint32_t,const VkComputePipelineCreateInfo*,const VkAllocationCallbacks*,VkPipeline*);
VKAPI_ATTR VkResult VKAPI_CALL __wrap_vkCreateDevice(VkPhysicalDevice physical,const VkDeviceCreateInfo* info,const VkAllocationCallbacks* allocator,VkDevice* output) {
    const auto result=__real_vkCreateDevice(physical,info,allocator,output);
    if(result==VK_SUCCESS && output && *output)try{attach(physical,*output);}catch(...){}
    return result;
}
VKAPI_ATTR void VKAPI_CALL __wrap_vkDestroyDevice(VkDevice device,const VkAllocationCallbacks* allocator) {
    std::shared_ptr<Entry> entry;
    try {auto& r=registry();std::lock_guard lock(r.mutex);const auto it=r.entries.find(device);
        if(it!=r.entries.end()){entry=std::move(it->second);r.entries.erase(it);}}
    catch(...){}
    if(entry)entry->finish();
    __real_vkDestroyDevice(device,allocator);
}
VKAPI_ATTR VkResult VKAPI_CALL __wrap_vkCreateGraphicsPipelines(VkDevice device,VkPipelineCache supplied,std::uint32_t count,const VkGraphicsPipelineCreateInfo* info,const VkAllocationCallbacks* allocator,VkPipeline* output) {
    std::shared_ptr<Entry> entry;try{entry=optional_cache(device,supplied,count,info);}catch(...){}
    if(!entry)return __real_vkCreateGraphicsPipelines(device,supplied,count,info,allocator,output);
    std::lock_guard lock(entry->cache_mutex);
    const auto start=entry->trace?Clock::now():Clock::time_point{};
    const auto result=__real_vkCreateGraphicsPipelines(device,entry->cache,count,info,allocator,output);
    if(entry->trace){++entry->graphics_calls;entry->pipeline_nanoseconds+=std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now()-start).count();}
    return result;
}
VKAPI_ATTR VkResult VKAPI_CALL __wrap_vkCreateComputePipelines(VkDevice device,VkPipelineCache supplied,std::uint32_t count,const VkComputePipelineCreateInfo* info,const VkAllocationCallbacks* allocator,VkPipeline* output) {
    std::shared_ptr<Entry> entry;try{entry=optional_cache(device,supplied,count,info);}catch(...){}
    if(!entry)return __real_vkCreateComputePipelines(device,supplied,count,info,allocator,output);
    std::lock_guard lock(entry->cache_mutex);
    const auto start=entry->trace?Clock::now():Clock::time_point{};
    const auto result=__real_vkCreateComputePipelines(device,entry->cache,count,info,allocator,output);
    if(entry->trace){++entry->compute_calls;entry->pipeline_nanoseconds+=std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now()-start).count();}
    return result;
}
}
