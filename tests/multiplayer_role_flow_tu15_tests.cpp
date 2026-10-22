#include "halo2/multiplayer_world.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>

using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool value,const char* why){if(!value)throw FormatError(why);}
std::uint32_t word(const SourceRuntimeMemory& m,std::uint32_t p){return Bytes(m.view(p,4)).u32(0);}
std::uint16_t half(const SourceRuntimeMemory& m,std::uint32_t p){return Bytes(m.view(p,2)).u16(0);}
struct Files {
  std::filesystem::path path;
  explicit Files(unsigned n) {
    const auto scratch=std::getenv("HALO2_TEST_SCRATCH");check(scratch,"Role flow requires owned scratch");
    path=std::filesystem::path(scratch)/("role-flow-"+std::to_string(n));std::filesystem::create_directories(path);
  }
  ~Files(){std::error_code error;std::filesystem::remove_all(path,error);}
};
}
int main(int argc,char** argv)try {
  std::cout<<std::unitbuf;
  if(argc!=4)throw FormatError("Usage: multiplayer_role_flow_tu15_tests MCC_MAP VARIANT_FIXTURE REPORT");
  if(!std::filesystem::is_regular_file(argv[1]))return 77;
  Json fixture;std::ifstream(argv[2])>>fixture;
  Json report={{"passed",false},{"cases",Json::array()},
    {"scope","All authored Juggernaut presets with canonical roster/unit birth and ordinary source frames. Shared original15F8B0 receives declared killer/victim/relationship/reporting inputs before physical D8C20/D8D50/CCF40 cleanup; no role, timer, score or spawn result is supplied. This is engine death-event integration, not projectile hit proof, full played matches, presentation or replication acceptance."}};
  const auto save=[&]{std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';check(bool(out),"Cannot retain role flow report");};
  unsigned index{},failures{};
  for(const auto& preset:fixture.at("variants")) {
    if(preset.at("source_type").get<unsigned>()!=7)continue;
    const auto name=preset.at("name").get<std::string>();
    if(const auto chosen=std::getenv("HALO2_ROLE_VARIANT");chosen && name!=chosen)continue;
    Json row={{"name",name},{"passed",false},{"stage","construct"}};
    try {
      Files files(index++);MultiplayerOptions options;options.map=argv[1];options.local_players=4;
      options.source_object_world=true;options.source_object_savegame_directory=files.path;
      options.variant={name,preset.at("frontend_engine").get<unsigned>(),{}};
      for(const auto& w:preset.at("writes"))options.variant.values.append(w[0].get<unsigned>(),w[1].get<int>());
      MultiplayerPreparedWorld prepared(options);auto& match=prepared.match;auto& objects=*prepared.source_objects;
      auto& world=objects.tu15_world();auto& memory=world.memory();
      check(prepared.rules.requires_source_engine,"Role flow selected a bounded adapter");
      const auto bank=word(memory,0x4f63b8u)+0xfcu;
      check(word(memory,0x51df9cu)==bank,"Original role bank was not published at its actual inline owner");
      const auto rate=half(memory,word(memory,0x51df54u)+2u);check(rate>0 && rate<=120,"Role flow has no admitted clock rate");
      const auto mask=[&]{return half(memory,bank);};
      const auto role=[&]{const auto bits=mask();check(std::popcount(bits)==1,"Original role selection did not retain exactly one holder");return unsigned(std::countr_zero(bits));};
      const auto stat=[&](unsigned slot,unsigned field){return half(memory,0x51eef4u+slot*437u*2u+field*2u)&0x7fffu;};
      const auto role_traits=[&](unsigned slot) {
        std::array<bool,3> traits{};
        const bool holder=(mask()&(1u<<slot))!=0;
        const auto flags=word(memory,word(memory,0x4f3218u)+(holder?0x22cu:0x184u));
        for(unsigned selector=1;selector<=3;++selector) {
          auto a=world.prepare_source_call(std::array{match.player(slot).source_datum,selector});
          world.run_actual(0x1986d0u,a);traits[selector-1]=(a.gp[0]&255u)!=0;
          const auto shift=holder?selector*2u:selector==1?2u:selector+10u;
          check(traits[selector-1]==bool((flags>>shift)&1u),"Selected original role trait does not follow its current holder/global flags");
        }
        return traits;
      };
      auto state=match.state();row["stages"]=Json::array();
      const auto verify=[&] {
        const auto original=world.multiplayer_engine_state();
        check(std::memcmp(&original.statistics,&match.source_match_statistics(),sizeof(original.statistics))==0,"Role flow advanced a separate native score/stat bank");
        check(word(memory,word(memory,0x51df54u)+8u)==state.tick,"Native and original role clocks differ");
        unsigned members{};
        for(unsigned slot=0;slot<multiplayer_capacity;++slot)if(mask()&(1u<<slot)) {
          const auto& p=match.player(slot);
          check(p.connected && !(p.source_flags&2u) && p.source_membership>=0 && p.source_membership<16,"Original role mask retained an ineligible roster entry");
          members|=1u<<unsigned(p.source_membership);
        }
        check(half(memory,bank+2u)==members,"Original role and membership masks disagree");
        if(const auto stats=match.recorded_statistics()) {
          check(std::memcmp(stats,memory.view(0x51eef4u,sizeof(SourceResultStatistics)).data(),sizeof(SourceResultStatistics))==0,
              "Native retained role statistics differ from their original source bank");
        }
      };
      const auto step=[&] {match.step({});state=match.state();verify();};
      const auto sample=[&](const char* phase) {
        Json players=Json::array();
        for(const auto slot:prepared.local_slots) {
          const auto& p=state.players[slot];players.push_back({{"slot",slot},{"player",p.source_player},{"unit",p.source_unit},
              {"flags",p.source_flags},{"membership",p.source_membership},{"score",p.score},{"deaths",p.deaths},
              {"role_seconds",stat(slot,40)},{"role_transfers",stat(slot,38)},{"role_kills",stat(slot,39)},
              {"traits",role_traits(slot)}});
        }
        row["stages"].push_back({{"phase",phase},{"tick",state.tick},{"role_mask",mask()},{"membership_mask",half(memory,bank+2u)},{"players",std::move(players)}});
        report["in_progress"]=row;save();
      };
      verify();const auto initial=role();
      check(match.player(initial).source_current_object!=null_datum,"Original initial role holder has no genuine unit");
      std::vector<unsigned> hunters;
      for(const auto slot:prepared.local_slots)if(slot!=initial && match.player(slot).source_current_object!=null_datum)hunters.push_back(slot);
      check(hunters.size()>=2,"Authored role cap did not admit two living hunters");
      const auto killer=hunters[0],other=hunters[1];sample("initial-role");
      const bool recorded=world.multiplayer_engine_state().recording.enabled && !world.multiplayer_engine_state().recording.frozen;
      check(recorded,"Canonical host session skipped the original retained-result startup owner");
      const auto before_seconds=stat(initial,40);row["stage"]="ordinary-role-time";
      for(unsigned n=0;n<unsigned(rate)*2u;++n){step();check(role()==initial,"Initial role changed without an owning event");}
      if(recorded)check(stat(initial,40)==before_seconds+2u,"Original role holder did not record two periodic seconds");
      row["role_time"]={{"recording_enabled",recorded},{"before",before_seconds},{"after",stat(initial,40)}};
      for(const auto slot:prepared.local_slots)if(match.player(slot).source_current_object!=null_datum) {
        const auto traits=role_traits(slot);
        if(traits[0])check(match.player(slot).source_powerup_timers[0]>0,
            "Original role camouflage trait did not refresh the actual player powerup timer");
      }
      const auto death=[&](unsigned victim,std::uint32_t credit,const char* phase) {
        const auto unit=match.player(victim).source_current_object;check(unit!=null_datum,"Declared role death has no actual current unit");
        row["stage"]=phase;sample(phase);
        world.call_actual(0x15f8b0u,std::array{credit,match.player(victim).source_datum,0u,63u});
        objects.kill_biped(unit);return unit;
      };
      const auto respawn=[&](unsigned slot,std::uint32_t previous,const char* phase) {
        bool born{},saw_dead{},released_cap{};unsigned frames{};row["stage"]=phase;
        for(;frames<600 && !born && !state.finished;++frames) {
          step();saw_dead=saw_dead || state.players[slot].dead;
          const auto current=match.player(slot).source_current_object;
          born=current!=null_datum && current!=previous;
          if(!born && !released_cap && frames>=unsigned(rate)*8u) {
            const auto options=word(memory,0x4f3218u);const auto cap=word(memory,options+0x1b4u);
            unsigned active{};for(const auto candidate:prepared.local_slots)
              if(!(match.player(candidate).source_flags&2u) && match.player(candidate).source_current_object!=null_datum)++active;
            if(cap>0 && active==cap) {
              auto admission=world.prepare_source_call();admission.gp[0]=match.player(slot).source_datum;
              world.run_actual(0x1601b0u,admission);
              check((admission.gp[0]&255u)!=0,"Original full-unit cap did not report the absent victim's blocked admission");
              for(const auto candidate:prepared.local_slots)
                if(candidate!=initial && candidate!=killer && candidate!=other &&
                    match.player(candidate).source_current_object!=null_datum && !(mask()&(1u<<candidate))) {
                  row["cap_waiter_handoff"]={{"victim",slot},{"admitted_waiter",candidate},{"active_units",active},
                      {"source_cap",cap},{"blocked_victim",true},{"tick",state.tick}};
                  check(match.depart(candidate),"Original cap handoff participant did not accept normal departure");
                  released_cap=true;break;
                }
            }
          }
        }
        row[phase]={{"frames",frames},{"before",previous},{"after",match.player(slot).source_current_object},
            {"saw_dead",saw_dead},{"released_cap_by_spare_departure",released_cap}};
        check(born && saw_dead && !state.players[slot].dead && state.players[slot].source_unit==match.player(slot).source_current_object,
            "Ordinary role flow did not adopt the genuine replacement unit");
      };
      const auto transfer_before=stat(killer,38);
      const auto first_body=death(initial,match.player(killer).source_datum,"credited-role-death");
      check(mask()==(1u<<killer),"Original credited role death did not transfer directly to its actual killer");
      if(recorded)check(stat(killer,38)==transfer_before+1u,"Original credited transfer omitted its retained role statistic");
      respawn(initial,first_body,"former-role-respawn");check(role()==killer,"Original transferred role was lost during former holder respawn");sample("after-transfer-respawn");
      const auto before_score=state.players[killer].score;
      const auto role_kills=stat(killer,39);
      const auto second_body=death(initial,match.player(killer).source_datum,"role-kills-hunter");
      check(mask()==(1u<<killer),"Original role-holder kill unexpectedly transferred its role");
      step();check(state.players[killer].score==before_score+1,"Original role-holder kill did not award one point");
      if(recorded)check(stat(killer,39)==role_kills+1u,"Original role-holder kill omitted its retained statistic");
      respawn(initial,second_body,"hunter-respawn");
      const auto penalty_before=state.players[other].score;
      const auto third_body=death(initial,match.player(other).source_datum,"hunter-kills-hunter");
      check(mask()==(1u<<killer),"Hunter-versus-hunter death changed the original role");
      step();const auto penalty=(word(memory,word(memory,0x4f3218u)+0x22cu)&8u)?-1:0;
      check(state.players[other].score==penalty_before+penalty,"Original hunter penalty option did not reach its score owner");
      row["hunter_penalty"]={{"before",penalty_before},{"after",state.players[other].score},{"delta",penalty}};
      respawn(initial,third_body,"penalty-victim-respawn");
      const auto suicide_body=death(killer,match.player(killer).source_datum,"role-suicide");
      const auto successor=role();check(successor!=killer,"Original role suicide did not choose a different eligible successor");
      row["suicide_successor"]=successor;
      respawn(killer,suicide_body,"suicide-respawn");check(role()==successor,"Reselected role was lost during suicide respawn");
      row["stage"]="ordinary-role-departure";const auto departed=match.player(successor).source_datum;
      check(match.depart(successor) && !match.depart(successor),"Role departure was not admitted exactly once");
      step();const auto after_departure=role();check(after_departure!=successor && match.player(after_departure).source_current_object!=null_datum,
          "Original ordinary departure did not reassign the role to an eligible living player");
      check(match.player(successor).source_datum==departed && (match.player(successor).source_flags&2u) && state.players[successor].source_unit==null_datum,
          "Original role departure lost its retained result row or current-unit release");
      row["departure"]={{"slot",successor},{"retained_player",departed},{"successor",after_departure}};sample("after-role-departure");
      row["tick"]=state.tick;row["passed"]=true;row["stage"]="complete";
    }catch(const std::exception& error){row["error"]=error.what();++failures;}
    std::cout<<row.dump()<<'\n';report["cases"].push_back(std::move(row));report.erase("in_progress");save();
  }
  check(index>0,"No role preset selected");
  if(const auto chosen=std::getenv("HALO2_ROLE_VARIANT"))report["selected_variant"]=chosen;
  else check(index==5,"Authored role preset coverage changed");
  report["passed"]=failures==0;save();return failures?1:0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
