#include "halo2/multiplayer_world.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/collision.hpp"
#include "halo2/source_camera.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>

using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool value,const char* why){if(!value)throw FormatError(why);}
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t address,unsigned size=4) {
  const auto bytes=memory.view(address,size);std::uint32_t value{};
  for(unsigned n=0;n<size;++n)value|=std::to_integer<std::uint32_t>(bytes[n])<<(8*n);
  return value;
}
struct Files {
  std::filesystem::path path;
  explicit Files(unsigned index) {
    const auto scratch=std::getenv("HALO2_TEST_SCRATCH");check(scratch,"Round wins require owned scratch");
    path=std::filesystem::path(scratch)/("engine-round-wins-"+std::to_string(index));std::filesystem::create_directories(path);
  }
  ~Files(){std::error_code error;std::filesystem::remove_all(path,error);}
};
struct Observation {
  MultiplayerObjectRuntime& objects;
  ~Observation(){objects.observe_physics_stores({});}
};
}
int main(int argc,char** argv)try {
  std::cout<<std::unitbuf;
  check(argc==4 || argc==5,"Usage: multiplayer_round_wins_tu15_tests MCC_MAP VARIANTS REPORT [ROUND_KIND]");
  if(!std::filesystem::is_regular_file(argv[1]))return 77;
  Json fixture;std::ifstream(argv[2])>>fixture;
  const int selected=argc==5?std::stoi(argv[4]):-1;
  check(selected>=-1 && selected<=6,"Round selector outside original range");
  Json report={{"passed",false},{"cases",Json::array()},
    {"scope","Canonical Slayer and Team Slayer with original score-limit parameter2=1 and round selector0=0..6. Each round is won by a fatal hit through ordinary accumulated aim/fire controls, actual attribution and engine scoring. Initial positions in each round use B95D0 with an authored clear spawn pair. No vitality, damage, score, win, clock, phase, timer, reset or completion result is supplied. Other edited settings, modes, presentation and network acceptance remain separate."}};
  const auto save=[&]{std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';check(bool(out),"Cannot retain winning-round report");};
  // Original authored round counts, independently checked by real endings:
  // one/two/four/six rounds; first to two/three/four wins.
  constexpr std::array<unsigned,7> rounds{1,2,4,6,2,3,4};
  unsigned index{},failures{};
  for(bool teams:{false,true})for(int kind=0;kind<=6;++kind) {
    if(selected!=-1 && selected!=kind)continue;
    const std::string name=teams?"Team Slayer":"Slayer";
    Json row={{"name",name},{"round_kind",kind},{"passed",false},{"stage","construct"},{"rounds",Json::array()}};
    try {
      Files files(index++);MultiplayerOptions options;options.map=argv[1];options.source_object_world=true;
      options.source_object_savegame_directory=files.path;options.local_players=2;
      const auto found=std::find_if(fixture.at("variants").begin(),fixture.at("variants").end(),
          [&](const auto& p){return p.at("name")==name;});
      check(found!=fixture.at("variants").end(),"Missing authored Slayer preset");
      options.variant={name,found->at("frontend_engine").template get<unsigned>(),{}};
      for(const auto& w:found->at("writes"))options.variant.values.append(w[0].template get<unsigned>(),w[1].template get<int>());
      const auto authored=source_game_variant(options.variant);
      options.variant.values.append(0,kind);options.variant.values.append(2,1);
      const auto variant=source_game_variant(options.variant);check(source_game_variant_valid(variant),"Original score/round edits are invalid");
      for(unsigned n=0;n<variant.bytes.size();++n)if(n<0x4c || n>=0x54)
        check(variant.bytes[n]==authored.bytes[n],"Round/score edit changed an unrelated authored option");
      if(teams)options.selected_teams={0,1};
      MultiplayerPreparedWorld prepared(options);auto& match=prepared.match;auto& objects=*prepared.source_objects;
      auto& world=objects.tu15_world();auto& memory=world.memory();
      check(prepared.rules.requires_source_engine,"Round wins selected a bounded adapter");
      const auto shooter=prepared.local_slots.at(0),victim=prepared.local_slots.at(1);
      CollisionWorld collision(prepared.content.collision);std::array<Spawn,2> positions{};
      float closest=std::numeric_limits<float>::infinity();
      for(const auto& a:prepared.cache->spawns())for(const auto& b:prepared.cache->spawns()) {
        const auto separation=length(b.position-a.position);if(separation<2 || separation>8 || separation>=closest)continue;
        const auto eye=a.position+Vec3{0,0,prepared.content.player.camera_height};
        const auto delta=b.position+Vec3{0,0,prepared.content.player.camera_height}-eye;
        if(collision.raycast(eye,delta/length(delta),length(delta)))continue;
        positions={a,b};closest=separation;
      }
      check(std::isfinite(closest),"Round map lacks a clear authored spawn pair");
      unsigned end_calls{},attribution{},death_stores{},frames{};
      Observation observation{objects};
      objects.observe_physics_stores([&](auto pc,const SourcePhysicsContactRecordStore& store) {
        if(pc==0x15ddc3) {
          check(store.written.size()==4,"Original round-ending prologue changed");++end_calls;
          row["last_end"]={{"caller",word(memory,store.address+0x18)},
              {"force",word(memory,store.address+0x1c,1)}};
        }
        if(pc>=0x1f2280 && pc<0x1f2888)++attribution;
        if(pc>=0x15f8b0 && pc<0x1600f4)++death_stores;
      });
      std::array<Controls,multiplayer_capacity> controls{};
      for(const auto slot:prepared.local_slots){controls[slot].yaw=match.unit(slot).yaw;controls[slot].pitch=match.unit(slot).pitch;}
      auto source=world.multiplayer_engine_state();auto state=match.state();
      const auto step=[&] {
        const auto unit=match.player(shooter).source_current_object;
        if(source.round.phase==1 && unit!=null_datum && !match.unit(victim).vitality.dead) {
          if(const auto look=objects.player_look(match.player(shooter).source_datum,unit)) {
            const auto& from=match.unit(shooter);const auto& to=match.unit(victim);
            const auto eye=ordinary_unit_eye(from.body.position,from.definition.camera_height,from.definition.crouch_camera_height,from.crouch);
            const auto delta=to.body.position+Vec3{0,0,to.body.height*.5f}-eye;
            controls[shooter].yaw+=std::remainder(std::atan2(delta.y,delta.x)-look->x,Bytes(memory.view(0x44923c,4)).f32(0));
            controls[shooter].pitch+=std::atan2(delta.z,std::hypot(delta.x,delta.y))-look->y;
          }
        }
        match.step(controls);++frames;state=match.state();source=world.multiplayer_engine_state();
        check(state.tick==word(memory,word(memory,0x51df54)+8),"Winning round clocks diverged");
        check(std::memcmp(&source.statistics,&match.source_match_statistics(),sizeof(source.statistics))==0,
            "Winning round advanced a parallel native score/win bank");
        check(match.recorded_statistics() && std::memcmp(match.recorded_statistics(),memory.view(0x51eef4,sizeof(SourceResultStatistics)).data(),sizeof(SourceResultStatistics))==0,
            "Winning round advanced a parallel retained bank");
      };
      std::uint32_t previous_shooter=null_datum,previous_victim=null_datum;
      for(unsigned round=0;round<rounds[unsigned(kind)];++round) {
        row["stage"]="winning-round-"+std::to_string(round);check(source.round.phase==1 && unsigned(source.round.index)==round,"Next winning round is not active");
        //15F450 enters the active phase after this tick's player/factory phase.
        // Let the next ordinary delivery perform actual current-unit birth.
        unsigned factory_frames{};
        while(factory_frames<32 && (match.player(shooter).source_current_object==null_datum ||
            match.player(victim).source_current_object==null_datum)) {step();++factory_frames;}
        const auto shooter_unit=match.player(shooter).source_current_object,victim_unit=match.player(victim).source_current_object;
        check(shooter_unit!=null_datum && victim_unit!=null_datum && !source.recording.frozen,"Winning round lacks genuine unfrozen units");
        check(shooter_unit!=previous_shooter && victim_unit!=previous_victim,"Winning round reused retired current-unit generations");
        previous_shooter=shooter_unit;previous_victim=victim_unit;
        world.set_object_position(shooter_unit,positions[0].position);world.set_object_position(victim_unit,positions[1].position);
        for(unsigned n=0;n<100;++n)step();
        const auto before_kills=match.player(shooter).kills,before_deaths=match.player(victim).deaths;
        const auto before_attribution=attribution,before_delivery=death_stores;
        controls[shooter].fire=true;unsigned firing{};
        while(firing<240 && source.round.phase==1 && !state.finished) {step();++firing;}
        controls[shooter].fire=false;
        check(end_calls==round+1 && source.round.phase==2,"Fatal scoring did not reach exactly one original round ending");
        check(match.player(shooter).kills==before_kills+1 && match.player(victim).deaths==before_deaths+1 &&
            attribution>before_attribution && death_stores>before_delivery,"Winning score bypassed fatal attribution or engine delivery");
        check(state.winner==(teams?-1:int(shooter)) && state.winner_team==(teams?0:-1),"Native consumer lost actual scoring winner");
        check(source.statistics.players[shooter].fields[0]==1 && source.statistics.players[shooter].fields[7]==int(round+1) &&
            state.players[shooter].wins==int(round+1) && match.recorded_statistics()->players[shooter][6]==round+1,
            "Winning round lost its score or accumulated live/retained player wins");
        if(teams)check(source.statistics.teams[0].fields[0]==1 && source.statistics.teams[0].fields[7]==int(round+1) &&
            state.team_wins[0]==int(round+1) && match.recorded_statistics()->teams[0][6]==round+1,"Winning team lost accumulated wins");
        const bool last=round+1==rounds[unsigned(kind)];
        check(state.finished==last,"Original round rule finished at the wrong number of actual wins");
        row["rounds"].push_back({{"index",round},{"tick",state.tick},{"firing_frames",firing},{"wins",round+1},{"finished",state.finished},
            {"factory_frames",factory_frames},{"shooter_unit",shooter_unit},{"victim_unit",victim_unit}});report["in_progress"]=row;save();
        if(!last) {
          const auto end_tick=state.tick;
          while(state.tick-end_tick<181 && source.round.phase!=1)step();
          check(state.tick-end_tick==180 && source.round.phase==1 && unsigned(source.round.index)==round+1 &&
              !source.recording.frozen,"Winning round did not complete its original150/30 reset");
          check(source.statistics.players[shooter].fields[0]==0 && source.statistics.players[shooter].fields[7]==int(round+1),
              "Round reset failed to clear current score while retaining actual wins");
        }
      }
      row["stage"]="natural-postgame";const auto completed=source.statistics;
      for(unsigned n=0;n<211;++n){step();check(std::memcmp(&completed,&source.statistics,sizeof(completed))==0,"Winning postgame changed completed scores/wins");}
      check(source.finalized && source.recording.frozen && source.game_over.complete() && source.game_over.fade_started && word(memory,0x51dfb1,1),
          "Winning match did not complete actual retained results/countdown/fade");
      row["ordinary_frames"]=frames;row["passed"]=true;row["stage"]="complete";
    }catch(const std::exception& error){row["error"]=error.what();++failures;}
    std::cout<<row.dump()<<'\n';report["cases"].push_back(std::move(row));report.erase("in_progress");save();
  }
  check(index==(selected==-1?14u:2u),"Winning round-rule coverage changed");report["passed"]=failures==0;save();return failures?1:0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
