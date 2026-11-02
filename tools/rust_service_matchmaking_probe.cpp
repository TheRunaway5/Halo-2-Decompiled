// Real native matchmaking/party clients against the Rust listener. Only the
// external provider handoff is a deterministic private fixture.
#include "halo2/online_account.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <thread>

using namespace halo2;
using Json=nlohmann::json;
std::string encode(const std::string &value) {
  auto *raw=curl_easy_escape(nullptr,value.c_str(),int(value.size()));
  if(!raw)throw std::runtime_error("Fixture URL encoding failed");
  std::string result(raw);curl_free(raw);return result;
}
template<class Predicate> void until(Predicate predicate,const char *message) {
  const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(12);
  while(std::chrono::steady_clock::now()<deadline) {
    if(predicate())return;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  throw std::runtime_error(message);
}
int main(int argc,char **argv) {try {
  if(argc!=2 || (std::string(argv[1])!="v12" && std::string(argv[1])!="v16"))
    throw std::runtime_error("Explicit native compatibility control required");
  if(curl_global_init(CURL_GLOBAL_DEFAULT)!=CURLE_OK)throw std::runtime_error("HTTP fixture initialization failed");
  const char *endpoint=std::getenv("H2_MATCHMAKING_URL"),*credential=std::getenv("H2_TEST_STEAM_TOKEN"),
    *subject=std::getenv("H2_TEST_STEAM_SUBJECT"),*maps=std::getenv("H2_TEST_MATCHMAKING_MAPS");
  if(!endpoint || !credential || !subject || !maps)throw std::runtime_error("Private fixture inputs required");
  const std::string base(endpoint),token(credential),device(43,'d');
  const std::string ns="http://specs.openid.net/auth/2.0",op="https://steamcommunity.com/openid/login";
  const auto url=op+"?openid.ns="+encode(ns)+"&openid.mode=checkid_setup&openid.realm="+encode(base+"/")+"&openid.return_to="+
    encode(base+"/v1/auth/steam/callback/"+device)+"&openid.identity="+encode(ns+"/identifier_select")+"&openid.claimed_id="+encode(ns+"/identifier_select");
  auto login=DiscordLogin::steam([](const auto&){return true;},[&](const DiscordHttpRequest &request,std::stop_token) {
    if(request.url==base+"/v1/auth/steam/start")return DiscordHttpResponse{201,Json{{"device",device},{"url",url},{"expires_in",300},{"interval",1}}.dump()};
    if(request.url!=base+"/v1/auth/steam/poll")throw std::runtime_error("Unexpected provider fixture request");
    return DiscordHttpResponse{200,Json{{"state","signed_in"},{"token",token},{"expires_in",28800},
      {"identity",{{"id",subject},{"username","Steam Fixture"},{"display_name","Steam Fixture"}}}}.dump()};
  });
  OnlineMatchmakingOptions options;options.endpoint=base;options.allow_loopback_http=true;
  options.compatibility="halo2-native-queue-"+std::string(argv[1]);options.poll_interval=std::chrono::milliseconds(500);
  std::istringstream input(maps);for(std::string map;std::getline(input,map,',');)options.maps.push_back(map);
  OnlineMatchmaking matchmaking(login,options);
  login->start();
  until([&]{return login->snapshot().identity.has_value();},"Native provider fixture timed out");
  matchmaking.refresh("Native Rust Pilot",{},FrontendAppearance{});
  until([&]{return matchmaking.snapshot().status==OnlineMatchmakingStatus::Ready;},"Native Rust matchmaking session/catalog/ticket did not become ready");
  const auto state=matchmaking.snapshot();
  const auto eligible=std::count_if(state.playlists.begin(),state.playlists.end(),[](const auto &p){return p.eligible;});
  const bool modern=std::string(argv[1])=="v16";
  if(state.playlists.size()!=14 || eligible!=(modern?12:0))throw std::runtime_error("Native catalog eligibility differed");
  if(modern) {
    const auto lab=std::find_if(state.playlists.begin(),state.playlists.end(),[](const auto &p){return p.id=="lab-test";});
    if(lab==state.playlists.end() || !lab->eligible)throw std::runtime_error("Native lab playlist unavailable");
    OnlineAccountOptions account_options;account_options.endpoint=base;account_options.allow_loopback_http=true;
    account_options.enable_parties=true;account_options.poll_interval=std::chrono::seconds(20);
    OnlineAccount account(login,account_options);account.connect("Native Rust Pilot");
    until([&]{const auto s=account.snapshot();return s.authenticated && !s.busy;},"Native party account did not connect");
    if(!account.party_command({"create"}))throw std::runtime_error("Native create-party command was not accepted");
    until([&]{const auto s=account.snapshot();return !s.busy && s.party && s.party_command_successful==true;},"Native create-party HTTP did not publish");
    const auto before=account.snapshot().party_command_serial;
    OnlinePartyCommand start;start.action="countdown_start";start.playlist=lab->id;start.revision=lab->revision;
    if(!account.party_command(start))throw std::runtime_error("Native Start command was not accepted");
    until([&]{const auto s=account.snapshot();return !s.busy && s.party_command_serial>before && s.party_command_successful==true &&
      s.party && s.party->countdown_supported && s.party->countdown && s.party->countdown->seconds &&
      *s.party->countdown->seconds>0 && *s.party->countdown->seconds<=10;},"Native Start command did not observe Rust countdown");
    if(!account.party_command({"countdown_cancel"}))throw std::runtime_error("Native cancel-countdown command was not accepted");
    until([&]{const auto s=account.snapshot();return !s.busy && s.party_command_successful==true && s.party && !s.party->countdown;},"Native countdown cancellation did not publish");
    account.sign_out();
    std::cout<<"PASS native party Start/cancel observes the Rust shared countdown\n";
  }
  matchmaking.sign_out();login->cancel();
  std::cout<<"PASS native matchmaking catalog 14 eligible "<<eligible<<"; fixture provider handoff\n";
  return 0;
} catch(const std::exception &error) {std::cerr<<error.what()<<'\n';return 1;}}
