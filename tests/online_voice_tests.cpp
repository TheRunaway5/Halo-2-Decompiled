#include "halo2/online_voice.hpp"
#include <nlohmann/json.hpp>
#include <opus.h>
#include <atomic>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>
using namespace halo2;
using namespace std::chrono_literals;
namespace {
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
template<class F>void eventually(F f){const auto until=std::chrono::steady_clock::now()+4s;while(!f() && std::chrono::steady_clock::now()<until)std::this_thread::sleep_for(5ms);check(f(),"Voice transport timed out");}
struct Synthetic final:VoiceDevices {
  std::atomic<unsigned> captures{},audible{},starts{},stops{};
  bool transmitting{};unsigned phase{};
  void start()override{++starts;}
  void transmit(bool value)override{transmitting=value;}
  bool capture(VoicePcm &pcm)override{
    if(!transmitting)return false;
    for(auto &v:pcm)v=.25f*std::sin(float(phase++%48000)*6.2831853f*440.f/48000.f);
    ++captures;return true;
  }
  void playback(const VoicePcm &pcm)override{float sum=0;for(auto v:pcm)sum+=v*v;if(sum>.001f)++audible;}
  void stop()override{transmitting=false;++stops;}
  void discard()override{} // Immediate synthetic device has no queued samples.
};
}
int main(int argc,char **argv){try{
  VoiceCodec encoder,decoder;VoicePcm tone{};
  for(unsigned i=0;i<tone.size();++i)tone[i]=.25f*std::sin(float(i)*6.2831853f*440.f/48000.f);
  for(unsigned i=0;i<10;++i){auto bytes=encoder.encode(tone);check(bytes.size()<=256,"Voice packet exceeds relay bound");auto pcm=decoder.decode(bytes);check(pcm.has_value(),"Opus roundtrip failed");float sum=0;for(auto v:*pcm){check(std::isfinite(v) && std::abs(v)<=1,"Invalid decoded sample");sum+=v*v;}check(sum>1,"Speech became silence");}
  check(!decoder.decode({}) && !decoder.decode(std::vector<std::uint8_t>(257)) && !decoder.decode(std::vector<std::uint8_t>{255}),"Invalid Opus accepted");
  int error{};auto *other=opus_encoder_create(48000,1,OPUS_APPLICATION_VOIP,&error);check(other && error==OPUS_OK,"Fixture encoder failed");
  std::array<float,1920> long_pcm{};std::array<unsigned char,256> bytes{};
  const auto size=opus_encode_float(other,long_pcm.data(),1920,bytes.data(),bytes.size());opus_encoder_destroy(other);
  check(size>0 && !decoder.decode(std::span(bytes).first(size)),"Non-20ms Opus packet accepted");
  tone[0]=std::numeric_limits<float>::quiet_NaN();tone[1]=std::numeric_limits<float>::infinity();tone[2]=100;
  check(decoder.decode(encoder.encode(tone)).has_value(),"Untrusted microphone samples corrupted codec");
  if(argc>1){
    std::ifstream file(argv[1]);nlohmann::json config;file>>config;
    const auto origin=config.at("origin").get<std::string>(),clan=config.at("clan").get<std::string>();
    const auto player_a=config.at("a").at("player").get<std::string>(),player_b=config.at("b").at("player").get<std::string>();
    const auto token_a=config.at("a").at("token").get<std::string>(),token_b=config.at("b").at("token").get<std::string>();
    const bool party=config.value("party",false);
    std::atomic<bool> valid{true};
    auto da=std::make_unique<Synthetic>(),db=std::make_unique<Synthetic>();auto *a=da.get(),*b=db.get();
    OnlineVoice speaker([&]()->std::optional<VoiceAuthorization>{if(!valid)return {};return VoiceAuthorization{origin,token_a,clan,player_a,true,party};},std::move(da));
    OnlineVoice listener([&]()->std::optional<VoiceAuthorization>{return VoiceAuthorization{origin,token_b,clan,player_b,true,party};},std::move(db));
    check(a->starts==0 && b->starts==0,"Voice opened microphone without joining");
    speaker.join();listener.join();
    eventually([&]{return speaker.snapshot().joined && listener.snapshot().joined;});
    if(config.contains("idle_seconds")) {
      std::this_thread::sleep_for(std::chrono::seconds(config.at("idle_seconds").get<unsigned>()));
      if(!speaker.snapshot().joined || !listener.snapshot().joined)
        std::cerr<<"Speaker: "<<speaker.snapshot().message<<" Listener: "<<listener.snapshot().message<<'\n';
      check(speaker.snapshot().joined && listener.snapshot().joined,"Idle heartbeat disconnected clan voice");
    }
    std::this_thread::sleep_for(60ms);check(a->captures==0 && b->captures==0,"Join automatically transmitted microphone");
    speaker.push_to_talk(true);eventually([&]{return b->audible>5;});
    check(!listener.snapshot().talking.empty(),"Incoming talker status absent");
    listener.mute(player_a,true);std::this_thread::sleep_for(120ms);auto before=b->audible.load();
    std::this_thread::sleep_for(140ms);check(b->audible==before,"Muted member still audible");
    listener.mute(player_a,false);eventually([&]{return b->audible>before+3;});
    speaker.mute_microphone(true);std::this_thread::sleep_for(120ms);before=b->audible.load();
    std::this_thread::sleep_for(140ms);check(b->audible==before,"Muted microphone still transmitted");
    speaker.mute_microphone(false);eventually([&]{return b->audible>before+3;});
    valid=false;eventually([&]{return !speaker.snapshot().joined && a->stops>0;});
    listener.leave();eventually([&]{return b->stops>0;});
    check(!listener.snapshot().joined,"Leave retained voice connection");
  }
  std::cout<<"PASS 20ms Opus roundtrip/bounds/invalid durations/finite samples; when configured, real two-client authenticated relay, opt-in capture, push-to-talk, member mute, microphone mute and revoked identity.\n";
  return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
