#include "halo2/simulation.hpp"
#include "halo2/residency.hpp"
#include "halo2/original_data.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string_view>

// The application's --place-squad/--weapon diagnostic path, headless. This
// exercises an authored encounter; it does not stand in for mission startup.
int main(int argc,char **argv) try {
    using namespace halo2;
    if(argc!=7)throw FormatError("Supply MAP SHARED XBE SQUAD|--list TICKS OUTPUT_DIRECTORY");
    Cache cache(argv[1],argv[2],OriginalData::load_xbe(argv[3]));
    auto content=cache.load_content(0);
    const auto spawn=std::find_if(cache.spawns().begin(),cache.spawns().end(),[](const auto&s){return s.bsp==0;});
    if(spawn==cache.spawns().end())throw FormatError("No authored BSP zero player start");
    if(std::string_view(argv[4])=="--list") {
        std::vector<std::pair<float,const SquadDefinition*>> squads;
        for(const auto&s:content.squads)if(s.count && s.team!=1 && s.team!=2 && (s.bsp==0 || s.bsp==-1)) {
            float distance=INFINITY;
            for(const auto&p:s.starts)distance=std::min(distance,length(p.position-spawn->position));
            squads.emplace_back(distance,&s);
        }
        std::sort(squads.begin(),squads.end(),[](const auto&a,const auto&b){return a.first<b.first;});
        for(const auto &[distance,squad]:squads)
            std::cout<<distance<<" count="<<squad->count<<" team="<<squad->team<<" squad="<<squad->name<<'\n';
        return 0;
    }
    const auto ticks=std::stoul(argv[5]);
    if(!ticks || ticks>18000)throw FormatError("Invalid encounter step budget");
    const std::filesystem::path output=argv[6];std::filesystem::create_directories(output);
    NativeResidency residency(cache);
    Simulation simulation(content,BspContent{std::move(content.collision),content.navigation},0,*spawn,
        [&](auto graph,auto name){return cache.load_named_animation(graph,name);},
        [&](auto bsp){return cache.load_bsp_content(bsp);},
        [&](const Simulation::Prediction&p){
            if(p.kind==Simulation::Prediction::Barrier)residency.barrier();
            else if(p.kind==Simulation::Prediction::Sound)residency.predict_sound(p.datum);
            else if(p.kind==Simulation::Prediction::Model)residency.model(p.datum,p.section);
            else if(p.kind==Simulation::Prediction::Object)residency.object(p.datum);
            else residency.structure(p.datum,p.section,p.instances);
        });
    const auto weapon=std::find_if(content.weapons.begin(),content.weapons.end(),[](const auto&w){
        return w.second.name.ends_with("\\smg\\smg");});
    if(weapon==content.weapons.end())throw FormatError("Authored SMG is unavailable");
    if(weapon->second.damage_amount)std::cout<<"Authored SMG damage: lower="<<weapon->second.damage_amount->lower
        <<" upper_minimum="<<weapon->second.damage_amount->upper_minimum
        <<" upper_maximum="<<weapon->second.damage_amount->upper_maximum<<std::endl;
    simulation.equip(weapon->first);
    const auto existing=simulation.actors().size();
    const auto placed=simulation.place_squad(argv[4]);
    if(!placed)throw FormatError("Authored squad was not placed");
    NavigationWorld navigation(content.navigation);std::vector<Vec3> route;
    const auto initial=simulation.player().body.position;
    std::ofstream trace(output/"ticks.tsv");
    if(!trace)throw FormatError("Cannot write encounter trace");
    trace<<"tick\tx\ty\tz\thealth\tshield\tshots\thits\tdead_actors\ttarget\ttarget_x\ttarget_y\ttarget_z\ttarget_body\ttarget_shield\tvisible\twall_distance\troute_points\tforward\tright\tyaw\tpitch\tfire\tammo\timpact_serial\timpact_terrain\timpact_x\timpact_y\timpact_z\n";
    unsigned moved{},dead{},steps{},player_fired{};
    for(unsigned step=0;step<ticks;++step) {
        Controls input;const auto &player=simulation.player();
        input.yaw=player.yaw;input.pitch=simulation.player_pitch();
        const auto origin=player.body.position+Vec3{0,0,player.definition.camera_height};
        const Unit *target{};
        for(const auto &a:simulation.actors())if(a.exists && a.state!=Unit::Dead && !simulation.allied(player.team,a.team) &&
            (!target || length(a.body.position-origin)<length(target->body.position-origin)))target=&a;
        bool visible{};float wall_distance=-1;Vec3 target_position{};
        const auto target_handle=target?target->source_datum:null_datum;
        if(target) {
            target_position=target->body.position;
            const auto delta=target->body.position+Vec3{0,0,target->body.height*.55f}-origin;
            const auto distance=length(delta);const auto wall=simulation.collision().raycast(origin,delta,distance);
            visible=!wall || wall->distance>=distance-.15f;
            if(wall)wall_distance=wall->distance;
            input.yaw=std::atan2(delta.y,delta.x);input.pitch=std::clamp(std::atan2(delta.z,std::hypot(delta.x,delta.y)),-1.5f,1.5f);
            input.fire=visible;
            if(step%30==0)route=navigation.route(player.body.position,target->body.position);
            while(!route.empty() && length(route.front()-player.body.position)<.5f)route.erase(route.begin());
            const auto toward=(visible || route.empty()?target->body.position:route.front())-player.body.position;
            if(!visible || distance>4.f) {
                const auto walk=normalized(Vec3{toward.x,toward.y,0});
                input.forward=dot(walk,Vec3{std::cos(input.yaw),std::sin(input.yaw),0});
                input.right=dot(walk,Vec3{std::sin(input.yaw),-std::cos(input.yaw),0});
            }
            input.jump=step%120==0;
        } else {input.forward=.8f;input.fire=true;}
        input.reload=player.weapon.loaded==0 && player.weapon.reserve>0;
        const auto ammunition=player.weapon.loaded;
        simulation.step(input);++steps;simulation.advance_audio(tick_seconds);simulation.take_sounds();
        const auto &p=simulation.player();moved+=length(p.body.position-initial)>1.f;
        if(input.fire && p.weapon.loaded<ammunition)player_fired+=unsigned(ammunition-p.weapon.loaded);
        dead=unsigned(std::count_if(simulation.actors().begin()+existing,simulation.actors().begin()+existing+placed,
            [](const auto&a){return a.state==Unit::Dead;}));
        trace<<simulation.ticks()<<'\t'<<p.body.position.x<<'\t'<<p.body.position.y<<'\t'<<p.body.position.z<<'\t'
             <<p.vitality.body<<'\t'<<p.vitality.shield<<'\t'<<simulation.shots()<<'\t'<<simulation.hits()<<'\t'<<dead<<'\t'
             <<target_handle<<'\t'<<target_position.x<<'\t'<<target_position.y<<'\t'<<target_position.z<<'\t'
             <<(target?target->vitality.body:0)<<'\t'<<(target?target->vitality.shield:0)<<'\t'
             <<visible<<'\t'<<wall_distance<<'\t'<<route.size()<<'\t'<<input.forward<<'\t'<<input.right<<'\t'
             <<input.yaw<<'\t'<<input.pitch<<'\t'<<input.fire<<'\t'<<p.weapon.loaded;
        if(simulation.bullet_impacts().empty())trace<<"\t0\t0\t0\t0\t0\n";
        else {const auto &hit=simulation.bullet_impacts().back();trace<<'\t'<<hit.serial<<'\t'<<hit.terrain<<'\t'
            <<hit.position.x<<'\t'<<hit.position.y<<'\t'<<hit.position.z<<'\n';}
        if(!simulation.scripts().diagnostics().empty())throw FormatError("Encounter script fault");
        if(dead==placed && player_fired>=12 && simulation.hits() && moved>100)break;
    }
    std::cout<<"Encounter observation: "<<simulation.status()<<" moved_ticks="<<moved<<" dead_actors="<<dead<<'\n';
    for(const auto &actor:simulation.actors()) {
        const auto *definition=cache.find_tag({actor.definition.datum});
        std::cout<<"Actor observation: definition="<<(definition?definition->name:"missing")
                 <<" handle="<<actor.source_datum<<" x="<<actor.body.position.x
                 <<" y="<<actor.body.position.y<<" z="<<actor.body.position.z
                 <<" height="<<actor.body.height<<" radius="<<actor.body.radius
                 <<" body="<<actor.vitality.body<<" shield="<<actor.vitality.shield
                 <<" dead="<<actor.vitality.dead<<" state="<<actor.state
                 <<" maximum_body="<<actor.definition.vitality.health
                 <<" maximum_shield="<<actor.definition.vitality.shield<<'\n';
    }
    if(steps<=100 || player_fired<12 || !simulation.hits() || dead!=placed || moved<=100)
        throw FormatError("Authored encounter did not complete movement, firing, hits and fatal damage checks");
    if(!trace)throw FormatError("Encounter trace write failed");
    std::cout<<"PASS authored campaign encounter: steps="<<steps<<" shots="<<simulation.shots()
             <<" hits="<<simulation.hits()<<" player_fired="<<player_fired
             <<" dead_actors="<<dead<<" moved_ticks="<<moved<<'\n';
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
