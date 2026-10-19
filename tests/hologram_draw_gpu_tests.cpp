#include "source_effect_light_gpu_fixture.hpp"
#include "halo2/source_hologram_material.hpp"
#include "halo2/source_scene_copy_gpu.hpp"
#include "halo2/menu_world_native_packet.hpp"
#include "halo2/menu_world_vertex_source.hpp"
#include "halo2/model.hpp"
#include "halo2/render_view.hpp"
#include "halo2/sampler.hpp"
#include "menu_world_vertex_catalog.generated.hpp"
#include <nlohmann/json.hpp>
#include <cmath>

// Actual original material, raw model streams and complete vertex/fragment
// programs, with declared diagnostic camera/palette/raster/background inputs.
// This checks the native copy/descriptor/draw/blend chain, not a live pose,
// group/body schedule, original Xbox framebuffer or finished cinematic.
using namespace halo2;
namespace fixture=halo2_effect_light_fixture;
namespace {
constexpr VkExtent2D extent{640,480};
void barrier(fixture::Gpu &g,VkImage image,unsigned mips,VkImageLayout old,VkImageLayout next,
    VkAccessFlags src,VkAccessFlags dst,VkPipelineStageFlags before,VkPipelineStageFlags after,
    unsigned layers=1) {
    VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};b.image=image;
    b.oldLayout=old;b.newLayout=next;b.srcAccessMask=src;b.dstAccessMask=dst;
    b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
    b.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,mips,0,layers};
    vkCmdPipelineBarrier(g.command,before,after,0,0,nullptr,0,nullptr,1,&b);
}
fixture::Image texture(fixture::Gpu &g,const Texture &t) {
    fixture::require(t.layers==1 && !t.depth,"hologram fixture requires an authored 2D image");
    const auto format=t.format==11?VK_FORMAT_R8G8B8A8_UNORM:t.format==14?VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
        t.format==15?VK_FORMAT_BC2_UNORM_BLOCK:t.format==16?VK_FORMAT_BC3_UNORM_BLOCK:VK_FORMAT_UNDEFINED;
    fixture::require(format!=VK_FORMAT_UNDEFINED,"unsupported original hologram bitmap storage");
    const auto mips=t.mips.empty()?std::vector<TextureMip>{{t.width,t.height,0,t.pixels.size()}}:t.mips;
    const auto image=g.image({t.width,t.height},format,VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT,mips.size());
    const auto upload=g.buffer(t.pixels.size(),VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    std::memcpy(upload.mapped,t.pixels.data(),t.pixels.size());
    std::vector<VkBufferImageCopy> copies;
    for(unsigned n=0;n<mips.size();++n) {
        VkBufferImageCopy c{};c.bufferOffset=mips[n].offset;c.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,n,0,1};
        c.imageExtent={mips[n].width,mips[n].height,1};copies.push_back(c);
    }
    g.begin();barrier(g,image.handle,mips.size(),VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        0,VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
    vkCmdCopyBufferToImage(g.command,upload.handle,image.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,copies.size(),copies.data());
    barrier(g,image.handle,mips.size(),VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_SHADER_READ_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);g.end();
    return image;
}
// Unused shader branches still require valid dimensional descriptor views.
fixture::Image unused_image(fixture::Gpu &g,bool cube) {
    fixture::Image image;
    VkImageCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};ci.imageType=cube?VK_IMAGE_TYPE_2D:VK_IMAGE_TYPE_3D;
    ci.flags=cube?VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT:0;ci.format=VK_FORMAT_R8G8B8A8_UNORM;
    ci.extent={1,1,1};ci.mipLevels=1;ci.arrayLayers=cube?6:1;ci.samples=VK_SAMPLE_COUNT_1_BIT;
    ci.tiling=VK_IMAGE_TILING_OPTIMAL;ci.usage=VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    fixture::check(vkCreateImage(g.device,&ci,nullptr,&image.handle));
    VkMemoryRequirements req;vkGetImageMemoryRequirements(g.device,image.handle,&req);
    VkMemoryAllocateInfo a{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};a.allocationSize=req.size;
    a.memoryTypeIndex=g.memory(req.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    fixture::check(vkAllocateMemory(g.device,&a,nullptr,&image.memory));fixture::check(vkBindImageMemory(g.device,image.handle,image.memory,0));
    VkImageViewCreateInfo v{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};v.image=image.handle;
    v.viewType=cube?VK_IMAGE_VIEW_TYPE_CUBE:VK_IMAGE_VIEW_TYPE_3D;v.format=ci.format;
    v.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,ci.arrayLayers};
    fixture::check(vkCreateImageView(g.device,&v,nullptr,&image.view));g.images.push_back(image);
    g.begin();barrier(g,image.handle,1,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,0,
        VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,ci.arrayLayers);
    const VkClearColorValue zero{};vkCmdClearColorImage(g.command,image.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,&zero,1,&v.subresourceRange);
    barrier(g,image.handle,1,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_SHADER_READ_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,ci.arrayLayers);g.end();
    return image;
}
}
int main(int argc,char **argv)try {
    fixture::require(argc==4,"Supply original opening map, shader directory and report");
    if(!std::filesystem::exists(argv[1]))return 77;
    Cache cache(argv[1]);auto authored=prepare_hologram_material(cache,cache.load_material(0xe87106fcu),{0,21,0,0});
    for(const auto &overlay:authored.material.animation.overlays)
        fixture::require(!overlay.input && !overlay.range,"hologram fixture has an unresolved object-function input");
    fixture::require(animate_forward_material(authored.material,36.305806978076362,{}),
        "original hologram overlays were not evaluated at V719's copied source clock");
    // V719's actual rigid Regret section/vertex class. Identity palette and
    // front diagnostic camera are explicit test inputs, not a cinematic pose.
    const auto geometry=std::make_shared<const MenuWorldSourceSection>(load_menu_world_source_model_section(cache,3865380080u,2));
    fixture::require(geometry->model_geometry_class()==1 && !geometry->parts().empty(),"actual rigid Regret source section changed");
    const auto model=cache.load_model_sections(3899590394u,false,0,5,std::array{2u});
    fixture::require(!model.mesh.vertices.empty(),"diagnostic section bounds missing");
    std::vector<Vec3> points(geometry->vertex_count());std::vector<bool> known(points.size());
    for(unsigned i=0;i<model.vertex_origins.size();++i) {
        const auto &origin=model.vertex_origins[i];if(origin.section!=2 || origin.part!=0)continue;
        fixture::require(origin.vertex<points.size(),"decoded diagnostic vertex identity escaped raw geometry");
        points[origin.vertex]=model.mesh.vertices[i].position;known[origin.vertex]=true;
    }
    const auto indices=geometry->part_indices(0);float largest=0;unsigned triangle=0;
    for(unsigned i=0;i+2<indices.size();i+=3) {
        if(!known[indices[i]]||!known[indices[i+1]]||!known[indices[i+2]])continue;
        const auto a=points[indices[i]],b=points[indices[i+1]],c=points[indices[i+2]];
        const float area=std::abs((b.x-a.x)*(c.z-a.z)-(c.x-a.x)*(b.z-a.z));
        if(area>largest){largest=area;triangle=i;}
    }
    fixture::require(largest>0,"rigid Regret section has no diagnostic XZ triangle");
    float minx=INFINITY,maxx=-INFINITY,minz=INFINITY,maxz=-INFINITY;
    for(unsigned n=0;n<3;++n) {
        const auto v=points[indices[triangle+n]];
        minx=std::min(minx,v.x);maxx=std::max(maxx,v.x);minz=std::min(minz,v.z);maxz=std::max(maxz,v.z);
    }
    const float scale=1.6f/std::max(maxx-minx,maxz-minz);
    MenuWorldSourceConstants constants{};
    constants[0]={scale,0,0,-scale*(minx+maxx)*.5f};
    constants[1]={0,0,scale,-scale*(minz+maxz)*.5f};
    constants[2]={0,0,0,.5f};constants[3]={0,0,0,1};
    constants[4]={0,0,1,0};constants[5]={1,0,0,.5f};constants[7]={0,0,0,765};
    const auto viewport=viewport_constants({0,0,640,480});std::copy(viewport.begin(),viewport.end(),constants.begin()+8);
    std::copy(authored.material.program_constants[1].begin(),authored.material.program_constants[1].end(),constants.begin()+18);
    const std::array<std::array<float,12>,1> palette{{{1,0,0,0,0,1,0,0,0,0,1,0}}};
    // Original4D5E7/4D5F3 chooses lists for this actual rigid unit (flags200
    // is clear), independently of the part's index count.
    auto binding=menu_world_source_binding_class(cache,{MenuWorldSourceOwnerKind::dynamic_object,3865380080u,
        3805216863u&65535u,3805216863u},menu_world_source_draw(geometry,0,MenuWorldSourceTopology::triangle_list,palette,
            MenuWorldSourceIndexRange{unsigned(geometry->parts()[0].index_first)+triangle,3}),
        authored.pass.vertex_shader(),1,constants);
    fixture::Gpu g;if(!g.initialize())return 77;
    VkPhysicalDeviceProperties properties;vkGetPhysicalDeviceProperties(g.physical,&properties);
    SourceSceneCopyGpu scene_copy({g.physical,g.device,g.family,VK_FORMAT_R8G8B8A8_UNORM,argv[2]});
    const auto copy_binding=scene_copy.prepare({{{0,0,480,640},320,240,.01f,1024.f},{640,480},extent});
    const Texture target_shape{320,240,11,{},{}},*shapes[]{&target_shape,&authored.material.detail,&authored.material.bump,&authored.material.auxiliary};
    std::array<VkImageView,4> views{copy_binding.view};
    for(unsigned s=1;s<4;++s)views[s]=texture(g,*shapes[s]).view;
    const auto cube=unused_image(g,true),volume=unused_image(g,false);
    VkDescriptorSetLayoutBinding bindings[7]{};
    for(unsigned n=0;n<7;++n)bindings[n]={n,n==4?VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,n>=5?4u:1u,VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,nullptr};
    VkDescriptorSetLayoutCreateInfo dl{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};dl.bindingCount=7;dl.pBindings=bindings;
    fixture::check(vkCreateDescriptorSetLayout(g.device,&dl,nullptr,&g.descriptor_layout));
    const VkDescriptorPoolSize sizes[]{{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,1},{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,12}};
    VkDescriptorPoolCreateInfo dp{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};dp.maxSets=1;dp.poolSizeCount=2;dp.pPoolSizes=sizes;
    fixture::check(vkCreateDescriptorPool(g.device,&dp,nullptr,&g.descriptors));
    VkDescriptorSet set;VkDescriptorSetAllocateInfo da{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};da.descriptorPool=g.descriptors;da.descriptorSetCount=1;da.pSetLayouts=&g.descriptor_layout;
    fixture::check(vkAllocateDescriptorSets(g.device,&da,&set));
    const auto &m=authored.material;
    WorldMaterialUniform uniform{};uniform.program=m.forward_program.words;uniform.vertex_attributes=m.program_constants[0];uniform.vertex_constants=m.program_constants[1];
    uniform.alpha_test={m.forward_raster.alpha_test,m.forward_raster.alpha_function,m.forward_raster.alpha_reference,0};
    std::array<VkDescriptorImageInfo,4> two{},cubes{},volumes{};
    for(unsigned s=0;s<4;++s) {
        uniform.texture_effects[s]={m.texture_states[s].alpha_kill,s==0?1u:0u,0,m.texture_states[s].key_color};
        const auto native=native_sampler(m.texture_states[s],properties.limits,true);
        fixture::require(!native.custom_border,"diagnostic hologram needs an owned custom-border sampler feature");
        VkSampler sampler;fixture::check(vkCreateSampler(g.device,&native.info,nullptr,&sampler));g.samplers.push_back(sampler);
        two[s]={sampler,views[s],VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        cubes[s]={sampler,cube.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};volumes[s]={sampler,volume.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    }
    const auto ub=g.buffer(sizeof(uniform),VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);std::memcpy(ub.mapped,&uniform,sizeof(uniform));
    const VkDescriptorBufferInfo buffer{ub.handle,0,sizeof(uniform)};
    VkWriteDescriptorSet writes[7]{};
    for(unsigned n=0;n<7;++n) {
        auto &w=writes[n];w={VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};w.dstSet=set;w.dstBinding=n;w.descriptorCount=n>=5?4:1;
        w.descriptorType=n==4?VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        if(n<4)w.pImageInfo=&two[n];else if(n==4)w.pBufferInfo=&buffer;else w.pImageInfo=n==5?cubes.data():volumes.data();
    }
    vkUpdateDescriptorSets(g.device,7,writes,0,nullptr);
    MenuWorldNativePacketInput input{binding,generated_menu_world_vertex_catalog::entries,uniform};
    input.fragment_offsets=m.forward_program.fragment_offsets;input.forward=m.forward_raster;
    input.forward.cull=0;input.forward.depth_enable=0;input.forward.depth_write=0; // Declared diagnostic raster.
    input.forward_blend=1;input.texture_states=m.texture_states;
    input.sampler_limits={properties.limits.maxSamplerLodBias,properties.limits.maxSamplerAnisotropy,true,false};
    std::copy(std::begin(shapes),std::end(shapes),input.images.begin());
    input.texture_coordinates[0]=MenuWorldTextureCoordinates::pixel;input.raster.surface_scale={1,1};
    input.raster.regions.fill({0,0,0,0});input.raster.regions[0]={0,0,640,480};
    input.clip_range={0,16777215,.01f,1024};input.fog_parameters={1,1};input.surface_size={640,480};
    input.demote_to_helper_enabled=true;input.material_set=set;
    std::vector<MenuWorldNativeDrawInput> packets;packets.push_back(menu_world_native_packet(input));
    // The original pass masks alpha writes. The opaque diagnostic enables
    // them to observe the shader's As independently of its compositing.
    input.forward.color_write=0x01010101u;
    input.forward_blend=0;packets.push_back(menu_world_native_packet(input));
    input.forward_blend=1;packets.push_back(menu_world_native_packet(input));
    const auto color=g.image(extent,VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    const auto depth=g.image(extent,VK_FORMAT_D24_UNORM_S8_UINT,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    const auto clear=g.scene_pass(true,VK_FORMAT_D24_UNORM_S8_UINT),load=g.scene_pass(false,VK_FORMAT_D24_UNORM_S8_UINT);
    const auto fb=g.framebuffer(clear,extent,color,depth);const auto rb=g.buffer(640*480*4,VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    VkFence fence;VkFenceCreateInfo fc{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};fixture::check(vkCreateFence(g.device,&fc,nullptr,&fence));
    std::array<std::vector<unsigned char>,3> results;
    {
        MenuWorldNativeDraw renderer({g.device,g.physical,load,g.descriptor_layout,VK_API_VERSION_1_3,true,true,true,true,argv[2]});renderer.prepare(packets);
        for(unsigned n=0;n<3;++n) {
            if(n){fixture::check(vkResetFences(g.device,1,&fence));scene_copy.prepare({{{0,0,480,640},320,240,.01f,1024.f},{640,480},extent});}
            g.begin();VkClearValue values[2]{};values[0].color={{29.f/255,71.f/255,113.f/255,197.f/255}};values[1].depthStencil={1,0};
            VkRenderPassBeginInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};pass.renderPass=clear;pass.framebuffer=fb;pass.renderArea.extent=extent;pass.clearValueCount=2;pass.pClearValues=values;
            vkCmdBeginRenderPass(g.command,&pass,VK_SUBPASS_CONTENTS_INLINE);vkCmdEndRenderPass(g.command);
            const auto produced=scene_copy.record({g.device,g.command,color.handle,VK_FORMAT_R8G8B8A8_UNORM,extent,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_SAMPLE_COUNT_1_BIT,1,1,g.family,fence,true,true});
            fixture::require(produced.view==copy_binding.view,"copy replaced the descriptor's prepared view");
            pass.renderPass=load;pass.clearValueCount=0;pass.pClearValues=nullptr;vkCmdBeginRenderPass(g.command,&pass,VK_SUBPASS_CONTENTS_INLINE);
            const VkViewport vp{0,0,640,480,0,1};const VkRect2D rect{{0,0},extent};
            vkCmdSetViewport(g.command,0,1,&vp);vkCmdSetScissor(g.command,0,1,&rect);renderer.record(g.command,n);vkCmdEndRenderPass(g.command);
            g.readback(color.handle,extent,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,rb);
            fixture::check(vkEndCommandBuffer(g.command));VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&g.command;
            fixture::check(vkQueueSubmit(g.queue,1,&submit,fence));fixture::check(vkWaitForFences(g.device,1,&fence,VK_TRUE,5000000000ull));
            fixture::require(scene_copy.retire(),"actual copy/draw fence did not retire");
            const auto *bytes=static_cast<const unsigned char*>(rb.mapped);results[n].assign(bytes,bytes+640*480*4);
        }
    }
    vkDestroyFence(g.device,fence,nullptr);
    unsigned covered=0,partial_alpha=0,channels=0;float maximum=0;
    constexpr unsigned background[]{29,71,113,197};
    for(unsigned p=0;p<640*480;++p) {
        const auto at=p*4;bool changed=false;
        for(unsigned c=0;c<4;++c)changed|=results[1][at+c]!=background[c];
        if(!changed)continue;++covered;const float alpha=results[1][at+3]/255.f;
        partial_alpha+=alpha>.05f && alpha<.95f;
        for(unsigned c=0;c<4;++c) {
            constexpr unsigned mask[]{16,8,0,24};
            const float expected=results[1][at+c]*alpha+background[c]*(1-alpha);
            const float authored_expected=((m.forward_raster.color_write>>mask[c])&1u)?expected:float(background[c]);
            maximum=std::max(maximum,std::abs(float(results[0][at+c])-authored_expected));
            maximum=std::max(maximum,std::abs(float(results[2][at+c])-expected));channels+=2;
        }
    }
    fixture::require(covered>100 && partial_alpha>100,"raw original hologram produced no useful translucent coverage");
    fixture::require(maximum<=2,"native raw source-alpha blend differs from source RGBA equation");
    fixture::require(fixture::validation_errors==0,"raw hologram chain has Vulkan validation errors");
    const nlohmann::json report{{"passed",true},{"device",properties.deviceName},{"covered_pixels",covered},
        {"partial_alpha_pixels",partial_alpha},{"checked_channels",channels},{"maximum_byte_error",maximum},
        {"validation_errors",fixture::validation_errors},{"target19_prebound_then_produced",true},
        {"scope","Actual original rigid Regret resource/program/material through native scene-copy and raw draw; explicit diagnostic camera/palette/raster/background, not live group/body ordering or Xbox GPU pixels"},
        {"large_artifacts_retained",nlohmann::json::array()}};
    const std::filesystem::path output=argv[3];std::filesystem::create_directories(output.parent_path());std::ofstream(output)<<report.dump(2)<<'\n';
    std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
