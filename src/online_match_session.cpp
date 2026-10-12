#include "halo2/online_match_session.hpp"
#include "halo2/source_game_variant.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <openssl/sha.h>
#include <chrono>
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
struct Frame { unsigned peer; std::vector<std::byte> data; };
void put(std::vector<std::byte>& p,unsigned value,unsigned count){for(unsigned i=0;i<count;++i)p.push_back(std::byte(value>>(i*8)));}
unsigned get(std::span<const std::byte> p,unsigned at,unsigned count){require(at+count<=p.size());unsigned n{};for(unsigned i=0;i<count;++i)n|=std::to_integer<unsigned>(p[at+i])<<(i*8);return n;}
std::vector<std::byte> voice_header(unsigned type){std::vector<std::byte> p;put(p,0x324e3248,4);put(p,native_multiplayer_protocol,2);put(p,6,1);put(p,type,1);return p;}
std::string digest(std::span<const std::byte> bytes) {
  unsigned char hash[SHA256_DIGEST_LENGTH];require(SHA256(reinterpret_cast<const unsigned char*>(bytes.data()),bytes.size(),hash));
  constexpr char digits[]="0123456789abcdef";std::string result;result.reserve(64);
  for(auto n:hash){result+=digits[n>>4];result+=digits[n&15];}return result;
}
}
struct OnlineMatchSession::Impl {
  unsigned self, host = 16;
  const std::vector<OnlineAdmission> roster;
  mutable std::mutex mutex;
  Phase phase = Phase::Connecting;
  std::string error, loaded;
  std::string completion, confirmation;
  std::vector<std::byte> report;
  std::deque<Frame> incoming, outgoing;
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
  std::function<void()> postgame_voice_pump;
  std::jthread worker;
  Impl(std::string origin, std::string room, std::string bearer, unsigned s,
       std::vector<OnlineAdmission> r, bool local) : self(s), roster(std::move(r)) {
    require(self < roster.size() && roster.size() >= 2 && roster.size() <= 16);
    voice_members=std::uint16_t((1u<<roster.size())-1);
    require(room.size() == 36 && room.find_first_not_of("0123456789abcdef-") == std::string::npos);
    require(bearer.size() >= 32 && bearer.size() <= 128 && bearer.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_") == std::string::npos);
    require(origin.starts_with("https://") || (local && origin.starts_with("http://127.0.0.1:")));
    require(origin.find_first_of("@?#\\\r\n\t ") == std::string::npos);
    const auto scheme = origin.find(':');
    const auto url = std::string(origin.starts_with("https:") ? "wss" : "ws") + origin.substr(scheme) + "/v1/matches/" + room + "/relay";
    worker = std::jthread([this, url, bearer](std::stop_token stop) { run(url, bearer, stop); });
  }
  ~Impl() { worker.request_stop(); if (worker.joinable()) worker.join(); }
  void run(const std::string &url, const std::string &bearer, std::stop_token stop) {
    try {
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
            if(data.contains("postgame")) {
              require(data.size()==1 && data["postgame"].is_string());
              const auto hex=data["postgame"].get<std::string>();
              require(hex.size()==2*0xdc78 && hex.find_first_not_of("0123456789abcdef")==std::string::npos);
              std::vector<std::byte> bytes;bytes.reserve(0xdc78);
              for(std::size_t i=0;i<hex.size();i+=2)bytes.push_back(std::byte(std::stoul(hex.substr(i,2),nullptr,16)));
              std::lock_guard lock(mutex);require(phase==Phase::Running);
              report=std::move(bytes);phase=Phase::Reviewing;
            } else if(data.contains("voice_blocked")) {
              require(data.size()==1 && data["voice_blocked"].is_number_unsigned());
              const auto mask=data["voice_blocked"].get<unsigned>();
              require(!(mask&~((1u<<roster.size())-1)) && !(mask&(1u<<self)));
              std::lock_guard lock(mutex);voice_blocked=std::uint16_t(mask);
            } else if(data.contains("voice_host")) {
              require(data.size()==2 && data["voice_host"].is_number_unsigned() && data["voice_members"].is_number_unsigned());
              const auto leader=data["voice_host"].get<unsigned>(),members=data["voice_members"].get<unsigned>();
              std::lock_guard lock(mutex);
              require(phase==Phase::Postgame && leader<roster.size() && (members&(1u<<leader)) &&
                  (members&(1u<<self)) && !(members&~((1u<<roster.size())-1)));
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
              require(data.size() == 2 && data["phase"].is_string());
              const auto p = data["phase"].get<std::string>();
              require(p == "connecting" || p == "loading" || p == "running");
              std::lock_guard lock(mutex);
              if (p != "connecting") { require(data["host"].is_number_unsigned() && data["host"] < roster.size()); host = data["host"].get<unsigned>(); }
              phase = p == "running" ? Phase::Running : p == "loading" ? Phase::Loading : Phase::Connecting;
            }
          } else {
            require(assembled.size() >= 8 && assembled.size()<=1401 && std::to_integer<unsigned>(assembled.front()) < roster.size());
            std::lock_guard lock(mutex);
            const auto packet=std::span(assembled).subspan(1);
            if(get(packet,6,1)==6) {
              require(get(packet,0,4)==0x324e3248 && get(packet,4,2)==native_multiplayer_protocol);
              MatchVoicePacket voice;voice.peer=std::to_integer<unsigned>(assembled.front());
              const auto subtype=get(packet,7,1);
              if(subtype==1 || subtype==2) {
                require(packet.size()==(subtype==1?11u:12u) && (subtype==1 || voice.peer==host));
                voice.preferences=true;voice.frame.source=subtype==1?voice.peer:get(packet,11,1);
                const auto enabled=get(packet,8,1);require(enabled<=1 && voice.frame.source<roster.size());
                voice.enabled=enabled;voice.muted=get(packet,9,2);
              }
              else {
                require(subtype==0 && packet.size()>22 && packet.size()<=278);
                voice.frame.source=get(packet,8,1);voice.frame.sequence=get(packet,9,4);
                const auto radio=get(packet,13,1);require(radio<=1 && voice.frame.source<16);voice.frame.radio=radio;
                voice.frame.gains={std::bit_cast<float>(get(packet,14,4)),std::bit_cast<float>(get(packet,18,4))};
                require(std::isfinite(voice.frame.gains.left) && std::isfinite(voice.frame.gains.right) && voice.frame.gains.left>=0 && voice.frame.gains.left<=1 && voice.frame.gains.right>=0 && voice.frame.gains.right<=1);
                for(auto byte:packet.subspan(22))voice.frame.opus.push_back(std::to_integer<std::uint8_t>(byte));
              }
              if(voices.size()==80)voices.pop_front();
              voices.push_back(std::move(voice));
            } else {require(incoming.size() < 512); incoming.push_back({std::to_integer<unsigned>(assembled.front()), {assembled.begin()+1, assembled.end()}});}
          }
          assembled.clear();
        }
        require(Clock::now() - last < std::chrono::seconds(20));
        std::function<void()> pump;
        {std::lock_guard lock(mutex);pump=postgame_voice_pump;}
        if(pump)pump();
        std::deque<Frame> frames; std::string admission, publication, approval;
        { std::lock_guard lock(mutex); frames.swap(outgoing); if (!loaded_sent && phase == Phase::Loading) admission = loaded;
          if(!completion_sent && phase==Phase::Running)publication=completion;
          if(!confirmation_sent && phase==Phase::Reviewing)approval=confirmation; }
        if (!admission.empty()) { const auto data = Json{{"loaded", admission}}.dump(); send(data.data(), data.size(), CURLWS_TEXT); loaded_sent = true; }
        for (auto &frame : frames) {
          frame.data.insert(frame.data.begin(), std::byte(frame.peer));
          send(frame.data.data(), frame.data.size(), CURLWS_BINARY);
        }
        if(!publication.empty()) {send(publication.data(),publication.size(),CURLWS_TEXT);completion_sent=true;}
        if(!approval.empty()) {send(approval.data(),approval.size(),CURLWS_TEXT);confirmation_sent=true;}
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
      }
      std::size_t ignored{}; curl_ws_send(client.get(), "", 0, &ignored, 0, CURLWS_CLOSE);
    } catch (const std::exception &e) {
      std::shared_ptr<GameVoice> stopped_audio;
      {std::lock_guard lock(mutex);if(phase!=Phase::Postgame)phase=Phase::Failed;
        error=e.what();voice_enabled.fill(false);stopped_audio=audio;}
      if(stopped_audio)stopped_audio->enable(false);
    }
  }
};
OnlineMatchSession::OnlineMatchSession(std::string o, std::string r, std::string b, unsigned s, std::vector<OnlineAdmission> roster, bool local)
    : impl_(std::make_unique<Impl>(std::move(o), std::move(r), std::move(b), s, std::move(roster), local)) {
  std::lock_guard lock(impl_->mutex);impl_->postgame_voice_pump=[this]{pump_postgame_voice();};
}
OnlineMatchSession::~OnlineMatchSession() {
  impl_->worker.request_stop();if(impl_->worker.joinable())impl_->worker.join();
  if(impl_->audio)impl_->audio->enable(false);
}
OnlineMatchSession::Phase OnlineMatchSession::phase() const { std::lock_guard lock(impl_->mutex); return impl_->phase; }
unsigned OnlineMatchSession::self() const { return impl_->self; }
unsigned OnlineMatchSession::host() const { std::lock_guard lock(impl_->mutex); return impl_->host; }
const std::vector<OnlineAdmission> &OnlineMatchSession::roster() const { return impl_->roster; }
std::string OnlineMatchSession::error() const { std::lock_guard lock(impl_->mutex); return impl_->error; }
void OnlineMatchSession::loaded(std::uint64_t fingerprint,const UiVariant& variant) {
  // The cache/weapon fingerprint alone does not distinguish score limits,
  // clocks, life limits or the other authored variant settings.
  const auto source=source_game_variant(variant);std::vector<std::byte> admitted;
  for(unsigned i=0;i<8;++i)admitted.push_back(std::byte(fingerprint>>(8*i)));
  for(auto byte:source.bytes)admitted.push_back(std::byte(byte));
  std::ostringstream value; value << digest(admitted).substr(0,16) << ':' << std::hex << std::setfill('0') << std::setw(7) << native_multiplayer_protocol;
  std::lock_guard lock(impl_->mutex); impl_->loaded = value.str();
}
void OnlineMatchSession::send(std::span<const std::byte> packet, unsigned peer) {
  std::lock_guard lock(impl_->mutex);
  if (impl_->phase != Phase::Running || peer >= impl_->roster.size() || packet.size() > 1400 || impl_->outgoing.size() >= 512) return;
  impl_->outgoing.push_back({peer, {packet.begin(), packet.end()}});
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
      target<impl_->roster.size() && impl_->outgoing.size()<512)impl_->outgoing.push_back({target,std::move(packet)});
}
void OnlineMatchSession::voice_preferences(unsigned target,bool enabled,std::uint16_t muted) {
  auto packet=voice_header(1);put(packet,enabled,1);put(packet,muted,2);
  std::lock_guard lock(impl_->mutex);
  if((impl_->phase==Phase::Running || impl_->phase==Phase::Reviewing || impl_->phase==Phase::Postgame) &&
      target<impl_->roster.size() && impl_->outgoing.size()<512)impl_->outgoing.push_back({target,std::move(packet)});
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
  std::lock_guard lock(impl_->mutex);return slot<impl_->roster.size() && (impl_->voice_members&(1u<<slot)) && impl_->voice_enabled[slot];
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
    for(const auto &p:roster())if(p.slot!=source && (impl_->voice_members&(1u<<p.slot)) && impl_->voice_enabled[source] && impl_->voice_enabled[p.slot] &&
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
  if(hosting && announce)for(const auto &p:roster())if(p.slot!=local && (impl_->voice_members&(1u<<p.slot)))for(const auto &member:roster()) {
    auto packet=voice_header(2);put(packet,impl_->voice_enabled[member.slot],1);
    put(packet,impl_->voice_mutes[member.slot],2);put(packet,member.slot,1);
    std::lock_guard lock(impl_->mutex);if(impl_->outgoing.size()<512)impl_->outgoing.push_back({p.slot,std::move(packet)});
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
