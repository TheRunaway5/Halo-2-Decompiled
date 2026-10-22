#include "halo2/multiplayer_world.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>

using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool value,const char* why){if(!value)throw FormatError(why);}
std::uint32_t word(const SourceRuntimeMemory& m,std::uint32_t p){return Bytes(m.view(p,4)).u32(0);}
template<class Address>std::int16_t half(const SourceRuntimeMemory& m,Address p){return std::bit_cast<std::int16_t>(Bytes(m.view(p,2)).u16(0));}
std::int8_t byte(const SourceRuntimeMemory& m,std::uint32_t p){return std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(m.view(p,1)[0]));}
float scalar(const SourceRuntimeMemory& m,std::uint32_t p){return Bytes(m.view(p,4)).f32(0);}
Json point(Vec3 p){return Json::array({p.x,p.y,p.z});}
struct Files {
  std::filesystem::path path;
  explicit Files(unsigned n) {
    const auto scratch=std::getenv("HALO2_TEST_SCRATCH");check(scratch,"Zone flow test needs owned scratch");
    path=std::filesystem::path(scratch)/("zone-flow-"+std::to_string(n));std::filesystem::create_directories(path);
  }
  ~Files(){std::error_code error;std::filesystem::remove_all(path,error);}
};
}
int main(int argc,char** argv)try {
  std::cout<<std::unitbuf;
  if(argc!=4)throw FormatError("Usage: multiplayer_zone_flow_tu15_tests MCC_MAP VARIANT_FIXTURE REPORT");
  if(!std::filesystem::is_regular_file(argv[1]))return 77;
  Json fixture;std::ifstream(argv[2])>>fixture;
  const bool movement=std::getenv("HALO2_ZONE_MOVEMENT");
  const bool regions=std::getenv("HALO2_ZONE_REGIONS");
  const bool winning=std::getenv("HALO2_ZONE_WIN_FLOW");
  check(!winning || (!movement && !regions),"Winning zone scope excludes movement/region cohorts");
  Json report={{"passed",false},{"movement",movement},{"all_regions",regions},{"cases",Json::array()},
    {"scope","Canonical source-born players and ordinary full source frames. Declared B95D0 position inputs use actual authored zones and original King containment queries. Occupancy, capture, ownership, timers and scores are never supplied. Played approach, complete matches, rendering/audio and replication remain separate acceptance."}};
  const auto save=[&]{std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';check(bool(out),"Cannot retain zone flow report");};
  report["winning_flow"]=winning;
  if(winning)report["scope"]="All ten authored King/Territories presets with only original score-limit parameter2 edited to one. B95D0 positions use actual authored zones; ordinary containment/capture/ownership/periodic scoring must select the original winner and complete its postgame or continuing-round reset. No score, win, clock, timer, phase, reset or completion result is supplied. Played approaches, simultaneous regions, other edited settings, presentation and networking remain separate.";
  unsigned index{},failures{};
  for(const auto& preset:fixture.at("variants")) {
    const auto type=preset.at("source_type").get<unsigned>();if(type!=4 && type!=8)continue;
    if(movement && type!=4)continue;
    if(regions && type!=8)continue;
    const auto name=preset.at("name").get<std::string>();
    if(const auto chosen=std::getenv("HALO2_ZONE_VARIANT");chosen && name!=chosen)continue;
    UiVariant selected{name,preset.at("frontend_engine").get<unsigned>(),{}};
    for(const auto& w:preset.at("writes"))selected.values.append(w[0].get<unsigned>(),w[1].get<int>());
    const auto authored_variant=source_game_variant(selected);
    const auto region_cases=regions?unsigned(source_game_variant_get(authored_variant,74)):1u;
    check(region_cases>0 && region_cases<=8,"Configured territory region count escapes its source bank");
    for(unsigned requested_region=0;requested_region<region_cases;++requested_region) {
      Json row={{"name",name},{"source_type",type},{"region",requested_region},{"passed",false},{"stage","construct"}};
      try {
        Files files(index++);MultiplayerOptions options;options.map=argv[1];options.local_players=2;
        options.source_object_world=true;options.source_object_savegame_directory=files.path;
        options.variant=selected;
        if(winning)options.variant.values.append(2,1);
        const auto variant=source_game_variant(options.variant);const bool teams=source_game_variant_get(variant,91)!=0;
        if(winning) {
          check(source_game_variant_valid(variant),"Original one-point zone edit is invalid");
          const auto descriptor=source_game_variant_descriptors()[2];
          for(unsigned n=0;n<variant.bytes.size();++n)if(n<descriptor.offset || n>=descriptor.offset+4)
            check(variant.bytes[n]==authored_variant.bytes[n],"Zone score edit changed an unrelated authored setting");
        }
        if(teams)options.selected_teams={0,1};
        MultiplayerPreparedWorld prepared(options);auto& match=prepared.match;auto& objects=*prepared.source_objects;
        auto& world=objects.tu15_world();auto& memory=world.memory();
        check(prepared.rules.requires_source_engine && prepared.local_slots.size()==2,"Zone flow requires the canonical source engine and two players");
        const auto actor=prepared.local_slots[0],enemy=prepared.local_slots[1];
        const auto actor_unit=match.player(actor).source_current_object,enemy_unit=match.player(enemy).source_current_object;
        check(actor_unit!=null_datum && enemy_unit!=null_datum,"Source factory did not birth both zone participants");
        const auto engine=word(memory,0x4f63b8u),bank=engine+0xfcu;
        const auto rate=half(memory,word(memory,0x51df54u)+2u);check(rate>0 && rate<=120,"Zone flow has no admitted source clock");
        std::array<Controls,multiplayer_capacity> controls{};
        for(const auto slot:prepared.local_slots){controls[slot].yaw=match.unit(slot).yaw;controls[slot].pitch=match.unit(slot).pitch;}
        auto state=match.state();row["frames"]=Json::array();
        const auto score=[&](unsigned slot){return teams?int(state.team_scores[unsigned(match.player(slot).source_membership)]):state.players[slot].score;};
        const auto step=[&] {
          match.step(controls);state=match.state();const auto original=world.multiplayer_engine_state();
          check(std::memcmp(&original.statistics,&match.source_match_statistics(),sizeof(original.statistics))==0,"Zone flow advanced a separate native score/stat bank");
          check(word(memory,word(memory,0x51df54u)+8u)==state.tick,"Native and original zone clocks differ");
          if(type==4)check(match.source_king_state() && std::memcmp(match.source_king_state(),memory.view(bank,sizeof(SourceKingState)).data(),sizeof(SourceKingState))==0,
              "Native King publication differs from its actual inline source bank");
        };
        const auto sample=[&](const char* phase) {
          row["frames"].push_back({{"phase",phase},{"tick",state.tick},{"scores",{score(actor),score(enemy)}},
              {"positions",{point(state.players[actor].position),point(state.players[enemy].position)}}});
        };
        const auto complete_win=[&] {
          row["stage"]="original-zone-winning-outcome";auto original=world.multiplayer_engine_state();unsigned scoring_frames{};
          while(scoring_frames<unsigned(rate)*2u+8u && original.round.phase==1) {
            step();++scoring_frames;original=world.multiplayer_engine_state();
          }
          const auto team=match.player(actor).source_membership;
          check(original.round.phase==2 && original.recording.enabled && original.recording.frozen &&
              state.winner==(teams?-1:int(actor)) && state.winner_team==(teams?int(team):-1),
              "Ordinary zone scoring did not select its real original frozen winner");
          check(original.statistics.players[actor].fields[7]==1 && (!teams || original.statistics.teams[unsigned(team)].fields[7]==1),
              "Winning zone omitted its original player/team win");
          const auto retained=[&] {
            check(match.recorded_statistics() && std::memcmp(match.recorded_statistics(),memory.view(0x51eef4u,sizeof(SourceResultStatistics)).data(),sizeof(SourceResultStatistics))==0,
                "Winning zone advanced a parallel retained bank");
          };retained();
          check(match.recorded_statistics()->players[actor][6]==1 && (!teams || match.recorded_statistics()->teams[unsigned(team)][6]==1),
              "Winning zone omitted its retained player/team win");
          const auto kind=source_game_variant_get(variant,0);
          check((kind==0 || kind==5) && state.finished==(kind==0),"Authored winning-zone rule selected the wrong match completion");
          row["winning_outcome"]={{"tick",state.tick},{"round_kind",kind},{"scoring_frames",scoring_frames},
              {"winner",state.winner},{"winner_team",state.winner_team},{"finished",state.finished}};
          if(state.finished) {
            const auto completed=original.statistics;
            for(unsigned n=0;n<211;++n){step();original=world.multiplayer_engine_state();retained();
              check(std::memcmp(&original.statistics,&completed,sizeof(completed))==0,"Zone postgame changed completed scores/wins");}
            check(original.finalized && original.game_over.complete() && original.game_over.fade_started && original.recording.frozen,
                "Winning zone did not complete actual retained results/countdown/fade");
            row["winning_outcome"]["postgame_frames"]=211;
          }else {
            const auto end_tick=state.tick;
            for(unsigned n=0;n<180;++n){step();original=world.multiplayer_engine_state();retained();}
            check(state.tick-end_tick==180 && original.round.phase==1 && original.round.index==1 && !original.recording.frozen &&
                original.statistics.players[actor].fields[7]==1,"Winning zone lost its real150/30 reset or accumulated win");
            row["winning_outcome"]["reset_tick"]=state.tick;
            unsigned factory_frames{};
            while(factory_frames<32 && (match.player(actor).source_current_object==null_datum || match.player(enemy).source_current_object==null_datum)) {
              step();retained();++factory_frames;
            }
            check(match.player(actor).source_current_object!=null_datum && match.player(actor).source_current_object!=actor_unit &&
                match.player(enemy).source_current_object!=null_datum && match.player(enemy).source_current_object!=enemy_unit,
                "Winning zone reset failed to replace genuine current units");
            row["winning_outcome"]["factory_frames"]=factory_frames;
          }
        };
        if(type==4) {
          row["stage"]="original-hill-input-selection";
          check(word(memory,0x52d8a0u)==bank,"Original King publication does not point at its inline state");
          const auto contains=[&](unsigned slot) {
            auto a=world.prepare_source_call();a.gp[0]=match.player(slot).source_datum;a.gp[7]=bank;
            world.run_actual(0x2c8640u,a);return (a.gp[0]&255u)!=0;
          };
          const auto initial=*match.source_king_state();check(initial.marker_count>=4 && initial.radius>0,"Original round did not build an authored hill");
          row["hill"]={{"index",initial.hill},{"center",initial.center},{"radius",initial.radius},
              {"minimum_z",initial.minimum_z},{"maximum_z",initial.maximum_z},{"move_ticks",initial.move_ticks}};
          Vec3 inside{};bool found{};
          for(const float z:{initial.center[2],(initial.minimum_z+initial.maximum_z)*.5f})
            for(const float x:{0.f,1.f,-1.f,2.f,-2.f})for(const float y:{0.f,1.f,-1.f,2.f,-2.f})if(!found) {
              const Vec3 p{initial.center[0]+initial.radius*x/32.f,initial.center[1]+initial.radius*y/32.f,z};
              world.set_object_position(actor_unit,p);if(contains(actor)){inside=p;found=true;}
            }
          check(found,"Original King containment query rejected all declared interior inputs");
          Vec3 outside{};found=false;
          for(const auto& spawn:prepared.cache->spawns())if(!found) {
            world.set_object_position(enemy_unit,spawn.position);if(!contains(enemy)){outside=spawn.position;found=true;}
          }
          check(found,"No authored spawn lies outside the current source hill");
          row["inside_input"]=point(inside);row["outside_input"]=point(outside);
          const auto before=score(actor);row["stage"]="ordinary-solo-hill";
          if(winning)complete_win();else {
          for(unsigned n=0;n<unsigned(rate)*2u;++n) {
            step();check(contains(actor) && match.player(actor).source_king_inside_ticks>0,"Genuine hill occupant lost containment during ordinary physics");
            if(n%unsigned(rate)==0)sample("solo");
          }
          check(score(actor)==before+2,"Original solo King owner did not award two periodic points");
          const auto flags=word(memory,word(memory,0x4f3218u)+0x22cu);
          const std::array before_contest{score(actor),score(enemy)};
          row["stage"]="ordinary-contested-hill";world.set_object_position(enemy_unit,inside);
          for(unsigned n=0;n<unsigned(rate)+1u;++n)step();sample("contested");
          check(contains(actor) && contains(enemy),"Declared contested hill did not retain both original occupants");
          if(flags&1u)check(score(actor)==before_contest[0] && score(enemy)==before_contest[1],"Original uncontested-only King rule awarded a contested point");
          else check(score(actor)>before_contest[0] && score(enemy)>before_contest[1],"Original shared King scoring did not award both occupants");
          row["contested"]={{"uncontested_only",bool(flags&1u)},{"before",before_contest},{"after",{score(actor),score(enemy)}}};
          row["stage"]="ordinary-uncontested-resume";world.set_object_position(enemy_unit,outside);
          const auto resume=score(actor);for(unsigned n=0;n<unsigned(rate)+1u;++n)step();
          check(score(actor)>resume,"King scoring did not resume after the other occupant left");
          row["stage"]="ordinary-hill-exit";world.set_object_position(actor_unit,outside);
          step();step();const std::array after_exit{score(actor),score(enemy)};
          for(unsigned n=0;n<unsigned(rate);++n)step();sample("outside");
          check(match.player(actor).source_king_inside_ticks==0 && match.player(enemy).source_king_inside_ticks==0 &&
              match.source_king_state()->occupants==0 && score(actor)==after_exit[0] && score(enemy)==after_exit[1],
              "Exited King occupants retained timers, occupants or periodic points");
          if(movement) {
            row["stage"]="ordinary-authored-hill-movement";
            const auto seconds=source_game_variant_get(variant,40);const auto before_move=*match.source_king_state();
            check(seconds>0 && before_move.move_ticks>0,"Authored King movement interval is absent");
            bool changed{};unsigned frames{};
            for(;frames<unsigned(before_move.move_ticks)+2u && !changed && !state.finished;++frames) {
              step();changed=match.source_king_state()->hill!=before_move.hill;
            }
            row["movement"]={{"seconds",seconds},{"frames",frames},{"before_hill",before_move.hill},
                {"after_hill",match.source_king_state()->hill},{"after_move_ticks",match.source_king_state()->move_ticks}};
            check(changed && frames==unsigned(before_move.move_ticks) && match.source_king_state()->marker_count>=4 && match.source_king_state()->move_ticks==seconds*rate,
                "Ordinary original movement clock did not select and publish the next authored hill");
            const std::array next_contains{contains(actor),contains(enemy)};
            step();
            for(unsigned n=0;n<prepared.local_slots.size();++n)
              check((match.player(prepared.local_slots[n]).source_king_inside_ticks>0)==next_contains[n],
                  "Moved King geometry did not reach the next ordinary player containment phase");
            row["movement"]["next_player_phase_verified"]=true;
          }
          }
        }else {
          row["stage"]="authored-territory-input-selection";
          check(word(memory,0x52d8a4u)==bank,"Original territory publication does not point at its inline state");
          const auto count=half(memory,bank+0x114u);check(count>0 && count<=8,"Original territory count escapes its bank");
          const auto markers=word(memory,word(memory,0x4ecc20u)+0x11cu);
          struct Zone{unsigned index;Vec3 center;float radius,down,up;};std::vector<Zone> zones;
          for(unsigned n=0;n<unsigned(count);++n)if(const auto marker=half(memory,bank+0x60u+n*2u);marker>=0) {
            const auto p=Bytes(memory.view(markers+unsigned(marker)*0x20u,12));
            zones.push_back({n,{p.f32(0),p.f32(4),p.f32(8)},scalar(memory,bank+n*4u),scalar(memory,bank+0x20u+n*4u),scalar(memory,bank+0x40u+n*4u)});
          }
          const auto chosen=std::ranges::find(zones,requested_region,&Zone::index);
          check(chosen!=zones.end(),"Configured territory region is absent from the authored scenario");const auto zone=*chosen;
          const auto inside_region=[](const Zone& z,Vec3 p){const auto x=p.x-z.center.x,y=p.y-z.center.y;return x*x+y*y<z.radius*z.radius && p.z-z.center.z<z.up && z.center.z-p.z<z.down;};
          check(inside_region(zone,zone.center),"Original territory bounds do not admit their authored center");
          Vec3 outside{};bool found{};
          for(const auto& spawn:prepared.cache->spawns())if(!found && std::ranges::none_of(zones,[&](const auto& z){return inside_region(z,spawn.position);})) {
            outside=spawn.position;found=true;
          }
          check(found,"No authored spawn lies outside all active territory regions");
          row["territory"]={{"index",zone.index},{"count",count},{"center",point(zone.center)},{"outside",point(outside)},
              {"capture_ticks",half(memory,bank+0x112u)},{"retake_ticks",half(memory,bank+0x110u)}};
          const auto owner=[&]{return word(memory,bank+0x70u+zone.index*4u);};
          const auto progress=[&](unsigned slot){return byte(memory,bank+0x96u+slot*8u);};
          const auto marker_object=[&] {
            const auto size=word(memory,engine+0xc18u);check(size<=8,"Original territory marker registry escaped capacity");
            auto found=null_datum;Json registry=Json::array();
            for(unsigned n=0;n<size;++n) {
              const auto object=word(memory,engine+0xc1cu+n*16u);
              const auto* tag=prepared.cache->find_tag(TagHandle{objects.object_definition(object)});
              const auto index=tag && tag->group=="weap"?int(half(memory,objects.object_body(object)+0x17eu)):-2;
              registry.push_back({{"object",object},{"index",index},{"definition",tag?tag->name:""}});
              if(index!=int(zone.index))continue;
              check(found==null_datum,"Original capture published duplicate marker objects for one territory");
              check(tag && tag->group=="weap" && objects.object_parent(object)==null_datum,
                  "Original territory marker is not a real unparented weapon object");
              found=object;
            }
            row["marker_registry_observations"].push_back(std::move(registry));
            return found;
          };
          std::uint32_t current_marker=null_datum;
          if(regions) {
            // The first factory frame can already enter another authored zone.
            // Let the ordinary outside phase clear that initial capture input.
            for(const auto slot:prepared.local_slots)world.set_object_position(match.player(slot).source_current_object,outside);
            step();step();
            for(const auto slot:prepared.local_slots)
              check(byte(memory,bank+0x95u+slot*8u)==-1 && half(memory,bank+0x90u+slot*8u)==0 && half(memory,bank+0x92u+slot*8u)==0,
                  "Ordinary outside frames did not clear initial territory capture progress");
          }
          const auto capture=[&](unsigned slot,unsigned other,bool retake) {
            world.set_object_position(match.player(other).source_current_object,outside);
            world.set_object_position(match.player(slot).source_current_object,zone.center);
            const int capture_ticks=half(memory,bank+0x112u),retake_ticks=retake?half(memory,bank+0x110u):0;
            check(capture_ticks>0 && (!retake || retake_ticks>0),"Original capture clocks are absent");
            // Original2C9F5D calls2CA5C0 with EAX=NONE after neutralization;
            // the subsequent unowned branch2CA034 uses the separate +112 clock.
            bool hud_progress{},neutralized{};unsigned frames{};auto previous_owner=owner();
            auto& transitions=row[retake?"retake":"capture"]["transitions"];transitions=Json::array();
            for(;frames<unsigned(capture_ticks)+unsigned(retake_ticks)+unsigned(rate)*2u && owner()!=match.player(slot).source_datum && !state.finished;++frames) {
              step();
              if(owner()!=previous_owner) {
                transitions.push_back({{"frame",frames+1},{"owner",owner()}});previous_owner=owner();
                if(regions && retake && owner()==null_datum) {
                  check(marker_object()==null_datum,"Neutralized territory retained its registered marker");
                  bool retired{};try{(void)objects.object_body(current_marker);}catch(const FormatError&){retired=true;}
                  check(retired,"Original neutralization did not retire the old marker generation");
                  row["retake"]["retired_marker"]=current_marker;
                }
              }
              if(retake && owner()==null_datum)neutralized=true;
              if(progress(slot)>0) {
                check(state.players[slot].source_hud_objectives.charge_visible,"Actual territory capture progress is absent from native charge publication");
                hud_progress=true;
              }
              if(frames%unsigned(rate)==0)sample(retake?"retake":"capture");
            }
            auto& result=row[retake?"retake":"capture"];result["frames"]=frames;result["owner"]=owner();
            result["hud_progress"]=hud_progress;result["neutralized"]=neutralized;
            check(owner()==match.player(slot).source_datum && hud_progress && (!retake || neutralized),
                "Ordinary original capture did not complete neutralization and publish its full player owner/progress");
            check(frames==unsigned(capture_ticks+retake_ticks),"Ordinary capture did not preserve both authored clock thresholds");
            if(retake)check(transitions.size()==2 && transitions[0]["frame"]==retake_ticks &&
                transitions[0]["owner"]==null_datum && transitions[1]["frame"]==capture_ticks+retake_ticks,
                "Original opposing capture skipped or mistimed its neutral owner stage");
            if(regions) {
              const auto marker=marker_object();result["marker_object"]=marker;
              check(marker!=null_datum && marker!=current_marker,"Original capture did not birth a new registered marker generation");
              current_marker=marker;
            }
          };
          row["stage"]="ordinary-neutral-territory-capture";capture(actor,enemy,false);
          if(winning)complete_win();else {
          const auto before=score(actor);for(unsigned n=0;n<unsigned(rate)*2u;++n)step();
          check(score(actor)==before+2,"Original territory owner did not receive two periodic points");
          row["stage"]="ordinary-defended-territory";world.set_object_position(enemy_unit,zone.center);
          for(unsigned n=0;n<unsigned(rate)+1u;++n)step();sample("defended");
          check(owner()==match.player(actor).source_datum && progress(enemy)==0,"Original owner presence did not stop an opposing capture");
          row["stage"]="ordinary-territory-retake";capture(enemy,actor,true);
          const auto before_enemy=score(enemy),before_actor=score(actor);
          for(unsigned n=0;n<unsigned(rate)*2u;++n)step();sample("retaken");
          check(score(enemy)==before_enemy+2 && score(actor)==before_actor,"Retaken territory continued scoring for its previous owner");
          }
        }
        row["tick"]=state.tick;row["passed"]=true;row["stage"]="complete";
      }catch(const std::exception& error){row["error"]=error.what();++failures;}
      std::cout<<row.dump()<<'\n';report["cases"].push_back(std::move(row));save();
    }
  }
  check(index>0,"No zone preset selected");
  if(const auto chosen=std::getenv("HALO2_ZONE_VARIANT"))report["selected_variant"]=chosen;
  else check(index==(movement?5u:regions?15u:10u),"Authored zone preset coverage changed");
  report["passed"]=failures==0;save();return failures?1:0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
