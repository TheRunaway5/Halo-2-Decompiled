#include "halo2/source_session_game_over.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace halo2;
using Json=nlohmann::json;
namespace {
unsigned checks{};
void check(bool ok,const char* message){++checks;if(!ok)throw std::runtime_error(message);}
SourceSessionPhaseState phase(const Json& row) {
  SourceSessionPhaseState out;
  out.phase=row.at("phase");out.previous_phase=row.at("previous_phase");out.record_count=row.at("record_count");
  out.generation=row.at("generation");out.sequence=row.at("sequence");out.phase_time=row.at("phase_time");
  out.previous_phase_time=row.at("previous_phase_time");out.transition_pending=row.at("transition_pending");
  check(row.at("records").size()==16,"Original session record array truncated");
  for(unsigned i=0;i<16;++i)out.records[i]={row.at("records")[i][0],row.at("records")[i][1]};
  return out;
}
SourceSessionRequest request(const Json& row) {
  return {row.at("pending"),row.at("kind"),row.at("length"),row.at("first_payload_word")};
}
SourceGameOverState over(const Json& row) {
  return {row.at("active"),row.at("fade_started"),row.at("countdown")};
}
SourceSessionGameOverInput input(const Json& row) {
  return {row.at("preflight_changed"),row.at("main_session_state"),row.at("world_active"),row.at("world_closing"),
      row.at("force_end"),row.at("game_ready"),row.at("game_kind"),row.at("same_game_identity")};
}
void compare(const SourceSessionPhaseState& actual,const Json& row) {
  const auto expected=phase(row);
  check(actual.phase==expected.phase,"Original session phase differs");
  check(actual.previous_phase==expected.previous_phase,"Original previous session phase differs");
  check(actual.record_count==expected.record_count,"Original record count changed");
  check(actual.generation==expected.generation && actual.sequence==expected.sequence,"Original uint32 counters differ");
  check(actual.phase_time==expected.phase_time && actual.previous_phase_time==expected.previous_phase_time,
      "Original separate millisecond times differ");
  check(actual.transition_pending==expected.transition_pending,"Original pending transition byte differs");
  for(unsigned i=0;i<16;++i) {
    check(actual.records[i].occupied==expected.records[i].occupied,"Original occupied byte changed");
    check(actual.records[i].changed==expected.records[i].changed,"Original changed byte differs");
  }
}
void compare(const SourceSessionRequest& actual,const Json& row) {
  const auto expected=request(row);
  check(actual.pending==expected.pending && actual.kind==expected.kind && actual.length==expected.length &&
      actual.first_payload_word==expected.first_payload_word,"Original queued session request differs");
}
void compare(const SourceGameOverState& actual,const Json& row) {
  const auto expected=over(row);
  check(actual.active==expected.active && actual.fade_started==expected.fade_started && actual.countdown==expected.countdown,
      "Original forced game-over start differs");
}
void compare(const SourceSessionPhaseState& phase,const SourceSessionRequest& request,
    const SourceGameOverState& over,const Json& row) {
  compare(phase,row.at("phase"));compare(request,row.at("request"));compare(over,row.at("game_over"));
}
void unavailable_boundaries(const Json& original) {
  const auto before=original.at("before");
  for(unsigned boundary=0;boundary<4;++boundary) {
    auto p=phase(before.at("phase"));auto r=request(before.at("request"));auto g=over(before.at("game_over"));
    auto i=input(original.at("input"));
    // The source owner does not provide a successful external map/campaign/
    // closing service in this oracle. Such inputs remain explicit failures.
    p.phase=4;r.pending=0;g.active=false;g.countdown=0;
    i.main_session_state=5;i.game_ready=true;
    if(boundary==0)i.same_game_identity=true;
    if(boundary==1){i.force_end=true;i.game_kind=1;}
    if(boundary==2){i.main_session_state=3;i.world_active=true;i.world_closing=true;}
    if(boundary==3)p.record_count=17;
    const auto saved_p=p;const auto saved_r=r;const auto saved_g=g;
    bool rejected=false;
    try {
      if(boundary==3)source_session_phase_set(p,5,123);
      else source_session_game_over_update(i,p,r,g,30,123,{});
    }catch(const std::invalid_argument&){rejected=true;}
    check(rejected,"Unrecovered session boundary was silently admitted");
    check(p.phase==saved_p.phase && p.previous_phase==saved_p.previous_phase && p.sequence==saved_p.sequence &&
        p.generation==saved_p.generation && p.transition_pending==saved_p.transition_pending,
        "Unavailable session boundary partially mutated its phase");
    check(r.pending==saved_r.pending && r.kind==saved_r.kind && g.active==saved_g.active && g.countdown==saved_g.countdown,
        "Unavailable session boundary partially mutated its request/countdown");
  }
}
}
int main(int argc,char**argv) {
  try {
    if(argc!=2)throw std::runtime_error("Supply original session game-over fixture");
    Json data;std::ifstream(argv[1])>>data;
    check(data.at("instruction_patches").empty() && data.at("supplied_returns").empty(),"Original dependency result was supplied");
    unsigned setters=0,updates=0,sequences=0,gates=0;
    for(const auto& version:data.at("versions")) {
      for(const auto& row:version.at("phase_setters")) {
        auto actual=phase(row.at("before").at("phase"));
        source_session_phase_set(actual,row.at("requested"),row.at("milliseconds"));compare(actual,row.at("after").at("phase"));
        ++setters;
      }
      for(const auto& row:version.at("updates")) {
        auto p=phase(row.at("before").at("phase"));auto r=request(row.at("before").at("request"));auto g=over(row.at("before").at("game_over"));
        check(source_session_game_over_update(input(row.at("input")),p,r,g,row.at("tick_rate"),row.at("milliseconds"),{})==row.at("changed"),
            "Original session update return differs");
        compare(p,r,g,row.at("after"));++updates;
      }
      for(const auto& row:version.at("sequences")) {
        auto p=phase(row.at("before").at("phase"));auto r=request(row.at("before").at("request"));auto g=over(row.at("before").at("game_over"));
        for(const auto& step:row.at("steps")) {
          check(source_session_game_over_update(input(row.at("input")),p,r,g,row.at("tick_rate"),row.at("milliseconds"),{})==step.at("changed"),
              "Original repeated session update return differs");
          compare(p,r,g,step.at("after"));
        }
        ++sequences;
      }
      for(const auto& row:version.at("menu_gates")) {
        SourceGameOverState g{row.at("active"),false,row.at("countdown")};
        check(source_game_over_requests_menu(row.at("mode"),g)==row.at("request_menu"),"Original signed-byte menu gate differs");
        ++gates;
      }
      unavailable_boundaries(version.at("updates")[0]);
    }
    check(setters==288 && updates==3012 && sequences==48 && gates==3072,"Original session coverage truncated");
    std::cout<<checks<<" comparisons: three original images,288 phase setters,3012 whole-owner updates,48 repeated sequences and3072 signed-mode gates passed\n";
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
