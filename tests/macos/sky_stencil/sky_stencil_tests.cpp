#include "halo2/source_sky_stencil.hpp"
#include "../depth_storage/gpu.hpp"
#include <bit>
#include <cstring>
#include <map>
#include <nlohmann/json.hpp>
using namespace halo2;
using nlohmann::json;
namespace {
constexpr VkExtent2D extent{640,480};
constexpr unsigned pixels=extent.width*extent.height;
constexpr VkImageAspectFlags aspects=VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT;
constexpr VkImageUsageFlags usage=VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT;
struct Fixture {
    unsigned event{};SourceSkyStencilUniform uniform{};std::array<std::uint32_t,2> rotations{};
    std::vector<SourceSkyStencilVertex> vertices;std::vector<SourceSkyStencilPostGeometryVertex> post;
    std::vector<std::uint32_t> indices,before,after;
    SourceSkyStencilDraw strip()const{return {vertices,indices,uniform,rotations};}
    SourceSkyStencilPostGeometryDraw passthrough()const{return {post,uniform};}
};
std::vector<std::uint8_t> bytes(const std::filesystem::path &path) {
    std::ifstream file(path,std::ios::binary|std::ios::ate);require(file&&file.tellg()>0,"Missing authenticated sky reference: "+path.string());std::vector<std::uint8_t> b(std::size_t(file.tellg()));file.seekg(0);require(bool(file.read(reinterpret_cast<char*>(b.data()),b.size())),"Truncated sky reference");return b;
}
Fixture load(unsigned event) {
    static_assert(std::endian::native==std::endian::little);
    const std::filesystem::path root=SKY_REFERENCE_ROOT;const auto proof=root/"analysis/multiplayer-shading/source-sky-stencil-prototype";const auto original=root/"analysis/multiplayer-shading/original20-sky-stencil-analysis";
    Fixture f;f.event=event;const auto name=std::to_string(event);
    auto b=bytes(proof/(name+"-retained-strip.fixture"));require(b.size()>=196&&!std::memcmp(b.data(),"H2SKST01",8),"Sky strip identity");std::array<std::uint32_t,7> h{};std::memcpy(h.data(),b.data()+8,28);require(h[0]==event&&h[1]==extent.width&&h[2]==extent.height&&b.size()==196+std::size_t(h[3])*32+std::size_t(h[4])*4,"Sky strip exact bounds");std::memcpy(&f.uniform,b.data()+36,160);f.rotations={h[5],h[6]};f.vertices.resize(h[3]);f.indices.resize(h[4]);std::memcpy(f.vertices.data(),b.data()+196,f.vertices.size()*32);std::memcpy(f.indices.data(),b.data()+196+f.vertices.size()*32,f.indices.size()*4);
    b=bytes(proof/(name+"-retained-gsout.fixture"));require(b.size()>=196&&!std::memcmp(b.data(),"H2SKGS01",8),"Sky post-geometry identity");std::memcpy(h.data(),b.data()+8,28);require(h[0]==event&&h[1]==extent.width&&h[2]==extent.height&&h[4]==0&&b.size()==196+std::size_t(h[3])*68&&!std::memcmp(&f.uniform,b.data()+36,160),"Sky post-geometry exact bounds");f.post.resize(h[3]);std::memcpy(f.post.data(),b.data()+196,f.post.size()*68);
    for(auto phase:{"before","after"}){auto raw=bytes(original/(name+"-"+phase+"-packed-d24s8.bin"));require(raw.size()==pixels*4,"Original packed depth size");auto &target=std::string(phase)=="before"?f.before:f.after;target.resize(pixels);std::memcpy(target.data(),raw.data(),raw.size());}
    validate_source_sky_stencil_draw(f.strip());validate_source_sky_stencil_draw(f.passthrough());return f;
}
struct Submission {
    Gpu &g;VkFence fence{};bool submitted{},completed{};std::vector<SourceSkyStencil*> producers;
    explicit Submission(Gpu &gpu):g(gpu){VkFenceCreateInfo c{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};check(vkCreateFence(g.device,&c,nullptr,&fence));g.begin();}
    void record(SourceSkyStencil &p){p.record({g.device,g.command,fence,g.family,true,true});producers.push_back(&p);}
    void finish(){check(vkEndCommandBuffer(g.command));VkSubmitInfo s{VK_STRUCTURE_TYPE_SUBMIT_INFO};s.commandBufferCount=1;s.pCommandBuffers=&g.command;check(vkQueueSubmit(g.queue,1,&s,fence));submitted=true;check(vkWaitForFences(g.device,1,&fence,VK_TRUE,UINT64_MAX));for(auto p:producers)p->retire(fence);completed=true;}
    ~Submission(){if(!completed&&!producers.empty()){if(!submitted){vkEndCommandBuffer(g.command);VkSubmitInfo s{VK_STRUCTURE_TYPE_SUBMIT_INFO};s.commandBufferCount=1;s.pCommandBuffers=&g.command;if(vkQueueSubmit(g.queue,1,&s,fence)==VK_SUCCESS)submitted=true;}if(submitted&&vkWaitForFences(g.device,1,&fence,VK_TRUE,UINT64_MAX)==VK_SUCCESS)for(auto p:producers)try{p->retire(fence);}catch(...){}}if(fence)vkDestroyFence(g.device,fence,nullptr);}
};
struct ImageAccess {
    Gpu &g;Gpu::Image image;Gpu::Buffer upload,read;bool carrier{},initialized{};
    ImageAccess(Gpu &gpu,bool use_carrier):g(gpu),carrier(use_carrier) {image=g.image(extent,carrier?VK_FORMAT_D32_SFLOAT_S8_UINT:VK_FORMAT_D24_UNORM_S8_UINT,usage);upload=g.buffer(pixels*5);read=g.buffer(pixels*5);}
    SourceWeatherDepthAliasImage metadata()const{SourceWeatherDepthAliasImage m{g.device,image.handle,carrier?VK_FORMAT_D32_SFLOAT_S8_UINT:VK_FORMAT_D24_UNORM_S8_UINT,extent,usage,VK_IMAGE_LAYOUT_GENERAL,VK_SAMPLE_COUNT_1_BIT,1,1,g.family,VK_SHARING_MODE_EXCLUSIVE};m.depth_encoding=carrier?SourceDepthStorageEncoding::d32_integer_d24:SourceDepthStorageEncoding::native_d24;return m;}
    void upload_words(std::span<const std::uint32_t> words) {
        require(words.size()==pixels,"Source word count");auto *d=static_cast<std::uint32_t*>(upload.mapped);auto *s=static_cast<std::uint8_t*>(upload.mapped)+pixels*4;for(unsigned i=0;i<pixels;++i){auto n=words[i]&0xffffff;d[i]=carrier?source_d24_carrier_word(n):n;s[i]=words[i]>>24;}
        barrier(g.command,image.handle,aspects,initialized?VK_IMAGE_LAYOUT_GENERAL:VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);auto depth=region(extent,VK_IMAGE_ASPECT_DEPTH_BIT),stencil=region(extent,VK_IMAGE_ASPECT_STENCIL_BIT);stencil.bufferOffset=pixels*4;vkCmdCopyBufferToImage(g.command,upload.handle,image.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&depth);vkCmdCopyBufferToImage(g.command,upload.handle,image.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&stencil);barrier(g.command,image.handle,aspects,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_GENERAL);initialized=true;
    }
    void read_words(Gpu::Buffer target) {
        barrier(g.command,image.handle,aspects,VK_IMAGE_LAYOUT_GENERAL,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);auto depth=region(extent,VK_IMAGE_ASPECT_DEPTH_BIT),stencil=region(extent,VK_IMAGE_ASPECT_STENCIL_BIT);stencil.bufferOffset=pixels*4;vkCmdCopyImageToBuffer(g.command,image.handle,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,target.handle,1,&depth);vkCmdCopyImageToBuffer(g.command,image.handle,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,target.handle,1,&stencil);barrier(g.command,image.handle,aspects,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_IMAGE_LAYOUT_GENERAL);VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER};host.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;host.dstAccessMask=VK_ACCESS_HOST_READ_BIT;vkCmdPipelineBarrier(g.command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&host,0,nullptr,0,nullptr);
    }
    std::vector<std::uint32_t> packed(Gpu::Buffer b)const{std::vector<std::uint32_t> result(pixels);auto *d=static_cast<const std::uint32_t*>(b.mapped);auto *s=static_cast<const std::uint8_t*>(b.mapped)+pixels*4;for(unsigned i=0;i<pixels;++i)result[i]=(carrier?source_d24_carrier_integer(d[i]):d[i]&0xffffff)|(std::uint32_t(s[i])<<24);return result;}
};
void measure(json &metrics,std::string path,unsigned event,std::span<const std::uint32_t> actual,std::span<const std::uint32_t> before,std::span<const std::uint32_t> expected) {
    unsigned depth_diff=0,stencil_diff=0,changed=0;json examples=json::array();
    for(unsigned i=0;i<pixels;++i){depth_diff+=(actual[i]&0xffffff)!=(before[i]&0xffffff);stencil_diff+=(actual[i]>>24)!=(expected[i]>>24);changed+=(actual[i]>>24)!=(before[i]>>24);if(actual[i]!=expected[i]&&examples.size()<8)examples.push_back({{"x",i%extent.width},{"y",i/extent.width},{"actual",actual[i]},{"expected",expected[i]}});}
    metrics.push_back({{"path",path},{"event",event},{"depth_differences",depth_diff},{"stencil_differences",stencil_diff},{"changed_pixels",changed},{"first_differences",examples}});
}
using Controls=std::map<std::string,std::vector<std::uint32_t>>;
void cohort(Gpu &g,std::array<Fixture,3> &fixtures,bool carrier,bool geometry,json &metrics,Controls &controls) {
    const auto label=std::string(carrier?"d32-":"d24-")+(geometry?"geometry":"instanced");
    ImageAccess access(g,carrier);const auto meta=access.metadata();SourceSkyStencilContext context{g.physical,g.device,g.family,DEPTH_SHADER_DIR,geometry,true};context.depth_encoding=meta.depth_encoding;SourceSkyStencil producer(context);
    const auto run=[&](const Fixture &f,bool post){if(post)producer.prepare(meta,f.passthrough());else producer.prepare(meta,f.strip());Submission s(g);access.upload_words(f.before);s.record(producer);access.read_words(access.read);s.finish();return access.packed(access.read);};
    bool metadata_rejected=false;try{auto wrong=meta;wrong.depth_encoding=carrier?SourceDepthStorageEncoding::native_d24:SourceDepthStorageEncoding::d32_integer_d24;producer.prepare(wrong,fixtures[0].strip());}catch(const FormatError&){metadata_rejected=true;}require(metadata_rejected,"Sky format/encoding metadata mismatch admitted");
    for(bool post:{false,true})for(auto &f:fixtures){auto result=run(f,post);measure(metrics,label+(post?"-gsout":"-strip"),f.event,result,f.before,f.after);}
    for(bool post:{false,true}) {
        SourceSkyStencil a(context),b(context),c(context);std::array<SourceSkyStencil*,3> producers{&a,&b,&c};std::array<Gpu::Buffer,3> readbacks{g.buffer(pixels*5),g.buffer(pixels*5),g.buffer(pixels*5)};
        for(unsigned i=0;i<3;++i){if(post)producers[i]->prepare(meta,fixtures[i].passthrough());else producers[i]->prepare(meta,fixtures[i].strip());}
        Submission s(g);access.upload_words(fixtures[0].before);for(unsigned i=0;i<3;++i){s.record(*producers[i]);access.read_words(readbacks[i]);}s.finish();for(unsigned i=0;i<3;++i){auto result=access.packed(readbacks[i]);measure(metrics,label+(post?"-chain-gsout":"-chain-strip"),fixtures[i].event,result,fixtures[i].before,fixtures[i].after);}
    }
    auto empty=fixtures[0];empty.uniform.regions={};empty.after=empty.before;auto result=run(empty,false);measure(metrics,label+"-empty-region",empty.event,result,empty.before,empty.after);
    auto fail=fixtures[0];for(unsigned i=0;i<pixels;++i)fail.before[i]=std::uint32_t(i%256)<<24;fail.after=fail.before;result=run(fail,true);measure(metrics,label+"-lequal-fail-all-stencil-bytes",fail.event,result,fail.before,fail.after);
    auto zero=fixtures[0];zero.uniform.clip_range[2]=zero.uniform.clip_range[3]=-0.0f;
    for(unsigned i=0;i<pixels;++i)zero.before[i]=std::uint32_t(i%256)<<24;
    auto expected_zero=run(zero,true);result=run(zero,false);
    require(expected_zero!=zero.before,"Zero-clamp control covered no stencil pixels");
    measure(metrics,label+"-signed-zero-clamp",zero.event,result,zero.before,expected_zero);
    auto degenerate=fixtures[0];degenerate.indices.insert(degenerate.indices.begin(),{degenerate.indices[0],degenerate.indices[0]});result=run(degenerate,false);measure(metrics,label+"-leading-degenerates",degenerate.event,result,degenerate.before,degenerate.after);
    if(!controls.empty()||(!carrier&&geometry))for(auto &source:fixtures)for(unsigned even=0;even<3;++even)for(unsigned odd=0;odd<3;++odd) {
        auto f=source;f.rotations={even,odd};result=run(f,false);const auto key=std::to_string(f.event)+"-"+std::to_string(even)+std::to_string(odd);
        if(!carrier&&geometry)controls[key]=result;
        measure(metrics,label+"-rotation-"+std::to_string(even)+std::to_string(odd),f.event,result,f.before,controls.at(key));
    }
}
}
int main(int argc,char **argv) {
    json report;int status=0;report["schema"]="H2_MACOS_SKY_STENCIL_V1";report["metrics"]=json::array();
    try {
        require(argc==2,"Expected small report path");std::array<Fixture,3> fixtures{load(8097),load(8102),load(8110)};require(fixtures[0].after==fixtures[1].before&&fixtures[1].after==fixtures[2].before,"Original sky chronology");
        {
            Gpu g;
#ifdef __APPLE__
            constexpr bool geometry=false;
#else
            constexpr bool geometry=true;
#endif
            g.initialize(true,geometry);report["device"]=g.properties.deviceName;report["geometry_feature_enabled"]=geometry;report["depth_clamp_enabled"]=true;report["validation_layer_active"]=g.validation;report["synchronization_validation_active"]=g.synchronization_validation;Controls controls;
            VkImageFormatProperties native{};const auto supported=vkGetPhysicalDeviceImageFormatProperties(g.physical,VK_FORMAT_D24_UNORM_S8_UINT,VK_IMAGE_TYPE_2D,VK_IMAGE_TILING_OPTIMAL,usage,0,&native)==VK_SUCCESS;report["native_d24_supported"]=supported;
            if(geometry&&supported)cohort(g,fixtures,false,true,report["metrics"],controls);
            if(supported)cohort(g,fixtures,false,false,report["metrics"],controls);
            if(geometry)cohort(g,fixtures,true,true,report["metrics"],controls);
            cohort(g,fixtures,true,false,report["metrics"],controls);
        }
        unsigned mismatches=0;for(auto &m:report["metrics"])mismatches+=m["depth_differences"].get<unsigned>()+m["stencil_differences"].get<unsigned>();report["byte_comparison_differences"]=mismatches;require(!mismatches,"Sky source depth/stencil byte comparison failed");require(!validation_errors,"Vulkan validation errors");report["passed"]=true;
    }catch(const std::exception &e){report["passed"]=false;report["error"]=e.what();std::cerr<<e.what()<<'\n';status=1;}
    report["validation_errors"]=validation_errors;
    if(argc==2){std::ofstream output(argv[1]);output<<report.dump(2)<<'\n';if(!output)status=1;}
    std::cout<<"Sky stencil cases: "<<report["metrics"].size()<<", passed: "<<report["passed"]<<'\n';return status;
}
