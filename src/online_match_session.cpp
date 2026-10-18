#include "halo2/custom_slayer.hpp"
#include "halo2/frontend.hpp"
#include "halo2/online_match_session.hpp"
#include "halo2/online_peer_transport.hpp"
#include "halo2/source_game_variant.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <openssl/sha.h>
#include <chrono>
#include <algorithm>
#include <atomic>
#include <bit>
#include <cmath>
#include <deque>
#include <iomanip>
#include <functional>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace halo2 {
namespace {
using Json = nlohmann::json;
using Clock = std::chrono::steady_clock;
void require(bool ok) { if (!ok) throw std::runtime_error("ONLINE match connection failed or ended. Search again."); }
struct Frame { unsigned peer; std::vector<std::byte> data;std::uint32_t epoch{}; };
struct TerminalMatchError : std::runtime_error {using std::runtime_error::runtime_error;};
struct ControlPresence {std::atomic<bool> &active;~ControlPresence(){active=false;}};
bool valid_world_digest(const std::string &value) {
  return value.size()==64 && value.find_first_not_of("0123456789abcdef")==std::string::npos;
}
void put(std::vector<std::byte>& p,unsigned value,unsigned count){for(unsigned i=0;i<count;++i)p.push_back(std::byte(value>>(i*8)));}
unsigned get(std::span<const std::byte> p,unsigned at,unsigned count){require(at+count<=p.size());unsigned n{};for(unsigned i=0;i<count;++i)n|=std::to_integer<unsigned>(p[at+i])<<(i*8);return n;}
std::vector<std::byte> voice_header(unsigned type){std::vector<std::byte> p;put(p,0x324e3248,4);put(p,native_multiplayer_protocol,2);put(p,6,1);put(p,type,1);return p;}
std::string digest(std::span<const std::byte> bytes) {
  unsigned char hash[SHA256_DIGEST_LENGTH];require(SHA256(reinterpret_cast<const unsigned char*>(bytes.data()),bytes.size(),hash));
  constexpr char digits[]="0123456789abcdef";std::string result;result.reserve(64);
  for(auto n:hash){result+=digits[n>>4];result+=digits[n&15];}return result;
}
std::uint64_t transfer_time(){return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now().time_since_epoch()).count();}
}
struct OnlineMatchSession::Impl {
  unsigned self, host = 16;
  const std::vector<OnlineAdmission> roster;
  mutable std::mutex mutex;
  Phase phase = Phase::Connecting;
  std::string error, loaded;
  std::optional<std::uint64_t> native_fingerprint;
  std::string completion, confirmation;
  std::vector<std::byte> report;
  std::deque<Frame> incoming, outgoing;
  std::deque<std::string> continuation_outgoing;
  std::deque<OnlineWorldSeal> seals;
  const bool supports_continuity;
  const OnlineMatchTransportOptions transport_options;
  OnlineContinuityState continuity_state;
  AuthorityTransferDigest authority_session{};
  std::shared_ptr<PeerAuthorityTransfer> authority;
  std::shared_ptr<PeerSourceAdmissions> source_admissions;
  std::shared_ptr<PeerMeshQuality> mesh_quality;
  std::deque<MatchVoicePacket> voices;
  std::shared_ptr<GameVoice> audio;
  std::array<bool,16> voice_enabled{};
  std::array<std::uint16_t,16> voice_mutes{};
  std::array<unsigned,16> voice_sequences{};
  std::uint16_t voice_members{};
  std::uint16_t voice_blocked{};
  Clock::time_point voice_preferences_at{};
  std::array<double,16> voice_credits{10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10};
  Clock::time_point voice_credit_at=Clock::now();
  std::atomic<bool> voice_focused{false};
  std::atomic<bool> control_online{false};
  std::atomic<std::uint16_t> direct_ready{0};
  std::function<void()> postgame_voice_pump;
  std::jthread worker;
  const std::optional<bool> team_mode;
  Impl(std::string origin, std::string room, std::string bearer, unsigned s,
       std::vector<OnlineAdmission> r, bool local,std::optional<bool> teams,bool continuation,OnlineMatchTransportOptions transport) : self(s), roster(std::move(r)),supports_continuity(continuation),transport_options(std::move(transport)),team_mode(teams) {
    require(self < roster.size() && roster.size() >= 2 && roster.size() <= 16);
    require(online_admission_peer(roster[self])==self && !roster[self].guest);
    for(unsigned i=0;i<roster.size();++i) {
      const auto &player=roster[i];const auto peer=online_admission_peer(player);
      require(player.slot==i && peer<roster.size() && online_admission_peer(roster[peer])==peer && !roster[peer].guest);
      if(team_mode.value_or(true))require(player.team==roster[peer].team);
      require(std::count_if(roster.begin(),roster.end(),[&](const auto &p){return online_admission_peer(p)==peer;})<=4);
    }
    voice_members=std::uint16_t((1u<<roster.size())-1);
    continuity_state.members=voice_members;
    require(room.size() == 36 && room.find_first_not_of("0123456789abcdef-") == std::string::npos);
    require(SHA256(reinterpret_cast<const unsigned char *>(room.data()),room.size(),reinterpret_cast<unsigned char *>(authority_session.data())));
    if(transport_options.direct_udp){
      std::uint16_t physical=0;for(const auto &player:roster)if(online_admission_peer(player)==player.slot)physical|=std::uint16_t(1u<<player.slot);
      if(std::popcount(physical)>1)mesh_quality=std::make_shared<PeerMeshQuality>(self,
          AuthorityTransferContext{authority_session,0,0,physical,16});
    }
    require(bearer.size() >= 32 && bearer.size() <= 128 && bearer.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_") == std::string::npos);
    require(origin.starts_with("https://") || (local && origin.starts_with("http://127.0.0.1:")));
    require(origin.find_first_of("@?#\\\r\n\t ") == std::string::npos);
    const auto scheme = origin.find(':');
    const auto url = std::string(origin.starts_with("https:") ? "wss" : "ws") + origin.substr(scheme) + "/v1/matches/" + room + (transport_options.direct_udp?"/signal":"/relay");
    worker = std::jthread([this, url, bearer](std::stop_token stop) { run(url, bearer, stop); });
  }
  ~Impl() { worker.request_stop(); if (worker.joinable()) worker.join(); }
  static OnlineWorldSeal parse_seal(const Json &value) {
    require(value.is_object() && value.size()==3 && value.contains("epoch") &&
        value.contains("tick") && value.contains("digest") && value["epoch"].is_number_unsigned() &&
        value["epoch"].get<std::uint64_t>()<=UINT32_MAX && value["tick"].is_number_unsigned() && value["digest"].is_string());
    OnlineWorldSeal seal{value["epoch"].get<std::uint32_t>(),value["tick"].get<std::uint64_t>(),value["digest"].get<std::string>()};
    require(valid_world_digest(seal.digest));return seal;
  }
  void control(const Json &data) {
    require(data.size()==(supports_continuity?3u:2u) && data["phase"].is_string());
    const auto name=data["phase"].get<std::string>();
    require(name=="connecting" || name=="loading" || name=="running" ||
        (supports_continuity && (name=="awaiting_world" || name=="migrating" || name=="rebinding")));
    std::lock_guard lock(mutex);
    const auto previous_host=host,previous_epoch=continuity_state.epoch;
    if(name!="connecting") {
      if(name=="awaiting_world"){require(data["host"].is_null());host=16;}
      else {
        require(data["host"].is_number_unsigned() && data["host"]<roster.size());
        host=data["host"].get<unsigned>();
        require(online_admission_peer(roster[host])==host && !roster[host].guest);
      }
    }
    if(supports_continuity) {
      require(data.contains("continuity"));const auto &value=data["continuity"];
      if(name=="connecting" || name=="loading")require(value.is_null());
      else {
        require(value.is_object() && value["epoch"].is_number_unsigned() &&
            value["epoch"].get<std::uint64_t>()<=UINT32_MAX && value["members"].is_number_unsigned() &&
            value["phase"]==name && value["host"]==data["host"]);
        const auto epoch=value["epoch"].get<std::uint32_t>(),members=value["members"].get<unsigned>();
        require(epoch>=continuity_state.epoch && !(members&~((1u<<roster.size())-1)) &&
            (members&(1u<<self)) && (host==16 || (members&(1u<<host))));
        for(const auto &player:roster)
          require(bool(members&(1u<<player.slot))==bool(members&(1u<<online_admission_peer(player))));
        std::optional<OnlineWorldSeal> selected;
        if(name=="migrating" || name=="rebinding") {
          require(value.size()==6);
          selected=parse_seal(Json{{"epoch",epoch},{"tick",value["tick"]},{"digest",value["digest"]}});
        } else require(value.size()==4);
        continuity_state={epoch,host,std::uint16_t(members),std::move(selected)};
        voice_members=std::uint16_t(members);
        for(unsigned i=0;i<roster.size();++i)if(!(members&(1u<<i)))voice_enabled[i]=false;
      }
      if(previous_epoch!=continuity_state.epoch || previous_host!=host) {
        incoming.clear();outgoing.clear();voices.clear();
      }
    }
    phase=name=="running"?Phase::Running:name=="loading"?Phase::Loading:
        name=="awaiting_world"?Phase::AwaitingWorld:name=="migrating"?Phase::Migrating:
        name=="rebinding"?Phase::Rebinding:Phase::Connecting;
    if(transport_options.direct_udp&&!authority&&(phase==Phase::Loading||phase==Phase::Running)&&host<16){
      std::uint16_t physical=0;for(const auto &player:roster)if(online_admission_peer(player)==player.slot)physical|=std::uint16_t(1u<<player.slot);
      const AuthorityTransferContext context{authority_session,continuity_state.epoch,0,physical,host};
      AuthorityTransferLimits limits;limits.features=authority_feature_source_replay|authority_feature_source_state|authority_feature_source_camera|authority_feature_source_lens_query|authority_feature_source_lens_draw|authority_feature_source_particle_collision|authority_feature_source_projectile_contacts|authority_feature_source_pipeline|authority_feature_source_host_continuation;
      authority=std::make_shared<PeerAuthorityTransfer>(self,context,limits);
      source_admissions=std::make_shared<PeerSourceAdmissions>(self,context,roster);
    }
    if(transport_options.direct_udp&&host<16&&phase!=Phase::Connecting){
      std::uint16_t physical=0;for(const auto &player:roster)
        if(online_admission_peer(player)==player.slot&&(continuity_state.members&(1u<<player.slot)))physical|=std::uint16_t(1u<<player.slot);
      const AuthorityTransferContext context{authority_session,continuity_state.epoch,continuity_state.members,physical,host};
      if(std::popcount(physical)>1&&(!mesh_quality||mesh_quality->context()!=context))mesh_quality=std::make_shared<PeerMeshQuality>(self,context);
    }
  }
  void queue_world(const char *action,const OnlineWorldSeal &seal) {
    require(supports_continuity && valid_world_digest(seal.digest) && continuation_outgoing.size()<64);
    continuation_outgoing.push_back(Json{{action,{{"epoch",seal.epoch},{"tick",seal.tick},{"digest",seal.digest}}}}.dump());
  }
  void datagram(unsigned source,std::span<const std::byte> packet,std::uint32_t epoch) {
    if(PeerMeshQuality::is_packet(packet)){
      std::shared_ptr<PeerMeshQuality> quality;
      {std::lock_guard lock(mutex);if(transport_options.direct_udp)quality=mesh_quality;}
      if(quality&&(!supports_continuity||epoch==quality->context().epoch))quality->receive(source,packet,transfer_time());
      return;
    }
    if(PeerSourceAdmissions::is_packet(packet)){
      std::shared_ptr<PeerSourceAdmissions> admissions;
      {std::lock_guard lock(mutex);if(transport_options.direct_udp&&(phase==Phase::Loading||phase==Phase::Running||phase==Phase::Migrating||phase==Phase::Rebinding))admissions=source_admissions;}
      if(admissions)admissions->receive(source,packet);
      return;
    }
    if(PeerAuthorityTransfer::is_packet(packet)){
      std::shared_ptr<PeerAuthorityTransfer> transfer;
      {std::lock_guard lock(mutex);if(transport_options.direct_udp&&(phase==Phase::Loading||phase==Phase::Running||phase==Phase::Migrating||phase==Phase::Rebinding))transfer=authority;}
      if(transfer)transfer->receive(source,packet,transfer_time());
      return;
    }
    std::lock_guard lock(mutex);
    if(phase!=Phase::Running && phase!=Phase::Reviewing && phase!=Phase::Postgame)return;
    require(source<roster.size() && source!=self && online_admission_peer(roster[source])==source);
    if(supports_continuity && (epoch!=continuity_state.epoch || !(continuity_state.members&(1u<<source))))return;
    require(packet.size()>=7 && packet.size()<=1400 && get(packet,0,4)==0x324e3248 && get(packet,4,2)==native_multiplayer_protocol);
    const auto kind=get(packet,6,1);
    require((self==host && (kind==1 || kind==2 || kind==4 || kind==6)) ||
            (source==host && (kind==3 || kind==5 || kind==6)));
    if(kind!=6) {
      if(phase!=Phase::Running)return;
      if(incoming.size()<512)incoming.push_back({source,{packet.begin(),packet.end()},epoch});
      return;
    }
    MatchVoicePacket voice;voice.peer=source;
    const auto subtype=get(packet,7,1);
    if(subtype==1 || subtype==2) {
      require(packet.size()==(subtype==1?11u:12u) && (subtype==1 || source==host));
      voice.preferences=true;voice.frame.source=subtype==1?source:get(packet,11,1);
      const auto enabled=get(packet,8,1);require(enabled<=1 && voice.frame.source<roster.size());
      voice.enabled=enabled;voice.muted=get(packet,9,2);
    } else {
      require(subtype==0 && packet.size()>22 && packet.size()<=278);
      voice.frame.source=get(packet,8,1);voice.frame.sequence=get(packet,9,4);
      require(voice.frame.source<roster.size() && (voice_members&(1u<<voice.frame.source)) &&
          (source==host || online_admission_peer(roster[voice.frame.source])==source));
      const auto radio=get(packet,13,1);require(radio<=1);voice.frame.radio=radio;
      voice.frame.gains={std::bit_cast<float>(get(packet,14,4)),std::bit_cast<float>(get(packet,18,4))};
      require(std::isfinite(voice.frame.gains.left) && std::isfinite(voice.frame.gains.right) && voice.frame.gains.left>=0 && voice.frame.gains.left<=1 && voice.frame.gains.right>=0 && voice.frame.gains.right<=1);
      // The coordinator supplies the account block mask. The receiving PC
      // enforces it even when a player host forwards a blocked speaker.
      if(self!=host && (voice_blocked&(1u<<voice.frame.source)))return;
      for(auto byte:packet.subspan(22))voice.frame.opus.push_back(std::to_integer<std::uint8_t>(byte));
    }
    if(voices.size()==80)voices.pop_front();voices.push_back(std::move(voice));
  }
  void fail(const std::string &message) {
    std::shared_ptr<GameVoice> stopped_audio;
    {std::lock_guard lock(mutex);if(phase!=Phase::Postgame)phase=Phase::Failed;
      error=message;voice_enabled.fill(false);stopped_audio=audio;}
    if(stopped_audio)stopped_audio->enable(false);
  }
  void run_peers(const std::shared_ptr<OnlinePeerTransport> &direct,std::stop_token stop) {
    try {
      std::vector<unsigned> physical;
      for(const auto &player:roster)if(online_admission_peer(player)==player.slot)physical.push_back(player.slot);
      bool announced=false;
      const auto started=Clock::now();
      while(!stop.stop_requested()) {
        std::uint16_t ready_mask=0;for(auto peer:physical)if(peer!=self&&direct->ready(peer))ready_mask|=std::uint16_t(1u<<peer);
        direct_ready.store(ready_mask,std::memory_order_relaxed);
        for(auto &signal:direct->signals()) {
          const auto value=Json{{"peer_signal",{{"peer",signal.peer},{"type",signal.type},{"payload",signal.payload},{"mid",signal.mid}}}}.dump();
          std::lock_guard lock(mutex);require(continuation_outgoing.size()<1024);continuation_outgoing.push_back(value);
        }
        std::shared_ptr<PeerMeshQuality> quality;Phase current_phase;
        {std::lock_guard lock(mutex);quality=mesh_quality;current_phase=phase;}
        const auto measured=quality?quality->local(transfer_time()):std::nullopt;
        const auto complete_measurement=std::all_of(physical.begin(),physical.end(),[&](auto peer){
          return peer==self||(measured&&measured->links[peer].measured);});
        if(!announced && (Clock::now()-started>=std::chrono::seconds(10) ||complete_measurement)) {
          Json ready=Json::array();
          for(auto peer:physical)if(peer!=self){
            const auto q=measured?measured->links[peer]:PeerLinkQuality{};
            ready.push_back({{"peer",peer},{"rtt_ms",q.measured?Json(q.rtt_ms):Json(nullptr)},
              {"upload_bytes_per_second",q.measured?Json(q.upload_bytes_per_second):Json(nullptr)},
              {"loss_per_mille",q.measured?Json(q.loss_per_mille):Json(nullptr)}});
          }
          std::lock_guard lock(mutex);continuation_outgoing.push_back(Json{{"peer_ready",ready}}.dump());announced=true;
        }
        std::vector<std::byte> packet;unsigned peer{};
        while(direct->receive(packet,peer)) {
          try {
            const auto offset=supports_continuity?4u:0u;require(packet.size()>=offset+7);
            datagram(peer,std::span(packet).subspan(offset),supports_continuity?get(packet,0,4):0);
          } catch(const std::exception &) {} // Untrusted bytes cannot evict healthy peers.
        }
        std::deque<Frame> frames;std::function<void()> pump;
        {std::lock_guard lock(mutex);frames.swap(outgoing);pump=postgame_voice_pump;}
        if(pump)pump();
        for(auto &frame:frames) {
          unsigned target=frame.peer;
          {std::lock_guard lock(mutex);if(phase==Phase::Postgame && self!=host && frame.data.size()>=7 && get(frame.data,6,1)==6)target=host;}
          if(supports_continuity) {
            std::vector<std::byte> prefix;put(prefix,frame.epoch,4);frame.data.insert(frame.data.begin(),prefix.begin(),prefix.end());
          }
          direct->send(frame.data,target);
        }
        std::shared_ptr<PeerAuthorityTransfer> transfer;
        {std::lock_guard lock(mutex);if(phase==Phase::Loading||phase==Phase::Running||phase==Phase::Migrating||phase==Phase::Rebinding)transfer=authority;}
        if(transfer){const auto epoch=transfer->context().epoch;
          // Gameplay and microphone frames are drained first. Authority data
          // has its own rate/window budget and cannot grow the media queue.
          for(auto packet:transfer->poll(transfer_time(),2)){
            if(supports_continuity){std::vector<std::byte> prefix;put(prefix,epoch,4);packet.bytes.insert(packet.bytes.begin(),prefix.begin(),prefix.end());}
            direct->send(packet.bytes,packet.peer);
          }
        }
        std::shared_ptr<PeerSourceAdmissions> admissions;
        {std::lock_guard lock(mutex);if(phase==Phase::Loading||phase==Phase::Running||phase==Phase::Migrating||phase==Phase::Rebinding)admissions=source_admissions;}
        if(admissions){const auto epoch=admissions->context().epoch;
          for(auto packet:admissions->poll(transfer_time(),2)){
            if(supports_continuity){std::vector<std::byte> prefix;put(prefix,epoch,4);packet.bytes.insert(packet.bytes.begin(),prefix.begin(),prefix.end());}
            direct->send(packet.bytes,packet.peer);
          }
        }
        if(quality){const auto epoch=quality->context().epoch;
          // Only setup/measurement summaries use the coordinator. All probes,
          // replies and full mesh rows use the authenticated direct links.
          for(auto packet:quality->poll(ready_mask,transfer_time(),
              current_phase==Phase::Connecting||current_phase==Phase::Loading?16:4)){
            if(supports_continuity){std::vector<std::byte> prefix;put(prefix,epoch,4);packet.bytes.insert(packet.bytes.begin(),prefix.begin(),prefix.end());}
            direct->send(packet.bytes,packet.peer);
          }
        }
        {
          std::lock_guard lock(mutex);
          if(phase==Phase::Failed)return;
          if(host<roster.size()&&self!=host&&phase!=Phase::Postgame&&phase!=Phase::Reviewing&&direct->failed(host))
            throw std::runtime_error("Direct player connection was lost. Search again.");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
      }
    } catch(const std::exception &e){fail(e.what());}
  }
  void run(const std::string &url,const std::string &bearer,std::stop_token stop) {
    try {
      std::shared_ptr<OnlinePeerTransport> direct;
      if(transport_options.direct_udp) {
        std::vector<unsigned> physical;
        for(const auto &player:roster)if(online_admission_peer(player)==player.slot)physical.push_back(player.slot);
        direct=std::make_shared<OnlinePeerTransport>(self,physical,transport_options.stun_servers);
      }
      std::jthread peer_worker;
      if(direct)peer_worker=std::jthread([this,direct](std::stop_token token){run_peers(direct,token);});
      while(!stop.stop_requested()) {
        try {run_control(url,bearer,stop,direct.get());}
        catch(const TerminalMatchError &){throw;}
        catch(const std::exception &) {
          // Once admitted, gameplay/audio do not depend on the master socket.
          // Its reconnect worker may stall on TLS without stalling peer UDP.
          std::lock_guard lock(mutex);
          if(!direct || (phase!=Phase::Running && phase!=Phase::Reviewing && phase!=Phase::Postgame))throw;
          error="Master connection unavailable; the player-hosted match continues. Results will retry.";
        }
        for(unsigned i=0;i<100 && !stop.stop_requested();++i)std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
    } catch(const std::exception &e){fail(e.what());}
  }
  void run_control(const std::string &url,const std::string &bearer,std::stop_token stop,OnlinePeerTransport *direct) {
      ControlPresence presence{control_online};
      require(curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK);
      std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> client(curl_easy_init(), curl_easy_cleanup);
      require(bool(client));
      const auto header = "Authorization: Bearer " + bearer;
      std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)> headers(curl_slist_append(nullptr, header.c_str()), curl_slist_free_all);
      curl_easy_setopt(client.get(), CURLOPT_URL, url.c_str());
      curl_easy_setopt(client.get(), CURLOPT_HTTPHEADER, headers.get());
      curl_easy_setopt(client.get(), CURLOPT_CONNECT_ONLY, 2L);
      curl_easy_setopt(client.get(), CURLOPT_PROTOCOLS_STR, "wss,ws");
      curl_easy_setopt(client.get(), CURLOPT_FOLLOWLOCATION, 0L);
      curl_easy_setopt(client.get(), CURLOPT_PROXY, "");
      curl_easy_setopt(client.get(), CURLOPT_NOSIGNAL, 1L);
      curl_easy_setopt(client.get(), CURLOPT_TIMEOUT_MS, 3000L);
      curl_easy_setopt(client.get(), CURLOPT_CONNECTTIMEOUT_MS, 2000L);
      curl_easy_setopt(client.get(), CURLOPT_SSL_VERIFYPEER, 1L);
      curl_easy_setopt(client.get(), CURLOPT_SSL_VERIFYHOST, 2L);
#ifdef CURLWS_NOAUTOPONG
      curl_easy_setopt(client.get(), CURLOPT_WS_OPTIONS, CURLWS_NOAUTOPONG);
#endif
      require(curl_easy_perform(client.get()) == CURLE_OK);
      control_online=true;
      auto send = [&](const void *bytes, std::size_t size, unsigned flags) {
        std::size_t offset{};
        const auto deadline = Clock::now() + std::chrono::milliseconds(500);
        for (;;) {
          std::size_t sent{};
          const auto code = curl_ws_send(client.get(), static_cast<const std::byte*>(bytes)+offset, size-offset, &sent, 0, flags);
          require(code == CURLE_OK || code == CURLE_AGAIN);
          offset+=sent;
          if(offset==size)break;
          require(!stop.stop_requested() && Clock::now() < deadline);
          std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
      };
      std::vector<std::byte> assembled;
      bool loaded_sent = false, completion_sent = false, confirmation_sent = false;
      auto last = Clock::now();
      while (!stop.stop_requested()) {
        for (unsigned i = 0; i < 256; ++i) {
          std::byte chunk[1401]; std::size_t count{}; const curl_ws_frame *meta{};
          const auto result = curl_ws_recv(client.get(), chunk, sizeof(chunk), &count, &meta);
          if (result == CURLE_AGAIN) break;
          require(result == CURLE_OK && meta);
          if(meta->flags&CURLWS_CLOSE) {
            std::string reason="ONLINE match ended.";
            if(count>2 && count<=122)reason.assign(reinterpret_cast<const char*>(chunk+2),count-2);
            const auto code=count>=2?(std::to_integer<unsigned>(chunk[0])<<8)|std::to_integer<unsigned>(chunk[1]):1000;
            if(direct && code!=1001 && reason!="Match service is restarting.")throw TerminalMatchError(reason);
            throw std::runtime_error(reason);
          }
          last = Clock::now();
          if (meta->flags & CURLWS_PING) { require(count <= 125 && !meta->bytesleft); send(chunk, count, CURLWS_PONG); continue; }
          if (meta->flags & CURLWS_PONG) continue;
          require((meta->flags & (CURLWS_BINARY | CURLWS_TEXT)) && assembled.size() + count <= 119000);
          assembled.insert(assembled.end(), chunk, chunk + count);
          if (meta->bytesleft || (meta->flags & CURLWS_CONT)) continue;
          if (meta->flags & CURLWS_TEXT) {
            const auto data = Json::parse(reinterpret_cast<const char *>(assembled.data()), reinterpret_cast<const char *>(assembled.data()+assembled.size()));
            if(data.contains("peer_signal")) {
              require(direct && data.size()==1);
              const auto &signal=data["peer_signal"];
              require(signal.is_object() && signal.size()==4 && signal["peer"].is_number_unsigned() &&
                  signal["type"].is_string() && signal["payload"].is_string() && signal["mid"].is_string());
              direct->signal({signal["peer"].get<unsigned>(),signal["type"].get<std::string>(),
                  signal["payload"].get<std::string>(),signal["mid"].get<std::string>()});
            } else if(data.contains("world_seal")) {
              require(supports_continuity && data.size()==1);auto seal=parse_seal(data["world_seal"]);
              std::lock_guard lock(mutex);require(seal.epoch<=continuity_state.epoch);
              if(seals.size()==64)seals.pop_front();seals.push_back(std::move(seal));
            } else if(data.contains("postgame")) {
              require(data.size()==1 && data["postgame"].is_string());
              const auto hex=data["postgame"].get<std::string>();
              require(hex.size()==2*0xdc78 && hex.find_first_not_of("0123456789abcdef")==std::string::npos);
              std::vector<std::byte> bytes;bytes.reserve(0xdc78);
              for(std::size_t i=0;i<hex.size();i+=2)bytes.push_back(std::byte(std::stoul(hex.substr(i,2),nullptr,16)));
              std::lock_guard lock(mutex);require(phase==Phase::Running || (direct && (phase==Phase::Reviewing || phase==Phase::Postgame)));
              require(report.empty() || report==bytes);report=std::move(bytes);phase=Phase::Reviewing;
            } else if(data.contains("voice_blocked")) {
              require(data.size()==1 && data["voice_blocked"].is_number_unsigned());
              const auto mask=data["voice_blocked"].get<unsigned>();
              require(!(mask&~((1u<<roster.size())-1)) && !(mask&(1u<<self)));
              std::lock_guard lock(mutex);voice_blocked=std::uint16_t(mask);
            } else if(data.contains("voice_host")) {
              require(data.size()==2 && data["voice_host"].is_number_unsigned() && data["voice_members"].is_number_unsigned());
              const auto leader=data["voice_host"].get<unsigned>(),members=data["voice_members"].get<unsigned>();
              std::lock_guard lock(mutex);
              require((phase==Phase::Postgame || ((supports_continuity || transport_options.direct_udp) && phase==Phase::Reviewing)) && leader<roster.size() && (members&(1u<<leader)) &&
                  (members&(1u<<self)) && !(members&~((1u<<roster.size())-1)));
              require(online_admission_peer(roster[leader])==leader && !roster[leader].guest);
              for(const auto &p:roster)require(bool(members&(1u<<p.slot))==bool(members&(1u<<online_admission_peer(p))));
              host=leader;voice_members=std::uint16_t(members);
              for(unsigned slot=0;slot<16;++slot)if(!(members&(1u<<slot)))voice_enabled[slot]=false;
            } else if(data.contains("completed")) {
              require(data.size()==1 && data["completed"].is_string());
              std::lock_guard lock(mutex);require(phase==Phase::Reviewing && !confirmation.empty() && data["completed"].get<std::string>()==digest(report));
              phase=Phase::Postgame;
            } else if (data.contains("ping")) {
              require(data.size() == 1 && data["ping"].is_string() && data["ping"].get<std::string>().size() == 16);
              const auto pong = Json{{"pong", data["ping"]}}.dump(); send(pong.data(), pong.size(), CURLWS_TEXT);
            } else {
              control(data);
            }
          } else {
            require(!direct);
            const auto offset=supports_continuity?5u:1u;
            require(assembled.size()>=offset+7 && assembled.size()<=offset+1400);
            datagram(std::to_integer<unsigned>(assembled.front()),std::span(assembled).subspan(offset),supports_continuity?get(assembled,1,4):0);
          }
          assembled.clear();
        }
        require(Clock::now() - last < std::chrono::seconds(20));
        std::function<void()> pump;
        {std::lock_guard lock(mutex);pump=postgame_voice_pump;}
        if(pump && !direct)pump();
        std::deque<Frame> frames;std::deque<std::string> commands; std::string admission, publication, approval;
        { std::lock_guard lock(mutex); if(!direct)frames.swap(outgoing); if (!loaded_sent && phase == Phase::Loading) admission = loaded;
          commands.swap(continuation_outgoing);
          if(!completion_sent && phase==Phase::Running)publication=completion;
          if(!confirmation_sent && phase==Phase::Reviewing)approval=confirmation; }
        if (!admission.empty()) { const auto data = Json{{"loaded", admission}}.dump(); send(data.data(), data.size(), CURLWS_TEXT); loaded_sent = true; }
        for(const auto &command:commands)send(command.data(),command.size(),CURLWS_TEXT);
        for (auto &frame : frames) {
          if(supports_continuity) {
            std::vector<std::byte> prefix;put(prefix,frame.epoch,4);
            frame.data.insert(frame.data.begin(),prefix.begin(),prefix.end());
          }
          require(!direct);
          frame.data.insert(frame.data.begin(), std::byte(frame.peer));
          send(frame.data.data(), frame.data.size(), CURLWS_BINARY);
        }
        if(!publication.empty()) {send(publication.data(),publication.size(),CURLWS_TEXT);completion_sent=true;}
        if(!approval.empty()) {send(approval.data(),approval.size(),CURLWS_TEXT);confirmation_sent=true;}
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
      }
      if(direct) {
        const auto value=Json{{"leave",true}}.dump();
        try {send(value.data(),value.size(),CURLWS_TEXT);} catch(const std::exception &) {}
      }
      std::size_t ignored{};curl_ws_send(client.get(),"",0,&ignored,0,CURLWS_CLOSE);
  }

};
OnlineMatchSession::OnlineMatchSession(std::string o, std::string r, std::string b, unsigned s, std::vector<OnlineAdmission> roster, bool local,std::optional<bool> teams)
    : OnlineMatchSession(std::move(o),std::move(r),std::move(b),s,std::move(roster),local,teams,false) {}
OnlineMatchSession::OnlineMatchSession(std::string o,std::string r,std::string b,unsigned s,
    std::vector<OnlineAdmission> roster,bool local,std::optional<bool> teams,bool continuation)
    : OnlineMatchSession(std::move(o),std::move(r),std::move(b),s,std::move(roster),local,teams,continuation,{}) {}
OnlineMatchSession::OnlineMatchSession(std::string o,std::string r,std::string b,unsigned s,
    std::vector<OnlineAdmission> roster,bool local,std::optional<bool> teams,bool continuation,OnlineMatchTransportOptions transport)
    : impl_(std::make_unique<Impl>(std::move(o), std::move(r), std::move(b), s, std::move(roster), local,teams,continuation,std::move(transport))) {
  std::lock_guard lock(impl_->mutex);impl_->postgame_voice_pump=[this]{pump_postgame_voice();};
}
OnlineMatchSession::~OnlineMatchSession() {
  impl_->worker.request_stop();if(impl_->worker.joinable())impl_->worker.join();
  if(impl_->audio)impl_->audio->enable(false);
}
OnlineMatchSession::Phase OnlineMatchSession::phase() const { std::lock_guard lock(impl_->mutex); return impl_->phase; }
unsigned OnlineMatchSession::self() const { return impl_->self; }
bool OnlineMatchSession::direct_udp() const {return impl_->transport_options.direct_udp;}
bool OnlineMatchSession::master_connected() const {return impl_->control_online;}
std::vector<unsigned> OnlineMatchSession::local_slots() const {
  std::vector<unsigned> slots;
  for(const auto &player:impl_->roster)
    if(online_admission_peer(player)==impl_->self)slots.push_back(player.slot);
  return slots;
}
unsigned OnlineMatchSession::host() const { std::lock_guard lock(impl_->mutex); return impl_->host; }
std::optional<bool> OnlineMatchSession::team_mode() const { return impl_->team_mode; }
const std::vector<OnlineAdmission> &OnlineMatchSession::roster() const { return impl_->roster; }
std::string OnlineMatchSession::error() const { std::lock_guard lock(impl_->mutex); return impl_->error; }
std::optional<OnlineContinuityState> OnlineMatchSession::continuity() const {
  std::lock_guard lock(impl_->mutex);if(!impl_->supports_continuity)return std::nullopt;return impl_->continuity_state;
}
std::vector<OnlineWorldSeal> OnlineMatchSession::world_seals() {
  std::lock_guard lock(impl_->mutex);std::vector<OnlineWorldSeal> result(impl_->seals.begin(),impl_->seals.end());impl_->seals.clear();return result;
}
std::optional<AuthorityTransferContext> OnlineMatchSession::authority_context() const {
  std::shared_ptr<PeerAuthorityTransfer> transfer;{std::lock_guard lock(impl_->mutex);transfer=impl_->authority;}
  if(!transfer)return std::nullopt;
  return transfer->context();
}
bool OnlineMatchSession::authority_supported(unsigned peer) const {
  std::shared_ptr<PeerAuthorityTransfer> transfer;{std::lock_guard lock(impl_->mutex);transfer=impl_->authority;}
  return peer<16&&(impl_->direct_ready.load(std::memory_order_relaxed)&(1u<<peer))&&transfer&&transfer->supported(peer);
}
bool OnlineMatchSession::authority_replay_supported(unsigned peer) const {
  std::shared_ptr<PeerAuthorityTransfer> transfer;{std::lock_guard lock(impl_->mutex);transfer=impl_->authority;}
  return peer<16&&(impl_->direct_ready.load(std::memory_order_relaxed)&(1u<<peer))&&transfer&&
      transfer->feature_supported(peer,authority_feature_source_replay|authority_feature_source_state|authority_feature_source_camera|authority_feature_source_lens_query|authority_feature_source_lens_draw|authority_feature_source_particle_collision|authority_feature_source_projectile_contacts);
}
bool OnlineMatchSession::authority_pipeline_supported(unsigned peer)const{
  std::shared_ptr<PeerAuthorityTransfer> transfer;
  {std::lock_guard lock(impl_->mutex);transfer=impl_->authority;}
  return peer<16&&(impl_->direct_ready.load(std::memory_order_relaxed)&(1u<<peer))&&transfer&&
      transfer->feature_supported(peer,authority_feature_source_pipeline);
}
bool OnlineMatchSession::authority_delivered(unsigned peer,const AuthorityTransferContext &context,const AuthorityCutSeal &seal)const{
  std::shared_ptr<PeerAuthorityTransfer> transfer;{std::lock_guard lock(impl_->mutex);transfer=impl_->authority;}
  return transfer&&transfer->delivered_to(context,seal,peer);
}
bool OnlineMatchSession::authority_host_continuation_supported(unsigned peer)const{
  std::shared_ptr<PeerAuthorityTransfer> transfer;{std::lock_guard lock(impl_->mutex);transfer=impl_->authority;}
  return peer<16&&(impl_->direct_ready.load(std::memory_order_relaxed)&(1u<<peer))&&transfer&&
      transfer->feature_supported(peer,authority_feature_source_host_continuation);
}
std::vector<PeerHostQuality> OnlineMatchSession::host_candidates()const {
  std::shared_ptr<PeerMeshQuality> quality;unsigned host;Phase phase;
  {std::lock_guard lock(impl_->mutex);quality=impl_->mesh_quality;host=impl_->host;phase=impl_->phase;}
  if(!quality||phase==Phase::Connecting||phase==Phase::Failed||quality->context().host!=host)return {};
  return quality->candidates(transfer_time());
}
bool OnlineMatchSession::publish_source_players(const AuthorityTransferContext &context,std::span<const SourcePlayerAdmission> players) {
  std::shared_ptr<PeerSourceAdmissions> admissions;
  {std::lock_guard lock(impl_->mutex);require(impl_->transport_options.direct_udp&&impl_->phase==Phase::Running&&impl_->host==impl_->self);admissions=impl_->source_admissions;}
  return admissions&&admissions->publish(context,players);
}
std::optional<AcceptedSourcePlayers> OnlineMatchSession::source_players()const {
  std::shared_ptr<PeerSourceAdmissions> admissions;{std::lock_guard lock(impl_->mutex);admissions=impl_->source_admissions;}
  return admissions?admissions->accepted():std::nullopt;
}
bool OnlineMatchSession::source_players_confirmed(unsigned peer)const {
  std::shared_ptr<PeerSourceAdmissions> admissions;{std::lock_guard lock(impl_->mutex);admissions=impl_->source_admissions;}
  return peer<16&&(impl_->direct_ready.load(std::memory_order_relaxed)&(1u<<peer))&&admissions&&admissions->confirmed(peer);
}
AuthorityCutSeal OnlineMatchSession::publish_authority(const AuthorityTransferContext &context,std::uint64_t tick,
    std::vector<std::byte> archive,std::span<const unsigned> targets) {
  std::shared_ptr<PeerAuthorityTransfer> transfer;
  {std::lock_guard lock(impl_->mutex);require(impl_->transport_options.direct_udp&&impl_->phase==Phase::Running&&impl_->host==impl_->self);
    transfer=impl_->authority;require(bool(transfer));}
  return transfer->publish(context,tick,std::move(archive),targets,transfer_time());
}
std::optional<ReceivedAuthorityCut> OnlineMatchSession::received_authority() {
  std::shared_ptr<PeerAuthorityTransfer> transfer;{std::lock_guard lock(impl_->mutex);transfer=impl_->authority;}
  if(!transfer)return std::nullopt;
  return transfer->take_completed();
}
AuthorityCutSeal OnlineMatchSession::publish_authority_ahead(const AuthorityTransferContext &context,const AuthorityCutSeal &previous,
    std::uint64_t tick,std::vector<std::byte> bytes,unsigned target){
  std::shared_ptr<PeerAuthorityTransfer> transfer;
  {std::lock_guard lock(impl_->mutex);require(impl_->transport_options.direct_udp&&impl_->phase==Phase::Running&&impl_->host==impl_->self);
    transfer=impl_->authority;require(bool(transfer));}
  return transfer->publish_ahead(context,previous,tick,std::move(bytes),target,transfer_time());
}
bool OnlineMatchSession::acknowledge_authority(const AuthorityTransferContext &context,const AuthorityCutSeal &seal) {
  std::shared_ptr<PeerAuthorityTransfer> transfer;{std::lock_guard lock(impl_->mutex);transfer=impl_->authority;}
  return transfer&&transfer->acknowledge_validated(context,seal);
}
bool OnlineMatchSession::reject_authority(const AuthorityTransferContext &context,const AuthorityCutSeal &seal){
  std::shared_ptr<PeerAuthorityTransfer> transfer;{std::lock_guard lock(impl_->mutex);transfer=impl_->authority;}
  return transfer&&transfer->reject_delivered(context,seal);
}
std::vector<ValidatedAuthorityCut> OnlineMatchSession::validated_authority() {
  std::shared_ptr<PeerAuthorityTransfer> transfer;{std::lock_guard lock(impl_->mutex);transfer=impl_->authority;}
  if(!transfer)return {};
  return transfer->validated_cuts();
}
void OnlineMatchSession::seal_world(std::uint64_t tick,const std::string &hash) {
  std::lock_guard lock(impl_->mutex);require(impl_->phase==Phase::Running && impl_->host==impl_->self);
  impl_->queue_world("world_seal",{impl_->continuity_state.epoch,tick,hash});
}
void OnlineMatchSession::acknowledge_world(const OnlineWorldSeal &seal) {
  std::lock_guard lock(impl_->mutex);
  require((impl_->phase==Phase::Running && seal.epoch==impl_->continuity_state.epoch) ||
      (impl_->phase==Phase::AwaitingWorld && seal.epoch+std::uint64_t{1}==impl_->continuity_state.epoch));
  impl_->queue_world("world_ack",seal);
}
void OnlineMatchSession::accept_migration(std::uint64_t tick,const std::string &hash) {
  std::lock_guard lock(impl_->mutex);const auto &selected=impl_->continuity_state.selected;
  require(impl_->phase==Phase::Migrating && impl_->self==impl_->host && selected && selected->tick==tick && selected->digest==hash);
  impl_->queue_world("migration_accept",*selected);
}
void OnlineMatchSession::rebind_migration(std::uint64_t tick,const std::string &hash) {
  std::lock_guard lock(impl_->mutex);const auto &selected=impl_->continuity_state.selected;
  require(impl_->phase==Phase::Rebinding && impl_->self!=impl_->host && selected && selected->tick==tick && selected->digest==hash);
  impl_->queue_world("migration_rebind",*selected);
}
void OnlineMatchSession::loaded(std::uint64_t fingerprint,const UiVariant& variant) {
  // The cache/weapon fingerprint alone does not distinguish score limits,
  // clocks, life limits or the other authored variant settings.
  const auto source=source_game_variant(variant);std::vector<std::byte> admitted;
  for(unsigned i=0;i<8;++i)admitted.push_back(std::byte(fingerprint>>(8*i)));
  for(auto byte:source.bytes)admitted.push_back(std::byte(byte));
  for(const auto &[id,value]:variant.values)if(id>=native_slayer_ruleset_parameter)
    for(auto word:{id,unsigned(value)})for(unsigned i=0;i<4;++i)admitted.push_back(std::byte(word>>(8*i)));
  std::ostringstream value; value << digest(admitted).substr(0,16) << ':' << std::hex << std::setfill('0') << std::setw(7) << native_multiplayer_protocol;
  std::lock_guard lock(impl_->mutex); impl_->loaded = value.str();impl_->native_fingerprint=fingerprint;
}
std::optional<std::uint64_t> OnlineMatchSession::loaded_fingerprint()const{
  std::lock_guard lock(impl_->mutex);return impl_->native_fingerprint;
}
void OnlineMatchSession::send(std::span<const std::byte> packet, unsigned peer) {
  std::lock_guard lock(impl_->mutex);
  if (impl_->phase != Phase::Running || peer >= impl_->roster.size() || packet.size() > 1400 || impl_->outgoing.size() >= 512) return;
  if(impl_->supports_continuity && !(impl_->continuity_state.members&(1u<<peer)))return;
  impl_->outgoing.push_back({peer, {packet.begin(), packet.end()},impl_->continuity_state.epoch});
}
bool OnlineMatchSession::receive(std::vector<std::byte> &packet, unsigned &peer) {
  std::lock_guard lock(impl_->mutex);
  if (impl_->incoming.empty()) return false;
  auto frame = std::move(impl_->incoming.front()); impl_->incoming.pop_front();
  peer = frame.peer; packet = std::move(frame.data); return true;
}
void OnlineMatchSession::voice(unsigned target,const GameVoiceFrame &frame) {
  if(frame.source>=16 || frame.opus.empty() || frame.opus.size()>256)return;
  auto packet=voice_header(0);put(packet,frame.source,1);put(packet,frame.sequence,4);put(packet,frame.radio,1);
  put(packet,std::bit_cast<unsigned>(frame.gains.left),4);put(packet,std::bit_cast<unsigned>(frame.gains.right),4);
  for(auto byte:frame.opus)packet.push_back(std::byte(byte));
  std::lock_guard lock(impl_->mutex);
  if((impl_->phase==Phase::Running || impl_->phase==Phase::Reviewing || impl_->phase==Phase::Postgame) &&
      target<impl_->roster.size() && impl_->outgoing.size()<512)impl_->outgoing.push_back({target,std::move(packet),impl_->continuity_state.epoch});
}
void OnlineMatchSession::voice_preferences(unsigned target,bool enabled,std::uint16_t muted) {
  auto packet=voice_header(1);put(packet,enabled,1);put(packet,muted,2);
  std::lock_guard lock(impl_->mutex);
  if((impl_->phase==Phase::Running || impl_->phase==Phase::Reviewing || impl_->phase==Phase::Postgame) &&
      target<impl_->roster.size() && impl_->outgoing.size()<512)impl_->outgoing.push_back({target,std::move(packet),impl_->continuity_state.epoch});
}
std::uint16_t OnlineMatchSession::voice_blocks() const {std::lock_guard lock(impl_->mutex);return impl_->voice_blocked;}
std::vector<MatchVoicePacket> OnlineMatchSession::voice_packets() {
  std::lock_guard lock(impl_->mutex);std::vector<MatchVoicePacket> packets(impl_->voices.begin(),impl_->voices.end());impl_->voices.clear();return packets;
}
void OnlineMatchSession::retain_voice(std::shared_ptr<GameVoice> audio) {
  std::lock_guard lock(impl_->mutex);require(!impl_->audio || impl_->audio==audio);impl_->audio=std::move(audio);
}
std::shared_ptr<GameVoice> OnlineMatchSession::retained_voice() const {
  std::lock_guard lock(impl_->mutex);return impl_->audio;
}
bool OnlineMatchSession::postgame_voice_enabled(unsigned slot) const {
  std::lock_guard lock(impl_->mutex);return slot<impl_->roster.size() && (impl_->voice_members&(1u<<slot)) && impl_->voice_enabled[online_admission_peer(impl_->roster[slot])];
}
void OnlineMatchSession::update_postgame_voice(bool focused) {
  impl_->voice_focused=focused;
}
void OnlineMatchSession::pump_postgame_voice() {
  const auto audio=retained_voice();if(!audio)return;
  const auto phase_now=phase();
  if(phase_now==Phase::Failed){audio->enable(false);return;}
  if(phase_now!=Phase::Reviewing && phase_now!=Phase::Postgame)return;
  audio->focus(impl_->voice_focused);audio->radio(false,true);
  audio->blocks(voice_blocks());
  const auto selected=audio->snapshot();const auto local=self(),leader=host();
  const bool hosting=local==leader;
  {std::lock_guard lock(impl_->mutex);impl_->voice_enabled[local]=selected.enabled;impl_->voice_mutes[local]=selected.muted;}
  const auto now=Clock::now();
  const double elapsed=std::chrono::duration<double>(now-impl_->voice_credit_at).count();impl_->voice_credit_at=now;
  for(auto &credit:impl_->voice_credits)credit=std::min(10.,credit+50*elapsed);
  const bool announce=now-impl_->voice_preferences_at>std::chrono::milliseconds(200);
  if(announce) {
    impl_->voice_preferences_at=now;
    if(!hosting)voice_preferences(leader,selected.enabled,selected.muted);
  }
  auto route=[&](GameVoiceFrame frame) {
    const auto source=frame.source;if(source>=roster().size() || !(impl_->voice_members&(1u<<source)))return;
    if(frame.sequence<=impl_->voice_sequences[source] || impl_->voice_credits[source]<1)return;
    impl_->voice_credits[source]-=1;
    impl_->voice_sequences[source]=frame.sequence;
    // Original passive mode2 uses session membership, without world distance
    // or team filtering. Both ends' actual mute/enable preferences still apply.
    for(const auto &p:roster())if(online_admission_peer(p)==p.slot && p.slot!=online_admission_peer(roster()[source]) && (impl_->voice_members&(1u<<p.slot)) && impl_->voice_enabled[source] && impl_->voice_enabled[p.slot] &&
        !(impl_->voice_mutes[source]&(1u<<p.slot)) && !(impl_->voice_mutes[p.slot]&(1u<<source))) {
      frame.gains={1,1};frame.radio=false;
      if(p.slot==local)audio->receive(frame);else voice(p.slot,frame);
    }
  };
  for(auto &packet:voice_packets()) {
    if(packet.preferences) {
      const auto source=hosting?packet.peer:packet.frame.source;
      if(source>=roster().size() || source==local || (!hosting && packet.peer!=leader))continue;
      std::lock_guard lock(impl_->mutex);impl_->voice_enabled[source]=packet.enabled;impl_->voice_mutes[source]=packet.muted;
    } else if(hosting) {packet.frame.source=packet.peer;route(std::move(packet.frame));}
    else if(packet.peer==leader && packet.frame.source<roster().size())audio->receive(std::move(packet.frame));
  }
  if(hosting && announce)for(const auto &p:roster())if(online_admission_peer(p)==p.slot && p.slot!=local && (impl_->voice_members&(1u<<p.slot)))for(const auto &member:roster()) {
    const auto peer=online_admission_peer(member);
    auto packet=voice_header(2);put(packet,impl_->voice_enabled[peer],1);
    put(packet,impl_->voice_mutes[peer],2);put(packet,member.slot,1);
    std::lock_guard lock(impl_->mutex);if(impl_->outgoing.size()<512)impl_->outgoing.push_back({p.slot,std::move(packet),impl_->continuity_state.epoch});
  }
  for(auto &frame:audio->outgoing()){frame.source=local;if(hosting)route(std::move(frame));else voice(leader,frame);}
}
void OnlineMatchSession::close_postgame() {
  if(const auto audio=retained_voice())audio->close();
  impl_->worker.request_stop();if(impl_->worker.joinable())impl_->worker.join();
}
void OnlineMatchSession::complete(std::span<const std::byte> report) {
  require(report.size()==0xdc78);
  std::string hex;hex.reserve(report.size()*2);constexpr char digits[]="0123456789abcdef";
  for(auto b:report){auto n=std::to_integer<unsigned>(b);hex+=digits[n>>4];hex+=digits[n&15];}
  std::lock_guard lock(impl_->mutex);require(impl_->self==impl_->host && impl_->phase==Phase::Running);
  if(impl_->completion.empty())impl_->completion=Json{{"postgame",hex}}.dump();
}
void OnlineMatchSession::confirm_result() {
  std::lock_guard lock(impl_->mutex);require(impl_->phase==Phase::Reviewing && impl_->report.size()==0xdc78);
  if(impl_->confirmation.empty())impl_->confirmation=Json{{"confirm_result",digest(impl_->report)}}.dump();
}
std::vector<std::byte> OnlineMatchSession::postgame() const {
  std::lock_guard lock(impl_->mutex);return impl_->report;
}
} // namespace halo2
