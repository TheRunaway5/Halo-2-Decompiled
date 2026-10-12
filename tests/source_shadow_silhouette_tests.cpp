#include "source_shadow_gpu_fixture.hpp"
#include "halo2/vertex_program.hpp"
#include <bit>
#include <numeric>
using namespace halo2;
using namespace halo2_shadow_fixture;
namespace {
struct SourceRegisters {
    std::array<VertexInstruction,136> instructions{};
    std::array<ShaderVector,16> attributes{};
    std::array<std::uint32_t,4> flags{};
    std::array<ShaderVector,192> constants{};
    ShaderVector conversion{};
};
static_assert(sizeof(SourceRegisters)==5536);
using SourceVertex=std::array<ShaderVector,4>;
struct TextureInput {unsigned width{},height{};std::vector<std::vector<std::byte>> mips;};
struct DrawInput {unsigned event{},texture{},classification{};SourceRegisters registers;
    std::vector<SourceVertex> vertices;std::vector<std::uint32_t> indices;};
template<class T>T read(std::istream &file){T result;require(bool(file.read(reinterpret_cast<char*>(&result),sizeof(result))),"shadow silhouette fixture is truncated");return result;}
template<class T>void read_array(std::istream &file,std::vector<T>&out,std::size_t count){out.resize(count);require(bool(file.read(reinterpret_cast<char*>(out.data()),count*sizeof(T))),"shadow silhouette fixture array is truncated");}
VkPipeline pipeline(Gpu &gpu,VkRenderPass pass) {
    const auto vertex=gpu.shader("source_shadow_retained.vert.spv"),fragment=gpu.shader("source_shadow_retained.frag.spv");
    const VkPipelineShaderStageCreateInfo stages[]{
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_VERTEX_BIT,vertex,"main",nullptr},
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_FRAGMENT_BIT,fragment,"main",nullptr}};
    VkVertexInputBindingDescription binding{0,sizeof(SourceVertex),VK_VERTEX_INPUT_RATE_VERTEX};
    std::array<VkVertexInputAttributeDescription,4> attrs;
    for(unsigned i=0;i<4;++i)attrs[i]={i,0,VK_FORMAT_R32G32B32A32_SFLOAT,i*16};
    VkPipelineVertexInputStateCreateInfo inputs{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    inputs.vertexBindingDescriptionCount=1;inputs.pVertexBindingDescriptions=&binding;
    inputs.vertexAttributeDescriptionCount=attrs.size();inputs.pVertexAttributeDescriptions=attrs.data();
    VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    assembly.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;assembly.primitiveRestartEnable=false;
    VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};viewport.viewportCount=viewport.scissorCount=1;
    VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    // Literal original20 generation raster state, not the controlled quad's.
    raster.depthClampEnable=true;raster.cullMode=VK_CULL_MODE_FRONT_BIT;
    raster.frontFace=VK_FRONT_FACE_CLOCKWISE;raster.polygonMode=VK_POLYGON_MODE_FILL;raster.lineWidth=1;
    VkPipelineMultisampleStateCreateInfo samples{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};samples.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
    VkPipelineDepthStencilStateCreateInfo depth{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    VkPipelineColorBlendAttachmentState write{};write.colorWriteMask=15;
    VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};blend.attachmentCount=1;blend.pAttachments=&write;
    const VkDynamicState states[]{VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};dynamic.dynamicStateCount=2;dynamic.pDynamicStates=states;
    VkGraphicsPipelineCreateInfo create{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    create.stageCount=2;create.pStages=stages;create.pVertexInputState=&inputs;create.pInputAssemblyState=&assembly;
    create.pViewportState=&viewport;create.pRasterizationState=&raster;create.pMultisampleState=&samples;
    create.pDepthStencilState=&depth;create.pColorBlendState=&blend;create.pDynamicState=&dynamic;
    create.layout=gpu.layout;create.renderPass=pass;VkPipeline result;
    check(vkCreateGraphicsPipelines(gpu.device,0,1,&create,nullptr,&result));gpu.pipelines.push_back(result);return result;
}
VkSampler sampler(Gpu &gpu,unsigned levels) {
    VkSamplerCreateInfo info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};info.magFilter=info.minFilter=VK_FILTER_LINEAR;
    info.mipmapMode=VK_SAMPLER_MIPMAP_MODE_LINEAR;
    info.addressModeU=info.addressModeV=info.addressModeW=VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.maxLod=float(levels-1);VkSampler result;check(vkCreateSampler(gpu.device,&info,nullptr,&result));
    gpu.samplers.push_back(result);return result;
}
void upload(Gpu &gpu,Image image,const TextureInput &input,bool transparent) {
    const auto bytes=std::accumulate(input.mips.begin(),input.mips.end(),std::size_t{},[](auto size,const auto &mip){return size+mip.size();});
    auto staging=gpu.buffer(bytes,VK_BUFFER_USAGE_TRANSFER_SRC_BIT);std::vector<VkBufferImageCopy> copies;
    std::size_t offset=0;
    for(unsigned mip=0;mip<input.mips.size();++mip) {
        std::memcpy(static_cast<std::byte*>(staging.mapped)+offset,input.mips[mip].data(),input.mips[mip].size());
        if(transparent)for(std::size_t i=3;i<input.mips[mip].size();i+=4)static_cast<std::byte*>(staging.mapped)[offset+i]=std::byte{};
        VkBufferImageCopy copy{};copy.bufferOffset=offset;copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,mip,0,1};
        copy.imageExtent={std::max(1u,input.width>>mip),std::max(1u,input.height>>mip),1};copies.push_back(copy);offset+=input.mips[mip].size();
    }
    gpu.begin();VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
    barrier.image=image.handle;barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,image.mip_levels,0,1};
    barrier.oldLayout=VK_IMAGE_LAYOUT_UNDEFINED;barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;barrier.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(gpu.command,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
    vkCmdCopyBufferToImage(gpu.command,staging.handle,image.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,copies.size(),copies.data());
    barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(gpu.command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,0,nullptr,0,nullptr,1,&barrier);gpu.end();
}
}
int main(){try {
    static_assert(std::endian::native==std::endian::little);
    const auto root=std::filesystem::path(HALO2_SOURCE_DIR);
    std::ifstream file(root/"tests/fixtures/retail_shadow_silhouette.bin",std::ios::binary);
    if(!file||!std::filesystem::exists(root/"assets/disc/maps/zanzibar.map"))return 77;
    require(read<std::uint32_t>(file)==0x47533248&&read<std::uint32_t>(file)==1,"shadow silhouette version differs");
    const auto count=read<unsigned>(file),texture_count=read<unsigned>(file);require(count==7&&texture_count==2,"retained source draw/texture count differs");
    std::vector<TextureInput> textures(texture_count);
    for(auto &texture:textures) {
        texture.width=read<unsigned>(file);texture.height=read<unsigned>(file);const auto levels=read<unsigned>(file);
        require(texture.width<=512&&texture.height<=512&&levels<=10&&levels>0,"source texture metadata differs");
        texture.mips.resize(levels);
        for(unsigned i=0;i<levels;++i){const auto bytes=read<unsigned>(file);require(bytes==std::max(1u,texture.width>>i)*std::max(1u,texture.height>>i)*4,"source mip size differs");read_array(file,texture.mips[i],bytes);}
    }
    std::vector<DrawInput> draws(count);unsigned vertices=0,indices=0;
    for(auto &draw:draws) {
        draw.event=read<unsigned>(file);const auto index_count=read<unsigned>(file),vertex_count=read<unsigned>(file);
        draw.texture=read<unsigned>(file);draw.classification=read<unsigned>(file);
        require(index_count<=2200&&vertex_count<=1600&&draw.texture<texture_count,"source geometry count differs");
        draw.registers.attributes=read<decltype(draw.registers.attributes)>(file);
        draw.registers.constants=read<decltype(draw.registers.constants)>(file);draw.registers.conversion=read<ShaderVector>(file);
        read_array(file,draw.vertices,vertex_count);read_array(file,draw.indices,index_count);
        std::ifstream vpfile(root/"tests/fixtures"/("retail_shadow_generate_class"+std::to_string(draw.classification)+".vp"),std::ios::binary);
        require(bool(vpfile),"retained authored shadow VP missing");const std::string code((std::istreambuf_iterator<char>(vpfile)),{});
        const auto program=decode_vertex_program({reinterpret_cast<const std::byte*>(code.data()),code.size()});
        draw.registers.instructions=program.instructions;draw.registers.flags[3]=program.count;
        for(auto index:draw.indices)require(index<vertex_count,"source strip outside raw vertex bank");
        vertices+=vertex_count;indices+=index_count;
    }
    require(vertices==4580&&indices==6631&&file.peek()==std::char_traits<char>::eof(),"retained complete geometry census differs");
    std::ifstream expected_file(root/"tests/fixtures/retail_shadow_silhouette_expected.bgra8",std::ios::binary);
    std::vector<std::byte> expected;read_array(expected_file,expected,128*128*4);
    std::ifstream filtered_file(root/"tests/fixtures/retail_shadow_filtered_expected.bgra8",std::ios::binary);
    std::vector<std::byte> filtered_expected;read_array(filtered_file,filtered_expected,128*128*4);
    unsigned black=0;
    for(unsigned pixel=0;pixel<128*128;++pixel)
        black+=expected[pixel*4]==std::byte{}&&expected[pixel*4+1]==std::byte{}&&expected[pixel*4+2]==std::byte{};
    require(black>1000&&black<128*128,"retained target has no meaningful silhouette");
    Cache cache(root/"assets/disc/maps/zanzibar.map");
    const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[](const auto&t){return t.group=="shad"&&t.name=="objects\\weapons\\rifle\\battle_rifle\\shaders\\battle_rifle_lod";});
    require(tag!=cache.tags().end(),"source material missing");const auto m=cache.load_material(tag->datum);
    const auto generation=prepare_shadow_material(cache,m,SourceShadowPhase::Generate,true,false,.75f);
    const auto application=prepare_shadow_material(cache,m,SourceShadowPhase::Apply,true,false,.75f);
    require(generation.size()==1&&application.size()==1,"source pass admission differs");
    Gpu gpu;if(!gpu.initialize())return 77;
    std::array<std::vector<Image>,2> images;std::vector<VkSampler> samplers;
    for(unsigned control=0;control<2;++control)for(const auto &texture:textures) {
        auto image=gpu.image({texture.width,texture.height},VK_FORMAT_B8G8R8A8_UNORM,VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT,texture.mips.size());
        upload(gpu,image,texture,control!=0);images[control].push_back(image);
        if(!control)samplers.push_back(sampler(gpu,texture.mips.size()));
    }
    auto initial=gpu.scene_pass(true),continuation=gpu.scene_pass(false);
    unsigned checked=0;
    for(const VkExtent2D extent:std::array{VkExtent2D{320,240},VkExtent2D{853,479}}) {
        const auto color=gpu.image(extent,VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT),depth=gpu.image(extent,VK_FORMAT_D32_SFLOAT,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
        const auto framebuffer=gpu.framebuffer(initial,extent,color,depth);
        SourceShadowRender renderer({gpu.device,gpu.physical,extent,continuation});const auto generation_pipeline=pipeline(gpu,renderer.generation_pass());
        const auto readback=gpu.buffer(128*128*4,VK_BUFFER_USAGE_TRANSFER_DST_BIT);
        for(unsigned filtered=0;filtered<2;++filtered)for(unsigned control=0;control<2;++control) {
            std::vector<SourceShadowDraw> casters;
            for(const auto &draw:draws) {
                const auto vb=gpu.buffer(draw.vertices.size()*sizeof(SourceVertex),VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
                const auto ib=gpu.buffer(draw.indices.size()*4,VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
                const auto registers=gpu.buffer(sizeof(SourceRegisters),VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
                std::memcpy(vb.mapped,draw.vertices.data(),vb.size);std::memcpy(ib.mapped,draw.indices.data(),ib.size);
                std::memcpy(registers.mapped,&draw.registers,sizeof(draw.registers));
                Uniform pixel{generation[0].material.forward_program.words,{},{},{},{4,0,0,0}};
                const auto descriptor=gpu.descriptor(images[control][draw.texture],images[control][draw.texture],samplers[draw.texture],samplers[draw.texture],pixel);
                const VkDescriptorBufferInfo buffer{registers.handle,0,sizeof(SourceRegisters)};VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
                write.dstSet=descriptor;write.dstBinding=3;write.descriptorCount=1;write.descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;write.pBufferInfo=&buffer;
                vkUpdateDescriptorSets(gpu.device,1,&write,0,nullptr);
                casters.push_back({&generation[0],[&,vb,ib,descriptor,index_count=draw.indices.size()](VkCommandBuffer cmd,const SourceShadowUniforms&){
                    const VkDeviceSize offset=0;vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,generation_pipeline);
                    vkCmdBindVertexBuffers(cmd,0,1,&vb.handle,&offset);vkCmdBindIndexBuffer(cmd,ib.handle,0,VK_INDEX_TYPE_UINT32);
                    vkCmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,gpu.layout,0,1,&descriptor,0,nullptr);
                    vkCmdDrawIndexed(cmd,index_count,1,0,0,0);
                }});
            }
            // This replay compares target10 only; receiver geometry/scene image
            // is covered separately and is not substituted with a synthetic draw.
            const SourceShadowDraw receiver{&application[0],[](VkCommandBuffer,const SourceShadowUniforms&){}};
            SourceShadowSubmission root_submission;root_submission.casters=casters;root_submission.receivers={&receiver,1};
            root_submission.filter=filtered!=0;
            gpu.begin();VkClearValue clear[2]{};clear[1].depthStencil={1,0};VkRenderPassBeginInfo begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
            begin.renderPass=initial;begin.framebuffer=framebuffer;begin.renderArea={{0,0},extent};begin.clearValueCount=2;begin.pClearValues=clear;
            vkCmdBeginRenderPass(gpu.command,&begin,VK_SUBPASS_CONTENTS_INLINE);
            renderer.record(gpu.command,{framebuffer,{0,0,float(extent.width),float(extent.height),0,1},{{0,0},extent}},{&root_submission,1});
            vkCmdEndRenderPass(gpu.command);gpu.readback(renderer.target_image(),{128,128},VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,readback);gpu.end();
            const auto bytes=std::span(static_cast<const std::byte*>(readback.mapped),std::size_t(readback.size));unsigned differing=0;
            if(!control&&extent.width==320) {
                std::ofstream output(root/"analysis/multiplayer-shading"/(filtered?"shadow-filter-native.bgra8":"shadow-silhouette-native.bgra8"),std::ios::binary);
                output.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
            }
            for(unsigned i=0;i<bytes.size();++i) {
                const auto reference=control?(i%4==3?std::byte{}:std::byte{255}):(filtered?filtered_expected[i]:expected[i]);
                differing+=bytes[i]!=reference;++checked;
            }
            if(differing){std::ofstream output(root/"analysis/multiplayer-shading"/(filtered?"shadow-filter-native.bgra8":"shadow-silhouette-native.bgra8"),std::ios::binary);output.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());throw std::runtime_error(std::string(filtered?"filtered":"generated")+" complete original shadow target differs in "+std::to_string(differing)+" bytes");}
        }
    }
    require(validation_errors==0,"retained shadow GPU validation errors");
    std::cout<<checked<<" complete 128x128 original silhouette/two-pass-filter/alpha-kill/resize bytes exact across 7 source draws, 4580 vertices, 6631 indices; "<<black<<" black silhouette pixels\n";
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
