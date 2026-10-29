#include "halo2/multiplayer_world.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/collision.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/online_postgame.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/native_player_profile.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
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
struct Files {
  std::filesystem::path path;
  explicit Files(unsigned index) {
    const auto scratch=std::getenv("HALO2_TEST_SCRATCH");check(scratch,"Combat flow needs owned scratch");
    path=std::filesystem::path(scratch)/("engine-combat-"+std::to_string(index));std::filesystem::create_directories(path);
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
  check(argc==4 || argc==5,"Usage: multiplayer_engine_combat_tu15_tests MCC_MAP VARIANTS REPORT [PRESET]");
  if(!std::filesystem::is_regular_file(argv[1]))return 77;
  Json fixture;std::ifstream(argv[2])>>fixture;
  const bool engine_event_flow=std::getenv("HALO2_COMBAT_ENGINE_EVENT_WIN_FLOW");
  const bool accuracy_flow=std::getenv("HALO2_COMBAT_ACCURACY_FLOW");
  const bool winning_flow=engine_event_flow || accuracy_flow || std::getenv("HALO2_COMBAT_WIN_FLOW");
  Json report={{"passed",false},{"cases",Json::array()},
    {"scope","Canonical source-born rosters, current units and original weapons receive ordinary aim/fire controls. Only initial positions use B95D0 and authored clear spawn pairs. No vitality, damage, credit, death, score, timer or callback result is supplied. This is bounded fatal-hit integration, not full matches, rendered/audio or network acceptance."}};
  const auto save=[&]{std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';check(bool(out),"Cannot retain engine combat report");};
  report["winning_flow"]=winning_flow;
  if(winning_flow)report["scope"]="Canonical original role holder receives ordinary aim/fire controls, with only score-limit parameter2 edited to one. The fatal hit must award its role point, select the original winner and complete real retained results/countdown/fade. B95D0 initial positions remain explicit; other role presets, weapon families, edited rules, presentation and networking remain separate acceptance.";
  report["declared_engine_death_input"]=engine_event_flow;
  if(engine_event_flow)report["scope"]="All nine authored Slayer and five Juggernaut presets with only score-limit parameter2 edited to one. Actual whole15F8B0 receives its declared credited-death event input before original physical unit cleanup, as in the existing role-flow contract. Original engine scoring must select its winner and retain a win through postgame or a genuine continuing-round reset. This does not qualify upstream projectile/damage attribution or every weapon. Positions, killer/victim identities and event arguments are explicit inputs; no score, win, clock, timer, phase or completion result is supplied.";
  if(accuracy_flow)report["scope"]="Original MCC-backed Slayer with one-point score limit, ordinary firing and actual hit/death/completion. Exact source51EEF4 shot/hit counts are compared with retained statistics, terminal snapshot and published PCR. Declared local roster identities and authored initial positions are fixtures; no shot/hit counter, damage, death, score, clock or completion result is supplied. Internet/installed-client acceptance remains separate.";
  unsigned index{},failures{};
  for(const auto& preset:fixture.at("variants")) {
    const auto name=preset.at("name").get<std::string>();
    const auto preset_type=preset.at("source_type").get<unsigned>();
    if(engine_event_flow && preset_type!=2 && preset_type!=7)continue;
    if(argc==5) {if(name!=argv[4])continue;}
    else if(!engine_event_flow && name!="Slayer" && name!="Oddball" && name!="King" && name!="3 on 1" &&
            name!="Multi Flag CTF" && name!="Multi Bomb" && name!="3 Plots")continue;
    Json row={{"name",name},{"passed",false},{"stage","construct"}};
    try {
      Files files(index++);MultiplayerOptions options;options.map=argv[1];options.source_object_world=true;
      options.source_object_savegame_directory=files.path;options.variant={name,preset.at("frontend_engine").get<unsigned>(),{}};
      for(const auto& w:preset.at("writes"))options.variant.values.append(w[0].get<unsigned>(),w[1].get<int>());
      if(winning_flow) {
        if(!engine_event_flow)check(accuracy_flow?name=="Slayer":name=="3 on 1","Winning combat scope requires its declared original preset");
        const auto authored=source_game_variant(options.variant);options.variant.values.append(2,1);
        const auto edited=source_game_variant(options.variant);check(source_game_variant_valid(edited),"Original one-point role edit is invalid");
        const auto descriptor=source_game_variant_descriptors()[2];
        for(unsigned n=0;n<edited.bytes.size();++n)if(n<descriptor.offset || n>=descriptor.offset+4)
          check(edited.bytes[n]==authored.bytes[n],"Role score edit changed an unrelated authored option");
      }
      const auto variant=source_game_variant(options.variant);const auto type=source_game_variant_type(variant);
      options.local_players=type==7?4:2;
      if(source_game_variant_get(variant,91))for(unsigned n=0;n<options.local_players;++n)options.selected_teams.push_back(n%2);
      if(accuracy_flow) {
        std::array<std::uint8_t,9> appearance{};
        for(unsigned n=0;n<options.local_players;++n)
          options.selected_profiles.push_back(native_selected_player_configuration("Accuracy "+std::to_string(n+1),appearance,-1));
      }
      MultiplayerPreparedWorld prepared(options);auto& match=prepared.match;auto& objects=*prepared.source_objects;
      auto& world=objects.tu15_world();auto& memory=world.memory();
      check(prepared.rules.requires_source_engine && objects.uses_scenario_player_spawns(prepared.cache->spawns()),
          "Combat flow selected a bounded or declared-placement adapter");
      const auto engine=Bytes(memory.view(0x4f63b8u,4)).u32(0);
      unsigned shooter=prepared.local_slots[0],victim=prepared.local_slots[1];
      std::unique_ptr<LanHost> accuracy_host;
      std::unique_ptr<LanClient> accuracy_peer;
      std::array<Controls,multiplayer_capacity> admission_controls{};
      std::array<Controls,1> remote_controls{};
      if(accuracy_flow) {
        accuracy_host=std::make_unique<LanHost>(match,0,0x6163637572616379ull,options.variant);
        std::array<std::uint8_t,9> appearance{};
        const std::array<SourcePlayerConfiguration,1> remote_profiles{native_selected_player_configuration("Accuracy peer",appearance,-1)};
        accuracy_peer=std::make_unique<LanClient>(accuracy_host->join_address(),0x6163637572616379ull,1,
            std::span<const std::int8_t>{},remote_profiles);
        for(unsigned n=0;n<80 && !accuracy_peer->ready() && accuracy_peer->error().empty();++n) {
          accuracy_peer->send(remote_controls);accuracy_host->receive(admission_controls);
          accuracy_host->broadcast();accuracy_peer->receive();
        }
        check(accuracy_peer->ready() && accuracy_peer->error().empty(),"Accuracy UDP participant failed admission");
      }
      if(type==7) {
        const auto mask=Bytes(memory.view(engine+0xfcu,2)).u16(0);check(std::popcount(mask)==1,"Combat role has no unique holder");
        if(winning_flow) {
          shooter=std::countr_zero(mask);for(const auto slot:prepared.local_slots)
            if(slot!=shooter && match.player(slot).source_current_object!=null_datum){victim=slot;break;}
        }else {victim=std::countr_zero(mask);for(const auto slot:prepared.local_slots)if(slot!=victim){shooter=slot;break;}}
      }
      const auto shooter_unit=match.player(shooter).source_current_object,victim_unit=match.player(victim).source_current_object;
      const auto shooter_lives=match.player(shooter).source_lives_remaining,victim_lives=match.player(victim).source_lives_remaining;
      check(shooter_unit!=null_datum && victim_unit!=null_datum,"Combat participants lack genuine current units");
      check(world.multiplayer_engine_state().recording.enabled && !world.multiplayer_engine_state().recording.frozen,
          "Combat session omitted original recording startup");
      CollisionWorld collision(prepared.content.collision);std::array<Spawn,2> selected{};
      float closest=std::numeric_limits<float>::infinity();
      for(const auto& a:prepared.cache->spawns())for(const auto& b:prepared.cache->spawns()) {
        const auto separation=length(b.position-a.position);if(separation<2 || separation>8 || separation>=closest)continue;
        const auto eye=a.position+Vec3{0,0,prepared.content.player.camera_height};
        const auto delta=b.position+Vec3{0,0,prepared.content.player.camera_height}-eye;
        if(collision.raycast(eye,delta/length(delta),length(delta)))continue;
        selected={a,b};closest=separation;
      }
      check(std::isfinite(closest),"Combat map lacks a declared clear authored spawn pair");
      objects.tu15_world().set_object_position(shooter_unit,selected[0].position);
      objects.tu15_world().set_object_position(victim_unit,selected[1].position);
      for(const auto slot:prepared.local_slots)if(slot!=shooter && slot!=victim) {
        const auto unit=match.player(slot).source_current_object;
        if(unit!=null_datum)world.set_object_position(unit,prepared.cache->spawns().back().position);
      }
      unsigned attribution_stores{},engine_death_stores{};
      objects.observe_physics_stores([&](auto pc,const auto&) {
        if(pc>=0x1f2280u && pc<0x1f2888u)++attribution_stores;
        if(pc>=0x15f8b0u && pc<0x1600f4u)++engine_death_stores;
      });Observation observation{objects};
      std::array<Controls,multiplayer_capacity> controls{};
      for(const auto slot:prepared.local_slots){controls[slot].yaw=match.unit(slot).yaw;controls[slot].pitch=match.unit(slot).pitch;}
      const auto aim=[&] {
        const auto current=match.player(shooter).source_current_object;if(current==null_datum)return;
        const auto look=objects.player_look(match.player(shooter).source_datum,current);
        if(!look)return; // First delivery constructs the actual source look owner.
        const auto& from=match.unit(shooter);const auto& to=match.unit(victim);
        const auto eye=ordinary_unit_eye(from.body.position,from.definition.camera_height,from.definition.crouch_camera_height,from.crouch);
        const auto delta=to.body.position+Vec3{0,0,to.body.height*.5f}-eye;
        // The PC ABI carries accumulated raw look;18A300 owns orientation.
        // Steer using that completed source view, retaining the accumulator
        // just as the ordinary runtime does across teleport/seat rebasing.
        controls[shooter].yaw+=std::remainder(std::atan2(delta.y,delta.x)-look->x,Bytes(memory.view(0x44923cu,4)).f32(0));
        controls[shooter].pitch+=std::atan2(delta.z,std::hypot(delta.x,delta.y))-look->y;
      };
      const auto step=[&] {
        if(accuracy_peer){accuracy_peer->send(remote_controls);accuracy_host->receive(controls);}
        aim();match.step(controls);const auto original=world.multiplayer_engine_state();
        if(accuracy_peer){accuracy_host->broadcast();accuracy_peer->receive();}
        check(std::memcmp(&original.statistics,&match.source_match_statistics(),sizeof(original.statistics))==0,
            "Combat flow advanced a parallel native score/stat bank");
      };
      row["stage"]="ordinary-warmup";for(unsigned n=0;n<100;++n)step();
      check(!match.unit(shooter).vitality.dead && !match.unit(victim).vitality.dead,"Combat warmup lost a participant");
      row["before"]={{"tick",match.state().tick},{"loaded",match.unit(shooter).weapon.loaded},
          {"victim_body",match.unit(victim).vitality.body},{"victim_shield",match.unit(victim).vitality.shield},
          {"shooter_unit",shooter_unit},{"victim_unit",victim_unit}};
      const auto before_kills=match.player(shooter).kills,before_deaths=match.player(victim).deaths;
      const auto point=[](Vec3 p){return Json::array({p.x,p.y,p.z});};
      row["aim_geometry"]=Json::array();
      for(const auto slot:{shooter,victim}) {
        const auto& u=match.unit(slot);const auto body=objects.object_body(match.player(slot).source_current_object);
        const auto source_aim=Bytes(memory.view(body+0x168u,12));
        row["aim_geometry"].push_back({{"slot",slot},{"position",point(u.body.position)},{"height",u.body.height},
            {"eye",point(ordinary_unit_eye(u.body.position,u.definition.camera_height,u.definition.crouch_camera_height,u.crouch))},
            {"yaw",u.yaw},{"pitch",u.pitch},{"source_aim",{source_aim.f32(0),source_aim.f32(4),source_aim.f32(8)}}});
      }
      row["projectiles"]=Json::array();
      row["stage"]=engine_event_flow?"declared-original-engine-death-input":"ordinary-fired-hit";
      unsigned frames{};controls[shooter].fire=!engine_event_flow;
      if(engine_event_flow) {
        world.call_actual(0x15f8b0u,std::array{match.player(shooter).source_datum,match.player(victim).source_datum,0u,63u});
        objects.kill_biped(victim_unit);step();++frames;
      }
      for(;!engine_event_flow && frames<240 && !match.unit(victim).vitality.dead && !match.finished();++frames) {
        step();
        if(frames<16)for(const auto& object:objects.live_objects())if(object.type==5 && row["projectiles"].size()<24) {
          const auto t=objects.object_transform(object.object);
          row["projectiles"].push_back({{"frame",frames},{"object",object.object},{"definition",object.definition},
              {"position",point(t.position)},{"forward",point(t.basis[0])}});
        }
      }
      row["after"]={{"tick",match.state().tick},{"frames",frames},{"loaded",match.unit(shooter).weapon.loaded},
          {"victim_body",match.unit(victim).vitality.body},{"victim_shield",match.unit(victim).vitality.shield},
          {"dead",match.unit(victim).vitality.dead},{"kills",match.player(shooter).kills},{"deaths",match.player(victim).deaths},
          {"attribution_stores",attribution_stores},{"engine_death_stores",engine_death_stores},
          {"native_shots",match.simulation().shots()},{"native_hits",match.simulation().hits()}};
      report["in_progress"]=row;save();
      check(match.unit(victim).vitality.dead,"Declared source input did not complete actual unit death/cleanup");
      if(!engine_event_flow)check(match.player(shooter).kills==before_kills+1 && match.player(victim).deaths==before_deaths+1,
          "Fatal source hit did not reach original killer/victim statistics");
      check((engine_event_flow || attribution_stores) && engine_death_stores,"Fatal source input omitted its declared original attribution/event owner");
      if(!engine_event_flow)check(match.recorded_statistics() && match.recorded_statistics()->players[shooter][7]>=1 &&
          match.recorded_statistics()->players[victim][9]>=1,"Fatal source hit omitted original retained kill/death statistics");
      if(accuracy_flow) {
        const auto actual=source_result_accuracy(*match.recorded_statistics());
        std::uint32_t shots{},hits{};
        for(unsigned weapon=0;weapon<SourceResultAccuracy::weapon_count;++weapon) {
          const auto at=0x51eef4u+2*(shooter*437+78+(weapon+5)*8+4);
          const auto source=Bytes(memory.view(at,4));
          check(actual.players[shooter][weapon][0]==(source.u16(0)&0x7fff) &&
                actual.players[shooter][weapon][1]==(source.u16(2)&0x7fff),"Retained accuracy differs from actual source bank");
          shots+=actual.players[shooter][weapon][0];hits+=actual.players[shooter][weapon][1];
        }
        check(shots>0 && hits>0,"Ordinary native shots/hits did not reach original retained accuracy counters");
        row["original_accuracy"]={{"shots",shots},{"hits",hits}};
      }
      if(type==7)check(Bytes(memory.view(engine+0xfcu,2)).u16(0)==1u<<shooter,"Credited source hit did not transfer the actual role");
      if(winning_flow) {
        row["stage"]="original-role-winning-outcome";auto original=world.multiplayer_engine_state();auto state=match.state();
        controls[shooter].fire=false;
        for(unsigned n=0;n<8 && original.round.phase==1;++n){step();original=world.multiplayer_engine_state();state=match.state();}
        const bool teams=source_game_variant_get(variant,91)!=0;const auto kind=source_game_variant_get(variant,0);
        const auto team=match.player(shooter).source_membership;
        check((kind==0 || kind==5) && state.finished==(kind==0) && state.winner==(teams?-1:int(shooter)) &&
            state.winner_team==(teams?int(team):-1) && original.round.phase==2 && original.recording.frozen,
            "Fatal source input did not select its real original round winner/policy");
        check(original.statistics.players[shooter].fields[0]==1 && original.statistics.players[shooter].fields[7]==1 &&
            match.recorded_statistics()->players[shooter][6]==1,"Winning role hit lost its original score or live/retained win");
        if(teams)check(original.statistics.teams[unsigned(team)].fields[7]==1 && match.recorded_statistics()->teams[unsigned(team)][6]==1,
            "Winning source input lost its original live/retained team win");
        row["winning_outcome"]={{"tick",state.tick},{"winner",state.winner},{"winner_team",state.winner_team},
            {"score",state.players[shooter].score},{"round_kind",kind},{"finished",state.finished}};
        if(state.finished) {
          const auto completed=original.statistics;
          for(unsigned n=0;n<211;++n){step();original=world.multiplayer_engine_state();check(std::memcmp(&completed,&original.statistics,sizeof(completed))==0,
              "Winning postgame changed its completed score/win bank");}
          check(original.finalized && original.recording.frozen && original.game_over.complete() && original.game_over.fade_started,
              "Winning source match did not complete actual retained results/countdown/fade");
          row["winning_outcome"]["postgame_frames"]=211;
          if(accuracy_flow) {
            const auto terminal=match.state();
            check(terminal.result_accuracy && *terminal.result_accuracy==source_result_accuracy(*match.recorded_statistics()),
                  "Terminal snapshot lost its completed original accuracy bank");
            std::vector<OnlineAdmission> roster;
            for(unsigned slot=0;slot<multiplayer_capacity;++slot) {
              if(!match.player(slot).connected)continue;
              OnlineAdmission admitted{slot,match.player(slot).name,match.player(slot).source_membership};
              admitted.identity[0]=std::byte(slot+1);roster.push_back(admitted);
            }
            auto report=online_postgame_publish(match,options.variant,roster);
            online_postgame_validate(terminal,options.variant,roster,report.source);
            check(accuracy_peer->state().result_accuracy==terminal.result_accuracy,
                  "Original terminal accuracy changed across native UDP snapshot encoding/decoding");
            online_postgame_validate(accuracy_peer->state(),options.variant,roster,report.source);
            const auto at=0xecc+2*(shooter*437+5*8+4);
            report.source[at]^=std::byte{1};bool rejected{};
            try {online_postgame_validate(terminal,options.variant,roster,report.source);}
            catch(const FormatError&){rejected=true;}
            check(rejected,"Modified PCR shot counter escaped final-state comparison");
            report.source[at]^=std::byte{1};
            auto unavailable=terminal;unavailable.result_accuracy.reset();rejected=false;
            try {online_postgame_validate(unavailable,options.variant,roster,report.source);}
            catch(const FormatError&){rejected=true;}
            check(rejected,"Missing native accuracy observations became confirmed zero counters");
            row["accuracy_report_checked"]=true;
            row["terminal_accuracy_peer_checked"]=true;
          }
        }else {
          const auto end_tick=state.tick;
          for(unsigned n=0;n<180;++n){step();original=world.multiplayer_engine_state();state=match.state();}
          check(state.tick-end_tick==180 && original.round.phase==1 && original.round.index==1 && !original.recording.frozen &&
              original.statistics.players[shooter].fields[7]==1,"Winning source input lost its actual150/30 reset or accumulated win");
          row["winning_outcome"]["reset_tick"]=state.tick;unsigned factory_frames{};
          while(factory_frames<32 && (match.player(shooter).source_current_object==null_datum || match.player(victim).source_current_object==null_datum)) {
            step();++factory_frames;
          }
          check(match.player(shooter).source_current_object!=null_datum && match.player(shooter).source_current_object!=shooter_unit &&
              match.player(victim).source_current_object!=null_datum && match.player(victim).source_current_object!=victim_unit,
              "Winning source reset failed to replace its genuine current units");
          check(match.player(shooter).source_lives_remaining==shooter_lives && match.player(victim).source_lives_remaining==victim_lives,
              "Winning source reset did not restore its original initial lives");
          row["winning_outcome"]["factory_frames"]=factory_frames;
          row["winning_outcome"]["reset_lives"]={match.player(shooter).source_lives_remaining,match.player(victim).source_lives_remaining};
        }
      }
      row["tick"]=match.state().tick;row["passed"]=true;row["stage"]="complete";
    }catch(const std::exception& error){row["error"]=error.what();++failures;}
    std::cout<<row.dump()<<'\n';report["cases"].push_back(std::move(row));report.erase("in_progress");save();
  }
  check(index==(argc==5?1u:engine_event_flow?14u:7u),"Combat family coverage changed");report["passed"]=failures==0;save();return failures?1:0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
