#include "halo2/multiplayer_network.hpp"
#include "halo2/multiplayer_scoreboard.hpp"
#include "halo2/original_data.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace halo2;
namespace {
void check(bool value,const char* message) {if(!value)throw FormatError(message);}

void original_gates(const OriginalData& original,const std::filesystem::path& fixture) {
  std::ifstream file(fixture);check(bool(file),"Missing original death scoreboard fixtures");
  unsigned count{};
  for(std::string line;std::getline(file,line);) {
    std::istringstream row(line);
    FullScoreboardClock clock{30,tick_seconds,0};FullScoreboardHeldInput input;
    std::int32_t limit;std::int16_t lives;unsigned flags,before,after;
    row>>clock.tick>>input.death_countdown>>flags>>limit>>lives>>input.current_unit
        >>input.held>>before>>after;
    check(bool(row),"Invalid original death scoreboard row");
    input.local_view=input.input_user=0;input.processed_record_present=true;
    input.engine_present=true;input.lifecycle=1;input.engine_status=1;input.options_state=4;
    input.player_flags=std::uint16_t(flags);
    input.exhausted=source_player_lives_exhausted(limit,input.current_unit,lives);
    FullScoreboardHeldState state;state.counters[0]=std::uint8_t(before);
    check(full_scoreboard_update_held(state,input,clock,FullScoreboardRounding::NearestEven,
        *original.full_scoreboard_constants) && state.counters[0]==after,
        "Automatic death scoreboard differs from unchanged original instructions");
    ++count;
  }
  check(count==240,"Original death scoreboard coverage changed");
}

void place(MultiplayerMatch& match,unsigned id,Vec3 root,float yaw) {
  Spawn spawn;spawn.position=root;spawn.facing=yaw;
  DebugGameRequest request;request.kind=DebugGameRequest::Teleport;request.player=id;
  match.debug_request(request,&spawn);
}

void live_cycle(const Cache& cache,const LevelContent& original,const UiVariant& variant,
    std::span<const Spawn> spawns,bool network) {
  auto content=original;
  // Existing native debug arena boundary: original weapon/damage/death and
  // respawn owners run; unrelated Lockout falls cannot kill the shooter.
  content.collision.clear();content.kill_triggers.clear();
  auto rules=slayer_rules(variant);rules.weapon=cache.slayer_starting_weapon(5);
  rules.score_limit=rules.time_ticks=0;rules.respawn_ticks=30;
  MultiplayerMatch match(content,0,spawns,rules);
  match.join("Shooter");match.join("Victim");
  DebugGameSettings settings;settings.players[0].noclip=settings.players[1].noclip=true;
  match.debug_settings(settings);
  std::array<Controls,multiplayer_capacity> inputs{};
  // Source automatic display excludes ticks0..90, independently of death.
  while(match.simulation().ticks()<100)match.step(inputs);
  place(match,0,{0,0,0},0);place(match,1,{1,0,0},0);
  const auto old_body=match.unit(1).source_datum;
  const auto old_owner=match.player(1).source_datum;
  inputs[0].fire=true;
  for(unsigned n=0;n<180 && !match.unit(1).vitality.dead;++n)match.step(inputs);
  check(match.unit(1).vitality.dead && match.simulation().hits()>0 &&
      match.player(0).kills==1 && match.player(1).deaths==1,
      "Actual stock SMG did not enter ordinary fatal cleanup");
  check(match.player(1).source_previous_unit==old_body &&
      match.player(1).source_current_object==null_datum,
      "Fatal cleanup did not release current ownership and retain corpse identity");
  std::optional<LanHost> host;std::optional<LanClient> peer;
  if(network) {
    host.emplace(match,0,0xdead0015);
    peer.emplace("127.0.0.1:"+std::to_string(host->port()),0xdead0015,1);
    for(unsigned n=0;n<80 && !peer->ready();++n) {
      peer->send(std::array<Controls,1>{});host->receive(inputs);host->broadcast();peer->receive();
    }
    check(peer->ready(),"Actual death-cycle UDP client did not join");
  }
  auto snapshot=[&] {
    if(!network)return match.state();
    host->broadcast();peer->receive();
    check(peer->state().tick==match.state().tick &&
        peer->state().players[1].source_unit==match.state().players[1].source_unit &&
        peer->state().players[1].source_respawn_countdown==match.player(1).respawn.countdown,
        "UDP snapshot lost actual death/respawn ownership or countdown");
    return peer->state();
  };
  Mesh mesh;MultiplayerScoreboard board(cache,cache.shared_directory()/"fonts",mesh,variant);
  check(board.available(),"Original scoreboard providers unavailable");
  const std::array<unsigned,2> ids{0,1};
  bool automatic{},respawned{},restored{};inputs={};
  for(unsigned n=0;n<160;++n) {
    const auto state=snapshot();
    for(unsigned view=0;view<ids.size();++view)board.step(view,false,state,ids[view]);
    const auto living=board.draw(state,ids,0,1280,720);
    const auto victim=board.draw(state,ids,1,1280,720);
    check(living.normal_hud && living.paint.draw.vertices.empty(),
        "Automatic victim scoreboard leaked to a living local view");
    if(state.players[1].source_unit==null_datum && state.players[1].source_respawn_countdown>1)
      check(victim.paint.draw.vertices.empty(),"Automatic scoreboard preceded the original countdown gate");
    if(!victim.normal_hud) {
      check(victim.layout.status==FullScoreboardStatus::Ready &&
          !victim.paint.draw.vertices.empty(),"Automatic death scoreboard did not submit actual roster/font/cells");
      const auto row=std::find_if(victim.layout.rows.begin(),victim.layout.rows.end(),
          [&](const auto& r){return r.datum==old_owner;});
      check(row!=victim.layout.rows.end() && row->current_player,
          "Automatic death scoreboard lost the victim's full player highlight");
      automatic=true;
    }
    if(state.players[1].source_unit!=null_datum) {
      check(state.players[1].source_unit!=old_body && state.players[1].source_player==old_owner,
          "Respawn reused the dead unit identity or replaced the player identity");
      respawned=true;
      if(automatic && victim.normal_hud && victim.paint.draw.vertices.empty())restored=true;
    }
    if(restored)break;
    match.step(inputs);
  }
  check(automatic && respawned && restored,
      "Actual fatal hit/automatic scoreboard/source-slot respawn/HUD restoration cycle incomplete");
  const auto* corpse=match.unit_from_datum(old_body);
  check(corpse && corpse->vitality.dead && corpse->source_previous_player==old_owner,
      "Respawn removed the retained corpse or its previous owner");
}
}

int main(int argc,char**argv)try {
  check(argc==3,"Supply project root and original update XBE");
  const std::filesystem::path root=argv[1],maps=root/"assets/disc/maps";
  if(!std::filesystem::exists(maps/"lockout.map") || !std::filesystem::exists(argv[2]))return 77;
  const auto original=OriginalData::load_xbe(argv[2]);
  original_gates(*original,root/"tests/fixtures/death_scoreboard_original.tsv");
  Cache cache(maps/"lockout.map",{},original);const auto content=cache.load_content(0,true);
  const UiVariant variant{"Death cycle",0,{{89,5},{2,0},{10,0}}};
  for(bool network:{false,true})live_cycle(cache,content,variant,cache.spawns(),network);
  std::cout<<"240 unchanged-original gates and actual SMG death, automatic per-view scoreboard, salted respawn and HUD restoration passed locally and over UDP\n";
  std::cout<<"LIMIT death animation/physical corpse pose and death-camera world delivery remain separate owners\n";
  return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
