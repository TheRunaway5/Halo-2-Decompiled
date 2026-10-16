// Linux capture-only negative control. Intercept only the capped wrapping
// z-fail volume pipeline; all game state, light uniforms and receiver draws
// remain unchanged. This library is never linked into the game.
#include <vulkan/vulkan.h>
#include <dlfcn.h>
#include <cstdlib>
#include <fstream>
#include <map>
#include <mutex>
#include <set>
namespace {
std::mutex lock;
std::set<VkPipeline> volumes;
std::map<VkCommandBuffer,bool> bound;
std::size_t omitted{},vertices{};
template<class T>T next(const char* name){return reinterpret_cast<T>(dlsym(RTLD_NEXT,name));}
struct Report {~Report() {
    if(const char* path=std::getenv("HALO2_PSTENCIL_CONTROL_REPORT")) {
        std::ofstream f(path);f<<"{\"omitted_volume_draws\":"<<omitted<<",\"omitted_vertices\":"<<vertices<<"}\n";
    }
}} report;
}
extern "C" VKAPI_ATTR VkResult VKAPI_CALL vkCreateGraphicsPipelines(VkDevice device,VkPipelineCache cache,
    std::uint32_t count,const VkGraphicsPipelineCreateInfo* info,const VkAllocationCallbacks* allocator,VkPipeline* output) {
    static const auto real=next<PFN_vkCreateGraphicsPipelines>("vkCreateGraphicsPipelines");
    const auto result=real(device,cache,count,info,allocator,output);
    if(result==VK_SUCCESS) {std::lock_guard guard(lock);for(unsigned n=0;n<count;++n) {
        const auto* d=info[n].pDepthStencilState;const auto* color=info[n].pColorBlendState;
        if(!d || !color || !d->stencilTestEnable || d->depthWriteEnable || color->attachmentCount!=1 ||
           color->pAttachments[0].colorWriteMask || d->front.writeMask!=255 || d->back.writeMask!=255)continue;
        if(d->front.depthFailOp==VK_STENCIL_OP_DECREMENT_AND_WRAP &&
           d->back.depthFailOp==VK_STENCIL_OP_INCREMENT_AND_WRAP)volumes.insert(output[n]);
    }}
    return result;
}
extern "C" VKAPI_ATTR void VKAPI_CALL vkCmdBindPipeline(VkCommandBuffer command,VkPipelineBindPoint point,VkPipeline pipeline) {
    static const auto real=next<PFN_vkCmdBindPipeline>("vkCmdBindPipeline");
    if(point==VK_PIPELINE_BIND_POINT_GRAPHICS) {std::lock_guard guard(lock);bound[command]=volumes.contains(pipeline);}
    real(command,point,pipeline);
}
extern "C" VKAPI_ATTR void VKAPI_CALL vkCmdDraw(VkCommandBuffer command,std::uint32_t vertex_count,
    std::uint32_t instances,std::uint32_t first,std::uint32_t first_instance) {
    static const auto real=next<PFN_vkCmdDraw>("vkCmdDraw");
    {std::lock_guard guard(lock);if(bound[command]){++omitted;vertices+=std::size_t(vertex_count)*instances;return;}}
    real(command,vertex_count,instances,first,first_instance);
}
