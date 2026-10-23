#include "rendering/models/model_skin_gpu.hpp"
#include "rendering/models/model_skin_vertices.hpp"
#include "halo2/math.hpp"
#include "halo2/transform.hpp"
#include <algorithm>
#include <atomic>
#include <bit>
#include <cfenv>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <random>
#include <stdexcept>
using namespace halo2;
namespace {
void need(bool value,const char *message){if(!value)throw std::runtime_error(message);}
void checked(VkResult result){if(result!=VK_SUCCESS)throw std::runtime_error("GPU skin test Vulkan failure "+std::to_string(result));}
// The original first-person append graph, kept independently of the shader
// and renderer oracle. Retain every identity MUL/ADD and its zero signs.
void original_identity_placement(Vertex &vertex) {
    const Transform identity{};
    const auto rotate=[&](Vec3 v) {
        return identity.basis[0]*v.x+identity.basis[1]*v.y+identity.basis[2]*v.z;
    };
    vertex.position=identity.position+rotate(vertex.position)*identity.scale;
    vertex.normal=rotate(vertex.normal);
    vertex.tangent=rotate(vertex.tangent);
    vertex.binormal=rotate(vertex.binormal);
}
void materialize(Vertex &vertex,const ModelSkinGpuRecord &record,
    std::span<const std::array<float,12>> palette) {
    SkinBinding binding;
    for(unsigned j=0;j<4;++j)binding.bones[j]=std::uint16_t(record.bones[j]);
    binding.weights=record.weights;
    skin_model_vertices({&vertex,1},{&binding,1},palette.subspan(record.palette,record.palette_count));
    if(record.post_identity_placement)original_identity_placement(vertex);
}
struct X87Effects {
    std::uint16_t control{},status{};
    static X87Effects read() {
        X87Effects result;
#if defined(__x86_64__) && !defined(_MSC_VER)
        asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(result.control),"=m"(result.status)::"memory");
#endif
        return result;
    }
    bool operator==(const X87Effects &)const=default;
};
struct Gpu {
    VkInstance instance{};VkDevice device{};VkPhysicalDevice physical{};VkQueue queue{};
    VkCommandPool pool{};VkCommandBuffer command{};VkDebugUtilsMessengerEXT messenger{};
    std::atomic<unsigned> errors{};
    Gpu() {
        VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.apiVersion=VK_API_VERSION_1_2;
        const char *layer="VK_LAYER_KHRONOS_validation",*extension=VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        VkDebugUtilsMessengerCreateInfoEXT debug{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
        debug.messageSeverity=VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug.messageType=VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug.pUserData=this;debug.pfnUserCallback=[](auto,auto,const VkDebugUtilsMessengerCallbackDataEXT *data,void *owner)->VkBool32 {
            ++static_cast<Gpu*>(owner)->errors;std::cerr<<data->pMessage<<'\n';return VK_FALSE;
        };
        VkInstanceCreateInfo info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};info.pApplicationInfo=&app;
        info.enabledLayerCount=1;info.ppEnabledLayerNames=&layer;info.enabledExtensionCount=1;info.ppEnabledExtensionNames=&extension;info.pNext=&debug;
        checked(vkCreateInstance(&info,nullptr,&instance));
        auto create=reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,"vkCreateDebugUtilsMessengerEXT"));
        need(bool(create),"Missing Vulkan validation messenger");checked(create(instance,&debug,nullptr,&messenger));
        unsigned n{};checked(vkEnumeratePhysicalDevices(instance,&n,nullptr));std::vector<VkPhysicalDevice> devices(n);
        checked(vkEnumeratePhysicalDevices(instance,&n,devices.data()));unsigned family{};
        for(auto candidate:devices) {
            if(!ModelSkinGpu::supported(candidate,VK_API_VERSION_1_2))continue;
            vkGetPhysicalDeviceQueueFamilyProperties(candidate,&n,nullptr);std::vector<VkQueueFamilyProperties> families(n);
            vkGetPhysicalDeviceQueueFamilyProperties(candidate,&n,families.data());
            for(unsigned i=0;i<n;++i)if((families[i].queueFlags&(VK_QUEUE_COMPUTE_BIT|VK_QUEUE_GRAPHICS_BIT))==
                (VK_QUEUE_COMPUTE_BIT|VK_QUEUE_GRAPHICS_BIT)){physical=candidate;family=i;break;}
            if(physical)break;
        }
        need(bool(physical),"No GPU with explicit float32 RTE/signed-zero controls");
        const float priority=1;VkDeviceQueueCreateInfo q{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        q.queueFamilyIndex=family;q.queueCount=1;q.pQueuePriorities=&priority;
        VkDeviceCreateInfo dc{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};dc.queueCreateInfoCount=1;dc.pQueueCreateInfos=&q;
        checked(vkCreateDevice(physical,&dc,nullptr,&device));vkGetDeviceQueue(device,family,0,&queue);
        VkCommandPoolCreateInfo pc{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};pc.queueFamilyIndex=family;pc.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        checked(vkCreateCommandPool(device,&pc,nullptr,&pool));
        VkCommandBufferAllocateInfo ca{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};ca.commandPool=pool;ca.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ca.commandBufferCount=1;
        checked(vkAllocateCommandBuffers(device,&ca,&command));
        VkPhysicalDeviceFloatControlsProperties fp{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT_CONTROLS_PROPERTIES};
        VkPhysicalDeviceProperties2 properties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};properties.pNext=&fp;
        vkGetPhysicalDeviceProperties2(physical,&properties);
        std::cout<<"GPU "<<properties.properties.deviceName<<", driver="<<properties.properties.driverVersion
            <<", RTE="<<fp.shaderRoundingModeRTEFloat32<<", signed-zero="<<fp.shaderSignedZeroInfNanPreserveFloat32
            <<", denorm-preserve="<<fp.shaderDenormPreserveFloat32<<'\n';
    }
    ~Gpu(){close();}
    void close() {
        if(device){vkDeviceWaitIdle(device);if(pool)vkDestroyCommandPool(device,pool,nullptr);vkDestroyDevice(device,nullptr);device={};}
        if(messenger) {auto destroy=reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,"vkDestroyDebugUtilsMessengerEXT"));destroy(instance,messenger,nullptr);messenger={};}
        if(instance){vkDestroyInstance(instance,nullptr);instance={};}
    }
};
struct Buffer {
    Gpu &gpu;VkBuffer buffer{};VkDeviceMemory memory{};void *mapped{};
    Buffer(Gpu &g,std::size_t bytes):gpu(g) {
        VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};info.size=bytes;
        info.usage=VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        checked(vkCreateBuffer(g.device,&info,nullptr,&buffer));
        VkMemoryRequirements req;vkGetBufferMemoryRequirements(g.device,buffer,&req);
        VkPhysicalDeviceMemoryProperties props;vkGetPhysicalDeviceMemoryProperties(g.physical,&props);
        constexpr auto flags=VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        unsigned type=props.memoryTypeCount;for(unsigned i=0;i<props.memoryTypeCount;++i)
            if((req.memoryTypeBits&(1u<<i)) && (props.memoryTypes[i].propertyFlags&flags)==flags){type=i;break;}
        need(type<props.memoryTypeCount,"No coherent GPU test buffer");
        VkMemoryAllocateInfo a{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};a.allocationSize=req.size;a.memoryTypeIndex=type;
        checked(vkAllocateMemory(g.device,&a,nullptr,&memory));checked(vkBindBufferMemory(g.device,buffer,memory,0));
        checked(vkMapMemory(g.device,memory,0,bytes,0,&mapped));
    }
    ~Buffer(){vkUnmapMemory(gpu.device,memory);vkDestroyBuffer(gpu.device,buffer,nullptr);vkFreeMemory(gpu.device,memory,nullptr);}
};
class PrepareFailureControl {
    std::string previous_;bool present_{};
    static void set(const char *value) {
#if defined(_WIN32)
        need(_putenv_s("HALO2_TEST_GPU_SKIN_PREPARE_FAILURE",value?value:"")==0,"Cannot set GPU prepare failure control");
#else
        need((value?setenv("HALO2_TEST_GPU_SKIN_PREPARE_FAILURE",value,1):
            unsetenv("HALO2_TEST_GPU_SKIN_PREPARE_FAILURE"))==0,"Cannot set GPU prepare failure control");
#endif
    }
public:
    explicit PrepareFailureControl(const char *value) {
        if(const auto *old=std::getenv("HALO2_TEST_GPU_SKIN_PREPARE_FAILURE")){previous_=old;present_=true;}
        set(value);
    }
    ~PrepareFailureControl()noexcept {
        try{set(present_?previous_.c_str():nullptr);}catch(...){std::terminate();}
    }
};
struct PrepareFailureEvidence {
    std::size_t resource_reasons{},fp_cases{},programming_precedence_cases{},hard_failures{},recovery_jobs{},empty_resets{};
};
PrepareFailureEvidence prepare_failure_cases(Gpu &gpu,ModelSkinGpu &skin) {
    PrepareFailureEvidence evidence;
    struct Restore {
        std::fenv_t fp{};int error=errno;
        Restore(){need(std::fegetenv(&fp)==0,"Cannot save service test FP owner");}
        ~Restore(){if(std::fesetenv(&fp)!=0)std::terminate();errno=error;}
    } restore;
    std::vector<Vertex> input(9);
    for(std::size_t i=0;i<input.size()*sizeof(Vertex);++i)
        std::as_writable_bytes(std::span{input})[i]=std::byte((i*13+71)%256);
    for(auto &vertex:input)for(auto *attribute:{&vertex.position,&vertex.normal,&vertex.tangent,&vertex.binormal})
        *attribute={-0.f,1.f,-1.f};
    const std::array<std::array<float,12>,2> palette{{
        {1,0,0,1,0,1,0,2,0,0,1,3},{-1,0,0,4,0,1,0,5,0,0,-1,6}}};
    std::array<ModelSkinGpuRecord,3> records{};
    records[0].destination=1;records[0].palette_count=1;records[0].weights={1,0,0,0};
    records[1].destination=4;records[1].palette=1;records[1].palette_count=1;
    records[1].weights={1,0,0,0};records[1].post_identity_placement=1;
    records[2].destination=7;records[2].bones.fill(65535);records[2].post_identity_placement=1;
    auto expected=input;
    for(const auto &record:records)materialize(expected[record.destination],record,palette);
    Buffer buffer(gpu,input.size()*sizeof(Vertex));
    const auto execute=[&] {
        checked(vkResetCommandBuffer(gpu.command,0));VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        checked(vkBeginCommandBuffer(gpu.command,&begin));
        if(!std::getenv("HALO2_TEST_GPU_SKIN_NO_DISPATCH"))skin.record(gpu.command);
        checked(vkEndCommandBuffer(gpu.command));VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submit.commandBufferCount=1;submit.pCommandBuffers=&gpu.command;
        checked(vkQueueSubmit(gpu.queue,1,&submit,{}));checked(vkQueueWaitIdle(gpu.queue));
    };
    const auto same_effects=[&](auto &&action) {
        std::fenv_t before{},after{};
        need(std::fegetenv(&before)==0,"Cannot capture service test FP owner");
        const auto before_error=errno;
        action();
        const auto after_error=errno;
        need(std::fegetenv(&after)==0,"Cannot recapture service test FP owner");
        need(!std::memcmp(&before,&after,sizeof(before)) && before_error==after_error,
             "GPU prepare changed complete FP environment or errno on success/refusal/error");
    };
    const auto recover=[&] {
        std::memcpy(buffer.mapped,input.data(),input.size()*sizeof(Vertex));
        same_effects([&]{skin.prepare(records,palette,buffer.buffer,input.size());});
        need(skin.vertices()==records.size(),"Next valid GPU prepare did not recover from refusal");
        execute();
        need(!std::memcmp(buffer.mapped,expected.data(),expected.size()*sizeof(Vertex)),
             "Recovered GPU dispatch did not produce complete original skin/post bytes");
        ++evidence.recovery_jobs;
    };
    const auto programming_error=[&](auto bad_records,auto bad_palette,VkBuffer destination) {
        bool rejected=false;
        same_effects([&] {
            try{skin.prepare(bad_records,bad_palette,destination,input.size());}
            catch(const FormatError&){rejected=true;}
        });
        need(rejected && skin.vertices()==0,"Resource refusal hid a programming error or retained a dispatch");
        ++evidence.programming_precedence_cases;
    };
    using Reason=ModelSkinGpuPrepareRefusalReason;
    struct Failure {const char *name;Reason reason;VkResult result;};
    const Failure failures[]{
        {"quota",Reason::device_quota,VK_SUCCESS},
        {"packing",Reason::input_allocation,VK_SUCCESS},
        {"create",Reason::input_allocation,VK_ERROR_OUT_OF_HOST_MEMORY},
        {"allocation",Reason::input_allocation,VK_ERROR_OUT_OF_HOST_MEMORY},
        {"device-allocation",Reason::input_allocation,VK_ERROR_OUT_OF_DEVICE_MEMORY},
        {"map",Reason::input_mapping,VK_ERROR_OUT_OF_HOST_MEMORY},
        {"map-range",Reason::input_mapping,VK_ERROR_MEMORY_MAP_FAILED}};
    for(const auto &failure:failures) {
        recover(); // Seed a prior nonzero dispatch; every refusal must retire it.
        {
            PrepareFailureControl control(failure.name);
            auto bad=records;bad.back().post_identity_placement=2;
            programming_error(std::span{bad},std::span{palette},buffer.buffer);
            bad=records;bad.back().destination=unsigned(input.size());
            programming_error(std::span{bad},std::span{palette},buffer.buffer);
            bad=records;bad.back().destination=bad.front().destination;
            programming_error(std::span{bad},std::span{palette},buffer.buffer);
            bad=records;bad.back().weights={1,0,0,0};bad.back().palette_count=1;bad.back().bones[0]=1;
            programming_error(std::span{bad},std::span{palette},buffer.buffer);
            bad=records;bad.back().weights[0]=std::bit_cast<float>(0x7f800001u);
            programming_error(std::span{bad},std::span{palette},buffer.buffer);
            auto bad_palette=palette;bad_palette.back().back()=std::bit_cast<float>(0x7f800001u);
            programming_error(std::span{records},std::span{bad_palette},buffer.buffer);
            bad_palette=palette;bad_palette.front().front()=std::bit_cast<float>(1u);
            programming_error(std::span{records},std::span{bad_palette},buffer.buffer);
            bad=records;bad.front().palette_count=2;bad.front().bones={0,1,0,0};bad.front().weights={0,1,0,0};
            programming_error(std::span{bad},std::span{bad_palette},buffer.buffer);
            programming_error(std::span{records},std::span{palette},VK_NULL_HANDLE);
#if defined(__x86_64__) || defined(_M_X64)
            constexpr int rounds[]{FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
            for(unsigned denorm=0;denorm<4;++denorm)for(unsigned round=0;round<4;++round)
                for(unsigned sticky:{0u,1u,2u,4u,8u,16u,32u,63u}) {
                    need(std::fesetround(rounds[(round+1)%4])==0,"Cannot set independent x87 service test rounding");
                    _mm_setcsr(0x1f80u|(round<<13)|sticky|((denorm&1)<<6)|((denorm>>1)<<15));
                    errno=ERANGE;
#else
            {
#endif
                    bool refused=false;
                    same_effects([&] {
                        try{skin.prepare(records,palette,buffer.buffer,input.size());}
                        catch(const ModelSkinGpuPrepareRefused &error) {
                            need(error.reason()==failure.reason && error.result()==failure.result,
                                 "GPU prepare returned the wrong typed resource refusal");
                            refused=true;
                        }
                    });
                    need(refused && skin.vertices()==0,"GPU prepare refusal retained a stale dispatch");
                    ++evidence.fp_cases;
                }
            std::memcpy(buffer.mapped,input.data(),input.size()*sizeof(Vertex));
            execute();
            need(!std::memcmp(buffer.mapped,input.data(),input.size()*sizeof(Vertex)),
                 "record() submitted a stale GPU skin after resource refusal");
            same_effects([&]{skin.prepare({}, {},VK_NULL_HANDLE,0);});
            need(skin.vertices()==0,"Empty prepare did not reset dispatch under fault injection");
            ++evidence.empty_resets;
        }
        ++evidence.resource_reasons;
    }
    for(const char *failure:{"device-lost","unknown-mode"}) {
        recover();
        {
            PrepareFailureControl control(failure);bool hard=false;
            same_effects([&] {
                try{skin.prepare(records,palette,buffer.buffer,input.size());}
                catch(const FormatError&){hard=true;}
            });
            need(hard && skin.vertices()==0,"GPU prepare swallowed device loss/invalid injection or retained a dispatch");
            ++evidence.hard_failures;
            same_effects([&]{skin.prepare({}, {},VK_NULL_HANDLE,0);});
            ++evidence.empty_resets;
        }
    }
    recover();
    return evidence;
}
struct PrepareRowEvidence {
    std::size_t cases{},hard_rejections{},quota_refusals{},boundary_vertex_comparisons{};
};
PrepareRowEvidence prepare_row_cases(Gpu &gpu,ModelSkinGpu &skin) {
    PrepareRowEvidence evidence;
    struct Restore {
        std::fenv_t fp{};int error=errno;
        Restore(){need(std::fegetenv(&fp)==0,"Cannot save row test FP owner");}
        ~Restore(){if(std::fesetenv(&fp)!=0)std::terminate();errno=error;}
    } restore;
    std::vector<std::array<float,12>> palette(4097);
    Buffer buffer(gpu,9*sizeof(Vertex));
    // Independent original checks, including their order. These do not use
    // the summary, normal_bounded(), or the new service's implementation.
    const auto original_error=[](auto records,auto rows)->const char* {
        const auto bits=[](float value){return std::bit_cast<std::uint32_t>(value)&0x7fffffffu;};
        for(const auto &row:rows)for(const auto value:row)
            if(bits(value)>0x49800000u)return "model skin GPU palette leaves its finite arithmetic domain";
        for(const auto &record:records) {
            if(record.post_identity_placement>1)return "model skin GPU has an unknown post-placement mode";
            int last=-1;
            for(unsigned j=0;j<4;++j)if(bits(record.weights[j]))last=int(j);
            for(int j=last;j>=0;--j)for(const auto value:rows[std::size_t(record.palette)+record.bones[unsigned(j)]])
                if(bits(value) && (bits(value)<0x00800000u || bits(value)>0x49800000u))
                    return "model skin GPU referenced palette leaves its normal arithmetic domain";
        }
        return nullptr; // The declared quota control runs after validation.
    };
    constexpr std::uint32_t values[]{0,0x80000000u,1,0x80000001u,0x007fffffu,0x807fffffu,
        0x00800000u,0x80800000u,0x3f800000u,0x49800000u,0x49800001u,
        0x7f800000u,0xff800000u,0x7f800001u,0x7fc00000u};
    {
        PrepareFailureControl control("quota");
        for(const auto count:{4095u,4096u,4097u}) {
            std::vector<unsigned> selected{0,63,64,count-1};
            if(count>4095)selected.push_back(4095);
            std::sort(selected.begin(),selected.end());selected.erase(std::unique(selected.begin(),selected.end()),selected.end());
            for(const auto row:selected)for(unsigned rebased=0;rebased<2;++rebased)
                for(unsigned usage=0;usage<4;++usage)for(unsigned component=0;component<12;++component)
                    for(const auto value:values)for(unsigned malformed=0;malformed<2;++malformed) {
                        const auto base=rebased && row?row-1:0;
                        const auto active=row-base,other=row==base?1u:0u;
                        std::array<ModelSkinGpuRecord,2> records{};
                        records[0].destination=1; // Prior all-zero job, no palette reads.
                        auto &record=records[1];record.destination=4;record.palette=base;record.palette_count=count-base;
                        record.bones={active,other,0xffffffffu,0xffffffffu};
                        if(usage==0)record.weights={1,0,0,0};
                        if(usage==1)record.weights={0,1,0,0}; // The zero interior influence still reads its row.
                        if(usage==2){record.bones={other,active,0xffffffffu,0xffffffffu};record.weights={1,0,0,0};}
                        if(malformed)record.post_identity_placement=2;
                        palette[row][component]=std::bit_cast<float>(value);
                        const auto rows=std::span{palette}.first(count);
                        const auto *expected=original_error(std::span{records},rows);
#if defined(__x86_64__) || defined(_M_X64)
                        constexpr int rounds[]{FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
                        need(std::fesetround(rounds[(evidence.cases/4)%4])==0,"Cannot set row test x87 rounding");
                        _mm_setcsr(0x1f80u|unsigned(evidence.cases%4)<<13|unsigned(evidence.cases%64)|
                            unsigned((evidence.cases/16)%2)<<6|unsigned((evidence.cases/32)%2)<<15);
#endif
#if defined(__GLIBC__) && defined(__x86_64__)
                        // Linux scheduling can clear native x87 instruction/
                        // operand bookkeeping even without any skin call.
                        // Seed only those address/opcode fields; retain every
                        // control/status/tag/MXCSR bit and compare ALL bytes.
                        std::fenv_t stable{};
                        need(std::fegetenv(&stable)==0,"Cannot capture row test metadata seed");
                        stable.__eip=0;stable.__cs_selector=0;stable.__opcode=0;
                        stable.__data_offset=0;stable.__data_selector=0;
                        need(std::fesetenv(&stable)==0,"Cannot seed row test native metadata");
#endif
                        errno=ERANGE;std::fenv_t before{},after{};
                        need(std::fegetenv(&before)==0,"Cannot capture row test FP owner");
                        bool correct=false;
                        try{skin.prepare(records,rows,buffer.buffer,9);}
                        catch(const ModelSkinGpuPrepareRefused &refusal) {
                            correct=!expected && refusal.reason()==ModelSkinGpuPrepareRefusalReason::device_quota && refusal.result()==VK_SUCCESS;
                            ++evidence.quota_refusals;
                        } catch(const FormatError &error) {
                            correct=expected && !std::strcmp(expected,error.what());++evidence.hard_rejections;
                        }
                        const auto after_errno=errno;
                        need(std::fegetenv(&after)==0,"Cannot recapture row test FP owner");
                        need(correct && skin.vertices()==0,"Row summary changes original rejection order or stale-dispatch reset");
                        need(after_errno==ERANGE && !std::memcmp(&before,&after,sizeof(before)),"Row summary changes complete FP/errno state");
                        palette[row][component]=0;++evidence.cases;
                    }
        }
    }
    // Real GPU work crosses the cached prefix boundary; verify complete
    // records, both placement modes, metadata and successful reuse after errors.
    need(std::fesetround(FE_TONEAREST)==0,"Cannot restore row test rounding");
#if defined(__x86_64__) || defined(_M_X64)
    _mm_setcsr(0x1fa0u);
#endif
    std::vector<Vertex> input(9);
    for(std::size_t i=0;i<input.size()*sizeof(Vertex);++i)
        std::as_writable_bytes(std::span{input})[i]=std::byte((i*13+71)%256);
    for(auto &vertex:input)for(auto *attribute:{&vertex.position,&vertex.normal,&vertex.tangent,&vertex.binormal})
        *attribute={-0.f,1.f,-1.f};
    std::array<ModelSkinGpuRecord,3> records{};
    for(unsigned i=0;i<records.size();++i) {
        const auto row=4094+i;palette[row]={1,0,0,1,0,1,0,2,0,0,1,3};
        auto &record=records[i];record.destination=1+i*3;record.palette=row;
        record.palette_count=1;record.weights={1,0,0,0};record.post_identity_placement=i%2;
    }
    auto expected=input;for(const auto &record:records)materialize(expected[record.destination],record,palette);
    std::memcpy(buffer.mapped,input.data(),input.size()*sizeof(Vertex));
    skin.prepare(records,palette,buffer.buffer,input.size());need(skin.vertices()==records.size(),"Row boundary prepare dropped work");
    checked(vkResetCommandBuffer(gpu.command,0));VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    checked(vkBeginCommandBuffer(gpu.command,&begin));
    if(!std::getenv("HALO2_TEST_GPU_SKIN_NO_DISPATCH"))skin.record(gpu.command);
    checked(vkEndCommandBuffer(gpu.command));VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount=1;submit.pCommandBuffers=&gpu.command;
    checked(vkQueueSubmit(gpu.queue,1,&submit,{}));checked(vkQueueWaitIdle(gpu.queue));
    need(!std::memcmp(buffer.mapped,expected.data(),expected.size()*sizeof(Vertex)),"Row boundary GPU bytes differ");
    evidence.boundary_vertex_comparisons=records.size();return evidence;
}
struct PersistentPrepareEvidence {
    std::size_t gpu_jobs{},complete_vertex_comparisons{},fp_cases{},refusals{},hard_errors{},empty_resets{};
    std::size_t warm_checks{},traffic_violations{},growths{};
    std::uint64_t record_pack_bytes{},record_upload_bytes{},palette_upload_bytes{},descriptor_writes{};
};
PersistentPrepareEvidence persistent_prepare_cases(Gpu &gpu) {
    PersistentPrepareEvidence evidence;
    struct Restore {
        std::fenv_t fp{};int error=errno;
        Restore(){need(std::fegetenv(&fp)==0,"Cannot save persistent-input test FP owner");}
        ~Restore(){if(std::fesetenv(&fp)!=0)std::terminate();errno=error;}
    } restore;
    ModelSkinGpu skin(gpu.device,gpu.physical,VK_API_VERSION_1_2,HALO2_MODEL_SKIN_GPU_SHADER);
    constexpr unsigned largest=8193,extent=largest*2+9;
    std::vector<Vertex> input(extent);
    for(std::size_t i=0;i<input.size()*sizeof(Vertex);++i)
        std::as_writable_bytes(std::span{input})[i]=std::byte((i*37+71)%256);
    for(unsigned i=0;i<input.size();++i)
        for(auto *attribute:{&input[i].position,&input[i].normal,&input[i].tangent,&input[i].binormal})
            *attribute={i%2?-0.f:1.f,2.f,-1.f};
    Buffer first(gpu,input.size()*sizeof(Vertex)),second(gpu,input.size()*sizeof(Vertex));
    std::uint64_t first_generation=1,second_generation=2;
    std::memcpy(first.mapped,input.data(),input.size()*sizeof(Vertex));
    std::memcpy(second.mapped,input.data(),input.size()*sizeof(Vertex));
    std::vector<std::array<float,12>> palette{
        {1,0,0,1,0,1,0,2,0,0,1,3},
        {-1,0,0,4,0,1,0,5,0,0,-1,6},
        {0,1,0,-2,1,0,0,1,0,0,1,-3},
        {1,0,0,7,0,-1,0,-4,0,0,1,2}};
    const auto make_records=[](unsigned count) {
        std::vector<ModelSkinGpuRecord> records(count);
        for(unsigned i=0;i<count;++i) {
            auto &record=records[i];record.destination=i*2+1;record.palette=(i%2)*2;
            record.palette_count=2;record.bones={0,1,65535,65535};
            record.weights={.75f,.25f,0,0};record.post_identity_placement=i%2;
            if(i%5==2){record.weights={0,-0.f,0,-0.f};record.bones.fill(65535);}
        }
        return records;
    };
    const auto preserve=[&](auto &&action) {
#if defined(__GLIBC__) && defined(__x86_64__)
        // Match the existing row fixture's stable native bookkeeping entry.
        // Preserve all controls/status/tags/MXCSR and compare every fenv byte.
        std::fenv_t stable{};need(std::fegetenv(&stable)==0,"Cannot capture persistent-input metadata seed");
        stable.__eip=0;stable.__cs_selector=0;stable.__opcode=0;
        stable.__data_offset=0;stable.__data_selector=0;
        need(std::fesetenv(&stable)==0,"Cannot seed persistent-input native metadata");
#endif
        errno=ERANGE;std::fenv_t before{},after{};
        need(std::fegetenv(&before)==0,"Cannot capture persistent-input FP owner");
        action();const auto error=errno;
        need(std::fegetenv(&after)==0,"Cannot recapture persistent-input FP owner");
        need(error==ERANGE && !std::memcmp(&before,&after,sizeof(before)),
             "Persistent GPU input prepare changed complete FP environment or errno");
        ++evidence.fp_cases;
    };
    const auto execute=[&] {
        checked(vkResetCommandBuffer(gpu.command,0));VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        checked(vkBeginCommandBuffer(gpu.command,&begin));
        if(!std::getenv("HALO2_TEST_GPU_SKIN_NO_DISPATCH"))skin.record(gpu.command);
        checked(vkEndCommandBuffer(gpu.command));VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submit.commandBufferCount=1;submit.pCommandBuffers=&gpu.command;
        checked(vkQueueSubmit(gpu.queue,1,&submit,{}));checked(vkQueueWaitIdle(gpu.queue));
    };
    const auto traffic=[&](bool condition) {if(!condition)++evidence.traffic_violations;};
    const auto no_traffic=[&] {
        const auto stats=skin.prepare_stats();
        return !stats.input_allocations && !stats.input_maps && !stats.input_unmaps && !stats.record_pack_bytes &&
            !stats.record_upload_bytes && !stats.palette_upload_bytes && !stats.descriptor_writes && !stats.record_cache_hits;
    };
    const auto warm=[&] {
        const auto stats=skin.prepare_stats();++evidence.warm_checks;
        traffic(!stats.input_allocations && !stats.input_maps && !stats.input_unmaps &&
            !stats.record_pack_bytes && !stats.record_upload_bytes && !stats.descriptor_writes && stats.record_cache_hits);
    };
    const auto run=[&](const auto &records,const auto &rows,Buffer &destination,std::size_t destination_extent,bool unchanged_records=false) {
        need(std::fesetround(FE_TONEAREST)==0,"Cannot set persistent-input reference rounding");
#if defined(__x86_64__) || defined(_M_X64)
        _mm_setcsr(0x1fa0u);
#endif
        auto expected=input;
        for(const auto &record:records)materialize(expected[record.destination],record,rows);
        auto &other=&destination==&first?second:first;
        std::vector<std::byte> untouched(input.size()*sizeof(Vertex));
        std::memcpy(untouched.data(),other.mapped,untouched.size());
        std::memcpy(destination.mapped,input.data(),input.size()*sizeof(Vertex));
        const auto generation=&destination==&first?first_generation:second_generation;
        preserve([&]{skin.prepare(records,rows,destination.buffer,destination_extent,generation);});
        need(skin.vertices()==records.size(),"Persistent GPU input prepare published the wrong dispatch count");
        const auto stats=skin.prepare_stats();
        evidence.record_pack_bytes+=stats.record_pack_bytes;evidence.record_upload_bytes+=stats.record_upload_bytes;
        evidence.palette_upload_bytes+=stats.palette_upload_bytes;evidence.descriptor_writes+=stats.descriptor_writes;
        evidence.growths+=stats.input_allocations!=0;
        if(unchanged_records)warm();
        execute();
        need(!std::memcmp(destination.mapped,expected.data(),input.size()*sizeof(Vertex)),
             "Persistent GPU input dispatch changed complete vertices, untouched tail, metadata or placement");
        need(!std::memcmp(other.mapped,untouched.data(),untouched.size()),
             "Persistent GPU input descriptor wrote the previous destination buffer");
        ++evidence.gpu_jobs;evidence.complete_vertex_comparisons+=input.size();
    };
    auto records=make_records(3);
    run(records,palette,first,extent);
    {
        const auto cold=skin.prepare_stats();
        traffic(cold.input_allocations && cold.input_maps && cold.record_pack_bytes==records.size()*44 &&
            cold.record_upload_bytes==records.size()*44 && cold.palette_upload_bytes==palette.size()*48 && cold.descriptor_writes);
    }
    run(records,palette,first,extent,true);
    palette[0][3]=9;run(records,palette,first,extent,true);
    traffic(skin.prepare_stats().palette_upload_bytes!=0);
    const auto equal_records=records;run(equal_records,palette,first,extent,true);
    records[0].palette=2;run(records,palette,first,extent);
    records[0].bones={1,0,65535,65535};run(records,palette,first,extent);
    records[0].weights={1,0,0,0};run(records,palette,first,extent);
    // All-zero skinning retains the input negative zero. The following
    // post-mode-only mutation must still execute identity placement and flip it.
    records[0].weights={0,0,0,0};run(records,palette,first,extent);
    records[0].post_identity_placement=1;run(records,palette,first,extent);
    records[0].destination=2;run(records,palette,first,extent);
    ++first_generation; // A fresh lifetime epoch must invalidate even the same numeric handle/range.
    run(records,palette,first,extent);traffic(skin.prepare_stats().descriptor_writes!=0);
    run(records,palette,first,extent,true);
    first_generation=0;
    for(unsigned repeat=0;repeat<2;++repeat) {
        run(records,palette,first,extent);traffic(skin.prepare_stats().descriptor_writes!=0);
    }
    first_generation=3;
    run(records,palette,first,extent);run(records,palette,first,extent,true);
    run(records,palette,second,extent);
    traffic(skin.prepare_stats().descriptor_writes!=0);
    run(records,palette,second,extent,true);
    run(records,palette,second,extent-1);
    traffic(skin.prepare_stats().descriptor_writes!=0);
    run(records,palette,second,extent-1,true);
    palette.push_back({1,0,0,-7,0,1,0,-8,0,0,1,-9});run(records,palette,second,extent-1);
    traffic(skin.prepare_stats().descriptor_writes!=0);
    palette.pop_back();run(records,palette,second,extent-1);
    traffic(skin.prepare_stats().descriptor_writes!=0);
    for(const unsigned count:{63u,64u,65u,7u,65u,largest,3u}) {
        if(count==largest) {
            const auto larger=make_records(count);bool refused=false;
            {
                PrepareFailureControl control("allocation");
                preserve([&]{try{skin.prepare(larger,palette,first.buffer,extent,first_generation);}
                    catch(const ModelSkinGpuPrepareRefused &error){refused=error.reason()==ModelSkinGpuPrepareRefusalReason::input_allocation &&
                        error.result()==VK_ERROR_OUT_OF_HOST_MEMORY;}});
            }
            need(refused && !skin.vertices(),"Failed persistent-input growth retained a dispatch or changed refusal");
            ++evidence.refusals;
            // A failed replacement must leave the old-size workload usable.
            run(records,palette,first,extent);
        }
        records=make_records(count);run(records,palette,first,extent);
        run(records,palette,first,extent,true);
    }
    // A cached GPU payload still has to validate the host-only palette extent.
    auto bad=records;bad[0].palette_count=0;bool hard=false;
    preserve([&]{try{skin.prepare(bad,palette,first.buffer,extent,first_generation);}catch(const FormatError &){hard=true;}});
    need(hard && !skin.vertices(),"Persistent input cache skipped a changed host-only palette extent");++evidence.hard_errors;
    traffic(no_traffic());
    hard=false;
    preserve([&]{try{skin.prepare(records,palette,first.buffer,records.back().destination,first_generation);}
        catch(const FormatError &){hard=true;}});
    need(hard && !skin.vertices(),"Persistent input cache skipped a changed destination extent");++evidence.hard_errors;
    traffic(no_traffic());
    std::memcpy(first.mapped,input.data(),input.size()*sizeof(Vertex));execute();
    need(!std::memcmp(first.mapped,input.data(),input.size()*sizeof(Vertex)),"Rejected cached job retained a stale dispatch");
    run(records,palette,first,extent);
    struct Failure {const char *name;ModelSkinGpuPrepareRefusalReason reason;VkResult result;};
    using Reason=ModelSkinGpuPrepareRefusalReason;
    const Failure failures[]{
        {"quota",Reason::device_quota,VK_SUCCESS},{"packing",Reason::input_allocation,VK_SUCCESS},
        {"create",Reason::input_allocation,VK_ERROR_OUT_OF_HOST_MEMORY},
        {"allocation",Reason::input_allocation,VK_ERROR_OUT_OF_HOST_MEMORY},
        {"device-allocation",Reason::input_allocation,VK_ERROR_OUT_OF_DEVICE_MEMORY},
        {"map",Reason::input_mapping,VK_ERROR_OUT_OF_HOST_MEMORY},
        {"map-range",Reason::input_mapping,VK_ERROR_MEMORY_MAP_FAILED}};
    for(const auto &failure:failures) {
        run(records,palette,first,extent,true);
        {
            PrepareFailureControl control(failure.name);bool refused=false;
            preserve([&]{try{skin.prepare(records,palette,first.buffer,extent,first_generation);}
                catch(const ModelSkinGpuPrepareRefused &error){
                    refused=error.reason()==failure.reason && error.result()==failure.result;}});
            need(refused && !skin.vertices(),"Persistent input cache bypassed typed refusal or retained a dispatch");++evidence.refusals;
            std::memcpy(first.mapped,input.data(),input.size()*sizeof(Vertex));execute();
            need(!std::memcmp(first.mapped,input.data(),input.size()*sizeof(Vertex)),"Refused persistent input submitted stale work");
            bad=records;bad.back().post_identity_placement=2;hard=false;
            preserve([&]{try{skin.prepare(bad,palette,first.buffer,extent,first_generation);}catch(const FormatError &){hard=true;}});
            need(hard && !skin.vertices(),"Persistent input cache reordered late validation behind refusal");++evidence.hard_errors;
            preserve([&]{skin.prepare({}, {},VK_NULL_HANDLE,0);});need(!skin.vertices(),"Empty persistent input did not reset dispatch");
            traffic(no_traffic());
            ++evidence.empty_resets;
        }
        run(records,palette,first,extent);
    }
    for(const char *failure:{"device-lost","unknown-mode"}) {
        run(records,palette,first,extent,true);
        {
            PrepareFailureControl control(failure);hard=false;
            preserve([&]{try{skin.prepare(records,palette,first.buffer,extent,first_generation);}catch(const FormatError &){hard=true;}});
            need(hard && !skin.vertices(),"Persistent input cache swallowed a hard error");++evidence.hard_errors;
        }
        run(records,palette,first,extent);
    }
    // Exercise warm validation under independent x87/SSE rounding and every
    // DAZ/FTZ combination. CPU reference arithmetic remains outside this loop.
#if defined(__x86_64__) || defined(_M_X64)
    constexpr int rounds[]{FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
    for(unsigned denorm=0;denorm<4;++denorm)for(unsigned round=0;round<4;++round)
        for(unsigned sticky:{0u,1u,2u,4u,8u,16u,32u,63u}) {
            need(std::fesetround(rounds[(round+1)%4])==0,"Cannot set persistent-input x87 rounding");
            _mm_setcsr(0x1f80u|(round<<13)|sticky|((denorm&1)<<6)|((denorm>>1)<<15));
            preserve([&]{skin.prepare(records,palette,first.buffer,extent,first_generation);});warm();
        }
#endif
    preserve([&]{skin.prepare({}, {},VK_NULL_HANDLE,0);});need(!skin.vertices(),"Final persistent-input reset retained work");
    traffic(no_traffic());
    std::memcpy(first.mapped,input.data(),input.size()*sizeof(Vertex));execute();
    need(!std::memcmp(first.mapped,input.data(),input.size()*sizeof(Vertex)),"Empty persistent-input reset submitted stale work");
    ++evidence.empty_resets;run(records,palette,first,extent);
    traffic(evidence.growths>=2);
    return evidence;
}
std::size_t post_identity_cases(Gpu &gpu,ModelSkinGpu &skin) {
    constexpr unsigned count=3*2*8*8*8;
    std::vector<Vertex> input(count+5);
    for(std::size_t i=0;i<input.size()*sizeof(Vertex);++i)
        std::as_writable_bytes(std::span{input})[i]=std::byte((i*37+19)%256);
    const float negative_zero=std::bit_cast<float>(0x80000000u);
    const std::array<std::array<float,12>,2> palette{{
        {1,0,0,0,0,1,0,0,0,0,1,0},
        {negative_zero,negative_zero,negative_zero,0,1,0,0,0,-1,negative_zero,negative_zero,0}}};
    std::vector<ModelSkinGpuRecord> records;records.reserve(count);
    unsigned zero_sign_witnesses{};
    for(unsigned kind=0;kind<3;++kind)for(unsigned post=0;post<2;++post)
        for(unsigned x=0;x<8;++x)for(unsigned y=0;y<8;++y)for(unsigned z=0;z<8;++z) {
            // The zero-influence case can retain the smallest normal inputs.
            // Weighted cases use the conservative one-influence q>=-126 bound.
            const float tiny=kind?0x1p-103f:0x1p-126f;
            const std::array values{0.f,negative_zero,1.f,-1.f,tiny,-tiny,0x1p20f,-0x1p20f};
            ModelSkinGpuRecord record;
            record.destination=count-unsigned(records.size())+4;
            record.palette=kind==2?1:0;record.palette_count=1;
            record.post_identity_placement=post;
            if(kind)record.weights={1,0,0,0};
            else {record.weights={0,negative_zero,0,negative_zero};record.bones.fill(65535);}
            auto &v=input[record.destination];
            v.position={values[x],values[y],values[z]};v.normal={values[z],values[x],values[y]};
            v.tangent={values[y],values[z],values[x]};v.binormal={values[x],values[z],values[y]};
            SkinBinding binding;
            for(unsigned j=0;j<4;++j)binding.bones[j]=std::uint16_t(record.bones[j]);binding.weights=record.weights;
            need(model_skin_gpu_arithmetic_admitted({&v,1},{&binding,1},std::span{palette}.subspan(record.palette,1)),
                 "Post-placement zero/normal fixture left the admitted arithmetic domain");
            records.push_back(record);
        }
    auto expected=input;
    for(const auto &record:records)materialize(expected[record.destination],record,palette);
    std::size_t index{};
    for(unsigned kind=0;kind<3;++kind)for(unsigned post=0;post<2;++post)
        for(unsigned x=0;x<8;++x)for(unsigned y=0;y<8;++y)for(unsigned z=0;z<8;++z,++index) {
            const auto &v=expected[records[index].destination];
            if((kind==0 && x==2 && y==3 && z==1) || (kind==2 && x==2 && y==2 && z==2)) {
                need(std::bit_cast<std::uint32_t>(v.normal.x)==(post?0u:0x80000000u),
                     "Post-placement witness did not change negative-zero normal to positive zero");
                ++zero_sign_witnesses;
            }
            if(kind==0 && x==3 && y==3 && z==1) {
                need(std::bit_cast<std::uint32_t>(v.normal.x)==0x80000000u,
                     "Identity rotate erased an all-negative zero-product sign");
                if(post)need(std::bit_cast<std::uint32_t>(v.position.z)==0u,
                             "Identity position translation did not add its positive zero");
            }
        }
    need(zero_sign_witnesses==4,"Missing weighted/unweighted mixed-mode zero-sign witnesses");
#if defined(__x86_64__) || defined(_M_X64)
    const auto saved_csr=_mm_getcsr();const auto saved_errno=errno;
    for(unsigned denorm=0;denorm<4;++denorm)for(unsigned sticky=0;sticky<32;++sticky) {
        auto reference=input;
        const auto csr=0x1fa0u|sticky|((denorm&1)<<6)|((denorm>>1)<<15);
        _mm_setcsr(csr);errno=ERANGE;const auto x87=X87Effects::read();
        for(const auto &record:records) {
            need(model_skin_gpu_host_effects_admitted(),"Post-placement fixture lost original host admission");
            materialize(reference[record.destination],record,palette);
        }
        need(_mm_getcsr()==csr && X87Effects::read()==x87 && errno==ERANGE,
             "Mixed-mode skin/identity changed preexisting SSE/x87/errno effects");
        need(!std::memcmp(reference.data(),expected.data(),expected.size()*sizeof(Vertex)),
             "Mixed-mode zero/normal placement changed bytes with sticky flags or DAZ/FTZ");
    }
    _mm_setcsr(saved_csr);errno=saved_errno;
#endif
    Buffer buffer(gpu,input.size()*sizeof(Vertex));std::memcpy(buffer.mapped,input.data(),input.size()*sizeof(Vertex));
    skin.prepare(records,palette,buffer.buffer,input.size());need(skin.vertices()==count,"GPU dropped mixed-mode placement work");
    checked(vkResetCommandBuffer(gpu.command,0));VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    checked(vkBeginCommandBuffer(gpu.command,&begin));
    if(!std::getenv("HALO2_TEST_GPU_SKIN_NO_DISPATCH"))skin.record(gpu.command);
    VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};barrier.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
    vkCmdPipelineBarrier(gpu.command,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&barrier,0,nullptr,0,nullptr);
    checked(vkEndCommandBuffer(gpu.command));VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&gpu.command;
    checked(vkQueueSubmit(gpu.queue,1,&submit,{}));checked(vkQueueWaitIdle(gpu.queue));
    need(!std::memcmp(buffer.mapped,expected.data(),expected.size()*sizeof(Vertex)),
         "GPU mixed-mode placement differs from original complete skin/append bytes");
    return input.size();
}
}
int main()try {
    std::fenv_t initial;std::fegetenv(&initial);std::fesetround(FE_TONEAREST);
    Gpu gpu;std::size_t comparisons{},post_comparisons{};PrepareFailureEvidence prepare_failures;PrepareRowEvidence prepare_rows;
    PersistentPrepareEvidence persistent;
    {
        ModelSkinGpu skin(gpu.device,gpu.physical,VK_API_VERSION_1_2,HALO2_MODEL_SKIN_GPU_SHADER);
        prepare_failures=prepare_failure_cases(gpu,skin);
        prepare_rows=prepare_row_cases(gpu,skin);
        unsigned seed=0x1376c0u;
        const auto next=[&] {seed=seed*1664525u+1013904223u;return seed;};
        const auto value=[&] {
            const auto bits=next();const unsigned exponent=103u+(bits>>27)%32;
            return std::bit_cast<float>((bits&0x807fffffu)|(exponent<<23));
        };
        for(const auto count:{0u,1u,63u,64u,65u,4096u,7u,8193u}) {
            std::vector<Vertex> input(count+5);
            for(auto &byte:std::as_writable_bytes(std::span{input}))byte=std::byte(next()>>24);
            for(auto &v:input)for(auto *a:{&v.position,&v.normal,&v.tangent,&v.binormal})
                *a={value(),value(),value()};
            std::vector<std::array<float,12>> palette(16);
            for(auto &row:palette)for(auto &v:row)v=value();
            std::vector<SkinBinding> bindings(count);
            std::vector<ModelSkinGpuRecord> records(count);
            auto expected=input;
            for(unsigned i=0;i<count;++i) {
                auto &b=bindings[i];const unsigned influences=i%5;
                for(unsigned j=0;j<4;++j){b.bones[j]=std::uint16_t(next()%8);b.weights[j]=j<influences?float((next()%255)+1)/256.f:0.f;}
                if(i%7==0 && influences>=3)b.weights[1]=-0.f;
                if(i%11==0 && influences)b.weights[0]=-b.weights[0];
                auto &record=records[i];record.destination=count-i+4;record.palette=(i%2)*8;record.palette_count=8;
                record.post_identity_placement=i%3!=1;
                for(unsigned j=0;j<4;++j)record.bones[j]=b.bones[j];record.weights=b.weights;
                if(i<4)for(auto *a:{&input[record.destination].position,&input[record.destination].normal,
                                   &input[record.destination].tangent,&input[record.destination].binormal})
                    *a={i%2?-0.f:0.f,1.f,-1.f};
                expected[record.destination]=input[record.destination];
                need(model_skin_gpu_arithmetic_admitted(std::span{input}.subspan(record.destination,1),std::span{&b,1},std::span{palette}.subspan(record.palette,record.palette_count)),"Admitted test domain unexpectedly rejected");
                materialize(expected[record.destination],record,palette);
            }
            if(count==1) {
                // One rigid influence, with DP3 cancellation exactly at the
                // smallest normal. Moving both attributes down one exponent
                // would produce2^-127 and must remain a CPU fallback.
                palette[0].fill(0);palette[0][0]=1;palette[0][1]=-1;
                auto &record=records[0];record.palette=0;record.bones={0,0,0,0};record.weights={1,0,0,0};
                const auto dest=record.destination;
                for(auto *attribute:{&input[dest].position,&input[dest].normal,&input[dest].tangent,&input[dest].binormal})
                    *attribute={std::bit_cast<float>(0x0c000001u),std::bit_cast<float>(0x0c000000u),0};
                SkinBinding binding;binding.weights=record.weights;
                need(model_skin_gpu_arithmetic_admitted(std::span{input}.subspan(dest,1),{&binding,1},std::span{palette}.first(8)),
                     "Adaptive rigid minimum normal was rejected");
                expected=input;materialize(expected[dest],record,palette);
                need(std::bit_cast<std::uint32_t>(expected[dest].position.x)==0x00800000u,"Rigid CPU dot did not reach exact2^-126");
                auto below=input[dest];
                for(auto *attribute:{&below.position,&below.normal,&below.tangent,&below.binormal})
                    *attribute={std::bit_cast<float>(0x0b800001u),std::bit_cast<float>(0x0b800000u),0};
                need(!model_skin_gpu_arithmetic_admitted({&below,1},{&binding,1},std::span{palette}.first(8)),
                     "Adaptive rigid qualification accepted possible subnormal dot");
            }
            if(count>=2) {
                // Deliberate cancellation reaches either the original2^-120
                // lattice or the adaptive minimum normal2^-126 boundary.
                const bool minimum_normal=count==65 || count==8193;
                for(unsigned i=0;i<2;++i)palette[i].fill(0);
                palette[0][0]=std::bit_cast<float>(0x2b800001u);palette[1][0]=std::bit_cast<float>(0xab800000u);
                palette[0][1]=std::bit_cast<float>(0xab800001u);palette[1][1]=std::bit_cast<float>(0x2b800000u);
                records[0].palette=0;records[0].bones={0,1,0,0};records[0].weights={0x1p-10f,0x1p-10f,0,0};
                const auto dest=records[0].destination;
                for(auto *a:{&input[dest].position,&input[dest].normal,&input[dest].tangent,&input[dest].binormal})
                    *a=minimum_normal?Vec3{std::bit_cast<float>(0x30800001u),0x1p-30f,0}:
                        Vec3{std::bit_cast<float>(0x33800001u),0x1p-24f,0};
                // Opposite extreme: four maximum row products and three
                // maximum attribute products, with a position translation.
                for(unsigned i=6;i<8;++i)palette[i].fill(0x1p20f);
                records[1].palette=0;records[1].bones={6,7,6,7};records[1].weights={1,1,1,1};
                for(auto *a:{&input[records[1].destination].position,&input[records[1].destination].normal,
                            &input[records[1].destination].tangent,&input[records[1].destination].binormal})
                    *a={0x1p20f,0x1p20f,0x1p20f};
                // Palette mutation affects all owners using this bank: rebuild
                // every reference record before comparing complete storage.
                expected=input;
                for(const auto &record:records) {
                    SkinBinding binding;for(unsigned j=0;j<4;++j)binding.bones[j]=std::uint16_t(record.bones[j]);binding.weights=record.weights;
                    need(model_skin_gpu_arithmetic_admitted(std::span{input}.subspan(record.destination,1),{&binding,1},std::span{palette}.subspan(record.palette,record.palette_count)),"Boundary lattice input was rejected");
                    materialize(expected[record.destination],record,palette);
                }
                need(std::bit_cast<std::uint32_t>(expected[dest].position.x)==(minimum_normal?0x00800000u:0x03800000u),
                     "CPU cancellation did not reach the declared exact normal boundary");
                if(minimum_normal) {
                    auto below=input[dest];
                    for(auto *a:{&below.position,&below.normal,&below.tangent,&below.binormal})
                        *a={std::bit_cast<float>(0x30000001u),0x1p-31f,0};
                    SkinBinding binding;binding.bones={0,1,0,0};binding.weights=records[0].weights;
                    need(!model_skin_gpu_arithmetic_admitted({&below,1},{&binding,1},std::span{palette}.first(8)),
                         "Adaptive qualification crossed into possible subnormal cancellation");
                }
            }
#if defined(__x86_64__) || defined(_M_X64)
            const auto saved_csr=_mm_getcsr();const auto saved_errno=errno;
            for(unsigned denorm=0;denorm<4;++denorm) {
                auto reference=input;
                const auto csr=0x1fa0u|(saved_csr&0x3fu)|((denorm&1)<<6)|((denorm>>1)<<15);
                _mm_setcsr(csr);errno=EDOM;
                const auto x87=X87Effects::read();
                for(const auto &record:records) {
                    need(model_skin_gpu_host_effects_admitted(),"Qualified CPU skin lost its host effects admission");
                    materialize(reference[record.destination],record,palette);
                    need(_mm_getcsr()==csr && errno==EDOM && X87Effects::read()==x87,
                         "Admitted CPU skin/placement produced new host SSE/x87/errno effects");
                }
                need(!std::memcmp(reference.data(),expected.data(),expected.size()*sizeof(Vertex)),"Normal-domain CPU bytes changed with DAZ/FTZ");
            }
            _mm_setcsr(saved_csr);errno=saved_errno;
#endif
            Buffer buffer(gpu,input.size()*sizeof(Vertex));std::memcpy(buffer.mapped,input.data(),input.size()*sizeof(Vertex));
            skin.prepare(records,palette,buffer.buffer,input.size());need(skin.vertices()==count,"GPU job dropped vertices");
            checked(vkResetCommandBuffer(gpu.command,0));VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
            checked(vkBeginCommandBuffer(gpu.command,&begin));
            if(!std::getenv("HALO2_TEST_GPU_SKIN_NO_DISPATCH"))skin.record(gpu.command);
            VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};barrier.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
            vkCmdPipelineBarrier(gpu.command,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&barrier,0,nullptr,0,nullptr);
            checked(vkEndCommandBuffer(gpu.command));VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&gpu.command;
            checked(vkQueueSubmit(gpu.queue,1,&submit,{}));checked(vkQueueWaitIdle(gpu.queue));
            need(!std::memcmp(buffer.mapped,expected.data(),expected.size()*sizeof(Vertex)),"GPU skin complete vertex bytes differ from existing CPU kernel");
            comparisons+=input.size();
            if(count) {
                auto bad=records;bad.push_back(records.front());bool rejected=false;
                try{skin.prepare(bad,palette,buffer.buffer,input.size());}catch(const FormatError&){rejected=true;}
                need(rejected,"GPU accepted two pose writes to one destination");
                bad=records;bad.front().destination=unsigned(input.size());rejected=false;
                try{skin.prepare(bad,palette,buffer.buffer,input.size());}catch(const FormatError&){rejected=true;}
                need(rejected,"GPU accepted an escaping destination");
                bad=records;bad.front().palette=0;bad.front().palette_count=8;
                bad.front().bones[0]=8;bad.front().weights={1,0,0,0};rejected=false;
                try{skin.prepare(bad,palette,buffer.buffer,input.size());}catch(const FormatError&){rejected=true;}
                need(rejected,"GPU bone escaped into a different owner's adjacent palette");
                bad=records;bad.front().post_identity_placement=2;rejected=false;
                try{skin.prepare(bad,palette,buffer.buffer,input.size());}catch(const FormatError&){rejected=true;}
                need(rejected && skin.vertices()==0,"GPU accepted an unknown post-placement mode or retained a failed job");
            }
        }
        need(comparisons==12529,"Original eight-size complete-vertex cohort changed extent");
        post_comparisons=post_identity_cases(gpu,skin);comparisons+=post_comparisons;
        Vertex v{};v.position={1,1,1};SkinBinding b{};std::array<std::array<float,12>,1> palette{};
        const auto admitted=[&]{return model_skin_gpu_arithmetic_admitted({&v,1},{&b,1},palette);};
        need(admitted(),"Finite zero/normal admission failed");v.normal.x=std::bit_cast<float>(1u);need(!admitted(),"Admission accepted a subnormal input");
        v.normal.x=std::bit_cast<float>(0x7f800001u);need(!admitted(),"Admission accepted a signaling NaN");
        v.normal.x=0;b.bones[0]=1;need(!admitted(),"Admission accepted an invalid active bone");
        b.bones[0]=0;b.weights={0x1p-10f,0,0x1p-10f,0};b.bones[1]=1;
        need(!admitted(),"Admission skipped an invalid interior zero influence");
        b.weights.fill(0);b.bones.fill(65535);need(admitted(),"All-zero influences incorrectly read unused bones");
        b={};
        for(auto bits:{0x007fffffu,0x49800001u,0x7f800000u,0xff800000u}) {
            v.normal.x=std::bit_cast<float>(bits);need(!admitted(),"Vertex qualification crossed a finite bound");
        }
        v.normal.x=0;
        for(auto bits:{0x007fffffu,0x49800001u,1u,0x7fc00000u}) {
            palette[0][0]=std::bit_cast<float>(bits);need(!admitted(),"Palette qualification crossed a finite bound");
        }
        palette[0][0]=0;
        for(auto bits:{0x007fffffu,0x3f800001u,1u,0x7fc00000u}) {
            b.weights[0]=std::bit_cast<float>(bits);need(!admitted(),"Weight qualification crossed a finite bound");
        }
        b={};
        // A one-influence DP3 can stay normal while the final translation
        // cancels to a subnormal. Its lattice needs a separate DPH guard.
        v={};v.position={3,2,0};palette[0].fill(0);
        palette[0][0]=std::bit_cast<float>(0x0b800000u);
        palette[0][1]=std::bit_cast<float>(0x8b800000u);
        palette[0][3]=std::bit_cast<float>(0x8b800001u);
        need(!admitted(),"Adaptive qualification missed final DPH subnormal cancellation");
#if defined(__x86_64__) || defined(_M_X64)
        const auto before_counterexample=_mm_getcsr();_mm_setcsr(0x1fa0u);
        auto counterexample=v;skin_model_vertices({&counterexample,1},{&b,1},palette);
        need(std::bit_cast<std::uint32_t>(counterexample.position.x)==0x80400000u,
             "CPU DPH counterexample did not reach exact negative2^-127");
        _mm_setcsr(before_counterexample);
        const auto csr=_mm_getcsr();
        for(unsigned mode=0;mode<4;++mode)for(unsigned precision=0;precision<2;++precision)
            for(unsigned mask=0;mask<7;++mask)for(unsigned denorm=0;denorm<4;++denorm) {
                const auto current=0x1f80u|(mode<<13)|(precision<<5)|((denorm&1)<<6)|((denorm>>1)<<15);
                const auto tested=mask?current&~(1u<<(mask+6)):current;
                _mm_setcsr(tested);
                need(model_skin_gpu_host_effects_admitted()==(mode==0 && precision && !mask),"Host FP admission accepted changed rounding/unmasked/new sticky effects");
                need(_mm_getcsr()==tested,"Host effect admission changed MXCSR");
            }
        _mm_setcsr(csr);
        // Rejected signaling inputs must also be classified without raising
        // an exception or changing the owner's errno/sticky state.
        const auto saved_errno=errno;errno=EDOM;
        v.normal.x=std::bit_cast<float>(0x7f800001u);const auto before=_mm_getcsr();
        need(!admitted(),"Signaling NaN input was admitted");
        need(before==_mm_getcsr() && errno==EDOM,"Numeric qualification changed FP/errno on rejection");errno=saved_errno;
#endif
    }
    persistent=persistent_prepare_cases(gpu);
    const auto *skip_traffic=std::getenv("HALO2_TEST_GPU_SKIN_SKIP_PERSISTENT_TRAFFIC_GATE");
    const bool traffic_gate_skipped=skip_traffic && *skip_traffic;
    const bool traffic_passed=!persistent.traffic_violations;
    VkPhysicalDeviceProperties properties;vkGetPhysicalDeviceProperties(gpu.physical,&properties);
    gpu.close();need(!gpu.errors,"Vulkan validation rejected GPU skin storage/synchronization/lifetime");
    std::ofstream report(HALO2_MODEL_SKIN_GPU_REPORT);
    report<<"{\"passed\":"<<((traffic_passed || traffic_gate_skipped)?"true":"false")<<",\"device\":"<<std::quoted(properties.deviceName)<<",\"driver\":"<<properties.driverVersion
        <<",\"api\":"<<properties.apiVersion<<",\"complete_vertex_comparisons\":"<<comparisons
        <<",\"original_complete_vertex_comparisons\":12529,\"post_identity_complete_vertex_comparisons\":"<<post_comparisons
        <<",\"prepare_resource_reasons\":"<<prepare_failures.resource_reasons<<",\"prepare_fp_cases\":"<<prepare_failures.fp_cases
        <<",\"prepare_programming_precedence_cases\":"<<prepare_failures.programming_precedence_cases
        <<",\"prepare_hard_failures\":"<<prepare_failures.hard_failures<<",\"prepare_recovery_jobs\":"<<prepare_failures.recovery_jobs
        <<",\"prepare_empty_resets\":"<<prepare_failures.empty_resets
        <<",\"prepare_row_cache_cases\":"<<prepare_rows.cases<<",\"prepare_row_hard_rejections\":"<<prepare_rows.hard_rejections
        <<",\"prepare_row_quota_refusals\":"<<prepare_rows.quota_refusals
        <<",\"prepare_row_boundary_vertex_comparisons\":"<<prepare_rows.boundary_vertex_comparisons
        <<",\"persistent_gpu_jobs\":"<<persistent.gpu_jobs<<",\"persistent_complete_vertex_comparisons\":"<<persistent.complete_vertex_comparisons
        <<",\"persistent_fp_cases\":"<<persistent.fp_cases<<",\"persistent_refusals\":"<<persistent.refusals
        <<",\"persistent_hard_errors\":"<<persistent.hard_errors<<",\"persistent_empty_resets\":"<<persistent.empty_resets
        <<",\"persistent_warm_checks\":"<<persistent.warm_checks<<",\"persistent_traffic_violations\":"<<persistent.traffic_violations
        <<",\"persistent_allocating_prepares\":"<<persistent.growths
        <<",\"persistent_record_pack_bytes\":"<<persistent.record_pack_bytes<<",\"persistent_record_upload_bytes\":"<<persistent.record_upload_bytes
        <<",\"persistent_palette_upload_bytes\":"<<persistent.palette_upload_bytes<<",\"persistent_descriptor_writes\":"<<persistent.descriptor_writes
        <<",\"persistent_traffic_gate_skipped\":"<<(traffic_gate_skipped?"true":"false")
        <<",\"post_identity_sticky_cases\":128,\"sizes\":8,\"normal_lattice_boundary_cases\":6,\"minimum_normal_boundary_cases\":3,\"host_fp_control_cases\":224,\"validation_errors\":0}\n";
    need(bool(report),"Could not save the bounded GPU skin evidence");
    need(traffic_passed || traffic_gate_skipped,"Persistent GPU input traffic gate failed after complete correctness checks");
    std::fesetenv(&initial);std::cout<<"PASS "<<comparisons<<" complete vertex-byte comparisons (12529 original + "<<post_comparisons
        <<" mixed-mode placement), across8 sizes,0..4 influences, signed zero, interior zero/negative weights, two palette owners, permutation, growth/reuse, untouched metadata and rejected aliases; Vulkan errors=0\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
