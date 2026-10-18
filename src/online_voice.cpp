#include "halo2/online_voice.hpp"
#include "halo2/online_account.hpp"
#include <SDL.h>
#include <curl/curl.h>
#include <opus.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <deque>
#include <map>
#include <mutex>
#include <set>
#include <stdexcept>
#include <thread>
#ifndef _WIN32
#include <sys/select.h>
#endif

namespace halo2 {
namespace {
using Clock = std::chrono::steady_clock;
void require(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
class SdlVoiceDevices final : public VoiceDevices {
  SDL_AudioDeviceID microphone{}, output{};
  bool initialized{}, transmitting{};
public:
  ~SdlVoiceDevices() override { stop(); }
  void start() override {
    stop();
    require(SDL_InitSubSystem(SDL_INIT_AUDIO) == 0,"Voice audio initialization failed."); initialized=true;
    SDL_AudioSpec requested{}, actual{};
    requested.freq=48000; requested.format=AUDIO_F32SYS; requested.channels=2; requested.samples=960;
    output=SDL_OpenAudioDevice(nullptr,0,&requested,&actual,0);
    require(output!=0,"Voice output device unavailable.");
    requested.channels=1; microphone=SDL_OpenAudioDevice(nullptr,1,&requested,&actual,0);
    require(microphone!=0,"Microphone unavailable. Check device permissions.");
    // Capture remains paused until the player holds the talk key.
    SDL_PauseAudioDevice(output,0);
  }
  void transmit(bool value) override {
    if (value==transmitting) return;
    transmitting=value; SDL_ClearQueuedAudio(microphone); SDL_PauseAudioDevice(microphone,value?0:1);
  }
  bool capture(VoicePcm &pcm) override {
    if (!transmitting || SDL_GetQueuedAudioSize(microphone)<sizeof(pcm)) return false;
    if(SDL_GetQueuedAudioSize(microphone)>5*sizeof(pcm)) SDL_ClearQueuedAudio(microphone);
    return SDL_DequeueAudio(microphone,pcm.data(),sizeof(pcm))==sizeof(pcm);
  }
  void playback(const VoicePcm &pcm) override {
    VoiceStereo stereo{};for(unsigned i=0;i<voice_frame_samples;++i)stereo[i*2]=stereo[i*2+1]=pcm[i];
    playback_stereo(stereo);
  }
  void playback_stereo(const VoiceStereo &pcm) override {
    if(SDL_GetQueuedAudioSize(output)>3*sizeof(pcm)) SDL_ClearQueuedAudio(output);
    require(SDL_QueueAudio(output,pcm.data(),sizeof(pcm))==0,"Voice playback failed.");
  }
  void discard() override {
    if(microphone)SDL_ClearQueuedAudio(microphone);
    if(output)SDL_ClearQueuedAudio(output);
  }
  void stop() override {
    if(microphone) SDL_CloseAudioDevice(microphone);
    if(output) SDL_CloseAudioDevice(output);
    microphone=output=0; transmitting=false;
    if(initialized) SDL_QuitSubSystem(SDL_INIT_AUDIO);
    initialized=false;
  }
};
std::string uuid(std::span<const std::uint8_t> bytes) {
  constexpr char hex[]="0123456789abcdef";
  std::string out;
  for(unsigned i=0;i<16;++i) { if(i==4 || i==6 || i==8 || i==10)out+='-'; out+=hex[bytes[i]>>4];out+=hex[bytes[i]&15]; }
  return out;
}
std::string websocket_origin(const VoiceAuthorization &authorization) {
  const auto &origin=authorization.origin;
  require(origin.size()<=2048 && origin.find_first_of("@?#\\\r\n\t ")==std::string::npos,"Invalid voice origin.");
  std::unique_ptr<CURLU,decltype(&curl_url_cleanup)> url(curl_url(),curl_url_cleanup);
  require(url && curl_url_set(url.get(),CURLUPART_URL,origin.c_str(),0)==CURLUE_OK,"Invalid voice origin.");
  auto part=[&](CURLUPart key) {
    char *raw=nullptr; auto code=curl_url_get(url.get(),key,&raw,0);
    std::unique_ptr<char,decltype(&curl_free)> owned(raw,curl_free);
    return code==CURLUE_OK && raw?std::string(raw):std::string{};
  };
  const auto scheme=part(CURLUPART_SCHEME), host=part(CURLUPART_HOST);
  require(!host.empty() && part(CURLUPART_PATH)=="/" && part(CURLUPART_USER).empty(),"Invalid voice origin.");
  require(scheme=="https" || (authorization.allow_loopback_http && scheme=="http" && (host=="127.0.0.1" || host=="[::1]")),"Voice requires HTTPS.");
  require(!authorization.bearer.empty() && authorization.bearer.size()<=128 &&
      authorization.bearer.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_")==std::string::npos,"Invalid voice authorization.");
  return std::string(scheme=="https"?"wss":"ws")+origin.substr(scheme.size())+(authorization.party?"/v1/party/voice":"/v1/clans/voice");
}
void wait_socket(CURL *curl, bool write) {
  curl_socket_t socket=CURL_SOCKET_BAD;
  if(curl_easy_getinfo(curl,CURLINFO_ACTIVESOCKET,&socket)!=CURLE_OK || socket==CURL_SOCKET_BAD) return;
#ifndef _WIN32
  if(socket>=FD_SETSIZE) { std::this_thread::sleep_for(std::chrono::milliseconds(5)); return; }
#endif
  fd_set descriptors; FD_ZERO(&descriptors); FD_SET(socket,&descriptors);
  timeval timeout{0,5000};
  select(int(socket)+1,write?nullptr:&descriptors,write?&descriptors:nullptr,nullptr,&timeout);
}
} // namespace
struct VoiceCodec::Impl {
  std::unique_ptr<OpusEncoder,decltype(&opus_encoder_destroy)> encoder{nullptr,opus_encoder_destroy};
  std::unique_ptr<OpusDecoder,decltype(&opus_decoder_destroy)> decoder{nullptr,opus_decoder_destroy};
  Impl(bool dtx) {
    int error{}; encoder.reset(opus_encoder_create(48000,1,OPUS_APPLICATION_VOIP,&error));
    require(encoder && error==OPUS_OK,"Voice encoder unavailable.");
    decoder.reset(opus_decoder_create(48000,1,&error)); require(decoder && error==OPUS_OK,"Voice decoder unavailable.");
    require(opus_encoder_ctl(encoder.get(),OPUS_SET_BITRATE(24000))==OPUS_OK &&
            opus_encoder_ctl(encoder.get(),OPUS_SET_COMPLEXITY(5))==OPUS_OK &&
            opus_encoder_ctl(encoder.get(),OPUS_SET_VBR_CONSTRAINT(1))==OPUS_OK &&
            opus_encoder_ctl(encoder.get(),OPUS_SET_DTX(dtx))==OPUS_OK,"Voice encoder configuration failed.");
  }
};
VoiceCodec::VoiceCodec(bool dtx):impl_(std::make_unique<Impl>(dtx)){}
VoiceCodec::~VoiceCodec()=default;
bool VoiceCodec::speech()const {int dtx{};return opus_encoder_ctl(impl_->encoder.get(),OPUS_GET_IN_DTX(&dtx))==OPUS_OK && !dtx;}
std::unique_ptr<VoiceDevices> sdl_voice_devices(){return std::make_unique<SdlVoiceDevices>();}
std::vector<std::uint8_t> VoiceCodec::encode(const VoicePcm &input) {
  VoicePcm pcm=input;
  for(auto &v:pcm) v=std::isfinite(v)?std::clamp(v,-1.f,1.f):0.f;
  std::vector<std::uint8_t> packet(256);
  const int size=opus_encode_float(impl_->encoder.get(),pcm.data(),voice_frame_samples,packet.data(),int(packet.size()));
  require(size>0,"Voice encoding failed."); packet.resize(unsigned(size)); return packet;
}
std::optional<VoicePcm> VoiceCodec::decode(std::span<const std::uint8_t> packet) {
  if(packet.empty() || packet.size()>256 || opus_packet_get_nb_samples(packet.data(),int(packet.size()),48000)!=voice_frame_samples) return {};
  VoicePcm pcm{};
  if(opus_decode_float(impl_->decoder.get(),packet.data(),int(packet.size()),pcm.data(),voice_frame_samples,0)!=voice_frame_samples) return {};
  for(auto &v:pcm) if(!std::isfinite(v))return {}; else v=std::clamp(v,-1.f,1.f);
  return pcm;
}
struct OnlineVoice::Impl {
  AuthorizationSource authorization;
  std::unique_ptr<VoiceDevices> devices;
  mutable std::mutex mutex;
  std::condition_variable_any wake;
  OnlineVoiceSnapshot state;
  bool enabled{}, held{};
  std::uint64_t generation{};
  std::set<std::string> muted;
  std::jthread worker;
  Impl(AuthorizationSource source,std::unique_ptr<VoiceDevices> audio):authorization(std::move(source)),devices(std::move(audio)) {
    require(bool(authorization) && bool(devices),"Missing voice service.");
    require(curl_global_init(CURL_GLOBAL_DEFAULT)==CURLE_OK,"Voice networking unavailable.");
    state.message="Voice is off."; state.revision=1;
    worker=std::jthread([this](std::stop_token stop){run(stop);});
  }
  ~Impl(){worker.request_stop(); {std::lock_guard lock(mutex); enabled=false;++generation;} wake.notify_all(); if(worker.joinable())worker.join();}
  bool current(std::uint64_t version) const {std::lock_guard lock(mutex); return enabled && version==generation;}
  void publish(std::uint64_t version,bool joined,bool connecting,std::string message) {
    std::lock_guard lock(mutex); if(version!=generation)return;
    state.joined=joined;state.connecting=connecting;state.message=std::move(message);state.transmitting=false;state.talking.clear();++state.revision;
  }
  void run(std::stop_token stop) {
    while(!stop.stop_requested()) {
      std::unique_lock lock(mutex);wake.wait(lock,stop,[&]{return enabled;});
      if(stop.stop_requested())break;
      const auto version=generation; lock.unlock();
      try {
        const auto auth=authorization(); require(auth.has_value(),"Sign into ONLINE and join a party or clan first.");
        const auto url=websocket_origin(*auth);
        std::unique_ptr<CURL,decltype(&curl_easy_cleanup)> client(curl_easy_init(),curl_easy_cleanup);
        require(bool(client),"Voice networking unavailable.");
        const auto header="Authorization: Bearer "+auth->bearer;
        std::unique_ptr<curl_slist,decltype(&curl_slist_free_all)> headers(curl_slist_append(nullptr,header.c_str()),curl_slist_free_all);
        curl_easy_setopt(client.get(),CURLOPT_URL,url.c_str());curl_easy_setopt(client.get(),CURLOPT_HTTPHEADER,headers.get());
        curl_easy_setopt(client.get(),CURLOPT_CONNECT_ONLY,2L);curl_easy_setopt(client.get(),CURLOPT_PROTOCOLS_STR,"wss,ws");
        curl_easy_setopt(client.get(),CURLOPT_FOLLOWLOCATION,0L);curl_easy_setopt(client.get(),CURLOPT_PROXY,"");
        curl_easy_setopt(client.get(),CURLOPT_NOSIGNAL,1L);curl_easy_setopt(client.get(),CURLOPT_TIMEOUT_MS,2000L);
        curl_easy_setopt(client.get(),CURLOPT_CONNECTTIMEOUT_MS,1500L);
#ifdef CURLWS_NOAUTOPONG
        // Answer control frames ourselves: some connect-only libcurl versions
        // defer automatic PONG writes until application data is received/sent.
        curl_easy_setopt(client.get(),CURLOPT_WS_OPTIONS,CURLWS_NOAUTOPONG);
#endif
        curl_easy_setopt(client.get(),CURLOPT_SSL_VERIFYPEER,1L);curl_easy_setopt(client.get(),CURLOPT_SSL_VERIFYHOST,2L);
        require(curl_easy_perform(client.get())==CURLE_OK,"Voice connection failed. Try joining again.");
        if(stop.stop_requested() || !current(version))continue;
        devices->start();
        publish(version,true,false,"Voice connected - hold V to talk.");
        VoiceCodec encoder;
        struct Talker {VoiceCodec codec;std::deque<VoicePcm> frames;Clock::time_point last{};};
        std::map<std::string,Talker> talkers;
        std::vector<std::uint8_t> received;
        std::uint32_t sequence=0;
        auto next_audio=Clock::now();
        while(!stop.stop_requested() && current(version)) {
          const auto live=authorization();
          require(live && live->bearer==auth->bearer && live->clan==auth->clan && live->player==auth->player,"Voice ended. Join again after reconnecting.");
          bool transmit;std::set<std::string> silenced;
          {std::lock_guard guard(mutex);transmit=held && !state.microphone_muted;silenced=muted;}
          devices->transmit(transmit);
          for(unsigned i=0;i<64;++i) {
            std::uint8_t chunk[277];std::size_t count{};const curl_ws_frame *meta=nullptr;
            const auto result=curl_ws_recv(client.get(),chunk,sizeof(chunk),&count,&meta);
            if(result==CURLE_AGAIN)break;
            if(result!=CURLE_OK)throw std::runtime_error(std::string("Voice disconnected: ")+curl_easy_strerror(result)+". Join again to reconnect.");
            require(meta && !(meta->flags&CURLWS_CLOSE),"Voice disconnected. Join again to reconnect.");
            if(meta->flags&CURLWS_PING) {
              require(meta->offset==0 && meta->bytesleft==0 && count<=125,"Invalid voice heartbeat.");
              const auto deadline=Clock::now()+std::chrono::milliseconds(100);
              for(;;) {
                std::size_t sent{};
                const auto pong=curl_ws_send(client.get(),chunk,count,&sent,0,CURLWS_PONG);
                require(pong==CURLE_OK || pong==CURLE_AGAIN,"Voice heartbeat failed.");
                if(pong==CURLE_OK) {require(sent==count,"Incomplete voice heartbeat.");break;}
                require(Clock::now()<deadline && !stop.stop_requested() && current(version),"Voice heartbeat timed out.");
                wait_socket(client.get(),true);
              }
              continue;
            }
            if(meta->flags&CURLWS_PONG)continue;
            require((meta->flags&CURLWS_BINARY) && received.size()+count<=277 && meta->bytesleft<=curl_off_t(277-received.size()-count),"Invalid voice frame from service.");
            received.insert(received.end(),chunk,chunk+count);
            if(meta->bytesleft || (meta->flags&CURLWS_CONT))continue;
            require(received.size()>=22 && received[0]==1,"Invalid voice frame from service.");
            const auto id=uuid(std::span(received).subspan(1,16));
            if(id!=auth->player && !silenced.contains(id) && (talkers.contains(id) || talkers.size()<16)) {
              auto &talker=talkers[id];
              if(auto pcm=talker.codec.decode(std::span(received).subspan(21))) {
                if(talker.frames.size()==5)talker.frames.pop_front();
                talker.frames.push_back(*pcm);talker.last=Clock::now();
              }
            }
            received.clear();
          }
          const auto now=Clock::now();
          if(now>=next_audio) {
            next_audio=now+std::chrono::milliseconds(20);
            VoicePcm microphone{};
            if(transmit && devices->capture(microphone)) {
              const auto encoded=encoder.encode(microphone);
              std::vector<std::uint8_t> packet{1,std::uint8_t(sequence>>24),std::uint8_t(sequence>>16),std::uint8_t(sequence>>8),std::uint8_t(sequence)};
              require(sequence!=UINT32_MAX,"Rejoin voice to continue.");++sequence;
              packet.insert(packet.end(),encoded.begin(),encoded.end());
              std::size_t offset{};auto deadline=now+std::chrono::milliseconds(100);
              while(offset<packet.size() && !stop.stop_requested() && current(version)) {
                std::size_t sent{};auto result=curl_ws_send(client.get(),packet.data()+offset,packet.size()-offset,&sent,0,CURLWS_BINARY);
                require(result==CURLE_OK || result==CURLE_AGAIN,"Voice send failed.");offset+=sent;
                require(Clock::now()<deadline,"Voice connection is too slow. Join again.");
                if(offset<packet.size())wait_socket(client.get(),true);
              }
            }
            VoicePcm mix{};std::vector<std::string> talking;
            for(auto it=talkers.begin();it!=talkers.end();) {
              if(silenced.contains(it->first) || now-it->second.last>std::chrono::seconds(2)) {it=talkers.erase(it);continue;}
              auto &talker=it->second;
              if(!talker.frames.empty()) {for(unsigned j=0;j<voice_frame_samples;++j)mix[j]+=talker.frames.front()[j];talker.frames.pop_front();}
              if(now-talker.last<std::chrono::milliseconds(250))talking.push_back(it->first);
              ++it;
            }
            for(auto &v:mix)v=std::clamp(v,-1.f,1.f);
            devices->playback(mix);
            std::lock_guard guard(mutex);
            if(version==generation && (state.talking!=talking || state.transmitting!=transmit)) {state.talking=std::move(talking);state.transmitting=transmit;++state.revision;}
          }
          wait_socket(client.get(),false);
        }
      } catch(const std::exception &error) {
        publish(version,false,false,error.what());
      } catch(...) { publish(version,false,false,"Voice failed. Try joining again."); }
      devices->stop();
      std::lock_guard guard(mutex);
      if(version==generation) {enabled=false;state.joined=false;state.connecting=false;state.transmitting=false;state.talking.clear();++state.revision;}
    }
    devices->stop();
  }
};
OnlineVoice::OnlineVoice(std::shared_ptr<OnlineAccount> account,bool party):OnlineVoice([account,party]{return account?account->voice_authorization(party):std::nullopt;},std::make_unique<SdlVoiceDevices>()){}
OnlineVoice::OnlineVoice(AuthorizationSource source,std::unique_ptr<VoiceDevices> audio):impl_(std::make_unique<Impl>(std::move(source),std::move(audio))){}
OnlineVoice::~OnlineVoice()=default;
void OnlineVoice::join(){std::lock_guard lock(impl_->mutex);if(impl_->enabled)return;impl_->enabled=true;impl_->held=false;++impl_->generation;impl_->state.connecting=true;impl_->state.message="Connecting voice...";++impl_->state.revision;impl_->wake.notify_all();}
void OnlineVoice::leave(){std::lock_guard lock(impl_->mutex);if(!impl_->enabled && !impl_->state.joined)return;impl_->enabled=false;impl_->held=false;++impl_->generation;impl_->state.joined=impl_->state.connecting=impl_->state.transmitting=false;impl_->state.talking.clear();impl_->state.message="Voice is off.";++impl_->state.revision;impl_->wake.notify_all();}
void OnlineVoice::push_to_talk(bool held){std::lock_guard lock(impl_->mutex);impl_->held=held;}
void OnlineVoice::mute_microphone(bool muted){std::lock_guard lock(impl_->mutex);impl_->state.microphone_muted=muted;++impl_->state.revision;}
void OnlineVoice::mute(const std::string &player,bool value){std::lock_guard lock(impl_->mutex);if(value)impl_->muted.insert(player);else impl_->muted.erase(player);impl_->state.muted.assign(impl_->muted.begin(),impl_->muted.end());++impl_->state.revision;}
OnlineVoiceSnapshot OnlineVoice::snapshot()const{std::lock_guard lock(impl_->mutex);return impl_->state;}
} // namespace halo2
