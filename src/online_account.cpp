#include "halo2/online_account.hpp"
#include "halo2/online_service.hpp"
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
void require(bool ok) { if (!ok) throw std::runtime_error("Invalid ONLINE response"); }
struct HttpError : std::runtime_error { using std::runtime_error::runtime_error; };
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
std::string request_id() {
  unsigned char bytes[16]; require(RAND_bytes(bytes, 16) == 1);
  bytes[6] = (bytes[6] & 15) | 64; bytes[8] = (bytes[8] & 63) | 128;
  constexpr char hex[] = "0123456789abcdef";
  std::string result;
  for (unsigned i = 0; i < 16; ++i) { if (i == 4 || i == 6 || i == 8 || i == 10) result += '-'; result += hex[bytes[i] >> 4]; result += hex[bytes[i] & 15]; }
  return result;
}
void parse_social(const Json &root, OnlineAccountSnapshot &s) {
  require(root.is_object() && root.size() == 2);
  auto invites = [](const Json &rows) {
    require(rows.is_array() && rows.size() <= 100);
    std::vector<OnlineClanInvite> out; std::set<std::string> ids;
    for (const auto &r : rows) {
      require(r.is_object() && r.size() == 2);
      auto id = text(r, "id", 36); require(ids.insert(id).second);
      out.push_back({id, text(r, "name")});
    }
    return out;
  };
  s.invitations = invites(root.at("invitations")); s.clan.reset();
  const auto &c = root.at("clan");
  if (c.is_null()) return;
  require(c.is_object() && c.size() == 6);
  OnlineClan clan; clan.id = text(c,"id",36); clan.name = text(c,"name",15); clan.role = number(c,"role",0,3);
  const auto &members = c.at("members"); require(members.is_array() && !members.empty() && members.size() <= 100);
  unsigned selves = 0, leaders = 0; std::set<std::string> ids;
  for (const auto &m : members) {
    require(m.is_object() && (m.size() == 4 || (m.size() == 5 && m.contains("presence"))));
    OnlineClanMember member{text(m,"id",36), text(m,"name"), number(m,"role",0,3), boolean(m,"self")};
    if(m.contains("presence")) {
      const auto value=text(m,"presence",16);
      require(value=="offline" || value=="online" || value=="playing");
      member.presence=value=="offline"?OnlineClanPresence::Offline:
          value=="online"?OnlineClanPresence::Online:OnlineClanPresence::Playing;
    }
    require(ids.insert(member.id).second);
    if (member.self) { ++selves; require(member.role == clan.role); }
    leaders += member.role == 3; clan.members.push_back(std::move(member));
  }
  require(selves == 1 && leaders >= 1);
  clan.outgoing = invites(c.at("outgoing"));
  require(c.at("messages").is_array() && c.at("messages").size() <= 20);
  for (const auto &m : c.at("messages")) {
    require(m.is_object() && m.size() == 3);
    clan.messages.push_back({text(m,"id",36), text(m,"name"), text(m,"body",256)});
  }
  s.clan = std::move(clan);
}
void social(const Json &root, OnlineAccountSnapshot &s) { auto next=s; parse_social(root,next); s=std::move(next); }
std::uint64_t version(const Json &r) {
  const auto &v=r.at("version");
  require(v.is_number_integer() && v>0 && v<=std::numeric_limits<std::int64_t>::max());
  return v.get<std::uint64_t>();
}
void parse_party(const Json &root, OnlineAccountSnapshot &s) {
  require(root.is_object() && root.size()==2);
  auto next=s;next.party.reset();next.party_invitations.clear();
  const auto &invitations=root.at("invitations");require(invitations.is_array() && invitations.size()<=100);
  std::set<std::string> ids;
  for(const auto &r:invitations) {
    require(r.is_object() && r.size()==3);
    auto id=text(r,"id",36);require(ids.insert(id).second);
    next.party_invitations.push_back({std::move(id),text(r,"name"),version(r)});
  }
  const auto &p=root.at("party");
  if(!p.is_null()) {
    require(p.is_object() && p.size()==4);
    OnlineParty party;party.version=version(p);party.leader=boolean(p,"leader");
    const auto &members=p.at("members");require(members.is_array() && !members.empty() && members.size()<=16);
    unsigned selves{},leaders{};ids.clear();
    for(const auto &r:members) {
      require(r.is_object() && (r.size()==6 || (r.size()==7 && r.contains("local_players"))));
      OnlinePartyMember member{text(r,"handle",36),text(r,"name"),text(r,"ready_playlist",64,true),
                               boolean(r,"self"),boolean(r,"leader"),boolean(r,"online")};
      require(ids.insert(member.handle).second);
      if(r.contains("local_players")) {
        const auto &locals=r.at("local_players");require(locals.is_array() && locals.size()<=3);
        for(const auto &local:locals){require(local.is_object() && local.size()==2);member.local_players.push_back({text(local,"name"),boolean(local,"guest")});}
      }
      if(member.self){++selves;require(member.leader==party.leader);}
      leaders+=member.leader;party.members.push_back(std::move(member));
    }
    require(selves==1 && leaders==1);
    const auto &outgoing=p.at("outgoing");require(outgoing.is_array() && outgoing.size()<=100);ids.clear();
    for(const auto &r:outgoing) {
      require(r.is_object() && r.size()==2);auto id=text(r,"id",36);require(ids.insert(id).second);
      party.outgoing.push_back({std::move(id),text(r,"name")});
    }
    require(party.leader || party.outgoing.empty());
    next.party=std::move(party);
  }
  s=std::move(next);
}
DiscordPartyLink parse_party_link(const Json &result,const OnlineParty &party) {
  require(result.is_object() && result.size()==6);
  DiscordPartyLink link;
  link.party_id=text(result,"party_id",36);link.join_secret=text(result,"join_secret",48);
  require(link.party_id.size()==36 && link.join_secret.size()==48 && link.join_secret.starts_with("h2p1_") &&
    std::all_of(link.join_secret.begin()+5,link.join_secret.end(),[](unsigned char v) {
      return (v>='A' && v<='Z') || (v>='a' && v<='z') || (v>='0' && v<='9') || v=='_' || v=='-';
    }));
  link.version=version(result);
  link.current_size=number(result,"current_size",1,15);link.max_size=number(result,"max_size",16,16);
  require(link.version==party.version && link.current_size==party.members.size());
  link.expires=Clock::now()+std::chrono::seconds(number(result,"expires_in",1,600));
  return link;
}
} // namespace
OnlineAccountOptions online_account_environment() {
  OnlineAccountOptions o;
  o.enable_parties=true;
  if (const auto *url = std::getenv("H2_ACCOUNT_URL")) o.endpoint = url;
  else o.endpoint = default_online_origin;
  if (const auto *http = std::getenv("H2_ACCOUNT_ALLOW_LOOPBACK_HTTP")) o.allow_loopback_http = std::string(http) == "1";
  return o;
}
struct OnlineAccount::Impl {
  std::shared_ptr<DiscordLogin> login;
  OnlineAccountOptions options;
  Http http;
  mutable std::mutex mutex;
  std::condition_variable_any wake;
  OnlineAccountSnapshot state;
  bool configured{}, active{}, pending{};
  std::string name;
  std::string voice_bearer;
  std::string service_subject;
  Clock::time_point voice_expires{};
  std::optional<Json> command;
  std::string command_subject;
  std::string command_path = "/v1/clans/commands";
  DiscordOperation discord_result;
  std::string discord_subject;
  // The current party capability outlives any individual recipient's send.
  // It stays private and bound to the authenticated Discord subject.
  std::optional<DiscordPartyLink> party_link;
  std::string party_link_subject;
  std::uint64_t next_discord_operation{}, queued_discord_operation{};
  bool queued_discord_issuer{};
  std::uint64_t generation{};
  std::stop_source cancellation;
  std::jthread worker;
  Impl(std::shared_ptr<DiscordLogin> l, OnlineAccountOptions o, Http h) : login(std::move(l)), options(std::move(o)), http(h ? std::move(h) : transport) {
    state.revision = 1;
    try {
      options.endpoint = origin(options.endpoint, options.allow_loopback_http);
      require(login && options.poll_interval >= std::chrono::milliseconds(10));
    } catch (...) { state.message = "ONLINE account service configuration is invalid."; return; }
    configured = true; state.message = "Sign in with "+std::string(login->uses_steam()?"Steam":"Discord")+".";
    worker = std::jthread([this](std::stop_token stop) { run(stop); });
  }
  ~Impl() { worker.request_stop(); { std::lock_guard lock(mutex); cancellation.request_stop(); } wake.notify_all(); if (worker.joinable()) worker.join(); }
  void connect(std::string value) {
    std::lock_guard lock(mutex);
    if (!configured || (active && name == value)) return;
    active = true; name = std::move(value); command.reset(); pending = true; ++generation;
    discord_result={};discord_subject.clear();queued_discord_operation=0;
    party_link.reset();party_link_subject.clear();
    voice_bearer.clear();
    service_subject.clear();
    cancellation.request_stop(); state = {false,true,"Verifying ONLINE identity...",{},{},state.revision+1,{},{},{}}; wake.notify_all();
  }
  void sign_out() {
    std::lock_guard lock(mutex);
    if (!active && !state.authenticated) return;
    active = false; pending = true; command.reset(); ++generation; cancellation.request_stop();
    discord_result={};discord_subject.clear();queued_discord_operation=0;
    party_link.reset();party_link_subject.clear();
    voice_bearer.clear();
    service_subject.clear();
    state = {false,false,"Signed out.",{},{},state.revision+1,{},{},{}}; wake.notify_all();
  }
  void publish(OnlineAccountSnapshot next, std::uint64_t version, const std::string &bearer = {}, Clock::time_point expires = {},
               std::optional<DiscordOperation> activity = {},const std::string &subject = {},
               std::optional<DiscordPartyLink> link = {}) {
    std::lock_guard lock(mutex);
    if (version != generation) return;
    const auto identity=login->service_identity();
    if(next.authenticated && (!identity || identity->subject!=subject)) {
      next={};next.message="Account identity changed. Reconnecting ONLINE...";
      if(activity) {activity->successful=false;activity->link.reset();}
    }
    voice_bearer = next.authenticated ? bearer : ""; voice_expires = expires;
    service_subject=next.authenticated?subject:std::string{};
    if(activity) {discord_result=std::move(*activity);discord_subject=next.authenticated?subject:std::string{};}
    if(!next.authenticated || !next.party || !next.party->leader || !link ||
       next.party->version!=link->version || next.party->members.size()!=link->current_size ||
       Clock::now()>=link->expires)link.reset();
    party_link=std::move(link);party_link_subject=party_link?subject:std::string{};
    next.revision = state.revision + 1; state = std::move(next);
  }
  void run(std::stop_token stop) {
    std::string session, session_name, session_subject;
    Clock::time_point expires{};
    auto cleanup = [&] { if (!session.empty()) try { http({"DELETE",options.endpoint+"/v1/account/session",{},session},{}); } catch (...) {} session.clear(); session_name.clear();session_subject.clear(); };
    while (!stop.stop_requested()) {
      std::unique_lock lock(mutex);
      if (!active) wake.wait(lock,stop,[&]{ return pending; });
      else wake.wait_for(lock,stop,options.poll_interval,[&]{ return pending; });
      if (stop.stop_requested()) break;
      const auto was_authenticated = state.authenticated; const auto enabled = active; const auto value = name; const auto version = generation;
      auto cmd = std::move(command); command.reset(); pending = false;
      const auto path=command_path;
      const auto requested_subject=command_subject;
      const auto operation=queued_discord_operation;queued_discord_operation=0;
      const auto issuer=queued_discord_issuer;
      const auto previous_activity=discord_result;
      const auto previous_activity_subject=discord_subject;
      auto link=party_link;
      const auto link_subject=party_link_subject;
      std::optional<DiscordOperation> activity;
      if(operation)activity=DiscordOperation{operation,false,false,{}};
      cancellation = std::stop_source{}; auto cancelled = cancellation.get_token();
      auto next = state; lock.unlock();
      if (!enabled) { cleanup(); continue; }
      try {
        auto call = [&](const std::string &method, const std::string &path, const std::string &body, const std::string &bearer) {
          auto response = http({method, options.endpoint+path, body, bearer}, cancelled);
          require(!cancelled.stop_requested());
          auto json = parse(response.body);
          if (response.status < 200 || response.status >= 300) {
            if (response.status == 401) { session.clear(); next.authenticated = false; }
            throw HttpError(text(json,"error",256));
          }
          return json;
        };
        const auto authorization = login->service_identity();
        if (!authorization) { cleanup(); next = {}; next.message = "Sign in with "+std::string(login->uses_steam()?"Steam":"Discord")+"."; publish(std::move(next),version,{}, {},activity); continue; }
        if(cmd && authorization->subject!=requested_subject) {
          cleanup();next={};throw HttpError("Account identity changed. Refresh ONLINE before trying again.");
        }
        if(link_subject!=authorization->subject)link.reset();
        if(!activity && previous_activity.serial && previous_activity_subject!=authorization->subject) {
          activity=previous_activity;activity->successful=false;activity->link.reset();
        }
        if (session.empty() || session_name != value || session_subject!=authorization->subject || Clock::now() >= expires) {
          cleanup();
          auto response = call("POST","/v1/account/session",Json{{"screen_name",value}}.dump(),authorization->bearer);
          require(response.size() == 2); session = text(response,"token",128); session_name = value;
          session_subject=authorization->subject;
          expires = Clock::now() + std::chrono::seconds(number(response,"expires_in",1,900)) - std::chrono::seconds(1);
        }
        if (cmd) {
          const auto current=login->service_identity();
          if(!current || current->subject!=requested_subject) {
            cleanup();next={};throw HttpError("Account identity changed. Refresh ONLINE before trying again.");
          }
          // The same command ID survives an uncertain network result. A second
          // submission is a replay on the server, never another mutation.
          auto consume=[&](const Json &r){if(path=="/v1/party/commands"){parse_party(r,next);next.party_message.clear();}else social(r,next);};
          try { consume(call("POST",path,cmd->dump(),session)); }
          catch (const HttpError &) { throw; }
          catch (const std::exception &) {
            if (cancelled.stop_requested()) throw;
            consume(call("POST",path,cmd->dump(),session));
          }
          if(activity) {
            if(issuer) {
              require(next.party && next.party->leader && next.party->members.size()<16);
              const auto old=link;
              link.reset();
              const auto result=call("POST","/v1/party/activity",Json{{"version",next.party->version}}.dump(),session);
              // The lease must still have time to survive the SDK publication.
              activity->link=parse_party_link(result,*next.party);
              if(old && old->party_id==activity->link->party_id && old->join_secret==activity->link->join_secret)
                activity->link->expires=std::min(activity->link->expires,old->expires);
              link=activity->link;
            }
            activity->successful=true;
          }
        } else social(call("GET","/v1/clans?presence=1",{},session),next);
        if(cmd && path=="/v1/party/commands")social(call("GET","/v1/clans?presence=1",{},session),next);
        if(options.enable_parties && (!cmd || path!="/v1/party/commands")) {
          try {parse_party(call("GET","/v1/party?local_players=1",{},session),next);next.party_message.clear();}
          catch(const HttpError &e){if(session.empty())throw;next.party_message=e.what();}
        }
        if(link &&
           next.party && next.party->leader && next.party->members.size()<16 &&
           Clock::now()<link->expires &&
           (next.party->version!=link->version ||
            next.party->members.size()!=link->current_size)) {
          // Refresh only this existing capability. A revocation must never
          // silently create a new Discord party/link or resend an invitation.
          const auto old=*link;link.reset();
          try {
            auto refreshed=parse_party_link(call("POST","/v1/party/activity",
                Json{{"version",next.party->version},{"join_secret",old.join_secret}}.dump(),session),*next.party);
            require(refreshed.party_id==old.party_id && refreshed.join_secret==old.join_secret);
            refreshed.expires=std::min(refreshed.expires,old.expires);
            link=std::move(refreshed);
          } catch (...) {if(session.empty())throw;}
        }
        next.authenticated = true; next.busy = false;
        if (cmd || !was_authenticated) next.message = cmd ? (path=="/v1/party/commands" ? "Party updated." : "Clan updated.") : "ONLINE connected.";
      } catch (const std::exception &e) {
        next.busy = false; next.message = e.what();
        if(activity || (cmd && cmd->value("action","")=="join_activity"))
          next.message="Discord party operation failed. Refresh ONLINE and try again.";
        if (next.message.size() > 256) next.message = "ONLINE connection failed. Retry shortly.";
        if(cmd && path=="/v1/party/commands")next.party_message=next.message;
      } catch (...) { next.busy = false; next.message = "ONLINE connection failed. Retry shortly."; }
      if (!cancelled.stop_requested()) publish(std::move(next),version,session,expires,std::move(activity),session_subject,std::move(link));
    }
    cleanup();
  }
  std::uint64_t party_command(OnlinePartyCommand, unsigned activity=0);
};
OnlineAccount::OnlineAccount(std::shared_ptr<DiscordLogin> l, OnlineAccountOptions o, Http h) : impl_(std::make_unique<Impl>(std::move(l),std::move(o),std::move(h))) {}
OnlineAccount::~OnlineAccount() = default;
void OnlineAccount::connect(const std::string &name) { impl_->connect(name); }
void OnlineAccount::sign_out() { impl_->sign_out(); }
OnlineAccountSnapshot OnlineAccount::snapshot() const { std::lock_guard lock(impl_->mutex); return impl_->state; }
std::optional<VoiceAuthorization> OnlineAccount::voice_authorization(bool party) const {
  std::lock_guard lock(impl_->mutex);
  const auto identity=impl_->login?impl_->login->service_identity():std::nullopt;
  if (!impl_->active || !impl_->state.authenticated ||
      impl_->voice_bearer.empty() || Clock::now() >= impl_->voice_expires || !identity || identity->subject!=impl_->service_subject) return {};
  if(party) {
    if(impl_->state.party)for(const auto &member:impl_->state.party->members)
      if(member.self)return VoiceAuthorization{impl_->options.endpoint,impl_->voice_bearer,"party",member.handle,impl_->options.allow_loopback_http,true};
    return {};
  }
  if(!impl_->state.clan)return {};
  for (const auto &member : impl_->state.clan->members)
    if (member.self) return VoiceAuthorization{impl_->options.endpoint,impl_->voice_bearer,impl_->state.clan->id,member.id,impl_->options.allow_loopback_http};
  return {};
}
bool OnlineAccount::command(OnlineClanCommand c) {
  std::lock_guard lock(impl_->mutex);
  if (!impl_->state.authenticated || impl_->state.busy || !impl_->active) return false;
  Json body{{"request_id",request_id()},{"action",c.action}};
  if (c.action == "create") body["name"] = c.value;
  else if (c.action == "invite") body["username"] = c.value;
  else if (c.action == "broadcast") body["message"] = c.value;
  else if (c.action != "leave") body["target"] = c.value;
  if (c.action == "role") { body["role"] = c.role; body["confirmed"] = c.confirmed; }
  impl_->command = std::move(body); impl_->pending = true; impl_->state.busy = true;
  impl_->command_subject=impl_->service_subject;
  // A previously started poll must not publish over this newer command and
  // clear busy before the worker consumes it. That allowed queued invites
  // to be replaced by another action submitted from the stale snapshot.
  ++impl_->generation;
  impl_->command_path="/v1/clans/commands?presence=1";
  impl_->state.message = "Updating clan..."; ++impl_->state.revision; impl_->wake.notify_all(); return true;
}
std::uint64_t OnlineAccount::Impl::party_command(OnlinePartyCommand c,unsigned activity) {
  std::lock_guard lock(mutex);
  if(!options.enable_parties || !state.authenticated || state.busy || !active)return 0;
  if(activity) {
    const auto identity=login->service_identity();
    if(!identity || identity->subject!=service_subject)return 0;
  }
  if(activity==1 && (!state.party || !state.party->leader || state.party->members.size()>=16))return 0;
  if(activity==1 && (c.value.empty() || c.value.size()>20 || !std::all_of(c.value.begin(),c.value.end(),[](unsigned char v){return v>='0' && v<='9';})))return 0;
  if(activity==2 && state.party)return 0;
  Json body{{"request_id",request_id()},{"action",c.action}};
  if(c.action=="join_activity") {
    if(c.version || !c.playlist.empty() || !c.revision.empty() || c.value.size()!=48 ||
       !c.value.starts_with("h2p1_") ||
       !std::all_of(c.value.begin()+5,c.value.end(),[](unsigned char v) {
         return (v>='A' && v<='Z') || (v>='a' && v<='z') || (v>='0' && v<='9') || v=='_' || v=='-';
       }))return 0;
    body["join_secret"]=c.value;
  }
  else if(c.action=="invite")body["username"]=c.value;
  else if(c.action=="invite_discord")body["discord_id"]=c.value;
  else if(c.action=="accept" || c.action=="decline" || c.action=="block" || c.action=="cancel" ||
          c.action=="remove" || c.action=="transfer")body["target"]=c.value;
  else if(c.action=="ready"){body["playlist_id"]=c.playlist;body["revision"]=c.revision;}
  if(c.action!="create" && c.action!="decline" && c.action!="block" && c.action!="join_activity") {
    const auto version=c.version ? c.version : state.party ? state.party->version : 0;
    if(!version)return 0;
    body["version"]=version;
  }
  command=std::move(body);command_path="/v1/party/commands";
  command_subject=service_subject;
  pending=true;state.busy=true;state.message="Updating party...";
  ++generation;
  if(activity) {
    queued_discord_operation=++next_discord_operation;queued_discord_issuer=activity==1;
    discord_result={queued_discord_operation,true,false,{}};
    discord_subject=service_subject;
  }
  // Target-specific invitations do not change the shared party capability.
  // Structural mutations revoke it immediately, before their HTTP response.
  if(c.action!="invite" && c.action!="invite_discord" && c.action!="cancel" && c.action!="decline" && c.action!="block") {
    party_link.reset();party_link_subject.clear();discord_result.link.reset();
  }
  ++state.revision;wake.notify_all();return activity?queued_discord_operation:1;
}
bool OnlineAccount::party_command(OnlinePartyCommand c) {return impl_->party_command(std::move(c))!=0;}
std::uint64_t OnlineAccount::begin_discord_invite(const std::string &subject) {
  return impl_->party_command({"invite_discord",subject,{},{},0},1);
}
std::uint64_t OnlineAccount::begin_discord_join(const std::string &secret) {
  return impl_->party_command({"join_activity",secret,{},{},0},2);
}
std::optional<OnlineAccount::DiscordOperation> OnlineAccount::discord_operation(std::uint64_t serial) const {
  std::lock_guard lock(impl_->mutex);
  const auto &r=impl_->discord_result;
  const auto identity=impl_->login?impl_->login->service_identity():std::nullopt;
  if(r.serial!=serial || !impl_->active || !impl_->state.authenticated || !identity ||
     identity->subject!=impl_->service_subject || identity->subject!=impl_->discord_subject)
    return DiscordOperation{serial,false,false,{}};
  if(r.link && (!impl_->state.party || !impl_->state.party->leader ||
       !impl_->party_link || impl_->party_link_subject!=identity->subject ||
       impl_->party_link->join_secret!=r.link->join_secret || impl_->party_link->party_id!=r.link->party_id ||
       impl_->state.party->version!=impl_->party_link->version ||
       impl_->state.party->members.size()!=impl_->party_link->current_size ||
       Clock::now()>=impl_->party_link->expires || Clock::now()>=impl_->voice_expires))
    return DiscordOperation{serial,false,false,{}};
  auto result=r;
  if(result.link)result.link=impl_->party_link;
  return result;
}
std::optional<DiscordPartyLink> OnlineAccount::discord_party_link() const {
  std::lock_guard lock(impl_->mutex);
  const auto &link=impl_->party_link;
  const auto identity=impl_->login?impl_->login->service_identity():std::nullopt;
  if(!link || !impl_->active || !impl_->state.authenticated || !identity ||
     identity->subject!=impl_->service_subject || identity->subject!=impl_->party_link_subject ||
     !impl_->state.party || !impl_->state.party->leader ||
     impl_->state.party->version!=link->version || impl_->state.party->members.size()!=link->current_size ||
     Clock::now()>=link->expires || Clock::now()>=impl_->voice_expires)return {};
  return link;
}
} // namespace halo2
