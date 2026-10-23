#include "halo2/multiplayer_world.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <map>
#include <set>

using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool ok,const char *why){if(!ok)throw FormatError(why);}
}
int main(int argc,char **argv)try {
    check(argc==3 || (argc==4 && std::string(argv[3])=="--all-maps"),"Supply prepared MCC Headlong (or all-map directory), report path and optional --all-maps");
    const bool all_maps=argc==4;
    if(all_maps?!std::filesystem::is_directory(argv[1]):!std::filesystem::is_regular_file(argv[1]))return 77;
    const auto scratch=std::getenv("HALO2_TEST_SCRATCH");check(scratch,"Equipment acceptance requires owned scratch");
    Json report={{"passed",false},{"cases",Json::array()},
        {"stimulus","A genuine player is placed at an existing authored pickup using original B95D0; subsequent source frames own admission, application and deletion. No shield, camouflage, timer or grenade result is assigned."}};
    const auto save=[&]{std::ofstream out(argv[2]);out<<report.dump(2)<<'\n';check(bool(out),"Cannot retain equipment report");};
    unsigned failures{},placement_rows{},choices{};
    std::vector<std::filesystem::path> maps;
    if(all_maps)for(const auto name:std::array{"ascension","beavercreek","burial_mounds","coagulation","colossus",
        "cyclotron","foundation","headlong","lockout","midship","waterworks","zanzibar","containment","warlock",
        "deltatap","turf","backwash","dune","elongation","gemini","triplicate"})maps.push_back(std::filesystem::path(argv[1])/(std::string(name)+".map"));
    else maps.emplace_back(argv[1]);
    for(const auto &map:maps) {
    Cache cache(map);check(cache.is_mcc(),"Equipment gameplay acceptance requires prepared MCC assets");
    const auto content=cache.load_content(0,true);
    struct Case {unsigned row{},type{},grenade{};std::uint32_t seed=0x78a8;};std::vector<Case> cases;std::set<std::pair<unsigned,unsigned>> kinds;
    std::map<unsigned,std::set<std::uint32_t>> expected_choices,seen_choices;
    for(unsigned row=0;row<content.netgame_equipment.size();++row) {
        const auto &placement=content.netgame_equipment[row];
        if(all_maps) {
            for(const auto &item:placement.items)if(item.group==0x65716970u) {
                const auto metadata=cache.load_source_tag_metadata(item.definition);
                const auto type=Bytes(metadata.bytes_at(metadata.source_address()+0x12c,2)).u16(0);
                if(type==2 || type==3)expected_choices[row].insert(item.definition);
            }
            if(expected_choices.contains(row)) {
                ++placement_rows;choices+=unsigned(expected_choices[row].size());
                const auto attempts=expected_choices[row].size()>1?16u:1u;
                for(unsigned seed=0;seed<attempts;++seed)cases.push_back({row,0,0,0x78a8u+seed});
            }
            continue;
        }
        if(placement.items.size()!=1 || placement.items.front().group!=0x65716970u)continue;
        const auto metadata=cache.load_source_tag_metadata(placement.items.front().definition);
        const auto p=metadata.source_address();const Bytes definition(metadata.bytes_at(p,metadata.size()));
        const auto type=definition.u16(0x12c),grenade=definition.u16(0x12e);
        if(type!=0 && type!=2 && type!=3 && type!=6)continue;
        if(kinds.emplace(type,type==6?grenade:0).second)cases.push_back({row,type,grenade});
    }
    if(!all_maps)check(cases.size()==5,"Selected map must contain an authored zero-duration ammo box, both powerups and both grenade pickups");
    for(auto c:cases) {
        if(all_maps && seen_choices[c.row]==expected_choices[c.row])continue;
        Json row={{"map",map.filename().string()},{"row",c.row},{"powerup_type",c.type},{"grenade_type",c.grenade},{"passed",false},{"stage","construct"},{"session_seed",c.seed}};
        try {
            MultiplayerOptions options;options.map=map;options.local_players=1;options.source_object_world=true;options.session_random_seed=c.seed;
            options.source_object_savegame_directory=std::filesystem::path(scratch)/(map.stem().string()+"-equipment-"+std::to_string(c.row)+"-"+std::to_string(c.seed));
            MultiplayerPreparedWorld prepared(options);auto &match=prepared.match;auto &objects=*prepared.source_objects;
            check(prepared.rules.requires_source_engine,"Equipment test selected a bounded match adapter");
            const auto slot=prepared.local_slots.at(0);
            for(unsigned n=0;n<15;++n)match.step({});
            auto object=objects.netgame_object(c.row);
            if(object==null_datum)object=objects.create_netgame_object(c.row);
            check(object!=null_datum,"Original netgame owner did not construct the authored equipment");
            const auto chosen=objects.object_definition(object);
            if(all_maps) {
                check(expected_choices.at(c.row).contains(chosen),"Original collection chose an unlisted powerup definition");
                const auto metadata=cache.load_source_tag_metadata(chosen);
                c.type=Bytes(metadata.bytes_at(metadata.source_address()+0x12c,2)).u16(0);row["powerup_type"]=c.type;
                row["chosen_definition"]=chosen;
            }
            const auto unit=match.player(slot).source_current_object;check(unit!=null_datum,"No genuine source player unit");
            const auto before=match.state().players.at(slot);const auto position=objects.object_transform(object).position;
            unsigned maximum{};std::vector<std::uint32_t> grenade_objects;
            if(c.type==6) {
                check(c.grenade<2,"Grenade type escapes the actual two-type inventory");
                const auto globals=cache.query_tag({.group=tag_groups::globals});check(bool(globals),"No authored globals");
                const auto metadata=cache.load_source_tag_metadata(globals->datum);
                const Bytes block(metadata.bytes_at(metadata.source_address()+0x100,8));
                check(c.grenade<block.u32(0),"Grenade type escapes its authored globals block");
                maximum=Bytes(metadata.bytes_at(block.u32(4)+c.grenade*0x2c,2)).u16(0);
                const auto definition=objects.object_definition(object);
                for(const auto &o:objects.live_objects())if(o.type==3 && o.definition==definition)
                    grenade_objects.push_back(o.object);
                row["maximum"]=maximum;
            }
            row["object"]=object;row["unit"]=unit;row["position"]={position.x,position.y,position.z};row["stage"]="pickup";
            row["respawn_seconds"]=content.netgame_equipment.at(c.row).respawn_seconds;
            row["grenades_before"]=before.grenades;
            objects.tu15_world().set_object_position(unit,position);
            bool consumed{};unsigned frames{};
            for(;frames<60 && !consumed;++frames) {
                match.step({});const auto live=objects.live_objects();
                consumed=std::none_of(live.begin(),live.end(),[&](const auto &o){return o.object==object;});
                // Original contact iteration can consume neighboring items
                // before this row. A full inventory correctly leaves it live.
                if(c.type==6 && match.state().players.at(slot).grenades[c.grenade]>=maximum) {++frames;break;}
            }
            row["consumed"]=consumed;row["frames"]=frames;
            const auto contact=match.state().players.at(slot);
            row["grenades_at_contact"]=contact.grenades;
            row["player_at_contact"]={contact.position.x,contact.position.y,contact.position.z};
            if(!consumed) {
                const auto remaining=objects.object_transform(object).position;
                row["item_at_contact"]={remaining.x,remaining.y,remaining.z};
            }
            check(consumed || c.type==0 || (c.type==6 && contact.grenades[c.grenade]>=maximum),
                "Ordinary source frames did not consume the authored equipment with inventory space");
            for(unsigned n=0;n<30;++n)match.step({});
            const auto after=match.state().players.at(slot);row["shield"]=after.shield;
            row["camouflage"]=after.source_camouflage.primary;row["timers"]=match.player(slot).source_powerup_timers;
            row["grenades_before"]=before.grenades;row["grenades_after"]=after.grenades;
            if(c.type==0) {
                const auto metadata=cache.load_source_tag_metadata(objects.object_definition(object));
                const Bytes definition(metadata.bytes_at(metadata.source_address(),metadata.size()));
                check(definition.f32(0x130)==0.f && !consumed && objects.netgame_object(c.row)==object &&
                    after.grenades==before.grenades && after.source_camouflage.primary==before.source_camouflage.primary,
                    "Zero-duration authored ammo box changed its original no-powerup behavior");
                row["expected_behavior"]="Original151EA0 rejects zero-duration equipment before application/deletion";
            }
            if(c.type==2) {
                check(after.shield>1.f,"Consumed overshield did not charge the actual source shield");
                for(unsigned n=0;n<40;++n)match.step({});
                const auto charged=match.state().players.at(slot).shield;
                check(charged>2.9f && charged<=3.f,"Source overshield did not finish its three-layer charge");
                for(unsigned n=0;n<30;++n)match.step({});
                const auto drained=match.state().players.at(slot).shield;
                check(drained<charged && drained>1.f,"Source multiplayer overshield did not drain after charge");
                row["charged_shield"]=charged;row["drained_shield"]=drained;
            }
            if(c.type==3)check(after.source_camouflage.primary>0 && match.player(slot).source_powerup_timers[0]>0,
                "Consumed camouflage did not reach the actual timer and fade owners");
            if(c.type==6) {
                const auto live=objects.live_objects();
                const auto removed=std::count_if(grenade_objects.begin(),grenade_objects.end(),[&](auto handle) {
                    return std::none_of(live.begin(),live.end(),[&](const auto &o){return o.object==handle;});
                });
                row["consumed_grenade_objects"]=removed;
                check(c.grenade<2 && removed>0 && after.grenades[c.grenade]<=maximum &&
                    after.grenades[c.grenade]-before.grenades[c.grenade]==removed,
                    "Consumed grenade generations do not match the actual source inventory increment");
                if(!consumed)check(after.grenades[c.grenade]==maximum &&
                    std::any_of(live.begin(),live.end(),[&](const auto &o){return o.object==object;}),
                    "Full grenade inventory consumed an additional authored item");
            }
            // Leave the contact before waiting: standing on a replacement
            // would consume it again and hide the ordinary respawn event.
            objects.tu15_world().set_object_position(unit,before.position);
            if(c.type!=0) {
                unsigned respawn_row=c.row;
                if(!consumed)for(unsigned r=0;r<content.netgame_equipment.size();++r) {
                    const auto &placement=content.netgame_equipment[r];
                    if(placement.items.size()==1 && placement.items.front().definition==objects.object_definition(object) &&
                        objects.netgame_object(r)==null_datum) {respawn_row=r;break;}
                }
                const auto old=respawn_row==c.row?object:null_datum;
                std::uint32_t replacement=null_datum;unsigned waited{};
                const auto seconds=content.netgame_equipment.at(respawn_row).respawn_seconds;
                check(seconds>0,"Respawn control requires an authored positive timer");
                for(;waited<unsigned(seconds)*30u+300u;++waited) {
                    match.step({});replacement=objects.netgame_object(respawn_row);
                    if(replacement!=null_datum && replacement!=old) {++waited;break;}
                }
                row["respawn_row"]=respawn_row;row["respawn_wait_frames"]=waited;row["replacement"]=replacement;
                check(replacement!=null_datum && replacement!=old,"Ordinary source netgame frames did not create a new equipment generation");
            }
            if(c.type==3) {
                unsigned waited{};
                for(;waited<1800 && (match.player(slot).source_powerup_timers[0]>0 ||
                    match.state().players.at(slot).source_camouflage.primary>0.f);++waited)match.step({});
                row["expiry_wait_frames"]=waited;row["expired_camouflage"]=match.state().players.at(slot).source_camouflage.primary;
                row["expired_timer"]=match.player(slot).source_powerup_timers[0];
                check(match.player(slot).source_current_object==unit && !match.state().players.at(slot).dead &&
                    match.player(slot).source_powerup_timers[0]==0 && match.state().players.at(slot).source_camouflage.primary==0.f,
                    "Camouflage did not expire and restore its actual zero-primary state");
            }
            row["passed"]=true;row["stage"]="complete";
            if(all_maps)seen_choices[c.row].insert(chosen);
        }catch(const std::exception &e){row["error"]=e.what();++failures;}
        report["cases"].push_back(row);save();std::cout<<row.dump()<<'\n';
    }
    for(const auto &[row,expected]:expected_choices)if(seen_choices[row]!=expected) {
        ++failures;report["cases"].push_back({{"map",map.filename().string()},{"row",row},{"passed",false},
            {"error","Declared session seeds did not complete every actual collection permutation"}});
    }
    }
    if(all_maps) {
        report["powerup_rows"]=placement_rows;report["powerup_choices"]=choices;
        check(placement_rows==16 && choices==18,"All-map powerup census changed its original row/permutation scope");
    }
    report["passed"]=failures==0;save();return failures?1:0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
