#include "halo2/pstencil/environment.hpp"
#include "halo2/pstencil/detail.hpp"
#include "halo2/pstencil/specular_mask.hpp"
#include "halo2/object_lighting.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
using namespace halo2;
using Json=nlohmann::json;
namespace {
Json describe(const Material& m) {
    Json out{{"vertex",m.forward_vertex},{"blend",m.forward_blend},
        {"source_environment_response",pstencil::supports_environment_response(m)},
        {"combined_environment_strength",pstencil::supports_combined_environment_strength(m)},
        {"texture_mask",pstencil::texture_mask_pass(m)},
        {"neutral_elite_detail",bool(pstencil::reference_neutral_detail_stage(m))}};
    if(m.forward_pass && m.forward_pass_index<m.passes.size())out["pass"]=m.passes[m.forward_pass_index].name;
    if(m.environment_pass)out["environment"]=describe(*m.environment_pass);
    for(const auto& p:m.world_overlays_before)if(p)out["overlays_before"].push_back(describe(*p));
    for(const auto& p:m.world_overlays_after)if(p)out["overlays_after"].push_back(describe(*p));
    if(!m.rendering_gaps.empty())out["rendering_gaps"]=m.rendering_gaps;
    return out;
}
}
int main(int argc,char**argv){try {
    if(argc!=3)throw FormatError("Supply current campaign cache and new JSON output");
    if(std::filesystem::exists(argv[2]))throw FormatError("Campaign material evidence already exists");
    Cache cache(argv[1]);Json report{{"map",argv[1]},
        {"scope","Every resident material and shader LOD, using current cinematic classification 2. Resource/adaptation audit, not live scene/part admission or original framebuffer parity."},
        {"materials",Json::array()},{"errors",Json::array()}};
    for(const auto& tag:cache.tags())if(tag.group=="shad" && tag.address) {
        try {
            const auto authored=cache.load_material(tag.datum);
            Json row{{"name",tag.name},{"datum",tag.datum},
                {"exact_2003_environment_parameters",bool(pstencil::reference_environment_parameters(tag.name))},
                {"lods",Json::array()}};
            for(unsigned lod=0;lod<authored.lods.size();++lod) {
                auto material=authored;material.selected_shader_lod=lod;
                Json entry{{"lod",lod},{"layers",authored.lods[lod].available_layers}};
                try {
                    const bool prepared=prepare_dynamic_material(cache,material,{},true,0.f);
                    entry["cinematic_prepared"]=prepared;
                    if(prepared)entry["response"]=describe(material);
                }catch(const std::exception& e){entry["error"]=e.what();}
                row["lods"].push_back(std::move(entry));
            }
            report["materials"].push_back(std::move(row));
        }catch(const std::exception& e){report["errors"].push_back({{"name",tag.name},{"error",e.what()}});}
    }
    std::ofstream(argv[2])<<report.dump(2)<<'\n';
    std::cout<<report["materials"].size()<<" resident materials, "<<report["errors"].size()<<" load errors\n";
    return report["errors"].empty()?0:1;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
