#pragma once
#include <vulkan/vulkan.h>
#include <array>
#include <cerrno>
#include <cfenv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <stdexcept>
#include <string>
#include <vector>

namespace halo2::detail {
// Optional, same-queue profiling only. The caller owns device/fence lifetime:
// harvest after the existing frame fence completes, begin outside render passes,
// and submitted only after successful queue submission. After device idle, call
// harvest once more and destroy this owner before destroying the VkDevice.
// No method adds a Vulkan wait. TOP/BOTTOM markers report approximate elapsed
// GPU intervals, not shader-stage occupancy or display/presentation latency.
// https://docs.vulkan.org/samples/latest/samples/api/timestamp_queries/README.html
class RendererGpuTimings {
    static constexpr std::uint32_t query_count=4;
    VkDevice device_{};
    VkQueryPool pool_{};
    VkCommandBuffer recording_{};
    std::uint32_t next_marker_{};
    std::uint32_t max_dimension_{};
    std::uint64_t mask_{},serial_{};
    std::uint32_t width_{},height_{},views_{};
    double period_ns_{};
    bool pending_{},has_serial_{};
    std::ofstream report_;

    static void checked(VkResult result,const char *operation) {
        if(result!=VK_SUCCESS)
            throw std::runtime_error(std::string(operation)+": "+std::to_string(int(result)));
    }
    struct CpuEnvironment {
        std::fenv_t saved{};
        int error=errno;
        CpuEnvironment() {
            if(std::fegetenv(&saved)!=0)throw std::runtime_error("Save GPU timing CPU environment");
        }
        ~CpuEnvironment(){std::fesetenv(&saved);errno=error;}
    };
public:
    RendererGpuTimings(VkDevice device,VkPhysicalDevice physical,std::uint32_t queue_family,
                       const std::filesystem::path &path):device_(device) {
        if(!device || !physical || path.empty())
            throw std::runtime_error("GPU timings require a device, physical device and report path");
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physical,&properties);
        period_ns_=properties.limits.timestampPeriod;
        max_dimension_=properties.limits.maxImageDimension2D;
        if(!(period_ns_>0) || !std::isfinite(period_ns_) || !max_dimension_)
            throw std::runtime_error("GPU timestamp period or image limit is unsupported");
        std::uint32_t count{};
        vkGetPhysicalDeviceQueueFamilyProperties(physical,&count,nullptr);
        if(queue_family>=count)throw std::runtime_error("GPU timing queue family is unavailable");
        std::vector<VkQueueFamilyProperties> families(count);
        vkGetPhysicalDeviceQueueFamilyProperties(physical,&count,families.data());
        if(queue_family>=count)throw std::runtime_error("GPU timing queue family changed");
        const auto &family=families[queue_family];
        const auto bits=family.timestampValidBits;
        if(!family.queueCount || !(family.queueFlags&(VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT)) ||
           !bits || bits>64)
            throw std::runtime_error("GPU timing queue lacks supported timestamp bits");
        mask_=bits==64?std::numeric_limits<std::uint64_t>::max():((std::uint64_t{1}<<bits)-1);
        report_.imbue(std::locale::classic());
        report_.open(path);
        if(!report_)throw std::runtime_error("Open renderer GPU timings");
        report_<<"frame\tgpu_total_ms\tgpu_compute_ms\tgpu_scene_ms\tgpu_present_copy_ms"
                 "\trender_width\trender_height\tviews\n"<<std::flush;
        if(!report_)throw std::runtime_error("Write renderer GPU timing header");
        VkQueryPoolCreateInfo info{};
        info.sType=VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        info.queryType=VK_QUERY_TYPE_TIMESTAMP;info.queryCount=query_count;
        checked(vkCreateQueryPool(device_,&info,nullptr,&pool_),"Create renderer GPU timestamp pool");
    }
    RendererGpuTimings(const RendererGpuTimings&)=delete;
    RendererGpuTimings& operator=(const RendererGpuTimings&)=delete;
    ~RendererGpuTimings() {
        // The renderer must already have waited for device idle. Deliberately
        // no implicit wait or final harvest in this nonthrowing destructor.
        if(pool_)vkDestroyQueryPool(device_,pool_,nullptr);
    }
    void harvest() {
        if(!pending_)return;
        CpuEnvironment preserve;
        if(std::fesetround(FE_TONEAREST)!=0)
            throw std::runtime_error("Select GPU timing report rounding");
        struct Query {std::uint64_t value{},available{};};
        static_assert(sizeof(Query)==2*sizeof(std::uint64_t));
        std::array<Query,query_count> queries{};
        const auto result=vkGetQueryPoolResults(device_,pool_,0,query_count,sizeof(queries),
            queries.data(),sizeof(Query),VK_QUERY_RESULT_64_BIT|VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);
        if(result==VK_NOT_READY)
            throw std::runtime_error("GPU timestamps unavailable after completed frame fence");
        checked(result,"Read renderer GPU timestamps");
        for(const auto &query:queries)if(!query.available)
            throw std::runtime_error("GPU timestamp marker unavailable after completed frame fence");
        const auto ticks=[&](unsigned first,unsigned last) {
            return (queries[last].value-queries[first].value)&mask_;
        };
        const auto total=ticks(0,3),compute=ticks(0,1),scene=ticks(1,2),copy=ticks(2,3);
        // Within one counter period, the three ordered intervals must compose
        // exactly. Reject ambiguous wrap/order rather than emit a false timing.
        if(compute>total || scene>total-compute || copy!=total-compute-scene)
            throw std::runtime_error("GPU timestamp intervals do not compose in queue order");
        const auto ms=[&](std::uint64_t value){return double(value)*period_ns_/1000000.0;};
        report_<<serial_<<'\t'<<std::setprecision(9)<<ms(total)<<'\t'<<ms(compute)<<'\t'
               <<ms(scene)<<'\t'<<ms(copy)<<'\t'<<width_<<'\t'<<height_<<'\t'<<views_<<'\n'
               <<std::flush;
        if(!report_)throw std::runtime_error("Write renderer GPU timing row");
        pending_=false;
    }
    void begin(VkCommandBuffer command) {
        if(!command || pending_)
            throw std::runtime_error("GPU timing begin requires a command and harvested frame fence");
        // An abandoned command recording has no pending GPU use and can be
        // replaced by the next frame without manufacturing a timing sample.
        recording_=command;next_marker_=1;
        vkCmdResetQueryPool(command,pool_,0,query_count);
        vkCmdWriteTimestamp(command,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,pool_,0);
    }
    void mark(VkCommandBuffer command,std::uint32_t index) {
        if(!command || command!=recording_ || index!=next_marker_ || index>=query_count || pending_)
            throw std::runtime_error("GPU timing markers must be recorded in order on the frame command");
        vkCmdWriteTimestamp(command,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,pool_,index);
        ++next_marker_;
    }
    void submitted(std::uint64_t serial,std::uint32_t width,std::uint32_t height,std::size_t views) {
        if(pending_ || !recording_ || next_marker_!=query_count || !width || !height ||
           width>max_dimension_ || height>max_dimension_ || views>std::numeric_limits<std::uint32_t>::max() ||
           (has_serial_ && serial<=serial_))
            throw std::runtime_error("GPU timing submission has incomplete markers or invalid frame metadata");
        serial_=serial;width_=width;height_=height;views_=std::uint32_t(views);
        has_serial_=true;pending_=true;recording_=VK_NULL_HANDLE;next_marker_=0;
    }
};
}
