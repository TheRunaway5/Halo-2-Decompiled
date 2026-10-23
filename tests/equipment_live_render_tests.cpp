#define SDL_MAIN_HANDLED
#include "halo2/multiplayer_world.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>

using namespace halo2;
namespace {
void check(bool ok,const char* why){if(!ok)throw FormatError(why);}
}
int main(int argc,char** argv)try {
    check(argc==4,"Supply prepared MCC Headlong, owned capture directory and bounded report path");
    if(!std::filesystem::is_regular_file(argv[1]) || !std::getenv("DISPLAY"))return 77;
    check(std::getenv("HALO2_TEST_SCRATCH"),"Live equipment rendering requires owned scratch");
    SDL_SetMainReady();Cache cache(argv[1]);check(cache.is_mcc(),"Live equipment acceptance requires prepared MCC assets");
    const auto content=cache.load_content(0,true);unsigned overshield=~0u,camouflage=~0u;
    for(unsigned row=0;row<content.netgame_equipment.size();++row) {
        const auto &p=content.netgame_equipment[row];if(p.items.size()!=1 || p.items.front().group!=0x65716970u)continue;
        const auto tag=cache.load_source_tag_metadata(p.items.front().definition);
        const auto type=Bytes(tag.bytes_at(tag.source_address()+0x12c,2)).u16(0);
        if(type==2)overshield=row;if(type==3)camouflage=row;
    }
    check(overshield!=~0u && camouflage!=~0u,"Live map lacks both authored powerups");
    const std::filesystem::path output=argv[2];std::filesystem::create_directories(output);
    nlohmann::json report={{"passed",false},{"cases",nlohmann::json::array()},
        {"stimulus","Only original B95D0 placement of a genuine player at existing authored pickups; ordinary source frames own charge, camouflage and expiry. Captures use run_multiplayer and its actual Vulkan camouflage draws."}};
    const auto save=[&]{std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';check(bool(out),"Cannot save live equipment report");};
    for(unsigned species=0;species<2;++species) {
        MultiplayerOptions options;options.map=argv[1];options.shared_maps=options.map.parent_path();
        options.source_object_world=true;options.source_object_savegame_directory=output/("source-"+std::to_string(species));
        options.width=640;options.height=480;options.local_players=1;options.frames=1;
        options.hidden=true;options.mute=true;options.validation=true;
        const std::array<std::array<std::uint8_t,9>,2> appearance{{{8,12,9,1,0,26,9,0,0},{3,4,3,4,1,5,31,0,0}}};
        options.selected_profiles.push_back(native_selected_player_configuration(species?"Elite":"Spartan",appearance[species],-1));
        options.prepared_world=std::make_shared<MultiplayerPreparedWorld>(options);
        auto &prepared=*options.prepared_world;auto &match=prepared.match;auto &objects=*prepared.source_objects;
        prepared.bind_source_local_views(prepared.local_slots);
        const auto slot=prepared.local_slots.at(0);for(unsigned n=0;n<15;++n)match.step({});
        const auto unit=match.player(slot).source_current_object;
        const auto spawn=match.state().players[slot].position;
        const auto pickup=[&](unsigned row) {
            auto object=objects.netgame_object(row);if(object==null_datum)object=objects.create_netgame_object(row);
            check(object!=null_datum,"Actual netgame owner did not spawn its powerup");
            objects.tu15_world().set_object_position(unit,objects.object_transform(object).position);
            bool consumed=false;
            for(unsigned n=0;n<60 && !consumed;++n) {
                match.step({});const auto live=objects.live_objects();
                consumed=std::none_of(live.begin(),live.end(),[&](const auto& o){return o.object==object;});
            }
            check(consumed,"Actual source contact did not consume its powerup");return object;
        };
        const auto shield_item=pickup(overshield);for(unsigned n=0;n<70;++n)match.step({});
        check(match.state().players[slot].shield>2.9f,"Overshield did not finish actual source charge");
        const auto camouflage_item=pickup(camouflage);for(unsigned n=0;n<25;++n)match.step({});
        check(match.state().players[slot].source_camouflage.primary>0.f,"Camouflage contact did not reach the source fade owner");
        objects.tu15_world().set_object_position(unit,spawn);
        match.step({});
        for(bool third_person:{false,true}) {
            unsigned world{},foreground{},shields{};float observed{};
            options.capture_third_person=third_person;
            options.camouflage_layer_observer=[&](unsigned view,std::uint32_t owner,unsigned layer,unsigned implementation,bool fp,float primary,unsigned vertices) {
                check(view==0 && owner==unit && primary>0.f && primary<=1.f && vertices>0 &&
                    ((layer==18 && implementation<3) || (layer==19 && implementation==0)),"Camouflage GPU draw lost its actual owner/pass/geometry");
                (fp?foreground:world)+=vertices;observed=primary;
            };
            options.shield_layer_observer=[&](unsigned view,unsigned owner,bool fp,std::uint32_t shader,std::uint32_t,std::span<const Vertex> vertices) {
                const auto &v=match.unit(slot).definition.vitality;
                if(shader==(fp?v.overshield_first_person_shader:v.overshield_shader))shields+=unsigned(vertices.size());
                check(view==0 && owner==slot,"Overshield layer lost its pickup owner");
            };
            options.screenshot=output/(std::to_string(species)+(third_person?"-world.ppm":"-fp.ppm"));
            check(run_multiplayer(options)==0,"Actual equipment renderer failed");
            std::cout<<"Equipment live layers species="<<species<<" third_person="<<third_person
                <<" world="<<world<<" foreground="<<foreground<<" overshield="<<shields
                <<" primary="<<match.state().players[slot].source_camouflage.primary
                <<" shield="<<match.state().players[slot].shield<<std::endl;
            check((third_person?world:foreground)>0 && shields>0,"Combined actual powerups lack their world/FP Vulkan layers");
            check(std::filesystem::file_size(options.screenshot)>640u*480u*3u,"Live equipment frame missing");
            report["cases"].push_back({{"species",species?"Elite":"Spartan"},{"third_person",third_person},
                {"unit",unit},{"overshield_pickup",shield_item},{"camouflage_pickup",camouflage_item},
                {"primary",observed},{"world_vertices",world},{"foreground_vertices",foreground},{"overshield_vertices",shields},
                {"passed",true},{"validation_layer",true}});save();
        }
        for(unsigned n=0;n<1800 && (match.player(slot).source_powerup_timers[0]>0 ||
            match.state().players[slot].source_camouflage.primary>0.f);++n)match.step({});
        check(match.player(slot).source_current_object==unit && !match.state().players[slot].dead &&
            match.player(slot).source_powerup_timers[0]==0 && match.state().players[slot].source_camouflage.primary==0.f,"Live camouflage failed to expire");
        unsigned stale{};options.capture_third_person=false;options.shield_layer_observer={};
        options.camouflage_layer_observer=[&](auto...){++stale;};
        options.screenshot=output/(std::to_string(species)+"-expired.ppm");
        check(run_multiplayer(options)==0 && stale==0,"Expired camouflage retained a GPU clone");
    }
    report["passed"]=true;save();std::cout<<report.dump(2)<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
