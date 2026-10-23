#include "halo2/online_matchmaking.hpp"
#include "halo2/online_service.hpp"
#include "halo2/online_search_qos.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <openssl/rand.h>
#include <algorithm>
#include <condition_variable>
#include <cctype>
#include <cstdlib>
#include <limits>
#include <mutex>
#include <set>
#include <stdexcept>
#include <thread>

namespace halo2 {
namespace {
using Json = nlohmann::json;
using Clock = std::chrono::steady_clock;
constexpr std::size_t max_body = 65536;
void require(bool ok) { if (!ok) throw std::runtime_error("Invalid matchmaking response"); }
struct StatusError { long status; std::string message{}; };
struct MatchConnectionError { std::string message; };
std::string origin(const std::string &value, bool local) {
  require(!value.empty() && value.size() <= 2048 && std::none_of(value.begin(), value.end(), [](unsigned char c) { return c <= 32 || c == 127 || c == '\\'; }));
  static const auto initialized = curl_global_init(CURL_GLOBAL_DEFAULT);
  require(initialized == CURLE_OK);
  std::unique_ptr<CURLU, decltype(&curl_url_cleanup)> url(curl_url(), curl_url_cleanup);
  require(url && curl_url_set(url.get(), CURLUPART_URL, value.c_str(), 0) == CURLUE_OK);
  auto part = [&](CURLUPart key) {
    char *raw = nullptr;
    const auto result = curl_url_get(url.get(), key, &raw, 0);
    std::unique_ptr<char, decltype(&curl_free)> owner(raw, curl_free);
    return result == CURLUE_OK && raw ? std::string(raw) : std::string{};
  };
  const auto scheme = part(CURLUPART_SCHEME), host = part(CURLUPART_HOST);
  require(!host.empty() && part(CURLUPART_USER).empty() && part(CURLUPART_PASSWORD).empty() && part(CURLUPART_OPTIONS).empty() &&
          part(CURLUPART_PATH) == "/" && value.find_first_of("@?#") == std::string::npos);
  require(scheme == "https" || (local && scheme == "http" && (host == "127.0.0.1" || host == "[::1]")));
  char *raw = nullptr;
  require(curl_url_get(url.get(), CURLUPART_URL, &raw, 0) == CURLUE_OK);
  std::unique_ptr<char, decltype(&curl_free)> owner(raw, curl_free);
  std::string result(raw);
  if (result.ends_with('/')) result.pop_back();
  return result;
}
struct Transfer { std::string body; std::stop_token stop; };
std::size_t receive(char *data, std::size_t size, std::size_t count, void *context) {
  auto &t = *static_cast<Transfer *>(context);
  if (t.stop.stop_requested() || (count && size > std::numeric_limits<std::size_t>::max() / count)) return 0;
  const auto bytes = size * count;
  if (bytes > max_body - t.body.size()) return 0;
  try { t.body.append(data, bytes); } catch (...) { return 0; }
  return bytes;
}
int progress(void *context, curl_off_t, curl_off_t, curl_off_t, curl_off_t) {
  return static_cast<Transfer *>(context)->stop.stop_requested() ? 1 : 0;
}
OnlineMatchmakingResponse transport(const OnlineMatchmakingRequest &r, std::stop_token stop) {
  std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> client(curl_easy_init(), curl_easy_cleanup);
  require(bool(client));
  const auto authorization = "Authorization: Bearer " + r.bearer;
  curl_slist *raw = curl_slist_append(nullptr, authorization.c_str());
  raw = curl_slist_append(raw, "Content-Type: application/json");
  std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)> headers(raw, curl_slist_free_all);
  Transfer t{{}, stop};
  curl_easy_setopt(client.get(), CURLOPT_URL, r.url.c_str());
  curl_easy_setopt(client.get(), CURLOPT_HTTPHEADER, headers.get());
  curl_easy_setopt(client.get(), CURLOPT_CUSTOMREQUEST, r.method.c_str());
  if (!r.body.empty()) { curl_easy_setopt(client.get(), CURLOPT_POSTFIELDS, r.body.c_str()); curl_easy_setopt(client.get(), CURLOPT_POSTFIELDSIZE, long(r.body.size())); }
  curl_easy_setopt(client.get(), CURLOPT_PROTOCOLS_STR, "https,http");
  curl_easy_setopt(client.get(), CURLOPT_FOLLOWLOCATION, 0L);
  curl_easy_setopt(client.get(), CURLOPT_PROXY, "");
  curl_easy_setopt(client.get(), CURLOPT_NOSIGNAL, 1L);
  curl_easy_setopt(client.get(), CURLOPT_CONNECTTIMEOUT_MS, 2000L);
  curl_easy_setopt(client.get(), CURLOPT_TIMEOUT_MS, 3000L);
  curl_easy_setopt(client.get(), CURLOPT_SSL_VERIFYPEER, 1L);
  curl_easy_setopt(client.get(), CURLOPT_SSL_VERIFYHOST, 2L);
  curl_easy_setopt(client.get(), CURLOPT_ACCEPT_ENCODING, "identity");
  curl_easy_setopt(client.get(), CURLOPT_HTTP_CONTENT_DECODING, 0L);
  curl_easy_setopt(client.get(), CURLOPT_WRITEFUNCTION, receive);
  curl_easy_setopt(client.get(), CURLOPT_WRITEDATA, &t);
  curl_easy_setopt(client.get(), CURLOPT_NOPROGRESS, 0L);
  curl_easy_setopt(client.get(), CURLOPT_XFERINFOFUNCTION, progress);
  curl_easy_setopt(client.get(), CURLOPT_XFERINFODATA, &t);
  require(curl_easy_perform(client.get()) == CURLE_OK && !stop.stop_requested());
  long status{};
  curl_easy_getinfo(client.get(), CURLINFO_RESPONSE_CODE, &status);
  return {status, std::move(t.body)};
}
Json parse(const std::string &body) {
  require(body.size() <= max_body);
  std::vector<std::set<std::string>> keys;
  return Json::parse(body, [&](int depth, Json::parse_event_t event, Json &v) {
    require(depth <= 16);
    if (event == Json::parse_event_t::object_start) keys.emplace_back();
    if (event == Json::parse_event_t::key) require(!keys.empty() && keys.back().insert(v.get<std::string>()).second);
    if (event == Json::parse_event_t::object_end) keys.pop_back();
    return true;
  });
}
std::string text(const Json &row, const char *key, std::size_t max = 64, bool empty = false) {
  const auto &v = row.at(key);
  require(v.is_string());
  auto s = v.get<std::string>();
  require((empty || !s.empty()) && s.size() <= max && std::none_of(s.begin(), s.end(), [](unsigned char c) { return c < 32 || c == 127; }));
  return s;
}
unsigned number(const Json &row, const char *key, unsigned low, unsigned high) {
  const auto &v = row.at(key);
  require(v.is_number_integer() && v >= low && v <= high);
  return v.get<unsigned>();
}
bool boolean(const Json &row, const char *key) { require(row.at(key).is_boolean()); return row.at(key).get<bool>(); }
std::vector<OnlinePlaylist> catalog(const Json &root) {
  require(root.is_object() && root.size() == 1);
  const auto &rows = root.at("playlists");
  require(rows.is_array() && rows.size() <= 64);
  std::vector<OnlinePlaylist> result;
  std::set<std::string> ids;
  unsigned defaults = 0;
  for (const auto &row : rows) {
    require(row.is_object() && (row.size() == 10 || (row.size() == 11 && row.contains("clan_match"))));
    OnlinePlaylist p;
    p.id = text(row, "id"); p.revision = text(row, "revision"); p.name = text(row, "name"); p.description = text(row, "description", 256);
    require(ids.insert(p.id).second);
    p.ranked = boolean(row, "ranked"); p.quickmatch = boolean(row, "quickmatch"); p.eligible = boolean(row, "eligible");
    p.clan_match = row.contains("clan_match") && boolean(row, "clan_match");
    require(!p.clan_match || p.ranked);
    defaults += p.quickmatch;
    p.minimum_players = number(row, "minimum_players", 2, 16); p.maximum_players = number(row, "maximum_players", p.minimum_players, 16);
    if (p.ranked && !(p.clan_match && row.at("level").is_null())) p.level = number(row, "level", 1, 50);
    else require(row.at("level").is_null());
    require(!p.clan_match || p.level || !p.eligible);
    result.push_back(std::move(p));
  }
  require(defaults <= 1);
  return result;
}
void ticket(const Json &root, OnlineMatchmakingSnapshot &s) {
  require(root.is_object() && (root.size() == 4 || (root.size() == 5 && root.contains("progress"))));
  const auto state = text(root, "state");
  s.playlist_id = text(root, "playlist_id", 64, true);
  s.queued_players = number(root, "queued_players", 0, 1000);
  s.match.reset();
  s.gathering.reset();
  if (root.contains("progress") && !root.at("progress").is_null()) {
    require(state == "searching");
    const auto &p = root.at("progress");
    require(p.is_object() && p.size() == 5);
    OnlineGatheringProgress gathering;
    const auto count = number(p, "players", 1, 16);
    gathering.minimum_players = number(p, "minimum_players", 2, 16);
    gathering.maximum_players = number(p, "maximum_players", gathering.minimum_players, 16);
    require(count <= gathering.maximum_players && count <= s.queued_players);
    if (!p.at("wait_ms").is_null()) {
      gathering.wait_ms = number(p, "wait_ms", 1, 0x7fffffff);
      require(count >= gathering.minimum_players);
    }
    const auto &rows = p.at("roster");
    require(rows.is_array() && rows.size() == count);
    unsigned selves = 0;
    for (const auto &row : rows) {
      require(row.is_object() && row.size() == 2);
      gathering.players.push_back({text(row, "name"), boolean(row, "self")});
      selves += gathering.players.back().self;
    }
    require(selves == 1);
    s.gathering = std::move(gathering);
  }
  if (state == "idle") { require(s.playlist_id.empty() && root.at("match").is_null()); s.status = OnlineMatchmakingStatus::Ready; s.message = "Choose a playlist."; return; }
  require(!s.playlist_id.empty());
  if (state == "searching") { s.status = OnlineMatchmakingStatus::Searching; s.message = "Searching for players..."; require(root.at("match").is_null()); return; }
  require(state == "checking_connections" || state == "awaiting_transport");
  const auto &m = root.at("match");
  require(m.is_object() && m.size() == 3);
  OnlineMatch match;
  match.map = text(m, "map"); match.variant = text(m, "variant");
  const auto &players = m.at("players");
  require(players.is_array() && players.size() >= 2 && players.size() <= 16);
  unsigned selves = 0;
  for (const auto &p : players) {
    require(p.is_object() && p.size() == 3);
    match.players.push_back({text(p, "name"), number(p, "team", 0, 15), boolean(p, "self")});
    selves += match.players.back().self;
  }
  require(selves == 1);
  s.match = std::move(match);
  s.status = state == "checking_connections" ? OnlineMatchmakingStatus::CheckingConnections : OnlineMatchmakingStatus::AwaitingTransport;
  s.message = state == "checking_connections" ? "Players found. Checking connections..." : "Match found. Connecting to game...";
}
std::string request_id() {
  unsigned char bytes[16]; require(RAND_bytes(bytes, 16) == 1);
  bytes[6] = (bytes[6] & 15) | 64; bytes[8] = (bytes[8] & 63) | 128;
  constexpr char hex[] = "0123456789abcdef";
  std::string result;
  for (unsigned i = 0; i < 16; ++i) { if (i == 4 || i == 6 || i == 8 || i == 10) result += '-'; result += hex[bytes[i] >> 4]; result += hex[bytes[i] & 15]; }
  return result;
}
} // namespace
OnlineMatchmakingOptions online_matchmaking_environment(const std::filesystem::path &maps) {
  OnlineMatchmakingOptions result;
  if (const auto *url = std::getenv("H2_MATCHMAKING_URL")) result.endpoint = url;
  else result.endpoint = default_online_origin;
  if(online_protocol_bridge::eos_runtime_options().transport)
    result.compatibility="halo2-native-queue-v14";
  if (const auto *http = std::getenv("H2_MATCHMAKING_ALLOW_LOOPBACK_HTTP")) result.allow_loopback_http = std::string(http) == "1";
  std::error_code error;
  for (std::filesystem::directory_iterator it(maps, error), end; !error && it != end; it.increment(error))
    if (it->is_regular_file(error) && it->path().extension() == ".map") result.maps.push_back(it->path().stem().string());
  std::sort(result.maps.begin(), result.maps.end());
  return result;
}
struct OnlineMatchmaking::Impl {
  std::shared_ptr<DiscordLogin> login;
  OnlineMatchmakingOptions options;
  Http http;
  mutable std::mutex mutex;
  std::condition_variable_any wake;
  OnlineMatchmakingSnapshot state;
  std::shared_ptr<OnlineMatchSession> match_session;
  bool transferred{};
  bool active{}, pending{}, configured{};
  std::string name, desired;
  std::vector<OnlineLocalPlayer> local_players;
  std::uint64_t generation{};
  std::stop_source request_stop;
  std::jthread worker;
  Impl(std::shared_ptr<DiscordLogin> l, OnlineMatchmakingOptions o, Http h) : login(std::move(l)), options(std::move(o)), http(h ? std::move(h) : transport) {
    state.revision = 1; state.message = "No matchmaking service configured.";
    if (options.endpoint.empty()) return;
    try {
      options.endpoint = origin(options.endpoint, options.allow_loopback_http);
      require(login && !options.maps.empty() && options.maps.size() <= 256 && options.poll_interval >= std::chrono::milliseconds(10) && options.language <= 8);
      const auto capabilities = parse(Json{{"maps", options.maps}, {"compatibility", options.compatibility}}.dump());
      text(capabilities, "compatibility");
      std::set<std::string> unique;
      for (const auto &m : options.maps) { text(Json{{"m", m}}, "m"); require(unique.insert(m).second); }
    } catch (...) { state.status = OnlineMatchmakingStatus::Failed; state.message = "Matchmaking configuration is invalid."; return; }
    configured = true; state.status = OnlineMatchmakingStatus::Idle; state.message = "Choose a playlist to connect.";
    worker = std::jthread([this](std::stop_token stop) { run(stop); });
  }
  ~Impl() {
    worker.request_stop();
    { std::lock_guard lock(mutex); request_stop.request_stop(); }
    wake.notify_all(); if (worker.joinable()) worker.join();
  }
  void publish(OnlineMatchmakingSnapshot next, std::uint64_t version) {
    std::lock_guard lock(mutex);
    if (version != generation) return;
    next.revision = state.revision + 1; state = std::move(next);
  }
  void command(bool enabled, std::string screen_name, std::string playlist,
               std::vector<OnlineLocalPlayer> additional = {}) {
    std::lock_guard lock(mutex);
    if (!configured) return;
    active = enabled; name = std::move(screen_name); desired = std::move(playlist);
    local_players=std::move(additional);
    match_session.reset(); transferred=false;
    ++generation; pending = true; request_stop.request_stop();
    state.status = enabled ? (desired.empty() ? OnlineMatchmakingStatus::Connecting : OnlineMatchmakingStatus::Joining) : OnlineMatchmakingStatus::Idle;
    state.message = enabled ? "Connecting to matchmaking..." : "Signed out of matchmaking.";
    state.playlist_id = desired; state.match.reset(); state.gathering.reset(); state.queued_players = 0;
    if (!enabled) state.playlists.clear();
    ++state.revision; wake.notify_all();
  }
  void run(std::stop_token stop) {
    std::string session, session_name, id;
    std::unique_ptr<OnlineSearchQos> qos;
    Clock::time_point expires{};
    bool queued = false, accepted = false;
    std::uint64_t previous = std::numeric_limits<std::uint64_t>::max();
    auto cleanup = [&](const char *path) {
      if (!session.empty()) try { http({"DELETE", options.endpoint + path, {}, session}, {}); } catch (...) {}
    };
    while (!stop.stop_requested()) {
      std::unique_lock lock(mutex);
      if (!active) wake.wait(lock, stop, [&] { return pending; });
      else wake.wait_for(lock, stop, options.poll_interval, [&] { return pending; });
      if (stop.stop_requested()) break;
      pending = false;
      const auto version = generation;
      const bool enabled = active;
      const auto screen_name = name, playlist = desired;
      const auto additional=local_players;
      auto next = state;
      request_stop = std::stop_source{};
      auto source = request_stop;
      lock.unlock();
      std::stop_callback shutdown(stop, [source]() mutable { source.request_stop(); });
      const auto token = source.get_token();
      const bool changed = version != previous;
      if (changed) {
        if (queued) cleanup("/v1/matchmaking/ticket");
        queued = false; accepted = false; id.clear(); previous = version;
      }
      if (!enabled || !login->authorization()) {
        qos.reset();
        cleanup("/v1/matchmaking/session"); session.clear();
        next = {}; next.status = OnlineMatchmakingStatus::Idle; next.message = "Sign in with Discord to use matchmaking.";
        publish(std::move(next), version); continue;
      }
      try {
        auto call = [&](const std::string &method, const char *path, const Json &body = Json()) {
          require(!token.stop_requested());
          const auto r = http({method, options.endpoint + path, body.is_null() ? std::string{} : body.dump(), session}, token);
          if (r.status != 200 && r.status != 201) {
            const auto error=parse(r.body);
            throw StatusError{r.status,error.is_object() && error.contains("error")?text(error,"error",256):std::string{}};
          }
          return parse(r.body);
        };
        if (session.empty() || session_name != screen_name || Clock::now() >= expires) {
          qos.reset();
          cleanup("/v1/matchmaking/session"); session.clear(); queued = false; accepted = false; id.clear();
          const auto authorization = login->authorization(); require(authorization.has_value());
          text(Json{{"name", screen_name}}, "name");
          auto session_body = Json{{"screen_name", screen_name}, {"maps", options.maps}, {"compatibility", options.compatibility}};
          const bool direct=options.compatibility=="halo2-native-queue-v12"||options.compatibility=="halo2-native-queue-v13"||options.compatibility=="halo2-native-queue-v14";
          const bool gathering=options.compatibility=="halo2-native-queue-v8" || options.compatibility=="halo2-native-queue-v9" || options.compatibility=="halo2-native-queue-v10" || direct;
          if(gathering)session_body["language"]=options.language;
          require(additional.size()<=3);
          if(!additional.empty()) {
            require(options.compatibility=="halo2-native-queue-v9" || options.compatibility=="halo2-native-queue-v10" || direct);
            session_body["local_players"]=Json::array();
            for(const auto &player:additional) {
              text(Json{{"name",player.name}},"name");
              Json row={{"screen_name",player.name}};
              if(player.login) {
                const auto grant=player.login->authorization();require(grant.has_value());
                row[player.login->uses_steam()?"identity_token":"discord_token"]=*grant;
              }
              session_body["local_players"].push_back(std::move(row));
            }
          }
          const auto r = http({"POST", options.endpoint + "/v1/matchmaking/session",
              session_body.dump(), *authorization}, token);
          if (r.status != 201) throw StatusError{r.status};
          const auto body = parse(r.body); require(body.is_object() && body.size() == 2);
          session = text(body, "token", 128);
          require(session.size() >= 32 && std::all_of(session.begin(), session.end(), [](unsigned char c) { return std::isalnum(c) || c == '-' || c == '_'; }));
          expires = Clock::now() + std::chrono::seconds(number(body, "expires_in", 1, 900)); session_name = screen_name;
          if(gathering)qos=std::make_unique<OnlineSearchQos>(options.endpoint,session,options.allow_loopback_http);
          next.playlists.clear();
        }
        if (changed || next.playlists.empty()) next.playlists = catalog(call("GET", "/v1/matchmaking/playlists"));
        if (playlist.empty() && !accepted) {
          // Accepted party members observe their leader's queue without
          // issuing a second enqueue or submitting a client-owned roster.
          ticket(call("GET", "/v1/matchmaking/ticket?progress=1"), next);
          if(next.status!=OnlineMatchmakingStatus::Ready)queued=accepted=true;
        } else if (!accepted) {
          const auto entry = std::find_if(next.playlists.begin(), next.playlists.end(), [&](const auto &p) { return p.id == playlist; });
          require(entry != next.playlists.end() && entry->eligible);
          if (id.empty()) id = request_id();
          // Retry an uncertain POST with the same key. A new command cancels
          // the old ticket before accepting another queue, even after a race.
          queued = true;
          ticket(call("POST", "/v1/matchmaking/ticket", {{"playlist_id", playlist}, {"revision", entry->revision}, {"request_id", id}}), next);
          accepted = true;
        } else {
          std::shared_ptr<OnlineMatchSession> relay;
          bool handed_off;
          { std::lock_guard guard(mutex); relay=match_session; handed_off=transferred; }
          if (handed_off) continue;
          if (relay) {
            if(relay->phase()==OnlineMatchSession::Phase::Failed) {
              const auto failure=relay->error();
              {std::lock_guard guard(mutex);match_session.reset();}
              cleanup("/v1/matchmaking/ticket");queued=false;accepted=false;id.clear();
              throw MatchConnectionError{failure};
            }
            next.status=relay->phase()==OnlineMatchSession::Phase::Connecting ? OnlineMatchmakingStatus::CheckingConnections : OnlineMatchmakingStatus::Loading;
            next.message=next.status==OnlineMatchmakingStatus::Loading ? "Loading ONLINE match..." : "Checking connections...";
            next.gathering.reset();
          } else ticket(call("GET", "/v1/matchmaking/ticket?progress=1"), next);
          if (next.status == OnlineMatchmakingStatus::Ready) { queued = false; accepted = false; id.clear(); }
        }
        if(next.match && (next.status==OnlineMatchmakingStatus::CheckingConnections || next.status==OnlineMatchmakingStatus::AwaitingTransport)) {
          bool missing; { std::lock_guard guard(mutex); missing=!match_session; }
          if(missing) {
            const auto descriptor=call("GET","/v1/matchmaking/transport");
            const bool eos=options.compatibility=="halo2-native-queue-v13"||options.compatibility=="halo2-native-queue-v14";
            const bool direct=options.compatibility=="halo2-native-queue-v12"||eos;
            const bool couch=descriptor.contains("local_slots");
            require(descriptor.is_object() && descriptor.size()==(direct?7u:couch?(options.compatibility=="halo2-native-queue-v10"?6u:5u):4u));
            OnlineMatchTransportOptions transport;
            if(direct) {
              require(couch && descriptor.contains("transport"));
              const auto &value=descriptor.at("transport");
              transport.direct_udp=true;
              if(eos){
                require(value.is_object()&&value.size()==4&&text(value,"kind")=="eos-p2p-v1"&&
                    value.at("peers").is_array()&&value.at("peers").size()<=15);
                OnlineEosOptions config;config.socket=text(value,"socket",32);
                config.account_login=options.compatibility=="halo2-native-queue-v14";
                const auto &session=value.at("session");require(session.is_object()&&session.size()==4);
                config.sessions=true;config.playlist=text(session,"playlist");config.revision=text(session,"revision");
                config.map=text(session,"map");config.variant=text(session,"variant");
                require(config.playlist==next.playlist_id&&config.map==next.match->map&&config.variant==next.match->variant);
                for(const auto &peer:value.at("peers")){
                  require(peer.is_object()&&peer.size()==2);
                  config.peers.push_back({number(peer,"slot",0,15),text(peer,"key",64)});
                }
                if(const auto *force=std::getenv("H2_EOS_FORCE_RELAY"))config.force_relay=std::string(force)=="1";
                transport.eos=std::move(config);
              }else{
                require(value.is_object() && value.size()==2 && text(value,"kind")=="direct-udp-v1" &&
                    value.at("stun_servers").is_array() && value.at("stun_servers").size()<=4);
                for(const auto &url:value.at("stun_servers")) {
                  require(url.is_string());transport.stun_servers.push_back(url.get<std::string>());
                }
              }
            }
            require(additional.empty() || couch);
            const auto &rows=descriptor.at("players");
            require(rows.is_array() && rows.size()==next.match->players.size());
            std::vector<OnlineAdmission> roster;
            for(unsigned i=0;i<rows.size();++i) {
              require(rows[i].is_object() && rows[i].size()==(couch?5u:3u));
              roster.push_back({i,text(rows[i],"name"),int(number(rows[i],"team",0,15))});
              if(couch) {
                roster.back().peer=number(rows[i],"peer",0,unsigned(rows.size()-1));
                roster.back().guest=boolean(rows[i],"guest");
              }
              const auto identity=text(rows[i],"identity");
              require(identity.size()==16 && identity.find_first_not_of("0123456789abcdef")==std::string::npos);
              for(unsigned j=0;j<8;++j)roster.back().identity[j]=std::byte(std::stoul(identity.substr(j*2,2),nullptr,16));
              require(roster.back().name==next.match->players[i].name && unsigned(roster.back().team)==next.match->players[i].team);
            }
            const auto self=number(descriptor,"slot",0,unsigned(roster.size()-1));
            require(next.match->players[self].self);
            if(couch) {
              const auto &slots=descriptor.at("local_slots");
              require(slots.is_array() && slots.size()==1+additional.size() && !slots.empty());
              std::set<unsigned> local;
              for(unsigned i=0;i<slots.size();++i) {
                require(slots[i].is_number_unsigned() || slots[i].is_number_integer());
                const auto slot=number(Json{{"slot",slots[i]}},"slot",0,unsigned(roster.size()-1));
                require(local.insert(slot).second && online_admission_peer(roster[slot])==self);
                require(roster[slot].name==(i?additional[i-1].name:screen_name));
                require(roster[slot].guest==(i && !additional[i-1].login));
              }
              require(slots[0]==self && local.contains(self));
              for(const auto &p:roster)require((online_admission_peer(p)==self)==local.contains(p.slot));
            }
            auto relay=std::make_shared<OnlineMatchSession>(options.endpoint,text(descriptor,"id"),text(descriptor,"token",128),self,std::move(roster),options.allow_loopback_http,descriptor.contains("team_mode")?std::optional(boolean(descriptor,"team_mode")):std::nullopt,false,std::move(transport));
            std::lock_guard guard(mutex); if(version==generation)match_session=std::move(relay);
          }
        }
        publish(std::move(next), version);
      } catch (const MatchConnectionError &e) {
        next.status=OnlineMatchmakingStatus::Failed;next.match.reset();next.gathering.reset();
        next.message=e.message.empty()?"The player connection failed. Retrying...":e.message;
        publish(std::move(next),version);
      } catch (const StatusError &e) {
        if (e.status == 401) { qos.reset(); session.clear(); queued = false; accepted = false; id.clear(); }
        if (e.status == 409) { cleanup("/v1/matchmaking/ticket"); queued = false; accepted = false; id.clear(); next.playlists.clear(); }
        next.status = OnlineMatchmakingStatus::Failed; next.match.reset(); next.gathering.reset();
        next.message = e.status == 503 ? "Matchmaking playlists are not configured on this service." : e.status == 401 ? "Matchmaking authorization expired. Reconnecting..." : "Matchmaking request failed. Reconnecting...";
        if(!e.message.empty() && e.status!=401)next.message=e.message;
        publish(std::move(next), version);
      } catch (...) {
        next.status = OnlineMatchmakingStatus::Failed; next.match.reset(); next.gathering.reset(); next.message = "Matchmaking service unavailable. Retrying...";
        publish(std::move(next), version);
      }
    }
    qos.reset();
    cleanup("/v1/matchmaking/session");
  }
};
OnlineMatchmaking::OnlineMatchmaking(std::shared_ptr<DiscordLogin> l, OnlineMatchmakingOptions o, Http h) : impl_(std::make_unique<Impl>(std::move(l), std::move(o), std::move(h))) {}
OnlineMatchmaking::~OnlineMatchmaking() = default;
void OnlineMatchmaking::refresh(const std::string &name, std::vector<OnlineLocalPlayer> additional) { impl_->command(true, name, {},std::move(additional)); }
void OnlineMatchmaking::join(const std::string &playlist) {
  std::string name;std::vector<OnlineLocalPlayer> additional;
  { std::lock_guard lock(impl_->mutex); name = impl_->name;additional=impl_->local_players; }
  impl_->command(true, std::move(name), playlist,std::move(additional));
}
void OnlineMatchmaking::cancel_queue() { std::string name;std::vector<OnlineLocalPlayer> additional; { std::lock_guard lock(impl_->mutex); name = impl_->name;additional=impl_->local_players; } impl_->command(true, std::move(name), {},std::move(additional)); }
void OnlineMatchmaking::sign_out() { impl_->command(false, {}, {}); }
OnlineMatchmakingSnapshot OnlineMatchmaking::snapshot() const { std::lock_guard lock(impl_->mutex); return impl_->state; }
std::optional<OnlineMatchmakingSnapshot> OnlineMatchmaking::snapshot_after(std::uint64_t revision) const {
  std::lock_guard lock(impl_->mutex); if (impl_->state.revision == revision) return {}; return impl_->state;
}
std::shared_ptr<OnlineMatchSession> OnlineMatchmaking::take_launch() {
  std::lock_guard lock(impl_->mutex);
  if(!impl_->match_session || impl_->transferred || impl_->match_session->phase()!=OnlineMatchSession::Phase::Loading)return {};
  impl_->transferred=true;
  return std::move(impl_->match_session);
}
} // namespace halo2
