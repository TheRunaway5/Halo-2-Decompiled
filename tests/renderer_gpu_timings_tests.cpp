#include "rendering/renderer/renderer_gpu_timings.hpp"
#include <bit>
#include <cstring>
#include <iostream>
#include <sstream>
#include <type_traits>

namespace {
struct Driver {
    std::uint32_t bits=64,queue_count=1,max_dimension=16384;
    float period=2.f;
    std::array<std::uint64_t,4> values{100,110,130,160};
    std::array<std::uint64_t,4> available{1,1,1,1};
    VkResult result=VK_SUCCESS;
    unsigned creates{},destroys{},reads{},resets{},waits{};
    std::vector<std::pair<VkPipelineStageFlagBits,std::uint32_t>> writes;
} driver;
template<class T>T handle(std::uintptr_t value) {
    if constexpr(std::is_pointer_v<T>)return reinterpret_cast<T>(value);
    else return static_cast<T>(value);
}
const auto device=handle<VkDevice>(1);
const auto physical=handle<VkPhysicalDevice>(2);
const auto command=handle<VkCommandBuffer>(3),other_command=handle<VkCommandBuffer>(4);
void need(bool condition,const char *message){if(!condition)throw std::runtime_error(message);}
template<class F>void rejected(F &&call) {
    try {call();}catch(const std::runtime_error &error){need(std::strlen(error.what())<200,"Unbounded error diagnostic");return;}
    throw std::runtime_error("Invalid timing lifecycle or driver input was accepted");
}
using Timings=halo2::detail::RendererGpuTimings;
static_assert(std::is_nothrow_destructible_v<Timings>);
void complete(Timings &timings,std::uint64_t serial=0) {
    timings.begin(command);timings.mark(command,1);timings.mark(command,2);timings.mark(command,3);
    timings.submitted(serial,3840,2160,4);
}
std::vector<std::string> lines(const std::filesystem::path &path) {
    std::ifstream input(path);std::vector<std::string> out;
    for(std::string line;std::getline(input,line);)out.push_back(line);
    return out;
}
void inspect(const std::filesystem::path &path,double total,double compute,double scene,double copy) {
    const auto rows=lines(path);need(rows.size()==2,"Expected one complete timing row");
    need(rows[0]=="frame\tgpu_total_ms\tgpu_compute_ms\tgpu_scene_ms\tgpu_present_copy_ms\trender_width\trender_height\tviews",
         "GPU TSV schema changed");
    std::istringstream row(rows[1]);std::uint64_t frame;double a,b,c,d;unsigned width,height,views;
    need(bool(row>>frame>>a>>b>>c>>d>>width>>height>>views),"Incomplete timing metadata");
    need(frame==0 && width==3840 && height==2160 && views==4,"GPU timing frame identity changed");
    need(std::abs(a-total)<1e-12 && std::abs(b-compute)<1e-12 && std::abs(c-scene)<1e-12 &&
         std::abs(d-copy)<1e-12,"GPU counter period or wrap conversion differs");
}
void happy_and_fp(const std::filesystem::path &path) {
    driver={};
    {
        Timings timings(device,physical,0,path);
        timings.harvest();need(driver.reads==0,"Unsubmitted query was read");
        timings.begin(command);timings.mark(command,1);timings.mark(command,2);timings.mark(command,3);
        timings.harvest();need(driver.reads==0,"Recorded-only queries became pending");
        timings.submitted(0,3840,2160,4);
        const auto resets=driver.resets;
        rejected([&]{timings.begin(command);});need(driver.resets==resets,"Pending pool was reset");
        std::fenv_t original,before,after;std::fegetenv(&original);
        std::fesetround(FE_DOWNWARD);std::feraiseexcept(FE_INVALID|FE_INEXACT);errno=ERANGE;
        std::fegetenv(&before);timings.harvest();std::fegetenv(&after);
        const bool same=errno==ERANGE && !std::memcmp(&before,&after,sizeof(before));
        std::fesetenv(&original);need(same,"Timing report changed CPU FP/errno effects");
        timings.harvest();need(driver.reads==1,"Harvest repeated a completed sample");
        need(driver.writes.size()==4 && driver.writes[0]==std::pair{VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,0u},
             "GPU timing start stage differs");
        for(unsigned i=1;i<4;++i)need(driver.writes[i]==std::pair{VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,i},
                                    "GPU timing end stage differs");
    }
    need(driver.creates==1 && driver.destroys==1 && !driver.waits,"GPU timer ownership added a wait or leaked a pool");
    inspect(path,.00012,.00002,.00004,.00006);
}
void wrapping(const std::filesystem::path &path,std::uint32_t bits,std::array<std::uint64_t,4> values,
              double total,double compute,double scene,double copy) {
    driver={};driver.bits=bits;driver.values=values;
    {Timings timings(device,physical,0,path);complete(timings);timings.harvest();}
    inspect(path,total,compute,scene,copy);
}
void not_ready_and_bad_order(const std::filesystem::path &path) {
    driver={};Timings timings(device,physical,0,path);
    timings.begin(command);rejected([&]{timings.mark(command,0);});
    rejected([&]{timings.mark(command,2);});rejected([&]{timings.mark(other_command,1);});
    rejected([&]{timings.submitted(0,3840,2160,4);});
    // An unsubmitted abandoned recording can be replaced.
    complete(timings);
    driver.result=VK_NOT_READY;rejected([&]{timings.harvest();});
    rejected([&]{timings.begin(command);});
    driver.result=VK_SUCCESS;driver.available[3]=0;rejected([&]{timings.harvest();});
    driver.available[3]=1;driver.values={100,130,120,160};rejected([&]{timings.harvest();});
    driver.values={100,110,130,160};timings.harvest();inspect(path,.00012,.00002,.00004,.00006);
    timings.begin(command);timings.mark(command,1);timings.mark(command,2);timings.mark(command,3);
    rejected([&]{timings.submitted(0,3840,2160,4);});
    rejected([&]{timings.submitted(1,0,2160,4);});
    rejected([&]{timings.submitted(1,20000,2160,4);});
    timings.submitted(1,3840,2160,4);timings.harvest();
    need(lines(path).size()==3,"Failed harvests fabricated extra result rows");
}
void unsupported(const std::filesystem::path &path) {
    driver={};rejected([&]{Timings t(VK_NULL_HANDLE,physical,0,path);});
    rejected([&]{Timings t(device,physical,0,{});});
    rejected([&]{Timings t(device,physical,1,path);});
    driver.period=0;rejected([&]{Timings t(device,physical,0,path);});
    driver.period=std::numeric_limits<float>::quiet_NaN();rejected([&]{Timings t(device,physical,0,path);});
    driver.period=2;driver.bits=0;rejected([&]{Timings t(device,physical,0,path);});
    driver.bits=65;rejected([&]{Timings t(device,physical,0,path);});
    driver.bits=64;driver.queue_count=0;rejected([&]{Timings t(device,physical,0,path);});
    need(!driver.creates && !driver.destroys && !driver.waits,"Unsupported timing created a pool or waited");
}
void destruction_after_failed_final_harvest(const std::filesystem::path &path) {
    driver={};
    {
        Timings timings(device,physical,0,path);complete(timings);
        driver.result=VK_ERROR_DEVICE_LOST;
        rejected([&]{timings.harvest();});
        need(driver.reads==1,"Failed final harvest did not report its query error");
    }
    need(driver.reads==1 && driver.destroys==1 && !driver.waits,
         "Destruction retried a failed final harvest, leaked ownership or added a wait");
}
}

extern "C" {
VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties(VkPhysicalDevice,VkPhysicalDeviceProperties *properties) {
    *properties={};properties->limits.timestampPeriod=driver.period;properties->limits.maxImageDimension2D=driver.max_dimension;
}
VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice,std::uint32_t *count,VkQueueFamilyProperties *families) {
    *count=1;if(families){families[0]={};families[0].queueCount=driver.queue_count;
        families[0].queueFlags=VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT;families[0].timestampValidBits=driver.bits;}
}
VKAPI_ATTR VkResult VKAPI_CALL vkCreateQueryPool(VkDevice,VkQueryPoolCreateInfo const *info,VkAllocationCallbacks const *,VkQueryPool *pool) {
    need(info->queryType==VK_QUERY_TYPE_TIMESTAMP && info->queryCount==4,"Incorrect timestamp pool");
    ++driver.creates;*pool=handle<VkQueryPool>(5);return VK_SUCCESS;
}
VKAPI_ATTR void VKAPI_CALL vkDestroyQueryPool(VkDevice,VkQueryPool,VkAllocationCallbacks const *){++driver.destroys;}
VKAPI_ATTR void VKAPI_CALL vkCmdResetQueryPool(VkCommandBuffer,VkQueryPool,std::uint32_t first,std::uint32_t count) {
    need(!first && count==4,"Incorrect timestamp pool reset");++driver.resets;
}
VKAPI_ATTR void VKAPI_CALL vkCmdWriteTimestamp(VkCommandBuffer,VkPipelineStageFlagBits stage,VkQueryPool,std::uint32_t index) {
    driver.writes.emplace_back(stage,index);
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetQueryPoolResults(VkDevice,VkQueryPool,std::uint32_t first,std::uint32_t count,
        std::size_t size,void *data,VkDeviceSize stride,VkQueryResultFlags flags) {
    need(!first && count==4 && size==64 && stride==16,"Incorrect GPU query read layout");
    need(flags==(VK_QUERY_RESULT_64_BIT|VK_QUERY_RESULT_WITH_AVAILABILITY_BIT),"GPU timing requested wait/partial results");
    ++driver.reads;
    for(unsigned i=0;i<4;++i) {
        const std::array words{driver.values[i],driver.available[i]};
        std::memcpy(static_cast<std::byte*>(data)+i*stride,words.data(),sizeof(words));
    }
    return driver.result;
}
VKAPI_ATTR VkResult VKAPI_CALL vkDeviceWaitIdle(VkDevice){++driver.waits;return VK_SUCCESS;}
}
int main(int argc,char **argv)try {
    need(argc==2,"Supply an owned test scratch directory");
    const std::filesystem::path root=argv[1];
    happy_and_fp(root/"gpu.tsv");
    wrapping(root/"wrap8.tsv",8,{250,3,20,40},.000092,.000018,.000034,.00004);
    const auto maximum=std::numeric_limits<std::uint64_t>::max();
    wrapping(root/"wrap64.tsv",64,{maximum-5,2,7,11},.000034,.000016,.00001,.000008);
    wrapping(root/"unused-high.tsv",8,{0x1fa,0x303,0x414,0x528},.000092,.000018,.000034,.00004);
    not_ready_and_bad_order(root/"lifecycle.tsv");unsupported(root/"unsupported.tsv");
    destruction_after_failed_final_harvest(root/"failed-final.tsv");
    std::cout<<"PASS GPU timestamps: markers, fence lifecycle, availability/no WAIT, wraps8/64, metadata, FP/errno and ownership\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
