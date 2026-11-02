#include "halo2/path_tracing/material_surface.hpp"
#include "halo2/path_tracing/surface.hpp"
#include "halo2/source_texture_filtering.hpp"
#include "halo2/source_vertex_inputs.hpp"
#include "halo2/texture.hpp"
#include "halo2/vertex_program.hpp"
#include "../scalar_combiners.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>

using namespace halo2;
namespace pt=halo2::path_tracing;
namespace {
using Json=nlohmann::json;
struct Images {
    const Material& material;
    pt::UnlitUv uv;
    std::array<Texture,2> images;
    std::array<TextureMip,2> mips;
    explicit Images(const Material& m):material(m),uv(*pt::qualified_unlit_uv(m)) {
        const Texture* textures[]{&m.diffuse,&m.detail};
        for(unsigned s=0;s<2;++s) {
            images[s]=m.animated_textures[s].empty()?*textures[s]:m.animated_textures[s].at(m.texture_frames[s]);
            if(images[s].format>=14 && images[s].format<=16)images[s]=prepare_xemu_bc_texture(images[s]);
            if(images[s].format!=11)throw FormatError("Opaque source oracle requires authored RGBA8/BC images");
            if(images[s].mips.empty())images[s].mips.push_back({images[s].width,images[s].height,0,
                std::size_t(images[s].width)*images[s].height*4});
            unsigned mip=std::min(m.texture_states[s].max_mip,unsigned(images[s].mips.size()-1));
            while(mip+1<images[s].mips.size() && (images[s].mips[mip].width>128 || images[s].mips[mip].height>128))++mip;
            mips[s]=images[s].mips[mip];
        }
    }
    ShaderVector fetch(unsigned stage,int x,int y)const {
        const auto& mip=mips[stage];const auto& sampler=material.texture_states[stage];
        const auto address=[](int p,unsigned extent,unsigned mode) {
            if(mode==3)return std::clamp(p,0,int(extent)-1);
            if(mode==4)return p<0 || p>=int(extent)?-1:p;
            if(mode==5)return std::min(p<0?-p-1:p,int(extent)-1);
            const int period=int(extent)*(mode==2?2:1);int at=p%period;if(at<0)at+=period;
            return mode==2 && at>=int(extent)?period-at-1:at;
        };
        x=address(x,mip.width,sampler.address_u);y=address(y,mip.height,sampler.address_v);
        if(x<0 || y<0)return pt::surface_detail::argb(sampler.border_color);
        const auto at=mip.offset+(std::size_t(y)*mip.width+unsigned(x))*4;ShaderVector result;
        for(unsigned c=0;c<4;++c)result[c]=float(std::to_integer<unsigned>(images[stage].pixels.at(at+c)))/255;
        return result;
    }
    std::array<ShaderVector,4> samples(Vec2 raw)const {
        const auto coords=pt::unlit_uv(uv,raw);std::array<ShaderVector,4> result{};
        for(unsigned s=0;s<2;++s) {
            const auto& mip=mips[s];const float x=coords[s].x*mip.width,y=coords[s].y*mip.height;
            if(source_texture_filtering(material.texture_states[s]).mag_filter==1)result[s]=fetch(s,int(std::floor(x)),int(std::floor(y)));
            else {
                const float xx=x-.5f,yy=y-.5f;const int left=int(std::floor(xx)),top=int(std::floor(yy));
                const float fx=xx-left,fy=yy-top;const auto a=fetch(s,left,top),b=fetch(s,left+1,top),c=fetch(s,left,top+1),d=fetch(s,left+1,top+1);
                for(unsigned channel=0;channel<4;++channel)
                    result[s][channel]=(a[channel]*(1-fx)+b[channel]*fx)*(1-fy)+(c[channel]*(1-fx)+d[channel]*fx)*fy;
            }
        }
        return result;
    }
};
ShaderVector diffuse_oracle(std::array<std::uint32_t,60> words,std::array<ShaderVector,4> samples) {
    // The authenticated opaque perpixel program adds a separate baked
    // specular branch through RGB constant15. Isolate diffuse only, retaining
    // its original alpha byte. Neutral T3=.5 cancels this program's 2*T3
    // lightmap multiplier; oD1.a=0 removes the baked incident correction.
    words[15]&=0xff000000u;
    samples[3]={.5f,.5f,.5f,1};
    return halo2_test::scalar_combiners(words,samples,{0,0,0,1},{.5f,.5f,1,0});
}
ShaderVector diffuse_oracle(const Material& material,std::array<ShaderVector,4> samples) {
    return diffuse_oracle(material.forward_program.words,samples);
}
float rgb_error(const ShaderVector& a,const ShaderVector& b) {
    float e{};for(unsigned c=0;c<3;++c)e=std::max(e,std::abs(a[c]-b[c]));return e;
}
unsigned control_cases{};
void require(bool value,const char* message){++control_cases;if(!value)throw FormatError(message);}
Texture solid(unsigned alpha){return {1,1,11,{std::byte{64},std::byte{128},std::byte{32},std::byte(alpha)}};}
void controls(Material material) {
    const auto qualified=pt::qualified_surface(material);
    require(qualified && qualified->family==pt::SurfaceFamily::OpaqueDetail,"Actual opaque source lost its fixed diffuse family");
    for(unsigned word=0;word<60;++word) {
        auto changed=material;changed.forward_program.words[word]^=1u;
        const bool color=(word>=10 && word<=25) || word==43 || word==44;
        require(pt::qualified_opaque_detail(changed)==color,"Opaque source confused authored color data with a pixel control");
    }
    auto changed=material;changed.forward_program.words[15]^=0x00ffffffu;
    require(pt::qualified_opaque_detail(changed),"Separate source specular RGB changed unlit diffuse qualification");
    changed=material;changed.forward_raster.alpha_test=1;
    require(!pt::qualified_surface(changed),"Opaque representative alpha was admitted for raster alpha testing");
    changed=material;changed.forward_blend=1;
    require(!pt::qualified_surface(changed),"Opaque representative alpha was admitted for blending");
    for(unsigned stage:{2u,3u}) {
        changed=material;changed.texture_states[stage].alpha_kill|=4u;
        require(!pt::qualified_surface(changed),"Opaque diffuse adapter silently omitted an unused-stage raw alpha owner");
    }
    material.animated_textures={};material.diffuse=solid(64);material.detail=solid(255);
    for(auto& state:material.texture_states)state.alpha_kill=0;
    std::array<Vertex,3> vertices{};vertices[1].position={1,0,0};vertices[2].position={0,1,0};
    for(auto& vertex:vertices){vertex.normal={0,0,1};vertex.color={1,1,1};}
    auto scene=pt::build_scene(vertices,std::span(&material,1));
    const auto result=pt::material_surface(scene,0,{.25f,.25f});
    require(result && (*result)[3]==64.f/255,"Opaque surface changed the existing T0 alpha representative");
    const auto size=scene.pixels.size();pt::refresh_materials(scene,std::span(&material,1));
    require(scene.pixels.size()==size,"Unchanged opaque source allocated duplicate resident records");
    const auto pointer=std::bit_cast<std::uint32_t>(scene.materials[0].base[3]);const auto key=scene.material_image_keys[0];
    auto animated=material;std::mt19937 random(0xc010a11u);
    for(unsigned frame=0;frame<16;++frame) {
        for(unsigned word=10;word<=25;++word)animated.forward_program.words[word]=random();
        animated.forward_program.words[43]=random();animated.forward_program.words[44]=random();
        pt::refresh_materials(scene,std::span(&animated,1));
        require(scene.pixels.size()==size && std::bit_cast<std::uint32_t>(scene.materials[0].base[3])==pointer && scene.material_image_keys[0]==key,
            "Color-only opaque source animation changed its resident image key or storage");
    }
    for(unsigned stage:{0u,1u}) {
        changed=material;changed.texture_states[stage].alpha_kill=4;
        (stage==0?changed.diffuse:changed.detail)=solid(0);
        pt::refresh_materials(scene,std::span(&changed,1));
        require(!pt::material_surface(scene,0,{.25f,.25f}),"Opaque surface discarded an authored raw texture alpha kill");
    }
    changed=material;changed.detail=solid(128);pt::refresh_materials(scene,std::span(&changed,1));
    const auto refreshed=pt::material_surface(scene,0,{.25f,.25f});
    require(refreshed && (*refreshed)[3]==64.f/255,"Animated opaque detail refresh changed primary alpha owner");
    changed.forward_program.words[34]^=1u;pt::refresh_materials(scene,std::span(&changed,1));
    require(!(scene.materials[0].texture[3]&pt::surface_flag),"Removed opaque source retained its resident family");
}
}
int main(int argc,char** argv)try {
    if(argc!=6)throw FormatError("Supply original/prepared Coagulation, original/prepared Lockout and report path");
    for(unsigned n=1;n<=4;++n)if(!std::filesystem::is_regular_file(argv[n]))return 77;
    Json report{{"passed",false},{"cohorts",Json::array()}};
    std::mt19937 random(0x1a1bed0u);std::uniform_real_distribution<float> channel(0,1),uv(-2,2);
    unsigned total{},source_cases{},atlas_cases{},vertex_cases{};float maximum_source_error{},maximum_atlas_error{},maximum_vertex_error{};
    for(unsigned n=1;n<=4;++n) {
        Cache cache(argv[n]);const auto mesh=cache.load_bsp(0);Json materials=Json::array();
        for(unsigned slot=0;slot<mesh.materials.size();++slot) {
            const auto& material=mesh.materials[slot];
            if(!pt::surface_detail::controls(material.forward_program,pt::surface_detail::opaque_detail))continue;
            if(!pt::qualified_unlit_uv(material))throw FormatError("Actual Lockout opaque source lost its authenticated UV family");
            std::array<Vertex,3> vertices{};vertices[1].position={1,0,0};vertices[2].position={0,1,0};
            for(auto& v:vertices){v.normal={0,0,1};v.color={1,1,1};}
            const auto scene=pt::build_scene(vertices,std::span(&material,1));Images images(material);
            float source_error{},atlas_error{},vertex_error{};Json examples=Json::array();
            const auto vertex_program=decode_vertex_program(material.vertex_program);unsigned sampled_vertices{};
            for(const auto& vertex:mesh.vertices) {
                if(vertex.material!=slot || sampled_vertices==32)continue;
                if(!material.vertex_declaration)throw FormatError("Opaque source oracle requires the actual native declaration");
                VertexProgramState state;state.attributes=source_world_vertex_inputs(*material.vertex_declaration,
                    vertex,material.program_constants[0],material.world_vertex_input_transform);
                for(unsigned c=0;c<4;++c)state.constants[c][c]=1;
                state.constants[4]={1,0,0,1};state.constants[5]={0,1,0,.5f};
                state.constants[6]={0,0,-1,2};state.constants[7]={0,0,1,767.8125f};
                state.constants[10]={32,-32,16777215,0};state.constants[11]={32.5f,32.5f,0,0};
                state.constants[172]={1,1,0,0};
                for(unsigned c=0;c<16;++c)state.constants[18+c]=material.program_constants[1][c];
                execute_vertex_program(vertex_program,state);
                vertex_error=std::max(vertex_error,std::abs(state.outputs[3][3]-1.f));
                const auto expected=pt::unlit_uv(images.uv,vertex.uv);
                for(unsigned stage=0;stage<2;++stage) {
                    vertex_error=std::max(vertex_error,std::abs(state.outputs[9+stage][0]-expected[stage].x));
                    vertex_error=std::max(vertex_error,std::abs(state.outputs[9+stage][1]-expected[stage].y));
                }
                ++sampled_vertices;++vertex_cases;
            }
            for(unsigned sample=0;sample<1024;++sample) {
                std::array<ShaderVector,4> t;for(auto& texel:t)for(auto& c:texel)c=channel(random);
                auto words=material.forward_program.words;
                if(sample&1) {
                    for(unsigned word=10;word<=25;++word)words[word]=random();
                    words[43]=random();words[44]=random();
                }
                const auto source=diffuse_oracle(words,t);ShaderVector fixed{};
                for(unsigned c=0;c<3;++c)fixed[c]=std::clamp(2.f*t[0][c]*t[1][c],0.f,1.f);fixed[3]=1;
                source_error=std::max(source_error,rgb_error(source,fixed));++source_cases;
            }
            for(unsigned sample=0;sample<128;++sample) {
                const Vec2 raw{uv(random),uv(random)};const auto expected=diffuse_oracle(material,images.samples(raw));
                const auto actual=pt::material_surface(scene,0,raw);const float e=actual?rgb_error(*actual,expected):1;
                atlas_error=std::max(atlas_error,e);++atlas_cases;
                if(examples.size()<3 && e>.02f)examples.push_back({{"raw_uv",{raw.x,raw.y}},
                    {"original_unlit_diffuse",expected},{"rt_surface",actual?Json(*actual):Json()},{"error",e}});
            }
            const auto& pass=material.passes.at(material.forward_pass_index);
            Json bindings=Json::array(),stages=Json::array();
            for(const auto& binding:pass.textures)bindings.push_back({{"stage",binding.stage},{"parameter",binding.parameter},
                {"external",binding.external},{"flags",binding.flags}});
            for(unsigned stage=0;stage<2;++stage) {
                const auto& sampler=material.texture_states[stage];const auto& image=images.images[stage];
                stages.push_back({{"stage",stage},{"width",image.width},{"height",image.height},
                    {"source_mip_width",images.mips[stage].width},{"source_mip_height",images.mips[stage].height},
                    {"decoded_image_fingerprint",image.pixels.fingerprint()},
                    {"address_u",sampler.address_u},{"address_v",sampler.address_v},
                    {"mag_filter",sampler.mag_filter},{"min_filter",sampler.min_filter},{"raw_alpha_kill",sampler.alpha_kill}});
            }
            materials.push_back({{"slot",slot},{"name",material.name},{"pass",pass.name},{"vertex",pass.vertex_shader},
                {"texture_bindings",bindings},{"color_stages",stages},{"forward_transforms",material.forward_transforms},
                {"program",material.forward_program.words},{"atlas_has_surface_record",bool(scene.materials[0].texture[3]&pt::surface_flag)},
                {"maximum_source_expression_error",source_error},{"maximum_rt_atlas_error",atlas_error},
                {"maximum_vertex_uv_alpha_error",vertex_error},{"source_vertex_cases",sampled_vertices},{"examples",examples}});
            maximum_source_error=std::max(maximum_source_error,source_error);maximum_atlas_error=std::max(maximum_atlas_error,atlas_error);
            maximum_vertex_error=std::max(maximum_vertex_error,vertex_error);++total;
            controls(material);
        }
        require(!materials.empty(),"Opaque source cohort missed its authenticated control family");
        report["cohorts"].push_back({{"map",argv[n]},{"mcc",cache.is_mcc()},{"materials",materials}});
    }
    const bool passed=total>=54 && vertex_cases>0 && maximum_vertex_error<2.e-6f && maximum_source_error<2.e-6f && maximum_atlas_error<2.e-6f;
    report.update({{"passed",passed},{"actual_materials",total},{"source_cases",source_cases},{"atlas_cases",atlas_cases},
        {"maximum_source_expression_error",maximum_source_error},{"maximum_rt_atlas_error",maximum_atlas_error},
        {"vertex_cases",vertex_cases},{"maximum_vertex_uv_alpha_error",maximum_vertex_error},
        {"control_cases",control_cases},
        {"scope","Actual source opaque Coagulation/Lockout diffuse RGB with baked light/specular isolated, including all color-word animation. Native ordered UVs and authored bounded mip sampler versus production RT atlas. Opaque T0 alpha is a representative only; no full source output-alpha, native-frame or GPU equivalence claim."}});
    std::ofstream(argv[5])<<report.dump(2)<<'\n';
    std::cout<<(passed?"PASS":"FAIL")<<" source opaque surface: materials="<<total<<" source_error="<<maximum_source_error<<" rt_atlas_error="<<maximum_atlas_error<<'\n';
    return passed?0:1;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
