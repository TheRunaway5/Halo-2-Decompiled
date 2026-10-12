#include "halo2/cache.hpp"
#include "halo2/source_shader_texture.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
}
int main(int argc,char **argv)try {
    check(argc==4,"Supply disc maps, map-pack maps and original shader census");
    std::ifstream file(argv[3]);check(bool(file),"Original shader census unavailable");
    const auto census=nlohmann::json::parse(file);
    unsigned maps=0,implementations=0,records=0;
    for(const auto &row:census.at("maps")) {
        const auto name=row.at("map").get<std::string>()+".map";
        auto path=std::filesystem::path(argv[1])/name;
        if(!std::filesystem::exists(path))path=std::filesystem::path(argv[2])/name;
        Cache cache(path);
        const auto material=cache.load_material(source_camouflage_shader(cache));
        for(const auto &raster:row.at("rasterizer_rows"))for(const auto &layer:raster.at("layers"))
            for(const auto &impl:layer.at("implementations")) {
                const auto found=std::find_if(material.passes.begin(),material.passes.end(),[&](const auto &pass){
                    return pass.template_pass==layer.at("template_pass") && pass.implementation==impl.at("index");
                });
                check(found!=material.passes.end(),"Authored global shader implementation lost");
                const auto &expected=impl.at("render_state_externs");
                check(found->state_externs.size()==expected.size(),"Authored state-extern range was ignored");
                for(std::size_t n=0;n<expected.size();++n) {
                    const auto &actual=found->state_externs[n];
                    check(actual.destination==expected[n].at("destination") &&
                        actual.component==expected[n].at("component") && actual.source==expected[n].at("source"),
                        "State-extern bytes differ from actual compiled metadata");
                    ++records;
                }
                ++implementations;
            }
        ++maps;
    }
    check(maps==21 && implementations==126 && records==42,"Owned state-range coverage changed");
    std::cout<<maps<<" actual global shaders, "<<implementations<<" authored implementation ranges and "
        <<records<<" state-extern records passed\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
