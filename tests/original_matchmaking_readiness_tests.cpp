#include "halo2/original_matchmaking_readiness.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <vector>
#include <nlohmann/json.hpp>
using namespace halo2;
static void check(bool v,const char *message){if(!v)throw std::runtime_error(message);}
int main(int argc,char **argv){try{
  check(argc==3,"Supply original readiness and error fixtures");
  std::array peers{OriginalMatchmakingPeer{3},OriginalMatchmakingPeer{3}};
  std::array seats{OriginalMatchmakingSeat{true,0,0},OriginalMatchmakingSeat{true,1,1}};
  check(original_matchmaking_connected_teams(peers,0,seats),"Connected two-team gate failed");
  seats[1].team=0;check(!original_matchmaking_connected_teams(peers,0,seats),"Single-team game admitted");
  seats[1].team=-1;check(!original_matchmaking_connected_teams(peers,0,seats),"Invalid team admitted");
  seats[1].team=1;peers[1].connections=1;
  check(!original_matchmaking_connected_teams(peers,0,seats),"Incomplete peer counted");
  check(original_matchmaking_connected_teams(peers,1,seats),"Original current peer exception lost");
  const auto team=original_matchmaking_eligibility(true,false,false,false,false);
  check(!team.ready && team.defer,"Team deferral differs");
  check(original_matchmaking_eligibility(false,false,false,false,false).ready,"Solo hopper deferred");
  check(original_matchmaking_eligibility(true,false,true,true,true).defer,"Original local policy ignored");
  check(original_matchmaking_eligibility(false,true,true,false,true).ready,"Policy used without original logon");
  check(original_matchmaking_eligibility(true,false,true,true,true,true,true).ready,"Original configuration overrides ignored");
  for(int state=-2;state<=22;++state){
    const auto expected=state==0||state==2||state==3?0u:state==5?0xacu:state==4?0xadu:state==6||state==14||state==19?0xaeu:0xabu;
    check(original_matchmaking_error_dialog(state).value_or(0)==expected,"Original dialog selector differs");
  }
  check(!original_matchmaking_error_dialog(4,true),"Local cancellation showed remote-party error");
  check(!original_matchmaking_error_clear(true,1,100,3099) && original_matchmaking_error_clear(true,1,100,3100),"Error expiry boundary differs");
  check(original_matchmaking_error_clear(true,1,0xfffffff0u,0xbc0u),"Original unsigned clock wrap differs");
  check(!original_matchmaking_error_clear(false,1,100,4000) && !original_matchmaking_error_clear(true,0,100,4000),"Nonhost/zero status cleared");
  std::ifstream readiness_stream(argv[1]),errors_stream(argv[2]);
  const auto readiness=nlohmann::json::parse(readiness_stream);
  const auto errors=nlohmann::json::parse(errors_stream);
  check(readiness.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Readiness fixture identity differs");
  check(errors.at("source_sha256")==readiness.at("original_xbe_sha256"),"Error fixture identity differs");
  for(const auto &row:readiness.at("readiness_controls")) {
    const auto &input=row.at("inputs");
    const auto teams=input.value("teams",std::vector<int>{0,1});
    const std::array observed_peers{OriginalMatchmakingPeer{3},OriginalMatchmakingPeer{input.value("second_connected",true)?3u:0u}};
    const std::array observed_seats{OriginalMatchmakingSeat{true,0,std::int8_t(teams[0])},OriginalMatchmakingSeat{true,1,std::int8_t(teams[1])}};
    const bool connected=original_matchmaking_connected_teams(observed_peers,input.value("current_peer",0u),observed_seats);
    const bool online=input.value("online",1)!=0;const auto flags=input.value("debug_policy",0u);
    const auto result=original_matchmaking_eligibility(online&&input.value("team_required",1)!=0,connected,
      online&&input.value("local_check",1)!=0,input.value("logon",1)==1,
      (input.value("local_flag",0u)&0x80000000u)!=0,(flags&255)!=0,(flags&0xff00)!=0);
    check(result.ready==row.at("ready") && result.defer==row.at("defer"),"Original readiness differs through Rust bridge");
  }
  for(const auto &row:errors.at("selector_controls")) {
    const auto result=original_matchmaking_error_dialog(row.at("state"),row.at("local_cancel"));
    check(row.at("dialog").is_null()?!result:result==row.at("dialog").get<unsigned>(),"Original error selector differs through Rust bridge");
  }
  for(const auto &row:errors.at("state_clear_controls")) {
    const int state=row.at("input_state");
    const bool clear=original_matchmaking_error_clear(row.at("local_host"),state,row.at("shown"),row.at("now"));
    check((clear?0:state)==row.at("result_state"),"Original error expiry differs through Rust bridge");
  }
  NativeMatchmakingLaunchObservation launch{true,false,true,true,0,true,0};
  seats[1].team=1;
  check(native_matchmaking_launch_ready(launch,seats),"Native Rust launch gate rejected admitted teams");
  launch.start_ms.reset();check(!native_matchmaking_launch_ready(launch,seats),"Absent countdown launched");
  launch.start_ms=1;check(!native_matchmaking_launch_ready(launch,seats),"Positive countdown launched");
  launch.start_ms=0;launch.transferred=true;check(!native_matchmaking_launch_ready(launch,seats),"Duplicate handoff launched");
  launch.transferred=false;seats[1].team=0;check(!native_matchmaking_launch_ready(launch,seats),"Single-team launch admitted");
  launch.team_mode=false;check(native_matchmaking_launch_ready(launch,seats),"Legacy FFA gate changed");
  std::cout<<"Original controls through Rust: readiness="<<readiness.at("readiness_controls").size()
    <<" selectors="<<errors.at("selector_controls").size()<<" state_clear="<<errors.at("state_clear_controls").size()<<'\n';
  std::cout<<"PASS original TU1.5 connected-team, explicit Live policy, deferral and lobby error kernels\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
