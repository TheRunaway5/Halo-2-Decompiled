#pragma once
// Finite offline references omitted by the ordinary source-directory filenames.
// These paths select no runtime content; complete original files authenticate
// both the Blastacular source and the one explicit filename/scenario rename.
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <array>
#include <fstream>
#include <memory>

namespace halo2 {
struct SkyReferenceInput {
    std::filesystem::path path;
    std::string authority_map;
};
namespace sky_reference_detail {
struct Authority {
    std::string_view map,filename,sha256,original_scenario,prepared_scenario,model;
    std::uint32_t checksum;
};
inline constexpr std::array<Authority,2> authorities{{
    {"derelict","derelict.map","85c9d98556a0cd8dd6bcce94992425518f8a5753c0adb0c353fddf83696f7efd",
        "scenarios\\multi\\halo\\derelict\\derelict","scenarios\\multi\\halo\\derelict\\derelict",
        "scenarios\\skies\\multi\\halo\\derelict\\derelict",337446937u},
    {"highplains","highplains2.map","b14a0debdb2e279c7bd7c7cda2b3e2a5cf69ca85f457923c1cfb9de6ca0b1bb1",
        "scenarios\\multi\\halo\\highplains\\highplains2","scenarios\\multi\\halo\\highplains\\highplains",
        "scenarios\\skies\\multi\\halo\\highplains\\highplains",520756742u},
}};
inline void require(bool value,const char* message){if(!value)throw FormatError(message);}
inline const Authority& authority(const std::string& name) {
    for(const auto& row:authorities)if(row.map==name)return row;
    throw FormatError("Sky reference has no finite original map authority");
}
inline nlohmann::json read_json(const std::filesystem::path& path) {
    std::ifstream file(path,std::ios::binary|std::ios::ate);
    require(bool(file)&&file.tellg()>=0&&std::uint64_t(file.tellg())<=16u*1024u*1024u,
        "Offline sky reference metadata is missing or excessive");
    file.seekg(0);return nlohmann::json::parse(file);
}
inline std::string file_sha256(const std::filesystem::path& path) {
    std::ifstream file(path,std::ios::binary);require(bool(file),"Original sky reference file is unavailable");
    std::unique_ptr<EVP_MD_CTX,decltype(&EVP_MD_CTX_free)> context(EVP_MD_CTX_new(),EVP_MD_CTX_free);
    require(bool(context)&&EVP_DigestInit_ex(context.get(),EVP_sha256(),nullptr)==1,"Cannot authenticate original sky reference");
    std::array<char,64*1024> buffer{};
    while(file) {
        file.read(buffer.data(),buffer.size());const auto count=file.gcount();
        if(count)require(EVP_DigestUpdate(context.get(),buffer.data(),std::size_t(count))==1,"Cannot hash original sky reference");
    }
    require(file.eof(),"Original sky reference read failed");
    std::array<unsigned char,32> hash{};unsigned length{};
    require(EVP_DigestFinal_ex(context.get(),hash.data(),&length)==1&&length==hash.size(),"Original sky reference digest failed");
    constexpr char digits[]="0123456789abcdef";std::string result;
    for(const auto byte:hash){result+=digits[byte>>4];result+=digits[byte&15];}return result;
}
inline void validate(const Cache& cache,const SkyReferenceInput& input,bool prepared) {
    const auto& expected=authority(input.authority_map);
    require(cache.is_mcc()==prepared,"Original/prepared sky reference cache format differs");
    const auto* scenario=cache.find_tag(cache.scenario_tag(),tag_groups::scenario);
    require(scenario&&scenario->address&&scenario->name==(prepared?expected.prepared_scenario:expected.original_scenario),
        "Finite sky reference scenario has another typed owner");
    const auto sky=cache.load_sky(0);require(sky.has_value(),"Finite sky reference has no authored first sky");
    const auto* model=cache.find_tag({sky->model},tag_groups::render_model);
    require(model&&model->address&&model->name==expected.model,"Finite sky reference model has another typed owner");
    require(Bytes(cache.tag_payload(model->handle())).u32(8)==expected.checksum,"Finite sky reference model checksum differs");
}
} // namespace sky_reference_detail

inline void validate_sky_reference_pair(const Cache& original,const Cache& prepared,const SkyReferenceInput& input) {
    sky_reference_detail::validate(original,input,false);
    sky_reference_detail::validate(prepared,input,true);
}

inline std::vector<SkyReferenceInput> original_sky_reference_inputs(
    const std::filesystem::path& root,const std::filesystem::path& prepared_root) {
    using namespace sky_reference_detail;
    const auto catalog=read_json(prepared_root/"native-shaders/catalog.json");
    require(catalog.at("schema")=="H2_PRESERVED_NATIVE_SHADER_CATALOG_V1"&&catalog.at("maps").is_array(),
        "Original sky reference source catalog has another schema");
    std::vector<SkyReferenceInput> result;
    for(const auto& expected:authorities) {
        const nlohmann::json* row=nullptr;
        for(const auto& candidate:catalog.at("maps"))if(candidate.at("map")==expected.map) {
            require(!row,"Original sky reference catalog owner is ambiguous");row=&candidate;
        }
        require(row&&row->at("complete")==true,"Original sky reference catalog owner is incomplete");
        const auto bank=read_json(prepared_root/"native-shaders"/std::string(expected.map)/"library.json");
        require(bank.at("schema")=="H2_PRESERVED_NATIVE_SHADERS_V1"&&bank.at("map")==expected.map&&
            bank.at("source_sha256")==expected.sha256,"Original sky reference native-bank source authority differs");
        std::filesystem::path path=row->at("source").get<std::string>();
        require(path.is_absolute()&&path.filename()==expected.filename,"Original sky reference catalog path differs");
        // Prefer the existing project reference while requiring the same full
        // original bytes; a changed local reference never silently falls back.
        for(const auto& directory:{root/"assets/disc/maps",root/"assets/map-pack/maps"})
            if(std::filesystem::is_regular_file(directory/std::string(expected.filename))) {
                path=directory/std::string(expected.filename);break;
            }
        require(file_sha256(path)==expected.sha256,"Original sky reference file differs from its complete pinned source");
        SkyReferenceInput input{path,std::string(expected.map)};
        Cache source(path,root/"assets/disc/maps");validate(source,input,false);
        result.push_back(std::move(input));
    }
    return result;
}
} // namespace halo2
