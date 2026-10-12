#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
void check(bool ok,const std::string& message) {if(!ok)throw std::runtime_error(message);}
std::uint32_t bits(float value){return std::bit_cast<std::uint32_t>(value);}
LevelContent arena(float damage=40.f) {
  LevelContent c;c.player.datum=1;c.player.height=1;c.player.crouch_height=.6f;
  c.player.radius=.2f;c.player.camera_height=.8f;c.player.crouch_camera_height=.5f;
  c.player.crouch_velocity=2;c.player.jump_velocity=3;c.player.vitality.health=100;
  c.collision={{{-100,-100,0},{100,-100,0},{100,100,0},0,0},{{-100,-100,0},{100,100,0},{-100,100,0},0,0}};
  c.movement={2,2,2,10,1,1,1,5,2,1};
  WeaponDefinition w;w.datum=3;w.instantaneous=true;w.capacity=20;w.initial_rounds=20;
  w.rounds_per_shot=1;w.minimum_rounds=1;w.rounds_per_second=30;w.burst=1;
  w.range=50;w.damage_min=damage;w.projectiles_per_shot=1;
  w.firing_camouflage_amount=.2f;w.damage_camouflage_amount=.1f;
  c.weapons.emplace(3,w);return c;
}
SlayerRules rules(){auto r=slayer_rules({"Slayer",0,{}});r.weapon=3;r.time_ticks=0;return r;}
std::array<Spawn,2> starts(){std::array<Spawn,2> s{};s[1].position={4,0,0};s[1].facing=3.14159265f;return s;}
void observe(const MultiplayerMatch& match,unsigned id,const nlohmann::json& expected,const std::string& label){
  const auto& u=match.unit(id);const auto& p=match.player(id);const auto& f=u.source_camouflage;
  check(bits(f.primary)==expected.at("camo_bits") && bits(f.secondary)==expected.at("secondary_camo_bits") &&
      bits(f.fade_speed)==expected.at("fade_speed_bits") && (u.source_action_word&24u)==expected.at("unit_flags") &&
      p.source_powerup_timers[0]==expected.at("powerup_ticks")[0] &&
      p.source_powerup_timers[1]==expected.at("powerup_ticks")[1],label);
}
void original_timelines(const nlohmann::json& fixture){
  auto c=arena();auto r=rules();const auto spawn=starts();
  for(std::size_t index=0;index<fixture.at("inputs").size();++index){
    MultiplayerMatch match(c,0,spawn,r);const auto id=match.join("Timeline");
    for(const auto& version:fixture.at("versions").items())
      observe(match,id,version.value().at("results")[index].at("cold"),version.key()+" native cold state");
    const auto& input=fixture.at("inputs")[index];match.grant_powerup(id,input.at("index"),input.at("ticks"));
    for(const auto& version:fixture.at("versions").items())
      observe(match,id,version.value().at("results")[index].at("after_grant"),version.key()+" native grant");
    std::array<Controls,multiplayer_capacity> controls{};
    for(unsigned step=0;step<input.at("steps");++step){
      match.step(controls);
      for(const auto& version:fixture.at("versions").items())
        observe(match,id,version.value().at("results")[index].at("steps")[step].at("after_timer"),
            version.key()+" native completed frame "+std::to_string(step+1));
    }
  }
}
void combat_and_lifetime(){
  auto c=arena();auto r=rules();const auto spawn=starts();MultiplayerMatch match(c,0,spawn,r);
  const auto a=match.join("Shooter"),b=match.join("Target");match.grant_powerup(a,0,120);match.grant_powerup(b,0,120);
  std::array<Controls,multiplayer_capacity> controls{};controls[a].yaw=0;controls[b].yaw=3.14159265f;
  for(unsigned i=0;i<30;++i)match.step(controls);
  const auto initial=match.unit(b).vitality.body;controls[a].fire=true;match.step(controls);controls[a].fire=false;
  check(match.unit(b).vitality.body<initial,"Native shot did not reach positive damage report");
  check(match.unit(a).weapon.loaded==19,"Native firing disruption was not an accepted shot");
  check(match.unit(a).source_camouflage.primary<match.unit(b).source_camouflage.primary &&
      match.unit(b).source_camouflage.primary<.2f,"Firing and selected damage attributes did not disrupt their own units");
  check(bits(match.unit(a).source_camouflage.fade_speed)==bits(.25f),"Zero authored limit did not select original .25 limit");
  const auto timer=match.player(b).source_powerup_timers;match.leave(b);
  check(match.player(b).source_powerup_timers==timer,"Actual leave destroyed retained source powerup timers");
  check(match.join("Rejoined")==b && match.player(b).source_powerup_timers==std::array<std::int16_t,2>{} &&
      bits(match.unit(b).source_camouflage.primary)==0 && bits(match.unit(b).source_camouflage.fade_speed)==0,
      "Same-slot allocation/rejoin did not clear original cold state");
  auto lethal=arena(4000);r.respawn_ticks=1;MultiplayerMatch death(lethal,0,spawn,r);
  const auto victim=death.join("Victim"),killer=death.join("Killer");death.grant_powerup(victim,0,120);death.grant_powerup(victim,1,120);
  controls={};controls[killer].yaw=3.14159265f;controls[killer].fire=true;death.step(controls);controls[killer].fire=false;
  check(death.unit(victim).vitality.dead,"Native powerup respawn fixture did not die");
  for(unsigned i=0;i<40 && death.unit(victim).vitality.dead;++i)death.step(controls);
  check(!death.unit(victim).vitality.dead && death.player(victim).source_powerup_timers==std::array<std::int16_t,2>{} &&
      bits(death.unit(victim).source_camouflage.primary)==0,"Successful native respawn retained powerup timer/fade state");
}
void authored_attributes(const char* map,const char* maps){
  Cache cache(map,maps);const auto content=cache.load_content(0,true);unsigned weapons=0,damages=0;
  for(const auto& [datum,w]:content.weapons){
    const auto tag=cache.load_source_tag_metadata(datum);const auto p=tag.source_address();
    const auto field=[&](unsigned offset){return Bytes(tag.bytes_at(p+offset,4)).u32(0);};
    check(bits(w.firing_camouflage_amount)==field(0x27c) && bits(w.firing_camouflage_limit)==field(0x280),
        "Imported native weapon camouflage attributes differ from actual Lockout tag bytes");++weapons;
    if(w.damage!=null_datum){const auto damage=cache.load_source_tag_metadata(w.damage);
      check(bits(w.damage_camouflage_amount)==Bytes(damage.bytes_at(damage.source_address()+0x30,4)).u32(0),
          "Native selected damage camouflage amount differs from actual jpt!+30");++damages;}
  }
  check(weapons>0 && damages>0,"No actual tag-backed native camouflage attributes checked");
  std::cout<<weapons<<" actual weapon and "<<damages<<" selected damage attributes; ";
}
void network_fades(){
  auto c=arena();const auto spawn=starts();MultiplayerMatch match(c,0,spawn,rules());const auto local=match.join("Host");
  LanHost host(match,0,0xcafefade);LanClient client("127.0.0.1:"+std::to_string(host.port()),0xcafefade,1);
  std::array<Controls,multiplayer_capacity> controls{};std::array<Controls,1> outgoing{};
  for(unsigned i=0;i<20 && !client.ready();++i){client.send(outgoing);host.receive(controls);host.broadcast();client.receive();}
  check(client.ready(),"Camouflage UDP fixture did not join");const auto remote=client.slots()[0];
  match.grant_powerup(remote,0,15);
  const auto compare=[&]{const auto state=match.state();host.broadcast();client.receive();
    check(client.state().tick==state.tick,"Camouflage snapshot was not received atomically");
    for(unsigned id:{local,remote}){const auto& expected=match.unit(id).source_camouflage;
      const auto& actual=client.state().players[id].source_camouflage;
      check(bits(expected.primary)==bits(actual.primary) && bits(expected.secondary)==bits(actual.secondary) &&
          bits(expected.fade_speed)==bits(actual.fade_speed) &&
          match.unit(id).source_action_word==client.state().players[id].source_action_word,
          "Actual UDP snapshot changed authoritative camouflage bits");}
  };
  compare();for(unsigned i=0;i<30;++i){match.step(controls);compare();}
  // Accepted firing on a cold, inactive unit produces negative primary until
  // the next ordinary unit fade. The wire must not apply a render-time clamp.
  controls[local].fire=true;match.step(controls);controls[local].fire=false;
  check(match.unit(local).source_camouflage.primary<0,"Native inactive shot did not produce the original negative primary");
  compare();match.step(controls);compare();
}
}
int main(int argc,char** argv){try{
  if(argc!=4)throw std::runtime_error("Supply original timeline fixture, actual MAP and SHARED_MAPS");
  std::ifstream file(argv[1]);nlohmann::json fixture;file>>fixture;
  original_timelines(fixture);combat_and_lifetime();authored_attributes(argv[2],argv[3]);network_fades();
  std::cout<<"native camouflage timelines, accepted firing/damage and lifetime checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
