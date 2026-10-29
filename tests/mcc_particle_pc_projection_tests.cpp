#include "halo2/cache.hpp"
#include "halo2/mcc_particle_pc_projection.hpp"
#include "halo2/particle_material.hpp"
#include "halo2/particle_material_validation.hpp"
#include "halo2/particle_model.hpp"
#include "halo2/particle_draw_resource_validation.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <set>

using namespace halo2;
namespace {
void check(bool value,const char* message){if(!value)throw FormatError(message);}
void controls() {
    constexpr std::string_view stem="shaders\\shader_templates\\transparent\\particle_additive_tint";
    MaterialPass pc;pc.name="shaders\\shader_passes\\transparent\\effect_shader_additive_tint";
    pc.bitmap_remaps={{0,0,0}};pc.pixel_remaps={{97,0,0}};
    pc.vertex_remaps={{432,0,0},{7279,0,1}};pc.textures={{0,0,0,0},{0,0,1,0}};
    pc.states={{168,7},{27,1},{19,5},{20,2},{171,1},{14,0},{175,0},{22,1}};
    std::array<std::byte,48> samplers{};
    for(unsigned at=0;at<48;at+=24){samplers[at]=std::byte{32};samplers[at+1]=std::byte{34};
        for(unsigned byte=2;byte<6;++byte)samplers[at+byte]=std::byte{1};}
    const std::array<std::byte,5> defaults{std::byte{22},std::byte{2}};
    const auto projected=project_mcc_particle_pc_pass(stem,pc,{1,1,2},samplers,defaults);
    check(projected.pixel_remaps.size()==1 && projected.pixel_remaps[0].destination==512 &&
        projected.pixel_remaps[0].component==3 && projected.pixel_remaps[0].source==0,
        "PC tint parameter did not reach the existing custom fragment alpha input");
    const auto reject=[&](auto mutate) {
        auto changed=pc;auto altered=samplers;mutate(changed,altered);bool rejected{};
        try{(void)project_mcc_particle_pc_pass(stem,changed,{1,1,2},altered,defaults);}
        catch(const FormatError&){rejected=true;}check(rejected,"Changed PC particle contract was admitted");
    };
    reject([](auto& p,auto&){p.definition.words[0]=1;});
    reject([](auto& p,auto&){p.states[2].value=2;});
    reject([](auto& p,auto&){p.textures[1].external=23;});
    reject([](auto& p,auto&){p.pixel_remaps[0].source=1;});
    reject([](auto& p,auto&){p.vertex_remaps[1].destination^=1;});
    reject([](auto& p,auto&){p.state_externs.push_back({0,0,0});});
    reject([](auto& p,auto&){p.implementation=1;});
    reject([](auto&,auto& s){s[24+2]^=std::byte{1};});
    bool rejected{};try{(void)project_mcc_particle_pc_pass(stem,pc,{1,0,2},samplers,defaults);}
    catch(const FormatError&){rejected=true;}check(rejected,"Missing PC tint constants were admitted");
    rejected=false;try{(void)mcc_particle_pc_vertex_signature(stem,pc.name,0,7,
        "rasterizer\\vertex_shaders_dx9\\particle_model",2);}
    catch(const FormatError&){rejected=true;}check(rejected,"A different PC vertex owner entered the sprite kernel");
}
}
int main(int argc,char** argv)try {
    controls();
    if(argc==2 && std::string_view(argv[1])=="--unit") {
        std::cout<<"PASS PC particle projection and ten changed-content controls\n";return 0;
    }
    check(argc==3,"Supply a prepared PC-only MCC map and owned JSON report");
    if(!*argv[1] || !std::filesystem::is_regular_file(argv[1]))return 77;
    Cache cache(argv[1],std::filesystem::path(argv[1]).parent_path());
    check(cache.is_mcc(),"PC projection regression requires MCC content");
    check(!cache.query_tag({.group=tag_groups::vertex_shader,
        .name="rasterizer\\vertex_shaders_xbox\\particle",.require_address=true}),
        "PC-only fixture unexpectedly contains a retained source vertex namespace");
    unsigned resources{},models{},distortion{};std::set<std::pair<std::string,unsigned>> profiles;
    for(const auto& tag:cache.tags()) {
        if(!tag.address || (tag.group!="prt3" && tag.group!="PRTM"))continue;
        const bool model=tag.group=="PRTM";
        const auto material=cache.load_particle_material(tag.datum,15,0,model?1:0);
        const auto contract=validate_particle_material(material);++resources;
        check(material.vertex.vertex_datum==0xffffffffu,"PC projection fabricated a source vertex tag");
        profiles.insert({material.pass.name,material.pass.implementation});
        if(model) {
            ++models;check(material.pass.vertex_declaration &&
                material.pass.vertex_declaration->semantics==std::vector<std::uint16_t>{0,3,14},
                "Custom model kernel lost its explicit position/UV/radiance interface");
            validate_particle_draw_resource(load_particle_model(cache,tag.datum));
        }
        if(contract.distortion) {
            ++distortion;check(contract.external_scene_texture && material.sampler_writes.size()==4,
                "PC distortion lost its real framebuffer/sampler producer contract");
        }
    }
    check(resources>=190 && models>=3 && distortion>=1 && profiles.size()>=9,
        "Actual PC map no longer exercises the complete discovered particle family set");
    nlohmann::json report{{"passed",true},{"map",cache.name()},{"resources",resources},
        {"models",models},{"distortion",distortion},{"pass_implementation_pairs",profiles.size()},
        {"source_vertex_tag_fabricated",false},{"negative_controls",10},
        {"scope","Every actual resident PC-only particle material and model resource; no framebuffer or simulation claim"}};
    std::ofstream output(argv[2]);check(bool(output),"Cannot write PC projection report");
    output<<report.dump(2)<<'\n';output.close();check(bool(output),"Cannot finish PC projection report");
    std::cout<<"PASS "<<resources<<" actual PC-only MCC particle materials, "<<models<<" models, "
        <<distortion<<" distortion resources\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
