#include "halo2/pstencil/caster.hpp"
#include "halo2/pstencil/geometry.hpp"
#include "halo2/source_bsp_admission.hpp"
#include "halo2/source_effect_light_opaque.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace halo2;
using Json=nlohmann::json;
namespace {
// This is a resource/declaration compatibility audit. Identity rows deliberately
// exercise stored bone-index bounds; they are not a gameplay pose or admission.
constexpr std::array<float,12> identity{1,0,0,0,0,1,0,0,0,0,1,0};
void audit(const std::filesystem::path& path,const std::filesystem::path& output) {
    Cache cache(path.string());Json report{{"map",path.string()},{"scope",
        "All resident raw sections; synthetic identity palettes and model owner token 0 check resource compatibility only. No view, live owner, pose or historical admission is inferred."},
        {"sections",Json::array()},{"errors",Json::array()}};
    const auto shader=std::ranges::find_if(cache.tags(),[](const auto& tag){
        return tag.group=="vrtx" && tag.name.ends_with("\\lightaccum_sphere_diffuse");
    });
    if(shader==cache.tags().end())throw FormatError("source position program absent");
    const auto vrtx=cache.load_source_tag_metadata(shader->datum);
    const auto classes=Bytes(vrtx.bytes_at(vrtx.source_address(),12)).u32(4);
    report["vertex_shader"]=shader->name;
    auto section=[&](MenuWorldSourceSectionKind kind,std::uint32_t model,unsigned bsp,unsigned ordinal,
                     std::string owner) {
        Json entry{{"kind",unsigned(kind)},{"owner",owner},{"datum",model},{"bsp",bsp},{"section",ordinal}};
        try {
            const auto raw=std::make_shared<const MenuWorldSourceSection>(kind==MenuWorldSourceSectionKind::render_model?
                load_menu_world_source_model_section(cache,model,ordinal):load_menu_world_source_bsp_section(cache,bsp,kind,ordinal));
            std::array<unsigned,6> counts{};unsigned first=~0u;
            for(unsigned p=0;p<raw->parts().size();++p) {
                const auto& part=raw->parts()[p];
                if(part.type>=counts.size())throw FormatError("unknown authored part type");
                ++counts[part.type];
                if(first==~0u && pstencil::shadow_casting_part(part.type) && part.index_count>=3)first=p;
            }
            entry["part_type_counts"]=counts;entry["vertices"]=raw->vertex_count();
            if(first!=~0u) {
                const auto meta=kind==MenuWorldSourceSectionKind::render_model?
                    load_source_opaque_model_geometry_metadata(cache,model,ordinal):
                    load_source_opaque_bsp_geometry_metadata(cache,bsp,kind,ordinal);
                const auto cls=source_opaque_vertex_class(meta.geometry_class,meta.selectors[0],[&]{return classes;});
                // Topology is explicit audit input. This checks declaration and
                // stored resources; actual native draw topology remains separate.
                std::vector<std::array<float,12>> palette(kind==MenuWorldSourceSectionKind::bsp_cluster?0:40,identity);
                auto draw=kind==MenuWorldSourceSectionKind::bsp_cluster?
                    menu_world_source_cluster_draw(raw,first,MenuWorldSourceTopology::triangle_strip):
                    menu_world_source_draw(raw,first,MenuWorldSourceTopology::triangle_strip,palette);
                const auto owner_kind=kind==MenuWorldSourceSectionKind::render_model?MenuWorldSourceOwnerKind::dynamic_object:
                    kind==MenuWorldSourceSectionKind::bsp_cluster?MenuWorldSourceOwnerKind::bsp_cluster:MenuWorldSourceOwnerKind::bsp_instance;
                // Instance owner validation requires a genuine instance ordinal.
                unsigned instance=ordinal;
                if(kind==MenuWorldSourceSectionKind::bsp_instance_definition) {
                    const auto admission=cache.load_source_bsp_admission(bsp);
                    const auto found=std::ranges::find_if(admission.instances,[&](const auto& i){return i.definition==int(ordinal);});
                    if(found==admission.instances.end()) {
                        entry["binding"]="uninstantiated definition; no native owner";
                        report["sections"].push_back(std::move(entry));return;
                    }
                    instance=unsigned(found-admission.instances.begin());
                }
                const MenuWorldSourceOwner binding_owner{owner_kind,raw->owner_datum(),
                    kind==MenuWorldSourceSectionKind::render_model?0u:instance,
                    kind==MenuWorldSourceSectionKind::render_model?std::optional<std::uint32_t>(0):std::nullopt};
                const auto binding=menu_world_source_binding_class(cache,binding_owner,
                    std::move(draw),shader->datum,cls,{});
                const auto positions=pstencil::world_positions(binding);
                entry["binding"]="pass";entry["vertex_class"]=cls;entry["position_count"]=positions.size();
            } else entry["binding"]="no indexed caster part";
            report["sections"].push_back(std::move(entry));
        } catch(const std::exception& e) {
            entry["error"]=e.what();report["errors"].push_back(std::move(entry));
        }
    };
    for(unsigned bsp=0;bsp<cache.bsps().size();++bsp) {
        const auto admission=cache.load_source_bsp_admission(bsp);
        for(unsigned n=0;n<admission.clusters.size();++n)if(admission.clusters[n].geometry_count)
            section(MenuWorldSourceSectionKind::bsp_cluster,0xffffffffu,bsp,n,"BSP cluster");
        for(unsigned n=0;n<admission.definitions.size();++n)if(admission.definitions[n].geometry_count)
            section(MenuWorldSourceSectionKind::bsp_instance_definition,0xffffffffu,bsp,n,"BSP instance definition");
    }
    for(const auto& tag:cache.tags())if(tag.group=="mode" && tag.address) {
        const auto owned=cache.load_source_tag_metadata(tag.datum);
        const auto count=Bytes(owned.bytes_at(owned.source_address(),0x2c)).u32(0x24);
        for(unsigned n=0;n<count;++n)section(MenuWorldSourceSectionKind::render_model,tag.datum,0,n,tag.name);
    }
    std::ofstream(output)<<report.dump(2)<<'\n';
    std::cout<<path.filename()<<": "<<report["sections"].size()<<" sections, "<<report["errors"].size()<<" unresolved resources\n"<<std::flush;
}
}
int main(int argc,char** argv){try {
    if(argc<3)throw FormatError("supply output directory and one or more current maps");
    std::filesystem::create_directories(argv[1]);
    for(int n=2;n<argc;++n) {
        const std::filesystem::path path(argv[n]);
        audit(path,std::filesystem::path(argv[1])/(path.stem().string()+".json"));
    }
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
