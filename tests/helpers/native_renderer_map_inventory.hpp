#pragma once
#include "halo2/sky_reference_inventory.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <set>
#include <tuple>

namespace halo2::renderer_test {
struct MapInput {
    std::filesystem::path path,prepared_root,shared;
    std::string source,logical_name;
    bool prepared()const{return !prepared_root.empty();}
    std::string key()const{return logical_name.empty()?path.stem().string():logical_name;}
    std::string cohort_key()const{return source=="prepared-custom"?
        "prepared-custom:"+prepared_root.filename().string()+":"+key():source+":"+key();}
};
inline void need(bool value,const std::string& why){if(!value)throw FormatError(why);}
inline std::vector<MapInput> map_inventory(const std::filesystem::path& root,
    const std::filesystem::path& prepared,nlohmann::json& evidence) {
    using json=nlohmann::json;
    std::vector<MapInput> out;
    const auto original=[&](const auto& directory,const std::string& source) {
        if(!std::filesystem::is_directory(directory))return;
        for(const auto& file:std::filesystem::directory_iterator(directory))
            if(file.is_regular_file()&&file.path().extension()==".map")out.push_back({file.path(),{},directory,source,{}});
    };
    original(root/"assets/disc/maps","original-disc");
    original(root/"assets/map-pack/maps","original-map-pack");
    const auto owned=[&](const auto& owner,const std::string& source) {
        std::ifstream stream(owner/"native-shaders/map-owners.json");
        need(bool(stream),"Missing complete native map-owner index: "+owner.string());
        const auto index=json::parse(stream);
        need(index.at("schema")=="H2_MCC_NATIVE_MAP_OWNERS_V1","Unexpected native map-owner schema");
        const auto relative=[](const std::string& text) {
            const std::filesystem::path path(text);
            need(!path.empty()&&!path.is_absolute()&&path==path.lexically_normal(),"Noncanonical owned map path");
            for(const auto& piece:path)need(piece!="..","Owned map path escapes its declared context");
            return path;
        };
        std::set<std::filesystem::path> declared,actual;
        json row={{"owner",owner.string()},{"source",source},{"missing",json::array()},{"unlisted",json::array()}};
        for(const auto& item:index.at("maps")) {
            const auto path=relative(item.at("map").template get<std::string>());
            const auto shared=relative(item.at("shared_directory").template get<std::string>());
            need(path.extension()==".map"&&declared.insert(path).second,"Duplicate or non-map declared owner");
            out.push_back({owner/path,owner,owner/shared,source,{}});
            if(!std::filesystem::is_regular_file(owner/path))row["missing"].push_back(path.generic_string());
        }
        need(!declared.empty(),"Native owner index declares no maps");
        for(auto it=std::filesystem::recursive_directory_iterator(owner);it!=std::filesystem::recursive_directory_iterator{};++it) {
            const auto path=it->path().lexically_relative(owner);
            if(source=="prepared-stock"&&it->is_directory()&&path=="custom-maps"){it.disable_recursion_pending();continue;}
            if(it->is_regular_file()&&it->path().extension()==".map")actual.insert(path);
        }
        for(const auto& path:actual)if(!declared.contains(path))row["unlisted"].push_back(path.generic_string());
        row["declared"]=declared.size();row["actual"]=actual.size();
        row["complete"]=row["missing"].empty()&&row["unlisted"].empty();evidence.push_back(std::move(row));
    };
    owned(prepared,"prepared-stock");
    if(std::filesystem::is_directory(prepared/"custom-maps"))
        for(const auto& owner:std::filesystem::directory_iterator(prepared/"custom-maps"))if(owner.is_directory())
            owned(owner.path(),"prepared-custom");
    for(const auto& reference:original_sky_reference_inputs(root,prepared)) {
        bool found=false;
        for(auto& input:out)if(!input.prepared()&&
            std::filesystem::weakly_canonical(input.path)==std::filesystem::weakly_canonical(reference.path)) {
            input.logical_name=reference.authority_map;input.shared=root/"assets/disc/maps";found=true;
        }
        if(!found)out.push_back({reference.path,{},root/"assets/disc/maps","original-preserved",reference.authority_map});
    }
    std::sort(out.begin(),out.end(),[](const auto& a,const auto& b) {
        return std::tuple{a.prepared(),a.source=="original-map-pack",a.path}<
               std::tuple{b.prepared(),b.source=="original-map-pack",b.path};
    });
    std::set<std::filesystem::path> seen;
    std::erase_if(out,[&](const auto& row){return !seen.insert(std::filesystem::weakly_canonical(row.path)).second;});
    return out;
}
} // namespace halo2::renderer_test
