#include "halo2/source_bsp_effect_lights.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void require(bool b,const char* why){if(!b)throw FormatError(why);}
float real(std::uint32_t w){return std::bit_cast<float>(w);}
SourceBspAdmission admission(const Json& j) {
    SourceBspAdmission a;
    require(j.at("entries").size()==a.cluster_entries.size(),"reference cluster bank extent");
    for(unsigned n=0;n<a.cluster_entries.size();++n)a.cluster_entries[n]=j.at("entries")[n].get<std::uint8_t>();
    const auto queue=[](const Json& rows,auto& out){for(const auto& r:rows)
        out.push_back({r[0].template get<std::uint32_t>(),r[1].template get<std::uint16_t>(),
                       r[2].template get<std::uint16_t>(),r[3].template get<std::uint32_t>()});};
    queue(j.at("clusters"),a.clusters);queue(j.at("instances"),a.instances);return a;
}
void compare(std::span<const EffectLightRawDraw> draws,const Json& expected) {
    require(draws.size()==expected.size(),"live original BSP light packet count differs");
    for(unsigned n=0;n<draws.size();++n) {
        const auto& actual=draws[n].source;const auto& source=actual.draw();const auto& e=expected[n];
        const bool instance=actual.owner().kind==MenuWorldSourceOwnerKind::bsp_instance;
        if(source.geometry->section()!=e.at("section").get<unsigned>() ||
           source.part!=e.at("part").get<unsigned>() ||
           (instance && actual.owner().ordinal!=e.at("instance").get<unsigned>()))
            std::cerr<<"Draw "<<n<<" native kind/section/instance/part "<<instance<<'/'<<source.geometry->section()<<'/'
                <<actual.owner().ordinal<<'/'<<source.part<<" original "<<e.at("kind")<<'/'<<e.at("section")<<'/'
                <<e.at("instance")<<'/'<<e.at("part")<<'\n';
        require(instance==bool(e.at("kind").get<unsigned>()),"live original BSP light owner order differs");
        require(source.geometry->section()==e.at("section").get<unsigned>(),"live original BSP light section order differs");
        if(instance)require(actual.owner().ordinal==e.at("instance").get<unsigned>(),"live original BSP light instance order differs");
        require(source.part==e.at("part").get<unsigned>(),"live original BSP light part order differs");
        require(actual.declaration().class_index==e.at("vertex_class").get<unsigned>(),"live original BSP light vertex class differs");
        require(source.topology==(e.at("topology").get<unsigned>()?MenuWorldSourceTopology::triangle_strip:MenuWorldSourceTopology::triangle_list),"live original BSP light topology differs");
        require(source.range && source.range->first==e.at("first").get<unsigned>() && source.range->count==e.at("count").get<unsigned>(),"live original BSP light index interval differs");
        const auto indices=source.indices();require(indices.size()==e.at("indices").size(),"live original BSP light index count differs");
        for(unsigned k=0;k<indices.size();++k)require(indices[k]==e.at("indices")[k].get<std::uint16_t>(),"live original BSP light index value differs");
    }
}
}
int main(int argc,char** argv)try {
    require(argc==3,"Supply MAP EXPORTED_ORIGINAL_JSON");
    std::ifstream f(argv[2]);Json j;f>>j;
    const Cache cache(argv[1]);Mesh mesh;
    const auto r=j.at("light").get<std::array<std::uint32_t,68>>();
    const auto scalar=[&](unsigned at){return real(r.at(at/4));};
    const auto point=[&](unsigned at){return Vec3{scalar(at),scalar(at+4),scalar(at+8)};};
    const auto definition=load_effect_light(cache,r[1]);const std::array ids{r[1]};
    SourceBspEffectLights owner(cache,mesh,0,ids);
    SourceEffectLightView lights;SourceEffectLightViewRecord light{};
    light.definition=&definition;light.definition_identity=r[1];
    light.runtime={j.at("identity"),r[0x4c/4],std::uint8_t(r[0]>>16),std::int16_t(r[0x54/4]),
        std::int16_t(r[0x48/4]),std::bit_cast<std::int32_t>(r[2]),scalar(0x24),scalar(0x100),
        point(0x18),point(0x84),point(0xac),point(0xb8),
        {scalar(0xc4),scalar(0xd0),point(0xd4),point(0xe0),scalar(0xec)}};
    light.fades={scalar(0xf8),scalar(0xfc),scalar(0x104),scalar(0x100),scalar(0x108)};
    lights.records.push_back(light);
    WorldEffectLightReceiver receiver;receiver.identity=light.runtime.identity;
    receiver.selected_fade=real(j.at("selected_fade"));receiver.view.bsp=admission(j.at("receiver"));
    receiver.view.pass_mode=j.at("pass_mode");receiver.view.query_mode=j.at("query_mode");
    SourceViewProjectionBank camera{};const auto camera_words=j.at("camera").get<std::array<std::uint32_t,48>>();
    std::memcpy(&camera,camera_words.data(),sizeof(camera));
    BspFogSample fog{};const auto& fj=j.at("fog");fog.classification=fj.at("classification");
    fog.planar_datum=fj.at("planar_datum");fog.eye_fraction=real(fj.at("eye_fraction"));
    fog.planar_flags=fj.at("planar_flags");fog.planar_only=fj.at("planar_only");
    auto main=admission(j.at("main"));
    const auto draws=owner.draw(lights,std::span(&receiver,1),main,camera,fog,camera.camera_to_world.position,
        {0,0,480,640},{640,480},1);
    compare(draws,j.at("expected"));
    const auto uniforms=j.at("uniforms").get<std::array<std::uint32_t,36>>();
    for(const auto& draw:draws)for(unsigned c=0;c<9;++c)for(unsigned lane=0;lane<4;++lane)
        require(std::bit_cast<std::uint32_t>(draw.source.constants()[31+c][lane])==uniforms[c*4+lane],
                "live BSP light constants differ from unchanged original4A1E0/497A0");
    // Below the source capacity boundary, translating the BSP unit prefix
    // must not change this BSP-only draw segment's actual order/indices.
    SourceBspAdmission empty;
    const auto shifted=owner.draw(lights,std::span(&receiver,1),empty,camera,fog,camera.camera_to_world.position,
        {0,0,480,640},{640,480},1);
    compare(shifted,j.at("expected"));
    std::cout<<"PASS "<<draws.size()<<" original ordered live BSP light packets and prefix translation; full dynamic/world frame separate\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
