// Actual Cache admission/material audit for the finite PC-exclusive sky cohort.
#include "halo2/sky_input_identity.hpp"
#include "halo2/appearance.hpp"
#include "halo2/vertex_program.hpp"
#include <fstream>
#include <iostream>
#include <map>
#include <memory>

namespace {
using namespace halo2;
using namespace halo2::sky_audit;
namespace fs=std::filesystem;
void need(bool value,const std::string& message){if(!value)throw FormatError(message);}
struct Pair {const char* map;const char* original;const char* shader;};
constexpr Pair pairs[]{
    {"needle","assets/map-pack/maps/dune.map","scenarios\\shaders\\multi\\dune\\dune_island"},
    {"needle","assets/map-pack/maps/dune.map","scenarios\\shaders\\multi\\dune\\dune_planet"},
    {"needle","assets/map-pack/maps/dune.map","scenarios\\shaders\\multi\\dune\\dune_ring"},
    {"needle","assets/map-pack/maps/dune.map","scenarios\\skies\\multi\\zanzibar\\shaders\\clouds_low"},
    {"needle","assets/map-pack/maps/dune.map","scenarios\\skies\\multi\\zanzibar\\shaders\\sky_dome"},
    {"street_sweeper","assets/disc/maps/03a_oldmombasa.map","scenarios\\skies\\solo\\earthcity\\shaders\\clouds_low2"},
    {"street_sweeper","assets/disc/maps/03a_oldmombasa.map","scenarios\\skies\\solo\\earthcity\\shaders\\ec_sky_dome2"},
    {"street_sweeper","assets/disc/maps/03a_oldmombasa.map","scenarios\\skies\\solo\\earthcity\\shaders\\ec_sky_tether_faded"},
    {"street_sweeper","assets/disc/maps/03a_oldmombasa.map","scenarios\\skies\\solo\\earthcity\\shaders\\ec_sky_tether_tile"},
};
const Tag& shader(const Cache& cache,const char* name) {
    const Tag* found=nullptr;
    for(const auto& tag:cache.tags())if(tag.group=="shad" && tag.name==name) {
        need(!found,"Ambiguous actual PC sky shader owner");found=&tag;
    }
    need(found && found->datum!=0xffffffffu && found->address && found->size,"Actual sky shader has no complete typed owner");
    return *found;
}
Material material(const Cache& cache,const Tag& tag,bool native) {
    if(native)need(cache.is_preserved_native_shader(tag),"PC sky shader lacks authenticated native ownership");
    auto value=cache.load_material(tag.datum);
    prepare_model_environment(cache,value);
    Mesh mesh;mesh.materials.push_back(std::move(value));prepare_forward_materials(cache,mesh);
    value=std::move(mesh.materials.front());
    need(value.source_shader_datum==tag.datum,"Sky material fallback lost its actual salted shader owner");
    need(value.forward_pass && value.forward_pass_index<value.passes.size(),"Sky material has no selected native forward pass");
    need(source_material_layer(value)==std::optional<unsigned>{15},"Sky material did not select its authored dedicated layer15");
    const auto& pass=value.passes[value.forward_pass_index];
    need(!value.vertex_program.empty() && !pass.vertex_code.empty() && decode_vertex_program(value.vertex_program).count>0,
        "Sky material has no complete source vertex program");
    need(texture_program_supported(value.forward_program),"Sky material selected an unsupported source pixel program");
    for(const auto& gap:value.rendering_gaps)
        need(!gap.starts_with("forward texture:") && !gap.starts_with("forward pass requires an external texture:"),
            "Sky material has an unresolved selected native texture: "+gap);
    return value;
}
json differences(const json& a,const json& b) {
    json result=json::object();
    for(auto it=a.begin();it!=a.end();++it)if(!b.contains(it.key()) || it.value()!=b.at(it.key()))
        result[it.key()]={{"original",it.value()},{"prepared",b.value(it.key(),json(nullptr))}};
    for(auto it=b.begin();it!=b.end();++it)if(!a.contains(it.key()))result[it.key()]={{"original",nullptr},{"prepared",it.value()}};
    return result;
}
void compare_textures(const Material& a,const Material& b,const std::string& path,json& failures,unsigned& count,unsigned depth=0) {
    need(depth<32,"Sky material nested pass graph exceeds bounded audit depth");
    const auto compare=[&](const Texture& x,const Texture& y,const std::string& label) {
        ++count;
        if(!texture_equal(x,y))failures.push_back({{"path",label},{"original",texture(x)},{"prepared",texture(y)}});
    };
    const std::array<const Texture*,4> x{&a.diffuse,&a.detail,&a.bump,&a.auxiliary},y{&b.diffuse,&b.detail,&b.bump,&b.auxiliary};
    for(unsigned slot=0;slot<4;++slot)compare(*x[slot],*y[slot],path+"/bound/"+std::to_string(slot));
    for(unsigned stage=0;stage<a.animated_textures.size();++stage) {
        if(a.animated_textures[stage].size()!=b.animated_textures[stage].size()) {
            failures.push_back({{"path",path+"/animated/"+std::to_string(stage)},{"original_frames",a.animated_textures[stage].size()},
                {"prepared_frames",b.animated_textures[stage].size()}});continue;
        }
        for(unsigned frame=0;frame<a.animated_textures[stage].size();++frame)
            compare(a.animated_textures[stage][frame],b.animated_textures[stage][frame],path+"/animated/"+std::to_string(stage)+"/"+std::to_string(frame));
    }
    const auto pass=[&](const std::shared_ptr<Material>& x,const std::shared_ptr<Material>& y,const std::string& label) {
        if(bool(x)!=bool(y)){failures.push_back({{"path",label},{"original_present",bool(x)},{"prepared_present",bool(y)}});return;}
        if(x)compare_textures(*x,*y,label,failures,count,depth+1);
    };
    pass(a.environment_pass,b.environment_pass,path+"/environment");pass(a.planar_fog_pass,b.planar_fog_pass,path+"/planar_fog");
    const auto vector=[&](const auto& x,const auto& y,const std::string& label) {
        if(x.size()!=y.size()){failures.push_back({{"path",label},{"original_passes",x.size()},{"prepared_passes",y.size()}});return;}
        for(unsigned index=0;index<x.size();++index)pass(x[index],y[index],label+"/"+std::to_string(index));
    };
    vector(a.world_overlays_before,b.world_overlays_before,path+"/overlays_before");
    vector(a.world_overlays_after,b.world_overlays_after,path+"/overlays_after");
    vector(a.world_bloom_passes,b.world_bloom_passes,path+"/bloom");vector(a.world_planar_passes,b.world_planar_passes,path+"/world_planar");
}
}
int main(int argc,char** argv)try {
    need(argc==4,"usage: audit_pc_sky_materials PROJECT PREPARED_ROOT REPORT");
    const fs::path root=argv[1],prepared=argv[2],output=argv[3];
    json report={{"schema","H2_PC_SKY_ACTUAL_NATIVE_MATERIAL_AUDIT_V1"},{"scope","Actual Cache native material/texture equality; runtime Vulkan upload observation belongs to separate GPU acceptance"},
        {"materials",json::array()}};
    std::map<std::string,std::unique_ptr<Cache>> originals,actuals;unsigned exact{},texture_pairs{},errors{};
    for(const auto& pair:pairs) {
        json row={{"map",pair.map},{"original_map",pair.original},{"shader",pair.shader}};
        try {
            if(!originals.contains(pair.original))originals[pair.original]=std::make_unique<Cache>(root/pair.original);
            const auto path=prepared/"halo2/h2_maps_win64_dx11"/(std::string(pair.map)+".map");
            if(!actuals.contains(pair.map))actuals[pair.map]=std::make_unique<Cache>(path,path.parent_path(),nullptr,prepared);
            const auto& original=*originals.at(pair.original);const auto& actual=*actuals.at(pair.map);
            need(actual.is_mcc() && !original.is_mcc(),"Actual/original PC sky audit format ownership differs");
            const auto& xowner=shader(original,pair.shader);const auto& yowner=shader(actual,pair.shader);
            const auto x=material(original,xowner,false),y=material(actual,yowner,true);
            row["original_datum"]=xowner.datum;row["prepared_datum"]=yowner.datum;
            row["native_forward_admitted"]=true;row["vertex_program_bytes"]=y.vertex_program.size();
            row["original_semantic_sha256"]=semantic_digest(x,original);row["prepared_semantic_sha256"]=semantic_digest(y,actual);
            row["semantic_differences"]=differences(complete_semantic(x,original),complete_semantic(y,actual));
            row["texture_differences"]=json::array();unsigned count{};compare_textures(x,y,"material",row["texture_differences"],count);
            row["direct_texture_pairs"]=count;texture_pairs+=count;
            row["equal"]=row["semantic_differences"].empty() && row["texture_differences"].empty();
            if(row["equal"].get<bool>())++exact;
        }catch(const std::exception& error){row["error"]=error.what();row["equal"]=false;++errors;}
        report["materials"].push_back(std::move(row));
    }
    report["native_materials_exact"]=exact;report["materials_checked"]=std::size(pairs);report["direct_texture_pairs"]=texture_pairs;
    report["errors"]=errors;report["passed"]=exact==std::size(pairs) && errors==0;
    std::ofstream file(output);file<<report.dump(2)<<'\n';need(bool(file),"Cannot save actual PC sky material audit");
    std::cout<<report["native_materials_exact"]<<"/9 actual native PC sky materials exact, "<<texture_pairs<<" direct bound/animated texture pairs\n";
    return report["passed"].get<bool>()?0:1;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
