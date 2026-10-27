#include "halo2/original_matchmaking_readiness.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
using namespace halo2;
static void check(bool v,const char *message){if(!v)throw std::runtime_error(message);}
int main(){try{
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
  std::cout<<"PASS original TU1.5 connected-team, explicit Live policy, deferral and lobby error kernels\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
