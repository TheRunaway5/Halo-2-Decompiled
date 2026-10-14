#include "halo2/cache.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/world_material.hpp"
#include "halo2/source_character_shaders.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
using namespace halo2;
using Json=nlohmann::json;
int main(int argc,char** argv)try {
    if(argc!=4 && argc!=5)throw FormatError("Supply original asset census, maps, fresh output JSON and optional --models");
    const bool models=argc==5 && std::string(argv[4])=="--models";
    if(argc==5 && !models)throw FormatError("Unknown character probe mode");
    const std::filesystem::path inputs=argv[1],maps=argv[2],output=argv[3];
    if(std::filesystem::exists(output))throw FormatError("Character shader probe output already exists");
    Json report={{"boundary","Native authored ordinary character material admission; explicit zero lighting and camouflage inputs. No final pixel or complete live owner proof."},{"maps",Json::array()}};
    unsigned total_failed=0;
    for(const auto& entry:std::filesystem::directory_iterator(inputs)) {
        if(entry.path().extension()!=".json" || entry.path().filename()=="manifest.json")continue;
        std::ifstream in(entry.path());Json original;in>>original;
        const Cache cache(original.contains("cache_path")?
            std::filesystem::path(original.at("cache_path").get<std::string>()):
            maps/(original.at("map").get<std::string>()+".map"),maps);
        Json map={{"map",original.at("map")},{"materials",Json::array()}};
        unsigned accepted=0,failed=0;
        for(const auto& [key,value]:original.at("shaders").items()) {
            const auto id=std::uint32_t(std::stoul(key,nullptr,16));
            Material authored;
            try {authored=cache.load_material(id);}
            catch(const std::exception& e){
                map["materials"].push_back({{"shader",key},{"name",value.at("name")},{"accepted",false},{"error",e.what()}});
                ++failed;continue;
            }
            for(unsigned lod=0;lod<authored.lods.size();++lod) {
                auto material=authored;material.selected_shader_lod=lod;
                Json row={{"shader",key},{"name",value.at("name")},{"lod",lod}};
                try {
                    const bool bound=prepare_dynamic_material(cache,material,{},true,0.f);
                    row["accepted"]=bound;
                    if(bound){++accepted;row["vertex"]=material.passes.at(material.forward_pass_index).vertex_shader;}
                    else {++failed;row["gaps"]=material.rendering_gaps;}
                }catch(const std::exception& e){row["accepted"]=false;row["error"]=e.what();++failed;}
                map["materials"].push_back(std::move(row));
            }
        }
        if(models) {
            map["models"]=Json::array();
            for(const auto& unit:original.at("units")) {
                if(unit.at("model").is_null())continue;
                const auto id=unit.at("datum").get<std::uint32_t>();
                Json row={{"unit",id},{"name",unit.at("name")}};
                try {
                    auto model=cache.load_model(id,true);
                    const auto& authored=original.at("models").at(std::to_string(unit.at("model").get<std::uint32_t>()));
                    if(model.mesh.materials.size()!=authored.at("materials").size()+1)
                        throw FormatError("Character material extent differs from independent original census");
                    for(unsigned i=0;i<authored.at("materials").size();++i)
                        if(model.mesh.materials[i].source_shader_datum!=authored.at("materials")[i].get<std::uint32_t>())
                            throw FormatError("Character material identity differs from independent original census");
                    SourceCharacterShaders resident(cache,model,id);
                    row["object_type"]=resident.object_type();
                    row["accepted"]=true;row["vertices"]=model.mesh.vertices.size();
                    row["resident_materials"]=model.mesh.materials.size();
                    row["geometry_warnings"]=model.mesh.material_warnings;
                }catch(const std::exception& e){row["accepted"]=false;row["error"]=e.what();++failed;}
                map["models"].push_back(std::move(row));
            }
        }
        map["accepted"]=accepted;map["failed"]=failed;
        total_failed+=failed;
        std::cout<<map.at("map")<<" accepted="<<accepted<<" failed="<<failed<<std::endl;
        report["maps"].push_back(std::move(map));std::ofstream out(output);out<<report.dump(2)<<'\n';
    }
    if(total_failed)throw FormatError("Campaign character shader admission failed in "+std::to_string(total_failed)+" cases");
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
