#include "halo2/online_account.hpp"
#include "halo2/discord_activity_inbox.hpp"
#include <curl/curl.h>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

using namespace halo2;
namespace {
void check(bool value,const char *message) { if(!value)throw std::runtime_error(message); }
std::size_t discard(char *,std::size_t size,std::size_t count,void *) { return size*count; }
template<class F> void wait(F predicate) {
  const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(8);
  while(!predicate() && std::chrono::steady_clock::now()<end)
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  check(predicate(),"Native activity handoff timed out");
}
}
// The token/provider below are local test fixtures. No real OAuth, SDK message,
// game process, or external account is used. The join secret arrives via stdin.
int main(int argc,char **argv) {
  try {
    check(argc==3 || (argc==4 && std::string(argv[3])=="third"),"Expected loopback service and callback port");
    const bool third=argc==4;
    const std::string endpoint=argv[1];
    check(endpoint.starts_with("http://127.0.0.1:"),"Activity fixture requires loopback HTTP");
    const auto port=std::stoul(argv[2]);check(port>0 && port<=65535,"Invalid fixture callback port");
    curl_global_init(CURL_GLOBAL_DEFAULT);
    DiscordLoginOptions options;options.callback_port=unsigned(port);
    std::string authorization;
    auto identity=std::make_shared<DiscordLogin>([&](const std::string &url){authorization=url;return true;},options,
      [third](const DiscordHttpRequest &request,std::stop_token)->DiscordHttpResponse {
        return request.url.ends_with("/token")
          ? DiscordHttpResponse{200,third?R"({"access_token":"three","token_type":"Bearer","scope":"identify","expires_in":3600})":R"({"access_token":"two","token_type":"Bearer","scope":"identify","expires_in":3600})"}
          : DiscordHttpResponse{200,third?R"({"id":"333333333333333333","username":"pilot_three"})":R"({"id":"222222222222222222","username":"pilot_two"})"};
      });
    identity->start();
    const auto start=authorization.find("state=");check(start!=std::string::npos,"Fixture OAuth lacks state");
    const auto state=authorization.substr(start+6,authorization.find('&',start)-start-6);
    const auto callback="http://127.0.0.1:"+std::to_string(port)+"/discord/callback?code=fixture&state="+state;
    auto *curl=curl_easy_init();check(curl!=nullptr,"Fixture callback client unavailable");
    curl_easy_setopt(curl,CURLOPT_URL,callback.c_str());curl_easy_setopt(curl,CURLOPT_NOPROXY,"*");
    curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,discard);curl_easy_setopt(curl,CURLOPT_TIMEOUT_MS,2000L);
    const auto result=curl_easy_perform(curl);curl_easy_cleanup(curl);
    check(result==CURLE_OK,"Fixture OAuth callback failed");
    wait([&]{return identity->authorization().has_value();});
    OnlineAccountOptions account_options;account_options.endpoint=endpoint;
    account_options.allow_loopback_http=true;account_options.enable_parties=true;
    account_options.poll_interval=std::chrono::milliseconds(50);
    auto account=std::make_shared<OnlineAccount>(identity,account_options);
    account->connect(third?"Third Pilot":"Activity Pilot");
    wait([&]{return account->snapshot().authenticated && !account->snapshot().busy;});
    check(!account->snapshot().party && account->snapshot().party_invitations.size()==1,"Fixture lacks a pending target invitation");
    std::string secret;for(unsigned i=0;i<=48;++i) {
      const auto c=std::cin.get();if(c=='\n' || c==EOF)break;secret.push_back(char(c));
    }
    check(secret.size()==48,"Private activity secret extent differs");
    DiscordPartyActivity activity(account,[](const auto &,auto){throw std::runtime_error("Join published an invite");},
      [](const auto &,auto){throw std::runtime_error("Join sent an invite");});
    DiscordActivityInbox inbox([&](std::string value){return activity.join(std::move(value));},[]{return std::string{};});
    inbox.received("private-fixture-message","Host Pilot",true,[&](auto done){done(true,secret);});
    check(inbox.snapshot().invitations.size()==1 && inbox.accept(inbox.snapshot().invitations.front().handle),
          "Explicit in-game SDK acceptance was not submitted");
    wait([&]{activity.pump(false);return !activity.status().busy;});
    const auto joined=account->snapshot();
    check(joined.party && joined.party->members.size()==(third?3u:2u) && !joined.party->leader,"Native activity join did not accept target membership");
    check(joined.party->members[0].name=="Host Pilot" && joined.party->members.back().name==(third?"Third Pilot":"Activity Pilot") &&
          joined.party->members.back().self && joined.party_invitations.empty(),"Activity join lost chosen names or retained consumed invitations");
    check(joined.message.find(secret)==std::string::npos && joined.party_message.find(secret)==std::string::npos,
          "Activity secret escaped into native presentation");
    check(activity.status().message=="Joined ONLINE party from Discord.","Accepted activity callback did not complete");
    std::cout<<"READY native party admission\n"<<std::flush;
    std::string finish;std::getline(std::cin,finish);check(finish=="finish","Fixture completion missing");
    wait([&]{const auto s=account->snapshot();return s.party && s.party->members.size()==3;});
    const auto roster=account->snapshot().party->members;
    check(roster[0].name=="Host Pilot" && roster[1].name=="Activity Pilot" && roster[2].name=="Third Pilot",
          "First member admission invalidated another native Discord invitation");
    account->sign_out();check(!account->snapshot().party,"Sign-out retained joined party presentation");
    std::cout<<"PASS native HTTP activity redemption, chosen names, consumed invitation and sign-out\n";
    return 0;
  } catch(const std::exception &) {
    // Private handoff data never belongs in fixture output, including errors.
    std::cerr<<"Native activity HTTP fixture failed\n";return 1;
  }
}
