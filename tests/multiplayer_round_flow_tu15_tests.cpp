#include "halo2/multiplayer_world.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>

using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool value,const char* why){if(!value)throw FormatError(why);}
std::uint32_t word(const SourceRuntimeMemory& m,std::uint32_t p,unsigned size=4) {
  const auto bytes=m.view(p,size);std::uint32_t value{};
  for(unsigned n=0;n<size;++n)value|=std::to_integer<std::uint32_t>(bytes[n])<<(n*8);
  return value;
}
struct Files {
  std::filesystem::path path;
  explicit Files(unsigned index) {
    const auto scratch=std::getenv("HALO2_TEST_SCRATCH");check(scratch,"Round flow requires owned scratch");
    path=std::filesystem::path(scratch)/("engine-round-flow-"+std::to_string(index));std::filesystem::create_directories(path);
  }
  ~Files(){std::error_code error;std::filesystem::remove_all(path,error);}
};
struct Observation {
  MultiplayerObjectRuntime& objects;
  ~Observation(){objects.observe_physics_stores({});}
};
Json phase(const SourceRoundLifecycle& p) {
  return {{"phase",p.phase},{"status",p.status},{"index",p.index},{"clock_seconds",p.clock_seconds},
          {"countdown",p.countdown},{"start_tick",p.start_tick},{"flags",p.flags}};
}
}
int main(int argc,char** argv)try {
  std::cout<<std::unitbuf;
  check(argc==4 || argc==5,"Usage: multiplayer_round_flow_tu15_tests MCC_MAP VARIANTS REPORT [PRESET]");
  if(!std::filesystem::is_regular_file(argv[1]))return 77;
  Json fixture;std::ifstream(argv[2])>>fixture;
  Json report={{"passed",false},{"cases",Json::array()},
    {"scope","Canonical source rosters, units and full ordinary frames. Original descriptor parameter10=1 is the sole authored-preset edit. No clock, score, winner, pending phase, timer, reset or completion result is supplied. Single-round presets must end naturally; continuing rules must complete real intermission/reset and a second natural round ending. This does not qualify all edited settings, combat/objective-driven wins, presentation, network modes or full played matches."}};
  const auto save=[&]{std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';check(bool(out),"Cannot retain ordinary round report");};
  unsigned index{},failures{};
  for(const auto& preset:fixture.at("variants")) {
    const auto name=preset.at("name").get<std::string>();if(argc==5 && name!=argv[4])continue;
    Json row={{"name",name},{"source_type",preset.at("source_type")},{"passed",false},{"stage","construct"}};
    try {
      Files files(index++);MultiplayerOptions options;options.map=argv[1];options.source_object_world=true;
      options.source_object_savegame_directory=files.path;options.variant={name,preset.at("frontend_engine").get<unsigned>(),{}};
      for(const auto& w:preset.at("writes"))options.variant.values.append(w[0].get<unsigned>(),w[1].get<int>());
      const auto authored=source_game_variant(options.variant);options.variant.values.append(10,1);
      const auto edited=source_game_variant(options.variant);check(source_game_variant_valid(edited),"Original one-second time edit is invalid");
      const auto descriptor=source_game_variant_descriptors()[10];
      for(unsigned n=0;n<edited.bytes.size();++n)if(n<descriptor.offset || n>=descriptor.offset+4)
        check(edited.bytes[n]==authored.bytes[n],"Time edit changed an unrelated authored option");
      const auto type=source_game_variant_type(edited),kind=source_game_variant_get(edited,0);
      options.local_players=type==7?4:2;const bool teams=source_game_variant_get(edited,91)!=0;
      if(teams)for(unsigned n=0;n<options.local_players;++n)options.selected_teams.push_back(n%2);
      MultiplayerPreparedWorld prepared(options);auto& match=prepared.match;auto& objects=*prepared.source_objects;
      auto& world=objects.tu15_world();auto& memory=world.memory();
      check(prepared.rules.requires_source_engine,"Round flow selected a bounded adapter");
      const auto engine=word(memory,0x4f63b8),rate=word(memory,word(memory,0x51df54)+2,2);
      check(rate==30 && word(memory,word(memory,0x4f3218)+0x190)==1,"Round clock did not consume the original one-second option");
      std::array<Controls,multiplayer_capacity> controls{};
      for(const auto slot:prepared.local_slots){controls[slot].yaw=match.unit(slot).yaw;controls[slot].pitch=match.unit(slot).pitch;}
      row["round_kind"]=kind;row["round_ends"]=Json::array();row["transitions"]=Json::array();
      unsigned ending_calls{};std::uint32_t last_winner=null_datum;
      Observation observation{objects};
      objects.observe_physics_stores([&](auto pc,const SourcePhysicsContactRecordStore& store) {
        if(pc!=0x15ddc3)return;
        // Actual15DDC0 push EBP after sub ESP,14. The timer caller's original
        // 15DD50 ranking buffer remains below this stack frame. Decode its
        // genuine tie/winner fields exactly as15DD78..15DDB3; no entry result
        // or winner is supplied. This also proves the actual timer caller.
        check(store.written.size()==4 && word(memory,store.address+0x18)==0x15e9e4,
            "Natural expiry did not enter15DDC0 through its original timer caller");
        const auto tied=word(memory,store.address+(teams?0xcu:std::uint32_t(-4)),1);
        last_winner=tied?null_datum:teams?
            std::uint32_t(std::int32_t(std::int16_t(word(memory,store.address-0x14,2)))):
            word(memory,store.address-0x54);
        ++ending_calls;
        row["round_ends"].push_back({{"tick",word(memory,word(memory,0x51df54)+8)},
            {"index",word(memory,engine+0x6e,2)},{"winner",last_winner},{"force",word(memory,store.address+0x1c,1)}});
      });
      auto source=world.multiplayer_engine_state();auto state=match.state();
      check(source.round.phase==1 && source.round.index==0 && source.recording.enabled && !source.recording.frozen,
          "Canonical round did not start active with enabled recording");
      row["initial"]=phase(source.round);std::array<std::uint32_t,multiplayer_capacity> first_units{};
      for(const auto slot:prepared.local_slots)first_units[slot]=match.player(slot).source_current_object;
      unsigned ordinary_frames{},round2_active_tick{},first_intermission_tick{},second_phase_tick{};
      bool observed_second_round{},replacement_units{};std::int16_t previous_phase=source.round.phase,previous_index=source.round.index;
      const auto step=[&] {
        const auto before_calls=ending_calls;match.step(controls);++ordinary_frames;state=match.state();source=world.multiplayer_engine_state();
        check(state.tick==word(memory,word(memory,0x51df54)+8),"Ordinary source/native round clocks diverged");
        check(std::memcmp(&source.statistics,&match.source_match_statistics(),sizeof(source.statistics))==0,
            "Round flow advanced a separate native score/win bank");
        const auto retained=match.recorded_statistics();
        check(retained && std::memcmp(retained,memory.view(0x51eef4,sizeof(SourceResultStatistics)).data(),sizeof(SourceResultStatistics))==0,
            "Round flow advanced a separate native retained bank");
        if(ending_calls!=before_calls) {
          check(state.winner==(teams || last_winner==null_datum?-1:int(last_winner&65535u)) &&
              state.winner_team==(!teams || last_winner==null_datum?-1:int(last_winner)),
              "Native outcome differs from the actual original round-end invocation");
        }
        if(previous_phase!=source.round.phase || previous_index!=source.round.index) {
          row["transitions"].push_back({{"tick",state.tick},{"round",phase(source.round)},
              {"recording_frozen",bool(source.recording.frozen)},{"finished",state.finished}});
          previous_phase=source.round.phase;previous_index=source.round.index;
          report["in_progress"]=row;save();
        }
      };
      row["stage"]="natural-time-limit";
      while(ordinary_frames<600 && !state.finished && !(ending_calls>=2 && source.round.phase==2)) {
        step();
        if(ending_calls && source.round.phase==2 && !first_intermission_tick) {
          first_intermission_tick=state.tick;
          check(source.round.countdown==int(rate*5) && source.recording.frozen,"Original round ending did not install its real frozen intermission");
        }
        if(source.round.phase==3 && !second_phase_tick)second_phase_tick=state.tick;
        if(source.round.index==1 && source.round.phase==1) {
          if(!observed_second_round) {
            observed_second_round=true;round2_active_tick=state.tick;
            check(source.round.clock_seconds==1 && !source.recording.frozen && (source.statistics.header&255u),
                "Original second-round preparation did not reset its clock/validity/recording gate");
          }
          unsigned units{};bool fresh=true;
          for(const auto slot:prepared.local_slots) {
            const auto unit=match.player(slot).source_current_object;
            if(unit!=null_datum) {++units;fresh&=unit!=first_units[slot];
              check(state.players[slot].source_unit==unit && !state.players[slot].dead,"Native second round lost its genuine replacement unit");}
          }
          const auto cap=source_game_variant_get(edited,90);const auto expected=cap>0 && !teams?std::min(options.local_players,unsigned(cap)):options.local_players;
          replacement_units|=fresh && units==expected;
        }
      }
      check(ending_calls>0,"Original timer never reached its own round-ending owner");
      if(kind==0)check(state.finished && ending_calls==1,"Original single-round timer failed to complete the match exactly once");
      else {
        check(kind==5 && !state.finished,"Authored first-to-three rules ended before a qualifying third win");
        check(ending_calls==2 && observed_second_round && replacement_units,"Ordinary continued-round reset did not reach a second genuine expiry with replacement units");
        check(second_phase_tick-first_intermission_tick==rate*5 && round2_active_tick-second_phase_tick==rate,
            "Original intermission/next-round intervals differ from the five/one-second owners");
      }
      row["reset"]={{"observed",observed_second_round},{"replacement_units",replacement_units},
          {"first_intermission_tick",first_intermission_tick},{"next_phase_tick",second_phase_tick},{"second_active_tick",round2_active_tick}};
      row["natural_end"]=phase(source.round);row["finished"]=state.finished;
      if(state.finished) {
        row["stage"]="natural-postgame";const auto frozen=source.statistics;
        for(unsigned n=0;n<211;++n) {
          step();check(std::memcmp(&source.statistics,&frozen,sizeof(frozen))==0,"Natural postgame changed its completed score/win bank");
        }
        check(source.finalized && source.game_over.complete() && source.game_over.fade_started && source.recording.frozen && word(memory,0x51dfb1,1),
            "Natural expiry did not complete actual retained results/countdown/fade");
        row["postgame_frames"]=211;
      }
      row["ordinary_frames"]=ordinary_frames;row["tick"]=state.tick;row["passed"]=true;row["stage"]="complete";
    }catch(const std::exception& error){row["error"]=error.what();++failures;}
    std::cout<<row.dump()<<'\n';report["cases"].push_back(std::move(row));report.erase("in_progress");save();
  }
  check(index==(argc==5?1u:39u),"Natural-round preset coverage changed");report["passed"]=failures==0;save();return failures?1:0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
