#pragma once
#include "source_weather_plate_gpu_fixture.hpp"
#include <fstream>
namespace halo2_weather_plate_typed_fixture {
using namespace halo2_weather_plate_fixture;
std::vector<std::byte> read(const std::filesystem::path &p) {
    std::ifstream f(p,std::ios::binary|std::ios::ate);require(bool(f)&&f.tellg()>0,"source scene artifact missing");
    std::vector<std::byte> b(std::size_t(f.tellg()));f.seekg(0);require(bool(f.read(reinterpret_cast<char*>(b.data()),b.size())),"short source scene artifact");return b;
}
template<class T>std::vector<T> decode(const std::vector<std::byte>&b) {
    require(b.size()%sizeof(T)==0,"source scene artifact alignment");std::vector<T> v(b.size()/sizeof(T));std::memcpy(v.data(),b.data(),b.size());return v;
}
void save(const std::filesystem::path &p,std::span<const std::byte> b) {
    std::ofstream f(p,std::ios::binary);require(bool(f.write(reinterpret_cast<const char*>(b.data()),b.size())),"scene readback write failed");
}
void barrier(Gpu &g,Image image,VkImageAspectFlags aspect,VkImageLayout from,VkImageLayout to,
             VkAccessFlags src,VkAccessFlags dst,VkPipelineStageFlags before,VkPipelineStageFlags after) {
    VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};b.image=image.handle;b.subresourceRange={aspect,0,image.mip_levels,0,1};
    b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;b.oldLayout=from;b.newLayout=to;b.srcAccessMask=src;b.dstAccessMask=dst;
    vkCmdPipelineBarrier(g.command,before,after,0,0,nullptr,0,nullptr,1,&b);
}
void upload_color(Gpu &g,Image image,std::span<const std::byte> raw,VkImageLayout from) {
    auto b=g.buffer(raw.size(),VK_BUFFER_USAGE_TRANSFER_SRC_BIT);std::memcpy(b.mapped,raw.data(),raw.size());
    g.begin();barrier(g,image,VK_IMAGE_ASPECT_COLOR_BIT,from,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        from==VK_IMAGE_LAYOUT_UNDEFINED?0:VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_ACCESS_TRANSFER_WRITE_BIT,
        from==VK_IMAGE_LAYOUT_UNDEFINED?VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT:VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
    VkBufferImageCopy r{};r.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};r.imageExtent={640,480,1};
    vkCmdCopyBufferToImage(g.command,b.handle,image.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&r);
    barrier(g,image,VK_IMAGE_ASPECT_COLOR_BIT,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);g.end();
}
void upload_depth(Gpu &g,Image image,std::span<const std::byte> low24,std::span<const std::byte> source) {
    require(low24.size()==640*480*4&&source.size()==low24.size(),"actual source D24 surface dimensions");
    auto b=g.buffer(low24.size()+640*480,VK_BUFFER_USAGE_TRANSFER_SRC_BIT);std::memcpy(b.mapped,low24.data(),low24.size());
    auto *stencil=static_cast<std::byte*>(b.mapped)+low24.size();
    for(unsigned i=0;i<640*480;++i){
        std::uint32_t original,native;std::memcpy(&original,source.data()+i*4,4);std::memcpy(&native,low24.data()+i*4,4);
        require(native==original>>8,"D24 upload did not preserve the source integer");stencil[i]=std::byte(original&255);
    }
    g.begin();barrier(g,image,VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        0,VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
    VkBufferImageCopy r[2]{};r[0].imageSubresource={VK_IMAGE_ASPECT_DEPTH_BIT,0,0,1};r[1].bufferOffset=low24.size();r[1].imageSubresource={VK_IMAGE_ASPECT_STENCIL_BIT,0,0,1};
    for(auto &copy:r)copy.imageExtent={640,480,1};
    vkCmdCopyBufferToImage(g.command,b.handle,image.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,2,r);
    barrier(g,image,VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);g.end();
}
void verify_depth(Gpu &g,Image image,std::span<const std::byte> low24,std::span<const std::byte> source) {
    auto out=g.buffer(low24.size()+640*480,VK_BUFFER_USAGE_TRANSFER_DST_BIT);g.begin();
    barrier(g,image,VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,VK_ACCESS_TRANSFER_READ_BIT,
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
    VkBufferImageCopy r[2]{};r[0].imageSubresource={VK_IMAGE_ASPECT_DEPTH_BIT,0,0,1};r[1].bufferOffset=low24.size();r[1].imageSubresource={VK_IMAGE_ASPECT_STENCIL_BIT,0,0,1};
    for(auto &copy:r)copy.imageExtent={640,480,1};
    vkCmdCopyImageToBuffer(g.command,image.handle,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,out.handle,2,r);
    barrier(g,image,VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_ACCESS_TRANSFER_READ_BIT,VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);g.end();
    const auto *raw=static_cast<std::byte*>(out.mapped);
    for(unsigned i=0;i<640*480;++i){std::uint32_t original,native;std::memcpy(&original,source.data()+i*4,4);std::memcpy(&native,raw+i*4,4);
        require((native&0xffffff)==original>>8,"native D24 upload/readback changed an original depth integer");require(raw[low24.size()+i]==std::byte(original&255),"native stencil upload/readback changed an original byte");}
}
void upload_texture(Gpu &g,Image image,const std::filesystem::path &inputs,unsigned stage,unsigned size) {
    std::vector<std::byte> raw;std::vector<VkBufferImageCopy> regions;
    for(unsigned i=0;i<image.mip_levels;++i){const auto level=read(inputs/("stage-"+std::to_string(stage)+"-mip-"+std::to_string(i)+".rgba"));const unsigned n=std::max(1u,size>>i);const unsigned h=stage? n:480u;
        require(level.size()==std::size_t(n)*h*4,"actual plate mip byte count changed");VkBufferImageCopy r{};r.bufferOffset=raw.size();r.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,i,0,1};r.imageExtent={n,h,1};regions.push_back(r);raw.insert(raw.end(),level.begin(),level.end());}
    auto b=g.buffer(raw.size(),VK_BUFFER_USAGE_TRANSFER_SRC_BIT);std::memcpy(b.mapped,raw.data(),raw.size());g.begin();
    barrier(g,image,VK_IMAGE_ASPECT_COLOR_BIT,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,0,VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
    vkCmdCopyBufferToImage(g.command,b.handle,image.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,regions.size(),regions.data());
    barrier(g,image,VK_IMAGE_ASPECT_COLOR_BIT,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_SHADER_READ_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);g.end();
}
}
