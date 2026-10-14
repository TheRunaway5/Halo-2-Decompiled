#include "halo2/online_account.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>
using namespace halo2;
using Json = nlohmann::json;
namespace {
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
std::size_t discard(char *, std::size_t size, std::size_t count, void *) { return size * count; }
template<class F> void eventually(F f) {
  const auto end = std::chrono::steady_clock::now() + std::chrono::seconds(4);
  while (!f() && std::chrono::steady_clock::now() < end) std::this_thread::sleep_for(std::chrono::milliseconds(2));
  check(f(), "Account worker timed out");
}
}
int main() {
  try {
    std::string browser;
    DiscordLoginOptions auth; auth.callback_port = 42106;
    auto login = std::make_shared<DiscordLogin>([&](const std::string &url) { browser = url; return true; }, auth,
      [](const DiscordHttpRequest &r, std::stop_token) -> DiscordHttpResponse {
        return r.url.ends_with("/token") ? DiscordHttpResponse{200, R"({"access_token":"discord-test","token_type":"Bearer","scope":"identify","expires_in":3600})"}
          : DiscordHttpResponse{200, R"({"id":"123456789012345678","username":"verified","global_name":"Verified"})"};
      });
    login->start();
    eventually([&]{return !browser.empty();});
    const auto start = browser.find("state=") + 6;
    const auto state = browser.substr(start, browser.find('&', start) - start);
    const auto url = "http://127.0.0.1:42106/discord/callback?code=test&state=" + state;
    std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(), curl_easy_cleanup);
    curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str()); curl_easy_setopt(curl.get(), CURLOPT_PROXY, "");
    curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT_MS, 2000L); curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, discard);
    check(curl_easy_perform(curl.get()) == CURLE_OK, "Discord fixture callback failed");
    eventually([&] { return login->authorization().has_value(); });
    OnlineAccountOptions options; options.endpoint="https://account.example"; options.poll_interval=std::chrono::milliseconds(10);
    const std::string token(43,'s');
    std::atomic<unsigned> posts{}, deletes{}; std::atomic<bool> entered{}, block{}, release{};
    std::mutex keys_mutex; std::vector<std::string> keys;
    const auto empty=R"({"clan":null,"invitations":[]})";
    auto http = [&](const OnlineMatchmakingRequest &r, std::stop_token stop)->OnlineMatchmakingResponse {
      check(r.url.starts_with(options.endpoint+"/v1/"),"Account request left pinned origin");
      if(r.url.ends_with("/session") && r.method=="POST") {
        check(r.bearer=="discord-test","Account failed to reuse Discord authorization");
        const auto body=Json::parse(r.body); check(body.size()==1 && body.at("screen_name")=="Chosen Pilot","Client asserted account identity");
        return {201,Json{{"token",token},{"expires_in",900}}.dump()};
      }
      check(r.bearer==token,"OAuth credential leaked beyond account exchange");
      if(r.method=="DELETE") {++deletes;return {200,"{}"};}
      if(r.method=="POST") {
        ++posts; auto body=Json::parse(r.body);
        check(body.size()==3 && body.at("action")=="create" && body.at("name")=="My Clan","Untrusted clan command fields");
        { std::lock_guard lock(keys_mutex); keys.push_back(body.at("request_id")); }
        if(posts==1) throw std::runtime_error("Lost response");
        if(block) {entered=true;while(!release && !stop.stop_requested())std::this_thread::sleep_for(std::chrono::milliseconds(1));}
      }
      return {200,empty};
    };
    {
      OnlineAccount client(login,options,http); client.connect("Chosen Pilot");
      eventually([&]{return client.snapshot().authenticated && !client.snapshot().busy;});
      check(client.command({"create","My Clan"}),"Authenticated command denied");
      eventually([&]{return posts>=2 && !client.snapshot().busy;});
      {std::lock_guard lock(keys_mutex);check(keys.size()==2 && keys[0]==keys[1],"Uncertain command duplicated mutation");}
      block=true; check(client.command({"create","My Clan"}),"Second command denied");
      eventually([&]{return entered.load();});
      client.sign_out(); release=true; eventually([&]{return deletes.load()>0;});
      check(!client.snapshot().authenticated && !client.snapshot().clan && client.snapshot().invitations.empty(),"Late command revived signed-out identity");
    }
    for(const auto &endpoint:{"http://account.example","http://127.0.0.1:8080","https://user@account.example","https://account.example/path","https://account.example?x"}) {
      auto bad=options;bad.endpoint=endpoint;OnlineAccount client(login,bad,http);client.connect("Chosen Pilot");
      check(!client.snapshot().authenticated && !client.snapshot().busy,"Unsafe account origin accepted");
    }
    for(const auto &body:{std::string("{\"clan\":null,\"invitations\":[],\"clan\":null}"),std::string(65537,' '),std::string("{\"clan\":{},\"invitations\":[{\"id\":\"x\",\"name\":\"partial\"}]}")}) {
      OnlineAccount client(login,options,[&](const auto &r,std::stop_token)->OnlineMatchmakingResponse {
        if(r.url.ends_with("/session") && r.method=="POST")return {201,Json{{"token",token},{"expires_in",900}}.dump()};
        return {200,body};
      });
      client.connect("Chosen Pilot");eventually([&]{return !client.snapshot().busy;});
      check(!client.snapshot().authenticated && !client.snapshot().clan && client.snapshot().invitations.empty(),"Malformed clan data admitted or partially populated UI");
    }
    {
      Json clan={{"clan",{{"id","clan-handle"},{"name","My Clan"},{"role",3},
          {"members",Json::array({
              {{"id","self-handle"},{"name","Chosen Pilot"},{"role",3},{"self",true},{"presence","online"}},
              {{"id","peer-handle"},{"name","Chosen Pilot"},{"role",1},{"self",false},{"presence","playing"}},
              {{"id","offline-handle"},{"name","Other Pilot"},{"role",1},{"self",false},{"presence","offline"}}
          })},{"outgoing",Json::array()},{"messages",Json::array()}}},
          {"invitations",Json::array()}};
      auto read_clan=[&](const Json &value) {
        OnlineAccount client(login,options,[&](const auto &r,std::stop_token)->OnlineMatchmakingResponse {
          if(r.url.ends_with("/session") && r.method=="POST")return {201,Json{{"token",token},{"expires_in",900}}.dump()};
          check(r.bearer==token,"Clan presence leaked Discord authorization");
          if(r.method=="DELETE")return {200,"{}"};
          check(r.url.ends_with("/clans?presence=1"),"Native client omitted the compatible presence query");
          return {200,value.dump()};
        });
        client.connect("Chosen Pilot");eventually([&]{return !client.snapshot().busy;});
        return client.snapshot();
      };
      auto current=read_clan(clan);
      check(current.authenticated && current.clan && current.clan->members.size()==3,"Valid clan presence rejected");
      const auto &members=current.clan->members;
      check(members[0].presence==OnlineClanPresence::Online && members[1].presence==OnlineClanPresence::Playing &&
            members[2].presence==OnlineClanPresence::Offline,"Authenticated presence mapped to the wrong status");
      check(members[0].name==members[1].name && members[0].id!=members[1].id,"Presence conflated duplicate chosen names");
      auto legacy=clan;for(auto &m:legacy["clan"]["members"])m.erase("presence");
      current=read_clan(legacy);
      check(current.authenticated && current.clan && current.clan->members[0].presence==OnlineClanPresence::Unknown,
            "Legacy clan response became offline or lost compatibility");
      for(const Json invalid:{Json("forged"),Json(true),Json(nullptr)}) {
        auto bad=clan;bad["clan"]["members"][1]["presence"]=invalid;current=read_clan(bad);
        check(!current.authenticated && !current.clan,"Malformed presence partially populated clan state");
      }
    }
    {
      auto party_options=options;party_options.enable_parties=true;
      std::atomic<unsigned> party_posts{};std::mutex guard;std::vector<std::string> party_keys;
      const auto no_party=R"({"party":null,"invitations":[{"id":"invite-key","name":"Same Pilot","version":3}]})";
      const auto joined=R"({"party":{"version":4,"leader":true,"members":[{"handle":"private-party-handle","name":"Chosen Pilot","self":true,"leader":true,"online":true,"ready_playlist":""},{"handle":"other-party-handle","name":"Chosen Pilot","self":false,"leader":false,"online":true,"ready_playlist":"ranked"}],"outgoing":[]},"invitations":[]})";
      OnlineAccount client(login,party_options,[&](const OnlineMatchmakingRequest &r,std::stop_token)->OnlineMatchmakingResponse {
        if(r.url.ends_with("/session") && r.method=="POST")return {201,Json{{"token",token},{"expires_in",900}}.dump()};
        check(r.bearer==token,"Party request leaked Discord authorization");
        if(r.method=="DELETE")return {200,"{}"};
        if(r.url.ends_with("/party/commands")) {
          auto body=Json::parse(r.body);++party_posts;
          if(body.at("action")=="invite_discord") {
            check(body.at("discord_id")=="222222222222222222" && body.at("version")==4 && body.size()==4,
                  "SDK invitation lost its stable private subject or asserted a roster");
            return {200,joined};
          }
          check(body.at("action")=="accept" && body.at("target")=="invite-key" && body.at("version")==3 && body.size()==4,
                "Party acceptance asserted roster or lost invitation version");
          {std::lock_guard lock(guard);party_keys.push_back(body.at("request_id"));}
          if(party_posts==1)throw std::runtime_error("Lost party response");
          return {200,joined};
        }
        if(r.url.ends_with("/party?local_players=1"))return {200,party_posts>=2 ? joined : no_party};
        return {200,empty};
      });
      client.connect("Chosen Pilot");
      eventually([&]{return client.snapshot().authenticated && !client.snapshot().party_invitations.empty();});
      const auto invitation=client.snapshot().party_invitations[0];
      check(client.party_command({"accept",invitation.id,{},{},invitation.version}),"Authenticated party acceptance denied");
      eventually([&]{return party_posts>=2 && !client.snapshot().busy && client.snapshot().party.has_value();});
      const auto snapshot=client.snapshot();check(snapshot.party->members.size()==2 && snapshot.party->leader &&
          snapshot.party->members[0].name==snapshot.party->members[1].name,"Chosen names conflated party identities");
      {std::lock_guard lock(guard);check(party_keys.size()==2 && party_keys[0]==party_keys[1],"Uncertain party request duplicated mutation");}
      check(client.party_command({"invite_discord","222222222222222222"}),"Private SDK invitation was not submitted");
      eventually([&]{return party_posts==3 && !client.snapshot().busy;});
      check(client.snapshot().party->members[0].name=="Chosen Pilot","Private SDK subject escaped into presentation");
      client.sign_out();check(!client.snapshot().party && client.snapshot().party_invitations.empty(),"Sign-out retained party presentation");
    }
    {
      auto activity_options=options;activity_options.enable_parties=true;
      const auto secret="h2p1_"+std::string(43,'J');
      std::atomic<unsigned> joins{};std::mutex guard;std::vector<std::string> attempts;
      const auto no_party=R"({"party":null,"invitations":[]})";
      const auto joined=R"({"party":{"version":2,"leader":false,"members":[{"handle":"leader-handle","name":"Party Leader","self":false,"leader":true,"online":true,"ready_playlist":""},{"handle":"own-handle","name":"Chosen Pilot","self":true,"leader":false,"online":true,"ready_playlist":""}],"outgoing":[]},"invitations":[]})";
      OnlineAccount client(login,activity_options,[&](const OnlineMatchmakingRequest &r,std::stop_token)->OnlineMatchmakingResponse {
        if(r.url.ends_with("/session") && r.method=="POST")return {201,Json{{"token",token},{"expires_in",900}}.dump()};
        check(r.url.starts_with(options.endpoint+"/v1/") && r.bearer==token,"Activity join left its origin or leaked Discord authorization");
        if(r.method=="DELETE")return {200,"{}"};
        if(r.url.ends_with("/party/commands")) {
          const auto body=Json::parse(r.body);
          check(body.size()==3 && body.at("action")=="join_activity" && body.at("join_secret")==secret,
                "Activity join asserted a version/roster or lost its private secret");
          {std::lock_guard lock(guard);attempts.push_back(body.at("request_id"));}
          if(++joins==1)throw std::runtime_error("Lost activity acceptance response");
          return {200,joined};
        }
        if(r.url.ends_with("/party?local_players=1"))return {200,joins>=2 ? joined : no_party};
        return {200,empty};
      });
      client.connect("Chosen Pilot");eventually([&]{return client.snapshot().authenticated && !client.snapshot().busy;});
      for(const auto &bad:{std::string{},std::string("https://another.example/join"),"h2p1_"+std::string(43,'/'),secret+"x"})
        check(!client.party_command({"join_activity",bad}),"Malformed activity secret reached the account worker");
      check(!client.party_command({"join_activity",secret,{},{},1}),"Activity join accepted an asserted version");
      check(client.party_command({"join_activity",secret}),"Signed-in account could not redeem an activity secret without an existing party/version");
      eventually([&]{return joins==2 && !client.snapshot().busy && client.snapshot().party.has_value();});
      {std::lock_guard lock(guard);check(attempts.size()==2 && attempts[0]==attempts[1],"Uncertain activity redemption changed its command ID");}
      const auto snapshot=client.snapshot();
      check(snapshot.party->members.size()==2 && !snapshot.party->leader && snapshot.party->members[1].name=="Chosen Pilot",
            "Activity acceptance lost chosen names or recipient membership");
      check(snapshot.message.find(secret)==std::string::npos && snapshot.party_message.find(secret)==std::string::npos,
            "Activity secret escaped into status presentation");
      client.sign_out();check(!client.party_command({"join_activity",secret}) && !client.snapshot().party,
                             "Sign-out retained activity acceptance or party presentation");
    }
    for(bool party_command : {false,true}) {
      // An older read must not clear busy while a newer command is queued.
      // Otherwise a fast sequence of clan/party invitations can overwrite it.
      auto raced_options=options;raced_options.enable_parties=true;
      std::atomic<bool> pause_poll{},poll_entered{},release_poll{},post_entered{},release_post{};
      OnlineAccount client(login,raced_options,[&](const auto &r,std::stop_token stop)->OnlineMatchmakingResponse {
        if(r.url.ends_with("/session") && r.method=="POST")return {201,Json{{"token",token},{"expires_in",900}}.dump()};
        if(r.method=="DELETE")return {200,"{}"};
        if(r.method=="POST") {
          post_entered=true;
          while(!release_post && !stop.stop_requested())std::this_thread::sleep_for(std::chrono::milliseconds(1));
          return r.url.ends_with("/party/commands") ? OnlineMatchmakingResponse{200,R"({"party":null,"invitations":[]})"}
                                                    : OnlineMatchmakingResponse{200,empty};
        }
        if(r.url.ends_with("/clans?presence=1") && pause_poll) {
          poll_entered=true;
          while(!release_poll && !stop.stop_requested())std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return r.url.ends_with("/party?local_players=1") ? OnlineMatchmakingResponse{200,R"({"party":null,"invitations":[]})"}
                                         : OnlineMatchmakingResponse{200,empty};
      });
      client.connect("Chosen Pilot");eventually([&]{return client.snapshot().authenticated && !client.snapshot().busy;});
      pause_poll=true;eventually([&]{return poll_entered.load();});
      check(party_command ? client.party_command({"create"}) : client.command({"create","My Clan"}),"Race command was not queued");
      release_poll=true;eventually([&]{return post_entered.load();});
      check(client.snapshot().busy,"Older account poll cleared a pending command's busy state");
      check(!client.command({"invite","another_user"}) && !client.party_command({"create"}),
            "A second social command replaced a pending command");
      release_post=true;eventually([&]{return !client.snapshot().busy;});
    }
    login->cancel();
    std::cout<<"PASS account token privacy, chosen names, pinned TLS origin, command replay, cancellation race, sign-out, strict bounded JSON and no partial clan state\n";
  }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
