#include "halo2/path_tracing/emission.hpp"
#include "halo2/path_tracing/material_emission.hpp"
#include "halo2/path_tracing/material_residency.hpp"
#include "halo2/texture.hpp"
#include "../scalar_combiners.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <random>

using namespace halo2;
namespace pt=halo2::path_tracing;
namespace {
unsigned checks{};
void require(bool value,const std::string& message){++checks;if(!value)throw FormatError(message);}
float error(Vec3 a,Vec3 b){return std::max({std::abs(a.x-b.x),std::abs(a.y-b.y),std::abs(a.z-b.z)});}
std::vector<const Material*> emission_passes(const Material& root) {
    std::vector<const Material*> result;
    if(root.forward_vertex==2)result.push_back(&root);
    for(const auto& pass:root.world_overlays_after)if(pass && pass->forward_vertex==2)result.push_back(pass.get());
    return result;
}
std::array<Vec2,4> source_uv(const Material& pass,Vec2 uv) {
    const auto& base=pass.forward_transforms[0];
    uv={uv.x*base[0]+base[2],uv.y*base[1]+base[3]};
    std::array<Vec2,4> result;
    for(unsigned i=0;i<4;++i) {
        const auto& stage=pass.forward_transforms[i+1];
        result[i]={uv.x*stage[0]+stage[2],uv.y*stage[1]+stage[3]};
    }
    return result;
}
ShaderVector oracle(const Material& pass,const std::array<ShaderVector,4>& samples) {
    return halo2_test::scalar_combiners(pass.forward_program.words,samples,
        pass.forward_diffuse,pass.forward_specular);
}
std::array<Vertex,3> triangle() {
    std::array<Vertex,3> result{};
    result[0].position={0,0,0};result[1].position={1,0,0};result[2].position={0,1,0};
    result[0].uv={0,0};result[1].uv={1,0};result[2].uv={0,1};
    for(auto& vertex:result){vertex.color={1,1,1};vertex.normal={0,0,1};}
    return result;
}
Texture solid(std::array<unsigned,4> rgba) {
    return {1,1,11,{std::byte(rgba[0]),std::byte(rgba[1]),std::byte(rgba[2]),std::byte(rgba[3])}};
}
std::string family_name(pt::EmissionFamily family) {
    switch(family) {
    case pt::EmissionFamily::Default:return "default";
    case pt::EmissionFamily::Multichannel:return "multichannel";
    case pt::EmissionFamily::Detail:return "detail";
    default:return "none";
    }
}
struct PassSamples {
    const Material* material{};
    std::array<Texture,4> images;
    explicit PassSamples(const Material& pass):material(&pass) {
        const Texture* stages[]{&pass.diffuse,&pass.detail,&pass.bump,&pass.auxiliary};
        for(unsigned i=0;i<4;++i) {
            const auto mode=(pass.forward_program.words[54]>>(i*5))&31u;
            require(mode==0 || mode==1,"Emission source requires an unqualified texture-coordinate program");
            if(!mode)continue;
            const auto& frames=pass.animated_textures[i];
            const auto& source=frames.empty()?*stages[i]:frames.at(pass.texture_frames[i]);
            images[i]=source.format>=14 && source.format<=16?prepare_xemu_bc_texture(source):source;
            require(images[i].format==11 && images[i].width && images[i].height &&
                images[i].layers==1 && !images[i].depth,"Emission source is not a decoded authored2D color image");
        }
    }
    ShaderVector texel(unsigned stage,int x,int y)const {
        const auto& image=images[stage];const auto& sampler=material->texture_states[stage];
        const auto coordinate=[](int value,unsigned extent,unsigned address) {
            if(address==3)return std::clamp(value,0,int(extent)-1);
            if(address==2) {
                int mirrored=value%(int(extent)*2);if(mirrored<0)mirrored+=int(extent)*2;
                return mirrored<int(extent)?mirrored:int(extent)*2-1-mirrored;
            }
            require(address==1,"Emission oracle requires authored wrap/mirror/clamp addressing");
            auto wrapped=value%int(extent);return wrapped<0?wrapped+int(extent):wrapped;
        };
        x=coordinate(x,image.width,sampler.address_u);y=coordinate(y,image.height,sampler.address_v);
        const auto first=image.mips.empty()?0:image.mips.front().offset;
        const auto offset=first+(std::size_t(y)*image.width+unsigned(x))*4;
        require(offset+4<=image.pixels.size(),"Emission source image escaped its authored mip");
        ShaderVector result{};
        for(unsigned c=0;c<4;++c)result[c]=float(std::to_integer<unsigned>(image.pixels[offset+c]))/255.f;
        return result;
    }
    std::array<ShaderVector,4> at(Vec2 uv)const {
        const auto coordinates=source_uv(*material,uv);std::array<ShaderVector,4> result{};
        for(unsigned stage=0;stage<4;++stage) {
            if(!((material->forward_program.words[54]>>(stage*5))&31u))continue;
            const auto& image=images[stage];const auto& sampler=material->texture_states[stage];
            if(sampler.mag_filter==1) {
                result[stage]=texel(stage,int(std::floor(coordinates[stage].x*image.width)),
                    int(std::floor(coordinates[stage].y*image.height)));continue;
            }
            require(sampler.mag_filter==2,"Emission oracle requires authored point/linear magnification");
            const float x=coordinates[stage].x*image.width-.5f,y=coordinates[stage].y*image.height-.5f;
            const int left=int(std::floor(x)),top=int(std::floor(y));const float fx=x-left,fy=y-top;
            const auto a=texel(stage,left,top),b=texel(stage,left+1,top),c=texel(stage,left,top+1),d=texel(stage,left+1,top+1);
            for(unsigned n=0;n<4;++n)result[stage][n]=(a[n]*(1-fx)+b[n]*fx)*(1-fy)+(c[n]*(1-fx)+d[n]*fx)*fy;
        }
        return result;
    }
};
Vec3 source_emission(const std::vector<PassSamples>& passes,Vec2 uv) {
    Vec3 result{};
    for(const auto& pass:passes) {
        const auto color=oracle(*pass.material,pass.at(uv));
        result.x+=color[0];result.y+=color[1];result.z+=color[2];
    }
    return result;
}
}
int main(int argc,char** argv)try {
    require(argc==5,"Supply original Lockout, original introduction, prepared MCC Lockout, and report path");
    for(unsigned i=1;i<=3;++i)if(!std::filesystem::is_regular_file(argv[i]))return 77;
    nlohmann::json report{{"scope","Actual authored emissive program/texture transport; source Xbox/MCC inputs remain unchanged"},
        {"cohorts",nlohmann::json::array()}};
    std::mt19937 random(0x15e71u);std::uniform_real_distribution<float> channel(0,1);
    std::map<std::string,unsigned> families;
    unsigned passes_checked{},atlas_roots{},distinct_base_masks{},positive_source_cases{},warm{},cool{};
    float maximum_program_error{},maximum_atlas_error{};
    std::optional<Material> animated_warm;
    const auto vertices=triangle();
    for(unsigned cohort=1;cohort<=3;++cohort) {
        Cache cache(argv[cohort]);require(cache.is_mcc()==(cohort==3),"Source emission cohort changed cache family");
        const auto mesh=cache.load_bsp(0);
        unsigned cohort_passes{},cohort_roots{};
        for(const auto& root:mesh.materials) {
            if(!pt::traces_material(root))continue;
            const auto passes=emission_passes(root);
            if(passes.empty())continue;
            std::vector<PassSamples> source;
            for(const auto* pass:passes) {
                require(pass->forward_pass && pass->forward_pass_index<pass->passes.size(),
                    "Emission pass lost its actual source selection");
                require(pt::supports_emission(*pass),"Unqualified actual emissive pass: "+pass->passes[pass->forward_pass_index].name);
                ++families[family_name(pt::emission_family(*pass))];++cohort_passes;++passes_checked;
                for(unsigned sample=0;sample<1024;++sample) {
                    std::array<ShaderVector,4> inputs{};
                    for(auto& stage:inputs)for(auto& value:stage)value=sample==0?.25f:channel(random);
                    const auto expected=oracle(*pass,inputs);const auto actual=pt::emission_rgb(*pass,inputs);
                    float difference{};
                    for(unsigned c=0;c<4;++c)difference=std::max(difference,std::abs(actual[c]-expected[c]));
                    maximum_program_error=std::max(maximum_program_error,difference);
                    require(difference<2.e-6f,"Native emission expression differs from the authored scalar combiner: "+root.name);
                    if(!sample) {
                        require(expected[0]>0 || expected[1]>0 || expected[2]>0,"Actual authored emission unexpectedly has no positive RGB response");
                        ++positive_source_cases;
                    }
                }
                for(const Vec2 uv:std::array<Vec2,3>{{{0,0},{.17f,.93f},{-1.25f,2.5f}}}) {
                    const auto expected=source_uv(*pass,uv),actual=pt::emission_uv(*pass,uv);
                    for(unsigned stage=0;stage<4;++stage)
                        require(std::abs(expected[stage].x-actual[stage].x)<1.e-6f &&
                            std::abs(expected[stage].y-actual[stage].y)<1.e-6f,"Emission UVs changed authored c18..22 transforms");
                }
                auto unknown=*pass;unknown.forward_program.words[34]^=1u;
                require(!pt::supports_emission(unknown),"Changed emission instructions were accepted through a family-name match");
                unknown=*pass;unknown.vertex_program.back()^=std::byte{1};
                require(!pt::supports_emission(unknown),"Changed source emission vertex code was admitted");
                unknown=*pass;unknown.forward_blend=1;
                require(!pt::supports_emission(unknown),"Alpha-blended emission was admitted as an additive source pass");
                unknown=*pass;unknown.texture_states[0].color_sign=1;
                require(!pt::supports_emission(unknown),"Signed emission texture bypassed source qualification");
                unknown=*pass;unknown.animated_textures[0].clear();unknown.diffuse.layers=6;
                require(!pt::supports_emission(unknown),"Cube emission texture bypassed the exact2D source interface");
                unknown.diffuse.layers=1;unknown.diffuse.depth=1;
                require(!pt::supports_emission(unknown),"Volume emission texture bypassed the exact2D source interface");
                if(root.forward_vertex!=2 && root.diffuse.pixels!=pass->diffuse.pixels)++distinct_base_masks;
                source.emplace_back(*pass);
            }
            auto scene=pt::build_scene(vertices,std::span(&root,1));
            require(scene.emissive_count==1,"Actual emissive root did not enter the secondary emitter index: "+root.name);
            ++cohort_roots;++atlas_roots;
            // Full authored mask images, transforms and colors are evaluated
            // here at raw source UVs, independently of the RT transport layout.
            // The oracle executes the independent full-program scalar combiner.
            for(const Vec2 uv:std::array<Vec2,4>{{{.125f,.125f},{.375f,.625f},{.625f,.375f},{.875f,.875f}}}) {
                const auto expected=source_emission(source,uv);
                const auto actual=pt::material_emission(scene,0,uv);
                const auto difference=error(expected,actual);maximum_atlas_error=std::max(maximum_atlas_error,difference);
                require(difference<=2.f/255.f,"Emission atlas differs from actual source images, tint or transforms: "+root.name);
            }
            if(root.name.ends_with("\\light_fixture_warm_glow")) {
                const auto value=pt::material_emission(scene,0,{.5f,.5f});
                require(value.x>value.z,"Actual Lockout warm emissive tint became neutral/cool");++warm;
                if(!animated_warm)animated_warm=root;
            }
            if(root.name.ends_with("\\light_fixture_cool_glow")) {
                const auto value=pt::material_emission(scene,0,{.5f,.5f});
                require(value.z>value.x,"Actual Lockout cool emissive tint became neutral/warm");++cool;
            }
        }
        require(cohort_passes==(cohort==2?34u:11u),"Actual authored emission pass coverage changed");
        const auto complete=pt::build_scene(mesh.vertices,mesh.materials);
        require(complete.emissive_count>0,"Complete actual map lost its emissive triangle bank");
        report["cohorts"].push_back({{"map",argv[cohort]},{"mcc",cache.is_mcc()},
            {"emissive_passes",cohort_passes},{"emissive_roots",cohort_roots},
            {"complete_scene_words",complete.texture_texel_count},{"complete_scene_emitters",complete.emissive_count}});
    }
    require(families["default"] && families["multichannel"] && families["detail"],"Actual source cohorts missed an emission family");
    require(distinct_base_masks>0 && warm>0 && cool>0,"Regression did not distinguish actual base albedo, emission mask and warm/cool tint");
    require(animated_warm.has_value(),"Actual warm source material unavailable for frame-refresh control");
    auto material=*animated_warm;
    for(auto& child:material.world_overlays_after)if(child && child->forward_vertex==2)child=std::make_shared<Material>(*child);
    auto child=std::find_if(material.world_overlays_after.begin(),material.world_overlays_after.end(),
        [](const auto& value){return value && value->forward_vertex==2;});
    require(child!=material.world_overlays_after.end(),"Warm frame control lost its separate source emission pass");
    (*child)->animated_textures[0]={solid({255,0,0,255}),solid({0,255,0,255})};(*child)->texture_frames[0]=0;
    auto scene=pt::build_scene(vertices,std::span(&material,1));
    const auto red=pt::material_emission(scene,0,{.5f,.5f});
    (*child)->texture_frames[0]=1;pt::refresh_materials(scene,std::span(&material,1));pt::update_geometry(scene,vertices);
    const auto green=pt::material_emission(scene,0,{.5f,.5f});
    require(red.x>0 && red.y==0 && green.x==0 && green.y>0,
        "Secondary emission retained the old authored child texture frame");
    const auto unchanged=scene.pixels;pt::refresh_materials(scene,std::span(&material,1));pt::update_geometry(scene,vertices);
    require(scene.pixels==unchanged,"Unchanged authored emission frame rewrote its transport payload");
    material.world_overlays_after.erase(child);pt::refresh_materials(scene,std::span(&material,1));pt::update_geometry(scene,vertices);
    require(scene.emissive_count==0 && error(pt::material_emission(scene,0,{.5f,.5f}),{})==0,
        "Deleting an emission pass left stale emission or made base albedo glow");
    report.update({{"passed",true},{"checks",checks},{"passes",passes_checked},{"atlas_roots",atlas_roots},
        {"positive_quarter_sample_programs",positive_source_cases},{"families",families},
        {"distinct_base_emission_masks",distinct_base_masks},{"warm_materials",warm},{"cool_materials",cool},
        {"maximum_program_error",maximum_program_error},{"maximum_atlas_error",maximum_atlas_error},
        {"animated_emission_frame_refresh",true},{"deleted_emission_pass_cleared",true}});
    std::ofstream output(argv[4]);require(bool(output),"Cannot save small emission regression receipt");output<<report.dump(2)<<'\n';
    std::cout<<"PASS actual-source emission: passes="<<passes_checked<<" atlas_roots="<<atlas_roots
        <<" program_error="<<maximum_program_error<<" atlas_error="<<maximum_atlas_error<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
