#include "halo2/path_tracing/surface.hpp"
#include "halo2/path_tracing/material_surface.hpp"
#include "halo2/path_tracing/scene.hpp"
#include "halo2/source_texture_filtering.hpp"
#include "halo2/texture.hpp"
#include "halo2/world_material.hpp"
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
unsigned checks{},qualified{},scalar_cases{},atlas_cases{},coverage_cases{};
float maximum_error{};
void require(bool value,const std::string& message){++checks;if(!value)throw FormatError(message);}
void check_color(const ShaderVector& a,const ShaderVector& b,const std::string& message) {
    float difference{};for(unsigned c=0;c<4;++c)difference=std::max(difference,std::abs(a[c]-b[c]));
    maximum_error=std::max(maximum_error,difference);require(difference<2.e-6f,message+" error="+std::to_string(difference)+
        " actual="+std::to_string(a[0])+","+std::to_string(a[1])+","+std::to_string(a[2])+","+std::to_string(a[3])+
        " expected="+std::to_string(b[0])+","+std::to_string(b[1])+","+std::to_string(b[2])+","+std::to_string(b[3]));
}
bool bump_coverage(pt::SurfaceFamily family) {
    return family==pt::SurfaceFamily::BumpLeafCoverage || family==pt::SurfaceFamily::BumpLeafCoveragePerVertex;
}
void check_surface(ShaderVector actual,const ShaderVector& expected,pt::SurfaceFamily family,const std::string& message) {
    if(bump_coverage(family)) {
        require((std::round(actual[3]*255)<255)==(std::round(expected[3]*255)<255),message+" native LESS255 coverage differs");
        actual[3]=expected[3]; // Alpha is a coverage representative for these two restricted families.
    }
    check_color(actual,expected,message);
}
std::array<Vertex,3> triangle() {
    std::array<Vertex,3> out{};out[1].position={1,0,0};out[2].position={0,1,0};
    out[1].uv={1,0};out[2].uv={0,1};
    for(auto& vertex:out){vertex.normal={0,0,1};vertex.color={.1f,.2f,.3f};}return out;
}
Texture solid(unsigned r,unsigned g,unsigned b,unsigned a) {
    return {1,1,11,{std::byte(r),std::byte(g),std::byte(b),std::byte(a)}};
}
ShaderVector oracle(const Material& material,std::array<ShaderVector,4> t) {
    auto v1=material.forward_specular;
    if(pt::qualified_unlit_uv(material)->family==pt::UnlitUvFamily::LightmapDetail) {
        if(material.per_vertex_lighting)v1[0]=v1[1]=v1[2]=.5f;
        else t[3][0]=t[3][1]=t[3][2]=.5f;
        v1[3]=0;
    }
    return halo2_test::scalar_combiners(material.forward_program.words,t,material.forward_diffuse,v1);
}
// Read authored RGBA8/BC images directly, independently of the resident record.
// Only secondary transport uses this bounded existing source mip; primary
// shading retains the original resident full-resolution four texture views.
struct SourceImages {
    const Material& material;pt::SurfaceSource source;std::array<Texture,4> images;
    std::array<TextureMip,4> mips{};
    explicit SourceImages(const Material& m):material(m),source(*pt::qualified_surface(m)) {
        const Texture* textures[]{&m.diffuse,&m.detail,&m.bump,&m.auxiliary};
        for(unsigned s=0;s<4;++s)if(source.stages&(1u<<s)) {
            const auto& frames=m.animated_textures[s];images[s]=frames.empty()?*textures[s]:frames.at(m.texture_frames[s]);
            auto& image=images[s];if(image.format>=14 && image.format<=16)image=prepare_xemu_bc_texture(image);
            require(image.format==11,"Source surface oracle requires decoded authored color");
            if(image.mips.empty())image.mips.push_back({image.width,image.height,0,std::size_t(image.width)*image.height*4});
            unsigned level=std::min(m.texture_states[s].max_mip,unsigned(image.mips.size()-1));
            while(level+1<image.mips.size() && (image.mips[level].width>128 || image.mips[level].height>128))++level;
            mips[s]=image.mips[level];
        }
    }
    ShaderVector fetch(unsigned stage,int x,int y)const {
        const auto& mip=mips[stage];const auto& sampler=material.texture_states[stage];
        const auto coordinate=[](int p,unsigned extent,unsigned mode) {
            switch(mode) {
            case 3:return std::clamp(p,0,int(extent)-1);
            case 4:return p<0 || p>=int(extent)?-1:p;
            case 5:return std::min(p<0?-p-1:p,int(extent)-1);
            default: {int at=p%(int(extent)*(mode==2?2:1));if(at<0)at+=int(extent)*(mode==2?2:1);
                return mode==2 && at>=int(extent)?int(extent)*2-at-1:at;}
            }
        };
        x=coordinate(x,mip.width,sampler.address_u);y=coordinate(y,mip.height,sampler.address_v);
        if(x<0 || y<0)return pt::surface_detail::argb(sampler.border_color);
        const auto at=mip.offset+(std::size_t(y)*mip.width+unsigned(x))*4;ShaderVector out;
        for(unsigned c=0;c<4;++c)out[c]=float(std::to_integer<unsigned>(images[stage].pixels.at(at+c)))/255;
        return out;
    }
    std::optional<ShaderVector> at(Vec2 raw)const {
        const auto coords=pt::unlit_uv(source.uv,raw);std::array<ShaderVector,4> t{};
        for(unsigned s=0;s<4;++s)if(source.stages&(1u<<s)) {
            const auto& mip=mips[s];const float x=coords[s].x*mip.width,y=coords[s].y*mip.height;
            if(source_texture_filtering(material.texture_states[s]).mag_filter==1)t[s]=fetch(s,int(std::floor(x)),int(std::floor(y)));
            else {
                const float xx=x-.5f,yy=y-.5f;const int left=int(std::floor(xx)),top=int(std::floor(yy));
                const float fx=xx-left,fy=yy-top;const auto a=fetch(s,left,top),b=fetch(s,left+1,top),c=fetch(s,left,top+1),d=fetch(s,left+1,top+1);
                for(unsigned lane=0;lane<4;++lane)t[s][lane]=(a[lane]*(1-fx)+b[lane]*fx)*(1-fy)+(c[lane]*(1-fx)+d[lane]*fx)*fy;
            }
            if((material.texture_states[s].alpha_kill&4u) && t[s][3]==0.f)return {};
        }
        return oracle(material,t);
    }
};
void verify(Material material,std::mt19937& random) {
    const auto source=pt::qualified_surface(material);if(!source)return;++qualified;
    const auto vertices=triangle();auto scene=pt::build_scene(vertices,std::span(&material,1));
    require(scene.materials[0].texture[3]&pt::surface_flag,"Qualified surface did not retain its source payload");
    require(scene.materials[0].texture[3]&128u,"Qualified native model radiance became unlit albedo");
    require(scene.materials[0].transform==source->uv.base,"Qualified source vertex family lost its native c18 base UVs");
    require(scene.triangles[0].metadata[1]==0xffffffffu,"Qualified source model radiance was not removed from secondary tint");
    const auto pointer=std::bit_cast<unsigned>(scene.materials[0].base[3]);
    const auto words=scene.texture_texel_count;pt::refresh_materials(scene,std::span(&material,1));
    require(std::bit_cast<unsigned>(scene.materials[0].base[3])==pointer && scene.texture_texel_count==words,
        "Unchanged source surface allocated new resident storage");
    std::uniform_real_distribution<float> channel(0,1);
    const auto original=material.forward_program;
    for(unsigned n=0;n<1024;++n) {
        std::array<ShaderVector,4> t{};for(auto& sample:t)for(auto& c:sample)c=channel(random);
        for(unsigned c:{13u,21u,43u,44u})material.forward_program.words[c]=random();
        if(bump_coverage(source->family)) {
            const auto below_opaque=[&]{return (random()&0xffffffu)|((random()%255u)<<24);};
            if(source->family==pt::SurfaceFamily::BumpLeafCoverage) {
                material.forward_program.words[13]=below_opaque();material.forward_program.words[22]=random()|0xff000000u;
            } else {
                material.forward_program.words[14]=random()|0xff000000u;
                material.forward_program.words[15]=below_opaque();material.forward_program.words[17]=random()|0xff000000u;
            }
            t[2][3]=float(n&1?128:127)/255;
        }
        const auto colors=pt::surface_constants(material,source->family);
        const auto actual=pt::surface_color(source->family,colors,t);
        check_surface(actual,oracle(material,t),source->family,"Fixed native unlit expression differs from full source combiner "+material.name+" family="+std::to_string(unsigned(source->family)));++scalar_cases;
        if(bump_coverage(source->family)) {
            ShaderVector v0,v1;for(unsigned c=0;c<4;++c){v0[c]=channel(random);v1[c]=channel(random);}
            if(n<4){t[0][3]=float(n&1);v1[3]=float(n>>1);}
            const auto full=halo2_test::scalar_combiners(material.forward_program.words,t,v0,v1);
            const auto representative=pt::surface_color(source->family,colors,t);
            require((std::round(full[3]*255)<255)==(std::round(representative[3]*255)<255),
                "Restricted bump coverage differs from the complete source program with authored lighting");++coverage_cases;
        }
    }
    material.forward_program=original;SourceImages images(material);
    for(const Vec2 uv:std::array<Vec2,5>{{{0,0},{.17f,.93f},{-.125f,1.125f},{.625f,.375f},{1.25f,-2.5f}}}) {
        const auto expected=images.at(uv),actual=pt::material_surface(scene,0,uv);
        require(bool(actual)==bool(expected),"Surface atlas changed raw used-stage alpha-kill");
        if(actual)check_surface(*actual,*expected,source->family,"Source surface atlas changed authored images, constants, sampler or ordered UVs");++atlas_cases;
    }
    material.forward_program.words[34]^=1u;
    require(!pt::qualified_surface(material),"Changed native pixel control bypassed exact qualification");
    require(pt::material_response(material).transform==source->uv.base,
        "Qualified source vertex UVs were lost by a legacy pixel-family fallback");
    pt::refresh_materials(scene,std::span(&material,1));
    require(!(scene.materials[0].texture[3]&pt::surface_flag),"A removed source family retained a stale resident surface pointer");
    material.forward_program=original;material.vertex_program.back()^=std::byte{1};
    require(!pt::qualified_surface(material),"Changed native vertex code bypassed exact qualification");
}
void bump_controls(Material material) {
    const auto source=pt::qualified_surface(material);require(source && bump_coverage(source->family),"Bump coverage controls lost their actual native family");
    const auto rejected=[&](Material changed,const std::string& reason){require(!pt::qualified_surface(changed),"Bump coverage admitted "+reason);};
    auto changed=material;changed.forward_raster.alpha_test=0;rejected(changed,"disabled raster alpha test");
    changed=material;changed.forward_raster.alpha_function=0x203;rejected(changed,"another alpha function");
    changed=material;changed.forward_raster.alpha_reference=254;rejected(changed,"another alpha reference");
    changed=material;changed.forward_blend=1;rejected(changed,"an alpha blended pass");
    if(source->family==pt::SurfaceFamily::BumpLeafCoverage) {
        changed=material;changed.forward_program.words[13]|=0xff000000u;rejected(changed,"an opaque bound of255");
        changed=material;changed.forward_program.words[22]&=0xfeffffffu;rejected(changed,"a hole below255");
    } else {
        changed=material;changed.forward_program.words[14]&=0xfeffffffu;rejected(changed,"an unqualified native alpha product");
        changed=material;changed.forward_program.words[15]|=0xff000000u;rejected(changed,"an opaque bound of255");
        changed=material;changed.forward_program.words[17]&=0xfeffffffu;rejected(changed,"a hole below255");
    }
    changed=material;changed.texture_states[2].color_sign=1;rejected(changed,"a signed selector texture");
    material.animated_textures={};material.diffuse=solid(64,128,32,255);material.detail=solid(128,128,128,255);
    material.bump=solid(128,128,255,128);for(auto& state:material.texture_states)state.alpha_kill=0;
    const auto vertices=triangle();auto scene=pt::build_scene(vertices,std::span(&material,1));pt::Hit hit;
    require(pt::intersect(scene,{.25f,.25f,1},{0,0,-1},2,hit),"Native bump T2 opaque selector was lost");
    material.bump=solid(128,128,255,127);pt::refresh_materials(scene,std::span(&material,1));
    require(!pt::intersect(scene,{.25f,.25f,1},{0,0,-1},2,hit),"Native bump T2 hole selector did not refresh");
    material.bump=solid(128,128,255,128);material.texture_states[1].alpha_kill=4;material.detail=solid(128,128,128,0);
    pt::refresh_materials(scene,std::span(&material,1));
    require(!pt::intersect(scene,{.25f,.25f,1},{0,0,-1},2,hit),"Bump coverage discarded an independent raw detail-stage kill");
}
void alpha_controls(Material material) {
    // Actual source leaf control program, synthetic texture inputs. Native
    // output alpha0 can be opaque under LESS255; raw stage kills remain early.
    require(bool(pt::qualified_surface(material)),"Leaf alpha control lost its actual native source family");
    material.animated_textures={};material.diffuse=solid(64,128,32,255);material.detail=solid(128,128,128,255);
    material.forward_program.words[13]=0;material.forward_program.words[21]=0xff000000u;
    material.forward_raster.alpha_test=1;material.forward_raster.alpha_function=0x201;material.forward_raster.alpha_reference=255;
    material.texture_states[1].alpha_kill=4;
    auto vertices=triangle();auto scene=pt::build_scene(vertices,std::span(&material,1));pt::Hit hit;
    require((scene.materials[0].texture[3]&8u)!=0,"Secondary alpha guard lost a used detail-stage kill");
    require(pt::intersect(scene,{.25f,.25f,1},{0,0,-1},2,hit),"Native leaf output-alpha0 was incorrectly killed after its mux");
    material.detail=solid(128,128,128,0);pt::refresh_materials(scene,std::span(&material,1));
    require(!pt::intersect(scene,{.25f,.25f,1},{0,0,-1},2,hit),"Used detail-stage raw alpha-kill was ignored");
    material.detail=solid(128,128,128,255);material.diffuse=solid(64,128,32,0);pt::refresh_materials(scene,std::span(&material,1));
    require(!pt::intersect(scene,{.25f,.25f,1},{0,0,-1},2,hit),"Native source LESS255 leaf coverage was reversed");
    material.diffuse=solid(64,128,32,255);material.forward_program.words[13]=0xff000000u;
    pt::refresh_materials(scene,std::span(&material,1));
    require(!pt::intersect(scene,{.25f,.25f,1},{0,0,-1},2,hit),"Changed native output-alpha constant did not refresh");
    material.forward_program.words[13]=0;material.detail.width=2;material.detail.pixels.resize(8,std::byte{255});
    pt::refresh_materials(scene,std::span(&material,1));
    require(pt::intersect(scene,{.25f,.25f,1},{0,0,-1},2,hit),"Relocated native surface record lost its relative stage-image offsets");
    Material procedural;auto colored=pt::build_scene(vertices,std::span(&procedural,1));
    require(!(colored.materials[0].texture[3]&128u) && colored.triangles[0].metadata[1]!=0xffffffffu,
        "Procedural authored vertex colors were confused with native model radiance");
}
}
int main(int argc,char** argv)try {
    require(argc==4,"Supply original Coagulation, prepared Coagulation and report path");
    for(unsigned n=1;n<3;++n)if(!std::filesystem::is_regular_file(argv[n]))return 77;
    std::mt19937 random(0xc0a615u);nlohmann::json report{{"cohorts",nlohmann::json::array()}};std::optional<Material> leaf;
    std::array<std::optional<Material>,2> bump;
    for(unsigned n=1;n<3;++n) {
        Cache cache(argv[n]);auto mesh=cache.load_bsp(0);nlohmann::json rows=nlohmann::json::array();
        const auto add=[&](Material material,const std::string& owner,unsigned slot) {
            const auto source=pt::qualified_surface(material);
            const auto* pass=material.forward_pass && material.forward_pass_index<material.passes.size()?&material.passes[material.forward_pass_index]:nullptr;
            rows.push_back({{"owner",owner},{"slot",slot},{"name",material.name},{"family",source?unsigned(source->family):0},
                {"pass",pass?pass->name:""},{"vertex",pass?pass->vertex_shader:""},{"per_vertex",material.per_vertex_lighting},
                {"shader_lod",material.selected_shader_lod},
                {"alpha_test",material.forward_raster.alpha_test},{"alpha_function",material.forward_raster.alpha_function},
                {"alpha_reference",material.forward_raster.alpha_reference}});
            if(source && source->family==pt::SurfaceFamily::LeafDetail)leaf=material;
            if(source && bump_coverage(source->family))bump[unsigned(source->family)-4]=material;
            verify(std::move(material),random);
        };
        for(unsigned slot=0;slot<mesh.materials.size();++slot)add(mesh.materials[slot],"bsp",slot);
        for(const auto& tag:cache.tags())if(tag.group=="shad" &&
            (tag.name.find("tree")!=std::string::npos || tag.name.find("pine")!=std::string::npos))
            {
                const auto raw=cache.load_material(tag.datum);
                for(unsigned lod=0;lod<raw.lods.size();++lod)for(bool per_vertex:{false,true}) {
                    auto material=raw;material.selected_shader_lod=lod;
                    if(prepare_world_material(cache,material,per_vertex))add(std::move(material),"prepared_tree_variant",tag.datum);
                }
            }
        report["cohorts"].push_back({{"map",argv[n]},{"mcc",cache.is_mcc()},{"materials",std::move(rows)}});
    }
    require(qualified>0 && leaf.has_value(),"Actual source surface census missed its terrain/leaf families");alpha_controls(*leaf);
    for(const auto& material:bump){require(material.has_value(),"Actual source census missed a bump coverage family");bump_controls(*material);}
    report.update({{"passed",true},{"checks",checks},{"qualified_materials",qualified},{"scalar_cases",scalar_cases},
        {"atlas_cases",atlas_cases},{"maximum_error",maximum_error},{"bump_coverage_cases",coverage_cases},
        {"scope","Exact admitted native unlit RGB and family1..3 output-alpha; family4..5 preserve only proven LESS255 coverage. Source mip/sampler transport and tint; no GPU or unknown-family parity claim"}});
    std::ofstream(argv[3])<<report.dump(2)<<'\n';
    std::cout<<"PASS native source surface: materials="<<qualified<<" scalar="<<scalar_cases<<" atlas="<<atlas_cases<<" checks="<<checks<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
