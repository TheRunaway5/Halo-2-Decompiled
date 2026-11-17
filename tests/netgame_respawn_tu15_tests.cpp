#include "halo2/multiplayer_world.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <set>

// Ordinary source frames own every netgame row decision here. The stimuli are
// a moved weapon, a deleted vehicle and a moved (abandoned) vehicle. Nothing
// assigns a timer, row binding or replacement: original 15D890/15C4E0 and
// the native match adapter produce them. The run also counts live
// generations per authored vehicle definition so that a second row owner
// cannot add vehicles beside the original's own replacement.
using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool ok,const char *why){if(!ok)throw FormatError(why);}
float distance(Vec3 a,Vec3 b){const auto d=a-b;return std::sqrt(dot(d,d));}
}
int main(int argc,char **argv)try {
    check(argc==3,"Supply a prepared MCC map with netgame vehicles and weapons, then the report path");
    if(!std::filesystem::is_regular_file(argv[1]))return 77;
    const auto scratch=std::getenv("HALO2_TEST_SCRATCH");check(scratch,"Netgame respawn acceptance requires owned scratch");
    const std::filesystem::path map=argv[1];
    Json report={{"passed",false},{"map",map.filename().string()},{"cases",Json::array()}};
    const auto save=[&]{std::ofstream out(argv[2]);out<<report.dump(2)<<'\n';check(bool(out),"Cannot retain netgame respawn report");};
    MultiplayerOptions options;options.map=map;options.local_players=1;options.source_object_world=true;
    options.session_random_seed=0x5eed1234u;
    options.source_object_savegame_directory=std::filesystem::path(scratch)/(map.stem().string()+"-netgame-respawn");
    MultiplayerPreparedWorld prepared(options);auto &match=prepared.match;auto &objects=*prepared.source_objects;
    const auto &content=prepared.content;
    report["source_engine_owned"]=match.source_engine_owned();
    report["requires_source_engine"]=prepared.rules.requires_source_engine;
    unsigned tick{};
    const auto step=[&](unsigned count){for(unsigned n=0;n<count;++n){match.step({});++tick;}};
    // Original 15AD30 arms every row with timer1; 15D890 visits each row
    // once per source second, so all initial generations exist by tick 60.
    step(90);
    const auto vehicle_census=[&] {
        std::map<std::uint32_t,unsigned> census;
        for(const auto &o:objects.live_objects())if(o.type==1 && o.parent==null_datum)++census[o.definition];
        return census;
    };
    // Live unparented vehicles that no netgame row owns. Before any stimulus
    // the original has no reason to leave one: each would be a second
    // generation created beside the row's own vehicle.
    const auto unbound_vehicles=[&] {
        std::set<std::uint32_t> bound;
        for(unsigned row=0;row<content.netgame_equipment.size() && row<100;++row)
            if(const auto object=objects.netgame_object(row);object!=null_datum)bound.insert(object);
        unsigned count{};
        for(const auto &o:objects.live_objects())if(o.type==1 && o.parent==null_datum && !bound.contains(o.object))++count;
        return count;
    };
    // Live unparented source vehicles absent from the native scene, whose
    // objects alone feed vehicle presentation and seat/flip consumers.
    const auto unpresented_vehicles=[&] {
        std::set<std::uint32_t> scene;
        for(const auto &o:match.simulation().scene().objects())if(o.handle!=null_datum)scene.insert(o.handle);
        unsigned count{};
        for(const auto &o:objects.live_objects())if(o.type==1 && o.parent==null_datum && !scene.contains(o.object))++count;
        return count;
    };
    // Initial rows and the authored vehicle count each definition may own.
    std::map<std::uint32_t,unsigned> authored_vehicles;
    Json rows=Json::array();
    int weapon_row=-1,vehicle_row=-1,abandon_row=-1;
    for(unsigned row=0;row<content.netgame_equipment.size() && row<100;++row) {
        const auto object=objects.netgame_object(row);
        const auto &d=content.netgame_equipment[row];
        Json r={{"row",row},{"classification",d.classification},{"object",object},{"flags",d.flags},
            {"respawn_seconds",d.respawn_seconds},{"empty_seconds",d.empty_seconds},{"timer_start",d.timer_start},
            {"collection_delay",d.collection_delay}};
        if(object!=null_datum) {
            const auto live=objects.live_objects();
            const auto found=std::find_if(live.begin(),live.end(),[&](const auto &o){return o.object==object;});
            check(found!=live.end(),"Row retains an object absent from the live source bank");
            r["type"]=found->type;r["definition"]=found->definition;
            if(found->type==1)++authored_vehicles[found->definition];
            if(found->type==2 && weapon_row<0 && d.timer_start==0)weapon_row=int(row);
            if(found->type==1 && vehicle_row<0)vehicle_row=int(row);
            else if(found->type==1 && abandon_row<0 && d.empty_seconds>0)abandon_row=int(row);
        }
        rows.push_back(r);
    }
    report["rows"]=rows;save();
    unsigned failures{},peak_excess{};
    // Untouched baseline: longer than the 30-second fallback delay.
    {
        unsigned peak_unbound{},first_unbound_tick{},peak_unpresented{},first_unpresented_tick{};
        const unsigned initial_unpresented=unpresented_vehicles();
        for(unsigned n=0;n<75*30;++n) {
            step(1);
            if(n%30)continue;
            const auto unbound=unbound_vehicles(),unpresented=unpresented_vehicles();
            if(unbound && !peak_unbound)first_unbound_tick=tick;
            if(unpresented && !peak_unpresented)first_unpresented_tick=tick;
            peak_unbound=std::max(peak_unbound,unbound);
            peak_unpresented=std::max(peak_unpresented,unpresented);
        }
        Json c={{"case","idle_baseline"},{"seconds",75},{"peak_unbound_vehicles",peak_unbound},
            {"first_unbound_tick",first_unbound_tick},{"initial_unpresented_vehicles",initial_unpresented},
            {"peak_unpresented_vehicles",peak_unpresented},{"first_unpresented_tick",first_unpresented_tick},
            {"passed",peak_unbound==0 && peak_unpresented==0}};
        Json census=Json::object();
        for(const auto &[definition,count]:vehicle_census())census[std::to_string(definition)]=count;
        c["vehicle_census"]=census;
        report["cases"].push_back(c);save();std::cout<<c.dump()<<'\n';
        if(peak_unbound || peak_unpresented)++failures;
    }
    const auto effective=[&](unsigned row) {
        const auto &d=content.netgame_equipment.at(row);
        return d.respawn_seconds?int(d.respawn_seconds):d.collection_delay?int(d.collection_delay):30;
    };
    Json duplicates=Json::array();
    unsigned peak_case_unpresented{},first_case_unpresented_tick{};
    const auto observe_duplicates=[&] {
        if(const auto unpresented=unpresented_vehicles()) {
            if(!peak_case_unpresented)first_case_unpresented_tick=tick;
            peak_case_unpresented=std::max(peak_case_unpresented,unpresented);
        }
        for(const auto &[definition,count]:vehicle_census()) {
            const auto authored=authored_vehicles.contains(definition)?authored_vehicles.at(definition):0u;
            if(count>authored) {
                peak_excess=std::max(peak_excess,count-authored);
                if(duplicates.size()<32)duplicates.push_back({{"tick",tick},{"definition",definition},
                    {"live",count},{"authored",authored}});
            }
        }
    };
    const auto wait_for_replacement=[&](unsigned row,std::uint32_t old,unsigned limit) {
        std::uint32_t replacement=null_datum;unsigned waited{};
        for(;waited<limit;++waited) {
            step(1);observe_duplicates();
            replacement=objects.netgame_object(row);
            if(replacement!=null_datum && replacement!=old)break;
        }
        return std::pair{replacement,waited};
    };
    const auto run_case=[&](const char *name,int row,auto &&stimulus,unsigned limit) {
        Json c={{"case",name},{"row",row},{"passed",false}};
        try {
            check(row>=0,"Map has no qualifying authored row for this case");
            const auto old=objects.netgame_object(unsigned(row));
            check(old!=null_datum,"Row has no live generation before the stimulus");
            const auto spawn=content.netgame_equipment.at(unsigned(row)).position;
            c["old"]=old;c["effective_respawn_seconds"]=effective(unsigned(row));
            stimulus(old,spawn,c);
            const auto [replacement,waited]=wait_for_replacement(unsigned(row),old,limit);
            c["replacement"]=replacement;c["wait_ticks"]=waited;c["wait_seconds"]=double(waited)/30.;
            check(replacement!=null_datum && replacement!=old,"Ordinary source frames did not create a replacement generation");
            const auto at=objects.object_transform(replacement).position;
            c["replacement_distance_from_spawn"]=distance(at,spawn);
            const auto live=objects.live_objects();
            c["old_still_live"]=std::any_of(live.begin(),live.end(),[&](const auto &o){return o.object==old;});
            c["passed"]=true;
        }catch(const std::exception &e){c["error"]=e.what();++failures;}
        report["cases"].push_back(c);save();std::cout<<c.dump()<<'\n';
    };
    // Weapon moved more than one world unit: timer-start0 begins the delay.
    run_case("weapon_moved",weapon_row,[&](std::uint32_t old,Vec3 spawn,Json &c) {
        objects.tu15_world().set_object_position(old,spawn+Vec3{1.5f,0,.25f});step(2);
        c["moved_distance"]=distance(objects.object_transform(old).position,spawn);
    },unsigned(effective(weapon_row<0?0u:unsigned(weapon_row)))*30u+300u);
    // A deleted vehicle releases its row and waits for the authored delay.
    run_case("vehicle_destroyed",vehicle_row,[&](std::uint32_t old,Vec3,Json &) {
        // Complete original BA3E0/BA490 deletion through the scene owner,
        // which publishes the retired identity to native presentation.
        match.destroy_world_object(old);
    },unsigned(effective(vehicle_row<0?0u:unsigned(vehicle_row)))*30u+300u);
    // An unoccupied vehicle away from its spawn counts its empty seconds.
    run_case("vehicle_abandoned",abandon_row,[&](std::uint32_t old,Vec3 spawn,Json &c) {
        // Original 15D890 counts empty seconds only beyond 10 world units
        // (squared 4500B8=100) of the authored row. Use the farthest other
        // authored vehicle spawn: known open ground, offset sideways.
        Vec3 destination=spawn;float farthest{};
        for(unsigned row=0;row<content.netgame_equipment.size();++row) {
            const auto &d=content.netgame_equipment[row];
            if(d.classification==0 || int(row)==abandon_row)continue;
            if(const auto far=distance(d.position,spawn);far>farthest){farthest=far;destination=d.position;}
        }
        check(farthest>14.f,"Map has no second vehicle area beyond the original 10-unit radius");
        objects.tu15_world().set_object_position(old,destination+Vec3{3.f,3.f,1.5f});step(2);
        c["moved_distance"]=distance(objects.object_transform(old).position,spawn);
        c["empty_seconds"]=content.netgame_equipment.at(unsigned(abandon_row)).empty_seconds;
    },abandon_row<0?0u:unsigned(content.netgame_equipment.at(unsigned(abandon_row)).empty_seconds+
        effective(unsigned(abandon_row))+10)*30u);
    // Idle frames after every replacement: no unrequested second generation.
    for(unsigned n=0;n<90*30;++n){step(1);observe_duplicates();}
    Json census=Json::object();
    for(const auto &[definition,count]:vehicle_census())census[std::to_string(definition)]=
        {{"live",count},{"authored",authored_vehicles.contains(definition)?authored_vehicles.at(definition):0u}};
    report["final_vehicle_census"]=census;report["duplicate_samples"]=duplicates;
    report["peak_vehicle_excess"]=peak_excess;report["ticks"]=tick;
    report["final_unbound_vehicles"]=unbound_vehicles();report["final_unpresented_vehicles"]=unpresented_vehicles();
    report["peak_unpresented_vehicles_during_cases"]=peak_case_unpresented;
    report["first_unpresented_tick_during_cases"]=first_case_unpresented_tick;
    if(peak_excess || peak_case_unpresented)++failures;
    report["passed"]=failures==0;save();
    std::cout<<(failures?"FAIL":"PASS")<<" netgame respawn failures="<<failures<<" vehicle_excess="<<peak_excess<<'\n';
    return failures?1:0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
