// Offline original-asset exporter. Runtime builds consume its finite immutable
// data, and never open original map files or execute this exporter.
#include "halo2/mcc_sky_geometry.hpp"
#include "halo2/sky_input_identity.hpp"
#include "halo2/sky_reference_inventory.hpp"
#include "halo2/sky.hpp"
#include "halo2/appearance.hpp"
#include <nlohmann/json.hpp>
#include <zlib.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <set>

using namespace halo2;using Json=nlohmann::json;namespace fs=std::filesystem;
namespace {
void need(bool ok,const std::string& message){if(!ok)throw FormatError(message);}
Json encoded(const RenderModel& model) {
    const auto raw=encode_mcc_sky_geometry(model);uLongf count=compressBound(raw.size());std::vector<unsigned char> compressed(count);
    need(compress2(compressed.data(),&count,reinterpret_cast<const Bytef*>(raw.data()),raw.size(),Z_BEST_COMPRESSION)==Z_OK,"Cannot compress original sky source data");
    constexpr char digits[]="0123456789abcdef";std::string hex;hex.reserve(count*2);
    for(std::size_t i=0;i<count;++i){hex+=digits[compressed[i]>>4];hex+=digits[compressed[i]&15];}
    const auto round=encode_mcc_sky_geometry(decode_mcc_sky_geometry(raw));need(raw==round,"Exact sky geometry serialization changed source words");
    return {{"sha256",mcc_sky_geometry_sha256(raw)},{"bytes",raw.size()},{"zlib_hex",hex}};
}
std::vector<unsigned> sections(const Cache& cache,std::uint32_t model,std::optional<unsigned> lod) {
    const Bytes root(cache.tag_payload({model}));const auto count=root.u32(0x1c),address=root.u32(0x20);std::vector<unsigned> out;
    for(unsigned i=0;i<count;++i) {
        const Bytes region(cache.read_tag_bytes(address+i*16,16));if(!region.u32(8))continue;
        const Bytes permutation(cache.read_tag_bytes(region.u32(12),16));
        if(lod) {
            const auto section=std::int16_t(permutation.u16(4+*lod*2));if(section>=0)out.push_back(unsigned(section));
        } else for(int p=14;p>=4;p-=2) {
            const auto section=std::int16_t(permutation.u16(p));if(section>=0){out.push_back(unsigned(section));break;}
        }
    }
    if(!lod){std::sort(out.begin(),out.end());out.erase(std::unique(out.begin(),out.end()),out.end());}return out;
}
std::vector<const Tag*> materials(const Cache& cache,std::uint32_t model) {
    const Bytes root(cache.tag_payload({model}));const auto count=root.u32(0x60),address=root.u32(0x64);std::vector<const Tag*> out;
    for(unsigned i=0;i<count;++i) {const Bytes record(cache.read_tag_bytes(address+i*32,32));const auto handle=record.u32(12);
        const auto* tag=cache.find_tag({handle},tag_groups::shader);need(tag,"Source sky material has no actual typed shader owner");out.push_back(tag);}
    out.push_back(nullptr);return out;
}
Mesh material_bank(const Cache& cache,std::span<const Tag* const> tags) {
    Mesh mesh;mesh.materials.resize(tags.size());
    for(unsigned i=0;i<tags.size();++i)if(tags[i])try {
        mesh.materials[i]=cache.load_material(tags[i]->datum);prepare_model_environment(cache,mesh.materials[i]);
    }catch(const FormatError&) {}
    prepare_forward_materials(cache,mesh);return mesh;
}
std::vector<fs::path> maps(const fs::path& directory) {
    std::vector<fs::path> out;if(fs::is_directory(directory))for(const auto& entry:fs::directory_iterator(directory))if(entry.is_regular_file()&&entry.path().extension()==".map")out.push_back(entry.path());
    std::sort(out.begin(),out.end());return out;
}
fs::path mcc_maps(const fs::path& root) {
    for(const auto& dir:{root/"maps",root/"content/halo2/maps",root/"content/halo2/h2_maps_win64_dx11",root/"halo2/maps",root/"halo2/h2_maps_win64_dx11",root/"h2_maps_win64_dx11"})if(fs::is_directory(dir))return dir;
    return root;
}
struct SourceInput {fs::path path;std::string target;bool finite_reference{},shared_needle{},pc_district{};};
}
int main(int argc,char** argv) {
    try {
        need(argc==5||(argc==6&&std::string_view(argv[5])=="--diagnostic"),"Usage: export_mcc_sky_geometry SOURCE_ROOT PREPARED_MCC_ROOT OUTPUT_INC REPORT_JSON [--diagnostic]");
        const bool diagnostic=argc==6;
        const fs::path root=argv[1],prepared=argv[2],output=argv[3],report_path=argv[4];const auto imported=mcc_maps(prepared);
        Json catalog={{"schema","H2_ORIGINAL_SKY_GEOMETRY_V1"},{"profiles",Json::array()}},report={{"schema","H2_ORIGINAL_SKY_GEOMETRY_EXPORT_V1"},{"pairs",Json::array()},{"errors",Json::array()}};
        std::map<std::string,std::string> seen;unsigned pairs{},semantic_differences{};
        std::vector<SourceInput> originals;
        for(const auto& directory:{root/"assets/disc/maps",root/"assets/map-pack/maps"})
            for(const auto& path:maps(directory))originals.push_back({path,path.stem().string()});
        for(const auto& reference:original_sky_reference_inputs(root,prepared)) {
            bool found=false;for(auto& input:originals)if(fs::weakly_canonical(input.path)==fs::weakly_canonical(reference.path)) {
                input.target=reference.authority_map;input.finite_reference=true;found=true;
            }
            if(!found)originals.push_back({reference.path,reference.authority_map,true});
        }
        // Independent census proved every normalized node, region,
        // permutation, LOD section and material in this exact shared model.
        // The complete input fingerprints below permit this one typed rename;
        // an arbitrary PC model with a similar name cannot acquire it.
        const auto dune=root/"assets/map-pack/maps/dune.map";
        need(sky_reference_detail::file_sha256(dune)=="740fff53eb0b0a1eeb80adc33c1c0d08ab9700debb6da59df5d6c23c8e401498",
            "Needle shared sky original Dune file differs from its authenticated source");
        originals.push_back({dune,"needle",false,true});
        const auto district=root/"Halo 2 Vista installer/maps/street_sweeper.map";
        need(sky_reference_detail::file_sha256(district)=="5acceaabef78f65ce5d81fc2d0a813c7a661faaf94b1e5f385e3edf73a6408a0",
            "District original PC11081 geometry file differs from its complete source authority");
        originals.push_back({district,"street_sweeper",false,false,true});
        std::map<fs::path,std::string> original_hashes;
        for(const auto& source_input:originals) {
            const auto& path=source_input.path;const auto target=imported/(source_input.target+".map");if(!fs::is_regular_file(target))continue;
            std::cerr<<"sky geometry: "<<path.filename()<<" -> "<<target.filename()<<'\n';
            try {
                Cache original(path,source_input.pc_district?path.parent_path():root/"assets/disc/maps");
                Cache mcc(target,imported,{},prepared);need(mcc.is_mcc(),"Geometry target is not an MCC cache");
                need(!original.is_mcc()&&original.is_vista()==source_input.pc_district,"Geometry source platform differs from its declared original authority");
                std::unique_ptr<Cache> district_materials;const Cache* material_owner=&original;
                auto material_path=path;
                if(source_input.pc_district) {
                    need(original.is_vista()&&!original.is_mcc()&&original.build()=="11081.07.04.30.0934.main",
                        "District geometry requires the authenticated original PC11081 cache schema");
                    material_path=root/"assets/disc/maps/03a_oldmombasa.map";
                    need(sky_reference_detail::file_sha256(material_path)=="7ea355ef79f13be83fe78546c0fd3b1009ef71c0dd2257b24996420d856f78f4",
                        "District native material authority differs from original Xbox OldMombasa");
                    district_materials=std::make_unique<Cache>(material_path,root/"assets/disc/maps");material_owner=district_materials.get();
                    need(!material_owner->is_mcc()&&!material_owner->is_vista(),"District native material authority cannot be a PC shader cache");
                }
                need(!material_owner->is_mcc()&&!material_owner->is_vista(),"Native shader parsing is restricted to original Xbox material authority");
                if(source_input.finite_reference)validate_sky_reference_pair(original,mcc,{path,source_input.target});
                if(!original_hashes.contains(path))original_hashes[path]=sky_reference_detail::file_sha256(path);
                const auto* scenario=original.find_tag(original.scenario_tag(),tag_groups::scenario);need(scenario,"Original sky scenario is unavailable");
                const auto sky_count=Bytes(original.tag_payload(scenario->handle())).u32(8);
                for(unsigned ordinal=0;ordinal<sky_count;++ordinal) {
                    const auto source_sky=original.load_sky(ordinal);if(!source_sky)continue;
                    const auto target_sky=mcc.load_sky(ordinal);need(target_sky.has_value(),"MCC paired sky is unavailable");
                    const auto* source_tag=original.find_tag({source_sky->model},tag_groups::render_model);const auto* target_tag=mcc.find_tag({target_sky->model},tag_groups::render_model);
                    need(source_tag&&target_tag,"Paired sky ordinal has no typed model owner");
                    const auto input=mcc_sky_geometry_input_sha256(mcc,target_sky->model);++pairs;
                    if(source_input.shared_needle) {
                        need(ordinal==0&&source_tag->name==R"(scenarios\skies\multi\dune\dune)"&&target_tag->name==R"(scenarios\skies\multi\needle\needle)"&&
                            input=="6d6cb2a9053e2ad8dba7981c3669f2f1d30031be3cd244e5e60df0e2a52c0dcb"&&
                            mcc_sky_geometry_input_sha256(original,source_sky->model)=="61c22a9b2a5894a9f223d5a2e75070378f06f1895e39ad4e7e593e2f09b1eea8"&&
                            Bytes(original.tag_payload(source_tag->handle())).u32(8)==303109384u&&Bytes(mcc.tag_payload(target_tag->handle())).u32(8)==303109384u,
                            "Needle shared sky differs from its complete authenticated original/MCC tuple");
                    } else need(source_tag->name==target_tag->name,"Paired sky ordinal does not bind the same typed model name");
                    if(source_input.pc_district)need(ordinal==0&&source_tag->name==R"(scenarios\skies\solo\earthcity\bsp1\bsp1)"&&
                        input=="60e2baee8f6e0544f4417391e39014560e5709045792e58a41911f8c3583fd49"&&
                        mcc_sky_geometry_input_sha256(original,source_sky->model)=="89c65b04709461a0adcf4e9b2cbb863828f4885e76e9e35e40255bde7fde2df9"&&
                        Bytes(original.tag_payload(source_tag->handle())).u32(8)==386535453u&&Bytes(mcc.tag_payload(target_tag->handle())).u32(8)==386535453u,
                        "District original PC/MCC geometry differs from its complete authenticated tuple");
                    // Geometry loading never decodes a Vista shader. A
                    // separate original Xbox cache owns native material data.
                    auto source_model=original.load_model(source_sky->model,false);
                    if(source_input.pc_district)need(mcc_sky_geometry_sha256(encode_mcc_sky_geometry(source_model))==
                        "5116b73dda6d4b01dbdb5fe00c97ee984164879afddb8360411c22b0b581b896",
                        "District original PC geometry output differs from its literal source state");
                    const auto* authority_tag=material_owner->query_tag({.group=tag_groups::render_model,.name=source_tag->name,.require_address=true});
                    need(authority_tag,"Original native material authority has no exact typed model owner");
                    const auto authority_input=mcc_sky_geometry_input_sha256(*material_owner,authority_tag->datum);
                    if(source_input.pc_district)need(authority_input=="a7ff8459ac9cdcf7269e9c2703175fddb8e86f9b47d687e71887bcba1096c6cf"&&
                        Bytes(material_owner->tag_payload(authority_tag->handle())).u32(8)==471007761u,
                        "District TU1.5-compatible Xbox native material authority differs");
                    const auto source_materials=materials(original,source_sky->model),target_materials=materials(mcc,target_sky->model);
                    const auto authority_materials=materials(*material_owner,authority_tag->datum);
                    if(source_input.pc_district) {
                        need(source_materials.size()==5&&authority_materials.size()==source_materials.size()&&target_materials.size()==source_materials.size(),
                            "District geometry/native material authority has another complete slot bank");
                        for(unsigned i=0;i+1<source_materials.size();++i)need(source_materials[i]->name==authority_materials[i]->name&&source_materials[i]->name==target_materials[i]->name,
                            "District PC geometry/TU1.5 native/MCC material order differs");
                    }
                    const auto source_bank=material_bank(*material_owner,authority_materials);
                    const auto target_bank=material_bank(mcc,target_materials);
                    std::set<unsigned> used;for(const auto& v:source_model.mesh.vertices)used.insert(v.material);
                    need(source_model.animation==0xffffffffu,"Static source sky unexpectedly owns an animation graph");
                    std::array<RenderModel,6> source_lods;std::array<std::vector<unsigned>,6> source_sections;
                    for(unsigned lod=0;lod<6;++lod) {
                        source_sections[lod]=sections(original,source_sky->model,lod);
                        source_lods[lod]=original.load_model_sections(source_sky->model,false,0,lod,source_sections[lod]);
                        if(source_input.pc_district)need(mcc_sky_geometry_sha256(encode_mcc_sky_geometry(source_lods[lod]))==
                            "5116b73dda6d4b01dbdb5fe00c97ee984164879afddb8360411c22b0b581b896",
                            "District original PC LOD differs from its literal source geometry");
                        for(const auto& v:source_lods[lod].mesh.vertices)used.insert(v.material);
                    }
                    Json mapping=Json::array(),semantics=Json::array();
                    for(unsigned slot=0;slot<source_materials.size();++slot) {
                        const auto* source=source_materials[slot];unsigned actual=target_materials.size()-1;
                        unsigned authority_slot=authority_materials.size()-1;
                        if(source) {
                            bool found=false;for(unsigned j=0;j+1<target_materials.size();++j)if(target_materials[j]->name==source->name){need(!found,"Ambiguous actual MCC shader binding");found=true;actual=j;}
                            need(found,"Original sky shader has no actual MCC binding: "+source->name);
                            found=false;for(unsigned j=0;j+1<authority_materials.size();++j)if(authority_materials[j]->name==source->name){need(!found,"Ambiguous original Xbox native shader authority");found=true;authority_slot=j;}
                            need(found,"Original geometry material has no typed Xbox native shader authority");
                        }
                        const auto source_semantic=sky_audit::semantic_digest(source_bank.materials.at(authority_slot),*material_owner);
                        const auto target_semantic=sky_audit::semantic_digest(target_bank.materials.at(actual),mcc);
                        if(used.contains(slot))need(source&&source_bank.materials.at(authority_slot).source_shader_datum==authority_materials[authority_slot]->datum&&
                            target_bank.materials.at(actual).source_shader_datum==target_materials[actual]->datum&&
                            source_bank.materials.at(authority_slot).forward_pass&&target_bank.materials.at(actual).forward_pass,
                            "Used original sky material has an unresolved actual source/MCC shader owner");
                        semantic_differences+=source_semantic!=target_semantic;
                        Json item={{"mcc_index",actual},{"name",source?source->name:""},{"mcc_shader",source?target_materials[actual]->datum:0xffffffffu},
                            {"native_authority_material_index",authority_slot},{"native_authority_shader",source?authority_materials[authority_slot]->datum:0xffffffffu},
                            {"mcc_shader_sha256",source?mcc_sky_geometry_sha256(mcc.tag_payload(target_materials[actual]->handle())):""},
                            {"original_semantic_sha256",source_semantic},{"mcc_semantic_sha256",target_semantic}};
                        mapping.push_back(item);semantics.push_back({{"source_slot",slot},{"mcc_slot",actual},{"name",item["name"]},{"used",used.contains(slot)},{"semantic_exact",source_semantic==target_semantic}});
                    }
                    const auto mapping_text=mapping.dump();
                    if(!original_hashes.contains(material_path))original_hashes[material_path]=sky_reference_detail::file_sha256(material_path);
                    Json profile={{"input_sha256",input},{"original_model",source_tag->name},{"original_map",path.filename().string()},
                        {"mcc_map",target.filename().string()},{"original_path",fs::absolute(path).lexically_normal().string()},
                        {"original_file_sha256",original_hashes.at(path)},
                        {"binding_authority",source_input.pc_district?"original-PC11081-geometry-with-Xbox-TU15-native-materials":source_input.shared_needle?"complete-authenticated-shared-Needle-Dune-model":source_input.finite_reference?"authenticated-Blastacular-source-pair":"same-typed-original-map-sky"},
                        {"geometry_source",source_input.pc_district?"originalPC11081":"originalXbox"},{"native_material_source","originalXboxTU15"},
                        {"native_material_path",fs::absolute(material_path).lexically_normal().string()},{"native_material_map",material_path.filename().string()},
                        {"native_material_file_sha256",original_hashes.at(material_path)},{"native_material_model_datum",authority_tag->datum},
                        {"native_material_model_input_sha256",authority_input},
                        {"mcc_model",target_tag->datum},{"original_model_datum",source_tag->datum},
                        {"original_model_sha256",mcc_sky_geometry_sha256(original.tag_payload(source_tag->handle()))},
                        {"original_model_input_sha256",mcc_sky_geometry_input_sha256(original,source_sky->model)},
                        {"materials",mapping},{"original_material_count",source_materials.size()},
                        {"material_mapping_sha256",mcc_sky_geometry_sha256(std::as_bytes(std::span(mapping_text)))},
                        {"preview",encoded(source_model)},{"lods",Json::array()}};
                    profile["preview"]["mcc_sections"]=sections(mcc,target_sky->model,{});
                    Json lod_report=Json::array();
                    for(unsigned lod=0;lod<6;++lod) {
                        const auto& selected=source_sections[lod];const auto& model=source_lods[lod];
                        auto entry=encoded(model);entry["mcc_sections"]=sections(mcc,target_sky->model,lod);entry["original_sections"]=selected;profile["lods"].push_back(entry);
                        lod_report.push_back({{"lod",lod},{"original_sections",selected},{"mcc_sections",entry["mcc_sections"]},{"vertices",model.mesh.vertices.size()},{"bones",model.bones.size()},{"compounds",model.compound_nodes.size()}});
                    }
                    Json contract={{"preview",profile["preview"]["sha256"]},{"lods",Json::array()},{"materials",Json::array()}};
                    for(const auto& lod:profile["lods"])contract["lods"].push_back(lod["sha256"]);
                    for(const auto& material:mapping)contract["materials"].push_back(material["original_semantic_sha256"]);
                    const auto contract_text=contract.dump();const auto source_contract=mcc_sky_geometry_sha256(std::as_bytes(std::span(contract_text)));
                    if(const auto previous=seen.find(input);previous!=seen.end()) {
                        need(previous->second==source_contract,"One complete MCC model input maps to conflicting original sky geometry/material/transform authorities");continue;
                    }
                    seen.emplace(input,source_contract);profile["original_contract_sha256"]=source_contract;
                    report["pairs"].push_back({{"map",path.filename().string()},{"mcc_map",target.filename().string()},{"original_path",path.string()},{"original_file_sha256",original_hashes.at(path)},
                        {"binding_authority",profile["binding_authority"]},{"ordinal",ordinal},{"model",source_tag->name},{"input_sha256",input},
                        {"original_vertices",source_model.mesh.vertices.size()},
                        {"source_sky",sky_audit::sky(*source_sky,original)},{"mcc_sky",sky_audit::sky(*target_sky,mcc)},
                        {"material_mappings",semantics},{"lods",lod_report},{"source_geometry_sha256",profile["preview"]["sha256"]}});
                    catalog["profiles"].push_back(std::move(profile));
                }
            } catch(const std::exception& e) {report["errors"].push_back({{"map",path.filename().string()},{"error",e.what()}});}
        }
        report["paired_sky_ordinals"]=pairs;report["unique_geometry_profiles"]=catalog["profiles"].size();report["semantic_differences"]=semantic_differences;report["diagnostic_only"]=diagnostic;
        catalog["geometry"]=Json::object();std::size_t decoded_bytes{};
        for(auto& profile:catalog["profiles"]) {
            const auto retain=[&](Json& entry) {
                const auto identity=entry.at("sha256").get<std::string>();
                if(!catalog["geometry"].contains(identity)){catalog["geometry"][identity]={{"sha256",identity},{"bytes",entry["bytes"]},{"zlib_hex",entry["zlib_hex"]}};decoded_bytes+=entry["bytes"].get<std::size_t>();}
                entry.erase("zlib_hex");
            };
            retain(profile["preview"]);for(auto& entry:profile["lods"])retain(entry);
        }
        report["unique_source_geometry_states"]=catalog["geometry"].size();report["source_geometry_decoded_bytes"]=decoded_bytes;
        need(pairs>0&&!catalog["profiles"].empty(),"No original/MCC sky geometry pair was exported");
        fs::create_directories(report_path.parent_path());std::ofstream report_file(report_path);report_file<<report.dump(2)<<'\n';need(bool(report_file),"Cannot write geometry export report");
        need(diagnostic||(report["errors"].empty()&&semantic_differences==0),"Exact sky source catalog rejected unresolved material equivalence; see geometry export report");
        fs::create_directories(output.parent_path());std::ofstream data(output);data<<"R\"sky("<<catalog.dump()<<")sky\"\n";data.flush();need(bool(data),"Cannot write original sky source catalog");
        std::cout<<Json({{"profiles",catalog["profiles"].size()},{"paired_ordinals",pairs},{"errors",report["errors"].size()},{"catalog_bytes",fs::file_size(output)}}).dump()<<'\n';
        return report["errors"].empty()?0:1;
    } catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
