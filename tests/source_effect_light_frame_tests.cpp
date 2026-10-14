#include "halo2/source_bsp_effect_lights.hpp"
#include "halo2/source_model_effect_lights.hpp"
#include "halo2/menu_world_native_packet.hpp"
#include "halo2/sampler.hpp"
#include "source_effect_light_gpu_fixture.hpp"
#include "menu_world_vertex_catalog.generated.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <map>
#if __has_include("renderdoc_app.h") && !defined(_WIN32)
#include "renderdoc_app.h"
#include <dlfcn.h>
#endif

// Replay only this source-produced illumination segment, with original color,
// depth and sampled images as explicit test inputs. Never a whole-gameframe
// or TU1.5 claim: the caller must separately authenticate the captured image.
using namespace halo2;
using namespace halo2_effect_light_fixture;
using Json=nlohmann::json;
namespace fs=std::filesystem;
namespace {
Json json(const fs::path& p){std::ifstream f(p);require(bool(f),"missing light frame JSON");Json j;f>>j;return j;}
std::vector<std::byte> read(const fs::path& p){std::ifstream f(p,std::ios::binary|std::ios::ate);require(bool(f)&&f.tellg()>0,"missing light frame bytes");std::vector<std::byte>b(static_cast<std::size_t>(f.tellg()));f.seekg(0);require(bool(f.read(reinterpret_cast<char*>(b.data()),b.size())),"short light frame bytes");return b;}
void save(const fs::path& p,std::span<const std::byte> b){std::ofstream f(p,std::ios::binary);require(bool(f.write(reinterpret_cast<const char*>(b.data()),b.size())),"light frame write failed");}
float real(std::uint32_t word){return std::bit_cast<float>(word);}
SourceBspAdmission admission(const Json& j){SourceBspAdmission a;require(j.at("entries").size()==a.cluster_entries.size(),"light frame cluster bank");for(unsigned n=0;n<a.cluster_entries.size();++n)a.cluster_entries[n]=j.at("entries")[n].get<std::uint8_t>();
    const auto q=[](const Json& rows,auto& out){for(const auto&r:rows)out.push_back({r[0].template get<std::uint32_t>(),r[1].template get<std::uint16_t>(),r[2].template get<std::uint16_t>(),r[3].template get<std::uint32_t>()});};q(j.at("clusters"),a.clusters);q(j.at("instances"),a.instances);return a;}
std::vector<EffectLightRawDraw> draws(SourceBspEffectLights& owner,const EffectLightDefinition& definition,const Json& j,
    const Cache& cache,Mesh& mesh){
    const auto r=j.at("light").get<std::array<std::uint32_t,68>>();
    const auto scalar=[&](unsigned at){return real(r.at(at/4));};const auto point=[&](unsigned at){return Vec3{scalar(at),scalar(at+4),scalar(at+8)};};
    SourceEffectLightView lights;SourceEffectLightViewRecord l{};l.definition=&definition;l.definition_identity=r[1];
    l.runtime={j.at("identity"),r[0x4c/4],std::uint8_t(r[0]>>16),std::int16_t(r[0x54/4]),std::int16_t(r[0x48/4]),std::bit_cast<std::int32_t>(r[2]),scalar(0x24),scalar(0x100),point(0x18),point(0x84),point(0xac),point(0xb8),{scalar(0xc4),scalar(0xd0),point(0xd4),point(0xe0),scalar(0xec)}};
    l.fades={scalar(0xf8),scalar(0xfc),scalar(0x104),scalar(0x100),scalar(0x108)};lights.records.push_back(l);
    WorldEffectLightReceiver receiver;receiver.identity=l.runtime.identity;receiver.selected_fade=real(j.at("selected_fade"));receiver.view.bsp=admission(j.at("receiver"));receiver.view.pass_mode=j.at("pass_mode");receiver.view.query_mode=j.at("query_mode");
    SourceViewProjectionBank camera{};const auto words=j.at("camera").get<std::array<std::uint32_t,48>>();std::memcpy(&camera,words.data(),sizeof(camera));
    BspFogSample fog{};const auto&f=j.at("fog");fog.classification=f.at("classification");fog.planar_datum=f.at("planar_datum");fog.eye_fraction=real(f.at("eye_fraction"));fog.planar_flags=f.at("planar_flags");fog.planar_only=f.at("planar_only");
    auto bsp=owner.draw(lights,std::span(&receiver,1),admission(j.at("main")),camera,fog,camera.camera_to_world.position,{0,0,480,640},{640,480},1);
    if(!j.contains("models"))return bsp;
    require(!bsp.empty(),"combined light fixture requires its source BSP camera bank");
    const auto constants=bsp.front().source.constants();
    const auto uniforms=source_effect_light_view_uniforms(definition,l.runtime.state,l.runtime.position,l.runtime.forward,
        l.runtime.up,l.fades,receiver.selected_fade,{});
    using Key=std::pair<std::uint32_t,unsigned>;
    std::map<Key,std::unique_ptr<SourceModelEffectLights>> models;
    const std::array ids{r[1]};
    for(const auto& model:j.at("models")){
        const Key key{model.at("model"),model.at("section")};
        if(!models.contains(key))models.emplace(key,std::make_unique<SourceModelEffectLights>(cache,mesh,key.first,key.second,ids));
    }
    unsigned next_bsp{};std::vector<EffectLightRawDraw> combined;
    for(const auto& expected:j.at("expected")){
        if(expected.at("kind")!=2){require(next_bsp<bsp.size(),"combined source BSP plan escaped native output");combined.push_back(std::move(bsp[next_bsp++]));continue;}
        const auto found=std::find_if(j.at("models").begin(),j.at("models").end(),[&](const auto& model){
            return model.at("model")==expected.at("model") && model.at("object")==expected.at("object") &&
                model.at("section")==expected.at("section") && model.at("part")==expected.at("part");});
        require(found!=j.at("models").end(),"original model draw has no explicit current owner/palette input");const auto& model=*found;
        const auto palette=model.at("palette").get<std::vector<std::array<float,12>>>();
        SourceModelEffectLightDrawInput in;
        in.object=model.at("object");in.light=r[1];in.shader=model.at("shader");in.part=model.at("part");
        in.shader_lod=model.at("lod");in.pass=model.at("pass_index");in.vertex_class=model.at("vertex_class");
        in.topology=expected.at("topology")==1?MenuWorldSourceTopology::triangle_strip:MenuWorldSourceTopology::triangle_list;
        in.range=MenuWorldSourceIndexRange{expected.at("first"),expected.at("count")};in.palette=palette;
        in.constants=constants;in.uniforms=uniforms;
        MenuWorldRasterContext raster;raster.viewport_reset({640,480});raster.game_initialize_offset();raster.layer_begin(MenuWorldRasterLayer::sphere_diffuse_12);
        raster.entry_begin(menu_world_raster_entry_from_source_flags(model.at("flags"),false,false));
        in.raster=raster.snapshot({MenuWorldRasterPrimitive::ordinary_triangles,MenuWorldRasterPolygon::fill,
            std::array<std::int32_t,2>{1,1},true});raster.entry_end();
        in.raster.window_exclusive=false;for(auto& rect:in.raster.regions)rect={0,0,0,0};in.raster.regions[0]={0,0,640,480};
        in.clip_range=bsp.front().clip_range;in.fog_parameters=bsp.front().fog_parameters;
        combined.push_back(models.at({model.at("model"),model.at("section")})->draw(in));
    }
    require(next_bsp==bsp.size(),"combined source plan dropped native BSP light output");return combined;
}
void barrier(Gpu&g,Image image,VkImageAspectFlags aspects,VkImageLayout old,VkImageLayout next,VkAccessFlags src,VkAccessFlags dst,VkPipelineStageFlags before,VkPipelineStageFlags after,unsigned layers=1){VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};b.image=image.handle;b.subresourceRange={aspects,0,image.mip_levels,0,layers};b.oldLayout=old;b.newLayout=next;b.srcAccessMask=src;b.dstAccessMask=dst;b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;vkCmdPipelineBarrier(g.command,before,after,0,0,nullptr,0,nullptr,1,&b);}
void seed(Gpu&g,Image color,Image depth,const std::vector<std::byte>&rgba,const std::vector<std::byte>&original){
    constexpr unsigned pixels=640*480;require(rgba.size()==pixels*4&&original.size()==rgba.size(),"original light target extent");
    auto b=g.buffer(pixels*9,VK_BUFFER_USAGE_TRANSFER_SRC_BIT);std::memcpy(b.mapped,rgba.data(),rgba.size());auto *raw=static_cast<std::byte*>(b.mapped);
    for(unsigned i=0;i<pixels;++i){std::uint32_t w;std::memcpy(&w,original.data()+i*4,4);const auto d=w>>8;std::memcpy(raw+pixels*4+i*4,&d,4);raw[pixels*8+i]=std::byte(w&255);}
    g.begin();barrier(g,color,VK_IMAGE_ASPECT_COLOR_BIT,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,0,VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
    barrier(g,depth,VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,0,VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
    VkBufferImageCopy c{};c.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};c.imageExtent={640,480,1};vkCmdCopyBufferToImage(g.command,b.handle,color.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&c);
    VkBufferImageCopy d[2]{};d[0].bufferOffset=pixels*4;d[0].imageSubresource={VK_IMAGE_ASPECT_DEPTH_BIT,0,0,1};d[1].bufferOffset=pixels*8;d[1].imageSubresource={VK_IMAGE_ASPECT_STENCIL_BIT,0,0,1};for(auto&r:d)r.imageExtent={640,480,1};vkCmdCopyBufferToImage(g.command,b.handle,depth.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,2,d);
    barrier(g,color,VK_IMAGE_ASPECT_COLOR_BIT,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    barrier(g,depth,VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);g.end();
    auto verify=g.buffer(pixels*5,VK_BUFFER_USAGE_TRANSFER_DST_BIT);g.begin();
    barrier(g,depth,VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,VK_ACCESS_TRANSFER_READ_BIT,VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
    d[0].bufferOffset=0;d[1].bufferOffset=pixels*4;vkCmdCopyImageToBuffer(g.command,depth.handle,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,verify.handle,2,d);
    barrier(g,depth,VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,VK_ACCESS_TRANSFER_READ_BIT,VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);g.end();
    const auto* checked=static_cast<const std::byte*>(verify.mapped);
    for(unsigned i=0;i<pixels;++i){std::uint32_t source,actual;std::memcpy(&source,original.data()+i*4,4);std::memcpy(&actual,checked+i*4,4);require((actual&0xffffff)==(source>>8)&&checked[pixels*4+i]==std::byte(source&255),"source D24/stencil seed is not exact");}
}
struct Sampled{Image image;Texture shape;};
Sampled sampled(Gpu&g,const fs::path& directory,const Json& descriptor){
    const auto meta=json(directory/"description.json");Sampled out;auto&shape=out.shape;
    shape.width=meta.at("width");shape.height=meta.at("height");shape.layers=meta.at("arraysize");shape.depth=meta.at("type")==9?meta.at("depth").get<unsigned>():0;shape.format=11;
    const unsigned mips=meta.at("mips"),components=meta.at("format").at("compCount");require(meta.at("format").at("compByteWidth")==1&&meta.at("format").at("compType")==2,"original light image is not byte UNORM");
    require(components==1||components==3||components==4,"original light image channels");
    auto&i=out.image;i.mip_levels=mips;
    VkImageCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};ci.imageType=shape.depth?VK_IMAGE_TYPE_3D:VK_IMAGE_TYPE_2D;ci.flags=shape.layers==6?VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT:0;ci.format=VK_FORMAT_R8G8B8A8_UNORM;ci.extent={shape.width,shape.height,std::max(1u,shape.depth)};ci.mipLevels=mips;ci.arrayLayers=shape.layers;ci.samples=VK_SAMPLE_COUNT_1_BIT;ci.tiling=VK_IMAGE_TILING_OPTIMAL;ci.usage=VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;check(vkCreateImage(g.device,&ci,nullptr,&i.handle));
    VkMemoryRequirements mr;vkGetImageMemoryRequirements(g.device,i.handle,&mr);VkMemoryAllocateInfo ma{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};ma.allocationSize=mr.size;ma.memoryTypeIndex=g.memory(mr.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);check(vkAllocateMemory(g.device,&ma,nullptr,&i.memory));check(vkBindImageMemory(g.device,i.handle,i.memory,0));
    VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};vi.image=i.handle;vi.viewType=shape.depth?VK_IMAGE_VIEW_TYPE_3D:shape.layers==6?VK_IMAGE_VIEW_TYPE_CUBE:VK_IMAGE_VIEW_TYPE_2D;vi.format=ci.format;vi.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,mips,0,shape.layers};check(vkCreateImageView(g.device,&vi,nullptr,&i.view));g.images.push_back(i);
    std::vector<std::byte> pixels;std::vector<VkBufferImageCopy> regions;
    for(unsigned layer=0;layer<shape.layers;++layer)for(unsigned mip=0;mip<mips;++mip){const auto w=std::max(1u,shape.width>>mip),h=std::max(1u,shape.height>>mip),d=std::max(1u,shape.depth>>mip);const auto bytes=read(directory/("mip-"+std::to_string(mip)+"-slice-"+std::to_string(layer)+".bin"));require(bytes.size()==std::size_t(w)*h*d*components,"original image mip extent differs");
        VkBufferImageCopy copy{};copy.bufferOffset=pixels.size();copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,mip,layer,1};copy.imageExtent={w,h,d};regions.push_back(copy);
        for(std::size_t p=0;p<bytes.size();p+=components)for(const char*channel:{"red","green","blue","alpha"}){const unsigned source=descriptor.at("swizzle").at(channel);require(source<6,"original image swizzle");pixels.push_back(source==4?std::byte{0}:source==5?std::byte{255}:source<components?bytes[p+source]:std::byte{255});}
    }
    auto b=g.buffer(pixels.size(),VK_BUFFER_USAGE_TRANSFER_SRC_BIT);std::memcpy(b.mapped,pixels.data(),pixels.size());g.begin();barrier(g,i,VK_IMAGE_ASPECT_COLOR_BIT,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,0,VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,shape.layers);vkCmdCopyBufferToImage(g.command,b.handle,i.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,regions.size(),regions.data());barrier(g,i,VK_IMAGE_ASPECT_COLOR_BIT,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_SHADER_READ_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,shape.layers);g.end();return out;
}
Json difference(std::span<const std::byte>a,std::span<const std::byte>b){
    require(a.size()==b.size(),"light frame comparison extent");
    unsigned maximum=0;std::size_t channels=0,pixels=0,rgb_channels=0,rgb_pixels=0,alpha_pixels=0;std::uint64_t total=0;
    for(std::size_t i=0;i<a.size();i+=4){
        bool changed=false,rgb_changed=false;
        for(unsigned c=0;c<4;++c){
            const unsigned delta=std::abs(int(a[i+c])-int(b[i+c]));
            channels+=delta!=0;changed|=delta!=0;
            if(c<3){rgb_channels+=delta!=0;rgb_changed|=delta!=0;}else alpha_pixels+=delta!=0;
            maximum=std::max(maximum,delta);total+=delta;
        }
        pixels+=changed;rgb_pixels+=rgb_changed;
    }
    return Json{{"changed_pixels",pixels},{"changed_channels",channels},{"changed_rgb_pixels",rgb_pixels},
        {"changed_rgb_channels",rgb_channels},{"changed_alpha_pixels",alpha_pixels},
        {"maximum_channel_error",maximum},{"absolute_channel_error_sum",total}};
}
}
int main(int argc,char**argv)try{
    require(argc==5,"Supply MAP ORIGINAL_OWNER_JSON ORIGINAL_GPU_EXPORT NEW_OUTPUT");const fs::path gpu=argv[3],out=argv[4];const auto manifest=json(gpu/"manifest.json");require(manifest.at("frames").size()==1,"select one original light frame");const auto frame=gpu/("frame-"+std::to_string(manifest.at("frames")[0].at("ordinal").get<unsigned>()));require(!fs::exists(out),"light frame output already exists");fs::create_directories(out);
    Cache cache(argv[1]);Mesh mesh;const auto reference=json(argv[2]);const std::array ids{reference.at("light")[1].get<std::uint32_t>()};const auto definition=load_effect_light(cache,ids[0]);SourceBspEffectLights owner(cache,mesh,0,ids);const auto selected=draws(owner,definition,reference,cache,mesh);require(selected.size()==reference.at("expected").size(),"original light draw count differs");
    Gpu g;if(!g.initialize())return 77;VkPhysicalDeviceProperties properties;vkGetPhysicalDeviceProperties(g.physical,&properties);
    const auto color=g.image({640,480},VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT);const auto depth=g.image({640,480},VK_FORMAT_D24_UNORM_S8_UINT,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT);const auto pass=g.scene_pass(false,VK_FORMAT_D24_UNORM_S8_UINT);const auto fb=g.framebuffer(pass,{640,480},color,depth);
    const auto start=std::getenv("HALO2_LIGHT_FRAME_DRAW_BEGIN");const auto begin_draw=start?std::stoul(start):0u;require(begin_draw<selected.size(),"light frame draw begin");char seed_name[40];std::snprintf(seed_name,sizeof(seed_name),"draw-%02u-before",unsigned(begin_draw));const std::string seed_phase=begin_draw?seed_name:"before";
    const auto before=read(frame/(seed_phase+"-color0.bin")),z=read(frame/(seed_phase+"-depth.bin"));seed(g,color,depth,before,z);
    VkDescriptorSetLayoutBinding bindings[7]{};for(unsigned i=0;i<7;++i)bindings[i]={i,i==4?VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,i>=5?4u:1u,VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,nullptr};VkDescriptorSetLayoutCreateInfo dl{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};dl.bindingCount=7;dl.pBindings=bindings;check(vkCreateDescriptorSetLayout(g.device,&dl,nullptr,&g.descriptor_layout));
    VkDescriptorPoolSize pool_sizes[]{{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,unsigned(selected.size())},{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,unsigned(selected.size()*12)}};VkDescriptorPoolCreateInfo dp{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};dp.maxSets=selected.size();dp.poolSizeCount=2;dp.pPoolSizes=pool_sizes;check(vkCreateDescriptorPool(g.device,&dp,nullptr,&g.descriptors));
    std::vector<VkDescriptorSetLayout> layouts(selected.size(),g.descriptor_layout);std::vector<VkDescriptorSet> sets(selected.size());VkDescriptorSetAllocateInfo da{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};da.descriptorPool=g.descriptors;da.descriptorSetCount=sets.size();da.pSetLayouts=layouts.data();check(vkAllocateDescriptorSets(g.device,&da,sets.data()));
    std::map<std::string,Sampled> images;std::vector<MenuWorldNativeDrawInput> packets;Json measurements=Json::array();std::size_t bank_errors=0,combiner_errors=0;
    for(unsigned n=0;n<selected.size();++n){const auto&draw=selected[n];const auto&material=mesh.materials.at(draw.material_slot);char label[32];std::snprintf(label,sizeof(label),"draw-%02u",n);const auto state=json(frame/(std::string(label)+"-state.json"));const auto&reads=state.at("read_resources");require(reads.size()==4,"original light texture stage extent");std::array<const Sampled*,4> stages{};
        for(unsigned stage=0;stage<4;++stage){const auto&desc=reads[stage].at("descriptor");const std::string resource=desc.at("resource");const auto key="bound-"+resource.substr(resource.find("::")+2);if(!images.contains(key))images.emplace(key,sampled(g,gpu/"textures"/key,desc));stages[stage]=&images.at(key);}
        WorldMaterialUniform uniform{};uniform.program=draw.program.words;uniform.vertex_attributes=draw.constants[0];uniform.vertex_constants=draw.constants[1];uniform.alpha_test={material.forward_raster.alpha_test,material.forward_raster.alpha_function,material.forward_raster.alpha_reference,0};for(unsigned s=0;s<4;++s)uniform.texture_effects[s]={material.texture_states[s].alpha_kill,material.accumulation_texture&&s==0?1u:0u,stages[s]->shape.depth?1u:0u,material.texture_states[s].key_color};
        auto ub=g.buffer(sizeof(uniform),VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);std::memcpy(ub.mapped,&uniform,sizeof(uniform));VkDescriptorBufferInfo buffer{ub.handle,0,sizeof(uniform)};
        std::array<VkDescriptorImageInfo,4> two{},cube{},volume{};
        for(unsigned s=0;s<4;++s){auto native=native_sampler(material.texture_states[s],properties.limits,true,!std::getenv("HALO2_LIGHT_FRAME_SEAMLESS_CONTROL"));require(!native.custom_border,"light test requires explicit custom-border device support");VkSampler sampler;check(vkCreateSampler(g.device,&native.info,nullptr,&sampler));g.samplers.push_back(sampler);two[s]={sampler,(s==1||s==3?stages[0]:stages[s])->image.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};cube[s]={sampler,stages[1]->image.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};volume[s]={sampler,stages[3]->image.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};}
        std::array<VkWriteDescriptorSet,7> writes{};for(unsigned s=0;s<7;++s){auto&w=writes[s];w={VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};w.dstSet=sets[n];w.dstBinding=s;w.descriptorCount=s>=5?4:1;w.descriptorType=s==4?VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;if(s<4)w.pImageInfo=&two[s];else if(s==4)w.pBufferInfo=&buffer;else w.pImageInfo=s==5?cube.data():volume.data();}vkUpdateDescriptorSets(g.device,writes.size(),writes.data(),0,nullptr);
        MenuWorldNativePacketInput input{draw.source,generated_menu_world_vertex_catalog::entries,uniform};input.fragment_offsets=draw.program.fragment_offsets;input.forward=material.forward_raster;input.forward_blend=10;input.texture_states=material.texture_states;input.sampler_limits={properties.limits.maxSamplerLodBias,properties.limits.maxSamplerAnisotropy,true,false};for(unsigned s=0;s<4;++s)input.images[s]=&stages[s]->shape;input.raster=draw.raster;input.clip_range=draw.clip_range;input.fog_parameters=draw.fog_parameters;input.surface_size={640,480};input.demote_to_helper_enabled=true;input.material_set=sets[n];if(material.accumulation_texture)input.texture_coordinates[0]=MenuWorldTextureCoordinates::pixel;
        packets.push_back(menu_world_native_packet(input));const auto&packet=packets.back();
        for(const auto&v:state.at("uniforms").at("0")[0].at("contents"))if(v.at("name")=="c")for(auto c:packet.vertex_shader->consumed_constants)for(unsigned lane=0;lane<4;++lane){const float expected=v.at("members")[c].at("value").at("f32v")[lane];bank_errors+=std::bit_cast<std::uint32_t>(expected)!=std::bit_cast<std::uint32_t>(packet.vertex.constants[c][lane]);}
        for(const auto&v:state.at("uniforms").at("4")[0].at("contents"))if(v.at("name")=="consts")for(unsigned c=0;c<v.at("members").size();++c)for(unsigned lane=0;lane<4;++lane){const float expected=v.at("members")[c].at("value").at("f32v")[lane];combiner_errors+=std::bit_cast<std::uint32_t>(expected)!=std::bit_cast<std::uint32_t>(packet.fragment.combiner_constants[c][lane]);}
    }
    const auto limit=std::getenv("HALO2_LIGHT_FRAME_DRAW_LIMIT");const auto count=limit?std::stoul(limit):packets.size();require(count>begin_draw&&count<=packets.size(),"light frame draw limit");
    const auto rb=g.buffer(before.size(),VK_BUFFER_USAGE_TRANSFER_DST_BIT);g.begin();g.readback(color.handle,{640,480},VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,rb);g.end();require(std::memcmp(rb.mapped,before.data(),before.size())==0,"source before color seed is not exact");
    {MenuWorldNativeDraw renderer({g.device,g.physical,pass,g.descriptor_layout,VK_API_VERSION_1_3,true,true,true,true});renderer.prepare(packets);
#if __has_include("renderdoc_app.h") && !defined(_WIN32)
        RENDERDOC_API_1_6_0* capture=nullptr;
        if(const auto path=std::getenv("HALO2_LIGHT_FRAME_CAPTURE"))if(auto library=dlopen("librenderdoc.so",RTLD_NOW|RTLD_NOLOAD)) {
            auto api=reinterpret_cast<pRENDERDOC_GetAPI>(dlsym(library,"RENDERDOC_GetAPI"));
            require(api&&api(eRENDERDOC_API_Version_1_6_0,reinterpret_cast<void**>(&capture)),"light frame RenderDoc API");
            capture->SetCaptureFilePathTemplate(path);capture->StartFrameCapture(RENDERDOC_DEVICEPOINTER_FROM_VKINSTANCE(g.instance),nullptr);
        }
#endif
        for(unsigned n=begin_draw;n<count;++n){g.begin();VkRenderPassBeginInfo begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};begin.renderPass=pass;begin.framebuffer=fb;begin.renderArea={{0,0},{640,480}};vkCmdBeginRenderPass(g.command,&begin,VK_SUBPASS_CONTENTS_INLINE);VkViewport viewport{0,0,640,480,0,1};VkRect2D rect{{0,0},{640,480}};vkCmdSetViewport(g.command,0,1,&viewport);vkCmdSetScissor(g.command,0,1,&rect);renderer.record(g.command,n);vkCmdEndRenderPass(g.command);g.readback(color.handle,{640,480},VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,rb);g.end();char label[32];std::snprintf(label,sizeof(label),"draw-%02u",n);const auto expected=read(frame/(std::string(label)+"-color0.bin"));auto result=difference({static_cast<const std::byte*>(rb.mapped),before.size()},expected);result["draw"]=n;measurements.push_back(result);save(out/(std::string(label)+"-native.bin"),{static_cast<const std::byte*>(rb.mapped),before.size()});}
#if __has_include("renderdoc_app.h") && !defined(_WIN32)
        if(capture)require(capture->EndFrameCapture(RENDERDOC_DEVICEPOINTER_FROM_VKINSTANCE(g.instance),nullptr),"light frame GPU capture failed");
#endif
    }
    char last[32];std::snprintf(last,sizeof(last),"draw-%02u-color0.bin",unsigned(count-1));
    save(out/"native-color.bin",{static_cast<const std::byte*>(rb.mapped),before.size()});const auto expected=read(frame/last);save(out/"original-color.bin",expected);save(out/"before-color.bin",before);
    auto final=difference({static_cast<const std::byte*>(rb.mapped),before.size()},expected);
    const auto contribution=difference(before,expected);const bool visible=contribution.at("changed_rgb_channels").get<unsigned>()!=0;
    const Json result{{"scope","original illumination-pass replay only; reference update version must be independently authenticated"},{"reference",reference.value("reference",std::string("original Xbox diagnostic"))},{"device",properties.deviceName},{"begin_draw",begin_draw},{"end_draw_exclusive",count},{"non_seamless_cube_map",!std::getenv("HALO2_LIGHT_FRAME_SEAMLESS_CONTROL")},{"source_color_depth_stencil_seed_exact",true},{"draws",packets.size()},{"consumed_vertex_bank_word_differences",bank_errors},{"combiner_constant_word_differences",combiner_errors},{"validation_errors",validation_errors},{"reference_contribution_from_seed",contribution},{"reference_segment_visible",visible},{"per_draw",measurements},{"final",final}};std::ofstream(out/"report.json")<<result.dump(2)<<'\n';std::cout<<result.dump()<<'\n';require(validation_errors==0,"light frame validation errors");require(bank_errors==0&&combiner_errors==0,"light frame consumed source inputs differ");require(visible,"original segment changes no RGB pixels; visible illumination parity is unproven");require(final.at("changed_channels")==0,"light frame pixels differ; inspect report");
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
