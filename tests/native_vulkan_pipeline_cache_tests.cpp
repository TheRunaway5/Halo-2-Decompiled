#include <vulkan/vulkan.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>
#include <cstddef>
#include <cstdint>
namespace {
void need(bool b,const char* why){if(!b)throw std::runtime_error(why);}
template<class T>T handle(std::uintptr_t n){return reinterpret_cast<T>((std::uintptr_t{1}<<40)+n);}
VkPhysicalDeviceProperties properties(){VkPhysicalDeviceProperties p{};p.vendorID=0x10de;p.deviceID=0x2b80;p.driverVersion=531;for(unsigned i=0;i<VK_UUID_SIZE;++i)p.pipelineCacheUUID[i]=0x40+i;return p;}
std::vector<std::byte> data(){auto p=properties();VkPipelineCacheHeaderVersionOne h{};h.headerSize=sizeof(h);h.headerVersion=VK_PIPELINE_CACHE_HEADER_VERSION_ONE;h.vendorID=p.vendorID;h.deviceID=p.deviceID;std::memcpy(h.pipelineCacheUUID,p.pipelineCacheUUID,VK_UUID_SIZE);std::vector<std::byte>b(64,std::byte{0x63});std::memcpy(b.data(),&h,sizeof(h));return b;}
std::map<VkPipelineCache,VkDevice> caches;std::map<VkDevice,const VkAllocationCallbacks*> devices;
std::mutex calls_mutex;unsigned next_device{},next_cache{},cache_creates{},device_destroys{},cache_destroys{};std::atomic<unsigned> calls{};bool fail_device{},fail_cache{},reject_warm{};std::size_t initial_bytes{};VkResult pipeline_result=VK_SUCCESS;VkPipelineCache last_cache{};const void* last_info{};const VkAllocationCallbacks*last_allocator{};VkPipeline*last_output{};unsigned last_count{};
VkResult capture(VkDevice d,VkPipelineCache c,unsigned n,const void*i,const VkAllocationCallbacks*a,VkPipeline*out){std::lock_guard lock(calls_mutex);need(devices.contains(d),"Pipeline device ownership changed");if(c && caches.contains(c))need(caches.at(c)==d,"Shared cache crossed devices");last_cache=c;last_info=i;last_allocator=a;last_output=out;last_count=n;++calls;if(pipeline_result==VK_SUCCESS)for(unsigned j=0;j<n;++j)out[j]=handle<VkPipeline>(0x700+j);return pipeline_result;}
}
extern "C" {
VKAPI_ATTR VkResult VKAPI_CALL __wrap_vkCreateDevice(VkPhysicalDevice,const VkDeviceCreateInfo*,const VkAllocationCallbacks*,VkDevice*);
VKAPI_ATTR void VKAPI_CALL __wrap_vkDestroyDevice(VkDevice,const VkAllocationCallbacks*);
VKAPI_ATTR VkResult VKAPI_CALL __wrap_vkCreateGraphicsPipelines(VkDevice,VkPipelineCache,unsigned,const VkGraphicsPipelineCreateInfo*,const VkAllocationCallbacks*,VkPipeline*);
VKAPI_ATTR VkResult VKAPI_CALL __wrap_vkCreateComputePipelines(VkDevice,VkPipelineCache,unsigned,const VkComputePipelineCreateInfo*,const VkAllocationCallbacks*,VkPipeline*);
VKAPI_ATTR VkResult VKAPI_CALL __real_vkCreateDevice(VkPhysicalDevice,const VkDeviceCreateInfo*,const VkAllocationCallbacks*a,VkDevice*out){if(fail_device)return VK_ERROR_OUT_OF_DEVICE_MEMORY;*out=handle<VkDevice>(++next_device);devices[*out]=a;return VK_SUCCESS;}
VKAPI_ATTR void VKAPI_CALL __real_vkDestroyDevice(VkDevice d,const VkAllocationCallbacks*a){need(devices.at(d)==a,"Device allocator changed");need(std::none_of(caches.begin(),caches.end(),[&](auto&v){return v.second==d;}),"Pipeline cache outlived device");devices.erase(d);++device_destroys;}
VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties(VkPhysicalDevice,VkPhysicalDeviceProperties*out){*out=properties();}
VKAPI_ATTR VkResult VKAPI_CALL vkCreatePipelineCache(VkDevice d,const VkPipelineCacheCreateInfo*i,const VkAllocationCallbacks*a,VkPipelineCache*out){need(!a && !i->flags && !i->pNext,"Optional cache allocation or synchronization contract changed");++cache_creates;initial_bytes=i->initialDataSize;if(fail_cache || (reject_warm && initial_bytes))return VK_ERROR_OUT_OF_HOST_MEMORY;if(initial_bytes)need(initial_bytes==data().size() && std::memcmp(i->pInitialData,data().data(),data().size())==0,"Warm cache bytes changed");*out=handle<VkPipelineCache>(0x1000+ ++next_cache);caches[*out]=d;return VK_SUCCESS;}
VKAPI_ATTR void VKAPI_CALL vkDestroyPipelineCache(VkDevice d,VkPipelineCache c,const VkAllocationCallbacks*a){need(!a && caches.at(c)==d && devices.contains(d),"Cache teardown ownership changed");caches.erase(c);++cache_destroys;}
VKAPI_ATTR VkResult VKAPI_CALL vkGetPipelineCacheData(VkDevice d,VkPipelineCache c,std::size_t*n,void*out){need(caches.at(c)==d,"Readback crossed native cache owner");auto b=data();if(!out){*n=b.size();return VK_SUCCESS;}if(*n<b.size())return VK_INCOMPLETE;std::memcpy(out,b.data(),b.size());*n=b.size();return VK_SUCCESS;}
VKAPI_ATTR VkResult VKAPI_CALL __real_vkCreateGraphicsPipelines(VkDevice d,VkPipelineCache c,unsigned n,const VkGraphicsPipelineCreateInfo*i,const VkAllocationCallbacks*a,VkPipeline*out){return capture(d,c,n,i,a,out);}
VKAPI_ATTR VkResult VKAPI_CALL __real_vkCreateComputePipelines(VkDevice d,VkPipelineCache c,unsigned n,const VkComputePipelineCreateInfo*i,const VkAllocationCallbacks*a,VkPipeline*out){return capture(d,c,n,i,a,out);}
}
int main(int argc,char** argv)try{
 const auto* scratch=std::getenv("HALO2_TEST_SCRATCH");need(scratch,"Use the disposable test runner");const auto root=std::filesystem::path(scratch)/"pipelines";setenv("HALO2_PIPELINE_CACHE_DIRECTORY",root.c_str(),1);unsetenv("HALO2_DISABLE_PIPELINE_CACHE");
 if(argc==2 && std::strcmp(argv[1],"--late-destroy")==0) {
  const auto late_root=std::filesystem::path(scratch)/"late-pipelines";
  setenv("HALO2_PIPELINE_CACHE_DIRECTORY",late_root.c_str(),1);
  struct FinalCheck {
   std::filesystem::path root;
   ~FinalCheck() {
    try {
     std::vector<std::filesystem::path> files;
     if(std::filesystem::exists(root))for(const auto& entry:std::filesystem::directory_iterator(root))if(entry.path().extension()==".bin")files.push_back(entry.path());
     need(files.size()==1 && std::filesystem::file_size(files[0])==data().size(),"Late device teardown did not persist its pipeline cache");
     need(caches.empty() && devices.empty(),"Late device/cache ownership leaked");
     std::cout<<"{\"status\":\"GREEN\",\"late_static_device_teardown_persists_cache\":true}\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';std::_Exit(1);}
   }
  };
  static FinalCheck check{late_root};
  struct LateDevice {VkDevice device{};~LateDevice(){if(device)__wrap_vkDestroyDevice(device,nullptr);}};
  // Construct the device owner before its first cache lookup. Its destructor
  // then runs after function-local statics initialized by device creation.
  static LateDevice owner;
  VkDeviceCreateInfo info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
  need(__wrap_vkCreateDevice(handle<VkPhysicalDevice>(0x30),&info,nullptr,&owner.device)==VK_SUCCESS,"Late device creation failed");
  VkGraphicsPipelineCreateInfo graphic{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};VkPipeline output{};
  need(__wrap_vkCreateGraphicsPipelines(owner.device,VK_NULL_HANDLE,1,&graphic,nullptr,&output)==VK_SUCCESS,"Late pipeline creation failed");
  return 0;
 }
 VkDeviceCreateInfo info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};VkAllocationCallbacks allocator{};VkDevice device{};const auto physical=handle<VkPhysicalDevice>(0x30);unsigned checks{};
 need(__wrap_vkCreateDevice(physical,&info,&allocator,&device)==VK_SUCCESS && initial_bytes==0,"Cold cache/device creation changed");++checks;
 VkGraphicsPipelineCreateInfo graphic{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};VkComputePipelineCreateInfo compute{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};VkPipeline output{};
 need(__wrap_vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&graphic,&allocator,&output)==VK_SUCCESS && last_cache && reinterpret_cast<std::uintptr_t>(last_cache)>UINT32_MAX && last_info==&graphic && last_allocator==&allocator && last_output==&output && last_count==1,"Graphics arguments or64-bit cache owner changed");const auto owned=last_cache;++checks;
 need(__wrap_vkCreateComputePipelines(device,VK_NULL_HANDLE,1,&compute,nullptr,&output)==VK_SUCCESS && last_cache==owned && last_info==&compute && !last_allocator,"Compute cache/device ownership differs");++checks;
 const auto explicit_cache=handle<VkPipelineCache>(0x9000);need(__wrap_vkCreateGraphicsPipelines(device,explicit_cache,1,&graphic,nullptr,&output)==VK_SUCCESS && last_cache==explicit_cache,"Caller cache was replaced");++checks;
 VkBaseInStructure chain{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};graphic.pNext=&chain;need(__wrap_vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&graphic,nullptr,&output)==VK_SUCCESS && last_cache==VK_NULL_HANDLE,"Extension chain acquired an unrequested cache");graphic.pNext=nullptr;++checks;
 pipeline_result=VK_ERROR_UNKNOWN;need(__wrap_vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&graphic,nullptr,&output)==VK_ERROR_UNKNOWN,"Pipeline result was hidden");pipeline_result=VK_SUCCESS;++checks;
 std::array<std::thread,4> threads;std::array<bool,4> okay{};for(unsigned i=0;i<4;++i)threads[i]=std::thread([&,i]{VkPipeline out{};okay[i]=__wrap_vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&graphic,nullptr,&out)==VK_SUCCESS;});for(auto&t:threads)t.join();for(bool v:okay)need(v,"Concurrent cache access failed");++checks;
 __wrap_vkDestroyDevice(device,&allocator);need(caches.empty() && cache_destroys==1 && device_destroys==1,"Cache was not retired before device");++checks;
 std::vector<std::filesystem::path> files;for(const auto&e:std::filesystem::directory_iterator(root))if(e.path().extension()==".bin")files.push_back(e.path());need(files.size()==1 && std::filesystem::file_size(files[0])==64,"Cache did not persist atomically");++checks;
 need(__wrap_vkCreateDevice(physical,&info,nullptr,&device)==VK_SUCCESS && initial_bytes==64,"Warm cache was not reused");__wrap_vkDestroyDevice(device,nullptr);++checks;
 // Driver rejection of an otherwise qualified file retries an empty cache.
 reject_warm=true;const auto before=cache_creates;need(__wrap_vkCreateDevice(physical,&info,nullptr,&device)==VK_SUCCESS && cache_creates==before+2 && initial_bytes==0,"Rejected driver data did not fall back");reject_warm=false;__wrap_vkDestroyDevice(device,nullptr);++checks;
 {std::fstream f(files[0],std::ios::in|std::ios::out|std::ios::binary);f.seekp(16);f.put('\0');}need(__wrap_vkCreateDevice(physical,&info,nullptr,&device)==VK_SUCCESS && initial_bytes==0,"Wrong UUID was consumed");__wrap_vkDestroyDevice(device,nullptr);++checks;
 std::filesystem::resize_file(files[0],65ull*1024*1024);need(__wrap_vkCreateDevice(physical,&info,nullptr,&device)==VK_SUCCESS && initial_bytes==0,"Oversized cache was read");__wrap_vkDestroyDevice(device,nullptr);++checks;
 fail_cache=true;need(__wrap_vkCreateDevice(physical,&info,nullptr,&device)==VK_SUCCESS,"Optional cache failure failed device creation");need(__wrap_vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&graphic,nullptr,&output)==VK_SUCCESS && !last_cache,"Cache allocation failure changed pipeline path");fail_cache=false;__wrap_vkDestroyDevice(device,nullptr);++checks;
 fail_device=true;const auto before_failed=cache_creates;need(__wrap_vkCreateDevice(physical,&info,nullptr,&device)==VK_ERROR_OUT_OF_DEVICE_MEMORY && cache_creates==before_failed,"Failed device acquired cache state");fail_device=false;++checks;
 setenv("HALO2_DISABLE_PIPELINE_CACHE","1",1);const auto before_disabled=cache_creates;need(__wrap_vkCreateDevice(physical,&info,nullptr,&device)==VK_SUCCESS && cache_creates==before_disabled,"Disabled cache was created");need(__wrap_vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&graphic,nullptr,&output)==VK_SUCCESS && !last_cache,"Disabled cache changed call");__wrap_vkDestroyDevice(device,nullptr);unsetenv("HALO2_DISABLE_PIPELINE_CACHE");++checks;
 const auto blocked=std::filesystem::path(scratch)/"blocked";{std::ofstream f(blocked);f<<'x';}setenv("HALO2_PIPELINE_CACHE_DIRECTORY",blocked.c_str(),1);need(__wrap_vkCreateDevice(physical,&info,nullptr,&device)==VK_SUCCESS,"Unwritable persistence path failed device creation");__wrap_vkDestroyDevice(device,nullptr);++checks;
 need(caches.empty() && devices.empty(),"Native device/cache ownership leaked");std::cout<<"{\"status\":\"GREEN\",\"control_groups\":"<<checks<<",\"graphics_and_compute_arguments_results_allocators_preserved\":true,\"native_handles_above4GiB\":true,\"no_shader_or_pipeline_description_changes\":true}\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
