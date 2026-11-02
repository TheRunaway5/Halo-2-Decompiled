// One-variable actual Cache/resource-history diagnostic; no authentication bypass.
#include "halo2/cache.hpp"
#include "halo2/appearance.hpp"
#include "halo2/content.hpp"
#include "halo2/mcc_sky_geometry.hpp"
#include "halo2/sky_input_identity.hpp"
#include "halo2/source_machine_scene.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
using namespace halo2;using Json=nlohmann::json;
RenderModel raw_materials(const Cache&cache,std::uint32_t model) {
    RenderModel raw;raw.name=cache.find_tag({model})->name;const Bytes root(cache.tag_payload({model}));const auto count=root.u32(0x60),address=root.u32(0x64);raw.mesh.materials.resize(count+1);
    for(unsigned i=0;i<count;++i){const auto shader=Bytes(cache.read_tag_bytes(address+i*32,32)).u32(12);raw.mesh.materials[i]=cache.load_material(shader);prepare_model_environment(cache,raw.mesh.materials[i]);}
    return raw;
}
int main(int argc,char**argv)try {
    if(argc!=4&&argc!=5)throw FormatError("Supply MCC Armory map, fresh|bsp|machines|bsp-machines, report, optional original Armory map");
    const std::string mode=argv[2];if(mode!="fresh"&&mode!="bsp"&&mode!="machines"&&mode!="bsp-machines")throw FormatError("Unknown history prefix");
    const Cache cache(argv[1],std::filesystem::path(argv[1]).parent_path());if(!cache.is_mcc())throw FormatError("Actual MCC inputs required");
    Mesh prefix;if(mode=="bsp"||mode=="bsp-machines")prefix=cache.load_bsp(0,true);
    std::unique_ptr<SourceMachineScene> machines;
    if(mode=="machines"||mode=="bsp-machines"){const auto content=cache.load_content(0,false);machines=SourceMachineScene::campaign_resources(cache,content.objects,0,prefix);}
    const auto catalog=Json::parse(mcc_sky_geometry_profiles());Json rows=Json::array();unsigned failures{};
    std::unique_ptr<Cache> original;if(argc==5)original=std::make_unique<Cache>(std::filesystem::canonical(argv[4]),std::filesystem::canonical(std::filesystem::path(argv[4]).parent_path()));
    for(const auto&sky:cache.select_tags({.group=tag_groups::sky})) {
        const auto sky_payload=cache.tag_payload(sky.handle());if(sky_payload.size()<8)continue;
        const auto model=Bytes(sky_payload).u32(4);if(model==null_datum)continue;
        const auto input=mcc_sky_geometry_input_sha256(cache,model);
        const auto profile=std::ranges::find_if(catalog.at("profiles"),[&](const auto&entry){return entry.at("input_sha256")==input;});
        if(profile==catalog.at("profiles").end())continue;
        Json row={{"sky",sky.name},{"model",model},{"model_name",cache.find_tag({model})->name},{"input_sha256",input},{"profile_original_map",profile->at("original_map")},{"materials",Json::array()}};
        RenderModel raw;raw.name=cache.find_tag({model})->name;const Bytes root(cache.tag_payload({model}));const auto count=root.u32(0x60),address=root.u32(0x64);raw.mesh.materials.resize(count+1);
        for(unsigned i=0;i<count;++i){const auto shader=Bytes(cache.read_tag_bytes(address+i*32,32)).u32(12);try{raw.mesh.materials[i]=cache.load_material(shader);prepare_model_environment(cache,raw.mesh.materials[i]);}catch(const FormatError&error){row["material_load_errors"].push_back({{"index",i},{"shader",shader},{"error",error.what()}});}}
        Mesh prepared;prepared.materials=raw.mesh.materials;prepare_forward_materials(cache,prepared);
        Mesh original_prepared;
        if(original){auto native=raw_materials(*original,profile->value("native_material_model_datum",profile->at("original_model_datum").get<std::uint32_t>()));original_prepared.materials=std::move(native.mesh.materials);prepare_forward_materials(*original,original_prepared);}
        for(const auto&mapping:profile->at("materials")) {
            const auto index=mapping.at("mcc_index").get<unsigned>();const auto&material=prepared.materials.at(index);
            const auto semantic=sky_audit::complete_semantic(material,cache);const auto digest=sky_audit::semantic_digest(material,cache);const bool matches=digest==mapping.at("mcc_semantic_sha256").get<std::string>();
            row["materials"].push_back({{"mcc_index",index},{"name",mapping.at("name")},{"shader",mapping.value("mcc_shader",0xffffffffu)},
                {"expected_digest",mapping.at("mcc_semantic_sha256")},{"actual_digest",digest},{"matches",matches},{"gaps",material.rendering_gaps},{"semantic",semantic}});
            if(original){const auto authority=mapping.value("native_authority_material_index",unsigned(row["materials"].size()-1));const auto&native=original_prepared.materials.at(authority);auto&report=row["materials"].back();report["original_digest"]=sky_audit::semantic_digest(native,*original);report["original_semantic"]=sky_audit::complete_semantic(native,*original);report["original_to_mcc_diff"]=Json::diff(report["original_semantic"],semantic);}
            if(!matches)++failures;
        }
        try{row["restored"]=restore_mcc_sky_geometry_profile(cache,model,raw,true,profile->dump());}catch(const FormatError&error){row["restore_error"]=error.what();}
        rows.push_back(std::move(row));
    }
    Json report={{"mode",mode},{"profiles",rows},{"mismatched_materials",failures},{"history_prefix_materials",prefix.materials.size()},
        {"genuine_machine_models",machines?machines->stats().render_models:0},{"scope","Fresh actual Cache versus real authored load histories; exact model-local authentication remains enabled"}};
    std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';if(!out)throw FormatError("Sky diagnosis report failed");
    std::cout<<"profiles="<<rows.size()<<" mismatched_materials="<<failures<<" prefix_materials="<<prefix.materials.size()<<'\n';return failures?1:0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 2;}
