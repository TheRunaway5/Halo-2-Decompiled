#include "halo2/source_world_draw_storage.hpp"
#include "halo2/cache.hpp"
#include "rendering/renderer/renderer_shadow_bitmap_pages.hpp"
#include <cstring>
#include <iostream>
#include <map>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace {
template<class T>T handle(std::uintptr_t value) {
    if constexpr(std::is_pointer_v<T>)return reinterpret_cast<T>(value);
    else return static_cast<T>(value);
}
template<class T>std::uintptr_t key(T value) {
    if constexpr(std::is_pointer_v<T>)return reinterpret_cast<std::uintptr_t>(value);
    else return value;
}
using ImageKey=std::pair<unsigned,unsigned>;
struct Driver {
    std::uintptr_t next=100;
    unsigned updates{},copies{},resets{};
    std::map<std::uintptr_t,VkDeviceSize> buffers;
    std::map<std::uintptr_t,std::vector<std::byte>> memory;
    std::map<std::uintptr_t,std::map<ImageKey,VkDescriptorImageInfo>> images;
    std::map<std::pair<std::uintptr_t,unsigned>,VkDescriptorBufferInfo> uniforms;
} driver;
void check(bool value,const char* message) {if(!value)throw std::runtime_error(message);}
bool same(const VkDescriptorImageInfo& a,const VkDescriptorImageInfo& b) {
    return a.sampler==b.sampler && a.imageView==b.imageView && a.imageLayout==b.imageLayout;
}
template<class F>void rejects(F&& fn) {
    const auto calls=driver.updates;
    try{fn();}catch(const halo2::FormatError&){check(driver.updates==calls,"rejected frame changed live descriptors");return;}
    throw std::runtime_error("invalid per-draw image bank was accepted");
}
void metadata() {
    using halo2::detail::shadow_bitmap_page_matches;
    halo2::Texture source{1,1,11,{std::byte{1},std::byte{2},std::byte{3},std::byte{4}},{{1,1,0,4}}};
    auto shape=source;shape.pixels.clear();const auto pin=source.pixels.fingerprint();
    const auto matches=[&](const auto& value,std::uint64_t hash,unsigned mips=1,bool authored=true,bool mutated=false) {
        return shadow_bitmap_page_matches(source,value,hash,mips,authored,mutated);
    };
    check(matches(shape,pin),"actual uploaded source page rejected");
    check(!matches(shape,pin^1),"another object's bitmap pixels were accepted");
    check(!matches(shape,pin,2),"different mip count accepted");
    check(!matches(shape,pin,1,false),"fallback image accepted as authored frame");
    check(!matches(shape,pin,1,true,true),"mutated uploaded frame accepted");
    auto wrong=shape;wrong.mips[0].offset=1;check(!matches(wrong,pin),"different mip layout accepted");
    wrong=shape;wrong.layers=6;check(!matches(wrong,pin),"different image dimensions accepted");
    wrong=shape;wrong.depth=1;check(!matches(wrong,pin),"different volume owner accepted");
    wrong=shape;wrong.pixels={std::byte{1}};check(!matches(wrong,pin),"unretired CPU pixels accepted as uploaded shape");
    source.pixels[0]^=std::byte{1};check(!matches(shape,pin),"stale source frame identity accepted");
}
void isolation() {
    using namespace halo2;
    const auto device=handle<VkDevice>(1);
    const auto physical=handle<VkPhysicalDevice>(2);
    const auto resident=handle<VkDescriptorSet>(3);const auto sampler=handle<VkSampler>(4);
    auto& original=driver.images[key(resident)];
    for(unsigned table=0;table<3;++table)for(unsigned stage=0;stage<4;++stage)
        original[{table?table+4:stage,table?stage:0}]={sampler,handle<VkImageView>(10+table*4+stage),VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    const auto snapshot=original;
    SourceWorldDrawStorage storage({device,physical,handle<VkDescriptorSetLayout>(5),32});
    std::array<std::byte,32> uniform{};
    std::array<SourceWorldDrawUpload,2> draws;
    for(auto& draw:draws){draw.resident=resident;draw.material_uniform=uniform;draw.vertex_bank.range={34,158,0,0};}
    const auto selected=[&](unsigned frame) {
        return SourceWorldDrawStageImages{sampler,handle<VkImageView>(50+frame),handle<VkImageView>(60),handle<VkImageView>(61)};
    };
    draws[0].stage_images[0]=selected(0);draws[1].stage_images[0]=selected(1);
    auto upload=storage.upload(draws);std::vector<VkDescriptorSet> first(upload.begin(),upload.end());
    check(first.size()==2 && first[0]!=first[1],"same-material owners shared a mutable descriptor");
    for(unsigned owner=0;owner<2;++owner) {
        const auto& bank=driver.images.at(key(first[owner]));
        check(bank.at({0,0}).imageView==handle<VkImageView>(50+owner),"selected owner frame was not bound");
        check(bank.at({5,0}).imageView==handle<VkImageView>(60) && bank.at({6,0}).imageView==handle<VkImageView>(61),
              "inactive cube/volume fallbacks were lost");
        for(unsigned stage=1;stage<4;++stage)for(unsigned table=0;table<3;++table) {
            const ImageKey index{table?table+4:stage,table?stage:0};
            check(same(bank.at(index),snapshot.at(index)),"another stage's resident image changed");
        }
    }
    check(driver.uniforms.at({key(first[0]),4}).offset!=driver.uniforms.at({key(first[1]),4}).offset,
          "per-owner material constants shared a UBO slice");
    for(const auto& [index,image]:snapshot)check(same(driver.images.at(key(resident)).at(index),image),"global resident set changed");
    // A completed fence permits the next upload to recycle storage; the
    // selected images reverse without carrying either previous owner forward.
    draws[0].stage_images[0]=selected(1);draws[1].stage_images[0]=selected(0);
    upload=storage.upload(draws);
    check(driver.images.at(key(upload[0])).at({0,0}).imageView==handle<VkImageView>(51) &&
          driver.images.at(key(upload[1])).at({0,0}).imageView==handle<VkImageView>(50),"reused per-owner frame selection leaked");
    auto invalid=draws[0];invalid.stage_images[0]->cube=VK_NULL_HANDLE;rejects([&]{storage.upload({&invalid,1});});
    invalid=draws[0];invalid.stage_images[0]->sampler=VK_NULL_HANDLE;rejects([&]{storage.upload({&invalid,1});});
    invalid=draws[0];invalid.target_view=handle<VkImageView>(70);invalid.target_sampler=sampler;
    rejects([&]{storage.upload({&invalid,1});});
    invalid.stage_images[0].reset();invalid.stage_images[1]=selected(1);upload=storage.upload({&invalid,1});
    check(driver.images.at(key(upload[0])).at({0,0}).imageView==handle<VkImageView>(70),"external target was displaced by another stage");
    check(driver.images.at(key(upload[0])).at({1,0}).imageView==handle<VkImageView>(51),"independent animated stage was not selected");
    check(driver.copies==6*5 && driver.resets==3,"unexpected upload ownership/lifecycle calls");
}
}

extern "C" {
VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties(VkPhysicalDevice,VkPhysicalDeviceProperties* p) {
    *p={};p->limits.maxUniformBufferRange=65536;p->limits.minUniformBufferOffsetAlignment=16;
}
VKAPI_ATTR VkResult VKAPI_CALL vkCreateBuffer(VkDevice,const VkBufferCreateInfo* p,const VkAllocationCallbacks*,VkBuffer* out) {
    *out=handle<VkBuffer>(driver.next++);driver.buffers[key(*out)]=p->size;return VK_SUCCESS;
}
VKAPI_ATTR void VKAPI_CALL vkGetBufferMemoryRequirements(VkDevice,VkBuffer b,VkMemoryRequirements* p) {
    *p={driver.buffers.at(key(b)),16,1};
}
VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice,VkPhysicalDeviceMemoryProperties* p) {
    *p={};p->memoryTypeCount=1;p->memoryTypes[0].propertyFlags=VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
}
VKAPI_ATTR VkResult VKAPI_CALL vkAllocateMemory(VkDevice,const VkMemoryAllocateInfo* p,const VkAllocationCallbacks*,VkDeviceMemory* out) {
    *out=handle<VkDeviceMemory>(driver.next++);driver.memory[key(*out)].resize(p->allocationSize);return VK_SUCCESS;
}
VKAPI_ATTR VkResult VKAPI_CALL vkBindBufferMemory(VkDevice,VkBuffer,VkDeviceMemory,VkDeviceSize){return VK_SUCCESS;}
VKAPI_ATTR VkResult VKAPI_CALL vkCreateDescriptorPool(VkDevice,const VkDescriptorPoolCreateInfo*,const VkAllocationCallbacks*,VkDescriptorPool* out) {
    *out=handle<VkDescriptorPool>(driver.next++);return VK_SUCCESS;
}
VKAPI_ATTR VkResult VKAPI_CALL vkResetDescriptorPool(VkDevice,VkDescriptorPool,VkDescriptorPoolResetFlags){++driver.resets;return VK_SUCCESS;}
VKAPI_ATTR VkResult VKAPI_CALL vkAllocateDescriptorSets(VkDevice,const VkDescriptorSetAllocateInfo* p,VkDescriptorSet* out) {
    for(unsigned n=0;n<p->descriptorSetCount;++n)out[n]=handle<VkDescriptorSet>(driver.next++);return VK_SUCCESS;
}
VKAPI_ATTR VkResult VKAPI_CALL vkMapMemory(VkDevice,VkDeviceMemory m,VkDeviceSize at,VkDeviceSize,VkMemoryMapFlags,void** out) {
    *out=driver.memory.at(key(m)).data()+at;return VK_SUCCESS;
}
VKAPI_ATTR void VKAPI_CALL vkUnmapMemory(VkDevice,VkDeviceMemory){}
VKAPI_ATTR void VKAPI_CALL vkDestroyDescriptorPool(VkDevice,VkDescriptorPool,const VkAllocationCallbacks*){}
VKAPI_ATTR void VKAPI_CALL vkDestroyBuffer(VkDevice,VkBuffer b,const VkAllocationCallbacks*){driver.buffers.erase(key(b));}
VKAPI_ATTR void VKAPI_CALL vkFreeMemory(VkDevice,VkDeviceMemory m,const VkAllocationCallbacks*){driver.memory.erase(key(m));}
VKAPI_ATTR void VKAPI_CALL vkUpdateDescriptorSets(VkDevice,unsigned count,const VkWriteDescriptorSet* writes,unsigned copies,const VkCopyDescriptorSet* copy) {
    ++driver.updates;driver.copies+=copies;
    for(unsigned n=0;n<copies;++n)for(unsigned j=0;j<copy[n].descriptorCount;++j)
        driver.images[key(copy[n].dstSet)][{copy[n].dstBinding,copy[n].dstArrayElement+j}]=
            driver.images.at(key(copy[n].srcSet)).at({copy[n].srcBinding,copy[n].srcArrayElement+j});
    for(unsigned n=0;n<count;++n) {
        const auto& write=writes[n];
        if(write.pImageInfo)for(unsigned j=0;j<write.descriptorCount;++j)
            driver.images[key(write.dstSet)][{write.dstBinding,write.dstArrayElement+j}]=write.pImageInfo[j];
        else if(write.pBufferInfo)driver.uniforms[{key(write.dstSet),write.dstBinding}]=*write.pBufferInfo;
    }
}
}
int main(){try{metadata();isolation();std::cout<<"per-owner frame descriptors and uploaded image identity passed\n";return 0;}
catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
