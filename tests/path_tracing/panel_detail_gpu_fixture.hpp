#pragma once
#include "halo2/path_tracing/material_surface.hpp"
#include "halo2/source_texture_filtering.hpp"
#include "halo2/texture.hpp"
#include <nlohmann/json.hpp>

namespace {
// Own exact decoded source mips rather than resampling them into the generic
// fixture's64x64 images. Destruction follows the enclosing GPU fixture, and
// only the small numeric report leaves the test runner's scratch directory.
struct PanelSourceTexture {
    Fixture* gpu{};
    Image image;
    Buffer upload;
    VkSampler sampler{};
    unsigned mip_level{},width{},height{};
    ~PanelSourceTexture() {
        if(!gpu)return;
        if(sampler)vkDestroySampler(gpu->device,sampler,nullptr);
        if(image.view)vkDestroyImageView(gpu->device,image.view,nullptr);
        if(image.handle)vkDestroyImage(gpu->device,image.handle,nullptr);
        if(image.memory)vkFreeMemory(gpu->device,image.memory,nullptr);
        if(upload.mapped)vkUnmapMemory(gpu->device,upload.memory);
        if(upload.handle)vkDestroyBuffer(gpu->device,upload.handle,nullptr);
        if(upload.memory)vkFreeMemory(gpu->device,upload.memory,nullptr);
    }
    void bind(Fixture& owner,const Material& material,unsigned stage) {
        gpu=&owner;
        const Texture* sources[]{&material.diffuse,&material.detail,&material.bump};
        auto source=material.animated_textures[stage].empty()?*sources[stage]:
            material.animated_textures[stage].at(material.texture_frames[stage]);
        if(source.format>=14 && source.format<=16)source=prepare_xemu_bc_texture(source);
        require(source.format==11 && source.width && source.height && source.layers==1 && !source.depth,
            "Actual panel source requires a decoded2D RGBA8 image");
        if(source.mips.empty())source.mips.push_back({source.width,source.height,0,std::size_t(source.width)*source.height*4});
        const auto& state=material.texture_states[stage];
        mip_level=std::min(state.max_mip,unsigned(source.mips.size()-1));
        while(mip_level+1<source.mips.size() && (source.mips[mip_level].width>128 || source.mips[mip_level].height>128))++mip_level;
        const auto& mip=source.mips[mip_level];width=mip.width;height=mip.height;
        const auto bytes=std::size_t(width)*height*4;
        require(mip.offset<=source.pixels.size() && bytes<=source.pixels.size()-mip.offset,
            "Actual panel source mip exceeds its owned image");
        gpu->image(image,VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT,width,height);
        gpu->buffer(upload,bytes,VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
        std::memcpy(upload.mapped,source.pixels.data()+mip.offset,bytes);
        const auto address=[](unsigned mode) {
            require(mode>=1 && mode<=3,"Panel source sampler left its repeat/mirror/clamp cohort");
            return mode==1?VK_SAMPLER_ADDRESS_MODE_REPEAT:mode==2?VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT:
                VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        };
        const auto filtering=source_texture_filtering(state);
        // Match the bounded source atlas's selected authored mip and its
        // source magnification filter; there is no replacement image palette.
        VkSamplerCreateInfo info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        info.magFilter=info.minFilter=filtering.mag_filter==1?VK_FILTER_NEAREST:VK_FILTER_LINEAR;
        info.mipmapMode=VK_SAMPLER_MIPMAP_MODE_NEAREST;
        info.addressModeU=address(state.address_u);info.addressModeV=address(state.address_v);
        info.addressModeW=VK_SAMPLER_ADDRESS_MODE_REPEAT;
        check(vkCreateSampler(gpu->device,&info,nullptr,&sampler),"create actual panel source sampler");
        gpu->begin();VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.image=image.handle;barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
        barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
        barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;barrier.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(gpu->command,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,0,nullptr,0,nullptr,1,&barrier);
        const VkBufferImageCopy copy{0,0,0,{VK_IMAGE_ASPECT_COLOR_BIT,0,0,1},{0,0,0},{width,height,1}};
        vkCmdCopyBufferToImage(gpu->command,upload.handle,image.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy);
        barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(gpu->command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,0,nullptr,0,nullptr,1,&barrier);gpu->submit();
        const VkDescriptorImageInfo sampled{sampler,image.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};write.dstSet=gpu->material;
        write.dstBinding=stage;write.descriptorCount=1;write.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo=&sampled;vkUpdateDescriptorSets(gpu->device,1,&write,0,nullptr);
    }
};
}

int panel_detail_gpu(const std::filesystem::path& shaders,const std::filesystem::path& map,
                     const std::filesystem::path& report_path) {
    if(!std::filesystem::is_regular_file(map))return 77;
    Cache cache(map);const auto mesh=cache.load_bsp(0);
    Fixture gpu;gpu.initialize(shaders,{width,height},true);gpu.fence_submissions=true;
    gpu.renderer->set_quality(1,1);
    using Json=nlohmann::json;
    Json report{{"map",map.string()},{"materials",Json::array()},{"samples",1},{"bounces",1},{"upscaler","Off"},
        {"scope","Actual frozen_panels/main_cell_exterior native programs, ordered UVs, decoded authored mips and samplers; diffuse color isolated under constant sky. Raw, source-depth and accumulated full-resolution resolve compared with the separately scalar-qualified CPU atlas; no bump/specular or full native-frame parity claim."}};
    unsigned found{};bool passed=true;
    for(const auto& material:mesh.materials) {
        const bool blend=material.name.ends_with("\\main_cell_exterior");
        if(!blend && !material.name.ends_with("\\frozen_panels"))continue;
        ++found;const auto source=pt::qualified_surface(material);
        require(source && source->family==(blend?pt::SurfaceFamily::TerrainBlendThreeStage:pt::SurfaceFamily::OpaqueDetail),
            "Actual panel GPU regression lost its native program/vertex qualification");
        std::array<PanelSourceTexture,3> images;
        Json bindings=Json::array();
        for(unsigned stage=0;stage<(blend?3u:2u);++stage) {
            images[stage].bind(gpu,material,stage);
            bindings.push_back({{"stage",stage},{"mip",images[stage].mip_level},
                {"width",images[stage].width},{"height",images[stage].height},
                {"address_u",material.texture_states[stage].address_u},{"address_v",material.texture_states[stage].address_v}});
        }
        native_uniform(gpu,material);
        std::vector<Vertex> primary;quad(primary,-1,-1,1,1,.5f,{0,0,-1},0);
        auto scene=pt::build_scene(primary,std::span(&material,1));
        scene.materials[0].response[1]=scene.materials[0].response[2]=0;
        scene.materials[0].color_guide={};scene.materials[0].emission={};
        // The CPU source test proves these full native expressions separately.
        // This test adds actual descriptor sampling and every GPU render role.
        std::vector<double> expected(width*height*3);double detail_signal{};unsigned samples{};
        auto neutral=material;neutral.animated_textures[1].clear();neutral.animated_textures[2].clear();
        neutral.detail=neutral.bump=solid_texture({128,128,128,255});
        const auto neutral_scene=pt::build_scene(primary,std::span(&neutral,1));
        for(unsigned y=4;y<height-4;++y)for(unsigned x=4;x<width-4;++x) {
            const Vec2 uv{(float(x)+.5f)/width,(float(y)+.5f)/height};
            const auto color=pt::material_surface(scene,0,uv),control=pt::material_surface(neutral_scene,0,uv);
            require(color.has_value() && control.has_value(),"Actual panel source rejected an opaque oracle pixel");
            for(unsigned c=0;c<3;++c) {
                expected[(y*width+x)*3+c]=(*color)[c]*63.75;
                const double difference=((*color)[c]-(*control)[c])*63.75;
                detail_signal+=difference*difference;++samples;
            }
        }
        detail_signal=std::sqrt(detail_signal/samples);
        Json modes=Json::array();
        for(unsigned mode=0;mode<3;++mode) {
            if(mode==2)for(unsigned frame=0;frame<15;++frame)
                gpu.draw(scene,primary,{.25f,.25f,.25f},Camera{},~0u,false,true,nullptr,true,false,false);
            const auto frame=gpu.draw(scene,primary,{.25f,.25f,.25f},Camera{},~0u,mode==1,mode==2,nullptr,true,mode==2);
            double squared{},maximum{};
            for(unsigned y=4;y<height-4;++y)for(unsigned x=4;x<width-4;++x)for(unsigned c=0;c<3;++c) {
                const auto difference=std::abs(frame[(y*width+x)*4+c]-expected[(y*width+x)*3+c]);
                squared+=difference*difference;maximum=std::max(maximum,difference);
            }
            const auto rms=std::sqrt(squared/samples);passed&=maximum<2.0 && rms<.7;
            modes.push_back({{"mode",mode==0?"raw":mode==1?"source_depth":"filtered16"},
                {"maximum_byte_error",maximum},{"rms_byte_error",rms}});
        }
        const auto history=gpu.renderer->temporal_stats();
        passed&=detail_signal>1.0 && history.banks[0].mean_count>15.5;
        report["materials"].push_back({{"name",material.name},{"family",unsigned(source->family)},
            {"transforms",material.forward_transforms},{"bindings",bindings},{"modes",modes},
            {"neutral_detail_rms_difference",detail_signal},{"history_mean",history.banks[0].mean_count}});
    }
    passed&=found==2 && gpu.validation_errors.load()==0;
    report["passed"]=passed;report["validation_errors"]=gpu.validation_errors.load();
    std::ofstream output(report_path);require(bool(output),"Cannot write actual panel GPU report");output<<report.dump(2)<<'\n';
    std::cout<<"Path Tracing actual panel detail: "<<report.dump()<<std::endl;
    require(passed,"Actual panel GPU source detail/UVs changed or full-resolution denoising blurred authored color");
    return 0;
}
