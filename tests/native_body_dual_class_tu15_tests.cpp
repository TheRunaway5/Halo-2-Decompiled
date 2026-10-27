#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/model.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <tuple>
#include <fstream>
#include <iostream>

using namespace halo2;
namespace {
void need(bool value,const char *message) {if(!value)throw FormatError(message);}
}
int main(int argc,char **argv)try {
    need(argc==5,"Supply prepared MCC map, shared-map directory, representation and report path");
    const auto *scratch=std::getenv("HALO2_TEST_SCRATCH");
    need(scratch && std::filesystem::path(scratch).is_absolute(),"Run body class acceptance through tools/run_test.py");
    const unsigned representation=std::stoul(argv[3]);
    need(representation==2 || representation==3,"Body class acceptance requires human or Elite");
    MultiplayerOptions options;
    options.map=argv[1];options.shared_maps=argv[2];options.dedicated=true;
    options.local_players=0;options.starting_weapon=5;options.mute=true;
    options.source_object_world=true;
    options.variant=UiVariant{"Slayer",0,{{2,0},{10,0},{89,5}}};
    options.source_object_savegame_directory=std::filesystem::path(scratch)/"native-body-class";
    MultiplayerPreparedWorld world(options);
    need(world.cache->is_mcc() && world.source_objects && world.source_objects->owns_physics() &&
        world.match.source_engine_owned(),"Body class acceptance requires canonical native MCC authority");
    auto &match=world.match;auto &native=*world.source_objects;
    auto configuration=source_default_player_configuration();configuration[0x44]=representation==3;
    const auto slot=match.join("Native body class",-1,configuration);
    const auto weapon=world.cache->slayer_starting_weapon(5);
    match.configure_replicated_first_person(world.cache->load_first_person_binding(weapon,representation));
    const auto &memory=native.memory();
    const auto word=[&](std::uint32_t address){return Bytes(memory.view(address,4)).u32(0);};
    const auto offset=[&](std::uint32_t address){return std::bit_cast<std::int16_t>(Bytes(memory.view(address,2)).u16(0));};
    nlohmann::json report{{"representation",representation},{"rows",nlohmann::json::array()}};
    std::array<Controls,multiplayer_capacity> controls{};
    unsigned dual_ticks{},failures{};
    for(unsigned tick=0;tick<100 && dual_ticks<24;++tick) {
        const auto before=match.state();const auto &old=before.players[slot];
        if(old.source_unit!=null_datum && old.secondary_weapon.datum==null_datum) {
            const auto item=std::find_if(before.world_weapons.begin(),before.world_weapons.end(),
                [&](const auto &entry){return entry.definition==weapon;});
            if(item!=before.world_weapons.end()) {
                native.tu15_world().set_object_position(old.source_unit,item->position+Vec3{-.2f,0,0});
                controls[slot].dual_wield=true;controls[slot].dual_wield_milliseconds=1000;
            }
        }
        match.step(controls);const auto state=match.state();const auto &player=state.players[slot];
        if(player.source_unit==null_datum || player.secondary_weapon.datum==null_datum)continue;
        controls[slot].dual_wield=false;++dual_ticks;
        const auto body=native.source_object_body(player.source_unit);
        const auto manager=body+std::uint32_t(offset(body+0x12au));
        const auto extension=body+std::uint32_t(offset(body+0x33eu));
        const auto graph=word(manager+0x68u),mode=word(manager+0x70u),type=word(manager+0x78u);
        nlohmann::json row{{"tick",state.tick},{"class",word(manager+0x74u)},
            {"mode",mode},{"type",type},{"graph",graph},{"unit",player.source_unit},
            {"hands",nlohmann::json::array()}};
        bool valid=word(manager+0x74u)==0x0400054bu;
        for(unsigned hand=0;hand<2;++hand) {
            nlohmann::json selected{{"hand",hand}};
            for(const auto [label,at,activity]:std::array{
                std::tuple{"arm_offsets",extension+0x104u+hand*4u,0x0b000556u},
                std::tuple{"grip",extension+0x10cu+hand*4u,0x04000617u}}) {
                std::optional<AnimationClip> expected;
                try {expected=world.cache->load_unit_weapon_action(graph,mode,0x04000557u,type,activity,true);}
                catch(const FormatError &error) {
                    // This original query permits NONE when neither overlay
                    // nor ordinary fallback authors the requested action.
                    const auto missing=std::string("authored unit weapon action missing: ")+label;
                    if(error.what()!=missing)throw;
                }
                const auto identifier=word(at);
                bool same=!expected && (identifier>>16u)==0xffffu;
                std::uint32_t actual_graph=null_datum,actual_index=null_datum;
                if((identifier>>16u)!=0xffffu) {
                    const auto actual=world.cache->load_animation_identifier(graph,identifier);
                    actual_graph=actual.source_graph;actual_index=actual.source_index;
                    same=expected && actual_graph==expected->source_graph && actual_index==expected->source_index;
                }
                selected[label]={{"identifier",identifier},{"actual_graph",actual_graph},{"actual_index",actual_index},
                    {"expected_present",bool(expected)},{"expected_graph",expected?expected->source_graph:null_datum},
                    {"expected_index",expected?expected->source_index:null_datum},{"same",same}};
                valid=valid && same;
            }
            row["hands"].push_back(std::move(selected));
        }
        row["valid"]=valid;if(!valid)++failures;
        report["rows"].push_back(std::move(row));
    }
    report["dual_ticks"]=dual_ticks;report["failures"]=failures;
    report["passed"]=dual_ticks==24 && failures==0;
    std::ofstream(argv[4])<<report.dump(2)<<'\n';
    need(dual_ticks==24,"Actual native scenario did not retain 24 dual-wield ticks");
    need(!failures,"Native dual body class/grip/arm-offset selectors differ from the authored dual hierarchy");
    std::cout<<"PASS native canonical dual class and both authored grip/arm-offset selections\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
