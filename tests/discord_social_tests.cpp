#include "halo2/discord_social.hpp"
#include "halo2/discord_voice_blocks.hpp"
#include <nlohmann/json.hpp>
#include <atomic>
#include <mutex>
#include <curl/curl.h>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
std::size_t discard(char *,std::size_t size,std::size_t count,void *){return size*count;}
void callback(const std::string &url,const std::string &query) {
  const auto start=url.find("state=")+6;
  const auto state=url.substr(start,url.find('&',start)-start);
  const auto address="http://127.0.0.1:42009/discord/callback?"+query+"&state="+state;
  auto *client=curl_easy_init();
  curl_easy_setopt(client,CURLOPT_URL,address.c_str());
  curl_easy_setopt(client,CURLOPT_NOPROXY,"*");
  curl_easy_setopt(client,CURLOPT_TIMEOUT,3L);
  curl_easy_setopt(client,CURLOPT_WRITEFUNCTION,discard);
  const auto result=curl_easy_perform(client);curl_easy_cleanup(client);
  check(result==CURLE_OK,"Loopback callback failed");
}
template<class F> void wait(F predicate) {
  const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(4);
  while(!predicate() && std::chrono::steady_clock::now()<until)
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  check(predicate(),"Consent transition timed out");
}
}
int main() {
  try {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    DiscordLoginOptions options;options.callback_port=42009;
    std::string url;unsigned browser_calls{},social_requests{};
    auto browser=[&](const std::string &value){url=value;++browser_calls;return true;};
    auto identity=std::make_shared<DiscordLogin>(browser,options,
      [](const DiscordHttpRequest &request,std::stop_token)->DiscordHttpResponse {
        if(request.url.ends_with("/token"))return {200,R"({"access_token":"identity-only","token_type":"Bearer","scope":"identify","expires_in":3600})"};
        return {200,R"({"id":"123456789012345678","username":"identity_user"})"};
      });
    DiscordSocial friends(identity,browser,options,
      [&](const DiscordHttpRequest &request,std::stop_token)->DiscordHttpResponse {
        ++social_requests;
        if(request.url.ends_with("/token"))return {200,R"({"access_token":"friends-only","token_type":"Bearer","scope":"identify openid sdk.social_layer","expires_in":3600})"};
        // A successful OAuth login using a different Discord account must
        // never connect the SDK or replace the game's authenticated account.
        return {200,R"({"id":"987654321098765432","username":"other_user"})"};
      });
    friends.enable();
    check(!friends.snapshot().enabled && !browser_calls,"Signed-out friends opened OAuth");
    identity->start();callback(url,"code=identity");
    wait([&]{return identity->snapshot().identity.has_value();});
    {
      DiscordVoiceBlockOptions block_options;block_options.endpoint="https://voice.example";
      block_options.retry_interval=std::chrono::milliseconds(30);
      std::atomic<unsigned> attempts{};std::mutex guard;std::vector<std::string> bodies;
      DiscordVoiceBlocks blocks(identity,block_options,[&](const auto &request,std::stop_token)->DiscordVoiceBlockResponse {
        check(request.url=="https://voice.example/v1/account/voice-blocks" && request.bearer=="identity-only",
              "Private voice block update left its origin or used another credential");
        {std::lock_guard lock(guard);bodies.push_back(request.body);}
        if(++attempts==1)throw std::runtime_error("Lost PUT response");
        return {200,R"({"state":"updated"})"};
      });
      check(!blocks.update({"not-an-id"}),"Malformed block subject was queued");
      check(blocks.update({"222222222222222222","222222222222222222"}),"Block snapshot was not accepted");
      wait([&]{return attempts>=2;});
      {std::lock_guard lock(guard);check(bodies.size()==2 && bodies[0]==bodies[1] &&
        nlohmann::json::parse(bodies[0])==nlohmann::json{{"blocked",{"222222222222222222"}}},"Uncertain block replacement lost its private full snapshot");}
      check(blocks.update({"222222222222222222"}),"Repeated snapshot rejected");
      std::this_thread::sleep_for(std::chrono::milliseconds(80));check(attempts==2,"Unchanged block list repeatedly transmitted");
      blocks.reset();std::this_thread::sleep_for(std::chrono::milliseconds(50));
      check(attempts==2,"SDK disconnect silently unblocked stored voice preferences");
      check(blocks.update({}),"Explicit full unblock snapshot rejected");wait([&]{return attempts==3;});
      for(const auto &endpoint:{"http://voice.example","http://127.0.0.1:8080","https://voice.example/path","https://user@voice.example","https://voice.example?x"}) {
        auto invalid=block_options;invalid.endpoint=endpoint;
        DiscordVoiceBlocks bad(identity,invalid);check(!bad.update({}),"Unsafe voice preference origin accepted");
      }
    }
    const auto available=friends.snapshot().available;
    friends.enable();
    if(!available) {
      check(!friends.snapshot().enabled && browser_calls==1,"Stub build requested unusable friends access");
    } else {
      check(browser_calls==2 && friends.snapshot().connecting,"Friends consent did not open separately");
      check(url.find("scope=identify%20openid%20sdk.social_layer&")!=std::string::npos,"Game invitations lack Discord communication consent");
      callback(url,"error=access_denied");
      wait([&]{friends.pump();return !friends.snapshot().enabled;});
      check(identity->authorization()=="identity-only" && !social_requests,"Declining friends changed ONLINE identity");
      friends.enable();callback(url,"code=other-account");
      wait([&]{friends.pump();return !friends.snapshot().enabled;});
      check(social_requests==2 && !friends.snapshot().ready && friends.snapshot().friends.empty(),"Different account reached the SDK");
      check(identity->authorization()=="identity-only","Friends account mismatch replaced ONLINE login");
      friends.enable();
      identity->cancel();friends.pump();
      check(!friends.snapshot().enabled && friends.snapshot().friends.empty(),"Sign-out retained friends consent");
    }
    check(!friends.command("friend-0-1","accept") && !friends.command("unknown","cancel"),"Stale friend command accepted");
    check(!friends.party_recipient("friend-0-1"),"Stale friend retained a private invitation target");
    friends.disconnect();
    check(!friends.snapshot().ready && friends.snapshot().friends.empty(),"Disconnect retained private state");
    std::cout<<"PASS Discord friends consent isolation, account binding, stale commands and sign-out cleanup"<<std::endl;
    return 0;
  } catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
