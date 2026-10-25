// Loader-free constructor-failure regression. This executable defines the Vulkan
// calls used by the service and must not link the Vulkan loader. Its deterministic
// owned-file short-read seam and strict 32-byte fenv contract require Linux x86-64.
// The 4-byte SPIR-V magic is a mock input, never submitted to a GPU.
#if !defined(__linux__) || !defined(__x86_64__)
#error "model_skin_gpu_initialization_tests requires Linux x86-64"
#endif
#include <cstdlib>
#include <new>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

namespace host_fault {
bool enabled{},arm_on_vk_error{};std::size_t count{},nth{},size{},failures{};
void arm_diagnostic(){enabled=true;count=0;nth=1;size=0;}
bool fail(std::size_t n){if(!enabled)return false;++count;const bool reject=(nth && count==nth)||(size && n==size);if(reject)++failures;return reject;}
}
void* operator new(std::size_t n){if(host_fault::fail(n))throw std::bad_alloc{};if(auto*p=std::malloc(n?n:1))return p;throw std::bad_alloc{};}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void*p)noexcept{std::free(p);}void operator delete[](void*p)noexcept{std::free(p);}
void operator delete(void*p,std::size_t)noexcept{std::free(p);}void operator delete[](void*p,std::size_t)noexcept{std::free(p);}
namespace short_read {
bool enabled{},arm_diagnostic{};dev_t device{};ino_t inode{};int writer=-1;unsigned truncated{};
}
extern "C" ssize_t read(int fd,void*data,std::size_t count){
    if(short_read::enabled){struct stat info{};if(syscall(SYS_fstat,fd,&info)==0 && info.st_dev==short_read::device && info.st_ino==short_read::inode){
        short_read::enabled=false;if(syscall(SYS_ftruncate,short_read::writer,0)==0)++short_read::truncated;
    }}
    const auto result=syscall(SYS_read,fd,data,count);
    if(short_read::arm_diagnostic && short_read::truncated){short_read::arm_diagnostic=false;host_fault::arm_diagnostic();}
    return result;
}
#include "rendering/models/model_skin_gpu.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cerrno>
#include <cfenv>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <type_traits>
#include <vector>
using namespace halo2;
void need(bool x,const char*m){if(!x)throw std::runtime_error(m);}
template<class T>T h(std::uint64_t n){if constexpr(std::is_pointer_v<T>)return reinterpret_cast<T>(n);else return T(n);}
template<class T>std::uint64_t id(T n){if constexpr(std::is_pointer_v<T>)return reinterpret_cast<std::uintptr_t>(n);else return n;}
struct Memory {std::vector<std::byte> bytes;bool mapped{};};
struct Mock {
 std::uint64_t next=100;std::map<std::uint64_t,std::string> objects;
 std::map<std::uint64_t,std::size_t> sizes;std::map<std::uint64_t,Memory> memory;
 std::string fail_stage;VkResult fail_result=VK_SUCCESS;bool unsupported{};unsigned stage_calls{};
 bool fail(const char*stage){++stage_calls;events.push_back(std::string("stage:")+stage);errno=EAGAIN;
  unsigned mxcsr;unsigned short cw;asm volatile("stmxcsr %0":"=m"(mxcsr));asm volatile("fnstcw %0":"=m"(cw));
  mxcsr=(mxcsr&~0x6000u)|0x2000u|0x20u;cw=(cw&~0x0c00u)|0x0400u;
  asm volatile("ldmxcsr %0"::"m"(mxcsr));asm volatile("fldcw %0"::"m"(cw));
  const bool rejected=fail_stage==stage;if(rejected && host_fault::arm_on_vk_error)host_fault::arm_diagnostic();return rejected;}
 VkResult bind_result=VK_SUCCESS;unsigned binds{},maps{},unmaps{},updates{},dispatches{};
 std::vector<std::string> events;
 template<class T>T create(const char*kind){auto n=next++;objects.emplace(n,kind);events.push_back(std::string("create:")+kind);return h<T>(n);}
 template<class T>void destroy(T handle,const char*kind){
  // Mock event bookkeeping must not consume the allocation fault reserved for
  // the service's actual hard-error diagnostic after partial-resource cleanup.
  const bool armed=host_fault::enabled;host_fault::enabled=false;
  need(objects.at(id(handle))==kind,"wrong destruction");objects.erase(id(handle));events.push_back(std::string("destroy:")+kind);host_fault::enabled=armed;}
} vk;
extern "C" {
VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties(VkPhysicalDevice,VkPhysicalDeviceProperties*p){*p={};p->apiVersion=vk.unsupported?VK_API_VERSION_1_1:VK_API_VERSION_1_2;p->limits.maxStorageBufferRange=1u<<24;p->limits.maxComputeWorkGroupCount[0]=65535;}
VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties2(VkPhysicalDevice d,VkPhysicalDeviceProperties2*p){vkGetPhysicalDeviceProperties(d,&p->properties);auto*f=static_cast<VkPhysicalDeviceFloatControlsProperties*>(p->pNext);f->shaderRoundingModeRTEFloat32=VK_TRUE;f->shaderSignedZeroInfNanPreserveFloat32=VK_TRUE;}
VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice,VkPhysicalDeviceMemoryProperties*p){*p={};p->memoryTypeCount=1;p->memoryTypes[0].propertyFlags=VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;}
VKAPI_ATTR VkResult VKAPI_CALL vkCreateDescriptorSetLayout(VkDevice,const VkDescriptorSetLayoutCreateInfo*,const VkAllocationCallbacks*,VkDescriptorSetLayout*p){if(vk.fail("descriptor-layout")){*p={};return vk.fail_result;}*p=vk.create<VkDescriptorSetLayout>("setlayout");return VK_SUCCESS;}
VKAPI_ATTR void VKAPI_CALL vkDestroyDescriptorSetLayout(VkDevice,VkDescriptorSetLayout p,const VkAllocationCallbacks*){vk.destroy(p,"setlayout");}
VKAPI_ATTR VkResult VKAPI_CALL vkCreateDescriptorPool(VkDevice,const VkDescriptorPoolCreateInfo*,const VkAllocationCallbacks*,VkDescriptorPool*p){if(vk.fail("descriptor-pool")){*p={};return vk.fail_result;}*p=vk.create<VkDescriptorPool>("pool");return VK_SUCCESS;}
VKAPI_ATTR void VKAPI_CALL vkDestroyDescriptorPool(VkDevice,VkDescriptorPool p,const VkAllocationCallbacks*){vk.destroy(p,"pool");}
VKAPI_ATTR VkResult VKAPI_CALL vkAllocateDescriptorSets(VkDevice,const VkDescriptorSetAllocateInfo*,VkDescriptorSet*p){if(vk.fail("descriptor-set")){*p={};return vk.fail_result;}*p=h<VkDescriptorSet>(55);return VK_SUCCESS;}
VKAPI_ATTR VkResult VKAPI_CALL vkCreatePipelineLayout(VkDevice,const VkPipelineLayoutCreateInfo*,const VkAllocationCallbacks*,VkPipelineLayout*p){if(vk.fail("pipeline-layout")){*p={};return vk.fail_result;}*p=vk.create<VkPipelineLayout>("layout");return VK_SUCCESS;}
VKAPI_ATTR void VKAPI_CALL vkDestroyPipelineLayout(VkDevice,VkPipelineLayout p,const VkAllocationCallbacks*){vk.destroy(p,"layout");}
VKAPI_ATTR VkResult VKAPI_CALL vkCreateShaderModule(VkDevice,const VkShaderModuleCreateInfo*i,const VkAllocationCallbacks*,VkShaderModule*p){need(i->codeSize==4 && i->pCode[0]==0x07230203u,"declared mock shader bytes");if(vk.fail("shader-module")){*p={};return vk.fail_result;}*p=vk.create<VkShaderModule>("shader");return VK_SUCCESS;}
VKAPI_ATTR void VKAPI_CALL vkDestroyShaderModule(VkDevice,VkShaderModule p,const VkAllocationCallbacks*){vk.destroy(p,"shader");}
VKAPI_ATTR VkResult VKAPI_CALL vkCreateComputePipelines(VkDevice,VkPipelineCache,std::uint32_t n,const VkComputePipelineCreateInfo*,const VkAllocationCallbacks*,VkPipeline*p){need(n==1,"pipeline count");if(vk.fail("compute-pipeline")){*p={};return vk.fail_result;}*p=vk.create<VkPipeline>("pipeline");return VK_SUCCESS;}
VKAPI_ATTR void VKAPI_CALL vkDestroyPipeline(VkDevice,VkPipeline p,const VkAllocationCallbacks*){vk.destroy(p,"pipeline");}
VKAPI_ATTR VkResult VKAPI_CALL vkCreateBuffer(VkDevice,const VkBufferCreateInfo*i,const VkAllocationCallbacks*,VkBuffer*p){*p=vk.create<VkBuffer>("buffer");vk.sizes[id(*p)]=i->size;return VK_SUCCESS;}
VKAPI_ATTR void VKAPI_CALL vkDestroyBuffer(VkDevice,VkBuffer p,const VkAllocationCallbacks*){vk.sizes.erase(id(p));vk.destroy(p,"buffer");}
VKAPI_ATTR void VKAPI_CALL vkGetBufferMemoryRequirements(VkDevice,VkBuffer b,VkMemoryRequirements*p){*p={};p->size=vk.sizes.at(id(b));p->alignment=1;p->memoryTypeBits=1;}
VKAPI_ATTR VkResult VKAPI_CALL vkAllocateMemory(VkDevice,const VkMemoryAllocateInfo*i,const VkAllocationCallbacks*,VkDeviceMemory*p){*p=vk.create<VkDeviceMemory>("memory");vk.memory.emplace(id(*p),Memory{std::vector<std::byte>(i->allocationSize),false});return VK_SUCCESS;}
VKAPI_ATTR void VKAPI_CALL vkFreeMemory(VkDevice,VkDeviceMemory p,const VkAllocationCallbacks*){need(!vk.memory.at(id(p)).mapped,"freed mapped memory");vk.memory.erase(id(p));vk.destroy(p,"memory");}
VKAPI_ATTR VkResult VKAPI_CALL vkBindBufferMemory(VkDevice,VkBuffer b,VkDeviceMemory m,VkDeviceSize off){need(off==0 && vk.sizes.at(id(b))<=vk.memory.at(id(m)).bytes.size(),"binding extents");++vk.binds;vk.events.push_back("bind:"+std::to_string(vk.bind_result));errno=EAGAIN;return vk.bind_result;}
VKAPI_ATTR VkResult VKAPI_CALL vkMapMemory(VkDevice,VkDeviceMemory m,VkDeviceSize offset,VkDeviceSize bytes,VkMemoryMapFlags,void**p){auto&v=vk.memory.at(id(m));need(!v.mapped && offset==0 && bytes<=v.bytes.size(),"invalid/duplicate map");v.mapped=true;*p=v.bytes.data();++vk.maps;vk.events.push_back("map");return VK_SUCCESS;}
VKAPI_ATTR void VKAPI_CALL vkUnmapMemory(VkDevice,VkDeviceMemory m){auto&v=vk.memory.at(id(m));need(v.mapped,"unmap of unmapped memory");v.mapped=false;++vk.unmaps;vk.events.push_back("unmap");}
VKAPI_ATTR void VKAPI_CALL vkUpdateDescriptorSets(VkDevice,std::uint32_t n,const VkWriteDescriptorSet*,std::uint32_t,const VkCopyDescriptorSet*){vk.updates+=n;vk.events.push_back("descriptors");}
VKAPI_ATTR void VKAPI_CALL vkCmdBindPipeline(VkCommandBuffer,VkPipelineBindPoint,VkPipeline){}
VKAPI_ATTR void VKAPI_CALL vkCmdBindDescriptorSets(VkCommandBuffer,VkPipelineBindPoint,VkPipelineLayout,std::uint32_t,std::uint32_t,const VkDescriptorSet*,std::uint32_t,const std::uint32_t*){}
VKAPI_ATTR void VKAPI_CALL vkCmdPushConstants(VkCommandBuffer,VkPipelineLayout,VkShaderStageFlags,std::uint32_t,std::uint32_t,const void*){}
VKAPI_ATTR void VKAPI_CALL vkCmdDispatch(VkCommandBuffer,std::uint32_t x,std::uint32_t y,std::uint32_t z){need(x>0 && y==1 && z==1,"actual dispatch shape");++vk.dispatches;}
VKAPI_ATTR void VKAPI_CALL vkCmdPipelineBarrier(VkCommandBuffer,VkPipelineStageFlags,VkPipelineStageFlags,VkDependencyFlags,std::uint32_t,const VkMemoryBarrier*,std::uint32_t,const VkBufferMemoryBarrier*,std::uint32_t,const VkImageMemoryBarrier*){}
}

struct Fp {std::array<unsigned char,32> bytes{};int error{};
 static Fp get(){static_assert(sizeof(std::fenv_t)==32);Fp p;std::fenv_t value{};p.error=errno;need(std::fegetenv(&value)==0,"capture fenv");std::memcpy(p.bytes.data(),&value,32);return p;}
 bool operator==(const Fp&)const=default;
};
struct Row {std::string name,outcome,message;int result{};bool equal_fp{},clean{},recovered{},diagnostic_oom{};unsigned calls{};};
int main(){
 std::vector<Row> rows;std::string error;bool passed=true;unsigned direct_constructor_errors{};
 try {
    const auto *scratch=std::getenv("HALO2_TEST_SCRATCH");
    need(scratch && *scratch,"HALO2_TEST_SCRATCH is required; run via tools/run_test.py");
    const std::filesystem::path good=std::filesystem::path(scratch)/"model-skin-initialization.spv";
    unsetenv("HALO2_TEST_GPU_SKIN_PREPARE_FAILURE");unsetenv("HALO2_REFERENCE_STREAMED_SKIN_INPUTS");unsetenv("HALO2_REFERENCE_SKIN_RECORD_VALIDATION");
    const auto bytes=[](const std::filesystem::path&path,std::size_t size){std::vector<unsigned char> b(size);if(size>=4){b[0]=3;b[1]=2;b[2]=0x23;b[3]=7;}std::ofstream f(path,std::ios::binary|std::ios::trunc);f.write(reinterpret_cast<const char*>(b.data()),b.size());need(bool(f),"write fixture shader");};
    const auto factory=[&](const std::filesystem::path&path){return ModelSkinGpu::create_optional(h<VkDevice>(1),h<VkPhysicalDevice>(2),VK_API_VERSION_1_2,path);};
    const auto recover=[&]{vk.fail_stage.clear();vk.unsupported=false;bytes(good,4);auto p=factory(good);need(bool(p)&&p->vertices()==0,"cold factory recovery failed");p.reset();need(vk.objects.empty(),"recovery leaked partial handles");};
    const auto run=[&](std::string name,const std::filesystem::path&path,bool expected_null,bool expected_hard,bool expected_diagnostic_oom=false) {
        Row row;row.name=std::move(name);row.result=vk.fail_result;const auto count=vk.stage_calls;
        const auto failures=host_fault::failures;
        errno=ERANGE;const auto before=Fp::get();
        try {auto service=factory(path);row.outcome=service?"service":"null";if(service)need(service->vertices()==0,"new service published dispatch");}
        catch(const FormatError&e){row.outcome="hard";row.message=e.what();}
        catch(const std::bad_alloc&){row.outcome="hard-bad-alloc";}
        host_fault::enabled=false;short_read::enabled=false;
        host_fault::arm_on_vk_error=false;short_read::arm_diagnostic=false;host_fault::nth=0;
        row.diagnostic_oom=expected_diagnostic_oom && host_fault::failures==failures+1;
        row.equal_fp=before==Fp::get();row.calls=vk.stage_calls-count;row.clean=vk.objects.empty()&&vk.memory.empty()&&vk.sizes.empty();
        if(!vk.fail_stage.empty()) {
            const std::array<const char*,6> stages{"descriptor-layout","descriptor-pool","descriptor-set","pipeline-layout","shader-module","compute-pipeline"};
            const auto stage=std::find(stages.begin(),stages.end(),vk.fail_stage);
            need(stage!=stages.end() && row.calls==unsigned(stage-stages.begin()+1),"failed stage did not execute once or crossed later constructor stage");
            if(expected_hard)need(row.message=="model skin GPU Vulkan operation failed: "+std::to_string(vk.fail_result),"hard device error diagnostic changed");
        }
        rows.push_back(row);
        need(row.equal_fp,"optional initialization changed complete32-byte fenv/errno");need(row.clean,"failed/successful factory leaked live handles");
        need(row.outcome==(expected_diagnostic_oom?"hard-bad-alloc":expected_null?"null":expected_hard?"hard":"service"),"wrong optional/hard initialization result");
        if(expected_diagnostic_oom)need(row.diagnostic_oom,"actual hard diagnostic allocation did not fail exactly once");
        recover();rows.back().recovered=true;
    };
    bytes(good,4);
    for(const char*stage:{"descriptor-layout","descriptor-pool","descriptor-set","pipeline-layout","shader-module","compute-pipeline"})
      for(VkResult result:{VK_ERROR_OUT_OF_HOST_MEMORY,VK_ERROR_OUT_OF_DEVICE_MEMORY,VK_ERROR_DEVICE_LOST,VK_ERROR_UNKNOWN}){
        vk=Mock{};vk.fail_stage=stage;vk.fail_result=result;
        run(std::string(stage)+":"+std::to_string(result),good,result==VK_ERROR_OUT_OF_HOST_MEMORY||result==VK_ERROR_OUT_OF_DEVICE_MEMORY,result==VK_ERROR_DEVICE_LOST||result==VK_ERROR_UNKNOWN);
      }
    for(unsigned nth:{1u,2u}){vk=Mock{};auto name="make-unique-bad-alloc-"+std::to_string(nth);host_fault::enabled=true;host_fault::count=0;host_fault::nth=nth;host_fault::size=0;run(std::move(name),good,true,false);host_fault::nth=0;}
    vk=Mock{};host_fault::enabled=true;host_fault::count=0;host_fault::size=4;run("shader-vector-bad-alloc",good,true,false);host_fault::size=0;
    vk=Mock{};run("normal-optional-factory",good,false,false);
    vk=Mock{};vk.unsupported=true;run("unsupported-capability",good,false,true);
    vk=Mock{};run("missing-shader",good.string()+".missing",false,true);
    vk=Mock{};bytes(good,0);run("empty-shader",good,false,true);
    vk=Mock{};bytes(good,3);run("unaligned-shader",good,false,true);
    vk=Mock{};bytes(good,4);
    short_read::writer=open(good.c_str(),O_WRONLY|O_CLOEXEC);need(short_read::writer>=0,"open controlled truncation writer");
    struct stat identity{};need(fstat(short_read::writer,&identity)==0,"controlled truncation identity");short_read::device=identity.st_dev;short_read::inode=identity.st_ino;short_read::enabled=true;
    run("real-short-shader-read",good,false,true);close(short_read::writer);short_read::writer=-1;need(short_read::truncated==1 && rows.back().message=="model skin GPU shader is truncated","did not execute original truncated-read diagnostic");
    // Fail the first allocation after an actual hard VkResult or short read,
    // including paths with partially initialized resources. These failures must
    // escape as hard bad_alloc rather than being mistaken for optional refusal.
    for(VkResult result:{VK_ERROR_DEVICE_LOST,VK_ERROR_UNKNOWN}) {
        vk=Mock{};vk.fail_stage="compute-pipeline";vk.fail_result=result;host_fault::arm_on_vk_error=true;
        run("hard-vulkan-diagnostic-oom:"+std::to_string(result),good,false,false,true);
    }
    vk=Mock{};bytes(good,4);short_read::truncated=0;
    short_read::writer=open(good.c_str(),O_WRONLY|O_CLOEXEC);need(short_read::writer>=0,"open combined-fault writer");
    need(fstat(short_read::writer,&identity)==0,"combined-fault shader identity");short_read::device=identity.st_dev;short_read::inode=identity.st_ino;short_read::enabled=true;short_read::arm_diagnostic=true;
    run("hard-truncated-shader-diagnostic-oom",good,false,false,true);close(short_read::writer);short_read::writer=-1;need(short_read::truncated==1,"combined-fault shader was not actually truncated");
    // Both successful entry points must issue the same real constructor-call
    // sequence. No GPU output or driver conformance is inferred from the stub.
    vk=Mock{};{auto direct=std::make_unique<ModelSkinGpu>(h<VkDevice>(1),h<VkPhysicalDevice>(2),VK_API_VERSION_1_2,good);}const auto direct_events=vk.events;
    vk=Mock{};{auto optional=factory(good);need(bool(optional),"successful optional creation missing");}need(vk.events==direct_events && vk.objects.empty(),"factory changed successful constructor resource sequence");
    need(rows.size()==36 && std::all_of(rows.begin(),rows.end(),[](const Row&r){return r.equal_fp && r.clean && r.recovered;}),"incomplete constructor initialization matrix");
    // Direct construction keeps its original hard FormatError interface,
    // including Vulkan OOM; only the optional factory may decline allocation.
    const auto direct_error=[&](const std::string&expected) {
        bool hard=false;
        try {ModelSkinGpu direct(h<VkDevice>(1),h<VkPhysicalDevice>(2),VK_API_VERSION_1_2,good);}
        catch(const FormatError&e){hard=expected==e.what();}
        need(hard && vk.objects.empty() && vk.memory.empty() && vk.sizes.empty(),"direct constructor error type/diagnostic/cleanup changed");
        ++direct_constructor_errors;
    };
    for(VkResult result:{VK_ERROR_OUT_OF_HOST_MEMORY,VK_ERROR_OUT_OF_DEVICE_MEMORY,VK_ERROR_DEVICE_LOST,VK_ERROR_UNKNOWN}) {
        vk=Mock{};vk.fail_stage="compute-pipeline";vk.fail_result=result;
        direct_error("model skin GPU Vulkan operation failed: "+std::to_string(result));
    }
    vk=Mock{};vk.unsupported=true;direct_error("model skin GPU lacks explicit float32 RTE/signed-zero controls");
    vk=Mock{};bytes(good,0);direct_error("model skin GPU shader is missing or malformed");
    need(direct_constructor_errors==6,"incomplete direct constructor compatibility checks");
 } catch(const std::exception&e){passed=false;error=e.what();host_fault::enabled=false;short_read::enabled=false;}
 if(short_read::writer>=0){close(short_read::writer);short_read::writer=-1;}
 std::ostream &out=std::cout;out<<"{\"schema\":\"H2_GPU_SKIN_OPTIONAL_INITIALIZATION_V1\",\"passed\":"<<(passed?"true":"false")<<",\"error\":"<<std::quoted(error)<<",\"mock_vulkan_only\":true,\"actual_gpu_output_qualified\":false,\"direct_constructor_errors_checked\":"<<direct_constructor_errors<<",\"rows\":[";
 for(unsigned i=0;i<rows.size();++i){if(i)out<<',';const auto&r=rows[i];out<<"{\"name\":"<<std::quoted(r.name)<<",\"outcome\":"<<std::quoted(r.outcome)<<",\"diagnostic\":"<<std::quoted(r.message)<<",\"injected_result\":"<<r.result<<",\"full_fenv_errno_equal\":"<<r.equal_fp<<",\"all_handles_freed\":"<<r.clean<<",\"cold_recovery\":"<<r.recovered<<",\"actual_diagnostic_allocation_failed\":"<<r.diagnostic_oom<<",\"actual_constructor_stage_calls\":"<<r.calls<<'}';}out<<"]}\n";
 if(!error.empty())std::cerr<<error<<'\n';return passed?0:1;
}
