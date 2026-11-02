#include "halo2/path_tracing/material_surface.hpp"
#include "halo2/path_tracing/surface.hpp"
#include "halo2/source_texture_filtering.hpp"
#include "halo2/texture.hpp"
#include "../scalar_combiners.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>

using namespace halo2;
namespace pt=halo2::path_tracing;
namespace {
// Complete controls of the actual visible Lockout owners. Keep these oracle
// operands separate from production qualification so a missing adapter is RED.
constexpr auto frozen_controls=[] {
    auto w=pt::surface_detail::opaque_detail;w[6]=0x1d1d8020;w[32]=0x00010d00;return w;
}();
constexpr std::array<std::uint32_t,60> blend_controls{
    0x3819181a,0,0,0,0,0,0,0,0x0c010002,0x00001c80,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x00000c00,0,0,0,0,0,0,0,
    0x3809180a,0x0c080000,0,0,0,0,0,0,0,0x00ffffff,0,
    0x00000c00,0x000100c0,0,0,0,0,0,0,0x00011102,0x00000421,0,0,0,0,0};
float error(const ShaderVector& a,const ShaderVector& b) {
    float e{};for(unsigned c=0;c<3;++c)e=std::max(e,std::abs(a[c]-b[c]));return e;
}
ShaderVector original_diffuse(std::array<std::uint32_t,60> w,std::array<ShaderVector,4> t,bool blend) {
    if(!blend) {
        // Remove only the source's separate specular RGB, then neutralize its
        // authored diffuse lighting operands, retaining every combiner stage.
        w[15]&=0xff000000u;t[3]={.5f,.5f,.5f,1};
    }
    return halo2_test::scalar_combiners(w,t,{0,0,0,1},{.5f,.5f,1,0});
}
ShaderVector expression(const std::array<std::uint32_t,60>& w,const std::array<ShaderVector,4>& t,bool blend) {
    ShaderVector out{};const auto gain=pt::surface_detail::argb(w[43]),bias=pt::surface_detail::argb(w[44]);
    for(unsigned c=0;c<3;++c) {
        const float detail=blend?(1-t[0][3])*t[1][c]+t[0][3]*t[2][c]:t[1][c];
        out[c]=std::clamp(2*t[0][c]*detail,0.f,1.f);
        if(blend)out[c]=std::clamp(out[c]*gain[c]+bias[c],0.f,1.f);
    }
    out[3]=blend?(1-t[0][3])*t[1][3]+t[0][3]*t[2][3]:t[0][3];return out;
}
// Read the actual source mip/sampler independently of the resident RT atlas.
struct Images {
    const Material& material;pt::UnlitUv uv;unsigned count;
    std::array<Texture,3> images;std::array<TextureMip,3> mips;
    Images(const Material& m,bool blend):material(m),uv(*pt::qualified_unlit_uv(m)),count(blend?3:2) {
        const Texture* source[]{&m.diffuse,&m.detail,&m.bump};
        for(unsigned s=0;s<count;++s) {
            images[s]=m.animated_textures[s].empty()?*source[s]:m.animated_textures[s].at(m.texture_frames[s]);
            if(images[s].format>=14 && images[s].format<=16)images[s]=prepare_xemu_bc_texture(images[s]);
            if(images[s].format!=11)throw FormatError("Panel source requires actual RGBA8/BC operands");
            if(images[s].mips.empty())images[s].mips.push_back({images[s].width,images[s].height,0,std::size_t(images[s].width)*images[s].height*4});
            unsigned mip=std::min(m.texture_states[s].max_mip,unsigned(images[s].mips.size()-1));
            while(mip+1<images[s].mips.size() && (images[s].mips[mip].width>128 || images[s].mips[mip].height>128))++mip;
            mips[s]=images[s].mips[mip];
        }
    }
    ShaderVector fetch(unsigned s,int x,int y)const {
        const auto& m=mips[s];const auto& state=material.texture_states[s];
        const auto address=[](int p,unsigned n,unsigned mode) {
            if(mode==3)return std::clamp(p,0,int(n)-1);
            if(mode==4)return p<0 || p>=int(n)?-1:p;
            if(mode==5)return std::min(p<0?-p-1:p,int(n)-1);
            const int period=int(n)*(mode==2?2:1);int a=p%period;if(a<0)a+=period;
            return mode==2 && a>=int(n)?period-a-1:a;
        };
        x=address(x,m.width,state.address_u);y=address(y,m.height,state.address_v);
        if(x<0 || y<0)return pt::surface_detail::argb(state.border_color);
        const auto at=m.offset+(std::size_t(y)*m.width+unsigned(x))*4;ShaderVector out;
        for(unsigned c=0;c<4;++c)out[c]=float(std::to_integer<unsigned>(images[s].pixels.at(at+c)))/255;return out;
    }
    std::array<ShaderVector,4> samples(Vec2 raw)const {
        const auto coordinates=pt::unlit_uv(uv,raw);std::array<ShaderVector,4> out{};
        for(unsigned s=0;s<count;++s) {
            const float x=coordinates[s].x*mips[s].width,y=coordinates[s].y*mips[s].height;
            if(source_texture_filtering(material.texture_states[s]).mag_filter==1)out[s]=fetch(s,int(std::floor(x)),int(std::floor(y)));
            else {
                const int left=int(std::floor(x-.5f)),top=int(std::floor(y-.5f));const float fx=x-.5f-left,fy=y-.5f-top;
                const auto a=fetch(s,left,top),b=fetch(s,left+1,top),c=fetch(s,left,top+1),d=fetch(s,left+1,top+1);
                for(unsigned k=0;k<4;++k)out[s][k]=(a[k]*(1-fx)+b[k]*fx)*(1-fy)+(c[k]*(1-fx)+d[k]*fx)*fy;
            }
        }return out;
    }
};
}
int main(int argc,char** argv)try {
    if(argc!=4)throw FormatError("Supply original Lockout, prepared Lockout and report path");
    for(unsigned n=1;n<3;++n)if(!std::filesystem::is_regular_file(argv[n]))return 77;
    nlohmann::json report{{"cohorts",nlohmann::json::array()}};std::mt19937 random(0x9a17u);
    std::uniform_real_distribution<float> channel(0,1),coordinate(-2,2);unsigned total{},scalar{},atlas{},controls{};
    float source_error{},atlas_error{},alpha_error{};bool admitted=true,controls_pass=true;
    for(unsigned n=1;n<3;++n) {
        Cache cache(argv[n]);const auto mesh=cache.load_bsp(0);nlohmann::json rows=nlohmann::json::array();unsigned found{};
        for(const auto& m:mesh.materials) {
            const bool blend=m.name.ends_with("\\main_cell_exterior");
            if(!blend && !m.name.ends_with("\\frozen_panels"))continue;
            ++found;const auto& expected=blend?blend_controls:frozen_controls;
            if(!pt::surface_detail::controls(m.forward_program,expected) || !pt::qualified_unlit_uv(m))
                throw FormatError("Actual visible panel changed its authenticated source control/UV identity");
            Images images(m,blend);std::array<Vertex,3> vertices{};vertices[1].position={1,0,0};vertices[2].position={0,1,0};
            for(auto& v:vertices){v.normal={0,0,1};v.color={1,1,1};}
            auto scene=pt::build_scene(vertices,std::span(&m,1));const auto q=pt::qualified_surface(m);
            const bool qualified=q && unsigned(q->family)==(blend?7u:6u);admitted&=qualified;
            float se{},ae{},fallback_error{};nlohmann::json examples=nlohmann::json::array();
            for(unsigned i=0;i<2048;++i) {
                std::array<ShaderVector,4> t;for(auto& texel:t)for(auto& c:texel)c=channel(random);
                auto w=m.forward_program.words;
                if(i&1){for(unsigned c=10;c<=25;++c)w[c]=random();w[43]=random();w[44]=random();}
                const auto full=original_diffuse(w,t,blend),fixed=expression(w,t,blend);
                se=std::max(se,error(full,fixed));if(blend)alpha_error=std::max(alpha_error,std::abs(full[3]-fixed[3]));++scalar;
            }
            for(unsigned i=0;i<256;++i) {
                const Vec2 uv{coordinate(random),coordinate(random)};const auto t=images.samples(uv);
                const auto wanted=original_diffuse(m.forward_program.words,t,blend);
                const auto actual=pt::material_surface(scene,0,uv);const float e=actual?error(*actual,wanted):1;
                ae=std::max(ae,e);if(blend && actual)alpha_error=std::max(alpha_error,std::abs((*actual)[3]-wanted[3]));
                fallback_error=std::max(fallback_error,error(t[0],wanted));++atlas;
                if(examples.size()<3 && e>.02f)examples.push_back({{"raw_uv",{uv.x,uv.y}},{"original_unlit",wanted},{"rt",actual?nlohmann::json(*actual):nlohmann::json()},{"error",e}});
            }
            if(qualified) {
                for(unsigned word=0;word<60;++word) {
                    auto changed=m;changed.forward_program.words[word]^=1u;
                    const bool color=(word>=10 && word<=25)||word==43||word==44;
                    const auto a=pt::qualified_surface(changed);controls_pass&=bool(a)==color;++controls;
                }
                auto changed=m;changed.forward_blend=1;controls_pass&=!pt::qualified_surface(changed);++controls;
                // OpaqueDetail's alpha is only a representative; the detail
                // blend's output alpha is reproduced, including alpha tests.
                if(!blend){changed=m;changed.forward_raster.alpha_test=1;controls_pass&=!pt::qualified_surface(changed);++controls;}
                changed=m;changed.texture_states[3].alpha_kill|=4;controls_pass&=!pt::qualified_surface(changed);++controls;
                changed=m;changed.vertex_program.back()^=std::byte{1};controls_pass&=!pt::qualified_surface(changed);++controls;
                for(unsigned stage=0;stage<(blend?3u:2u);++stage) {
                    changed=m;changed.animated_textures={};for(auto& state:changed.texture_states)state.alpha_kill=0;
                    Texture* images[]{&changed.diffuse,&changed.detail,&changed.bump};
                    for(unsigned s=0;s<(blend?3u:2u);++s)*images[s]={1,1,11,{std::byte{128},std::byte{128},std::byte{128},std::byte(s==stage?0:255)}};
                    changed.texture_states[stage].alpha_kill=4;
                    const auto killed=pt::build_scene(vertices,std::span(&changed,1));
                    controls_pass&=!pt::material_surface(killed,0,{.25f,.25f});++controls;
                }
                if(blend) {
                    changed=m;changed.forward_program.words[43]=0x004080c0;changed.forward_program.words[44]=0x000a0b0c;
                    pt::refresh_materials(scene,std::span(&changed,1));
                    const Vec2 uv{.37f,.91f};const auto result=pt::material_surface(scene,0,uv);
                    controls_pass&=result && error(*result,original_diffuse(changed.forward_program.words,images.samples(uv),true))<2.e-6f;++controls;
                }
            }
            source_error=std::max(source_error,se);atlas_error=std::max(atlas_error,ae);++total;
            nlohmann::json bindings=nlohmann::json::array();const auto& p=m.passes.at(m.forward_pass_index);
            for(const auto& b:p.textures)bindings.push_back({{"stage",b.stage},{"parameter",b.parameter},{"external",b.external}});
            rows.push_back({{"name",m.name},{"pass",p.name},{"vertex",p.vertex_shader},{"program",m.forward_program.words},
                {"bindings",bindings},{"transforms",m.forward_transforms},{"admitted",qualified},
                {"maximum_source_expression_error",se},{"maximum_atlas_error",ae},{"maximum_T0_fallback_error",fallback_error},{"examples",examples}});
        }
        if(found!=2)throw FormatError("Actual Lockout cohort did not contain both visible panel owners");
        report["cohorts"].push_back({{"map",argv[n]},{"materials",rows}});
    }
    const bool passed=total==4 && admitted && controls_pass && source_error<2.e-6f && atlas_error<2.e-6f && alpha_error<2.e-6f;
    report.update({{"passed",passed},{"actual_materials",total},{"scalar_cases",scalar},{"atlas_cases",atlas},{"control_cases",controls},
        {"maximum_source_expression_error",source_error},{"maximum_atlas_error",atlas_error},{"maximum_three_stage_alpha_error",alpha_error},
        {"admitted",admitted},{"controls_passed",controls_pass},
        {"scope","Exact visible Lockout panel diffuse RGB isolated from original PS lighting/specular. Actual source texture/sampler/mip and ordered native UV atlas comparison. No T2 normal-mapping or full native-frame parity claim."}});
    std::ofstream(argv[3])<<report.dump(2)<<'\n';std::cout<<(passed?"PASS":"FAIL")<<" panel source: expression="<<source_error<<" atlas="<<atlas_error<<" admitted="<<admitted<<'\n';return passed?0:1;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
