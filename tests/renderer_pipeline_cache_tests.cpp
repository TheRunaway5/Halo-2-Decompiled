#include "rendering/renderer/renderer_pipeline_cache.hpp"
#include <cstring>
#include <iostream>
#include <stdexcept>
namespace {
std::vector<std::byte> initial,driver_data(48,std::byte{0x31});
unsigned creates{},destroys{},fail_creates{},imports{},exports{};
bool fail_query{},incomplete{};
void check(bool value,const char* why){if(!value)throw std::runtime_error(why);}
}
extern "C" void halo2_native_pipeline_cache_merge(VkDevice,VkPipelineCache,bool publish) noexcept {
  if(publish)++exports;else ++imports;
}
extern "C" VKAPI_ATTR VkResult VKAPI_CALL vkCreatePipelineCache(VkDevice,const VkPipelineCacheCreateInfo* info,
    const VkAllocationCallbacks*,VkPipelineCache* out) {
  ++creates;initial.clear();
  if(info->initialDataSize)initial.assign(static_cast<const std::byte*>(info->pInitialData),
      static_cast<const std::byte*>(info->pInitialData)+info->initialDataSize);
  if(fail_creates){--fail_creates;return VK_ERROR_OUT_OF_HOST_MEMORY;}
  *out=(VkPipelineCache)(std::uintptr_t)1;return VK_SUCCESS;
}
extern "C" VKAPI_ATTR void VKAPI_CALL vkDestroyPipelineCache(VkDevice,VkPipelineCache,const VkAllocationCallbacks*){++destroys;}
extern "C" VKAPI_ATTR VkResult VKAPI_CALL vkGetPipelineCacheData(VkDevice,VkPipelineCache,std::size_t* size,void* data) {
  if(fail_query)return VK_ERROR_OUT_OF_HOST_MEMORY;
  if(!data){*size=driver_data.size();return VK_SUCCESS;}
  std::memcpy(data,driver_data.data(),std::min(*size,driver_data.size()));*size=driver_data.size();
  return incomplete?VK_INCOMPLETE:VK_SUCCESS;
}
int main()try {
  using halo2::detail::RendererPipelineCache;
  VkPhysicalDeviceProperties properties{};properties.vendorID=0x1002;properties.deviceID=7;
  properties.driverVersion=9;properties.apiVersion=VK_API_VERSION_1_0;properties.pipelineCacheUUID[0]=1;
  {RendererPipelineCache cache({},properties);check(initial.empty() && cache.get(),"First device did not start empty");cache.publish();check(imports==1 && !exports,"Cache publication repeated the native import/export merge");}
  {RendererPipelineCache cache({},properties);check(initial==driver_data,"Exact driver cache bytes were not replayed");
    check(imports==1 && exports==1,"Warm process cache redundantly imported the native bank");}
  for(unsigned i=0;i<5;++i){auto other=properties;
    switch(i){case 0:other.vendorID=0x8086;break;case 1:++other.deviceID;break;case 2:++other.driverVersion;break;
      case 3:++other.apiVersion;break;default:++other.pipelineCacheUUID[0];}
    RendererPipelineCache cache({},other);check(initial.empty(),"Foreign device/driver cache bytes were reused");}
  {fail_creates=1;const auto before=creates;RendererPipelineCache cache({},properties);
    check(cache.get() && creates==before+2 && initial.empty(),"Failed initial cache did not retry empty");}
  {fail_creates=2;const auto before=destroys;RendererPipelineCache cache({},properties);cache.publish();
    check(!cache.get() && destroys==before,"Cache failure prevented uncached rendering");}
  {RendererPipelineCache cache({},properties);fail_query=true;cache.publish();fail_query=false;
    driver_data.resize(64);incomplete=true;cache.publish();incomplete=false;}
  {RendererPipelineCache cache({},properties);check(initial.size()==48,"Incomplete/error result replaced valid driver cache");
    driver_data.resize(33u*1024u*1024u);cache.publish();}
  {RendererPipelineCache cache({},properties);check(initial.size()==48,"Oversized cache was retained");}
  check(destroys==creates-3,"Cache handles were not destroyed exactly once");
  std::cout<<"PASS exact pipeline cache replay, device/driver isolation, API failure fallback, bounded data and lifetime\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
