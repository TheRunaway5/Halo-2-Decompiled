#include "halo2/cache.hpp"
#include "halo2/source_vertex_inputs.hpp"
#include "halo2/vertex_program.hpp"
#include "halo2/world_material.hpp"
#include "halo2/source_decorator_decal_material.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <map>

using namespace halo2;
namespace {
void check(bool value,const char* message){if(!value)throw FormatError(message);}
std::uint64_t fingerprint(std::span<const std::byte> bytes) {
    std::uint64_t value=14695981039346656037ull;
    for(const auto byte:bytes)value=(value^std::to_integer<unsigned>(byte))*1099511628211ull;
    return value;
}
int export_programs(const std::filesystem::path& input,const std::filesystem::path& output) {
    nlohmann::json source;std::ifstream(input)>>source;
    const auto& cases=source.is_array()?source:source.at("cases");
    std::map<std::filesystem::path,std::vector<nlohmann::json>> maps;
    for(const auto& item:cases)maps[std::filesystem::absolute(item.at("map").get<std::string>())].push_back(item);
    nlohmann::json report{{"passed",true},{"scope","Actual native Cache assembled decorator pixel words; no Python shader assembly, GPU or view admission claim"},
        {"cases",nlohmann::json::array()}};
    for(const auto& [map,items]:maps) {
        const auto shared=items.front().value("shared_maps",map.parent_path().string());
        Cache cache(map,shared);
        for(const auto& item:items) {
            auto row=item;
            try {
                const auto name=item.at("shader_name").get<std::string>();
                const auto* shader=cache.query_tag({.group=tag_groups::shader,.name=name,.require_address=true});
                check(shader,"Actual decorator shader export identity is absent");
                if(item.contains("shader_datum"))check(shader->datum==item.at("shader_datum").get<std::uint32_t>(),
                    "Decorator export full salted shader identity differs");
                const auto material=cache.load_material(shader->datum);
                row["resolved_shader"]=shader->datum;row["passes"]=nlohmann::json::array();
                for(const auto& pass:material.passes) {
                    if(pass.name!="shaders\\shader_passes\\overlay\\decorator_decal" &&
                       pass.name!="shaders\\shader_passes\\overlay\\decorator_decal_multiply")continue;
                    check(pass.vertex_code.size()==212 && fingerprint(pass.vertex_code)==0xfdbc61d36bf6be78ull &&
                        pass.vertex_declaration && pass.vertex_declaration->class_index==0 &&
                        pass.vertex_declaration->semantics==std::vector<std::uint16_t>{0,3,9,14},
                        "Decorator export program/declaration differs from authenticated source");
                    auto controls=pass.program.words;
                    for(unsigned word=10;word<26;++word)controls[word]=0;
                    controls[43]=controls[44]=0;
                    row["passes"].push_back({{"pass_name",pass.name},{"implementation",pass.implementation},
                        {"pixel_program",pass.program.words},{"controls",controls}});
                }
                check(!row["passes"].empty(),"Actual export shader has no original decorator pass");
                row["passed"]=true;
            } catch(const std::exception& error) {row["passed"]=false;row["error"]=error.what();report["passed"]=false;}
            report["cases"].push_back(std::move(row));
        }
    }
    std::ofstream stream(output);stream<<report.dump(2)<<'\n';check(bool(stream),"Cannot save native decorator program export");
    std::cout<<"Native decorator program export cases="<<report["cases"].size()<<" passed="<<report["passed"]<<'\n';
    return report["passed"].get<bool>()?0:1;
}
}
int main(int argc,char**argv) {
    nlohmann::json report{{"passed",false},{"scope","Actual Beaver Creek decorator material metadata and explicit native world-pass binding; no geometry admission or pixels claimed"}};
    const auto save=[&] {if(argc==3){std::ofstream output(argv[2]);output<<report.dump(2)<<'\n';}};
    try {
        if(argc==4 && std::string_view(argv[1])=="--export")return export_programs(argv[2],argv[3]);
        check(argc==3,"Supply prepared MCC Beaver Creek MAP REPORT");
        if(!std::filesystem::is_regular_file(argv[1]))return 77;
        Cache cache(argv[1]);check(cache.is_mcc() && cache.name()=="beavercreek","Requires actual prepared MCC Beaver Creek");
        const auto* shader=cache.query_tag({.group=tag_groups::shader,
            .name="scenarios\\decorators\\dirty_interior\\stains",.require_address=true});
        check(shader,"Actual dirty-interior stain shader is absent");
        check(!cache.query_tag({.group=tag_groups::shader,
            .name="scenarios\\decorators\\dirty_interior\\stains_missing",.require_address=true}),
            "Missing decorator shader was replaced by a named fallback");
        auto material=cache.load_material(shader->datum);
        report["map"]=std::filesystem::absolute(argv[1]).string();report["shader"]=shader->name;
        report["shader_datum"]=shader->datum;report["template"]=material.template_name;
        report["raw_forward_pass"]=material.forward_pass;report["raw_rendering_gaps"]=material.rendering_gaps;
        report["passes"]=nlohmann::json::array();
        for(const auto& pass:material.passes) {
            report["passes"].push_back({{"name",pass.name},{"implementation",pass.implementation},
                {"vertex_shader",pass.vertex_shader},{"program_bytes",pass.vertex_code.size()},
                {"program_fingerprint",fingerprint(pass.vertex_code)},
                {"pixel_program",pass.program.words},
                {"semantics",pass.vertex_declaration?pass.vertex_declaration->semantics:std::vector<std::uint16_t>{}},
                {"texture_modes",pass.program.words[54]},
                {"vertex_externs",pass.vertex_externs.size()},{"pixel_externs",pass.pixel_externs.size()}});
        }
        const bool generic=prepare_world_material(cache,material,false);report["generic_world_binding"]=generic;
        material=source_decorator_decal_material_tu15(cache,shader->datum);
        const bool bound=material.forward_pass;report["explicit_decorator_binding"]=bound;
        report["prepared_forward_pass"]=material.forward_pass;report["prepared_vertex"]=material.forward_vertex;
        report["prepared_gaps"]=material.rendering_gaps;
        report["prepared_blend"]=material.forward_blend;
        report["prepared_cull"]=material.forward_raster.cull;
        report["prepared_color_write"]=material.forward_raster.color_write;
        report["prepared_depth_write"]=material.forward_raster.depth_write;
        report["prepared_has_extended_vertex_bank"]=bool(material.vertex_bank);
        save();
        check(bound && material.forward_pass && material.forward_pass_index<material.passes.size(),
            "Actual decorator layer2 has no explicit native material binding");
        const auto& pass=material.passes[material.forward_pass_index];
        check(pass.name=="shaders\\shader_passes\\overlay\\decorator_decal_multiply" &&
            pass.vertex_shader=="rasterizer\\vertex_shaders_xbox\\decorator_decal" &&
            material.vertex_program.size()==212 && material.vertex_declaration &&
            material.vertex_declaration->class_index==0 &&
            material.vertex_declaration->semantics==std::vector<std::uint16_t>{0,3,9,14},
            "Actual source decorator pass/program/declaration identity changed");
        check(material.forward_program.words[54]==1,"Actual decorator texture modes require a separate TEX1 input owner");
        check(!material.diffuse.pixels.empty() && !material.diffuse.mips.empty(),"Actual decorator texture has no resident pixels or mip chain");
        check(decode_vertex_program(material.vertex_program).count==13,"Actual decorator compiled program changed");
        auto wrong=*material.vertex_declaration;wrong.class_index=1;
        bool rejected=false;try{(void)pack_source_vertex_declaration(wrong);}catch(const FormatError&){rejected=true;}
        check(rejected,"Different compiled vertex classification was admitted as a world decorator");
        auto changed=material;changed.forward_program.words[54]|=1u<<5;
        rejected=false;try{validate_source_decorator_decal_material_tu15(changed);}catch(const FormatError&){rejected=true;}
        check(rejected,"Decorator material admitted a live unqualified TEX1 sampler");
        changed=material;changed.vertex_program[4]^=std::byte{1};
        rejected=false;try{validate_source_decorator_decal_material_tu15(changed);}catch(const FormatError&){rejected=true;}
        check(rejected,"Decorator material admitted different original vertex instructions");
        report["negative_missing_shader_rejected"]=true;report["negative_wrong_class_rejected"]=true;
        report["negative_live_tex1_rejected"]=true;report["negative_changed_program_rejected"]=true;
        report["passed"]=true;save();
        std::cout<<"PASS actual MCC stain metadata, native explicit layer2 binding, texture and compiled declaration; fetch/color and geometry remain separate\n";
        return 0;
    } catch(const std::exception& error) {
        report["error"]=error.what();save();std::cerr<<error.what()<<'\n';return 1;
    }
}
