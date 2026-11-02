#include "halo2/online_matchmaking.hpp"
#include "halo2/online_service.hpp"
#include "halo2/online_search_qos.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <curl/curl.h>
#include <algorithm>
#include <bit>
#include <condition_variable>
#include <cstdlib>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>

namespace halo2 {
namespace {
namespace bridge = online_protocol_bridge;
constexpr std::size_t max_body = 65536;
void require_io(bool value) { if (!value) throw std::runtime_error("Matchmaking IO failed"); }
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
  require_io(bool(client));
  const auto authorization = "Authorization: Bearer " + r.bearer;
  curl_slist *raw = curl_slist_append(nullptr, authorization.c_str());
  raw = curl_slist_append(raw, "Content-Type: application/json");
  std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)> headers(raw, curl_slist_free_all);
  Transfer t{{}, stop};
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
  require_io(curl_easy_perform(client.get()) == CURLE_OK && !stop.stop_requested());
  long status{};
  curl_easy_getinfo(client.get(), CURLINFO_RESPONSE_CODE, &status);
  return {status, std::move(t.body)};
}

bridge::MatchmakingClientConfig client_config(const OnlineMatchmakingOptions &o, bool login) {
  bridge::MatchmakingClientConfig c;
  try {
  c.input_valid=true;
  c.endpoint=o.endpoint;c.compatibility=o.compatibility;c.language=o.language;
  c.poll_ms=o.poll_interval.count();c.loopback=o.allow_loopback_http;c.login_present=login;
  c.terminal_recovery=o.terminal_recovery;
  if(const auto *value=std::getenv("H2_EOS_FORCE_RELAY"))c.force_relay=std::string(value)=="1";
  for(const auto &map:o.maps)c.maps.push_back(map);
  static const auto initialized=curl_global_init(CURL_GLOBAL_DEFAULT);
  if(initialized!=CURLE_OK)return c;
  std::unique_ptr<CURLU,decltype(&curl_url_cleanup)> url(curl_url(),curl_url_cleanup);
  if(!url || curl_url_set(url.get(),CURLUPART_URL,o.endpoint.c_str(),0)!=CURLUE_OK)return c;
  const auto part=[&](CURLUPart key) {
    char *raw=nullptr;const auto result=curl_url_get(url.get(),key,&raw,0);
    std::unique_ptr<char,decltype(&curl_free)> owner(raw,curl_free);
    return result==CURLUE_OK && raw?std::string(raw):std::string{};
  };
  c.url_parsed=true;c.url_normalized=part(CURLUPART_URL);c.url_scheme=part(CURLUPART_SCHEME);
  c.url_host=part(CURLUPART_HOST);c.url_user=part(CURLUPART_USER);c.url_password=part(CURLUPART_PASSWORD);
  c.url_options=part(CURLUPART_OPTIONS);c.url_path=part(CURLUPART_PATH);
  return c;
  }catch(...){c.input_valid=false;return c;}
}
rust::Vec<std::uint8_t> appearance_bytes(const std::optional<FrontendAppearance> &value) {
  rust::Vec<std::uint8_t> out;
  if(value)for(auto byte:std::bit_cast<std::array<std::uint8_t,9>>(*value))out.push_back(byte);
  return out;
}
std::optional<FrontendAppearance> native_appearance(const rust::Vec<std::uint8_t> &value) {
  if(value.empty())return {};
  require_io(value.size()==9);std::array<std::uint8_t,9> bytes{};
  std::copy(value.begin(),value.end(),bytes.begin());return std::bit_cast<FrontendAppearance>(bytes);
}
OnlineMatchmakingSnapshot native_snapshot(const bridge::MatchmakingClientSnapshot &s) {
  OnlineMatchmakingSnapshot out;
  out.status=static_cast<OnlineMatchmakingStatus>(s.status);
  out.playlist_id=std::string(s.playlist_id);out.message=std::string(s.message);
  out.queued_players=s.queued_players;out.revision=s.revision;
  out.start_countdown_supported=s.start_supported;
  if(s.start_present)out.start_countdown_ms=s.start_ms;
  if(s.failure_present)out.failure_state=s.failure_state;
  for(const auto &p:s.playlists) {
    OnlinePlaylist row;
    row.id=std::string(p.id);row.revision=std::string(p.revision);row.name=std::string(p.name);
    row.description=std::string(p.description);row.ranked=p.ranked;row.quickmatch=p.quickmatch;row.eligible=p.eligible;
    row.minimum_players=p.minimum;row.maximum_players=p.maximum;row.clan_match=p.clan_match;
    if(p.level_present)row.level=p.level;
    out.playlists.push_back(std::move(row));
  }
  if(s.game_present) {
    OnlineMatch game;game.map=std::string(s.map);game.variant=std::string(s.variant);game.game_id=std::string(s.game_id);
    if(!s.settings.empty()) {
      require_io(s.settings.size()==304);game.settings.emplace();
      std::copy(s.settings.begin(),s.settings.end(),game.settings->bytes.begin());
    }
    for(const auto &p:s.players) {
      OnlineMatchPlayer row;row.name=std::string(p.name);row.team=p.team;row.self=p.is_self;
      if(p.level_present)row.level=p.level;
      row.appearance=native_appearance(p.appearance);game.players.push_back(std::move(row));
    }
    out.match=std::move(game);
  }
  if(s.gathering_present) {
    OnlineGatheringProgress gathering;gathering.minimum_players=s.minimum;gathering.maximum_players=s.maximum;
    if(s.wait_present)gathering.wait_ms=s.wait_ms;
    for(const auto &p:s.gathering_players) {
      OnlineGatheringPlayer row;row.name=std::string(p.name);row.self=p.is_self;
      if(p.level_present)row.level=p.level;
      row.appearance=native_appearance(p.appearance);gathering.players.push_back(std::move(row));
    }
    out.gathering=std::move(gathering);
  }
  return out;
}
static_assert(int(OnlineMatchmakingStatus::Unconfigured)==int(bridge::MatchmakingClientStatus::Unconfigured));
static_assert(int(OnlineMatchmakingStatus::Idle)==int(bridge::MatchmakingClientStatus::Idle));
static_assert(int(OnlineMatchmakingStatus::Connecting)==int(bridge::MatchmakingClientStatus::Connecting));
static_assert(int(OnlineMatchmakingStatus::Ready)==int(bridge::MatchmakingClientStatus::Ready));
static_assert(int(OnlineMatchmakingStatus::Joining)==int(bridge::MatchmakingClientStatus::Joining));
static_assert(int(OnlineMatchmakingStatus::Searching)==int(bridge::MatchmakingClientStatus::Searching));
static_assert(int(OnlineMatchmakingStatus::CheckingConnections)==int(bridge::MatchmakingClientStatus::CheckingConnections));
static_assert(int(OnlineMatchmakingStatus::AwaitingTransport)==int(bridge::MatchmakingClientStatus::AwaitingTransport));
static_assert(int(OnlineMatchmakingStatus::Loading)==int(bridge::MatchmakingClientStatus::Loading));
static_assert(int(OnlineMatchmakingStatus::Failed)==int(bridge::MatchmakingClientStatus::Failed));
} // namespace

OnlineMatchmakingOptions online_matchmaking_environment(const std::filesystem::path &maps) {
  OnlineMatchmakingOptions result;
  if(const auto *url=std::getenv("H2_MATCHMAKING_URL"))result.endpoint=url;
  else result.endpoint=default_online_origin;
  result.compatibility=std::string(online_protocol_bridge::matchmaking_client_runtime_compatibility());
  if(const auto *http=std::getenv("H2_MATCHMAKING_ALLOW_LOOPBACK_HTTP"))result.allow_loopback_http=std::string(http)=="1";
  std::error_code error;
  for(std::filesystem::directory_iterator it(maps,error),end;!error && it!=end;it.increment(error))
    if(it->is_regular_file(error) && it->path().extension()==".map")result.maps.push_back(it->path().stem().string());
  std::sort(result.maps.begin(),result.maps.end());return result;
}

struct OnlineMatchmaking::Impl {
  std::shared_ptr<DiscordLogin> login;
  OnlineMatchmakingOptions options;
  Http http;
  mutable std::mutex mutex;
  std::condition_variable_any wake;
  std::vector<OnlineLocalPlayer> local_players;
  std::stop_source request_stop;
  bool pending{};
  rust::Box<bridge::MatchmakingClient> client;
  std::shared_ptr<OnlineMatchSession> match_session;
  std::unique_ptr<OnlineSearchQos> qos;
  std::jthread worker;

  struct Io final : bridge::NativeMatchmakingIo {
    Impl &owner;
    std::stop_token token;
    Io(Impl &value,std::stop_token stop):owner(value),token(stop){}
    std::uint64_t client_now_ms()const override {
      return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }
    bool client_cancelled(std::uint64_t version)const override {
      return token.stop_requested() || !bridge::matchmaking_client_current(*owner.client,version);
    }
    bridge::MatchmakingClientIdentity client_identity(std::uint64_t version)const override {
      std::vector<OnlineLocalPlayer> locals;
      {std::lock_guard lock(owner.mutex);require_io(!client_cancelled(version));locals=owner.local_players;}
      bridge::MatchmakingClientIdentity out;
      const auto primary=owner.login->authorization();
      out.primary_present=primary.has_value();if(primary)out.primary=*primary;
      for(const auto &player:locals) {
        bridge::MatchmakingClientGrant grant;
        if(player.login) {
          const auto authorization=player.login->authorization();grant.present=authorization.has_value();
          if(authorization)grant.authorization=*authorization;
          grant.steam=player.login->uses_steam();
        }
        out.locals.push_back(std::move(grant));
      }
      return out;
    }
    bridge::MatchmakingClientResponse client_request(const bridge::MatchmakingClientRequest &r)const override {
      if(!r.cleanup)require_io(!client_cancelled(r.version));
      const auto response=owner.http({std::string(r.method),std::string(r.url),std::string(r.body),std::string(r.bearer)},
                                    r.cleanup?std::stop_token{}:token);
      bridge::MatchmakingClientResponse out;out.status=response.status;out.body=response.body;return out;
    }
    bridge::MatchmakingClientConnection client_connection(std::uint64_t version)const override {
      std::shared_ptr<OnlineMatchSession> connection;
      {std::lock_guard lock(owner.mutex);require_io(!client_cancelled(version));connection=owner.match_session;}
      bridge::MatchmakingClientConnection out;out.phase=bridge::MatchmakingClientPhase::Missing;
      if(connection)switch(connection->phase()) {
        case OnlineMatchSession::Phase::Connecting:out.phase=bridge::MatchmakingClientPhase::Connecting;break;
        case OnlineMatchSession::Phase::Loading:out.phase=bridge::MatchmakingClientPhase::Loading;break;
        case OnlineMatchSession::Phase::Failed:out.phase=bridge::MatchmakingClientPhase::Failed;out.error=connection->error();break;
        default:out.phase=bridge::MatchmakingClientPhase::Other;break;
      }
      return out;
    }
    bool client_install(std::uint64_t version,const bridge::MatchmakingClientAdmission &v)const override {
      if(client_cancelled(version))return false;
      std::vector<OnlineAdmission> roster;
      for(const auto &s:v.seats) {
        OnlineAdmission row;row.slot=s.slot;row.name=std::string(s.name);row.team=int(s.team);
        for(unsigned i=0;i<8;++i)row.identity[i]=std::byte(s.identity[i]);
        row.peer=s.peer;row.guest=s.guest;if(s.level_present)row.matchmaking_level=s.level;
        roster.push_back(std::move(row));
      }
      OnlineMatchTransportOptions transport;transport.direct_udp=v.direct;transport.terminal_recovery=v.terminal_recovery;
      for(const auto &server:v.stun)transport.stun_servers.push_back(std::string(server));
      if(v.eos_present) {
        OnlineEosOptions eos;eos.socket=std::string(v.eos_socket);eos.account_login=v.eos_account_login;
        eos.sessions=true;eos.playlist=std::string(v.eos_playlist);eos.revision=std::string(v.eos_revision);
        eos.map=std::string(v.eos_map);eos.variant=std::string(v.eos_variant);eos.force_relay=v.eos_force_relay;
        for(const auto &peer:v.eos_peers)eos.peers.push_back({peer.slot,std::string(peer.key)});
        transport.eos=std::move(eos);
      }
      auto connection=std::make_shared<OnlineMatchSession>(std::string(v.endpoint),std::string(v.room),std::string(v.bearer),
          v.current,std::move(roster),v.loopback,v.team_present?std::optional(v.team_mode):std::nullopt,false,std::move(transport));
      std::lock_guard lock(owner.mutex);
      if(client_cancelled(version))return false;
      owner.match_session=std::move(connection);return true;
    }
    void client_disconnect(std::uint64_t version)const override {
      std::lock_guard lock(owner.mutex);
      if(bridge::matchmaking_client_current(*owner.client,version))owner.match_session.reset();
    }
    bool client_qos(std::uint64_t version,rust::Str endpoint,rust::Str bearer,bool loopback)const override {
      if(!bridge::matchmaking_client_current(*owner.client,version))return false;
      std::unique_ptr<OnlineSearchQos> connection;
      if(!bearer.empty())connection=std::make_unique<OnlineSearchQos>(std::string(endpoint),std::string(bearer),loopback);
      if(!bridge::matchmaking_client_current(*owner.client,version))return false;
      owner.qos=std::move(connection);return true;
    }
  };

  Impl(std::shared_ptr<DiscordLogin> l,OnlineMatchmakingOptions o,Http h)
      :login(std::move(l)),options(std::move(o)),http(h?std::move(h):transport),
       client(bridge::new_matchmaking_client(client_config(options,bool(login)))) {
    if(bridge::matchmaking_client_configured(*client))worker=std::jthread([this](std::stop_token stop){run(stop);});
  }
  ~Impl() {
    worker.request_stop();{std::lock_guard lock(mutex);request_stop.request_stop();}
    wake.notify_all();if(worker.joinable())worker.join();
  }
  void command(bridge::MatchmakingClientCommandKind kind,std::string name={},std::string playlist={},
               std::vector<OnlineLocalPlayer> additional={},std::optional<FrontendAppearance> appearance={}) {
    bridge::MatchmakingClientCommand c;c.kind=kind;c.input_valid=true;
    try {
    c.name=std::move(name);c.playlist=std::move(playlist);
    c.appearance=appearance_bytes(appearance);
    for(const auto &player:additional) {
      bridge::MatchmakingClientLocal row;row.name=player.name;row.signed_in=bool(player.login);
      row.appearance=appearance_bytes(player.appearance);c.locals.push_back(std::move(row));
    }
    }catch(...){c.input_valid=false;}
    std::lock_guard lock(mutex);
    bridge::matchmaking_client_command(*client,c);
    if(kind==bridge::MatchmakingClientCommandKind::Refresh)local_players=std::move(additional);
    if(kind==bridge::MatchmakingClientCommandKind::SignOut)local_players.clear();
    match_session.reset();request_stop.request_stop();pending=true;wake.notify_all();
  }
  void run(std::stop_token stop) {
    while(!stop.stop_requested()) {
      std::unique_lock lock(mutex);
      if(bridge::matchmaking_client_active(*client))wake.wait_for(lock,stop,options.poll_interval,[&]{return pending;});
      else wake.wait(lock,stop,[&]{return pending;});
      if(stop.stop_requested())break;
      pending=false;request_stop=std::stop_source{};const auto source=request_stop;lock.unlock();
      std::stop_callback shutdown(stop,[source]()mutable{source.request_stop();});
      Io io(*this,source.get_token());bridge::matchmaking_client_poll(*client,io);
    }
    Io io(*this,{});bridge::matchmaking_client_shutdown(*client,io);
  }
};

OnlineMatchmaking::OnlineMatchmaking(std::shared_ptr<DiscordLogin> login,OnlineMatchmakingOptions options,Http http)
  :impl_(std::make_unique<Impl>(std::move(login),std::move(options),std::move(http))){}
OnlineMatchmaking::~OnlineMatchmaking()=default;
void OnlineMatchmaking::refresh(const std::string &name,std::vector<OnlineLocalPlayer> additional,std::optional<FrontendAppearance> appearance) {
  impl_->command(bridge::MatchmakingClientCommandKind::Refresh,name,{},std::move(additional),appearance);
}
void OnlineMatchmaking::join(const std::string &playlist) {impl_->command(bridge::MatchmakingClientCommandKind::Join,{},playlist);}
void OnlineMatchmaking::cancel_queue() {impl_->command(bridge::MatchmakingClientCommandKind::Cancel);}
void OnlineMatchmaking::sign_out() {impl_->command(bridge::MatchmakingClientCommandKind::SignOut);}
void OnlineMatchmaking::reject_content() {
  std::lock_guard lock(impl_->mutex);
  if(bridge::matchmaking_client_reject_content(*impl_->client)) {
    impl_->request_stop.request_stop();impl_->pending=true;impl_->wake.notify_all();
  }
}
OnlineMatchmakingSnapshot OnlineMatchmaking::snapshot()const {return native_snapshot(bridge::matchmaking_client_snapshot(*impl_->client));}
std::optional<OnlineMatchmakingSnapshot> OnlineMatchmaking::snapshot_after(std::uint64_t revision)const {
  const auto state=bridge::matchmaking_client_snapshot(*impl_->client);
  if(state.revision==revision)return {};
  return native_snapshot(state);
}
std::optional<unsigned> OnlineMatchmaking::player_speaker(unsigned row)const {
  std::shared_ptr<OnlineMatchSession> session;
  {std::lock_guard lock(impl_->mutex);session=impl_->match_session;
    if(!session || row>=session->roster().size())return {};
    const auto &admitted=session->roster()[row];
    if(!bridge::matchmaking_client_speaker_binding(*impl_->client,row,admitted.name,admitted.team,admitted.slot,online_admission_peer(admitted)))return {};
  }
  const auto audio=session->retained_voice();if(!audio)return {};
  const auto state=audio->snapshot();
  if(row==session->self())return !state.enabled||state.microphone_muted?2u:state.transmitting?0u:1u;
  if(!state.enabled)return {};
  if(state.talking[row])return 0u;
  return session->postgame_voice_enabled(row)?std::optional<unsigned>(1):std::nullopt;
}
std::shared_ptr<OnlineMatchSession> OnlineMatchmaking::take_launch() {
  std::lock_guard lock(impl_->mutex);
  bridge::MatchmakingLaunchObservation observation;observation.session_present=bool(impl_->match_session);
  std::vector<bridge::MatchmakingSeat> seats;
  if(impl_->match_session) {
    observation.loading=impl_->match_session->phase()==OnlineMatchSession::Phase::Loading;
    const auto mode=impl_->match_session->team_mode();observation.team_mode_present=mode.has_value();
    if(mode)observation.team_mode=*mode;
    observation.current_peer=impl_->match_session->self();
    for(const auto &seat:impl_->match_session->roster())
      seats.push_back({true,online_admission_peer(seat),seat.team});
  }
  if(!bridge::matchmaking_client_take_launch(*impl_->client,observation,{seats.data(),seats.size()}))return {};
  return std::move(impl_->match_session);
}
} // namespace halo2
