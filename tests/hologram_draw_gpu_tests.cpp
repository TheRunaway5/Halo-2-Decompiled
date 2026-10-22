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
#include <chrono>
#include <set>

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
    const bool triangle_only=argc==5&&std::string_view(argv[4])=="--triangle-carrier";
    const bool depth_differential=argc==5&&std::string_view(argv[4])=="--depth-differential";
    fixture::require(argc==4||triangle_only||depth_differential,"Supply original opening map, shader directory, report and optional --triangle-carrier or --depth-differential");
    const auto started=std::chrono::steady_clock::now();
    const auto checkpoint=[&](const std::string &phase,unsigned completed=0) {
        if(!depth_differential)return;
        const std::filesystem::path output=argv[3];std::filesystem::create_directories(output.parent_path());
        const nlohmann::json progress{{"passed",false},{"status","in progress"},{"phase",phase},
            {"completed_cases",completed},{"elapsed_seconds",std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count()}};
        std::ofstream(output)<<progress.dump(2)<<'\n';
        std::cout<<progress.dump()<<std::endl;
    };
    checkpoint("loading_source");
    if(!std::filesystem::exists(argv[1])){fixture::require(!triangle_only&&!depth_differential,"Required original opening map is absent");return 77;}
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
    fixture::Gpu g;if(!g.initialize(triangle_only)){fixture::require(!triangle_only,"Required production triangle draw features are absent");return 77;}
    VkPhysicalDeviceProperties properties;vkGetPhysicalDeviceProperties(g.physical,&properties);
    checkpoint("device_ready");
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
    const auto depth_format=triangle_only?VK_FORMAT_D32_SFLOAT_S8_UINT:VK_FORMAT_D24_UNORM_S8_UINT;
    const auto depth=g.image(extent,depth_format,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    const auto clear=g.scene_pass(true,depth_format),load=g.scene_pass(false,depth_format);
    const auto fb=g.framebuffer(clear,extent,color,depth);const auto rb=g.buffer(640*480*4,VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    VkFence fence;VkFenceCreateInfo fc{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};fixture::check(vkCreateFence(g.device,&fc,nullptr,&fence));
    std::array<std::vector<unsigned char>,3> results;
    bool submitted=false;
    for(bool triangles:{false,true}) {
        if(triangle_only&&!triangles)continue;
        MenuWorldNativeDrawContext context{g.device,g.physical,load,g.descriptor_layout,VK_API_VERSION_1_3,g.geometry_enabled,g.geometry_enabled,true,true,argv[2]};
        context.triangle_vertices=triangles;
        context.depth_encoding=triangle_only?SourceDepthStorageEncoding::d32_integer_d24:SourceDepthStorageEncoding::native_d24;
        context.strict_shader_math_enabled=g.strict_shader_math;
        MenuWorldNativeDraw renderer(context);renderer.prepare(packets);
        for(unsigned n=0;n<3;++n) {
            if(submitted){fixture::check(vkResetFences(g.device,1,&fence));scene_copy.prepare({{{0,0,480,640},320,240,.01f,1024.f},{640,480},extent});}
            g.begin();VkClearValue values[2]{};values[0].color={{29.f/255,71.f/255,113.f/255,197.f/255}};values[1].depthStencil={1,0};
            if(triangle_only)values[1].depthStencil.depth=std::bit_cast<float>(source_d24_carrier_word(source_d24_maximum));
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
            submitted=true;
            fixture::require(scene_copy.retire(),"actual copy/draw fence did not retire");
            const auto *bytes=static_cast<const unsigned char*>(rb.mapped);
            if(triangles&&!triangle_only)fixture::require(std::equal(results[n].begin(),results[n].end(),bytes),"geometry-free native draw changed hologram color bytes");
            else results[n].assign(bytes,bytes+640*480*4);
        }
    }
    checkpoint("color_controls_complete");
    unsigned depth_cases=0,depth_coverage_pixels=0;
    if(triangle_only) {
        // The explicit diagnostic c3 row above makes every source W exactly 1.
        // Use the production ALWAYS/write draw to expose coverage, then check
        // integer comparisons and both write policies against that coverage.
        // This is a depth-state regression, not an original coverage oracle.
        MenuWorldNativeDrawContext context{g.device,g.physical,load,g.descriptor_layout,VK_API_VERSION_1_3,false,false,true,true,argv[2]};
        context.triangle_vertices=true;context.depth_encoding=SourceDepthStorageEncoding::d32_integer_d24;
        context.strict_shader_math_enabled=g.strict_shader_math;
        MenuWorldNativeDraw renderer(context);
        const auto depth_read=g.buffer(640*480*4,VK_BUFFER_USAGE_TRANSFER_DST_BIT);
        const auto stencil_read=g.buffer(640*480,VK_BUFFER_USAGE_TRANSFER_DST_BIT);
        const auto draw_depth=[&](unsigned comparison,bool write,unsigned clear_depth) {
            input.forward.depth_enable=2;input.forward.depth_write=write;input.forward.depth_function=comparison;
            input.forward_blend=0;
            const auto packet=menu_world_native_packet(input);renderer.prepare(std::span(&packet,1));
            fixture::check(vkResetFences(g.device,1,&fence));
            scene_copy.prepare({{{0,0,480,640},320,240,.01f,1024.f},{640,480},extent});
            g.begin();VkClearValue values[2]{};values[0].color={{29.f/255,71.f/255,113.f/255,197.f/255}};
            values[1].depthStencil={std::bit_cast<float>(source_d24_carrier_word(clear_depth)),0xa7};
            VkRenderPassBeginInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};pass.renderPass=clear;pass.framebuffer=fb;
            pass.renderArea.extent=extent;pass.clearValueCount=2;pass.pClearValues=values;
            vkCmdBeginRenderPass(g.command,&pass,VK_SUBPASS_CONTENTS_INLINE);vkCmdEndRenderPass(g.command);
            scene_copy.record({g.device,g.command,color.handle,VK_FORMAT_R8G8B8A8_UNORM,extent,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_SAMPLE_COUNT_1_BIT,1,1,g.family,fence,true,true});
            pass.renderPass=load;pass.clearValueCount=0;pass.pClearValues=nullptr;
            vkCmdBeginRenderPass(g.command,&pass,VK_SUBPASS_CONTENTS_INLINE);
            const VkViewport vp{0,0,640,480,0,1};const VkRect2D rect{{0,0},extent};
            vkCmdSetViewport(g.command,0,1,&vp);vkCmdSetScissor(g.command,0,1,&rect);renderer.record(g.command,0);vkCmdEndRenderPass(g.command);
            g.readback(color.handle,extent,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,rb);
            VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
            barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
            barrier.image=depth.handle;barrier.subresourceRange={VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT,0,1,0,1};
            barrier.oldLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
            vkCmdPipelineBarrier(g.command,VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
            VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_DEPTH_BIT,0,0,1};copy.imageExtent={640,480,1};
            vkCmdCopyImageToBuffer(g.command,depth.handle,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,depth_read.handle,1,&copy);
            copy.imageSubresource.aspectMask=VK_IMAGE_ASPECT_STENCIL_BIT;
            vkCmdCopyImageToBuffer(g.command,depth.handle,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,stencil_read.handle,1,&copy);
            barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            barrier.srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;barrier.dstAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            vkCmdPipelineBarrier(g.command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,0,0,nullptr,0,nullptr,1,&barrier);
            VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER};host.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;host.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
            vkCmdPipelineBarrier(g.command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&host,0,nullptr,0,nullptr);
            fixture::check(vkEndCommandBuffer(g.command));VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&g.command;
            fixture::check(vkQueueSubmit(g.queue,1,&submit,fence));fixture::check(vkWaitForFences(g.device,1,&fence,VK_TRUE,5000000000ull));
            fixture::require(scene_copy.retire(),"depth comparison scene-copy fence did not retire");
        };
        draw_depth(0x207,true,2);
        const auto *depth_words=static_cast<const std::uint32_t*>(depth_read.mapped);
        std::vector<bool> coverage(640*480);
        for(unsigned p=0;p<coverage.size();++p) {
            fixture::require(depth_words[p]==source_d24_carrier_word(1)||depth_words[p]==source_d24_carrier_word(2),"production fragment did not write canonical flat-W integer depth");
            coverage[p]=depth_words[p]==source_d24_carrier_word(1);depth_coverage_pixels+=coverage[p];
        }
        fixture::require(depth_coverage_pixels>100,"production fragment depth has no useful coverage");
        constexpr unsigned background[]{29,71,113,197};
        for(unsigned comparison=0x200;comparison<=0x207;++comparison)for(bool write:{false,true})for(unsigned clear_depth:{0u,1u,2u,source_d24_maximum}) {
            draw_depth(comparison,write,clear_depth);
            const bool passes=comparison==0x207||(comparison==0x201&&1<clear_depth)||(comparison==0x202&&1==clear_depth)||
                (comparison==0x203&&1<=clear_depth)||(comparison==0x204&&1>clear_depth)||(comparison==0x205&&1!=clear_depth)||(comparison==0x206&&1>=clear_depth);
            const auto *color_bytes=static_cast<const unsigned char*>(rb.mapped),*stencil_bytes=static_cast<const unsigned char*>(stencil_read.mapped);
            for(unsigned p=0;p<coverage.size();++p) {
                fixture::require(depth_words[p]==source_d24_carrier_word(passes&&write&&coverage[p]?1:clear_depth),"production integer depth comparison/write result differs");
                fixture::require(stencil_bytes[p]==0xa7,"production integer depth draw changed the source stencil byte");
                for(unsigned c=0;c<4;++c)fixture::require(color_bytes[p*4+c]==(passes?results[1][p*4+c]:background[c]),"production integer depth comparison changed color admission");
            }
            ++depth_cases;
        }
    }
    nlohmann::json gradient_reports=nlohmann::json::array();
    if(depth_differential) {
        // Same-device original GS/D24, triangle/D24 and triangle/canonical-D32.
        // These declared view/bias controls are not an original camera/pose or
        // a cross-GPU coverage oracle. No production shader arithmetic changes.
        struct Target {
            fixture::Image color,depth;
            VkRenderPass clear,load;
            VkFramebuffer frame;
            fixture::Buffer rgba,words,stencil;
            std::unique_ptr<MenuWorldNativeDraw> renderer;
        };
        std::vector<Target> targets;
        for(unsigned mode=0;mode<3;++mode) {
            const auto format=mode==2?VK_FORMAT_D32_SFLOAT_S8_UINT:VK_FORMAT_D24_UNORM_S8_UINT;
            Target target;
            target.color=g.image(extent,VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
            target.depth=g.image(extent,format,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
            target.clear=g.scene_pass(true,format);target.load=g.scene_pass(false,format);
            target.frame=g.framebuffer(target.clear,extent,target.color,target.depth);
            target.rgba=g.buffer(640*480*4,VK_BUFFER_USAGE_TRANSFER_DST_BIT);
            target.words=g.buffer(640*480*4,VK_BUFFER_USAGE_TRANSFER_DST_BIT);
            target.stencil=g.buffer(640*480,VK_BUFFER_USAGE_TRANSFER_DST_BIT);
            MenuWorldNativeDrawContext context{g.device,g.physical,target.load,g.descriptor_layout,VK_API_VERSION_1_3,
                g.geometry_enabled,g.geometry_enabled,true,true,argv[2]};
            context.triangle_vertices=mode!=0;
            context.depth_encoding=mode==2?SourceDepthStorageEncoding::d32_integer_d24:SourceDepthStorageEncoding::native_d24;
            context.strict_shader_math_enabled=g.strict_shader_math;
            target.renderer=std::make_unique<MenuWorldNativeDraw>(context);
            targets.push_back(std::move(target));
        }
        const std::array<std::array<float,4>,3> profiles{{
            {0,0,0,1},
            {2/(maxx-minx),0,0,2-2*minx/(maxx-minx)},
            {0,0,2/(maxz-minz),.75f-2*minz/(maxz-minz)}}};
        const std::array<std::array<float,2>,7> biases{{{0,0},{0,.75f},{0,-.75f},
            {128,0},{-128,0},{0,float(source_d24_maximum)},{0,-float(source_d24_maximum)}}};
        const auto draw=[&](Target &target,unsigned mode,unsigned clear_depth) {
            const bool trace=(gradient_reports.size()<48 && clear_depth==0) || gradient_reports.size()%48==0;
            if(trace)checkpoint("preparing_mode_"+std::to_string(mode),gradient_reports.size());
            const auto packet=menu_world_native_packet(input);target.renderer->prepare(std::span(&packet,1));
            if(trace)checkpoint("recording_mode_"+std::to_string(mode),gradient_reports.size());
            fixture::check(vkResetFences(g.device,1,&fence));
            scene_copy.prepare({{{0,0,480,640},320,240,.01f,1024.f},{640,480},extent});
            g.begin();VkClearValue values[2]{};values[0].color={{29.f/255,71.f/255,113.f/255,197.f/255}};
            values[1].depthStencil={mode==2?std::bit_cast<float>(source_d24_carrier_word(clear_depth)):
                float(clear_depth)/float(source_d24_maximum),0xa7};
            VkRenderPassBeginInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};pass.renderPass=target.clear;
            pass.framebuffer=target.frame;pass.renderArea.extent=extent;pass.clearValueCount=2;pass.pClearValues=values;
            vkCmdBeginRenderPass(g.command,&pass,VK_SUBPASS_CONTENTS_INLINE);vkCmdEndRenderPass(g.command);
            scene_copy.record({g.device,g.command,target.color.handle,VK_FORMAT_R8G8B8A8_UNORM,extent,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_SAMPLE_COUNT_1_BIT,1,1,g.family,fence,true,true});
            pass.renderPass=target.load;pass.clearValueCount=0;pass.pClearValues=nullptr;
            vkCmdBeginRenderPass(g.command,&pass,VK_SUBPASS_CONTENTS_INLINE);
            const VkViewport vp{0,0,640,480,0,1};const VkRect2D rect{{0,0},extent};
            vkCmdSetViewport(g.command,0,1,&vp);vkCmdSetScissor(g.command,0,1,&rect);
            target.renderer->record(g.command,0);vkCmdEndRenderPass(g.command);
            g.readback(target.color.handle,extent,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,target.rgba);
            VkImageMemoryBarrier image{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
            image.srcQueueFamilyIndex=image.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;image.image=target.depth.handle;
            image.subresourceRange={VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT,0,1,0,1};
            image.oldLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;image.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            image.srcAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            image.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
            vkCmdPipelineBarrier(g.command,VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&image);
            VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_DEPTH_BIT,0,0,1};copy.imageExtent={640,480,1};
            vkCmdCopyImageToBuffer(g.command,target.depth.handle,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,target.words.handle,1,&copy);
            copy.imageSubresource.aspectMask=VK_IMAGE_ASPECT_STENCIL_BIT;
            vkCmdCopyImageToBuffer(g.command,target.depth.handle,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,target.stencil.handle,1,&copy);
            image.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;image.newLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            image.srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;image.dstAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            vkCmdPipelineBarrier(g.command,VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,0,0,nullptr,0,nullptr,1,&image);
            VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER};host.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;host.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
            vkCmdPipelineBarrier(g.command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&host,0,nullptr,0,nullptr);
            fixture::check(vkEndCommandBuffer(g.command));VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
            submit.commandBufferCount=1;submit.pCommandBuffers=&g.command;fixture::check(vkQueueSubmit(g.queue,1,&submit,fence));
            fixture::check(vkWaitForFences(g.device,1,&fence,VK_TRUE,5000000000ull));
            fixture::require(scene_copy.retire(),"differential depth copy/draw fence did not retire");
        };
        for(unsigned profile=0;profile<profiles.size();++profile) {
            auto bank=constants;bank[3]=profiles[profile];
            binding=menu_world_source_binding_class(cache,binding.owner(),binding.draw(),authored.pass.vertex_shader(),1,bank);
            for(const auto bias:biases) {
                input.raster.depth_factor_offset=bias;
                for(unsigned comparison=0x200;comparison<=0x207;++comparison)for(bool write:{false,true})
                    for(unsigned clear_depth:{0u,3u,source_d24_maximum}) {
                    input.forward.depth_enable=2;input.forward.depth_write=write;
                    input.forward.depth_function=comparison;input.forward_blend=0;
                    for(unsigned mode=0;mode<targets.size();++mode)draw(targets[mode],mode,clear_depth);
                    std::array<std::size_t,2> depth_mismatches{},color_mismatches{};
                    std::size_t stencil_mismatches=0;
                    const bool measure=profile && bias==biases[0] && comparison==0x207 && write && clear_depth==source_d24_maximum;
                    std::set<unsigned> written_depths;
                    unsigned smallest=source_d24_maximum,largest_depth=0;
                    for(unsigned pixel=0;pixel<640*480;++pixel) {
                        const auto reference=static_cast<const std::uint32_t*>(targets[0].words.mapped)[pixel]&source_d24_maximum;
                        smallest=std::min(smallest,reference);largest_depth=std::max(largest_depth,reference);
                        if(measure && reference!=clear_depth)written_depths.insert(reference);
                        for(unsigned mode=1;mode<targets.size();++mode) {
                            const auto preceding=static_cast<const std::uint32_t*>(targets[mode-1].words.mapped)[pixel]&source_d24_maximum;
                            const auto word=static_cast<const std::uint32_t*>(targets[mode].words.mapped)[pixel];
                            const auto value=mode==2?source_d24_carrier_integer(word):word&source_d24_maximum;
                            depth_mismatches[mode-1]+=preceding!=value;
                            color_mismatches[mode-1]+=std::memcmp(static_cast<const unsigned char*>(targets[mode-1].rgba.mapped)+pixel*4,
                                static_cast<const unsigned char*>(targets[mode].rgba.mapped)+pixel*4,4)!=0;
                            stencil_mismatches+=static_cast<const unsigned char*>(targets[mode].stencil.mapped)[pixel]!=0xa7;
                        }
                        stencil_mismatches+=static_cast<const unsigned char*>(targets[0].stencil.mapped)[pixel]!=0xa7;
                    }
                    gradient_reports.push_back({{"profile",profile},{"factor",bias[0]},{"offset",bias[1]},
                        {"comparison",comparison},{"write",write},{"clear",clear_depth},{"minimum_depth",smallest},
                        {"maximum_depth",largest_depth},{"depth_mismatches",depth_mismatches},
                        {"color_mismatches",color_mismatches},{"stencil_mismatches",stencil_mismatches},
                        {"measured_nonflat_depth_count",measure?nlohmann::json(written_depths.size()):nlohmann::json(nullptr)}});
                    if(depth_mismatches[0]||depth_mismatches[1]||color_mismatches[0]||color_mismatches[1]||stencil_mismatches||
                       (measure && written_depths.size()<2)) {
                        std::filesystem::create_directories(std::filesystem::path(argv[3]).parent_path());
                        std::ofstream(argv[3])<<nlohmann::json{{"passed",false},
                            {"depth_differential_pairs",{"GS_D24_vs_triangle_D24","triangle_D24_vs_triangle_D32"}},
                            {"depth_differential_cases",gradient_reports}}.dump(2)<<'\n';
                        throw std::runtime_error("production nonflat-W depth differential differs: "+gradient_reports.back().dump());
                    }
                }
            }
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
    std::array<std::uint64_t,3> color_hashes{};
    for(unsigned n=0;n<results.size();++n) {
        auto &hash=color_hashes[n];hash=14695981039346656037ull;
        for(const auto byte:results[n]){hash^=byte;hash*=1099511628211ull;}
    }
    const nlohmann::json report{{"color_fnv64",color_hashes},{"triangle_vertices_exact",triangle_only?nlohmann::json(nullptr):nlohmann::json(true)},{"triangle_only",triangle_only},{"geometry_shader_enabled",g.geometry_enabled},{"strict_shader_math_enabled",g.strict_shader_math},{"depth_format",depth_format},{"validation_layer_active",g.validation},{"passed",true},{"device",properties.deviceName},{"covered_pixels",covered},
        {"partial_alpha_pixels",partial_alpha},{"checked_channels",channels},{"maximum_byte_error",maximum},{"depth_comparison_cases",depth_cases},{"depth_coverage_pixels",depth_coverage_pixels},
        {"validation_errors",fixture::validation_errors},{"target19_prebound_then_produced",true},
        {"depth_differential_pairs",{"GS_D24_vs_triangle_D24","triangle_D24_vs_triangle_D32"}},{"depth_differential_cases",gradient_reports},
        {"scope","Actual original rigid Regret resource/program/material through native scene-copy and raw draw; explicit diagnostic camera/palette/raster/background, not live group/body ordering or Xbox GPU pixels"},
        {"large_artifacts_retained",nlohmann::json::array()}};
    const std::filesystem::path output=argv[3];std::filesystem::create_directories(output.parent_path());std::ofstream(output)<<report.dump(2)<<'\n';
    std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
