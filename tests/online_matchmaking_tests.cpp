#include "halo2/online_matchmaking.hpp"
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
  check(f(), "Queue worker timed out");
}
}
int main() {
  try {
    std::string browser;
    DiscordLoginOptions auth; auth.callback_port = 42105;
    auto login = std::make_shared<DiscordLogin>([&](const std::string &url) { browser = url; return true; }, auth,
      [](const DiscordHttpRequest &r, std::stop_token) -> DiscordHttpResponse {
        return r.url.ends_with("/token") ? DiscordHttpResponse{200, R"({"access_token":"discord-test","token_type":"Bearer","scope":"identify","expires_in":3600})"}
          : DiscordHttpResponse{200, R"({"id":"123456789012345678","username":"verified","global_name":"Verified"})"};
      });
    login->start();
    const auto start = browser.find("state=") + 6;
    const auto state = browser.substr(start, browser.find('&', start) - start);
    const auto url = "http://127.0.0.1:42105/discord/callback?code=test&state=" + state;
    std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(), curl_easy_cleanup);
    curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str()); curl_easy_setopt(curl.get(), CURLOPT_PROXY, "");
    curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT_MS, 2000L); curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, discard);
    check(curl_easy_perform(curl.get()) == CURLE_OK, "Discord fixture callback failed");
    eventually([&] { return login->authorization().has_value(); });
    OnlineMatchmakingOptions options; options.endpoint = "https://queue.example"; options.maps = {"lockout"}; options.poll_interval = std::chrono::milliseconds(10);
    // This test replaces HTTP and cannot provide an authenticated probe socket.
    // The independent native-process HTTP test exercises v8 and actual probes.
    options.compatibility="test-v1";
    const auto playlists = Json{{"playlists", Json::array({{{"id","ranked"},{"revision","1"},{"name","Ranked Slayer"},{"description","Lab fixture"},
        {"ranked",true},{"quickmatch",true},{"eligible",true},{"minimum_players",2},{"maximum_players",2},{"level",1}}})}}.dump();
    const std::string session_token(43, 's');
    std::atomic<bool> queued{}, block{}, release{}, entered{}, revoke{};
    std::atomic<unsigned> posts{}, deletes{};
    std::mutex keys_mutex;
    std::vector<std::string> keys;
    auto http = [&](const OnlineMatchmakingRequest &r, std::stop_token stop) -> OnlineMatchmakingResponse {
      check(r.url.starts_with(options.endpoint + "/v1/matchmaking/"), "Queue request left pinned origin");
      if (r.url.ends_with("/session") && r.method == "POST") {
        check(r.bearer == "discord-test", "Session did not reuse Discord login");
        const auto b = Json::parse(r.body);
        check(b.size() == 3 && b.at("screen_name") == "Chosen Pilot" && !b.contains("player_id"), "Client asserted identity or lost chosen name");
        return {201, Json{{"token",session_token},{"expires_in",900}}.dump()};
      }
      check(r.bearer == session_token, "OAuth token leaked past session exchange");
      if (r.method == "DELETE") { queued = false; ++deletes; if (r.url.ends_with("/session")) revoke = true; return {200,"{}"}; }
      if (r.url.ends_with("/playlists")) return {200,playlists};
      if (r.method == "POST") {
        ++posts;
        const auto b = Json::parse(r.body);
        check(b.size() == 3 && b.at("playlist_id") == "ranked" && b.at("revision") == "1" && b.at("request_id").get<std::string>().size() == 36, "Client supplied untrusted queue fields");
        { std::lock_guard lock(keys_mutex); keys.push_back(b.at("request_id")); }
        queued = true;
        if (posts == 1) throw std::runtime_error("Lost HTTP response");
        if (block) { entered = true; while (!release && !stop.stop_requested()) std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
      }
      if(!queued)return {200,R"({"state":"idle","playlist_id":"","queued_players":0,"match":null})"};
      return {200, R"({"state":"searching","playlist_id":"ranked","queued_players":1,"match":null})"};
    };
    {
      OnlineMatchmaking client(login, options, http);
      client.refresh("Chosen Pilot"); eventually([&] { return client.snapshot().status == OnlineMatchmakingStatus::Ready; });
      client.join("ranked"); eventually([&] { return client.snapshot().status == OnlineMatchmakingStatus::Searching; });
      { std::lock_guard lock(keys_mutex); check(keys.size() == 2 && keys[0] == keys[1], "Uncertain POST was not retried idempotently"); }
      check(client.snapshot().playlists[0].level == 1 && client.snapshot().playlist_id == "ranked", "Verified catalog or queue missing");
      client.cancel_queue(); eventually([&] { return !queued && client.snapshot().status == OnlineMatchmakingStatus::Ready; });
      block = true; client.join("ranked"); eventually([&] { return entered.load(); });
      client.sign_out(); release = true;
      eventually([&] { return revoke.load(); });
      check(!queued && client.snapshot().playlists.empty() && !client.snapshot().match, "Late POST revived queue after sign-out");
    }
    block=false; release=false; revoke=false;
    {
      // The leader owns this accepted member's ticket. The member must
      // discover it through authenticated polling without a second enqueue.
      const auto before=posts.load();
      OnlineMatchmaking member(login,options,http);
      member.refresh("Chosen Pilot");
      eventually([&]{return member.snapshot().status==OnlineMatchmakingStatus::Ready;});
      queued=true;
      eventually([&]{return member.snapshot().status==OnlineMatchmakingStatus::Searching;});
      check(posts==before,"Passive party member enqueued an independent ticket");
      member.cancel_queue();
      eventually([&]{return !queued && member.snapshot().status==OnlineMatchmakingStatus::Ready;});
      member.sign_out();eventually([&]{return revoke.load();});
    }
    {
      OnlineMatchmaking denied(login,options,[&](const auto &r,std::stop_token stop)->OnlineMatchmakingResponse {
        if(r.url.ends_with("/ticket") && r.method=="POST")
          return {409,R"({"error":"Every party member must choose this playlist first"})"};
        return http(r,stop);
      });
      denied.refresh("Chosen Pilot");
      eventually([&]{return denied.snapshot().status==OnlineMatchmakingStatus::Ready;});
      denied.join("ranked");
      eventually([&]{return denied.snapshot().status==OnlineMatchmakingStatus::Failed;});
      check(denied.snapshot().message=="Every party member must choose this playlist first","Server party consent failure was hidden");
    }
    {
      // Opted-in progress shares v8 admission with old four-field responses.
      // Only the admitted gathering's chosen names belong in the roster.
      std::atomic<unsigned> phase{};
      OnlineMatchmaking client(login,options,[&](const auto &r,std::stop_token stop)->OnlineMatchmakingResponse {
        if(r.method=="GET" && r.url.ends_with("/ticket?progress=1") && queued) {
          Json progress={{"players",2},{"minimum_players",2},{"maximum_players",4},{"wait_ms",15000},
            {"roster",Json::array({{{"name","Chosen Pilot"},{"self",true}},{{"name","Peer Pilot"},{"self",false}}})}};
          Json body={{"state","searching"},{"playlist_id","ranked"},{"queued_players",2},{"match",nullptr},{"progress",progress}};
          switch(phase.load()) {
          case 0: break;
          case 1: body["progress"]=nullptr; break;
          case 2: body["progress"]["private_id"]="must-not-enter-ui"; break;
          case 3: body["state"]="idle";body["playlist_id"]="";break;
          case 4: body["progress"]["roster"][1]["self"]=true;break;
          case 5: body["progress"]["players"]=1;break;
          case 6: body["progress"]["minimum_players"]=3;break;
          case 7: body["progress"]["wait_ms"]=-1;break;
          case 8: body["progress"]["maximum_players"]=1;break;
          case 9: body["progress"]["roster"][1]["id"]="private";break;
          case 10: body["unexpected"]=true;break;
          }
          return {200,body.dump()};
        }
        return http(r,stop);
      });
      client.refresh("Chosen Pilot");eventually([&]{return client.snapshot().status==OnlineMatchmakingStatus::Ready;});
      const auto before=posts.load();queued=true;
      auto gathered=[&]{const auto s=client.snapshot();return s.status==OnlineMatchmakingStatus::Searching && s.gathering.has_value();};
      eventually(gathered);
      const auto s=client.snapshot();
      check(s.gathering->players.size()==2 && s.gathering->players[1].name=="Peer Pilot" &&
        s.gathering->players[0].self && !s.gathering->players[1].self && s.gathering->wait_ms==15000,
        "Actual gathering roster or growth wait was lost");
      check(posts==before,"Gathering progress created a second party ticket");
      phase=1;eventually([&]{const auto s=client.snapshot();return s.status==OnlineMatchmakingStatus::Searching && !s.gathering;});
      for(unsigned invalid=2;invalid<=10;++invalid) {
        phase=0;eventually(gathered);phase=invalid;
        eventually([&]{return client.snapshot().status==OnlineMatchmakingStatus::Failed;});
        check(!client.snapshot().gathering,"Invalid progress retained stale admitted players");
      }
      phase=0;eventually(gathered);client.cancel_queue();
      check(!client.snapshot().gathering,"Queue cancellation retained gathering presentation");
      eventually([&]{return !queued && client.snapshot().status==OnlineMatchmakingStatus::Ready;});
    }
    for (const auto &endpoint : {"http://queue.example", "http://127.0.0.1:8080", "https://user@queue.example", "https://queue.example/path", "https://queue.example?x"}) {
      auto invalid = options; invalid.endpoint = endpoint;
      OnlineMatchmaking client(login, invalid, http);
      check(client.snapshot().status == OnlineMatchmakingStatus::Failed, "Invalid or unapproved plaintext origin accepted");
    }
    for (unsigned variant=0;variant<7;++variant) {
      auto body=Json::parse(playlists);
      auto &row=body["playlists"][0];row["clan_match"]=true;row["eligible"]=false;
      row["level"]=nullptr;
      if(variant==1){row["level"]=50;row["eligible"]=true;}
      if(variant==2)row["clan_match"]="true";
      if(variant==3)row["ranked"]=false;
      if(variant==4)row["eligible"]=true;
      if(variant==5)row["forged_clan_id"]="untrusted";
      if(variant==6)row.erase("clan_match");
      OnlineMatchmaking client(login,options,[&](const auto &r,std::stop_token)->OnlineMatchmakingResponse {
        if(r.url.ends_with("/session") && r.method=="POST")return {201,Json{{"token",session_token},{"expires_in",900}}.dump()};
        if(r.url.ends_with("/ticket?progress=1"))return {200,R"({"state":"idle","playlist_id":"","queued_players":0,"match":null})"};
        return {200,body.dump()};
      });
      client.refresh("Chosen Pilot");
      const auto expected=variant<2?OnlineMatchmakingStatus::Ready:OnlineMatchmakingStatus::Failed;
      eventually([&]{return client.snapshot().status==expected;});
      if(variant<2){const auto p=client.snapshot().playlists.front();check(p.clan_match && p.ranked &&
        (variant==0?!p.level:p.level==50),"Clan catalog lost private membership/level semantics");}
    }
    for (const auto &body : {std::string("{\"playlists\":[],\"playlists\":[]}"), std::string(65537,' '), std::string("{\"playlists\":[{}]}")}) {
      OnlineMatchmaking client(login, options, [&](const auto &r, std::stop_token) -> OnlineMatchmakingResponse {
        if (r.url.ends_with("/session") && r.method == "POST") return {201,Json{{"token",session_token},{"expires_in",900}}.dump()};
        return {200,body};
      });
      client.refresh("Chosen Pilot"); eventually([&] { return client.snapshot().status == OnlineMatchmakingStatus::Failed; });
      check(client.snapshot().playlists.empty(), "Malformed catalog partially populated UI");
    }
    login->cancel();
    check(!login->authorization(), "Sign-out retained OAuth handoff");
    check(deletes >= 2, "Cancellation never reached queue owner");
    std::cout << "PASS pinned origin, existing Discord session, chosen name, trusted queue fields, uncertain POST idempotence, cancellation race, sign-out and bounded strict JSON\n";
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
