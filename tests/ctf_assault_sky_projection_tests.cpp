#include "halo2/mcc_sky_geometry.hpp"
#include "halo2/sky_input_identity.hpp"
#include "halo2/appearance.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>

using namespace halo2;
int main(int argc,char** argv)try {
    if(argc!=4)throw FormatError("Supply the actual prepared MCC map, report path and original asset root");
    const std::filesystem::path path=argv[1];
    if(!std::filesystem::is_regular_file(path))return 77;
    nlohmann::json report={{"passed",false},{"map",path.string()},{"stage","actual sky admission"}};
    const auto save=[&]{std::ofstream out(argv[2]);out<<report.dump(2)<<'\n';};
    try {
        Cache cache(path,path.parent_path());
        if(!cache.is_mcc())throw FormatError("Runtime sky regression requires prepared MCC inputs");
        const auto sky=cache.load_sky(0);
        if(!sky)throw FormatError("Objective regression map has no actual sky zero");
        report["model"]=sky->model;report["stage"]="ordinary textured sky projection";
        // The normal model loader must authenticate the prepared material and
        // texture semantics. Do not omit textures or bypass the projection.
        const auto model=cache.load_model(sky->model,true);
        if(model.mesh.vertices.empty())throw FormatError("Actual objective sky emitted no geometry");
        report["vertices"]=model.mesh.vertices.size();report["materials"]=model.mesh.materials.size();
        report["stage"]="independent original Xbox geometry/material comparison";
        const auto input=mcc_sky_geometry_input_sha256(cache,sky->model);
        const auto catalog=nlohmann::json::parse(mcc_sky_geometry_profiles());
        const auto profile=std::ranges::find_if(catalog.at("profiles"),[&](const auto& p){return p.at("input_sha256")==input;});
        if(profile==catalog.at("profiles").end())throw FormatError("Actual objective sky lacks an authenticated original projection");
        const std::filesystem::path root=argv[3];
        auto original_path=root/"assets/disc/maps"/profile->at("original_map").get<std::string>();
        if(!std::filesystem::is_regular_file(original_path))original_path=root/"assets/map-pack/maps"/profile->at("original_map").get<std::string>();
        Cache original(original_path,root/"assets/disc/maps");
        if(original.is_mcc() || original.is_vista())throw FormatError("Objective sky authority must be original Xbox data");
        const auto datum=profile->at("original_model_datum").get<std::uint32_t>();
        if(mcc_sky_geometry_input_sha256(original,datum)!=profile->at("original_model_input_sha256").get<std::string>())
            throw FormatError("Independent original Xbox sky model identity differs");
        auto reference=original.load_model(datum,true);
        if(encode_mcc_sky_geometry(reference)!=encode_mcc_sky_geometry(model))
            throw FormatError("Objective sky geometry differs from original Xbox source words");
        Mesh actual;actual.materials=model.mesh.materials;
        prepare_forward_materials(cache,actual);prepare_forward_materials(original,reference.mesh);
        if(reference.mesh.materials.size()!=actual.materials.size())throw FormatError("Original objective sky material extent differs");
        for(unsigned i=0;i<actual.materials.size();++i) {
            const auto expected=sky_audit::complete_semantic(reference.mesh.materials[i],original);
            const auto observed=sky_audit::complete_semantic(actual.materials[i],cache);
            if(expected!=observed) {
                report["material_slot"]=i;report["semantic_differences"]=nlohmann::json::diff(expected,observed);
                throw FormatError("Objective sky native materials or complete textures differ from original Xbox authority");
            }
        }
        report["original_geometry_exact"]=true;report["original_complete_materials_exact"]=true;
        report["original_model_input_sha256"]=profile->at("original_model_input_sha256");
        report["passed"]=true;report["stage"]="complete";save();return 0;
    }catch(const std::exception& e){report["error"]=e.what();save();throw;}
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
