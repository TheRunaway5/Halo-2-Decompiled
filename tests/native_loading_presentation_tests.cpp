#include "halo2/native_loading_presentation.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>

namespace {
void check(bool condition,const char* message) { if(!condition)throw std::runtime_error(message); }
std::uint32_t word(const std::string& value) { return std::uint32_t(std::stoul(value,nullptr,16)); }
}
int main(int argc,char** argv) {
    try {
        check(argc==2,"original draw fixture required");
        std::ifstream file(argv[1]);const auto fixture=nlohmann::json::parse(file);
        check(fixture.at("source_sha256")=="33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109","wrong Vista source");
        halo2::VistaLoadingAssets assets;
        for(auto& image:assets)image={1,1,{1,1,11,std::vector<std::byte>(4,std::byte{255}),{}}};
        halo2::NativeLoadingPresentation presentation(assets);
        assets[1].source_width=1024;assets[1].source_height=128;
        halo2::NativeLoadingPresentation proportioned(assets,true);
        check(presentation.resident_mesh().materials.size()==12,"loading resident draw bank changed");
        unsigned cases{},vertices{};
        for(const auto& row:fixture.at("cases")) {
            if(row.at("mode")!=0 || row.at("throttled").get<bool>() || row.at("device_guard").get<bool>())continue;
            std::vector<halo2::VistaLoadingDraw> draws;
            for(const auto& source:row.at("draws")) {
                halo2::VistaLoadingDraw draw{};draw.primitive=source.at("kind");draw.shader=source.at("shader");
                draw.texture=draw.shader==3?1:source.at("texture_handles").at("0").get<unsigned>()-0x7000;
                const unsigned src=source.at("states").at("19"),dst=source.at("states").at("20");
                draw.blend=src==2 && dst==2?halo2::VistaLoadingBlend::one_one:src==9 && dst==1?
                    halo2::VistaLoadingBlend::destination_color_zero:halo2::VistaLoadingBlend::source_alpha_inverse_source_alpha;
                for(unsigned v=0;v<4;++v) {
                    const auto& words=source.at("vertices")[v].at("words");auto& vertex=draw.vertices[v];
                    vertex.position={word(words[0]),word(words[1])};vertex.texcoord0={word(words[2]),word(words[3])};
                    vertex.texcoord1={word(words[4]),word(words[5])};vertex.argb=word(words[6]);
                }
                if(source.at("constants").contains("pixel:0")) {
                    std::array<std::uint32_t,4> c{};const auto values=source.at("constants").at("pixel:0").at("values");
                    for(unsigned k=0;k<4;++k)c[k]=std::bit_cast<std::uint32_t>(values[k].get<float>());
                    draw.pixel_constant0=c;
                }
                draws.push_back(draw);
            }
            presentation.update(draws,row.at("width"),row.at("height"));
            proportioned.update(draws,row.at("width"),row.at("height"));
            const auto logo=proportioned.vertices().subspan(60,6);
            float left=1,right=0,top=1,bottom=0;
            for(const auto &v:logo){left=std::min(left,v.position.x);right=std::max(right,v.position.x);
                top=std::min(top,v.position.y);bottom=std::max(bottom,v.position.y);}
            const float aspect=(right-left)*row.at("width").get<float>()/
                ((bottom-top)*row.at("height").get<float>());
            check(std::abs(aspect-8.f)<.0001f && std::abs(left+right-1.f)<.00001f &&
                std::abs(top+bottom-1.f)<.00001f,"Imported logo stretched or left its authored centre");
            check(presentation.vertices().size()==draws.size()*6,"loading triangles dropped");
            for(unsigned i=0;i<draws.size();++i) {
                const auto& draw=draws[i];const auto& material=*presentation.materials()[i].vista_loading;
                check(material.shader==draw.shader,"loading program changed");
                check(unsigned(material.blend)==unsigned(draw.blend),"loading blend changed");
                check(material.raster_offset_pixels.x==.5f && material.raster_offset_pixels.y==.5f,
                      "measured original viewport pixel-center mapping changed");
                if(draw.pixel_constant0)for(unsigned k=0;k<4;++k)
                    check(std::bit_cast<std::uint32_t>(material.constant0[k])==(*draw.pixel_constant0)[k],"loading constant changed");
                for(unsigned v=0;v<6;++v) {
                    constexpr std::array<unsigned,6> strip{0,1,2,2,1,3},fan{0,1,2,0,2,3};
                    const auto& original=draw.vertices[(draw.primitive==5?strip:fan)[v]];
                    const auto& native=presentation.vertices()[i*6+v];
                    check(native.material==i,"loading draw order changed");
                    check(std::bit_cast<std::uint32_t>(native.position.x)==original.position[0] &&
                          std::bit_cast<std::uint32_t>(native.position.y)==original.position[1],"loading position changed");
                    check(std::bit_cast<std::uint32_t>(native.uv.x)==original.texcoord0[0] &&
                          std::bit_cast<std::uint32_t>(native.uv.y)==original.texcoord0[1],"loading UV changed");
                    check(std::lround(native.color.x*255)==((original.argb>>16)&255) &&
                          std::lround(native.color.y*255)==((original.argb>>8)&255) &&
                          std::lround(native.color.z*255)==(original.argb&255) &&
                          std::lround(native.position.z*255)==(original.argb>>24),"loading ARGB changed");
                    ++vertices;
                }
            }
            ++cases;
        }
        std::cout<<"PASS original loading triangle adapter cases="<<cases<<" vertices="<<vertices<<'\n';
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; }
}
