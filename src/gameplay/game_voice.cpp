#include "halo2/game_voice.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <deque>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
namespace halo2 {
bool game_voice_active(bool valid,bool current,bool previous,std::optional<float> age) {
  return valid && (current || !previous || (age && std::isfinite(*age) && *age>=0 && *age<=1));
}
std::optional<GameVoiceGains> game_voice_route(unsigned sender,unsigned listener,
    const std::array<GameVoicePlayer,16> &players,bool radio,bool team,bool juggernaut,float range) {
  if(sender>=16 || listener>=16 || sender==listener)return {};
  const auto &a=players[sender],&b=players[listener];
  if(!a.connected || !b.connected || a.disabled || b.disabled || (a.muted&(1u<<listener)) || (b.muted&(1u<<sender)))return {};
  const bool audience=juggernaut ? !a.juggernaut && !b.juggernaut : !team || (a.team>=0 && b.team>=0 && a.team==b.team);
  if(!a.active) return !b.active && audience ? std::optional(GameVoiceGains{1,1}) : std::nullopt;
  if(radio && audience)return GameVoiceGains{1,1};
  if(!(range>0) || !std::isfinite(range))range=8;
  const auto delta=a.position-b.position;
  const float distance=length(delta);
  if(!std::isfinite(distance) || distance>range+1.5f)return {};
  float volume=-80*distance/(range+1.5f);
  if(distance>range)volume+=(-80/range)*(distance-range)*150;
  // Explicit native stereo adapter. The recovered DirectSound distance term
  // is retained; front/rear/center hardware mix bins are not Xbox emulated.
  const Vec3 direction=distance>0 ? Vec3{(std::cos(b.yaw)*delta.x+std::sin(b.yaw)*delta.y)/distance,
      (-std::sin(b.yaw)*delta.x+std::cos(b.yaw)*delta.y)/distance,delta.z/distance} : Vec3{};
  constexpr float q=.7071067690849304f;
  constexpr std::array<Vec3,5> speakers{{{q,q,0},{q,-q,0},{-q,q,0},{-q,-q,0},{q,0,0}}};
  std::array<float,5> gains{};
  for(unsigned i=0;i<5;++i){const auto d=(direction-speakers[i])*.5f;gains[i]=1-std::clamp(dot(d,d),0.f,1.f);}
  const auto omit=unsigned(std::min_element(gains.begin(),gains.end())-gains.begin());
  const auto amplitude=[&](float directional) {return std::pow(10.f,std::clamp(int(volume+(1-directional)*-1000),-10000,0)/2000.f);};
  std::array<float,5> bins{};for(unsigned i=0;i<5;++i)if(i!=omit)bins[i]=amplitude(gains[i]);
  // Front/rear channels on each side plus a center contribution. This bounded
  // stereo downmix is explicitly native; original hardware has five speakers.
  return GameVoiceGains{(bins[0]+bins[2]+bins[4]*q)/(2+q),(bins[1]+bins[3]+bins[4]*q)/(2+q)};
}
bool GameVoiceRadio::step(double now,bool pressed,bool speech,bool active) {
  if(!pressed)released_=true;
  if(!active) {armed_=false;released_=!pressed;}
  if(active && pressed && !previous_ && released_) {armed_=true;released_=false;start_=last_=now;}
  if(armed_ && (pressed || speech))last_=now;
  if(armed_ && (now-start_>=10 || now-last_>=.75 || !active)) {armed_=false;released_=!pressed;}
  previous_=pressed;
  return armed_;
}
std::optional<unsigned> GameVoiceSpeaker::update(const std::array<bool,16>& talking,unsigned local) {
  std::optional<unsigned> selected;
  for(unsigned i=0;i<16;++i) {
    counters_[i]=i!=local && talking[i] ? std::min(counters_[i]+1,65535u) : 0;
    if(counters_[i]>=4 && (!selected || counters_[i]<counters_[*selected]))selected=i;
  }
  return selected;
}
struct GameVoice::Impl {
  std::unique_ptr<VoiceDevices> devices;
  mutable std::mutex mutex;
  GameVoiceSnapshot state;
  bool focused=true,pressed{},active=true,closed{};
  std::uint64_t generation{},applied_generation{};
  bool suspended{},stopped{};
  std::condition_variable_any condition;
  std::deque<GameVoiceFrame> sent,received;
  std::jthread worker;
  Impl(std::unique_ptr<VoiceDevices> d):devices(std::move(d)),worker([this](std::stop_token stop){run(stop);}){}
  ~Impl(){worker.request_stop();condition.notify_all();if(worker.joinable())worker.join();}
  void run(std::stop_token stop) {
    using Clock=std::chrono::steady_clock;
    bool started=false;
    auto encoder=std::make_unique<VoiceCodec>(true);
    struct Talker {VoiceCodec decoder;std::deque<GameVoiceFrame> frames;unsigned sequence{};bool seen{};Clock::time_point last{};};
    std::map<unsigned,Talker> talkers;
    GameVoiceRadio radio;
    unsigned sequence{};
    while(!stop.stop_requested()) {
      const auto next=Clock::now()+std::chrono::milliseconds(20);
      GameVoiceSnapshot selected;bool focus,held,eligible,paused;std::uint64_t stamp,applied;std::deque<GameVoiceFrame> input;
      {std::lock_guard lock(mutex);selected=state;focus=focused;held=pressed;eligible=active;paused=suspended;
        stamp=generation;applied=applied_generation;input.swap(received);}
      try {
        if(stamp!=applied){
          // All device work stays on this worker. An old in-flight capture,
          // decode or playback completes before this retirement acknowledgment.
          if(started)devices->transmit(false);
          devices->discard();talkers.clear();input.clear();encoder=std::make_unique<VoiceCodec>(true);
          radio=GameVoiceRadio{};
          (void)radio.step(std::chrono::duration<double>(Clock::now().time_since_epoch()).count(),held,false,false);
          std::lock_guard lock(mutex);
          if(generation==stamp){applied_generation=stamp;state.talking={};state.transmitting=state.radio=false;}
          continue;
        }
        if(selected.enabled && !started){devices->start();started=true;}
        if(!selected.enabled && started){devices->stop();started=false;talkers.clear();radio=GameVoiceRadio{};}
        bool speech=false,armed=false;
        if(started && !paused) {
          devices->transmit(focus && !selected.microphone_muted);
          VoicePcm microphone{};
          if(devices->capture(microphone)) {
            const auto opus=encoder->encode(microphone);speech=encoder->speech();
            armed=radio.step(std::chrono::duration<double>(Clock::now().time_since_epoch()).count(),held,speech,eligible && focus);
            if(speech && focus && !selected.microphone_muted) {
              std::lock_guard lock(mutex);
              if(state.enabled && focused && !state.microphone_muted && generation==stamp && !suspended) {
                if(sent.size()==5)sent.pop_front();
                sent.push_back({0,++sequence,armed,{1,1},opus,stamp});
              }
            }
          }else armed=radio.step(std::chrono::duration<double>(Clock::now().time_since_epoch()).count(),held,false,eligible && focus);
          for(auto &frame:input)if(frame.delivery_generation==stamp&&frame.source<16 && !((selected.muted|selected.blocked)&(1u<<frame.source))) {
            auto &talker=talkers[frame.source];
            if(talker.seen && frame.sequence<=talker.sequence)continue;
            talker.seen=true;talker.sequence=frame.sequence;
            if(talker.frames.size()==5)talker.frames.pop_front();
            talker.frames.push_back(std::move(frame));
          }
          VoiceStereo mix{};std::array<bool,16> talking{};
          for(auto &[id,talker]:talkers) {
            if((selected.muted|selected.blocked)&(1u<<id)){talker.frames.clear();continue;}
            if(!talker.frames.empty()) {
              const auto frame=std::move(talker.frames.front());talker.frames.pop_front();
              if(auto pcm=talker.decoder.decode(frame.opus)) {
                for(unsigned i=0;i<voice_frame_samples;++i){mix[i*2]+=(*pcm)[i]*frame.gains.left;mix[i*2+1]+=(*pcm)[i]*frame.gains.right;}
                talker.last=Clock::now();
              }
            }
            talking[id]=Clock::now()-talker.last<std::chrono::milliseconds(250);
          }
          for(auto &v:mix)v=std::clamp(v,-1.f,1.f);
          // A fence can race the device call. Its acknowledgment is emitted
          // only on the next worker retirement, after this call and discard().
          bool deliver;{std::lock_guard lock(mutex);deliver=generation==stamp&&!suspended;}
          if(deliver)devices->playback_stereo(mix);
          std::lock_guard lock(mutex);if(generation==stamp&&!suspended)state.talking=talking;
        }
        {std::lock_guard lock(mutex);if(generation==stamp&&!suspended){state.transmitting=selected.enabled && focus && !selected.microphone_muted && speech;state.radio=armed;}}
      }catch(const std::exception &e) {devices->stop();started=false;talkers.clear();std::lock_guard lock(mutex);
        state.enabled=false;state.error=e.what();state.talking={};state.transmitting=state.radio=false;
        // A closed device has no queued predecessor media. Voice failure must
        // not keep an otherwise valid native takeover waiting for a microphone.
        applied_generation=generation;}
      std::unique_lock lock(mutex);
      condition.wait_until(lock,stop,next,[&]{return generation!=stamp||suspended!=paused||state.enabled!=selected.enabled;});
    }
    devices->stop();
    {std::lock_guard lock(mutex);stopped=true;}
  }
};
GameVoice::GameVoice(std::unique_ptr<VoiceDevices> d):impl_(std::make_unique<Impl>(std::move(d))){}
GameVoice::~GameVoice()=default;
void GameVoice::enable(bool v){std::lock_guard lock(impl_->mutex);impl_->state.enabled=v && !impl_->closed;impl_->state.error.clear();impl_->state.talking={};impl_->state.transmitting=false;impl_->sent.clear();impl_->received.clear();impl_->condition.notify_all();}
void GameVoice::close(){
  {std::lock_guard lock(impl_->mutex);impl_->closed=true;impl_->state.enabled=false;impl_->state.transmitting=false;impl_->state.talking={};impl_->sent.clear();impl_->received.clear();}
  impl_->worker.request_stop();impl_->condition.notify_all();if(impl_->worker.joinable())impl_->worker.join();
  std::lock_guard lock(impl_->mutex);impl_->state.enabled=false;impl_->state.transmitting=false;impl_->state.talking={};
}
void GameVoice::microphone_muted(bool v){std::lock_guard lock(impl_->mutex);impl_->state.microphone_muted=v;if(v)impl_->sent.clear();}
void GameVoice::focus(bool v){std::lock_guard lock(impl_->mutex);impl_->focused=v;if(!v)impl_->sent.clear();}
void GameVoice::radio(bool p,bool a){std::lock_guard lock(impl_->mutex);impl_->pressed=p;impl_->active=a;}
void GameVoice::mute(unsigned p,bool v){if(p>=16)return;std::lock_guard lock(impl_->mutex);if(v)impl_->state.muted|=1u<<p;else impl_->state.muted&=~(1u<<p);}
void GameVoice::blocks(std::uint16_t mask){std::lock_guard lock(impl_->mutex);impl_->state.blocked=mask;for(unsigned i=0;i<16;++i)if(mask&(1u<<i))impl_->state.talking[i]=false;}
std::uint64_t GameVoice::fence(){
  static std::atomic<std::uint64_t> next{1};auto stamp=next.load(std::memory_order_relaxed);
  do{if(!stamp||stamp==UINT64_MAX)throw std::overflow_error("Voice delivery generation exhausted");}
  while(!next.compare_exchange_weak(stamp,stamp+1,std::memory_order_relaxed));
  std::lock_guard lock(impl_->mutex);impl_->generation=stamp;impl_->suspended=true;
  impl_->sent.clear();impl_->received.clear();impl_->state.talking={};impl_->state.transmitting=impl_->state.radio=false;
  impl_->condition.notify_all();return stamp;
}
bool GameVoice::ready(std::uint64_t stamp)const{std::lock_guard lock(impl_->mutex);return stamp&&((impl_->closed&&impl_->stopped)||(impl_->generation==stamp&&impl_->applied_generation==stamp));}
bool GameVoice::resume(std::uint64_t stamp){std::lock_guard lock(impl_->mutex);
  if(!stamp||impl_->closed||impl_->generation!=stamp||impl_->applied_generation!=stamp)return false;
  if(impl_->suspended){impl_->suspended=false;impl_->condition.notify_all();}return true;
}
GameVoiceSnapshot GameVoice::snapshot()const{std::lock_guard lock(impl_->mutex);return impl_->state;}
std::vector<GameVoiceFrame> GameVoice::outgoing(){std::lock_guard lock(impl_->mutex);std::vector<GameVoiceFrame> result(impl_->sent.begin(),impl_->sent.end());impl_->sent.clear();return result;}
void GameVoice::receive(GameVoiceFrame frame){std::lock_guard lock(impl_->mutex);if(impl_->closed||impl_->suspended||frame.delivery_generation!=impl_->generation)return;if(impl_->received.size()==80)impl_->received.pop_front();impl_->received.push_back(std::move(frame));}
} // namespace halo2
