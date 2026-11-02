// Native cold Steam invitation over actual account HTTP. Provider handoff is a
// private fixture; the gate files never contain a bearer or private identity.
#include "halo2/online_account.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>
using namespace halo2;
using Json=nlohmann::json;
using Clock=std::chrono::steady_clock;
static std::string env(const char *name) {
  const auto *value=std::getenv(name);if(!value||!*value)throw std::runtime_error("Private invitation fixture input missing");return value;
}
static std::string encode(const std::string &value) {
  auto *raw=curl_easy_escape(nullptr,value.c_str(),int(value.size()));if(!raw)throw std::runtime_error("Fixture encoding failed");
  std::string out(raw);curl_free(raw);return out;
}
static void require(bool value){if(!value)throw std::runtime_error("Native Steam invitation projection differed");}
int main(int argc,char **argv){try {
  require((argc==3 || (argc==4 && std::string(argv[3])=="--cancel-lookup")) && curl_global_init(CURL_GLOBAL_DEFAULT)==CURLE_OK);
  const auto base=env("H2_ACCOUNT_URL"),token=env("H2_TEST_STEAM_TOKEN"),subject=env("H2_TEST_STEAM_SUBJECT"),target=env("H2_TEST_STEAM_TARGET");
  const std::string device(43,'d'),ns="http://specs.openid.net/auth/2.0";
  const auto url="https://steamcommunity.com/openid/login?openid.ns="+encode(ns)+"&openid.mode=checkid_setup&openid.realm="+encode(base+"/")+"&openid.return_to="+
    encode(base+"/v1/auth/steam/callback/"+device)+"&openid.identity="+encode(ns+"/identifier_select")+"&openid.claimed_id="+encode(ns+"/identifier_select");
  auto login=DiscordLogin::steam([](const auto&){return true;},[=](const DiscordHttpRequest &request,std::stop_token){
    if(request.url==base+"/v1/auth/steam/start")return DiscordHttpResponse{201,Json{{"device",device},{"url",url},{"expires_in",300},{"interval",1}}.dump()};
    if(request.url!=base+"/v1/auth/steam/poll")throw std::runtime_error("Unexpected private provider request");
    return DiscordHttpResponse{200,Json{{"state","signed_in"},{"token",token},{"expires_in",28800},
      {"identity",{{"id",subject},{"username","Native Invite Fixture"},{"display_name","Native Invite Fixture"}}}}.dump()};
  });
  OnlineAccountOptions options;options.endpoint=base;options.allow_loopback_http=true;options.enable_parties=true;
  options.poll_interval=std::chrono::seconds(5);auto owner=std::make_unique<OnlineAccount>(login,options);
  auto &account=*owner;login->start();
  if(argc==4) {
    bool requested{};
    const auto gate=Clock::now()+std::chrono::seconds(15);
    while(!std::filesystem::exists(argv[2])) {
      require(Clock::now()<gate);
      if(login->snapshot().identity)account.connect("Native Cancel Lookup");
      if(account.snapshot().authenticated && !requested){account.refresh_steam_friends();requested=true;}
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    const auto began=Clock::now();account.sign_out();login->cancel();owner.reset();
    const auto elapsed=std::chrono::duration<double>(Clock::now()-began).count();require(elapsed<2.5);
    std::cout<<"PASS native cancelled Steam lookup seconds "<<elapsed<<"; private provider fixture\n";return 0;
  }
  auto until=[&](auto predicate,std::chrono::seconds budget) {
    const auto end=Clock::now()+budget;
    while(Clock::now()<end){
      if(login->snapshot().status==DiscordLoginStatus::Failed)throw std::runtime_error("Private provider handoff failed");
      const auto state=account.snapshot();if(!state.authenticated&&login->snapshot().identity)account.connect("Native Cold Invite");
      if(state.authenticated&&!state.busy&&predicate(state))return state;
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    throw std::runtime_error("Native cold invitation timed out");
  };
  until([](const auto&){return true;},std::chrono::seconds(12));account.refresh_steam_friends();
  auto initial=until([&](const auto&s){return s.steam_friends_state=="ready" && s.steam_friends.size()==1 && s.steam_friends[0].id==target && s.steam_friends[0].registered && s.steam_friends[0].halo_online;},std::chrono::seconds(12));
  require(!initial.party);OnlinePartyCommand create;create.action="create";require(account.party_command(create));
  until([](const auto&s){return s.party && s.party->leader;},std::chrono::seconds(8));
  {std::ofstream ready(argv[1]);ready<<"ready\n";require(bool(ready));}
  const auto gate=Clock::now()+std::chrono::seconds(75);
  while(!std::filesystem::exists(argv[2])){require(Clock::now()<gate);std::this_thread::sleep_for(std::chrono::milliseconds(20));}
  const auto state=until([](const auto&s){return s.party && s.party->leader;},std::chrono::seconds(8));
  OnlinePartyCommand invite;invite.action="invite_steam";invite.value=target;invite.version=state.party->version;
  const auto began=Clock::now();require(account.party_command(invite));
  until([](const auto&s){return s.party && s.party->outgoing.size()==1 && s.party_message.empty();},std::chrono::seconds(100));
  const auto elapsed=std::chrono::duration<double>(Clock::now()-began).count();require(elapsed>3);
  account.sign_out();login->cancel();
  std::cout<<"PASS native cold Steam invitation seconds "<<elapsed<<"; private provider fixture\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
