#include "halo2/debug_game.hpp"
#include "halo2/multiplayer.hpp"
#include <cmath>
#include <iostream>
using namespace halo2;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class F> void rejects(F action){try{action();}catch(const FormatError&){return;}throw std::runtime_error("invalid debug request was accepted");}
LevelContent arena(bool wall=false) {
    LevelContent c;c.bsp_count=1;
    c.collision={{{-100,-100,0},{100,-100,0},{100,100,0},0,0},{{-100,-100,0},{100,100,0},{-100,100,0},0,0}};
    if(wall) {
        c.collision.push_back({{1,-100,0},{1,100,0},{1,100,10},0,0});
        c.collision.push_back({{1,-100,0},{1,100,10},{1,-100,10},0,0});
    }
    c.player.datum=1;c.player.height=1;c.player.crouch_height=.6f;c.player.radius=.2f;
    c.player.camera_height=.8f;c.player.crouch_camera_height=.5f;c.player.crouch_velocity=2;c.player.jump_velocity=3;
    c.player.team=1;c.player.vitality={100,100,2,.1f,0};
    c.movement={2,2,2,10,1,1,1,5,2,1};
    WeaponDefinition w;w.datum=3;w.name="test weapon";w.instantaneous=true;w.capacity=20;w.reserve=100;w.initial_rounds=120;
    w.reload_rounds=20;w.reload_time=1;w.rounds_per_shot=w.minimum_rounds=1;w.rounds_per_second=30;
    w.burst=1;w.range=50;w.damage_min=25;w.projectiles_per_shot=1;c.weapons.emplace(3,w);c.starting_weapon=3;
    CharacterDefinition ch;ch.datum=2;ch.biped=c.player;ch.biped.datum=4;ch.biped.team=3;
    ch.vision_distance=30;ch.vision_angle=1.5f;ch.acknowledgement=.1f;c.characters.emplace(2,ch);
    SquadDefinition squad;squad.name="enemy";squad.count=1;squad.bsp=0;
    squad.starts.push_back({{2,0,0},3.14159265f,0,2,3,{}});c.squads.push_back(squad);
    return c;
}
void cheats() {
    const auto content=arena();
    Simulation baseline(content,0,{}),defaults(content,0,{});
    defaults.debug_settings({});baseline.equip(3);defaults.equip(3);
    for(unsigned i=0;i<120;++i) {
        Controls input{.4f,.1f,.2f,0,false,false,i%3==0,i==60};
        baseline.step(input);defaults.step(input);
        check(baseline.status()==defaults.status(),"disabled controls changed native simulation");
    }
    Simulation normal(content,0,{}),infinite(content,0,{});normal.equip(3);infinite.equip(3);
    DebugGameSettings settings;settings.players[0].unlimited_ammo=true;infinite.debug_settings(settings);
    for(unsigned i=0;i<60;++i){Controls input;input.fire=true;normal.step(input);infinite.step(input);}
    check(normal.shots()==20 && infinite.shots()==60,"unlimited ammo did not preserve firing cadence");
    check(infinite.player().weapon.loaded==20 && infinite.player().weapon.reserve==100,"infinite ammunition changed magazine source");
    infinite.debug_settings({});infinite.step({0,0,0,0,false,false,true});
    check(infinite.player().weapon.loaded==19,"disabled ammo cheat kept refilling");

    Simulation hostile(content,0,{}),protected_player(content,0,{}),ignored(content,0,{});
    hostile.place_squad("enemy");protected_player.place_squad("enemy");ignored.place_squad("enemy");
    settings={};settings.players[0].invulnerable=true;protected_player.debug_settings(settings);
    settings={};settings.players[0].enemies_ignore=true;ignored.debug_settings(settings);
    for(unsigned i=0;i<60;++i){hostile.step({});protected_player.step({});ignored.step({});}
    check(hostile.player().vitality.dead,"hostile fixture did not damage player");
    check(!protected_player.player().vitality.dead && protected_player.player().vitality.body==1 && protected_player.player().vitality.shield==1,"invulnerability leaked incoming damage");
    check(ignored.shots()==0 && ignored.actors()[0].target==-2,"enemy ignore retained a target");

    Simulation lethal(content,0,{}),ordinary(content,0,{});lethal.equip(3);ordinary.equip(3);
    lethal.place_squad("enemy");ordinary.place_squad("enemy");
    settings={};settings.players[0].maximum_damage=true;lethal.debug_settings(settings);
    Controls fire;fire.fire=true;lethal.step(fire);ordinary.step(fire);
    check(lethal.actors()[0].vitality.dead && !ordinary.actors()[0].vitality.dead,"maximum damage was not scoped to outgoing player damage");
}
void movement_and_snapshots() {
    const auto content=arena(true);
    Simulation normal(content,0,{}),flying(content,0,{});
    DebugGameSettings settings;settings.players[0].noclip=true;flying.debug_settings(settings);
    for(unsigned i=0;i<60;++i){normal.step({1,0,0,0});flying.step({1,0,0,0});}
    check(normal.player().body.position.x<1 && flying.player().body.position.x>3,"noclip did not bypass wall collision");
    const auto z=flying.player().body.position.z;flying.step({0,0,0,0,true});
    check(flying.player().body.position.z>z,"noclip jump did not ascend");
    flying.step({0,0,0,0,false,true});check(std::abs(flying.player().body.position.z-z)<.000001f,"noclip crouch did not descend");
    Spawn spawn;spawn.position={6,2,1};spawn.facing=.7f;
    flying.debug_teleport(0,spawn);check(flying.player().body.position.x==6 && flying.player().yaw==.7f,"authored teleport did not set full player transform");
    spawn.bsp=1;rejects([&]{flying.debug_teleport(0,spawn);});rejects([&]{flying.debug_heal(16);});

    Simulation sim(content,0,{});sim.request_checkpoint();sim.step({});
    const auto original_tick=sim.ticks();sim.debug_settings(settings);
    for(unsigned i=0;i<10;++i)sim.step({1,0,0,0});
    const auto saved_tick=sim.ticks();const auto saved_position=sim.player().body.position;
    sim.debug_save_snapshot("first");check(sim.debug_snapshot_names()==std::vector<std::string>{"first"},"snapshot not listed");
    for(unsigned i=0;i<10;++i)sim.step({1,0,0,0});
    sim.debug_load_snapshot("first");
    check(sim.ticks()==saved_tick && sim.player().body.position.x==saved_position.x && sim.debug_settings().players[0].noclip,"snapshot failed to restore native state or reset optional settings");
    sim.request_revert();sim.step({});check(sim.ticks()==original_tick,"session snapshot overwrote original campaign checkpoint");
    sim.debug_delete_snapshot("first");check(sim.debug_snapshot_names().empty(),"snapshot delete did not release state");
    rejects([&]{sim.debug_load_snapshot("first");});rejects([&]{sim.debug_save_snapshot("");});
    for(unsigned i=0;i<16;++i)sim.debug_save_snapshot(std::to_string(i));
    rejects([&]{sim.debug_save_snapshot("seventeenth");});
    sim.debug_save_snapshot("0"); // Overwriting does not allocate another slot.
}
void multiplayer() {
    auto content=arena();
    std::vector<Spawn> spawns(2);spawns[0].player_type=0;spawns[1]=spawns[0];spawns[1].position={4,0,0};
    auto rules=slayer_rules({"Slayer",0,{}});rules.weapon=3;
    MultiplayerMatch match(content,0,spawns,rules);const auto a=match.join("first"),b=match.join("second");
    DebugGameSettings settings;settings.players[a].unlimited_ammo=true;match.debug_settings(settings);
    std::array<Controls,multiplayer_capacity> controls{};controls[a].fire=controls[b].fire=true;
    for(unsigned i=0;i<10;++i)match.step(controls);
    check(match.unit(a).weapon.loaded==20 && match.unit(b).weapon.loaded<20,"multiplayer debug settings escaped selected participant");
    match.debug_request({DebugGameRequest::Teleport,a},&spawns[1]);
    check(match.unit(a).body.position.x==4,"multiplayer teleport did not affect selected source unit");
    rejects([&]{match.debug_request({DebugGameRequest::SaveSnapshot,a,0,"unsupported"});});
}
}
int main(){try{cheats();movement_and_snapshots();multiplayer();std::cout<<"Optional debug gameplay controls passed\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
