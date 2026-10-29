#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/source_execution_calls.hpp"
#include "halo2/full_scoreboard_assets.hpp"
#include "halo2/system_link_state.hpp"
#include "halo2/online_postgame.hpp"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
MultiplayerOptions options(const std::filesystem::path &map,unsigned players,bool enabled) {
  MultiplayerOptions result;result.map=map;result.local_players=players;result.source_object_world=true;
  result.variant={"Team switching",0,{{2,0},{10,0},{22,int(enabled)},{91,1}}};
  for(unsigned player=0;player<players;++player) {
    result.selected_teams.push_back(std::int8_t(player%2));
    result.selected_profiles.push_back(native_selected_player_configuration("Team "+std::to_string(player),
        std::array<std::uint8_t,9>{8,12,9,1,std::uint8_t(player%2),26,9,0,0},std::int8_t(player%2)));
  }
  return result;
}
void step(MultiplayerMatch &match,unsigned count) {
  std::array<Controls,multiplayer_capacity> controls{};
  for(unsigned i=0;i<count;++i)match.step(controls);
}
void local(const std::filesystem::path &map) {
  auto request=options(map,4,true);MultiplayerPreparedWorld world(request);auto &match=world.match;
  check(match.source_engine_owned(),"Team switching selected a bounded replacement engine");
  for(unsigned i=0;i<120 && match.state().players[0].source_unit==null_datum;++i)step(match,1);
  step(match,5);
  auto before=match.state();check(before.players[0].source_unit!=null_datum,"Team switch has no live original unit");
  check(before.team_changing && before.available_team_mask==0xff,"Source team policy was not published");
  check(!match.change_team(0,8).empty() && !match.change_team(0,-1).empty(),"Invalid team was accepted");
  check(match.state().players[0].source_membership==before.players[0].source_membership,
      "Rejected team request changed the actual player");
  check(match.change_team(0,1).empty(),"Allowed local team change failed");
  auto after=match.state();
  check(after.players[0].source_player==before.players[0].source_player &&
      after.players[0].source_membership==1 && after.players[0].source_configuration->at(0x7c)==1 &&
      after.players[0].source_unit==null_datum && !(after.players[0].source_flags&2u),
      "Original profile/team/detach owner was bypassed");
  step(match,1);after=match.state();
  const auto corpse=std::ranges::find(after.corpses,before.players[0].source_unit,&MultiplayerState::Corpse::object);
  if(corpse==after.corpses.end() || !corpse->colors) {
    const auto object=before.players[0].source_unit;
    std::cerr<<"Team corpse diagnostic tick="<<after.tick<<" object="<<std::hex<<object<<" previous_player="<<match.unit(0).source_previous_player
        <<" bridge_player="<<match.unit(0).source_player<<std::dec<<" corpses="<<after.corpses.size()<<" dead="<<match.unit(0).vitality.dead;
    if(world.source_objects->camera_target(object)) {
      const auto body=world.source_objects->object_body(object);const auto &memory=world.source_objects->memory();
      const auto layout=Bytes(memory.view(body+0x124u,4));
      std::cerr<<" extent="<<layout.u16(0)<<" offset="<<layout.u16(2)<<" source_flags="<<Bytes(memory.view(body+0x10au,2)).u16(0);
    }
    std::cerr<<'\n';throw FormatError("Team change lost its original detached body color bank");
  }
  const auto &palette=world.cache->load_full_scoreboard_assets().team_palette;
  check(length(corpse->colors->at(0)-std::bit_cast<Vec3>(palette.at(0)))<1e-6f,
      "The old corpse changed to its owner's new team color");
  step(match,89);after=match.state();
  check(after.players[0].source_unit!=null_datum && after.players[0].source_unit!=before.players[0].source_unit &&
      after.players[0].source_membership==1,"Original team-change respawn did not bind the new team");
  const auto respawned=after.players[0].source_unit;
  check(match.change_team(0,1).empty() && match.state().players[0].source_unit==respawned,
      "Repeated team request detached a newly respawned unit");
  for(std::int16_t team=0;team<8;++team) {
    check(match.change_team(0,team).empty(),"An original team color is unavailable to local switching");
    const auto state=match.state();
    check(state.players[0].source_membership==team && state.players[0].source_configuration->at(0x7c)==team,
        "Accepted team color did not reach the source profile");
    check(state.players[1].source_membership==1 && state.players[2].source_membership==0,
        "One local player changed another couch player's team");
  }
  std::cout<<"PASS native local switching: all eight teams, invalid requests, original detach/respawn, identity and couch isolation\n";
}
void disabled(const std::filesystem::path &map) {
  auto request=options(map,2,false);MultiplayerPreparedWorld world(request);
  check(!world.match.state().team_changing && !world.match.change_team(0,1).empty() &&
      world.match.state().players[0].source_membership==0,"Team Changing Off was ignored");
  std::cout<<"PASS original Team Changing Off\n";
}
void objective_teams(const std::filesystem::path &map) {
  for(unsigned engine:{4u,5u}) {
    auto request=options(map,2,true);request.variant.engine=engine;
    MultiplayerPreparedWorld world(request);auto &match=world.match;
    const auto state=match.state();const auto mask=match.source_available_team_mask();
    check(state.team_changing && state.available_team_mask==mask && mask==3,
        "Objective team selector differs from the original engine/map availability");
    check(!match.change_team(0,2).empty() && match.change_team(0,1).empty(),
        "Objective team switching ignored the source two-team limit");
  }
  std::cout<<"PASS original CTF/Assault team availability and switching\n";
}
void network(const std::filesystem::path &map) {
  auto request=options(map,2,true);request.variant.values.set(10,1);
  MultiplayerPreparedWorld world(request);auto &match=world.match;
  LanHost host(match,0,0x7465616d73776974ull,request.variant);host.enable_system_link_return("coagulation");
  const std::array<std::int8_t,2> teams{0,1};
  const std::array<SourcePlayerConfiguration,2> profiles{
      native_selected_player_configuration("Remote Spartan",std::array<std::uint8_t,9>{3,4,5,6,0,5,3,0,0},0),
      native_selected_player_configuration("Remote Elite",std::array<std::uint8_t,9>{8,12,9,1,1,26,9,0,0},1)};
  LanClient client(host.join_address(),0x7465616d73776974ull,2,teams,profiles);
  std::array<Controls,multiplayer_capacity> controls{};std::array<Controls,2> remote{};
  auto pump=[&]{client.send(remote);host.receive(controls);host.broadcast();client.receive();};
  auto until=std::chrono::steady_clock::now()+std::chrono::seconds(8);
  while(!client.ready() && std::chrono::steady_clock::now()<until){pump();std::this_thread::sleep_for(std::chrono::milliseconds(2));}
  check(client.ready(),"Team switching UDP peer did not join");
  const auto slot=client.slots()[1],other=client.slots()[0];
  check(!client.request_team_change(2,0).empty(),"Client accepted a view it does not own");
  check(client.request_team_change(1,7).empty(),"Remote team request could not be queued");
  bool acknowledged{};until=std::chrono::steady_clock::now()+std::chrono::seconds(5);
  while(!acknowledged && std::chrono::steady_clock::now()<until) {
    pump();for(const auto &result:client.take_team_change_results()) {
      check(result.view==1 && result.team==7 && result.error.empty(),"Host rejected or misrouted the remote team result");
      acknowledged=true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  check(acknowledged && match.player(slot).source_membership==7 && match.player(other).source_membership==0 &&
      match.player(world.local_slots[0]).source_membership==0,"Authenticated couch-view ownership was lost");
  for(unsigned i=0;i<16;++i)pump();
  check(client.state().team_changing && client.state().available_team_mask==0xff &&
      client.state().players[slot].source_membership==7 &&
      client.state().players[slot].source_configuration->at(0x7c)==7,
      "UDP snapshot did not deliver accepted policy, membership and armor profile");
  std::cout<<"PASS real native UDP team request, host acceptance, couch ownership and replicated colors\n";
  for(unsigned ticks=0;ticks<2400 && !match.state().game_over_complete();++ticks) {
    match.step(controls);
    if(ticks%15==0)pump();
  }
  check(match.state().game_over_complete(),"Original timed team match did not complete its own game-over owner");
  const auto report=online_postgame_publish(match,request.variant,host.system_link_state().roster);
  online_postgame_validate(match.state(),request.variant,host.system_link_state().roster,report.source);
  host.publish_system_link_results();
  until=std::chrono::steady_clock::now()+std::chrono::seconds(8);
  while(host.system_link_state().phase!=SystemLinkPhase::postgame && std::chrono::steady_clock::now()<until) {
    pump();
    if(client.system_link_state() && client.system_link_state()->phase==SystemLinkPhase::reviewing)
      client.confirm_system_link_results();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  check(host.system_link_state().phase==SystemLinkPhase::postgame,"Original final team report did not receive peer confirmation");
  host.retire_system_link_world();host.continue_system_link_lobby();
  const auto lobby_pump=[&]{client.service_system_link_lobby();host.service_system_link_lobby();host.broadcast();client.receive();};
  until=std::chrono::steady_clock::now()+std::chrono::seconds(8);
  while((!client.system_link_state() || client.system_link_state()->phase!=SystemLinkPhase::lobby) && std::chrono::steady_clock::now()<until) {
    lobby_pump();std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  check(client.system_link_state() && client.system_link_state()->phase==SystemLinkPhase::lobby,
      "Accepted team change could not return to the retained System Link lobby");
  check(host.change_system_link_team(world.local_slots[0],5).empty(),"Local lobby team change failed");
  check(client.request_team_change(1,6).empty(),"Remote lobby team change failed to queue");
  acknowledged=false;until=std::chrono::steady_clock::now()+std::chrono::seconds(5);
  while(!acknowledged && std::chrono::steady_clock::now()<until) {
    lobby_pump();for(const auto &result:client.take_team_change_results()) {
      check(result.view==1 && result.team==6 && result.error.empty(),"Retained lobby misrouted the remote team change");acknowledged=true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  check(acknowledged && host.system_link_state().profiles[slot]->at(0x7c)==6 &&
      host.system_link_state().profiles[world.local_slots[0]]->at(0x7c)==5,
      "Lobby team changes did not reach the next world's authoritative profiles");
  host.system_link_countdown(3);check(!host.change_system_link_team(world.local_slots[0],4).empty(),"Lobby team change bypassed the countdown owner");
  host.system_link_countdown(-1);
  auto next=request;next.variant.values.set(22,0);
  for(unsigned view=0;view<2;++view) {
    next.selected_profiles[view]=*host.system_link_state().profiles[world.local_slots[view]];
    next.selected_teams[view]=std::bit_cast<std::int8_t>(next.selected_profiles[view][0x7c]);
  }
  MultiplayerPreparedWorld restarted(next);host.resume_system_link_world(restarted.match,0x7465616d73776974ull,next.variant,"coagulation");
  until=std::chrono::steady_clock::now()+std::chrono::seconds(5);
  while(client.system_link_state()->phase!=SystemLinkPhase::starting && std::chrono::steady_clock::now()<until)lobby_pump();
  check(client.system_link_state()->phase==SystemLinkPhase::starting,"System Link did not publish its next generation");
  client.resume_system_link_world();
  until=std::chrono::steady_clock::now()+std::chrono::seconds(5);
  while(!client.ready() && std::chrono::steady_clock::now()<until) {
    client.send(remote);host.receive(controls);host.broadcast();client.receive();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  check(client.ready() && restarted.match.player(slot).source_membership==6 &&
      restarted.match.player(world.local_slots[0]).source_membership==5 && !client.state().team_changing &&
      !client.request_team_change(1,7).empty(),"The next world lost accepted teams or ignored Team Changing Off");
  std::cout<<"PASS original timed game-over/report with changed teams, retained lobby ownership/choices, countdown and next-world admission\n";

}
}
int main(int argc,char **argv)try {
  check(argc==2,"Supply prepared MCC map");
  if(!std::filesystem::is_regular_file(argv[1]))return 77;
  check(std::getenv("HALO2_TEST_SCRATCH"),"Team switching tests require owned scratch");
  local(argv[1]);disabled(argv[1]);objective_teams(argv[1]);network(argv[1]);
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
