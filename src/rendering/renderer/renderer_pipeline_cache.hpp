#pragma once
#include <vulkan/vulkan.h>
#include <array>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <memory>
#include <mutex>
#include <tuple>
#include <vector>

namespace halo2::detail {
#if defined(__linux__) && defined(__GNUC__)
// Native Linux builds also retain a per-device disk cache. Other renderer
// clients need no link dependency on that optional executable-level owner.
extern "C" void halo2_native_pipeline_cache_merge(VkDevice,VkPipelineCache,bool) noexcept __attribute__((weak));
#endif
// Opaque driver data only. Keep it across the menu and map's logical devices;
// pipeline state and shader modules are still supplied in full to Vulkan.
class RendererPipelineCache {
  using Key=std::tuple<std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,
      std::array<std::uint8_t,VK_UUID_SIZE>>;
  struct Bank {
    std::mutex mutex;
    std::map<Key,std::shared_ptr<const std::vector<std::byte>>> entries;
  };
  static Bank& bank(){static Bank value;return value;}
  static constexpr std::size_t limit=32u*1024u*1024u;
  VkDevice device_{};
  VkPipelineCache cache_{};
  Key key_;
  void merge_native(bool publish) noexcept {
#if defined(__linux__) && defined(__GNUC__)
    if(cache_ && halo2_native_pipeline_cache_merge)halo2_native_pipeline_cache_merge(device_,cache_,publish);
#else
    (void)publish;
#endif
  }
public:
  RendererPipelineCache(VkDevice device,const VkPhysicalDeviceProperties& properties):device_(device) {
    std::array<std::uint8_t,VK_UUID_SIZE> uuid;
    std::copy_n(properties.pipelineCacheUUID,VK_UUID_SIZE,uuid.begin());
    key_={properties.vendorID,properties.deviceID,properties.driverVersion,properties.apiVersion,uuid};
    if(std::getenv("HALO2_REFERENCE_NO_PIPELINE_CACHE"))return;
    std::shared_ptr<const std::vector<std::byte>> bytes;
    {auto& b=bank();std::lock_guard lock(b.mutex);if(const auto it=b.entries.find(key_);it!=b.entries.end())bytes=it->second;}
    VkPipelineCacheCreateInfo info{VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
    if(bytes){info.initialDataSize=bytes->size();info.pInitialData=bytes->data();}
    bool restored=bool(bytes);
    if(vkCreatePipelineCache(device_,&info,nullptr,&cache_)!=VK_SUCCESS) {
      restored=false;
      cache_=VK_NULL_HANDLE;info.initialDataSize=0;info.pInitialData=nullptr;
      if(vkCreatePipelineCache(device_,&info,nullptr,&cache_)!=VK_SUCCESS)cache_=VK_NULL_HANDLE;
    }
    // A process snapshot already contains the first device's persisted bank.
    // Import it only once instead of merging the same large bank twice.
    if(!restored)merge_native(false);
  }
  RendererPipelineCache(const RendererPipelineCache&)=delete;
  RendererPipelineCache& operator=(const RendererPipelineCache&)=delete;
  ~RendererPipelineCache(){if(cache_){merge_native(true);vkDestroyPipelineCache(device_,cache_,nullptr);}}
  VkPipelineCache get()const noexcept{return cache_;}
  void publish() noexcept {
    if(!cache_)return;
    try {
      std::size_t size{};
      if(vkGetPipelineCacheData(device_,cache_,&size,nullptr)!=VK_SUCCESS || !size || size>limit)return;
      auto bytes=std::make_shared<std::vector<std::byte>>(size);
      if(vkGetPipelineCacheData(device_,cache_,&size,bytes->data())!=VK_SUCCESS || size>bytes->size())return;
      bytes->resize(size);
      auto& b=bank();std::lock_guard lock(b.mutex);
      const auto it=b.entries.find(key_);
      if(it!=b.entries.end() && it->second->size()>size)return;
      if(it==b.entries.end() && b.entries.size()>=4)b.entries.erase(b.entries.begin());
      b.entries[key_]=std::move(bytes);
    }catch(...){/* Optional cache exhaustion must not prevent rendering. */}
  }
};
}
