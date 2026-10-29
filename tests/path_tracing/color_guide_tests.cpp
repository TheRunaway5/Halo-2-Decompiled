#include "halo2/path_tracing/color_guide.hpp"
#include "halo2/path_tracing/material_residency.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>

using namespace halo2;
namespace pt=halo2::path_tracing;
namespace {
unsigned checks{};
void require(bool value,const char* message){++checks;if(!value)throw FormatError(message);}
float brightness(Vec3 c){return (c.x*.299f+c.y*.587f)+c.z*.114f;}
float error(Vec3 a,Vec3 b){return std::max({std::abs(a.x-b.x),std::abs(a.y-b.y),std::abs(a.z-b.z)});}
nlohmann::json vector(Vec3 c){return nlohmann::json::array({c.x,c.y,c.z});}
Texture solid(unsigned r,unsigned g,unsigned b){return {1,1,11,{std::byte(r),std::byte(g),std::byte(b),std::byte{255}}};}
void controls(){
    pt::Scene scene;pt::GpuMaterial material;material.color_guide={1,3u|(3u<<4)|(2u<<8),0,0};scene.materials.push_back(material);
    const std::array images{solid(128,32,16),solid(64,64,64),solid(0,0,0),solid(64,16,8)};
    pt::refresh_color_guides(scene,images);const auto resident=scene.color_guide_pixels.data();
    pt::refresh_color_guides(scene,images);require(scene.color_guide_pixels.data()==resident,"Unchanged source guide reallocated its resident atlas");
    const Vec3 incoming{.1f,.4f,.9f};const auto first=pt::sample_color_guide(scene,material,1,{.5f,.5f});
    const auto scaled=pt::sample_color_guide(scene,material,4,{.5f,.5f});
    require(first && scaled,"Positive source color guide was not admitted");
    const auto guided=pt::guide_static_color(incoming,first),half=pt::guide_static_color(incoming,scaled);
    require(error(guided,half)<2.e-6f,"Baked guide intensity leaked into RT lighting");
    require(std::abs(brightness(guided)-brightness(incoming))<2.e-6f,"Color guide changed traced source brightness");
    require(error(pt::guide_static_color(incoming,{}),incoming)==0,"Absent guide altered original RT color");
    const auto black=pt::sample_color_guide(scene,material,3,{.5f,.5f});
    require(!black && error(pt::guide_static_color(incoming,black),incoming)==0,"Black baked shadow was reused as RT darkness");
    const auto gray=pt::sample_color_guide(scene,material,2,{.5f,.5f});require(gray.has_value(),"Actual gray source guide was confused with absent guide");
    const auto neutral=pt::guide_static_color(incoming,gray);
    require(std::abs(neutral.x-neutral.y)<1.e-6f && std::abs(neutral.y-neutral.z)<1.e-6f,
        "Valid neutral source guide retained captured sky blue");
    require(error(neutral,incoming)>.1f,"Gray guide and absent guide behaved identically");
    Material procedural;procedural.diffuse=solid(255,255,255);
    require(pt::color_guide_material(procedural)[0]==0,"Arbitrary procedural/model colors became baked lighting guides");
    Vertex a,b,c;a.lightmap=b.lightmap=c.lightmap=1;
    a.lightmap_uv={0,0};b.lightmap_uv={1,0};c.lightmap_uv={0,1};
    pt::GpuTriangle triangle;triangle.color_guide=pt::color_guide_triangle(a,b,c,material);
    const auto secondary=pt::sample_color_guide(scene,triangle,{.2f,.3f,.5f});
    require(secondary && error(*first,*secondary)<2.e-6f,"Secondary lightmap guide changed source RGB/channel order");
    material.color_guide[0]=2;scene.materials[0]=material;a.color={1,0,0};b.color={0,1,0};c.color={0,0,1};
    triangle.color_guide=pt::color_guide_triangle(a,b,c,material);
    const auto vertex=pt::sample_color_guide(scene,triangle,{.2f,.3f,.5f});
    require(vertex && error(*vertex,{.2f,.3f,.5f})<2.e-6f,"Proven vertex radiance interpolation changed source RGB order");
    pt::refresh_color_guides(scene,{});require(scene.color_guide_pixels.size()==4 && scene.color_guide_pixels[0]==0 &&
        scene.color_guide_image_keys.empty(),"Empty source guide bank retained map data");
}
nlohmann::json actual_map(const std::filesystem::path& path){
    Cache cache(path);require(cache.is_mcc(),"Color guide source cohort requires prepared MCC runtime data");
    const auto mesh=cache.load_bsp(0);const auto lighting=cache.load_bsp_lighting(0);
    require(lighting.structure_matches,"Actual color guide source geometry checksum did not match");
    auto scene=pt::build_scene(mesh.vertices,mesh.materials);pt::refresh_color_guides(scene,mesh.lightmaps);
    require(!mesh.lightmaps.empty() && scene.color_guide_pixels[0]==mesh.lightmaps.size(),"Actual map lightmap/palette bank was not retained");
    std::map<unsigned,std::size_t> triangles;std::size_t textured{},vertex{},sampled{},zero{};float maximum_error{};
    Vec3 sum{};nlohmann::json examples=nlohmann::json::array();
    for(const auto& triangle:scene.triangles){
        const auto image=std::bit_cast<std::uint32_t>(triangle.color_guide[3]);if(!image)continue;
        ++triangles[triangle.metadata[0]];textured+=image!=0xffffffffu;vertex+=image==0xffffffffu;
        if(sampled>=2048)continue;
        const auto value=pt::sample_color_guide(scene,triangle,{1.f/3,1.f/3,1.f/3});++sampled;
        if(!value){++zero;continue;}
        sum.x+=value->x;sum.y+=value->y;sum.z+=value->z;
        const auto result=pt::guide_static_color({.1f,.4f,.9f},value);
        maximum_error=std::max(maximum_error,std::abs(brightness(result)-brightness({.1f,.4f,.9f})));
        if(examples.size()<16)examples.push_back({{"material",mesh.materials[triangle.metadata[0]].name},
            {"image",image},{"guide_rgb",vector(*value)},{"guided_blue_input",vector(result)}});
    }
    require(textured && sampled && zero<sampled,"Actual source color guide census admitted no positive lightmap color");
    require(maximum_error<2.e-6f,"Actual authored lightmap colors changed traced brightness");
    nlohmann::json materials=nlohmann::json::array();
    for(const auto& [slot,count]:triangles)materials.push_back({{"slot",slot},{"name",mesh.materials[slot].name},
        {"kind",scene.materials[slot].color_guide[0]},{"triangles",count}});
    return {{"map",path.string()},{"bsp_triangles",mesh.vertices.size()/3},{"textured_guide_triangles",textured},
        {"vertex_guide_triangles",vertex},{"images",mesh.lightmaps.size()},{"atlas_bytes",scene.color_guide_pixels.size()*4},
        {"sampled",sampled},{"zero_samples",zero},{"mean_sample_rgb",vector({sum.x/float(sampled-zero),sum.y/float(sampled-zero),sum.z/float(sampled-zero)})},
        {"maximum_brightness_error",maximum_error},{"materials",std::move(materials)},{"examples",std::move(examples)}};
}
}
int main(int argc,char** argv)try{
    require(argc==4,"Supply prepared Coagulation, Lockout and bounded report path");
    for(unsigned n=1;n<3;++n)if(!std::filesystem::is_regular_file(argv[n]))return 77;
    controls();nlohmann::json cohorts=nlohmann::json::array();for(unsigned n=1;n<3;++n)cohorts.push_back(actual_map(argv[n]));
    nlohmann::json report{{"passed",true},{"checks",checks},{"cohorts",std::move(cohorts)},
        {"scope","Actual prepared source lightmap/palette and proven per-vertex RGB transport. Static sky/sun hue is brightness-preserving and independent of baked intensity. No GPU/runtime color-patch or missing-energy acceptance claim."}};
    std::ofstream output(argv[3]);require(bool(output),"Cannot save bounded source color-guide report");output<<report.dump(2)<<'\n';
    std::cout<<"PASS source baked COLOR guide: checks="<<checks<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
