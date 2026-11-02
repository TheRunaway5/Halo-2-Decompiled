// Actual native friend/party clients over Rust HTTP; private provider handoff.
#include "halo2/online_account.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace halo2;
using Json = nlohmann::json;
using Clock = std::chrono::steady_clock;

static std::string env(const char *name) {
  const auto *value = std::getenv(name);
  if (!value || !*value) throw std::runtime_error("Private native social fixture input missing");
  return value;
}
static std::string encode(const std::string &value) {
  auto *raw = curl_easy_escape(nullptr, value.c_str(), int(value.size()));
  if (!raw) throw std::runtime_error("Fixture URL encoding failed");
  std::string result(raw); curl_free(raw); return result;
}
struct Client {
  std::shared_ptr<DiscordLogin> login;
  std::unique_ptr<OnlineAccount> account;
  std::string name;
};
static Client client(const std::string &base, const std::string &token,
                     const std::string &subject, const std::string &name) {
  const std::string device(43, 'd'), ns="http://specs.openid.net/auth/2.0";
  const auto url="https://steamcommunity.com/openid/login?openid.ns="+encode(ns)+
    "&openid.mode=checkid_setup&openid.realm="+encode(base+"/")+"&openid.return_to="+
    encode(base+"/v1/auth/steam/callback/"+device)+"&openid.identity="+encode(ns+"/identifier_select")+
    "&openid.claimed_id="+encode(ns+"/identifier_select");
  auto login=DiscordLogin::steam([](const auto&){return true;},
    [=](const DiscordHttpRequest &request,std::stop_token) {
      if(request.url==base+"/v1/auth/steam/start")
        return DiscordHttpResponse{201,Json{{"device",device},{"url",url},{"expires_in",300},{"interval",1}}.dump()};
      if(request.url!=base+"/v1/auth/steam/poll")throw std::runtime_error("Unexpected private provider request");
      return DiscordHttpResponse{200,Json{{"state","signed_in"},{"token",token},{"expires_in",28800},
        {"identity",{{"id",subject},{"username","Native Social Fixture"},{"display_name","Native Social Fixture"}}}}.dump()};
    });
  OnlineAccountOptions options;options.endpoint=base;options.allow_loopback_http=true;
  options.enable_parties=true;options.poll_interval=std::chrono::milliseconds(500);
  auto account=std::make_unique<OnlineAccount>(login,options);
  login->start();return {std::move(login),std::move(account),name};
}
template<class Predicate> static OnlineAccountSnapshot until(Client &c, Predicate predicate) {
  const auto deadline=Clock::now()+std::chrono::seconds(6);
  while(Clock::now()<deadline) {
    if(c.login->snapshot().status==DiscordLoginStatus::Failed)throw std::runtime_error("Private provider handoff failed");
    auto state=c.account->snapshot();
    if(!state.authenticated && c.login->snapshot().identity)c.account->connect(c.name);
    if(state.authenticated && !state.busy && predicate(state))return state;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  throw std::runtime_error("Native Rust social flow timed out");
}
static void require(bool condition) {
  if(!condition)throw std::runtime_error("Native Rust social projection differed");
}
int main() {try {
  require(curl_global_init(CURL_GLOBAL_DEFAULT)==CURLE_OK);
  const auto base=env("H2_ACCOUNT_URL");
  auto a=client(base,env("H2_TEST_SOCIAL_TOKEN_A"),env("H2_TEST_SOCIAL_SUBJECT_A"),"Native Social A");
  auto b=client(base,env("H2_TEST_SOCIAL_TOKEN_B"),env("H2_TEST_SOCIAL_SUBJECT_B"),"Native Social B");
  auto first=until(a,[](const auto&s){return s.friends_available && !s.friend_tag.empty();});
  auto second=until(b,[](const auto&s){return s.friends_available && !s.friend_tag.empty();});
  require(first.friends.empty() && second.friends.empty());
  require(a.account->friend_command("search",second.friend_tag));
  auto found=until(a,[](const auto&s){return s.friend_search.has_value();});
  require(found.friend_search->presence=="offline");
  require(a.account->friend_command("request",found.friend_search->id));
  until(a,[](const auto&s){return s.friends.size()==1 && s.friends[0].state=="outgoing" && s.friends[0].presence=="offline";});
  auto incoming=until(b,[](const auto&s){return s.friends.size()==1 && s.friends[0].state=="incoming";});
  require(b.account->friend_command("accept",incoming.friends[0].id));
  until(b,[](const auto&s){return s.friends.size()==1 && s.friends[0].state=="friend" && s.friends[0].presence=="online";});
  until(a,[](const auto&s){return s.friends.size()==1 && s.friends[0].state=="friend" && s.friends[0].presence=="online";});
  OnlinePartyCommand create;create.action="create";require(b.account->party_command(create));
  until(b,[](const auto&s){return s.party && s.party->leader;});
  auto joinable=until(a,[](const auto&s){return s.friends.size()==1 && s.friends[0].party_joinable==true && s.friends[0].party_context.size()==36;});
  OnlinePartyCommand join;join.action="join_friend";join.value=joinable.friends[0].id;join.join_context=joinable.friends[0].party_context;
  require(a.account->party_command(join));
  until(a,[](const auto&s){return s.party && !s.party->leader && s.party->members.size()==2;});
  until(b,[](const auto&s){return s.party && s.party->members.size()==2;});
  require(b.account->friend_command("block",incoming.friends[0].id));
  until(b,[](const auto&s){return s.friends.size()==1 && s.friends[0].state=="blocked" && s.friends[0].presence=="offline";});
  until(a,[](const auto&s){return s.friends.empty();});
  a.account->sign_out();b.account->sign_out();a.login->cancel();b.login->cancel();
  std::cout<<"PASS native friend consent, private presence, party context/join and bilateral block over Rust HTTP; private provider fixture\n";
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
