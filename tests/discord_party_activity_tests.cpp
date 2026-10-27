#include "halo2/discord_party_activity.hpp"
#include "halo2/online_account.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <chrono>
#include <atomic>
#include <iostream>
#include <mutex>
#include <map>
#include <stdexcept>
#include <thread>

using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
template<class F> void eventually(F f) {
  const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(4);
  while(!f()) {
    check(std::chrono::steady_clock::now()<end,"Activity operation timed out");
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
}
std::size_t discard(char *,std::size_t size,std::size_t count,void *){return size*count;}
const std::string target="222222222222222222",secret="h2p1_"+std::string(43,'x');
const std::string second_target="444444444444444444";
struct Server {
  std::mutex mutex;
  std::atomic<bool> block_poll{},poll_entered{},release_poll{};
  bool party=true,leader=true,fail_invite{},fail_link{},lost_once{},lost{},mutated{},preserve_link{};
  unsigned version=1,size=1,invites{},mutations{},leases{},refreshes{},joins{},sessions{};
  std::map<std::string,std::string> requests;
  std::string account_token=std::string(43,'s');
  Json replacement;
  Json view() const {
    Json members=Json::array();
    for(unsigned i=0;i<size;++i)members.push_back({{"handle","member-"+std::to_string(i)},
      {"name",i==0?"Chosen Pilot":"Other Pilot"},{"ready_playlist",""},
      {"self",leader?i==0:i==1},{"leader",i==0},{"online",true}});
    return {{"party",party?Json{{"version",version},{"leader",leader},{"members",members},{"outgoing",Json::array()}}:Json{}},
            {"invitations",Json::array()}};
  }
  OnlineMatchmakingResponse http(const OnlineMatchmakingRequest &r,std::stop_token stop) {
    std::lock_guard lock(mutex);
    check(r.url.starts_with("https://account.example/v1/"),"Activity request left pinned origin");
    if(r.url.ends_with("/session") && r.method=="POST") {
      check(r.bearer=="identity-fixture" || r.bearer=="identity-other","Discord credential missing from identity exchange");
      check(Json::parse(r.body)==Json{{"screen_name","Chosen Pilot"}},"Client asserted an account identity");
      ++sessions;
      if(r.bearer=="identity-other"){party=false;account_token=std::string(43,'t');}
      return {201,Json{{"token",account_token},{"expires_in",900}}.dump()};
    }
    check(r.bearer==account_token,"Activity did not use the private account session");
    if(r.method=="DELETE")return {200,"{}"};
    if(r.url.ends_with("/v1/clans?presence=1") && block_poll) {
      poll_entered=true;
      while(!release_poll && !stop.stop_requested())std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    if(r.url.ends_with("/v1/party/commands?countdown=1&privacy=1")) {
      const auto body=Json::parse(r.body);
      if(body.at("action")=="invite_discord") {
        ++invites;
        check(body.size()==4 && (body.at("discord_id")==target || body.at("discord_id")==second_target) && body.at("version")==version,
              "Native invite asserted invalid identity or version fields");
        const auto key=body.at("request_id").get<std::string>();
        const auto recipient=body.at("discord_id").get<std::string>();
        auto [request,inserted]=requests.emplace(recipient,key);
        if(!inserted)check(request->second==key,"Uncertain invitation retry changed its ID");
        if(fail_invite)return {403,Json{{"error","denied "+target}}.dump()};
        if(inserted){mutated=true;++mutations;}
        if(lost_once && !lost){lost=true;throw std::runtime_error("Lost response");}
      } else if(body.at("action")=="cancel") {
        check(body.size()==4 && body.at("version")==version,"Cancellation changed party authority");
      } else if(body.at("action")=="leave") {
        party=false;++version;
      } else {
        check(body.size()==3 && body.at("action")=="join_activity" && body.at("join_secret")==secret,
              "Join added an asserted party, version or identity");
        ++joins;party=true;leader=false;size=2;++version;
      }
      return {200,view().dump()};
    }
    if(r.url.ends_with("/v1/party/activity")) {
      ++leases;
      const auto body=Json::parse(r.body);
      if(body.contains("join_secret")) {
        ++refreshes;
        check(body==Json{{"version",version},{"join_secret",secret}},"Conditional refresh asserted a different capability");
        if(!preserve_link)return {404,R"({"error":"invitation unavailable"})"};
      } else check(mutated && body==Json{{"version",version}},"Activity link preceded confirmed native invitation");
      if(fail_link)return {409,R"({"error":"party changed"})"};
      if(!replacement.is_null())return {200,replacement.dump()};
      return {200,Json{{"party_id","00000000-0000-0000-0000-000000000001"},{"version",version},
          {"current_size",size},{"max_size",16},{"join_secret",secret},{"expires_in",600}}.dump()};
    }
    if(r.url.ends_with("/v1/party?local_players=1&appearance=1&postgame=1&countdown=1&privacy=1"))return {200,view().dump()};
    return {200,R"({"clan":null,"invitations":[]})"};
  }
};
struct Harness {
  std::shared_ptr<Server> server=std::make_shared<Server>();
  std::shared_ptr<OnlineAccount> account;
  std::unique_ptr<DiscordPartyActivity> activity;
  DiscordPartyActivity::Complete publish_done,send_done;
  std::optional<DiscordPartyLink> published;
  unsigned publications{},sends{};
  std::string recipient=target;
  Harness(std::shared_ptr<DiscordLogin> login,bool party=true,bool connect=true) {
    server->party=party;
    OnlineAccountOptions options{"https://account.example",false,std::chrono::milliseconds(10),true};
    account=std::make_shared<OnlineAccount>(std::move(login),options,[s=server](const auto &r,auto stop){return s->http(r,stop);});
    activity=std::make_unique<DiscordPartyActivity>(account,[this](const auto &link,auto done) {
      {std::lock_guard lock(server->mutex);check(server->mutations>=publications+1 && server->leases>=publications+1,"SDK publication preceded the server handoff");}
      ++publications;published=link;publish_done=std::move(done);
    },[this](const auto &subject,auto done) {
      check(subject==recipient && published && !publish_done,"SDK invite preceded successful presence or changed recipient");
      ++sends;send_done=std::move(done);
    });
    if(connect) {
      account->connect("Chosen Pilot");
      eventually([&]{const auto s=account->snapshot();return s.authenticated && !s.busy;});
    }
  }
  void prepare() {
    check(activity->invite(target),"Explicit party invitation was refused");
    check(!activity->invite(target),"Second click replaced an in-flight invitation");
    eventually([&]{activity->pump(false);return bool(publish_done);});
    check(!sends,"SDK invitation sent before presence completed");
    check(published && published->current_size==1 && published->max_size==16,"SDK party size changed");
  }
  void publish(bool successful) {auto done=std::move(publish_done);publish_done={};done(successful);}
  void private_status() {
    for(const auto &message:{activity->status().message,account->snapshot().message,account->snapshot().party_message})
      check(message.find(target)==std::string::npos && message.find(second_target)==std::string::npos && message.find(secret)==std::string::npos &&
            message.find(std::string(43,'s'))==std::string::npos,"Private activity data entered a display message");
  }
};
}
int main() {
  try {
    std::string browser;std::atomic<bool> switched{};DiscordLoginOptions options;options.callback_port=42117;
    auto login=std::make_shared<DiscordLogin>([&](const auto &url){browser=url;return true;},options,
      [&](const DiscordHttpRequest &r,std::stop_token)->DiscordHttpResponse {
        if(r.url.ends_with("/token"))return {200,Json{{"access_token",switched?"identity-other":"identity-fixture"},
          {"token_type","Bearer"},{"scope","identify"},{"expires_in",3600}}.dump()};
        return {200,Json{{"id",switched?"333333333333333333":"111111111111111111"},
          {"username","pilot"},{"global_name","Pilot"}}.dump()};
      });
    login->start();const auto begin=browser.find("state=")+6;
    const auto state=browser.substr(begin,browser.find('&',begin)-begin);
    std::unique_ptr<CURL,decltype(&curl_easy_cleanup)> curl(curl_easy_init(),curl_easy_cleanup);
    const auto callback="http://127.0.0.1:42117/discord/callback?code=test&state="+state;
    curl_easy_setopt(curl.get(),CURLOPT_URL,callback.c_str());curl_easy_setopt(curl.get(),CURLOPT_PROXY,"");
    curl_easy_setopt(curl.get(),CURLOPT_TIMEOUT_MS,2000L);curl_easy_setopt(curl.get(),CURLOPT_WRITEFUNCTION,discard);
    check(curl_easy_perform(curl.get())==CURLE_OK,"Identity fixture callback failed");
    eventually([&]{return login->authorization().has_value();});
    {
      Harness h(login);h.prepare();const auto repeated=h.publish_done;h.publish(true);repeated(true);
      check(h.sends==1 && h.send_done,"SDK presence callback sent zero or duplicate invitations");
      auto done=std::move(h.send_done);done(true);h.activity->pump(false);
      check(!h.activity->status().busy && h.activity->link() && h.sends==1,"Completed invitation lost its lease or resent itself");h.private_status();
    }
    for(bool reject_second:{false,true}) {
      Harness h(login);h.prepare();h.publish(true);auto first_done=std::move(h.send_done);first_done(true);
      const auto first=*h.activity->link();
      {std::lock_guard lock(h.server->mutex);h.server->fail_invite=reject_second;}
      h.recipient=second_target;
      check(h.activity->invite(second_target),"Second explicit recipient was refused");
      const auto preparing=h.activity->link();
      check(preparing && preparing->party_id==first.party_id && preparing->join_secret==first.join_secret,
            "Preparing a second invitation removed the first recipient's Discord party presence");
      eventually([&]{h.activity->pump(false);return reject_second?!h.activity->status().busy:bool(h.publish_done);});
      const auto second=h.activity->link();
      check(second && second->party_id==first.party_id && second->join_secret==first.join_secret && second->expires<=first.expires,
            "Second invitation invalidated or extended the first recipient's party lease");
      if(!reject_second){h.publish(true);auto second_done=std::move(h.send_done);second_done(true);}
      check(h.sends==(reject_second?1u:2u),"Second recipient caused an automatic or missing Discord send");
      h.private_status();
    }
    for(bool fail_publication:{false,true}) {
      Harness h(login);h.prepare();h.publish(true);auto first_done=std::move(h.send_done);first_done(true);
      const auto first=*h.activity->link();h.recipient=second_target;
      check(h.activity->invite(second_target),"Second SDK failure fixture was refused");
      eventually([&]{h.activity->pump(false);return bool(h.publish_done);});
      h.publish(!fail_publication);
      if(!fail_publication){auto done=std::move(h.send_done);done(false);}
      const auto retained=h.activity->link();
      check(retained && retained->join_secret==first.join_secret && retained->expires<=first.expires,
            "Second recipient's SDK failure invalidated a previously sent invitation");
      check(!h.activity->status().busy && h.sends==(fail_publication?1u:2u),"SDK failure resent an invitation");
    }
    {
      Harness h(login);h.prepare();h.publish(true);auto done=std::move(h.send_done);done(true);
      check(h.account->party_command({"invite_discord",second_target}),"Native second invitation was refused");
      check(h.activity->link().has_value(),"Native target invitation removed existing Discord party presence");
      eventually([&]{return !h.account->snapshot().busy;});
      check(h.account->party_command({"cancel","invitation-handle"}),"Native invitation cancellation was refused");
      check(h.activity->link().has_value(),"Cancelling one target revoked other recipients' Discord party presence");
      eventually([&]{return !h.account->snapshot().busy;});
      check(h.activity->link() && h.sends==1,"Target-specific native actions resent or revoked Discord invitations");
    }
    {
      Harness h(login);h.prepare();h.publish(true);auto done=std::move(h.send_done);done(true);
      const auto first=*h.activity->link();
      {std::lock_guard lock(h.server->mutex);h.server->preserve_link=true;++h.server->version;++h.server->size;}
      eventually([&]{const auto link=h.activity->link();return link && link->version==2 && link->current_size==2;});
      const auto refreshed=*h.activity->link();
      check(refreshed.party_id==first.party_id && refreshed.join_secret==first.join_secret && refreshed.expires<=first.expires,
            "Member admission rotated or extended the published Discord capability");
      {std::lock_guard lock(h.server->mutex);check(h.server->refreshes==1 && h.server->invites==1,"Presence refresh created another native invitation");}
      check(h.sends==1 && h.publications==1,"Member admission automatically sent another Discord invitation");h.private_status();
    }
    {
      Harness h(login);h.prepare();
      {std::lock_guard lock(h.server->mutex);h.server->preserve_link=true;++h.server->version;++h.server->size;}
      eventually([&]{const auto link=h.activity->link();return link && link->current_size==2;});
      h.publish(true);check(h.sends==1,"Same-party admission incorrectly cancelled a pending explicit invitation");
      auto done=std::move(h.send_done);done(true);h.private_status();
    }
    for(bool fail_invite:{true,false}) {
      Harness h(login);{std::lock_guard lock(h.server->mutex);h.server->fail_invite=fail_invite;h.server->fail_link=!fail_invite;}
      check(h.activity->invite(target),"Server rejection fixture was not submitted");
      eventually([&]{h.activity->pump(false);return !h.activity->status().busy;});
      check(!h.publications && !h.sends,"Failed server handoff reached Discord");h.private_status();
    }
    {
      Harness h(login);h.prepare();h.publish(false);
      check(!h.sends && !h.activity->status().busy,"Failed presence sent an invitation");h.private_status();
    }
    {
      Harness h(login);h.prepare();{std::lock_guard lock(h.server->mutex);++h.server->version;}
      eventually([&]{return h.account->snapshot().party->version==2;});h.publish(true);
      check(!h.sends && !h.activity->link(),"Party change allowed a stale SDK invitation");
    }
    {
      Harness h(login);h.prepare();h.account->sign_out();h.publish(true);
      check(!h.sends && !h.activity->link(),"Sign-out allowed a stale SDK invitation");h.private_status();
    }
    {
      Harness h(login);h.prepare();check(h.account->party_command({"leave"}),"Leave command was not queued");
      h.publish(true);check(!h.sends && !h.activity->link(),"Queued party departure allowed an SDK invitation");
    }
    {
      Harness h(login);h.prepare();h.activity->reset();h.publish(true);
      check(!h.sends,"Cancelled callback sent an SDK invitation");
    }
    {
      Harness h(login);h.prepare();h.activity.reset();h.publish(true);
      check(!h.sends,"Destroyed owner callback sent an SDK invitation");
    }
    {
      Harness h(login);h.prepare();h.activity->pump(true);h.publish(true);
      check(!h.sends && !h.activity->link(),"Entering a match allowed an SDK invitation");
    }
    const Json valid={{"party_id","00000000-0000-0000-0000-000000000001"},{"version",1},
      {"current_size",1},{"max_size",16},{"join_secret",secret},{"expires_in",600}};
    for(unsigned field=0;field<6;++field) {
      Harness h(login);auto invalid=valid;
      if(field==0)invalid["version"]=2;
      if(field==1)invalid["current_size"]=2;
      if(field==2)invalid["join_secret"]="invalid";
      if(field==3)invalid["extra"]="private";
      if(field==4)invalid["expires_in"]=0;
      if(field==5)invalid["max_size"]=32;
      {std::lock_guard lock(h.server->mutex);h.server->replacement=invalid;}
      check(h.activity->invite(target),"Malformed resource fixture was not submitted");
      eventually([&]{h.activity->pump(false);return !h.activity->status().busy;});
      check(!h.publications && !h.sends,"Malformed or mismatched link reached SDK presence");h.private_status();
    }
    {
      Harness h(login);{std::lock_guard lock(h.server->mutex);h.server->lost_once=true;}
      h.prepare();h.publish(true);auto done=std::move(h.send_done);done(false);
      {std::lock_guard lock(h.server->mutex);check(h.server->invites==2 && h.server->mutations==1,"Uncertain native invite duplicated membership authorization");}
      check(h.sends==1 && !h.activity->status().busy,"SDK send failure caused an automatic resend");h.private_status();
    }
    {
      Harness h(login);auto short_lease=valid;short_lease["expires_in"]=1;
      {std::lock_guard lock(h.server->mutex);h.server->replacement=short_lease;}
      h.prepare();std::this_thread::sleep_for(std::chrono::milliseconds(1100));h.publish(true);
      check(!h.sends && !h.activity->link(),"Expired lease was sent through Discord");
    }
    for(bool cold:{false,true}) {
      Harness h(login,false,!cold);check(h.activity->join(secret),"Accepted Discord invitation was refused");
      if(cold){h.activity->pump(false);check(h.server->joins==0,"Cold invite bypassed ONLINE sign-in");h.account->connect("Chosen Pilot");}
      eventually([&]{h.activity->pump(false);return !h.activity->status().busy;});
      const auto state=h.account->snapshot();
      check(state.party && !state.party->leader && state.party->members.size()==2 && state.party->members[1].self,
            "Accepted Discord join did not consume the authoritative roster");
      check(!h.publications && !h.sends && !h.activity->join(secret),"Join sent an invite or silently left an existing party");h.private_status();
    }
    {
      Harness h(login,false);h.activity->pump(true);check(!h.activity->join(secret),"Discord join interrupted gameplay");
      check(h.server->joins==0,"In-game invite acceptance issued a membership change");
    }
    for(const auto &invalid:{std::string{},std::string(48,'x'),std::string("h2p1_")+std::string(42,'x')+"/"}) {
      Harness h(login,false);check(!h.activity->join(invalid),"Malformed Discord join reached the account worker");
      check(h.server->joins==0,"Invalid join made an HTTP command");
    }
    {
      Harness h(login);h.prepare();Harness queued(login);
      queued.server->block_poll=true;eventually([&]{return queued.server->poll_entered.load();});
      check(queued.activity->invite(target),"Account-switch command fixture was not queued");
      switched=true;login->start();
      const auto begin=browser.find("state=")+6;
      const auto new_state=browser.substr(begin,browser.find('&',begin)-begin);
      const auto url="http://127.0.0.1:42117/discord/callback?code=other&state="+new_state;
      curl_easy_setopt(curl.get(),CURLOPT_URL,url.c_str());
      check(curl_easy_perform(curl.get())==CURLE_OK,"Switched identity fixture callback failed");
      eventually([&]{const auto identity=login->snapshot().identity;return identity && identity->id=="333333333333333333";});
      queued.server->release_poll=true;
      h.publish(true);check(!h.sends && !h.activity->link(),"Same screen name reused another Discord account's party link");
      eventually([&]{const auto state=h.account->snapshot();return state.authenticated && !state.busy && !state.party;});
      eventually([&]{queued.activity->pump(false);return !queued.activity->status().busy;});
      {std::lock_guard lock(queued.server->mutex);check(!queued.server->invites && !queued.sends,
        "Queued invitation moved from the old identity to a new Discord account");}
      {std::lock_guard lock(h.server->mutex);check(h.server->sessions>=2 && h.server->account_token==std::string(43,'t'),
        "Same-name Discord account switch reused the previous service credential");}
      h.private_status();
    }
    std::cout<<"PASS: explicit native/presence/SDK order, retry, failure, privacy, expiry, cancellation and accepted joins. SDK calls are fixtures; no Discord invitations were sent.\n";
    return 0;
  } catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
