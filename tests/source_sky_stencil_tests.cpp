#include "source_sky_stencil_gpu_fixture.hpp"
#include <bit>
#include <cmath>
#include <filesystem>
#include <sstream>
using namespace halo2_sky_stencil_fixture;
namespace {
const std::filesystem::path root=HALO2_SOURCE_DIR;
const auto proof=root/"analysis/multiplayer-shading/source-sky-stencil-prototype";
const auto originals=root/"analysis/multiplayer-shading/original20-sky-stencil-analysis";
std::vector<std::uint8_t> bytes(const std::filesystem::path &path) {
    std::ifstream f(path,std::ios::binary|std::ios::ate);require(bool(f),"sky fixture file missing");std::vector<std::uint8_t> b(std::size_t(f.tellg()));f.seekg(0);require(bool(f.read(reinterpret_cast<char*>(b.data()),b.size())),"sky fixture file read failed");return b;
}
std::vector<std::uint32_t> words(const std::filesystem::path &path) {
    auto b=bytes(path);require(b.size()%4==0,"sky word file size");std::vector<std::uint32_t> w(b.size()/4);std::memcpy(w.data(),b.data(),b.size());return w;
}
struct Fixture {
    unsigned event{};VkExtent2D extent{};SourceSkyStencilUniform uniform{};std::array<std::uint32_t,2> rotations{};
    std::vector<SourceSkyStencilVertex> vertices;std::vector<SourceSkyStencilPostGeometryVertex> gs_vertices;std::vector<std::uint32_t> indices,before,after;
    SourceSkyStencilDraw strip()const{return {vertices,indices,uniform,rotations};}
    SourceSkyStencilPostGeometryDraw passthrough()const{return {gs_vertices,uniform};}
};
Fixture load(unsigned event) {
    static_assert(std::endian::native==std::endian::little);
    Fixture f;f.event=event;
    auto b=bytes(proof/(std::to_string(event)+"-retained-strip.fixture"));require(b.size()>=196&&!std::memcmp(b.data(),"H2SKST01",8),"sky strip fixture tag");std::array<std::uint32_t,7> h;std::memcpy(h.data(),b.data()+8,28);require(h[0]==event&&h[1]==640&&h[2]==480,"sky strip fixture actual extent/event");f.extent={h[1],h[2]};f.rotations={h[5],h[6]};std::memcpy(&f.uniform,b.data()+36,160);const auto n=std::size_t(h[3]),i=std::size_t(h[4]);require(b.size()==196+n*32+i*4,"sky strip fixture exact bounds");f.vertices.resize(n);f.indices.resize(i);std::memcpy(f.vertices.data(),b.data()+196,n*32);std::memcpy(f.indices.data(),b.data()+196+n*32,i*4);
    auto g=bytes(proof/(std::to_string(event)+"-retained-gsout.fixture"));require(g.size()>=196&&!std::memcmp(g.data(),"H2SKGS01",8),"sky GSOut fixture tag");std::array<std::uint32_t,7> gh;std::memcpy(gh.data(),g.data()+8,28);require(gh[0]==event&&gh[1]==640&&gh[2]==480&&gh[4]==0&&g.size()==196+std::size_t(gh[3])*68,"sky GSOut fixture bounds");require(!std::memcmp(&f.uniform,g.data()+36,160),"sky paths source uniforms differ");f.gs_vertices.resize(gh[3]);std::memcpy(f.gs_vertices.data(),g.data()+196,f.gs_vertices.size()*68);
    f.before=words(originals/(std::to_string(event)+"-before-packed-d24s8.bin"));f.after=words(originals/(std::to_string(event)+"-after-packed-d24s8.bin"));require(f.before.size()==640*480&&f.after.size()==640*480,"sky original D24S8 sizes");validate_source_sky_stencil_draw(f.strip());validate_source_sky_stencil_draw(f.passthrough());return f;
}
unsigned rejected;
template<class F> void reject(F fn){try{fn();}catch(const FormatError&){++rejected;return;}throw std::runtime_error("sky invalid input was accepted");}
void cpu(std::array<Fixture,3> &fixtures) {
    require(fixtures[0].after==fixtures[1].before&&fixtures[1].after==fixtures[2].before,"original sky draw depth/stencil chronology mismatch");
    auto d=fixtures[0].strip();reject([&]{auto x=d;x.vertices={};validate_source_sky_stencil_draw(x);});reject([&]{auto x=d;x.indices=x.indices.first(2);validate_source_sky_stencil_draw(x);});
    std::vector<std::uint32_t> iv(d.indices.begin(),d.indices.end());iv[0]=d.vertices.size();reject([&]{auto x=d;x.indices=iv;validate_source_sky_stencil_draw(x);});iv[0]=0xffffffff;reject([&]{auto x=d;x.indices=iv;validate_source_sky_stencil_draw(x);});
    auto vv=fixtures[0].vertices;vv[0].clip[1]=std::numeric_limits<float>::infinity();reject([&]{auto x=d;x.vertices=vv;validate_source_sky_stencil_draw(x);});
    reject([&]{auto x=d;x.uniform.clip_range[2]=-1;validate_source_sky_stencil_draw(x);});reject([&]{auto x=d;x.uniform.clip_range[2]=16777216;validate_source_sky_stencil_draw(x);});reject([&]{auto x=d;x.uniform.clip_range[3]=NAN;validate_source_sky_stencil_draw(x);});reject([&]{auto x=d;x.uniform.depth_factor_offset[0]=INFINITY;validate_source_sky_stencil_draw(x);});reject([&]{auto x=d;x.uniform.surface_scale[0]=0;validate_source_sky_stencil_draw(x);});reject([&]{auto x=d;x.rotations[1]=3;validate_source_sky_stencil_draw(x);});
    auto p=fixtures[0].passthrough();reject([&]{auto x=p;x.vertices=x.vertices.first(x.vertices.size()-1);validate_source_sky_stencil_draw(x);});auto gv=fixtures[0].gs_vertices;gv[1].window0[1]+=1;reject([&]{auto x=p;x.vertices=gv;validate_source_sky_stencil_draw(x);});gv=fixtures[0].gs_vertices;gv[0].tri_mz=NAN;reject([&]{auto x=p;x.vertices=gv;validate_source_sky_stencil_draw(x);});
    std::cout<<"Source sky stencil CPU: three retained strip/GSOut fixtures, exact source chronology, "<<rejected<<" rejected inputs\n";
}
struct Metric {std::string label;unsigned event{},depth_diff{},stencil_diff{},source_only{},native_only{},changed{},expected_changed{};};
Metric measure(const Fixture &f,const std::vector<std::uint32_t> &result,std::string label,const std::filesystem::path &out) {
    Metric m{std::move(label),f.event};for(std::size_t i=0;i<result.size();++i){m.depth_diff+=(result[i]&0xffffff)!=(f.before[i]&0xffffff);m.stencil_diff+=(result[i]>>24)!=(f.after[i]>>24);const bool actual=(result[i]>>24)!=(f.before[i]>>24),expected=(f.after[i]>>24)!=(f.before[i]>>24);m.changed+=actual;m.expected_changed+=expected;m.source_only+=expected&&!actual;m.native_only+=actual&&!expected;}
    std::ofstream raw(out/(m.label+"-"+std::to_string(f.event)+"-packed-d24s8.bin"),std::ios::binary);raw.write(reinterpret_cast<const char*>(result.data()),result.size()*4);return m;
}
void gpu(std::array<Fixture,3> &fixtures,const std::filesystem::path &out) {
    std::filesystem::create_directories(out);std::vector<Metric> metrics;
    {
        SkyGpu g;if(!g.initialize())throw std::runtime_error("sky stencil required validation/features unavailable");const VkExtent2D extent{640,480};auto image=g.image(extent,VK_FORMAT_D24_UNORM_S8_UINT,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT);const auto meta=metadata(g,image,extent);SourceSkyStencilContext context{g.physical,g.device,g.family,HALO2_SKY_STENCIL_SHADER_DIR,true,true};SourceSkyStencil p(context);bool initialized=false;
        // Image and command guards precede any producer command mutation.
        auto draw=fixtures[0].strip();p.prepare(meta,draw);
        reject([&]{auto s=meta;s.image={};p.prepare(s,draw);});reject([&]{auto s=meta;s.owner={};p.prepare(s,draw);});reject([&]{auto s=meta;s.format=VK_FORMAT_D32_SFLOAT;p.prepare(s,draw);});reject([&]{auto s=meta;s.extent.width=0;p.prepare(s,draw);});reject([&]{auto s=meta;s.mip_levels=2;p.prepare(s,draw);});reject([&]{auto s=meta;s.array_layers=2;p.prepare(s,draw);});reject([&]{auto s=meta;s.samples=VK_SAMPLE_COUNT_4_BIT;p.prepare(s,draw);});reject([&]{auto s=meta;s.usage=VK_IMAGE_USAGE_TRANSFER_SRC_BIT;p.prepare(s,draw);});reject([&]{auto s=meta;s.layout=VK_IMAGE_LAYOUT_UNDEFINED;p.prepare(s,draw);});reject([&]{auto s=meta;s.sharing=VK_SHARING_MODE_CONCURRENT;p.prepare(s,draw);});reject([&]{auto s=meta;s.type=VK_IMAGE_TYPE_3D;p.prepare(s,draw);});reject([&]{auto s=meta;s.tiling=VK_IMAGE_TILING_LINEAR;p.prepare(s,draw);});reject([&]{auto s=meta;s.queue_family=~0u;p.prepare(s,draw);});reject([&]{auto s=meta;s.create_flags=VK_IMAGE_CREATE_SPARSE_BINDING_BIT;p.prepare(s,draw);});
        for(bool gs:{false,true})for(auto &f:fixtures) {
            if(gs)p.prepare(meta,f.passthrough());else p.prepare(meta,f.strip());Submission s(g);upload_depth(g,image,extent,f.before,initialized);initialized=true;
            if(!gs&&f.event==8097){SourceSkyStencilRecord r{g.device,g.command,s.fence,g.family,true,true};reject([&]{auto x=r;x.command={};p.record(x);});reject([&]{auto x=r;x.completion={};p.record(x);});reject([&]{auto x=r;x.command_owner={};p.record(x);});reject([&]{auto x=r;x.command_queue_family=~0u;p.record(x);});reject([&]{auto x=r;x.outside_render_pass=false;p.record(x);});reject([&]{auto x=r;x.exclusive_access=false;p.record(x);});}
            s.record(p);reject([&]{p.prepare(meta,f.strip());});reject([&]{p.record({g.device,g.command,s.fence,g.family,true,true});});reject([&]{p.retire(s.fence);});reject([&]{p.retire({});});auto b=read_depth(g,image,extent);s.finish();metrics.push_back(measure(f,packed(b,f.before.size()),gs?"gsout":"vsout-strip",out));
        }
        // One fence covers the actual A -> B -> moon chronology, including
        // all intermediate copies. Three producers retain their uploads.
        for(bool gs:{false,true}) {
            SourceSkyStencil a(context),b(context),c(context);std::array<SourceSkyStencil*,3> producers{&a,&b,&c};
            for(unsigned i=0;i<3;++i){if(gs)producers[i]->prepare(meta,fixtures[i].passthrough());else producers[i]->prepare(meta,fixtures[i].strip());}
            Submission s(g);upload_depth(g,image,extent,fixtures[0].before,true);std::array<Buffer,3> reads;
            for(unsigned i=0;i<3;++i){s.record(*producers[i]);reads[i]=read_depth(g,image,extent);}s.finish();for(unsigned i=0;i<3;++i)metrics.push_back(measure(fixtures[i],packed(reads[i],fixtures[i].before.size()),gs?"chain-gsout":"chain-vsout-strip",out));
        }
        // Negative coverage controls use actual retained geometry. No masks
        // or fullscreen geometry are supplied to the producer.
        auto empty=fixtures[0];empty.uniform.regions={};empty.after=empty.before;p.prepare(meta,empty.strip());{Submission s(g);upload_depth(g,image,extent,empty.before,true);s.record(p);auto b=read_depth(g,image,extent);s.finish();auto m=measure(empty,packed(b,empty.before.size()),"empty-region-control",out);require(!m.depth_diff&&!m.stencil_diff,"empty source region wrote depth/stencil");metrics.push_back(m);}
        auto fail=fixtures[0];for(std::size_t i=0;i<fail.before.size();++i)fail.before[i]=std::uint32_t(i%256)<<24;fail.after=fail.before;p.prepare(meta,fail.passthrough());{Submission s(g);upload_depth(g,image,extent,fail.before,true);s.record(p);auto b=read_depth(g,image,extent);s.finish();auto m=measure(fail,packed(b,fail.before.size()),"lequal-fail-allbyte-control",out);require(!m.depth_diff&&!m.stencil_diff,"LEQUAL failure changed depth/stencil");metrics.push_back(m);}
        auto pattern=fixtures[0];for(std::size_t i=0;i<pattern.before.size();++i){pattern.before[i]=(pattern.before[i]&0xffffff)|(std::uint32_t(i%256)<<24);pattern.after[i]=(pattern.after[i]&0xffffff)|((fixtures[0].after[i]>>24)==255?0xff000000u:std::uint32_t(i%256)<<24);}p.prepare(meta,pattern.passthrough());{Submission s(g);upload_depth(g,image,extent,pattern.before,true);s.record(p);auto b=read_depth(g,image,extent);s.finish();metrics.push_back(measure(pattern,packed(b,pattern.before.size()),"always-replace-allbyte-control",out));}
        auto reuse=fixtures[0];reuse.before=reuse.after;p.prepare(meta,reuse.passthrough());{Submission s(g);upload_depth(g,image,extent,reuse.before,true);s.record(p);auto b=read_depth(g,image,extent);s.finish();metrics.push_back(measure(reuse,packed(b,reuse.before.size()),"reuse-gsout",out));}{Submission s(g);s.record(p);auto b=read_depth(g,image,extent);s.finish();metrics.push_back(measure(reuse,packed(b,reuse.before.size()),"reuse-gsout-repeat",out));}
    }
    std::ofstream report(out/"report.json");report<<"{\n  \"scope\": \"Isolated retained original20 sky stencil-only diagnostic; no sky color or authored native vertex production\",\n  \"validation_errors\": "<<validation_errors<<",\n  \"rejected_inputs_and_ownership\": "<<rejected<<",\n  \"metrics\": [\n";
    unsigned mismatches=0;for(std::size_t i=0;i<metrics.size();++i){const auto&m=metrics[i];mismatches+=m.depth_diff+m.stencil_diff;report<<"    {\"path\": \""<<m.label<<"\", \"event\": "<<m.event<<", \"depth_differences\": "<<m.depth_diff<<", \"stencil_differences\": "<<m.stencil_diff<<", \"changed_pixels\": "<<m.changed<<", \"original_changed_pixels\": "<<m.expected_changed<<", \"source_only_changes\": "<<m.source_only<<", \"native_only_changes\": "<<m.native_only<<"}"<<(i+1==metrics.size()?"\n":",\n");std::cout<<m.label<<' '<<m.event<<": depth differences "<<m.depth_diff<<", stencil differences "<<m.stencil_diff<<", changed "<<m.changed<<", original "<<m.expected_changed<<'\n';}report<<"  ]\n}\n";report.close();require(!validation_errors,"sky stencil Vulkan validation errors");require(!mismatches,"sky stencil exact original byte comparison failed; inspect report");
}
}
int main(int argc,char **argv) {
    try{std::array<Fixture,3> fixtures{load(8097),load(8102),load(8110)};cpu(fixtures);if(argc==2&&std::string(argv[1])=="--cpu")return 0;if(argc==3&&std::string(argv[1])=="--gpu"){gpu(fixtures,argv[2]);return 0;}throw std::runtime_error("usage: --cpu | --gpu output-directory");}
    catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
