#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include "helpers/gameplay_fixture.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <set>

using namespace halo2;
int main(int argc,char** argv) {
    if(argc!=5){std::cerr<<"Supply prepared MCC map,shared maps,report and species\n";return 1;}
    nlohmann::json report{{"passed",false},{"runtime_cache_source","prepared MCC requested"},
        {"source_results_or_ready_flags_supplied",false},{"full_weapon_parity",false},
        {"frames",nlohmann::json::array()}};
    try {
        Cache admission(argv[1],argv[2]);
        if(!admission.is_mcc())throw FormatError("Source weapon publication requires prepared MCC content");
        report["runtime_cache_source"]="MCC";
        const auto representation=std::stoul(argv[4]);
        if(representation!=2&&representation!=3)throw FormatError("Unsupported native species");
        MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];
        options.local_players=1;options.starting_weapon=5;options.mute=true;
        if(representation==3){auto profile=source_default_player_configuration();profile[0x44]=1;options.selected_profiles={profile};}
        // Preserve the map's actual spawn inputs and canonical engine admission.
        MultiplayerPreparedWorld world(options);
        report["native_world_constructor_completed"]=true;report["representation"]=representation;
        std::array<Controls,multiplayer_capacity> neutral{};unsigned checked{};
        for(unsigned frame=0;frame<3;++frame) {
            world.match.step(neutral);const auto state=world.match.state();
            auto& source=*world.source_objects;const auto objects=source.live_objects();
            std::set<std::uint32_t> expected,published;
            auto& row=report["frames"].emplace_back(nlohmann::json{{"frame",frame},
                {"published_count",state.world_weapons.size()},{"source_objects",nlohmann::json::array()}});
            for(const auto& object:objects)if(object.type==2&&object.parent==null_datum) {
                expected.insert(object.object);row["source_objects"].push_back({{"object",object.object},{"definition",object.definition}});
                const auto found=std::find_if(state.world_weapons.begin(),state.world_weapons.end(),
                    [&](const auto& value){return value.handle==object.object;});
                if(found==state.world_weapons.end())throw FormatError("Actual loose source weapon is absent from world publication");
                if(found->definition!=object.definition)throw FormatError("World weapon publication changed its source definition");
                const auto root=source.object_transform(object.object);
                if(std::memcmp(&found->position,&root.position,sizeof(Vec3))||
                   std::memcmp(&found->forward,&root.basis[0],sizeof(Vec3))||
                   std::memcmp(&found->up,&root.basis[2],sizeof(Vec3)))
                    throw FormatError("World weapon publication changed the completed source root");
                const auto body=source.object_body(object.object);const auto& memory=source.memory();
                const auto word=[&](unsigned offset){return Bytes(memory.view(body+offset,4)).u32(0);};
                if(found->shader.item_flags!=word(0x12c)||found->shader.weapon_flags!=word(0x16c)||
                   std::bit_cast<std::uint32_t>(found->shader.heat)!=word(0x180)||
                   std::bit_cast<std::uint32_t>(found->shader.power_fraction)!=word(0x184))
                    throw FormatError("World weapon publication changed actual source material inputs");
                const auto& definition=world.content.weapons.at(object.definition);int loaded{},reserve{};
                if(!definition.triggers.empty()&&definition.triggers[0].primary_barrel>=0) {
                    const auto barrel=unsigned(definition.triggers[0].primary_barrel);
                    const auto magazine=definition.barrel_ammunition.at(barrel).magazine;
                    if(magazine>=0) {
                        if(magazine>=2)throw FormatError("Authored magazine exceeds the original runtime banks");
                        loaded=std::int16_t(Bytes(memory.view(body+0x22c+unsigned(magazine)*16,2)).u16(0));
                        reserve=std::int16_t(Bytes(memory.view(body+0x22a+unsigned(magazine)*16,2)).u16(0));
                    }
                }
                if(found->loaded!=loaded||found->reserve!=reserve)throw FormatError("World weapon publication changed actual source ammunition");
                ++checked;
            }
            for(const auto& weapon:state.world_weapons)
                if(!published.insert(weapon.handle).second)throw FormatError("World weapon publication repeated a full source handle");
            if(expected!=published)throw FormatError("World weapon publication included an attached or unowned weapon");
            row["checked_loose_owners"]=expected.size();
        }
        if(!checked)throw FormatError("No actual loose source weapon was exercised");
        report["checked_loose_owner_samples"]=checked;
        // Exercise the real command consumer after publication. Scene placement
        // is a declared input; target admission and the transfer stay original.
        auto& source=*world.source_objects;auto state=world.match.state();
        const auto pickup=std::find_if(state.world_weapons.begin(),state.world_weapons.end(),[&](const auto& item) {
            return world.content.weapons.at(item.definition).name.ends_with("battle_rifle");
        });
        if(pickup==state.world_weapons.end())throw FormatError("No actual map battle rifle for source command regression");
        const auto target=pickup->handle;Spawn at;at.position=pickup->position;at.position.x-=.2f;at.bsp=0;
        report["pickup_target"]=target;report["diagnostic_placement"]="Original B95D0 at actual loose battle rifle root minus0.2X";
        auto& samples=report["pickup_commands"]=nlohmann::json::array();bool equipped{};
        unsigned prompt_checks{},prompt_mismatches{};
        for(unsigned tick=0;tick<64&&!equipped;++tick) {
            state=world.match.state();const auto unit=state.players[0].source_unit;
            test::place_player(world.match,0,at);source.tu15_world().set_object_position(unit,at.position);
            neutral[0].pickup=true;neutral[0].pickup_milliseconds=tick*1000/30;
            world.match.step(neutral);state=world.match.state();
            const auto word=[&](std::uint32_t address){return Bytes(source.memory().view(address,4)).u32(0);};
            const auto player=world.match.player(0).source_datum;
            const auto command=0x6002800cu+(player&65535u)*0x5cu;
            const auto& prompt=state.players[0].action_prompt;
            const auto published_action=std::uint16_t(prompt.target.action)|
                (std::uint32_t(std::uint16_t(prompt.target.index))<<16);
            samples.push_back({{"tick",tick},{"original_action_word",word(0x60041000u)},
                {"original_action_object",word(0x60041004u)},{"command_action_word",word(command+0x28u)},
                {"command_action_object",word(command+0x2cu)},{"held_weapon",state.players[0].source_weapon},
                {"published_action_word",published_action},{"published_action_object",prompt.target.object}});
            if(word(0x60041004u)==target) {
                if(!prompt_checks) {
                    unsigned writes{};
                    struct ReadScope {
                        const SourceRuntimeMemory& memory;const void* identity;
                        ~ReadScope(){source_runtime_unregister_write_observer(memory,identity);}
                    } read_scope{source.memory(),&writes};
                    source_runtime_register_write_observer(source.memory(),&writes,[&](auto,auto){++writes;});
                    const auto first=world.match.action_prompt(0),second=world.match.action_prompt(0);
                    report["repeated_prompt_read_source_writes"]=writes;
                    if(writes||first.target!=second.target||first.definition!=second.definition)
                        throw FormatError("Repeated weapon prompt observation mutated source state or changed its result");
                    for(const auto [query_player,query_unit]:std::array{
                        std::pair{null_datum,unit},std::pair{player,null_datum},
                        std::pair{player^0x10000u,unit},std::pair{player,unit^0x10000u}})
                        if(source.tu15_world().player_action_target(query_player,query_unit))
                            throw FormatError("Source action prompt accepted an absent or stale full player/unit generation");
                    report["invalid_prompt_generation_controls"]=4;
                }
                ++prompt_checks;prompt_mismatches+=published_action!=word(0x60041000u)||prompt.target.object!=target;
            }
            equipped=state.players[0].source_weapon==target;
        }
        report["actual_map_pickup_completed"]=equipped;
        if(!equipped)throw FormatError("Actual map battle rifle pickup did not complete through original commands");
        report["source_weapon_prompt_checks"]=prompt_checks;report["source_weapon_prompt_mismatches"]=prompt_mismatches;
        if(!prompt_checks||prompt_mismatches)throw FormatError("Completed original weapon target differs from native action prompt");
        report["passed"]=true;
    }catch(const std::exception& error){report["runtime_error"]=error.what();}
    std::ofstream output(argv[3]);output<<report.dump(2)<<'\n';
    std::cout<<report.value("runtime_error",std::string("Source world weapon publication matches actual native owners"))<<'\n';
    return output&&report["passed"].get<bool>()?0:1;
}
