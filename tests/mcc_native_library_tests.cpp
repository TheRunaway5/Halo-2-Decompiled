#include "halo2/mcc_native_library.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <set>
#include <stdexcept>

namespace {
using Json=nlohmann::json;
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
Json read(const std::filesystem::path& path){std::ifstream in(path);return Json::parse(in);}
}
int main(int argc,char**argv)try{
    using namespace halo2;
    Json customized{{"map","lockout"},{"shader_tags",Json::array()},{"custom",true}};
    const auto before=customized;
    require(supplement_mcc_native_library(customized,"/unavailable/native-shader-root").empty() && customized==before,
        "Customized material library was changed or required an unrelated source");
    if(argc==1){std::cout<<"Customized native library preservation passed\n";return 0;}
    const std::filesystem::path root(argv[1]);
    if(root.empty() || !std::filesystem::is_directory(root))return 77;
    unsigned maps{},additions{},unchanged{};
    for(const auto& entry:std::filesystem::directory_iterator(root))if(entry.is_directory() && std::filesystem::exists(entry.path()/"library.json")){
        auto library=read(entry.path()/"library.json");const auto original=library;
        const auto count=library.at("shader_tags").size();
        const auto paths=supplement_mcc_native_library(library,root);
        require(library.at("shader_tags").size()>=count,"Existing native definitions removed");
        for(std::size_t i=0;i<count;++i)require(library.at("shader_tags").at(i)==original.at("shader_tags").at(i),
            "Existing native definition/program/relocation changed");
        auto other=library;other["shader_tags"]=original.at("shader_tags");
        require(other==original,"Native rasterizer/global metadata changed");
        std::set<std::pair<std::string,std::string>> identities;
        for(const auto& record:library.at("shader_tags"))require(identities.emplace(record.at("group"),record.at("name")).second,
            "Supplemented native namespace is ambiguous");
        for(std::size_t i=count;i<library.at("shader_tags").size();++i){const auto& record=library.at("shader_tags").at(i);
            const auto filename=record.at("payload").get<std::string>();
            require(paths.contains(filename) && std::filesystem::is_regular_file(paths.at(filename)),"New native payload has no retained owner");
            for(const auto& ref:record.at("references"))require(identities.contains({ref.at(1).at(0),ref.at(1).at(1)}),
                "New native definition has an unowned program dependency");
        }
        auto altered=original;altered["policy"]="customized";const auto altered_before=altered;
        require(supplement_mcc_native_library(altered,root).empty() && altered==altered_before,"Unknown library fingerprint admitted");
        auto again=library;require(supplement_mcc_native_library(again,root).empty() && again==library,"Repeated supplementation changed its result");
        additions+=unsigned(library.at("shader_tags").size()-count);unchanged+=unsigned(count);++maps;
    }
    require(maps==39 && additions==67,"Audited complete native library census differs");
    std::cout<<"native_libraries="<<maps<<" existing_records_unchanged="<<unchanged<<" missing_records_owned="<<additions<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
