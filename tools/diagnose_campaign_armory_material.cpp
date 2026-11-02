// Additive diagnostic at the actual SourceCampaignModel preparation seam.
// It reads authored MCC data and calls the same native material preparer.
#include "halo2/cache.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/source_prt.hpp"
#include "halo2/original_data.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

int main(int argc,char **argv)try {
    using namespace halo2;using Json=nlohmann::json;
    if(argc!=3&&argc!=4)throw FormatError("Supply actual Armory MCC map and diagnostic JSON; optional TU1.5 XBE is offline reference only");
    const auto original=argc==4?OriginalData::load_xbe(argv[3]):std::shared_ptr<const OriginalData>{};
    if(original&&original->image!=OriginalData::Image::TitleUpdate15)throw FormatError("Expected authenticated TU1.5 reference");
    const Cache cache(argv[1],std::filesystem::path(argv[1]).parent_path(),original);
    if(argc==3&&!cache.is_mcc())throw FormatError("Expected actual MCC runtime inputs");
    const std::string name="scenarios\\objects\\solo\\spacestation\\ss_exterior_gun\\ss_exterior_gun_hinge\\shaders\\panel_mech_tile_ext";
    const auto tag=cache.query_tag({.group=TagGroup::from_string("shad"),.name=name});
    if(!tag)throw FormatError("Actual authored shader tag is absent");
    auto material=cache.load_material(tag->datum);
    Json report={{"scope","Exact authored shader and SourceCampaignModel dynamic preparation arguments; no model, GPU, gameplay, original execution or supplied results"},
        {"shader",name},{"datum",tag->datum},{"template",material.template_name},
        {"authored_gaps",material.rendering_gaps},{"lods",Json::array()},{"passes",Json::array()},
        {"machine_definitions",Json::array()},{"offline_original_reference",bool(original)}};
    for(const auto&candidate:cache.tags())if(candidate.group=="mach"&&
        candidate.name.find("ss_exterior_gun_hinge")!=std::string::npos){
        const auto initial=cache.load_initial_model_definition(candidate.datum);
        const auto prt=cache.load_source_prt_definition(initial.render_model);
        Json owner={{"definition",candidate.datum},{"name",candidate.name},
            {"render_model",initial.render_model},{"region_sections",initial.render_sections},
            {"prt",bool(prt)}};
        if(prt)owner["prt_definition"]={{"order",prt->order},{"clusters",prt->clusters},
            {"pca_vectors",prt->pca_vectors},{"lods",prt->lods.size()},
            {"stream_bytes",prt->stream.size()}};
        report["machine_definitions"].push_back(std::move(owner));
    }
    for(const auto&lod:material.lods)report["lods"].push_back({{"available_layers",lod.available_layers},{"layers_first",lod.layers.first()},{"layers_count",lod.layers.count()}});
    for(const auto&pass:material.passes){Json row={{"name",pass.name},{"vertex_shader",pass.vertex_shader},
        {"template_pass",pass.template_pass},{"implementation",pass.implementation},{"vertex_bytes",pass.vertex_code.size()},
        {"vertex_externs",pass.vertex_externs.size()},{"pixel_externs",pass.pixel_externs.size()},
        {"state_externs",pass.state_externs.size()},{"program_words",pass.program.words.size()}};
        if(pass.vertex_shader.ends_with("\\prt")||pass.vertex_shader.ends_with("\\stencil_texture")){
            const char*digits="0123456789abcdef";std::string raw;
            for(auto byte:pass.vertex_code){const auto value=std::to_integer<unsigned>(byte);raw+=digits[value>>4];raw+=digits[value&15];}
            row["vertex_hex"]=std::move(raw);row["pixel_words"]=pass.program.words;
        }
        report["passes"].push_back(std::move(row));
    }
    const bool prepared=prepare_dynamic_material(cache,material,{},true,0.f);
    report["prepared"]=prepared;report["prepared_gaps"]=material.rendering_gaps;
    report["selected_pass"]=material.forward_pass_index;
    std::ofstream out(argv[2]);out<<report.dump(2)<<'\n';
    if(!out)throw FormatError("Diagnostic report write failed");
    if(!prepared)throw FormatError("Campaign cinematic model shader is unresolved: "+name);
    std::cout<<"PASS exact authored Armory shader native dynamic preparation\n";return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
