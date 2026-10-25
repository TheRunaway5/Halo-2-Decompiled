#include "halo2/multiplayer_world.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_objective_native_tu15.hpp"
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
std::uint32_t word(const SourceRuntimeMemory& m,std::uint32_t p,unsigned size=4) {
  const auto bytes=m.view(p,size);std::uint32_t value{};
  for(unsigned n=0;n<size;++n)value|=std::to_integer<std::uint32_t>(bytes[n])<<(8*n);
  return value;
}
template<class Address>std::int16_t half(const SourceRuntimeMemory& m,Address p){return std::bit_cast<std::int16_t>(Bytes(m.view(p,2)).u16(0));}
struct Files {
  std::filesystem::path path;
  explicit Files(unsigned n) {
    const auto scratch=std::getenv("HALO2_TEST_SCRATCH");check(scratch,"Objective flow test needs owned scratch");
    path=std::filesystem::path(scratch)/("objective-flow-"+std::to_string(n));std::filesystem::create_directories(path);
  }
  ~Files(){std::error_code error;std::filesystem::remove_all(path,error);}
};
}
int main(int argc,char** argv)try {
  std::cout<<std::unitbuf;
  if(argc!=4)throw FormatError("Usage: multiplayer_objective_flow_tu15_tests MCC_MAP VARIANT_FIXTURE REPORT");
  if(!std::filesystem::is_regular_file(argv[1]))return 77;
  Json fixture;std::ifstream(argv[2])>>fixture;
  Json report={{"passed",false},{"cases",Json::array()},
    {"scope","Actual canonical player/objective birth and normal delivered pickup controls. Initial placement inputs use originalB95D0; no inventory, target, pickup, timer or score result is supplied. Full played approach, objective match completion, rendering and replication remain separate acceptance."}};
  const auto save=[&]{std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';check(bool(out),"Cannot retain objective flow report");};
  const bool win_flow=std::getenv("HALO2_OBJECTIVE_WIN_FLOW");
  const bool overtime_flow=std::getenv("HALO2_OBJECTIVE_OVERTIME_FLOW");
  const bool disable_overtime=std::getenv("HALO2_OBJECTIVE_OVERTIME_DISABLED");
  const bool return_flow=std::getenv("HALO2_OBJECTIVE_RETURN_FLOW");
  const bool short_return=std::getenv("HALO2_OBJECTIVE_SHORT_RETURN");
  const bool touch_return=std::getenv("HALO2_OBJECTIVE_TOUCH_RETURN");
  check((!short_return && !touch_return) || return_flow,"Edited return inputs require the actual return cohort");
  const bool arming_decay=std::getenv("HALO2_OBJECTIVE_ARMING_DECAY");
  check(!return_flow || (!win_flow && !overtime_flow && !arming_decay),"Objective return cohort needs its own world");
  check(!win_flow || !overtime_flow,"Objective win and overtime cohorts are independent");
  check(!disable_overtime || overtime_flow,"Disabled overtime requires its objective time-limit cohort");
  const bool goal_flow=win_flow || arming_decay || std::getenv("HALO2_OBJECTIVE_GOAL_FLOW");
  report["goal_flow"]=goal_flow;report["winning_flow"]=win_flow;
  report["overtime_flow"]=overtime_flow;
  report["overtime_disabled_edit"]=disable_overtime;
  report["return_flow"]=return_flow;report["arming_decay"]=arming_decay;
  report["short_return_edit"]=short_return;report["touch_return_edit"]=touch_return;
  if(return_flow)report["scope"]="All nine unchanged authored CTF/Assault presets: real objective birth, illegal-side pickup rejection, normal pickup/carry/trigger drop and unattended timed home return through ordinary full frames. Initial/away player positions use B95D0. No objective position, possession, flag, timer, score or return result is supplied. Played routes, touch-return edits, presentation and network actions remain separate.";
  if(short_return)report["scope"]="All nine CTF/Assault presets with only the original return interval set to one second. Actual pickup, trigger drop, unattended countdown and authored home placement execute in complete frames. Player positions are declared B95D0 inputs; no objective, timer, score or return result is supplied.";
  if(touch_return)report["scope"]="All nine CTF/Assault presets with only the original touch-return bit enabled. Actual pickup and trigger drop precede eligible player contact, home placement and retained CTF return awards. The original long return interval excludes unattended expiry. Neutral-index objectives keep their original touch exclusion and complete unattended return. Player positions are declared B95D0 inputs; no objective, timer, score or return result is supplied.";
  if(arming_decay)report["scope"]="All five unchanged authored Assault presets: normal pickup, partial arming at an actual enemy goal, three ticks outside the goal with exact original three-per-tick progress decay, then resumed arming, plant, fuse and one score. Only player positions are explicit B95D0 inputs; no objective, progress, timer, score or phase result is supplied.";
  if(overtime_flow)report["scope"]="All nine authored CTF/Assault presets with only original time-limit parameter10 edited to five seconds. Ordinary possession must exercise the authored overtime policy; enabled cases drop through normal trigger input and move actual players to a distant authored spawn, then let the original eligibility/countdown end the round. B95D0 position inputs are explicit. No possession, overtime flag, timer, clock, score, winner, phase or completion result is supplied. Physical played approaches, complete return variants, announcement consumer delivery and network acceptance remain separate.";
  if(win_flow)report["scope"]="All15 authored objective presets with only original score-limit parameter2 edited to one. Ordinary pickup/possession/capture/arming/plant/fuse selects the actual original winner. Single-round games must finish and continuing rules must reset through ordinary frames with replacement units. B95D0 initial/goal positions remain declared inputs; full played approaches, other settings, presentation and network acceptance remain separate.";
  if(disable_overtime)report["scope"]="All nine authored CTF/Assault presets with only time-limit parameter10 edited to five seconds and original overtime bit parameter29/60 disabled. Byte audit preserves every other authored field. Normal possession must no longer extend the clock; the actual original tied ending and postgame or150/30 reset must complete. B95D0 initial pickup positions remain explicit; no timer, clock, eligibility, phase, score, winner or completion result is supplied.";
  unsigned index{},failures{};
  for(const auto& preset:fixture.at("variants")) {
    const auto type=preset.at("source_type").get<unsigned>();if(type!=1 && type!=3 && type!=9)continue;
    if(goal_flow && type==3 && !win_flow)continue;
    if(overtime_flow && type==3)continue;
    if(return_flow && type==3)continue;
    if(arming_decay && type!=9)continue;
    const auto name=preset.at("name").get<std::string>();
    if(const auto chosen=std::getenv("HALO2_OBJECTIVE_VARIANT");chosen && name!=chosen)continue;
    Json row={{"name",name},{"source_type",type},{"passed",false},{"stage","construct"}};
    try {
      source_objective_native_tu15_stats={};
      Files files(index++);MultiplayerOptions options;options.map=argv[1];options.local_players=2;
      options.source_object_world=true;options.source_object_savegame_directory=files.path;
      options.variant={name,preset.at("frontend_engine").get<unsigned>(),{}};
      for(const auto& w:preset.at("writes"))options.variant.values.append(w[0].get<unsigned>(),w[1].get<int>());
      if(return_flow && (short_return || touch_return)) {
        const auto authored=source_game_variant(options.variant);
        if(short_return)options.variant.values.append(type==1?54:61,1);
        if(touch_return)options.variant.values.append(type==1?52:92,1);
        const auto edited=source_game_variant(options.variant);
        check(source_game_variant_valid(edited),"Edited objective return fails the original checker");
        for(unsigned n=0;n<edited.bytes.size();++n) {
          if(short_return && n>=0xf4 && n<0xf8)continue;
          check(edited.bytes[n]==(touch_return && n==0xf0?(authored.bytes[n]|4u):authored.bytes[n]),
              "Objective return edit changed an unrelated authored setting");
        }
        row["return_option_edits"]={{"one_second",short_return},{"touch",touch_return}};
      }
      if(win_flow) {
        const auto authored=source_game_variant(options.variant);options.variant.values.append(2,1);
        const auto edited=source_game_variant(options.variant);check(source_game_variant_valid(edited),"Original one-point objective edit is invalid");
        const auto descriptor=source_game_variant_descriptors()[2];
        for(unsigned n=0;n<edited.bytes.size();++n)if(n<descriptor.offset || n>=descriptor.offset+4)
          check(edited.bytes[n]==authored.bytes[n],"Objective score edit changed an unrelated authored setting");
      }
      if(overtime_flow) {
        const auto authored=source_game_variant(options.variant);options.variant.values.append(10,5);
        const auto overtime_parameter=type==1?29u:60u;
        if(disable_overtime)options.variant.values.append(overtime_parameter,0);
        const auto edited=source_game_variant(options.variant);check(source_game_variant_valid(edited),"Original five-second objective edit is invalid");
        const auto descriptor=source_game_variant_descriptors()[10];
        const auto overtime_descriptor=source_game_variant_descriptors()[overtime_parameter];
        check(overtime_descriptor.kind==1 && overtime_descriptor.offset==0xf0 && overtime_descriptor.operand==1,
            "Original objective overtime descriptor differs from its actual flag consumer");
        for(unsigned n=0;n<edited.bytes.size();++n)if(n<descriptor.offset || n>=descriptor.offset+4)
          check(edited.bytes[n]==(disable_overtime && n==overtime_descriptor.offset?
              (authored.bytes[n]&~2u):authored.bytes[n]),"Objective time/overtime edit changed an unrelated authored setting");
      }
      const auto variant=source_game_variant(options.variant);const bool teams=source_game_variant_get(variant,91)!=0;
      if(teams)options.selected_teams={0,1};
      MultiplayerPreparedWorld prepared(options);auto& match=prepared.match;auto& objects=*prepared.source_objects;
      auto& world=objects.tu15_world();auto& memory=world.memory();
      check(prepared.rules.requires_source_engine,"Objective flow selected a bounded adapter");
      check(world.multiplayer_engine_state().recording.enabled && !world.multiplayer_engine_state().recording.frozen,
          "Objective session skipped original retained-result startup");
      const auto retained=[&]() -> const SourceResultStatistics& {
        const auto stats=match.recorded_statistics();
        check(stats && std::memcmp(stats,memory.view(0x51eef4u,sizeof(SourceResultStatistics)).data(),sizeof(SourceResultStatistics))==0,
            "Objective flow retained a separate native result bank");
        return *stats;
      };
      const auto engine=word(memory,0x4f63b8u);auto count=word(memory,engine+0xc18u);
      row["objective_birth_frames"]=Json::array({{{"tick",match.state().tick},{"count",count}}});
      for(unsigned n=0;n<8 && !count;++n) {
        match.step({});count=word(memory,engine+0xc18u);
        row["objective_birth_frames"].push_back({{"tick",match.state().tick},{"count",count}});
      }
      if(!count && type!=3) {
        const auto bank=word(memory,0x52d710u),scenario=word(memory,0x4ecc20u);
        row["empty_registry"]={{"bank",bank},{"target_team",std::bit_cast<std::int32_t>(word(memory,bank+0x1f4u))},
            {"round",world.multiplayer_engine_state().round.index},{"marker_count",word(memory,scenario+0x118u)}};
        row["empty_registry"]["spawn_markers"]=Json::array();
        for(unsigned team=0;team<9;++team)row["empty_registry"]["spawn_markers"].push_back(
            half(memory,bank+2u+team*2u));
      }
      check(count>0 && count<=8,"Original objective registry is empty or exceeds capacity");
      std::vector<std::uint32_t> items;row["items"]=Json::array();
      for(unsigned n=0;n<count;++n) {
        const auto item=word(memory,engine+0xc1cu+n*16u);items.push_back(item);
        const auto* tag=prepared.cache->find_tag(TagHandle{objects.object_definition(item)});
        check(tag && tag->group=="weap","Original objective has no real weapon tag");
        row["items"].push_back({{"object",item},{"definition",tag->name}});
      }
      std::array<Vec3,multiplayer_capacity> starts{};
      std::array<Controls,multiplayer_capacity> controls{};
      for(const auto slot:prepared.local_slots) {
        starts[slot]=objects.biped_motion(match.player(slot).source_current_object).position;
        controls[slot].yaw=match.unit(slot).yaw;controls[slot].pitch=match.unit(slot).pitch;
      }
      row["attempts"]=Json::array();std::uint32_t carried=null_datum;unsigned carrier{};
      const auto carrier_item=[&](unsigned slot) {
        // Exact registered carrier contracts: Oddball queries subtype2;
        // the shared CTF/Assault callback queries subtype1.
        auto a=world.prepare_source_call(std::array{type==3?2u:1u});a.gp[0]=match.player(slot).source_datum;
        world.run_actual(0x1a8fd0u,a);return a.gp[0];
      };
      if(return_flow) {
        row["illegal_side_pickups"]=Json::array();
        for(const auto item:items) {
          const auto owner=half(memory,objects.object_body(item)+0x17eu);
          check(owner>=0 && owner<9,"Original objective has no team/neutral index");
          for(const auto slot:prepared.local_slots) {
            const auto team=match.player(slot).source_membership;
            if(owner==8 || (type==1?team!=owner:team==owner))continue;
            auto position=objects.object_transform(item).position;position.x-=.2f;
            world.set_object_position(match.player(slot).source_current_object,position);
            controls[slot].yaw=0;controls[slot].pitch=0;
            for(unsigned n=0;n<20;++n) {
              controls[slot].pickup=n>=2;
              controls[slot].pickup_milliseconds=controls[slot].pickup?std::uint16_t((n-2)*1000/30):0;
              match.step(controls);
              check(carrier_item(slot)==null_datum,"Original objective admitted an illegal-side carrier");
            }
            controls[slot].pickup=false;controls[slot].pickup_milliseconds=0;
            world.set_object_position(match.player(slot).source_current_object,starts[slot]);
            row["illegal_side_pickups"].push_back({{"object",item},{"objective_index",owner},{"slot",slot},{"team",team},{"frames",20}});
          }
        }
      }
      for(const auto item:items)for(const auto slot:prepared.local_slots) {
        if(carried!=null_datum)continue;
        row["stage"]="normal-pickup-command";
        const auto target=objects.object_transform(item).position;
        for(const auto other:prepared.local_slots)if(other!=slot)
          world.set_object_position(match.player(other).source_current_object,starts[other]);
        auto position=target;position.x-=.2f;
        world.set_object_position(match.player(slot).source_current_object,position);
        controls[slot].yaw=0;controls[slot].pitch=0;
        Json attempt={{"slot",slot},{"object",item},{"frames",Json::array()}};bool admitted{};
        for(unsigned n=0;n<64 && carried==null_datum;++n) {
          controls[slot].pickup=n>=2 && !admitted;
          controls[slot].pickup_milliseconds=controls[slot].pickup?std::uint16_t((n-2)*1000/30):0;
          match.step(controls);const auto state=match.state();const auto queried=carrier_item(slot);
          const auto source_unit=match.player(slot).source_current_object;
          const auto actual_equipment=world.player_equipment(source_unit);
          const auto parent=objects.object_parent(item);
          admitted=admitted || queried!=null_datum;
          if(queried!=null_datum && parent==source_unit && state.players[slot].source_weapon==queried &&
              std::ranges::find(actual_equipment.weapons,queried)!=actual_equipment.weapons.end())carried=queried;
          const auto& prompt=state.players[slot].action_prompt;
          if(n<4 || n%10==0 || queried!=null_datum)attempt["frames"].push_back({{"n",n},{"tick",state.tick},
            {"target_action",prompt.target.action},{"target_object",prompt.target.object},
            {"held",state.players[slot].source_weapon},{"carried_query",queried},{"item_parent",parent},
            {"unit",source_unit},{"slots",actual_equipment.weapons},{"current",actual_equipment.current},
            {"requested",actual_equipment.requested}});
          if(carried!=null_datum)carrier=slot;
        }
        controls[slot].pickup=false;controls[slot].pickup_milliseconds=0;
        row["attempts"].push_back(std::move(attempt));
      }
      check(carried!=null_datum && std::ranges::find(items,carried)!=items.end(),
          "Normal delivered commands did not acquire a genuinely registered objective");
      const auto unit=match.player(carrier).source_current_object;
      const auto objective_index=half(memory,objects.object_body(carried)+0x17eu);
      Vec3 return_home{};int home_marker=-1;
      if(return_flow) {
        check(objective_index>=0 && objective_index<9,"Actual held objective index exceeds its original bank");
        auto home=world.prepare_source_call(std::array<std::uint32_t,0>{});home.gp[0]=unsigned(objective_index);
        world.run_actual(0x24b4c0u,home);home_marker=std::bit_cast<std::int32_t>(home.gp[0]);
        const auto scenario=word(memory,0x4ecc20u);
        check(home_marker>=0 && unsigned(home_marker)<word(memory,scenario+0x118u),"Timed return has no actual authored home");
        const auto marker=Bytes(memory.view(word(memory,scenario+0x11cu)+unsigned(home_marker)*32u,12));
        return_home={marker.f32(0),marker.f32(4),marker.f32(8)};
        Vec3 away{};float distance{};
        for(const auto& spawn:prepared.cache->spawns())if(length(spawn.position-return_home)>distance) {
          away=spawn.position;distance=length(away-return_home);
        }
        check(distance>8.f,"Timed return needs an authored player start away from home");
        world.set_object_position(unit,away);
      }
      std::array<std::uint32_t,multiplayer_capacity> original_units{};
      for(const auto slot:prepared.local_slots)original_units[slot]=match.player(slot).source_current_object;
      const auto equipment=world.player_equipment(unit);
      check(objects.object_parent(carried)==unit && std::ranges::find(equipment.weapons,carried)!=equipment.weapons.end(),
          "Original carried item is absent from its actual parent/inventory");
      auto state=match.state();
      if(type==1) {
        const auto grabs=retained().players[carrier][45+18]&0x7fffu;
        check(grabs==1,"Original completed CTF pickup did not retain exactly one flag-grab award");
        row["recorded_flag_grabs"]=grabs;
      }
      check(state.players[carrier].source_weapon==carried,"Native state did not adopt the actual held objective");
      check(std::ranges::none_of(state.world_weapons,[&](const auto& w){return w.handle==carried;}),
          "Attached objective remained published as a loose world weapon");
      row["carrier_slot"]=carrier;row["carried_object"]=carried;row["native_definition"]=state.players[carrier].weapon;
      row["stage"]="ordinary-carrier-hold";
      const auto before_score=state.players[carrier].score;
      const auto rate=Bytes(memory.view(word(memory,0x51df54u)+2u,2)).u16(0);
      check(rate>0 && rate<=120,"Original objective tick rate is unavailable");
      unsigned held_frames{};
      const auto hold_budget=overtime_flow?rate*6u:goal_flow && type!=3?0u:rate*2u;
      for(;held_frames<hold_budget && !state.finished &&
          (!(win_flow || overtime_flow) || world.multiplayer_engine_state().round.phase==1);++held_frames) {
        match.step(controls);state=match.state();const auto original=world.multiplayer_engine_state();
        check(std::memcmp(&original.statistics,&match.source_match_statistics(),sizeof(original.statistics))==0,
            "Carrier flow advanced a separate native score/stat bank");
        (void)retained();
        if(!state.finished && (!(win_flow || overtime_flow) || original.round.phase==1))check(carrier_item(carrier)==carried && objects.object_parent(carried)==unit &&
            state.players[carrier].source_weapon==carried,"Original possession or native held-item binding was lost during ordinary frames");
      }
      row["holding"]={{"frames",held_frames},{"tick_rate",rate},{"before_score",before_score},
          {"after_score",state.players[carrier].score},{"finished",state.finished}};
      if(overtime_flow) {
        const bool enabled=(word(memory,word(memory,0x4f3218u)+0x22cu,1)&2u)!=0;
        if(disable_overtime)check(!enabled,"Actual original overtime consumer ignored its disabled variant flag");
        const auto bank=word(memory,0x52d710u);const auto original=world.multiplayer_engine_state();
        row["overtime"]={{"enabled",enabled},{"eligible",word(memory,bank+0x1fcu,1)},
            {"extension",word(memory,bank+0x200u)},{"clock",original.round.clock_seconds},
            {"elapsed_ticks",state.tick-original.round.start_tick},
            {"announcement_latch",word(memory,0x56e224u,1)}};
        if(enabled)check(!state.finished && original.round.phase==1 && state.tick-original.round.start_tick>rate*5u && original.round.clock_seconds==5 &&
            word(memory,bank+0x1fcu,1)==1 && word(memory,bank+0x200u)==rate*5u && word(memory,0x56e224u,1)==1,
            "Original carried objective failed its authored five-second overtime extension/latch");
        else check(original.round.phase==2 && !word(memory,bank+0x200u),"Disabled authored overtime did not expire naturally");
      }
      if(type==3)check(state.players[carrier].score!=before_score || state.finished,
          "Actual held Oddball did not reach its original periodic score owner");
      if(goal_flow && type!=3) {
        row["stage"]="ordinary-authored-goal";
        const auto membership=match.player(carrier).source_membership;
        check(membership>=0 && membership<8,"Objective carrier has no actual team membership");
        const auto bank=word(memory,0x52d710u),scenario=word(memory,0x4ecc20u);
        const auto markers=word(memory,scenario+0x11cu),marker_count=word(memory,scenario+0x118u);
        int goal=-1,goal_team=-1;
        for(unsigned team=0;team<8 && goal==-1;++team) {
          if(type==1?int(team)!=membership:int(team)==membership)continue;
          if(std::ranges::none_of(prepared.local_slots,[&](auto slot){return match.player(slot).source_membership==int(team);}))continue;
          auto mapping=world.prepare_source_call(std::array{team});world.run_actual(0x15b3b0u,mapping);
          const auto mapped=std::bit_cast<std::int32_t>(mapping.gp[0]);if(mapped<0 || mapped>=9)continue;
          const auto selected=half(memory,bank+0x14u+unsigned(mapped)*2u);
          if(selected>=0){goal=selected;goal_team=int(team);}
        }
        check(goal>=0 && unsigned(goal)<marker_count,"Original engine did not select an authored capture/plant marker");
        const auto marker=Bytes(memory.view(markers+unsigned(goal)*0x20u,0x20));
        const Vec3 position{marker.f32(0),marker.f32(4),marker.f32(8)};
        const auto objective=half(memory,objects.object_body(carried)+0x17eu);
        check(objective>=0 && objective<9,"Held original objective index is unavailable");
        const auto arm=half(memory,word(memory,0x4f3218u)+0x244u);
        const auto fuse=half(memory,word(memory,0x4f3218u)+0x246u);
        const auto before_team=state.team_scores[unsigned(membership)];
        row["goal"]={{"marker",goal},{"team",goal_team},{"position",{position.x,position.y,position.z}},
            {"objective_index",objective},{"arm_seconds",arm},{"fuse_seconds",fuse},{"before_team_score",before_team}};
        world.set_object_position(unit,position);
        if(arming_decay) {
          const auto progress_at=bank+0x1a0u+unsigned(objective)*2u;
          const auto flags_at=bank+0x170u+unsigned(objective);
          check(arm>0,"Authored Assault arming interval is not positive");
          const auto partial=unsigned(arm)*rate/2u;
          for(unsigned n=0;n<partial;++n)match.step(controls);
          auto previous=half(memory,progress_at);
          check(previous>0 && previous<int(unsigned(arm)*rate) && !(word(memory,flags_at,1)&10u),
              "Partial Assault arming already planted or failed to accumulate progress");
          Vec3 away{};float distance{};
          for(const auto& spawn:prepared.cache->spawns())if(length(spawn.position-position)>distance) {
            away=spawn.position;distance=length(away-position);
          }
          check(distance>8.f,"Arming decay needs an authored start outside the goal");
          world.set_object_position(unit,away);
          row["arming_decay"]={{"partial_frames",partial},{"initial_progress",previous},{"samples",Json::array()}};
          for(unsigned n=0;n<3;++n) {
            match.step(controls);const auto progress=half(memory,progress_at);
            row["arming_decay"]["samples"].push_back(progress);
            check(progress==std::max(-1,int(previous)-3),"Leaving the Assault goal did not apply original three-tick arming decay");
            check(!(word(memory,flags_at,1)&10u) && carrier_item(carrier)==carried,
                "Interrupted Assault arming planted or released its actual bomb");
            previous=progress;
          }
          world.set_object_position(unit,position);
        }
        bool armed{},placed{},scored{};auto& frames=row["goal"]["frames"];frames=Json::array();
        const auto budget=type==1?32u:rate*(unsigned(std::max(0,int(arm)))+unsigned(fuse?std::max(0,int(fuse)):10)+2u)+32u;
        for(unsigned n=0;n<budget && !scored && !state.finished;++n) {
          row["goal"]["delivered_frames"]=n+1;
          match.step(controls);state=match.state();const auto original=world.multiplayer_engine_state();
          check(std::memcmp(&original.statistics,&match.source_match_statistics(),sizeof(original.statistics))==0,
              "Authored goal advanced a separate native score/stat bank");
          const auto flags=std::to_integer<unsigned>(memory.view(bank+0x170u+unsigned(objective),1)[0]);
          const auto progress=half(memory,bank+0x1a0u+unsigned(objective)*2u);
          const bool next_armed=flags&8u,next_placed=flags&2u;
          if(type==9 && next_placed && !placed) {
            const auto after=world.player_equipment(unit);
            row["goal"]["plant_parent"]=objects.object_parent(carried);
            row["goal"]["plant_inventory"]=after.weapons;
            check(objects.object_parent(carried)==null_datum &&
                std::ranges::find(after.weapons,carried)==after.weapons.end(),
                "Original planted bomb remains in its carrier parent/inventory");
          }
          scored=state.team_scores[unsigned(membership)]!=before_team;
          if(n<3 || n%30==0 || next_armed!=armed || next_placed!=placed || scored)
            frames.push_back({{"n",n},{"tick",state.tick},{"flags",flags},{"arm_ticks",progress},
                {"team_score",state.team_scores[unsigned(membership)]},{"position",{state.players[carrier].position.x,state.players[carrier].position.y,state.players[carrier].position.z}}});
          armed=armed || next_armed;placed=placed || next_placed;
        }
        row["goal"]["armed"]=armed;row["goal"]["placed"]=placed;
        row["goal"]["scored"]=scored;row["goal"]["after_team_score"]=state.team_scores[unsigned(membership)];
        if(type==9)check(armed && placed,"Normal bomb carrier did not arm and place at the actual enemy goal");
        check(scored,"Normal authored objective goal did not reach the original scoring owner");
        check(state.team_scores[unsigned(membership)]==before_team+1,"One authored objective goal awarded an unexpected score");
        if(type==9) {
          const auto plants=retained().players[carrier][45+21]&0x7fffu;
          check(plants==1,"Original completed bomb plant did not retain exactly one planted-bomb award");
          row["recorded_bomb_plants"]=plants;
        }
        if(type==1) {
          const auto after=world.player_equipment(unit);
          row["goal"]["capture_parent"]=objects.object_parent(carried);
          row["goal"]["capture_inventory"]=after.weapons;
          row["goal"]["capture_carrier_query"]=carrier_item(carrier);
          check(carrier_item(carrier)==null_datum && objects.object_parent(carried)==null_datum &&
              std::ranges::find(after.weapons,carried)==after.weapons.end(),
              "Original captured flag remains in its carrier parent/inventory");
        }
      }else if(!state.finished && !win_flow && (!overtime_flow || world.multiplayer_engine_state().round.phase==1)) {
        row["stage"]="ordinary-objective-release";bool detached{};
        row["release_frames"]=Json::array();
        for(unsigned n=0;n<16 && !detached;++n) {
          // Original152957..1529C8 restores the current requested selection
          // for forced items, then primary fire/grenade calls1546B0(CL=1).
          // Inventory cycling intentionally keeps the objective selected.
          controls[carrier].fire=n==0;match.step(controls);state=match.state();
          const auto equipment_after=world.player_equipment(unit);
          detached=carrier_item(carrier)==null_datum && objects.object_parent(carried)==null_datum &&
              std::ranges::find(equipment_after.weapons,carried)==equipment_after.weapons.end();
          row["release_frames"].push_back({{"n",n},{"current",equipment_after.current},{"requested",equipment_after.requested},
              {"slots",equipment_after.weapons},{"item_parent",objects.object_parent(carried)},
              {"carrier_query",carrier_item(carrier)},{"held",state.players[carrier].source_weapon}});
          if(detached)check(std::ranges::any_of(state.world_weapons,[&](const auto& w){return w.handle==carried;}),
              "Actually detached objective is absent from loose world publication");
        }
        controls[carrier].fire=false;
        check(detached,"Normal primary-trigger input did not detach the actual carried objective");
        row["detached"]=true;
      }else if(!win_flow)row["release_scope"]="Objective already ended the round; active-round release remains unqualified in this case";
      if(return_flow) {
        row["stage"]="ordinary-unattended-home-return";
        check(row.value("detached",false),"Timed return requires an actual trigger drop");
        const auto bank=word(memory,0x52d710u);
        const auto dropped=objects.object_transform(carried).position;
        Vec3 far{};float distance{};
        for(const auto& spawn:prepared.cache->spawns())if(length(spawn.position-dropped)>distance) {
          far=spawn.position;distance=length(far-dropped);
        }
        check(distance>8.f,"Timed return needs players outside the original proximity region");
        for(const auto slot:prepared.local_slots)world.set_object_position(match.player(slot).source_current_object,far);
        const auto timer_at=bank+0x114u+unsigned(objective_index)*2u;
        const auto flags_at=bank+0x170u+unsigned(objective_index);
        unsigned touch_frames{};bool touch_returned{};
        if(touch_return && objective_index==8) {
          for(const auto slot:prepared.local_slots) {
            auto approach=dropped;approach.x-=.2f;
            world.set_object_position(match.player(slot).source_current_object,approach);
            for(unsigned n=0;n<4;++n) {
              match.step(controls);++touch_frames;
              check((word(memory,flags_at,1)&1u) && objects.object_parent(carried)==null_datum,
                  "Neutral objective contact incorrectly returned or acquired the dropped item");
            }
            world.set_object_position(match.player(slot).source_current_object,far);
          }
          state=match.state();row["neutral_touch_exclusion_frames"]=touch_frames;
        }
        if(touch_return && objective_index!=8) {
          check(!short_return,"Touch return needs the unchanged long timer to exclude unattended expiry");
          const auto toucher=std::ranges::find_if(prepared.local_slots,[&](auto slot) {
            return type==1?match.player(slot).source_membership==objective_index:
                match.player(slot).source_membership!=objective_index;
          });
          check(toucher!=prepared.local_slots.end(),"Touch return has no actual eligible player");
          const auto before=retained().players[*toucher][45+20]&0x7fffu;
          auto approach=dropped;approach.x-=.2f;
          world.set_object_position(match.player(*toucher).source_current_object,approach);
          while(touch_frames<20 && !touch_returned) {
            match.step(controls);++touch_frames;
            touch_returned=!(word(memory,flags_at,1)&11u) && half(memory,timer_at)==-1;
          }
          state=match.state();
          check(touch_returned,"Normal eligible player contact did not return the dropped objective");
          if(type==1)check((retained().players[*toucher][45+20]&0x7fffu)==before+1,
              "Actual CTF touch return did not retain exactly one original flag-return award");
          row["touch_return"]={{"slot",*toucher},{"frames",touch_frames}};
        }
        auto previous=half(memory,timer_at);unsigned decrements{},terminal_expiries{},frames{};bool returned=touch_returned;
        const auto before_teams=state.team_scores;
        const auto seconds=source_game_variant_get(variant,type==1?54:61);
        row["home_return"]={{"home_marker",home_marker},{"configured_seconds",seconds},{"initial_seconds",previous},{"samples",Json::array()}};
        const auto budget=rate*unsigned(std::max(6,seconds)+2);
        for(;frames<budget && !returned;++frames) {
          match.step(controls);state=match.state();(void)retained();
          const auto remaining=half(memory,timer_at);const auto flags=word(memory,flags_at,1);
          returned=!(flags&11u) && remaining==-1;
          if(remaining>=0 && previous>remaining)++decrements;
          // Whole24B950 clears the timer to NONE in the same frame as
          // its final decrement. A one-second interval never publishes
          // a zero at the frame boundary, on either source path.
          if(returned && previous>0)++terminal_expiries;
          if(frames<3 || remaining!=previous || returned)row["home_return"]["samples"].push_back(
              {{"frame",frames+1},{"seconds",remaining},{"flags",flags}});
          previous=remaining;
          check(state.team_scores==before_teams,"Unattended dropped objective awarded a score");
          check(carrier_item(carrier)==null_datum,"Unattended return fabricated a carrier");
        }
        row["home_return"]["frames"]=frames;row["home_return"]["decrements"]=decrements;
        row["home_return"]["terminal_expiries"]=terminal_expiries;
        check(returned,"Dropped original objective did not complete its unattended home return");
        if(!touch_returned)check(decrements+terminal_expiries>0,"Objective bypassed the ordinary return countdown");
        if(short_return)check(frames==rate && terminal_expiries==1,
            "One-second objective return did not preserve its original full tick interval");
        const auto position=objects.object_transform(carried).position;
        row["home_return"]["distance_from_authored_home"]=length(position-return_home);
        check(length(position-return_home)<1.f,"Returned original objective missed its authored home");
        check(objects.object_parent(carried)==null_datum,"Returned original objective retained a unit parent");
      }
      if(overtime_flow && row["overtime"]["enabled"].get<bool>()) {
        row["stage"]="original-overtime-eligibility-expiry";
        check(row.value("detached",false),"Overtime flow did not actually release its objective");
        const auto position=objects.object_transform(carried).position;
        const auto globals=word(memory,word(memory,0x4f0414u)+((word(memory,word(memory,0x4ecc1cu)+0x16cu)&65535u)*16u)+8u);
        const auto radius=Bytes(memory.view(word(memory,word(memory,globals+0xcu)+0x534u)+0xd8u,4)).f32(0);
        check(std::isfinite(radius) && radius>0,"Original overtime proximity radius is unavailable");
        Vec3 far{};float distance{};
        for(const auto& spawn:prepared.cache->spawns())if(length(spawn.position-position)>distance) {
          far=spawn.position;distance=length(far-position);
        }
        check(distance>radius+4.f,"Authored overtime map has no distant spawn outside the actual source radius");
        for(const auto slot:prepared.local_slots)world.set_object_position(match.player(slot).source_current_object,far);
        const auto bank=word(memory,0x52d710u);auto original=world.multiplayer_engine_state();
        unsigned frames{},decays{};bool ineligible{};std::uint32_t previous=word(memory,bank+0x200u),previous_eligible=word(memory,bank+0x1fcu,1);
        row["overtime"]["expiry_samples"]=Json::array();
        while(frames<rate*5u+16u && original.round.phase==1) {
          match.step(controls);state=match.state();original=world.multiplayer_engine_state();(void)retained();++frames;
          check(std::memcmp(&original.statistics,&match.source_match_statistics(),sizeof(original.statistics))==0,
              "Overtime expiry advanced a parallel score/win bank");
          const auto eligible=word(memory,bank+0x1fcu,1),remaining=word(memory,bank+0x200u);
          if(!eligible && original.round.phase==1) {
            if(!previous_eligible) {
              check(remaining+1u==previous,"Actual overtime countdown did not decrease once per ordinary frame");++decays;
            }
            ineligible=true;
          }
          if(frames<=3 || frames%rate==0 || original.round.phase!=1)
            row["overtime"]["expiry_samples"].push_back({{"frame",frames},{"tick",state.tick},
                {"eligible",eligible},{"remaining",remaining},{"phase",original.round.phase}});
          previous=remaining;previous_eligible=eligible;
        }
        check(ineligible && decays>=rate*5u-2u && original.round.phase==2 && original.recording.frozen,
            "Original dropped/distant objective did not complete genuine overtime expiry");
        row["overtime"]["expiry_frames"]=frames;row["overtime"]["decay_frames"]=decays;
        row["overtime"]["radius"]=radius;row["overtime"]["distance"]=distance;
      }
      if(overtime_flow) {
        row["stage"]="original-overtime-ending";auto original=world.multiplayer_engine_state();
        const auto kind=source_game_variant_get(variant,0);
        check(original.round.phase==2 && state.winner==-1 && state.winner_team==-1 &&
            (kind==0 || kind==5) && state.finished==(kind==0),"Original objective expiry lost its tied round policy/outcome");
        const auto completed=original.statistics;
        if(state.finished) {
          for(unsigned n=0;n<211;++n) {
            match.step(controls);state=match.state();original=world.multiplayer_engine_state();(void)retained();
            check(std::memcmp(&original.statistics,&completed,sizeof(completed))==0,"Overtime postgame changed completed scores/wins");
          }
          check(original.finalized && original.recording.frozen && original.game_over.complete() && original.game_over.fade_started,
              "Original overtime ending lost actual retained results/countdown/fade");
          row["overtime"]["postgame_frames"]=211;
        }else {
          const auto end_tick=state.tick;
          for(unsigned n=0;n<180;++n){match.step(controls);state=match.state();original=world.multiplayer_engine_state();(void)retained();}
          check(state.tick-end_tick==180 && original.round.phase==1 && original.round.index==1 && !original.recording.frozen,
              "Tied overtime ending lost its original150/30 reset");
          row["overtime"]["reset_tick"]=state.tick;
          unsigned factory_frames{};
          while(factory_frames<32 && std::ranges::any_of(prepared.local_slots,[&](auto slot){return match.player(slot).source_current_object==null_datum;})) {
            match.step(controls);state=match.state();(void)retained();++factory_frames;
          }
          for(const auto slot:prepared.local_slots)check(match.player(slot).source_current_object!=null_datum &&
              match.player(slot).source_current_object!=original_units[slot],"Overtime reset lost its genuine replacement current unit");
          row["overtime"]["factory_frames"]=factory_frames;
        }
      }
      if(win_flow) {
        row["stage"]="original-objective-winning-outcome";auto original=world.multiplayer_engine_state();
        for(unsigned n=0;n<8 && original.round.phase==1;++n) {
          match.step(controls);state=match.state();original=world.multiplayer_engine_state();
        }
        check(original.round.phase==2 && original.recording.frozen,"One-point objective did not reach its original frozen round ending");
        check(std::memcmp(&original.statistics,&match.source_match_statistics(),sizeof(original.statistics))==0,
            "Winning objective advanced a parallel native score/win bank");
        int winning_player=-1,winning_team=-1;
        for(const auto slot:prepared.local_slots)if(original.statistics.players[slot].fields[7]==1) {
          check(winning_player==-1,"One-round objective awarded multiple player winners");winning_player=int(slot);
        }
        for(unsigned team=0;team<8;++team)if(original.statistics.teams[team].fields[7]==1) {
          check(winning_team==-1,"One-round objective awarded multiple team winners");winning_team=int(team);
        }
        check(winning_player>=0 && state.winner==(teams?-1:winning_player) && state.winner_team==(teams?winning_team:-1) &&
            (!teams || winning_team>=0),"Native objective result differs from the original awarded winner");
        check(retained().players[unsigned(winning_player)][6]==1 &&
            (!teams || retained().teams[unsigned(winning_team)][6]==1),"Objective winning result omitted retained wins");
        const auto kind=source_game_variant_get(variant,0);
        check((kind==0 || kind==5) && state.finished==(kind==0),"Authored objective round rule selected the wrong completion");
        row["winning_outcome"]={{"tick",state.tick},{"round_kind",kind},{"winner",state.winner},
            {"winner_team",state.winner_team},{"awarded_player",winning_player},{"finished",state.finished}};
        if(state.finished) {
          const auto completed=original.statistics;
          for(unsigned n=0;n<211;++n) {
            match.step(controls);state=match.state();original=world.multiplayer_engine_state();(void)retained();
            check(std::memcmp(&original.statistics,&completed,sizeof(completed))==0,"Objective postgame changed its completed score/win bank");
          }
          check(original.finalized && original.game_over.complete() && original.game_over.fade_started && original.recording.frozen,
              "Winning objective did not complete actual retained results/countdown/fade");
          row["winning_outcome"]["postgame_frames"]=211;
        }else {
          const auto end_tick=state.tick;
          for(unsigned n=0;n<180;++n){match.step(controls);state=match.state();original=world.multiplayer_engine_state();(void)retained();}
          check(state.tick-end_tick==180 && original.round.phase==1 && original.round.index==1 && !original.recording.frozen &&
              original.statistics.players[unsigned(winning_player)].fields[7]==1,"Objective winning round lost its original reset or retained win");
          row["winning_outcome"]["reset_tick"]=state.tick;
          unsigned factory_frames{};
          while(factory_frames<32 && std::ranges::any_of(prepared.local_slots,[&](auto slot){return match.player(slot).source_current_object==null_datum;})) {
            match.step(controls);state=match.state();(void)retained();++factory_frames;
          }
          for(const auto slot:prepared.local_slots)check(match.player(slot).source_current_object!=null_datum &&
              match.player(slot).source_current_object!=original_units[slot],"Objective reset lost its replacement current unit");
          row["winning_outcome"]["factory_frames"]=factory_frames;
        }
      }
      row["native_kernels"]={{"arming",source_objective_native_tu15_stats.arming},{"decay",source_objective_native_tu15_stats.decay}};
      if(return_flow) {
        row["native_owner_calls"]=Json::array();
        for(unsigned n=0;n<source_objective_native_owner_entries.size();++n)
          row["native_owner_calls"].push_back({{"entry",source_objective_native_owner_entries[n]},
              {"calls",source_objective_native_tu15_stats.owners[n]}});
      }
      if(arming_decay) {
        const bool reference=std::getenv("HALO2_REFERENCE_OBJECTIVE_NATIVE_KERNELS");
        check(reference?source_objective_native_tu15_stats.arming==0 && source_objective_native_tu15_stats.decay==0:
            source_objective_native_tu15_stats.arming>0 && source_objective_native_tu15_stats.decay>=3,
            "Assault runtime did not execute its selected native/reference arming owner");
      }
      row["tick"]=state.tick;row["score"]=state.players[carrier].score;row["passed"]=true;row["stage"]="complete";
    }catch(const std::exception& error){row["error"]=error.what();++failures;}
    std::cout<<row.dump()<<'\n';report["cases"].push_back(std::move(row));save();
  }
  check(index>0,"No objective preset selected");
  if(const auto chosen=std::getenv("HALO2_OBJECTIVE_VARIANT"))report["selected_variant"]=chosen;
  else check(index==(arming_decay?5u:return_flow || overtime_flow || (goal_flow && !win_flow)?9u:15u),"Authored objective preset coverage changed");
  report["passed"]=failures==0;save();return failures?1:0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
