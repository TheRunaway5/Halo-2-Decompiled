#include "halo2/multiplayer_network.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/fp_replication.hpp"
#include <algorithm>
#include <iostream>

using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
}
int main(int argc,char **argv)try {
  check(argc==2 || argc==3,"Supply the repository root and optional gameplay XBE");
  const std::filesystem::path root=argv[1];
  const auto gameplay_xbe=argc==3?std::filesystem::path(argv[2]):
      root/"assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03";
  if(!std::filesystem::exists(root/"assets/disc/maps/lockout.map") || !std::filesystem::exists(gameplay_xbe))return 77;
  Cache cache(root/"assets/disc/maps/lockout.map",{},OriginalData::load_xbe(gameplay_xbe));
  auto content=cache.load_content(0,true);
  auto rules=slayer_rules({"Mixed species",0,{{0x59,5}}});
  rules.weapon=cache.slayer_starting_weapon(5);rules.score_limit=rules.time_ticks=0;
  check(content.player_representations.size()==4,"Original representation table was not retained");
  const std::array<FirstPersonBindingDefinition,2> definitions{
      cache.load_first_person_binding(rules.weapon,2),cache.load_first_person_binding(rules.weapon,3)};
  check(definitions[0].arms_model!=definitions[1].arms_model && definitions[0].graph!=definitions[1].graph,
      "Original species-specific arms/weapon graphs are absent");
  for(const auto &definition:definitions) {
    bool absent=false;
    try{cache.load_unit_weapon_action(definition.graph,0x030000d9,0x030000d9,0x030000d9,0x1000006c);}
    catch(const FormatError&){absent=true;}
    check(absent,"Authored SMG FP throw_overheated absence changed");
    FirstPersonViewBinding view;
    observe_first_person_binding(view,{1,2,definition.representation,{3,null_datum}},
        [&](auto,auto){return &definition;});
    const auto before=first_person_channel_state(view.hands[0]);
    first_person_activity(view.hands[0],definition,0x1000006c);
    check(same_first_person_channel(before,first_person_channel_state(view.hands[0])),
        "Absent optional FP throw changed the existing channel");
  }
  for(bool teams:{false,true}) {
    rules.team_mode=teams;
    MultiplayerMatch match(content,0,cache.spawns(),rules),server(content,0,cache.spawns(),rules);
    const auto team=std::int8_t(teams?1:-1);
    std::array<SourcePlayerConfiguration,4> profiles;
    for(unsigned i=0;i<4;++i) {
      std::array<std::uint8_t,9> appearance{10,0,0,10,std::uint8_t(i),0,0,0,0};
      profiles[i]=native_selected_player_configuration("Species "+std::to_string(i),appearance,team);
      check(match.join("Species "+std::to_string(i),team,profiles[i])==i,"Mixed species local join failed");
      const unsigned representation=i%2+2;
      const auto &authored=content.player_representations[representation];
      const auto &unit=match.unit(i);
      check(unit.definition.datum==authored.biped.datum && unit.body.radius==authored.biped.radius &&
          unit.definition.camera_height==authored.biped.camera_height,
          "Selected profile did not spawn with its authored biped/physics/camera");
    }
    const std::array<unsigned,4> ids{0,1,2,3};match.configure_local_views(ids,definitions);
    for(const auto &definition:definitions)server.configure_replicated_first_person(definition);
    LanHost host(server,0,0x14f040);
    const std::array<std::int8_t,4> selected_teams{team,team,team,team};
    LanClient client("127.0.0.1:"+std::to_string(host.port()),0x14f040,4,
        teams?std::span<const std::int8_t>(selected_teams):std::span<const std::int8_t>{},profiles);
    std::array<Controls,16> commands{};std::array<Controls,4> local{};
    for(unsigned n=0;n<64 && !client.ready();++n) {
      client.send(local);host.receive(commands);host.broadcast();client.receive();
    }
    check(client.ready(),"Mixed species UDP hello was rejected");
    for(unsigned i=4;i<multiplayer_capacity;++i) {
      const auto name="Remote "+std::to_string(i);
      check(match.join(name,team,profiles[i%4])==i && server.join(name,team,profiles[i%4])==i,
          "Mixed species full roster could not join");
    }
    std::array<FirstPersonViewBinding,4> views{},origins{};
    for(unsigned tick=0;tick<90;++tick) {
      for(unsigned i=0;i<4;++i) {
        local[i].yaw=match.unit(i).yaw;local[i].pitch=1.4f;
        local[i].fire=tick>=6 && tick<12;local[i].reload=tick==20;
      }
      client.send(local);host.receive(commands);server.step(commands);match.step(commands);
      host.broadcast();client.receive();
      check(client.state().tick==server.state().tick,"Mixed species UDP snapshot was lost");
      for(unsigned i=0;i<multiplayer_capacity;++i)
        check(client.state().players[i].source_configuration==match.state().players[i].source_configuration &&
            server.unit(i).definition.datum==content.player_representations[i%2+2].biped.datum,
            "Full mixed species roster lost its selected profiles or bipeds");
      for(unsigned i=0;i<4;++i) {
        const auto &p=client.state().players[i];const auto &definition=definitions[i%2];
        check(p.source_configuration==match.state().players[i].source_configuration &&
            server.unit(i).definition.datum==match.unit(i).definition.datum,
            "Species selection changed across native UDP delivery");
        const auto &binding=match.first_person_view(i);
        check(binding.representation==definition.representation && binding.hands[0].graph==definition.graph &&
            binding.hands[0].weapon==match.unit(i).weapon.source_datum && (binding.hands[0].flags&6)==6,
            "Mixed local first-person view retained another species' arms/weapon graph");
        observe_multiplayer_first_person(views[i],p,definition);
        check(apply_replicated_first_person_clock(origins[i],views[i],p.first_person_clock,definition),
            "Selected species did not receive its replicated first-person clock");
        check(same_first_person_channel(first_person_channel_state(views[i].hands[0]),
            first_person_channel_state(binding.hands[0])),"Species-specific UDP and local clocks differ");
        check(!server.unit(i).weapon.smg.hand_timer_registered && match.unit(i).weapon.smg.hand_timer_registered,
            "Species support changed dedicated/local hand feedback ownership");
      }
    }
    // Existing native lifecycle seam: a depleted body enters normal ownership
    // cleanup and source respawn. This is not an original damage oracle.
    const auto departed=match.unit(1).source_datum;
    const_cast<Unit&>(match.unit(1)).vitality.dead=true;
    commands={};
    for(unsigned n=0;n<400 && (match.unit(1).source_datum==departed || match.unit(1).vitality.dead);++n)
      match.step(commands);
    check(!match.unit(1).vitality.dead && match.unit(1).source_datum!=departed &&
        match.unit(1).definition.datum==content.player_representations[3].biped.datum &&
        match.first_person_view(1).representation==3 && match.first_person_view(1).hands[0].graph==definitions[1].graph,
        "Elite respawn lost its biped or first-person representation");
    match.leave(1);
    check(match.join("Spartan replacement",team,profiles[0])==1,"Species slot reuse failed");
    match.step(commands);
    check(
        match.unit(1).definition.datum==content.player_representations[2].biped.datum &&
        match.first_person_view(1).representation==2,
        "Reused Elite slot retained Elite body/arms for a Spartan");
  }
  std::cout<<"PASS original0/1/2/3 selections: mixed FFA/Team local and actual UDP bipeds, physics, cameras, arms, graphs, fire/reload clocks, Elite respawn and species slot reuse\n";
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
