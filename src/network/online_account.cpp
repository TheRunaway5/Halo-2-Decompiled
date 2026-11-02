#include "halo2/online_account.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include "halo2/online_service.hpp"
#include <bit>
#include <condition_variable>
#include <cstdlib>
#include <curl/curl.h>
#include <exception>
#include <limits>
#include <mutex>
#include <nlohmann/json.hpp>
#include <openssl/rand.h>
#include <stdexcept>
#include <thread>
namespace halo2 {
namespace {
namespace bridge = online_protocol_bridge;
using Json = nlohmann::json;
using Clock = std::chrono::steady_clock;
void require(bool ok) {
  if (!ok)
    throw std::runtime_error("Invalid ONLINE response");
}
struct Transfer {
  std::string body;
  std::stop_token stop;
  std::size_t limit;
};
std::size_t receive(char *data, std::size_t size, std::size_t count,
                    void *context) {
  auto &t = *static_cast<Transfer *>(context);
  if (t.stop.stop_requested() ||
      (count && size > std::numeric_limits<std::size_t>::max() / count))
    return 0;
  const auto bytes = size * count;
  if (bytes > t.limit - t.body.size())
    return 0;
  try {
    t.body.append(data, bytes);
  } catch (...) {
    return 0;
  }
  return bytes;
}
int progress(void *context, curl_off_t, curl_off_t, curl_off_t, curl_off_t) {
  return static_cast<Transfer *>(context)->stop.stop_requested() ? 1 : 0;
}
OnlineMatchmakingResponse transport(const OnlineMatchmakingRequest &r,
                                    std::stop_token stop) {
  std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> client(curl_easy_init(),
                                                             curl_easy_cleanup);
  require(bool(client));
  const auto authorization = "Authorization: Bearer " + r.bearer;
  curl_slist *raw = curl_slist_append(nullptr, authorization.c_str());
  raw = curl_slist_append(raw, "Content-Type: application/json");
  std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)> headers(
      raw, curl_slist_free_all);
  Transfer t{
      {},
      stop,
      online_protocol_bridge::account_http_response_limit(r.method, r.url)};
  curl_easy_setopt(client.get(), CURLOPT_URL, r.url.c_str());
  curl_easy_setopt(client.get(), CURLOPT_HTTPHEADER, headers.get());
  curl_easy_setopt(client.get(), CURLOPT_CUSTOMREQUEST, r.method.c_str());
  if (!r.body.empty()) {
    curl_easy_setopt(client.get(), CURLOPT_POSTFIELDS, r.body.c_str());
    curl_easy_setopt(client.get(), CURLOPT_POSTFIELDSIZE, long(r.body.size()));
  }
  curl_easy_setopt(client.get(), CURLOPT_PROTOCOLS_STR, "https,http");
  curl_easy_setopt(client.get(), CURLOPT_FOLLOWLOCATION, 0L);
  curl_easy_setopt(client.get(), CURLOPT_PROXY, "");
  curl_easy_setopt(client.get(), CURLOPT_NOSIGNAL, 1L);
  curl_easy_setopt(client.get(), CURLOPT_CONNECTTIMEOUT_MS, 2000L);
  curl_easy_setopt(
      client.get(), CURLOPT_TIMEOUT_MS,
      static_cast<long>(online_protocol_bridge::account_http_timeout_ms(
          r.method, r.url, r.body)));
  curl_easy_setopt(client.get(), CURLOPT_SSL_VERIFYPEER, 1L);
  curl_easy_setopt(client.get(), CURLOPT_SSL_VERIFYHOST, 2L);
  curl_easy_setopt(client.get(), CURLOPT_ACCEPT_ENCODING, "identity");
  curl_easy_setopt(client.get(), CURLOPT_HTTP_CONTENT_DECODING, 0L);
  curl_easy_setopt(client.get(), CURLOPT_WRITEFUNCTION, receive);
  curl_easy_setopt(client.get(), CURLOPT_WRITEDATA, &t);
  curl_easy_setopt(client.get(), CURLOPT_NOPROGRESS, 0L);
  curl_easy_setopt(client.get(), CURLOPT_XFERINFOFUNCTION, progress);
  curl_easy_setopt(client.get(), CURLOPT_XFERINFODATA, &t);
  // Return a complete bounded reply even if cancellation raced its completion.
  // Rust owns cancellation and can retire a session grant from that reply.
  require(curl_easy_perform(client.get()) == CURLE_OK);
  long status{};
  curl_easy_getinfo(client.get(), CURLINFO_RESPONSE_CODE, &status);
  return {status, std::move(t.body)};
}
// URL parsing is native Curl IO. Rust alone admits the resulting origin.
bridge::MatchmakingClientConfig config(const OnlineAccountOptions &o,
                                       bool login) {
  bridge::MatchmakingClientConfig c{};
  c.input_valid = true;
  c.loopback = o.allow_loopback_http;
  c.poll_ms = o.poll_interval.count();
  c.login_present = login;
  try {
    c.endpoint = o.endpoint;
    static const auto initialized = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (initialized != CURLE_OK)
      return c;
    std::unique_ptr<CURLU, decltype(&curl_url_cleanup)> url(curl_url(),
                                                            curl_url_cleanup);
    if (!url || curl_url_set(url.get(), CURLUPART_URL, o.endpoint.c_str(), 0) !=
                    CURLUE_OK)
      return c;
    const auto part = [&](CURLUPart key) {
      char *raw = nullptr;
      const auto result = curl_url_get(url.get(), key, &raw, 0);
      std::unique_ptr<char, decltype(&curl_free)> owner(raw, curl_free);
      return result == CURLUE_OK && raw ? std::string(raw) : std::string{};
    };
    c.url_parsed = true;
    c.url_normalized = part(CURLUPART_URL);
    c.url_scheme = part(CURLUPART_SCHEME);
    c.url_host = part(CURLUPART_HOST);
    c.url_user = part(CURLUPART_USER);
    c.url_password = part(CURLUPART_PASSWORD);
    c.url_options = part(CURLUPART_OPTIONS);
    c.url_path = part(CURLUPART_PATH);
  } catch (...) {
    c.input_valid = false;
  }
  return c;
}
std::optional<FrontendAppearance> appearance(const Json &value) {
  if (value.is_null())
    return {};
  std::array<std::uint8_t, 9> bytes{};
  for (unsigned i = 0; i < bytes.size(); ++i)
    bytes[i] = value.at(i).get<std::uint8_t>();
  return std::bit_cast<FrontendAppearance>(bytes);
}
std::optional<DiscordPartyLink> link(const bridge::AccountClientLink &value) {
  if (!value.present)
    return {};
  return DiscordPartyLink{
      std::string(value.party_id),
      std::string(value.join_secret),
      value.version,
      value.current_size,
      value.max_size,
      Clock::time_point(std::chrono::duration_cast<Clock::duration>(
          std::chrono::nanoseconds(value.expires_ns)))};
}
// Presentation-only projection of the trusted, sanitized Rust snapshot. No
// server response enters this function and no eligibility/retry rule is here.
OnlineAccountSnapshot snapshot(const Json &v) {
  OnlineAccountSnapshot s;
  s.authenticated = v.at("authenticated");
  s.busy = v.at("busy");
  s.message = v.at("message");
  s.revision = v.at("revision");
  const auto invitations = [](const Json &rows) {
    std::vector<OnlineClanInvite> out;
    for (const auto &r : rows)
      out.push_back({r.at("id"), r.at("name")});
    return out;
  };
  s.invitations = invitations(v.at("invitations"));
  if (const auto &c = v.at("clan"); !c.is_null()) {
    OnlineClan clan;
    clan.id = c.at("id");
    clan.name = c.at("name");
    clan.role = c.at("role");
    clan.outgoing = invitations(c.at("outgoing"));
    for (const auto &r : c.at("members"))
      clan.members.push_back(
          {r.at("id"), r.at("name"), r.at("role"), r.at("self"),
           static_cast<OnlineClanPresence>(r.at("presence").get<unsigned>())});
    for (const auto &r : c.at("messages"))
      clan.messages.push_back({r.at("id"), r.at("name"), r.at("body")});
    s.clan = std::move(clan);
  }
  for (const auto &r : v.at("party_invitations"))
    s.party_invitations.push_back({r.at("id"), r.at("name"), r.at("version")});
  if (const auto &p = v.at("party"); !p.is_null()) {
    OnlineParty party;
    party.version = p.at("version");
    party.leader = p.at("leader");
    party.outgoing = invitations(p.at("outgoing"));
    party.countdown_supported = p.at("countdown_supported");
    if (const auto &r = p.at("privacy"); !r.is_null())
      party.privacy =
          OnlineParty::Privacy{r.at("mode"), r.at("revision"), r.at("context")};
    if (const auto &r = p.at("countdown"); !r.is_null()) {
      OnlineParty::Countdown c;
      c.serial = r.at("serial");
      if (!r.at("seconds").is_null())
        c.seconds = r.at("seconds").get<std::int32_t>();
      c.mode = r.at("mode");
      c.complete = r.at("complete");
      c.requester = r.at("requester");
      party.countdown = std::move(c);
    }
    for (const auto &r : p.at("members")) {
      OnlinePartyMember member;
      member.handle = r.at("handle");
      member.name = r.at("name");
      member.ready_playlist = r.at("ready_playlist");
      member.self = r.at("self");
      member.leader = r.at("leader");
      member.online = r.at("online");
      member.appearance = appearance(r.at("appearance"));
      if (!r.at("postgame").is_null())
        member.postgame = r.at("postgame").get<bool>();
      for (const auto &local : r.at("local_players"))
        member.local_players.push_back({local.at("name"), local.at("guest"),
                                        appearance(local.at("appearance"))});
      party.members.push_back(std::move(member));
    }
    s.party = std::move(party);
  }
  s.party_message = v.at("party_message");
  s.party_command_serial = v.at("party_command_serial");
  if (!v.at("party_command_successful").is_null())
    s.party_command_successful = v.at("party_command_successful").get<bool>();
  s.profile_message = v.at("profile_message");
  for (const auto &r : v.at("steam_friends"))
    s.steam_friends.push_back(
        {r.at("id"), r.at("name"), r.at("registered"), r.at("halo_online")});
  s.steam_friends_state = v.at("steam_friends_state");
  s.steam_friends_message = v.at("steam_friends_message");
  s.friend_tag = v.at("friend_tag");
  s.friends_message = v.at("friends_message");
  s.friends_available = v.at("friends_available");
  const auto friend_row = [](const Json &r) {
    OnlineFriend f;
    f.id = r.at("id");
    f.name = r.at("name");
    f.tag = r.at("tag");
    f.state = r.at("state");
    f.presence = r.at("presence");
    f.party_context = r.at("party_context");
    if (!r.at("party_joinable").is_null())
      f.party_joinable = r.at("party_joinable").get<bool>();
    return f;
  };
  for (const auto &r : v.at("friends"))
    s.friends.push_back(friend_row(r));
  if (!v.at("friend_search").is_null())
    s.friend_search = friend_row(v.at("friend_search"));
  return s;
}
} // namespace
OnlineAccountOptions online_account_environment() {
  OnlineAccountOptions o;
  o.enable_parties = true;
  if (const auto *url = std::getenv("H2_ACCOUNT_URL"))
    o.endpoint = url;
  else
    o.endpoint = default_online_origin;
  if (const auto *http = std::getenv("H2_ACCOUNT_ALLOW_LOOPBACK_HTTP"))
    o.allow_loopback_http = std::string(http) == "1";
  return o;
}
struct OnlineAccount::Impl {
  std::shared_ptr<DiscordLogin> login;
  OnlineAccountOptions options;
  Http http;
  mutable std::mutex mutex;
  std::condition_variable_any wake;
  rust::Box<bridge::AccountClient> client;
  std::stop_source cancellation;
  std::jthread worker;
  struct Io final : bridge::NativeAccountClientIo {
    Impl &s;
    mutable std::unique_lock<std::mutex> held;
    mutable std::stop_token token;
    mutable std::exception_ptr exception;
    explicit Io(Impl &s)
        : s(s), held(s.mutex, std::defer_lock),
          token(s.cancellation.get_token()) {}
    template <class F> decltype(auto) capture(F &&f) const {
      try {
        return f();
      } catch (...) {
        exception = std::current_exception();
        throw;
      }
    }
    template <class F> decltype(auto) invoke(F &&f) const {
      try {
        return f();
      } catch (...) {
        if (exception)
          std::rethrow_exception(exception);
        throw;
      }
    }
    void account_lock() const override {
      if (!held.owns_lock())
        held.lock();
    }
    void account_unlock() const override { held.unlock(); }
    std::int64_t account_now_ns() const override {
      return std::chrono::duration_cast<std::chrono::nanoseconds>(
                 Clock::now().time_since_epoch())
          .count();
    }
    bridge::AccountClientIdentity account_identity() const override {
      return capture([&] {
        bridge::AccountClientIdentity value{};
        if (s.login)
          if (const auto id = s.login->service_identity()) {
            value.present = true;
            value.bearer = id->bearer;
            value.subject = id->subject;
          }
        return value;
      });
    }
    bridge::MatchmakingClientResponse
    account_request(const bridge::MatchmakingClientRequest &r) const override {
      return capture([&] {
        const auto response = s.http(
            {r.method.operator std::string(), r.url.operator std::string(),
             r.body.operator std::string(), r.bearer.operator std::string()},
            r.cleanup ? std::stop_token{} : token);
        return bridge::MatchmakingClientResponse{response.status,
                                                 response.body};
      });
    }
    bool account_cancelled() const override { return token.stop_requested(); }
    void account_reset_cancellation() const override {
      s.cancellation = std::stop_source{};
      token = s.cancellation.get_token();
    }
    void account_cancel() const override { s.cancellation.request_stop(); }
    void account_wake() const override { s.wake.notify_all(); }
    rust::Vec<std::uint8_t> account_entropy() const override {
      return capture([&] {
        unsigned char bytes[16];
        require(RAND_bytes(bytes, 16) == 1);
        rust::Vec<std::uint8_t> out;
        out.reserve(16);
        for (auto b : bytes)
          out.push_back(b);
        return out;
      });
    }
    bool account_eos_profiles() const override {
      return capture([] { return bridge::eos_runtime_options().profiles; });
    }
    bridge::EosIdentity account_eos_identity(rust::Str token) const override {
      return capture([&] { return bridge::eos_account_identity(token); });
    }
  };
  Impl(std::shared_ptr<DiscordLogin> l, OnlineAccountOptions o, Http h)
      : login(std::move(l)), options(std::move(o)),
        http(h ? std::move(h) : transport),
        client(bridge::new_account_client(
            config(options, bool(login)), login && login->uses_steam(),
            options.enable_parties, options.profile_only)) {
    options.endpoint = std::string(bridge::account_client_endpoint(*client));
    if (bridge::account_client_configured(*client))
      worker = std::jthread([this](std::stop_token stop) { run(stop); });
  }
  ~Impl() {
    worker.request_stop();
    {
      std::lock_guard lock(mutex);
      cancellation.request_stop();
    }
    wake.notify_all();
    if (worker.joinable())
      worker.join();
  }
  void run(std::stop_token stop) {
    while (!stop.stop_requested()) {
      std::unique_lock lock(mutex);
      if (!bridge::account_client_active(*client))
        wake.wait(lock, stop,
                  [&] { return bridge::account_client_pending(*client); });
      else
        wake.wait_for(lock, stop,
                      std::chrono::milliseconds(
                          bridge::account_client_cadence_ms(*client)),
                      [&] { return bridge::account_client_pending(*client); });
      if (stop.stop_requested())
        break;
      Io io(*this);
      io.held = std::move(lock);
      bridge::account_client_poll(*client, io);
    }
    Io io(*this);
    bridge::account_client_shutdown(*client, io);
  }
  std::uint64_t party_command(OnlinePartyCommand c, unsigned activity = 0) {
    std::lock_guard lock(mutex);
    Io io(*this);
    bridge::AccountClientPartyCommand command{};
    command.action = c.action;
    command.value = c.value;
    command.playlist = c.playlist;
    command.revision = c.revision;
    command.version = c.version;
    command.privacy_present = c.privacy.has_value();
    command.privacy = c.privacy.value_or(0);
    command.privacy_revision = c.privacy_revision;
    command.privacy_context = c.privacy_context;
    command.join_context = c.join_context;
    return io.invoke([&] {
      return bridge::account_client_party_command(*client, command, activity,
                                                  io);
    });
  }
};
OnlineAccount::OnlineAccount(std::shared_ptr<DiscordLogin> l,
                             OnlineAccountOptions o, Http h)
    : impl_(std::make_unique<Impl>(std::move(l), std::move(o), std::move(h))) {}
OnlineAccount::~OnlineAccount() = default;
std::shared_ptr<OnlineAccount>
OnlineAccount::profile_worker(std::shared_ptr<DiscordLogin> login) const {
  auto options = impl_->options;
  options.profile_only = true;
  options.enable_parties = false;
  options.poll_interval = std::chrono::seconds(20);
  return std::make_shared<OnlineAccount>(std::move(login), std::move(options),
                                         impl_->http);
}
void OnlineAccount::connect(const std::string &name) {
  std::lock_guard lock(impl_->mutex);
  Impl::Io io(*impl_);
  bridge::account_client_connect(*impl_->client, name, io);
}
void OnlineAccount::sign_out() {
  std::lock_guard lock(impl_->mutex);
  Impl::Io io(*impl_);
  bridge::account_client_sign_out(*impl_->client, io);
}
void OnlineAccount::profile(const FrontendAppearance &appearance,
                            const std::string &tag) {
  const auto bytes = std::bit_cast<std::array<std::uint8_t, 9>>(appearance);
  std::lock_guard lock(impl_->mutex);
  Impl::Io io(*impl_);
  io.invoke([&] {
    bridge::account_client_profile(*impl_->client, {bytes.data(), bytes.size()},
                                   tag, io);
  });
}
OnlineAccountSnapshot OnlineAccount::snapshot() const {
  std::lock_guard lock(impl_->mutex);
  return halo2::snapshot(Json::parse(
      std::string(bridge::account_client_snapshot(*impl_->client))));
}
std::optional<VoiceAuthorization>
OnlineAccount::voice_authorization(bool party) const {
  std::lock_guard lock(impl_->mutex);
  Impl::Io io(*impl_);
  const auto v = io.invoke(
      [&] { return bridge::account_client_voice(*impl_->client, party, io); });
  if (!v.present)
    return {};
  return VoiceAuthorization{std::string(v.origin),
                            std::string(v.bearer),
                            std::string(v.channel),
                            std::string(v.player),
                            v.loopback,
                            v.party};
}
bool OnlineAccount::command(OnlineClanCommand c) {
  std::lock_guard lock(impl_->mutex);
  Impl::Io io(*impl_);
  return io.invoke([&] {
    return bridge::account_client_clan_command(
        *impl_->client, c.action, c.value, c.role, c.confirmed, io);
  });
}
bool OnlineAccount::party_command(OnlinePartyCommand c) {
  return impl_->party_command(std::move(c)) != 0;
}
bool OnlineAccount::friend_command(const std::string &action,
                                   const std::string &value) {
  std::lock_guard lock(impl_->mutex);
  Impl::Io io(*impl_);
  return io.invoke([&] {
    return bridge::account_client_friend_command(*impl_->client, action, value,
                                                 io);
  });
}
void OnlineAccount::refresh_steam_friends() {
  std::lock_guard lock(impl_->mutex);
  Impl::Io io(*impl_);
  bridge::account_client_refresh_steam(*impl_->client, io);
}
std::uint64_t OnlineAccount::begin_discord_invite(const std::string &subject) {
  OnlinePartyCommand command;
  command.action = "invite_discord";
  command.value = subject;
  return impl_->party_command(std::move(command), 1);
}
std::uint64_t OnlineAccount::begin_discord_join(const std::string &secret) {
  OnlinePartyCommand command;
  command.action = "join_activity";
  command.value = secret;
  return impl_->party_command(std::move(command), 2);
}
std::optional<OnlineAccount::DiscordOperation>
OnlineAccount::discord_operation(std::uint64_t serial) const {
  std::lock_guard lock(impl_->mutex);
  Impl::Io io(*impl_);
  const auto v = io.invoke([&] {
    return bridge::account_client_discord_operation(*impl_->client, serial, io);
  });
  return DiscordOperation{v.serial, v.pending, v.successful, link(v.link)};
}
std::optional<DiscordPartyLink> OnlineAccount::discord_party_link() const {
  std::lock_guard lock(impl_->mutex);
  Impl::Io io(*impl_);
  return link(io.invoke(
      [&] { return bridge::account_client_discord_link(*impl_->client, io); }));
}
bool online_party_follower(const OnlineParty &p) {
  return bridge::account_party_follower(p.leader, p.members.size());
}
bool online_party_leader_left_postgame(const OnlineParty &p) {
  if (!bridge::account_party_follower(p.leader, p.members.size()))
    return false;
  std::vector<bridge::AccountClientPostgameMember> members;
  members.reserve(p.members.size());
  for (const auto &m : p.members)
    members.push_back(
        {m.self, m.leader, m.postgame.has_value(), m.postgame.value_or(false)});
  return bridge::account_party_leader_left_postgame(
      p.leader, {members.data(), members.size()});
}
} // namespace halo2
