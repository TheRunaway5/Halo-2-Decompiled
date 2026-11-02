// Real native account HTTP against the Rust process; only provider handoff is a fixture.
#include "halo2/online_account.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <iostream>
#include <thread>
using namespace halo2;
using Json=nlohmann::json;
std::string encode(const std::string &value) {
  auto *raw=curl_easy_escape(nullptr,value.c_str(),int(value.size()));
  if(!raw)throw std::runtime_error("Fixture URL encoding failed");
  std::string result(raw);curl_free(raw);return result;
}
int main() {try {
  if(curl_global_init(CURL_GLOBAL_DEFAULT)!=CURLE_OK)throw std::runtime_error("HTTP fixture initialization failed");
  const char *endpoint=std::getenv("H2_ACCOUNT_URL"),*credential=std::getenv("H2_TEST_STEAM_TOKEN");
  if(!endpoint || !credential)throw std::runtime_error("Private fixture inputs required");
  const std::string base(endpoint),token(credential),device(43,'d');
  const std::string ns="http://specs.openid.net/auth/2.0",op="https://steamcommunity.com/openid/login";
  const auto url=op+"?openid.ns="+encode(ns)+"&openid.mode=checkid_setup&openid.realm="+encode(base+"/")+"&openid.return_to="+
    encode(base+"/v1/auth/steam/callback/"+device)+"&openid.identity="+encode(ns+"/identifier_select")+"&openid.claimed_id="+encode(ns+"/identifier_select");
  auto login=DiscordLogin::steam([](const auto&){return true;},[&](const DiscordHttpRequest &request,std::stop_token) {
    if(request.url==base+"/v1/auth/steam/start")return DiscordHttpResponse{201,Json{{"device",device},{"url",url},{"expires_in",300},{"interval",1}}.dump()};
    if(request.url!=base+"/v1/auth/steam/poll")throw std::runtime_error("Unexpected provider fixture request");
    return DiscordHttpResponse{200,Json{{"state","signed_in"},{"token",token},{"expires_in",28800},
      {"identity",{{"id","76561197960287930"},{"username","Steam Fixture"},{"display_name","Steam Fixture"}}}}.dump()};
  });
  OnlineAccountOptions options;options.endpoint=base;options.allow_loopback_http=true;options.poll_interval=std::chrono::seconds(20);
  OnlineAccount account(login,options);account.profile(FrontendAppearance{},"A117");login->start();
  const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);
  while(std::chrono::steady_clock::now()<deadline) {
    if(login->snapshot().status==DiscordLoginStatus::Failed)throw std::runtime_error("Provider fixture handoff failed");
    if(login->snapshot().identity)account.connect("Native Rust Pilot");
    const auto state=account.snapshot();
    if(state.authenticated && !state.busy) {
      if(!state.clan || state.clan->name!="Rust Clan" || state.clan->members.size()!=2 || !state.profile_message.empty())throw std::runtime_error("Native account/profile/clan projection differed");
      account.sign_out();login->cancel();
      std::cout<<"PASS native account/profile/clan client over Rust HTTP; fixture provider handoff\n";return 0;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
  throw std::runtime_error("Native Rust account HTTP timed out");
} catch(const std::exception &error) {std::cerr<<error.what()<<'\n';return 1;}}
