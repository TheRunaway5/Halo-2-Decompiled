#include "vulkan_lens_retirement_observer.hpp"
#include <cstddef>
#include <vulkan/vulkan.h>

namespace {
thread_local bool observing{};
thread_local VulkanLensRetirementCalls calls;
}
void begin_vulkan_lens_retirement_observation() noexcept {calls={};observing=true;}
VulkanLensRetirementCalls end_vulkan_lens_retirement_observation() noexcept {observing=false;return calls;}
extern "C" {
VKAPI_ATTR VkResult VKAPI_CALL __real_vkDeviceWaitIdle(VkDevice);
VKAPI_ATTR void VKAPI_CALL __real_vkDestroyBuffer(VkDevice,VkBuffer,const VkAllocationCallbacks *);
VKAPI_ATTR void VKAPI_CALL __real_vkFreeMemory(VkDevice,VkDeviceMemory,const VkAllocationCallbacks *);
VKAPI_ATTR void VKAPI_CALL __real_vkDestroyPipeline(VkDevice,VkPipeline,const VkAllocationCallbacks *);
VKAPI_ATTR void VKAPI_CALL __real_vkDestroyPipelineLayout(VkDevice,VkPipelineLayout,const VkAllocationCallbacks *);
VKAPI_ATTR void VKAPI_CALL __real_vkDestroyQueryPool(VkDevice,VkQueryPool,const VkAllocationCallbacks *);
VKAPI_ATTR VkResult VKAPI_CALL __real_vkGetQueryPoolResults(VkDevice,VkQueryPool,std::uint32_t,std::uint32_t,
    std::size_t,void *,VkDeviceSize,VkQueryResultFlags);
VKAPI_ATTR VkResult VKAPI_CALL __wrap_vkDeviceWaitIdle(VkDevice device){
    if(observing)++calls.waits;return __real_vkDeviceWaitIdle(device);
}
VKAPI_ATTR void VKAPI_CALL __wrap_vkDestroyBuffer(VkDevice device,VkBuffer buffer,const VkAllocationCallbacks *allocator){
    if(observing)++calls.buffers;__real_vkDestroyBuffer(device,buffer,allocator);
}
VKAPI_ATTR void VKAPI_CALL __wrap_vkFreeMemory(VkDevice device,VkDeviceMemory memory,const VkAllocationCallbacks *allocator){
    if(observing)++calls.memory;__real_vkFreeMemory(device,memory,allocator);
}
VKAPI_ATTR void VKAPI_CALL __wrap_vkDestroyPipeline(VkDevice device,VkPipeline pipeline,const VkAllocationCallbacks *allocator){
    if(observing)++calls.pipelines;__real_vkDestroyPipeline(device,pipeline,allocator);
}
VKAPI_ATTR void VKAPI_CALL __wrap_vkDestroyPipelineLayout(VkDevice device,VkPipelineLayout layout,const VkAllocationCallbacks *allocator){
    if(observing)++calls.layouts;__real_vkDestroyPipelineLayout(device,layout,allocator);
}
VKAPI_ATTR void VKAPI_CALL __wrap_vkDestroyQueryPool(VkDevice device,VkQueryPool pool,const VkAllocationCallbacks *allocator){
    if(observing)++calls.pools;__real_vkDestroyQueryPool(device,pool,allocator);
}
VKAPI_ATTR VkResult VKAPI_CALL __wrap_vkGetQueryPoolResults(VkDevice device,VkQueryPool pool,std::uint32_t first,
    std::uint32_t count,std::size_t bytes,void *data,VkDeviceSize stride,VkQueryResultFlags flags){
    if(observing)++calls.query_reads;return __real_vkGetQueryPoolResults(device,pool,first,count,bytes,data,stride,flags);
}
}
