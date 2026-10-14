#include "halo2/bsp_atmosphere.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
using namespace halo2;
int main(int argc,char **argv)try {
    if(argc!=2)throw FormatError("Supply original TU1.5 fog selection fixtures");
    std::ifstream input(argv[1]);if(!input)return 77;
    const auto fixture=nlohmann::json::parse(input);
    if(fixture.at("xbe_sha256")!="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd")
        throw FormatError("Wrong fog selection authority");
    unsigned checked{};
    for(const auto& c:fixture.at("cases")) {
        const auto choice=source_fog_draw_selection(c.at("classification"),
            {c.at("suppressed").get<unsigned>()!=0,c.at("force_atmospheric").get<unsigned>()!=0});
        nlohmann::json selected=nullptr;
        if(choice)selected=*choice==SourceFogDrawKind::Atmospheric?"atmospheric":"sky";
        if(selected!=c.at("selected"))throw FormatError("Original fog draw selection differs");
        ++checked;
    }
    if(checked!=81)throw FormatError("Original fog selection coverage changed");
    BspFogSample sample;sample.classification=3;
    sample.primary={{.2f,.3f,.4f},.2f,0.f,100.f};
    if(bsp_fog_composite(sample,1024).program.words[54]!=1 || bsp_fog_draw(sample,1024))
        throw FormatError("Planar classification received the unselected atmospheric composite");
    sample.sky_opacity=.5f;sample.sky_color_premultiplied={.1f,.2f,.3f};
    const auto sky=bsp_fog_draw(sample,1024);
    if(!sky || sky->program.words[54]!=0 || !sky->program.words[43])
        throw FormatError("Sky-only fog lost maximum-depth admission or eye color");
    if(bsp_fog_draw(sample,1024,{true,false}))throw FormatError("Suppressed fog emitted a draw");
    const auto forced=bsp_fog_draw(sample,1024,{false,true});
    if(!forced || forced->program.words[54]!=1)throw FormatError("Actual override did not select atmosphere");
    std::cout<<"PASS "<<checked<<" unchanged TU1.5 selections; zero-opacity skip and sky-only depth contract\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
