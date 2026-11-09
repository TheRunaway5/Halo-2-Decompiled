#include "halo2/custom_slayer.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_physics_entry_observation.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace halo2;
namespace {
using Json=nlohmann::json;
void need(bool ok,const char *why){if(!ok)throw FormatError(why);}
std::uint32_t word(const SourceRuntimeMemory &memory,std::uint32_t at,unsigned bytes=4){
    const Bytes value(memory.view(at,bytes));return bytes==2?value.u16(0):value.u32(0);
}
struct Files {
    std::filesystem::path path;
    explicit Files(std::string_view name){const auto scratch=std::getenv("HALO2_TEST_SCRATCH");
        need(scratch,"Custom powerup regression requires owned test scratch");
        path=std::filesystem::path(scratch)/("native-powerup-"+std::string(name));
        need(std::filesystem::create_directory(path),"Cannot create owned powerup World directory");}
    ~Files(){std::error_code ignored;std::filesystem::remove_all(path,ignored);}
};
Json exercise(const std::filesystem::path &map,unsigned row,const UiVariant &variant,bool custom){
    Json result{{"passed",false},{"custom",custom},{"variant",variant.name},{"netgame_row",row},
        {"stimulus","Only genuine authored15D4F0 equipment birth when absent, and public sourceB95D0 entrant position inputs"}};
    try{
        Files files(custom?"fiesta":"classic");MultiplayerOptions options;
        options.map=map;options.shared_maps=map.parent_path();options.variant=variant;
        options.local_players=1;options.mute=true;options.source_object_world=true;
        options.source_object_savegame_directory=files.path;
        MultiplayerPreparedWorld prepared(options);auto &match=prepared.match;
        need(prepared.cache->is_mcc() && prepared.source_objects && prepared.source_objects->owns_physics(),
            "Custom powerup regression needs its genuine MCC native World");
        need(custom?(prepared.rules.custom==CustomSlayer::fiesta):prepared.rules.requires_source_engine,
            "Powerup comparison selected the wrong physical frame owner");
        auto &objects=*prepared.source_objects;auto &world=objects.tu15_world();const auto slot=prepared.local_slots.front();
        for(unsigned n=0;n<15;++n)match.step({});
        const auto unit=match.player(slot).source_current_object,player=match.player(slot).source_datum;
        need(unit!=null_datum && player!=null_datum,"Powerup fixture has no genuine player unit");
        const auto pool=world.source_player_pool(),records=word(world.memory(),pool+0x44u),stride=word(world.memory(),pool+0x24u);
        need(stride==0x21cu,"Powerup source player record stride changed");const auto player_row=records+(player&65535u)*stride;
        need(word(world.memory(),player_row,2)==(player>>16u),"Powerup source player generation changed");
        const auto timer_address=player_row+0x198u;
        const auto observe=[&](){
            need(match.player(slot).source_current_object==unit,"Powerup entrant source generation changed");
            const auto body=world.source_object_body(unit);
            const auto timer=std::bit_cast<std::int16_t>(std::uint16_t(word(world.memory(),timer_address,2)));
            const auto primary=std::bit_cast<float>(word(world.memory(),body+0x2b0u));
            const auto fade=std::bit_cast<float>(word(world.memory(),body+0x2b8u));
            need(std::isfinite(primary) && std::isfinite(fade),"Powerup native camouflage became nonfinite");
            return Json{{"tick",match.state().tick},{"timer",timer},{"primary",primary},{"fade_speed",fade},
                {"native_action_word",word(world.memory(),body+0x134u)},
                {"adopted_timer",match.player(slot).source_powerup_timers[0]},
                {"adopted_primary",match.state().players.at(slot).source_camouflage.primary}};
        };
        auto item=objects.netgame_object(row);const bool declared_birth=item==null_datum;
        if(declared_birth)item=objects.create_netgame_object(row);
        need(item!=null_datum,"Original netgame owner failed to birth the declared authored equipment");
        const auto definition=objects.object_definition(item);const auto metadata=prepared.cache->load_source_tag_metadata(definition);
        const Bytes equipment(metadata.bytes_at(metadata.source_address(),metadata.size()));
        need(equipment.u16(0x12c)==3 && equipment.f32(0x130)>0,"Selected collection did not produce its authored camouflage powerup");
        const auto before_position=objects.biped_motion(unit).position,point=objects.object_transform(item).position;
        result["unit"]=unit;result["player"]=player;result["item"]=item;result["definition"]=definition;
        result["declared_authored_birth"]=declared_birth;result["authored_duration_seconds"]=equipment.f32(0x130);
        result["contact_position"]={point.x,point.y,point.z};result["before"]=observe();
        std::uint64_t postpass_entries{},postpass_returns{},timer_stores{};Json examples=Json::array();
        SourcePhysicsEntryObserverBinding entries(world.memory(),[&](bool entering,auto owner,const auto&){
            if(owner==0x1503d0u)(entering?postpass_entries:postpass_returns)++;
        });
        world.observe_physics_stores_read_only([&](auto pc,const auto& store){
            if(pc!=0x154d3eu || store.address!=timer_address)return;
            need(store.before.size()==2 && store.written.size()==2,"Native154D00 timer store width changed");
            ++timer_stores;
            if(examples.size()<12 || (store.written[0]==std::byte{} && store.written[1]==std::byte{}))
                examples.push_back({{"instruction",pc},{"address",store.address},
                    {"before",Bytes(store.before).u16(0)},{"after",Bytes(store.written).u16(0)}});
        });
        struct Cleanup {PhysicsLiveWorldTu15 &world;~Cleanup(){world.observe_physics_stores_read_only({});}} cleanup{world};
        world.set_object_position(unit,point);bool consumed{};unsigned contact_ticks{};
        for(;contact_ticks<60 && !consumed;++contact_ticks){
            match.step({});const auto live=objects.live_objects();
            consumed=std::ranges::none_of(live,[&](const auto &object){return object.object==item;});
        }
        result["consumed"]=consumed;result["contact_ticks"]=contact_ticks;result["at_contact"]=observe();
        // Leave before a replacement can be consumed. No timer/camo/death
        // field is ever written by the fixture or supplied as an outcome.
        world.set_object_position(unit,before_position);const auto first=observe();
        for(unsigned tick=0;tick<10;++tick)match.step({});const auto later=observe();
        result["after10"]=later;result["native_player_postpass_entries"]=postpass_entries;
        result["native_player_postpass_returns"]=postpass_returns;result["native_timer_stores"]=timer_stores;
        result["native_timer_store_examples"]=examples;
        need(consumed && first["timer"].get<int>()>10 && later["timer"].get<int>()==first["timer"].get<int>()-10 &&
            later["primary"].get<float>()>0 && postpass_entries==postpass_returns && timer_stores>=10,
            "Genuine authored camouflage pickup did not reach native1503D0/154D00 timer decay and body fade");
        unsigned expiry{};bool expired{};
        for(;expiry<2100;++expiry){
            const auto now=observe();if(now["timer"]==0 && now["primary"].get<float>()==0.f){expired=true;break;}
            match.step({});
        }
        result["expiry_ticks"]=expiry;result["expired"]=observe();result["native_player_postpass_entries"]=postpass_entries;
        result["native_player_postpass_returns"]=postpass_returns;result["native_timer_stores"]=timer_stores;
        result["native_timer_store_examples"]=examples;
        need(expired && postpass_entries==postpass_returns,"Native camouflage timer/fade did not expire on its original player generation");
        need(result["expired"]["adopted_timer"]==0 && result["expired"]["adopted_primary"].get<float>()==0.f &&
            later["adopted_timer"]==later["timer"] && later["adopted_primary"]==later["primary"],
            "Custom native timer/camouflage adoption differs from actual source body/player readback");
        result["passed"]=true;
    }catch(const std::exception &error){result["error"]=error.what();}
    return result;
}
}
int main(int argc,char **argv)try{
    std::cout<<std::unitbuf;if(argc!=4)return 2;
    if(!std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_directory(argv[2]))return 77;
    const Cache cache(argv[1]);need(cache.is_mcc(),"Powerup test needs prepared MCC input");const auto content=cache.load_content(0,true);
    unsigned selected=unsigned(content.netgame_equipment.size());
    for(unsigned row=0;row<content.netgame_equipment.size();++row){
        const auto &placement=content.netgame_equipment[row];if(placement.items.size()!=1 || placement.items.front().group!=0x65716970u)continue;
        const auto metadata=cache.load_source_tag_metadata(placement.items.front().definition);
        const Bytes equipment(metadata.bytes_at(metadata.source_address(),metadata.size()));
        if(equipment.u16(0x12c)==3 && equipment.f32(0x130)>0){selected=row;break;}
    }
    need(selected<content.netgame_equipment.size(),"Map lacks a single-item authored camouflage row");
    const auto variants=load_custom_slayer_variants(argv[2]);const auto fiesta=std::ranges::find_if(variants,
        [](const auto &variant){return custom_slayer(variant)==CustomSlayer::fiesta;});
    need(fiesta!=variants.end(),"Actual bundled Fiesta definition absent");UiVariant classic;classic.name="Classic Slayer";classic.engine=0;
    Json report{{"passed",false},{"source_results_supplied",false},{"selected_authored_row",selected},{"cases",Json::array()}};
    for(const auto custom:{false,true}){
        report["cases"].push_back(exercise(argv[1],selected,custom?*fiesta:classic,custom));
        std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';need(bool(out),"Cannot retain custom powerup report");
        std::cout<<report["cases"].back().dump()<<'\n';
    }
    report["passed"]=std::ranges::all_of(report["cases"],[](const auto &row){return row["passed"].template get<bool>();});
    std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';need(bool(out),"Cannot retain final custom powerup report");
    return report["passed"].get<bool>()?0:1;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
