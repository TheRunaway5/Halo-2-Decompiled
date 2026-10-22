#include "halo2/mcc_native_library.hpp"
#include "halo2/cache.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <set>
#include <stdexcept>
#include <algorithm>

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
    require(argc==4,"Supply both authenticated actual introduction bitmap namespaces");
    std::vector<Tag> runtime_directory;
    for(unsigned n=2;n<4;++n) {
      const auto fixture=read(argv[n]);
      for(const auto& tag:fixture.at("namespace"))if(tag.at("group")=="bitm") {
        Tag value{tag.at("group"),tag.at("name"),tag.at("datum"),tag.at("address"),tag.at("size")};
        const auto found=std::ranges::find(runtime_directory,value.datum,&Tag::datum);
        if(found==runtime_directory.end())runtime_directory.push_back(std::move(value));
        else require(found->name==value.name && found->address==value.address && found->size==value.size,
            "Captured namespaces disagree on an actual bitmap owner");
    }
    }
    unsigned maps{},additions{},unchanged{};
    for(const auto& entry:std::filesystem::directory_iterator(root))if(entry.is_directory() && std::filesystem::exists(entry.path()/"library.json")){
        auto library=read(entry.path()/"library.json");const auto original=library;
        const auto count=library.at("shader_tags").size();
        if(entry.path().filename()=="00a_introduction") {
            auto rejected=original;bool threw=false;
            try{supplement_mcc_native_library(rejected,root);}catch(const FormatError&){threw=true;}
            require(threw && rejected==original,"Missing MCC bitmap ownership partially published a gauge closure");
        }
        const auto paths=supplement_mcc_native_library(library,root,runtime_directory);
        require(library.at("shader_tags").size()>=count,"Existing native definitions removed");
        for(std::size_t i=0;i<count;++i)require(library.at("shader_tags").at(i)==original.at("shader_tags").at(i),
            "Existing native definition/program/relocation changed");
        auto other=library;other["shader_tags"]=original.at("shader_tags");
        require(other==original,"Native rasterizer/global metadata changed");
        std::set<std::pair<std::string,std::string>> identities;
        for(const auto& record:library.at("shader_tags"))require(identities.emplace(record.at("group"),record.at("name")).second,
            "Supplemented native namespace is ambiguous");
        if(entry.path().filename()=="01a_tutorial")
            for(const auto* name:{"widget_gradient_flash_pulse","plasma_meter","camo_meter"})
                require(identities.contains({"shad",std::string("ui\\hud\\shaders\\")+name}),
                    "Armory shared weapon HUD material has no original native owner");
        for(std::size_t i=count;i<library.at("shader_tags").size();++i){const auto& record=library.at("shader_tags").at(i);
            const auto filename=record.at("payload").get<std::string>();
            require(paths.contains(filename) && std::filesystem::is_regular_file(paths.at(filename)),"New native payload has no retained owner");
            for(const auto& ref:record.at("references")) {
                if(ref.at(1).at(0)=="bitm") {
                    // Native program libraries bind these two images to the
                    // actual MCC cache; gauge delivery checks
                    // own that separate namespace, not shader_tags.
                    const bool gauge=record.at("name")==R"(objects\weapons\rifle\plasma_rifle\shaders\plasma_rifle_gauge)" &&
                        (ref.at(1).at(1)==R"(objects\weapons\rifle\plasma_rifle\bitmaps\plasma_rifle_heat_gauge)" ||
                         ref.at(1).at(1)==R"(objects\weapons\rifle\plasma_rifle\bitmaps\plasma_rifle_heat_gauge_alpha)");
                    const bool current=record.at("name")==R"(objects\weapons\rifle\plasma_rifle\shaders\plasma_rifle_current)" &&
                        (ref.at(1).at(1)==R"(objects\weapons\rifle\plasma_rifle\bitmaps\plasma_current_noise_a)" ||
                         ref.at(1).at(1)==R"(shaders\default_bitmaps\bitmaps\clouds_256)");
                    require(entry.path().filename()=="00a_introduction" && record.at("group")=="shad" && (gauge||current),
                        "Native dependency escaped the exact scoped gauge bitmap contract");
                } else require(identities.contains({ref.at(1).at(0),ref.at(1).at(1)}),
                    "New native definition has an unowned program dependency");
            }
        }
        auto altered=original;altered["policy"]="customized";const auto altered_before=altered;
        require(supplement_mcc_native_library(altered,root).empty() && altered==altered_before,"Unknown library fingerprint admitted");
        auto again=library;require(supplement_mcc_native_library(again,root,runtime_directory).empty() && again==library,"Repeated supplementation changed its result");
        additions+=unsigned(library.at("shader_tags").size()-count);unchanged+=unsigned(count);++maps;
    }
    require(maps==39 && additions==76,"Audited complete native library census differs");
    std::cout<<"native_libraries="<<maps<<" existing_records_unchanged="<<unchanged<<" missing_records_owned="<<additions<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
