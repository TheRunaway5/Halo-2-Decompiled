#include "source_shadow_gpu_fixture.hpp"
#include "halo2/source_scene_copy_gpu.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <cmath>
using namespace halo2;
namespace {
namespace fixture=halo2_shadow_fixture;
void transition(VkCommandBuffer c,VkImage i,VkImageLayout old,VkImageLayout next,VkAccessFlags src,VkAccessFlags dst,VkPipelineStageFlags first,VkPipelineStageFlags last) {
    VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};b.image=i;b.oldLayout=old;b.newLayout=next;
    b.srcAccessMask=src;b.dstAccessMask=dst;b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
    b.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};vkCmdPipelineBarrier(c,first,last,0,0,nullptr,0,nullptr,1,&b);
}
void readback(VkCommandBuffer c,const SourceSceneCopyGpuImage &image,VkBuffer output) {
    transition(c,image.image,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT|VK_ACCESS_SHADER_READ_BIT,VK_ACCESS_TRANSFER_READ_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
    VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={image.extent.width,image.extent.height,1};
    vkCmdCopyImageToBuffer(c,image.image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,output,1,&copy);
    transition(c,image.image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_ACCESS_TRANSFER_READ_BIT,VK_ACCESS_SHADER_READ_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
}
float linear(std::span<const std::byte> pixels,VkExtent2D extent,float u,float v,unsigned channel) {
    const float x=u*extent.width-.5f,y=v*extent.height-.5f;const int ix=int(std::floor(x)),iy=int(std::floor(y));const float dx=x-ix,dy=y-iy;
    const auto texel=[&](int a,int b){a=std::clamp(a,0,int(extent.width)-1);b=std::clamp(b,0,int(extent.height)-1);return float(std::to_integer<unsigned>(pixels[(std::size_t(b)*extent.width+a)*4+channel]));};
    return (texel(ix,iy)*(1-dx)+texel(ix+1,iy)*dx)*(1-dy)+(texel(ix,iy+1)*(1-dx)+texel(ix+1,iy+1)*dx)*dy;
}
}
int main(int argc,char **argv)try {
    fixture::require(argc==4,"Supply original copy fixture, shader directory and report path");
    std::ifstream input(argv[1]);const auto original=nlohmann::json::parse(input);
    fixture::require(original.at("schema")=="original-scene-copy-destinations-v1"&&original.at("instruction_patches").empty()&&original.at("supplied_returns").empty(),"Invalid original copy fixture");
    fixture::Gpu gpu;if(!gpu.initialize())return 77;
    VkPhysicalDeviceProperties device;vkGetPhysicalDeviceProperties(gpu.physical,&device);
    nlohmann::json cases=nlohmann::json::array();std::size_t channels=0;float maximum=0;
    for(const auto format:{VK_FORMAT_R8G8B8A8_UNORM,VK_FORMAT_B8G8R8A8_UNORM}) {
        SourceSceneCopyGpu copy({gpu.physical,gpu.device,gpu.family,format,argv[2]});
        for(const VkExtent2D extent:{VkExtent2D{640,480},VkExtent2D{1280,720}})for(const unsigned row_index:{0u,13u}) {
            const auto &row=original.at("versions").at("retail").at("results").at(row_index);
            const auto dimensions=row.at("destination_dimensions").get<std::array<int,2>>();
            const VkExtent2D destination{unsigned(dimensions[0]),unsigned(dimensions[1])};
            const auto source=gpu.image(extent,format,VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
            const auto upload=gpu.buffer(std::size_t(extent.width)*extent.height*4,VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
            const auto first=gpu.buffer(std::size_t(destination.width)*destination.height*4,VK_BUFFER_USAGE_TRANSFER_DST_BIT);
            const auto second=gpu.buffer(first.size,VK_BUFFER_USAGE_TRANSFER_DST_BIT);
            const auto bytes=std::span(static_cast<std::byte*>(upload.mapped),std::size_t(upload.size));
            for(unsigned y=0;y<extent.height;++y)for(unsigned x=0;x<extent.width;++x)for(unsigned c=0;c<4;++c)
                bytes[(std::size_t(y)*extent.width+x)*4+c]=std::byte((x*(37+11*c)+y*(19+7*c)+(x^y)*13)%256);
            gpu.begin();transition(gpu.command,source.handle,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,0,VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
            VkBufferImageCopy upload_copy{};upload_copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};upload_copy.imageExtent={extent.width,extent.height,1};
            vkCmdCopyBufferToImage(gpu.command,upload.handle,source.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&upload_copy);
            transition(gpu.command,source.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);gpu.end();
            const auto bits=[](const auto &v){return std::bit_cast<float>(v.template get<std::uint32_t>());};
            SourceSceneCopyGpuInput plan{{{0,0,480,640},dimensions[0],dimensions[1],bits(row.at("near_bits")),bits(row.at("far_bits"))},{640,480},extent};
            const auto binding=copy.prepare(plan);
            fixture::require(binding.view && binding.format==format && binding.extent.width==destination.width &&
                binding.extent.height==destination.height,"Prepared descriptor lacks its actual destination facts");
            VkFence fence;VkFenceCreateInfo fc{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};fixture::check(vkCreateFence(gpu.device,&fc,nullptr,&fence));
            SourceSceneCopyGpuRecord call{gpu.device,gpu.command,source.handle,format,extent,
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,VK_SAMPLE_COUNT_1_BIT,1,1,gpu.family,fence,true,true};
            gpu.begin();const auto initial=copy.record(call);const bool pending=!copy.retire();bool pending_prepare_rejected=false,other_fence_rejected=false;
            fixture::require(binding.view==initial.view && binding.format==initial.format &&
                binding.extent.width==initial.extent.width && binding.extent.height==initial.extent.height,
                "Recorded scene copy replaced its prebound descriptor image");
            try{copy.prepare(plan);}catch(const FormatError&){pending_prepare_rejected=true;}
            auto invalid=call;invalid.completion=VK_NULL_HANDLE;try{copy.record(invalid);}catch(const FormatError&){other_fence_rejected=true;}
            readback(gpu.command,initial,first.handle);
            // A later group's body changes the actual main image. The first
            // copied result must retain the earlier image, and the second
            // ordered copy must observe the new one, in the same command.
            transition(gpu.command,source.handle,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
            const VkClearColorValue color{{17.f/255.f,53.f/255.f,101.f/255.f,197.f/255.f}};const VkImageSubresourceRange range{VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
            vkCmdClearColorImage(gpu.command,source.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,&color,1,&range);
            transition(gpu.command,source.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
            const auto next=copy.record(call);readback(gpu.command,next,second.handle);
            fixture::check(vkEndCommandBuffer(gpu.command));VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&gpu.command;
            fixture::check(vkQueueSubmit(gpu.queue,1,&submit,fence));fixture::check(vkWaitForFences(gpu.device,1,&fence,VK_TRUE,5000000000ull));
            fixture::require(copy.retire()&&pending&&pending_prepare_rejected&&other_fence_rejected,"Copy frame ownership/fence gates failed");
            fixture::require(initial.image==next.image&&initial.view==next.view&&initial.extent.width==destination.width&&initial.extent.height==destination.height,"Copy did not reuse its actual destination");
            const auto a=static_cast<const std::byte*>(first.mapped),b=static_cast<const std::byte*>(second.mapped);
            const auto clear=format==VK_FORMAT_B8G8R8A8_UNORM?std::array{101,53,17,197}:std::array{17,53,101,197};
            float case_max=0;
            for(unsigned y=0;y<destination.height;++y)for(unsigned x=0;x<destination.width;++x)for(unsigned c=0;c<4;++c) {
                float expected=0;
                for(unsigned tap=0;tap<4;++tap) {
                    const auto &v=row.at("vertices").at(0).at("attributes").at(tap+1).at("value_bits");
                    const float u=(float(x)+.5f)/destination.width+bits(v.at(0))/640.f;
                    const float t=(float(y)+.5f)/destination.height+bits(v.at(1))/480.f;
                    expected+=linear(bytes,extent,u,t,c)*.25f;
                }
                const auto i=(std::size_t(y)*destination.width+x)*4+c;
                const float error=std::abs(float(std::to_integer<unsigned>(a[i]))-expected);case_max=std::max(case_max,error);maximum=std::max(maximum,error);
                fixture::require(error<=2.f,"GPU destination copy differs from original attributes");
                fixture::require(std::abs(int(std::to_integer<unsigned>(b[i]))-clear[c])<=1,"Ordered second copy did not observe changed main image");channels+=2;
            }
            // Reset a genuinely never-submitted command and recover the
            // pre-record layouts; no completion is claimed for discarded work.
            fixture::check(vkResetFences(gpu.device,1,&fence));gpu.begin();copy.record(call);
            fixture::check(vkResetCommandBuffer(gpu.command,0));copy.discard_unsubmitted(gpu.command);
            fixture::require(copy.retire(),"Discarded command remained pending");
            // A new completed copy after discard must still use valid layouts.
            gpu.begin();copy.record(call);fixture::check(vkEndCommandBuffer(gpu.command));
            fixture::check(vkQueueSubmit(gpu.queue,1,&submit,fence));fixture::check(vkWaitForFences(gpu.device,1,&fence,VK_TRUE,5000000000ull));fixture::require(copy.retire(),"Reused copy fence did not retire");
            vkDestroyFence(gpu.device,fence,nullptr);
            cases.push_back({{"format",format},{"source",{extent.width,extent.height}},{"destination",dimensions},{"maximum_byte_error",case_max},{"ordered_copies",2},{"discard_reuse",true}});
        }
    }
    fixture::require(fixture::validation_errors==0,"Scene-copy native resources produced Vulkan validation errors");
    const std::filesystem::path output=argv[3];std::filesystem::create_directories(output.parent_path());
    const nlohmann::json report{{"passed",true},{"device",{{"name",device.deviceName},{"type",device.deviceType},{"vendor_id",device.vendorID},{"device_id",device.deviceID},{"driver_version",device.driverVersion}}},
        {"cases",cases},{"checked_channels",channels},{"maximum_byte_error",maximum},{"validation_errors",fixture::validation_errors},
        {"scope","Actual persistent native copy images/pipeline, ordered command snapshots, fences, layout restoration and discarded-command reuse; declared host images; not live hologram scheduling or Xbox GPU pixels"},{"large_artifacts_retained",nlohmann::json::array()}};
    std::ofstream file(output);file<<report.dump(2)<<'\n';fixture::require(bool(file),"Cannot retain native copy report");
    std::cout<<cases.size()<<" native copy resource cases, "<<channels<<" channel checks; max error "<<maximum<<"; no Vulkan validation errors\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
