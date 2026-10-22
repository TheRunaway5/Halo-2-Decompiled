#include "halo2/native_vista_loading_audio.hpp"
#include <SDL.h>
#include <nlohmann/json.hpp>
#include <openssl/sha.h>
#include <algorithm>
#include <atomic>
#include <bit>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <thread>

using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool condition,const std::string& message) {
  if(!condition)throw std::runtime_error(message);
}
std::vector<std::uint8_t> read(const std::filesystem::path& path) {
  std::ifstream input(path,std::ios::binary);
  check(bool(input),"Cannot read "+path.string());
  return {std::istreambuf_iterator<char>(input),{}};
}
template<class F> void eventually(F condition,const std::string& message) {
  const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(3);
  do {
    if(condition())return;
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }while(std::chrono::steady_clock::now()<end);
  throw std::runtime_error(message);
}
bool all_playing(const NativeVistaLoadingAudioSnapshot& state) {
  return state.device_open && state.worker_running &&
      std::all_of(state.playing.begin(),state.playing.end(),[](bool value){return value;});
}
void released(const NativeVistaLoadingAudioSnapshot& state) {
  check(!state.device_open && !state.worker_running && !state.source.worker &&
      !state.source.device && state.source.buffers==std::array<std::uint32_t,4>{},
      "Native loading resources survived source shutdown");
  check(state.playing==std::array<bool,4>{},"Native loading streams survived source release");
  check(state.failure.empty(),"Native loading backend failure: "+state.failure);
}
// Original worker fixture commands, grouped by the actual wait boundaries.
// No host approximation of the envelope or elapsed-clock arithmetic is used.
std::vector<std::array<std::int32_t,4>> original_frames(const Json& row) {
  std::array<std::int32_t,4> volumes{-10000,-10000,-10000,-10000};
  std::vector<std::array<std::int32_t,4>> result;
  for(const auto& event:row.at("events")) {
    if(event[0]=="SetVolume") {
      const auto buffer=event[1].get<std::uint32_t>();
      check(buffer>=25233408 && buffer<=25233456 && (buffer-25233408)%16==0,"Original volume token");
      volumes[(buffer-25233408)/16]=std::bit_cast<std::int32_t>(event[2].get<std::uint32_t>());
    }else if(event[0]=="WaitForSingleObject")result.push_back(volumes);
  }
  return result;
}
NativeVistaLoadingAudioConfiguration manual(std::atomic<std::uint32_t>& clock,
    std::atomic<unsigned>& queries) {
  NativeVistaLoadingAudioConfiguration configuration;configuration.realtime=false;
  configuration.clock=[&]{++queries;return clock.load();};
  return configuration;
}
void finite_stereo(const std::vector<float>& pcm) {
  bool audible=false;
  for(std::size_t i=0;i<pcm.size();i+=2) {
    check(std::isfinite(pcm[i]) && std::abs(pcm[i])<=1.f && pcm[i]==pcm[i+1],
        "Native output is not finite clipped mono duplicated to stereo");
    audible|=pcm[i]!=0;
  }
  check(audible,"Native loading output is silent");
}
}
int main(int argc,char** argv) {try {
  check(argc==4,"Supply original controls, encoded bank, and decoded PCM or sha256:digest");
  std::ifstream fixture_file(argv[1]);const auto fixture=Json::parse(fixture_file);
  check(fixture.at("source_sha256")=="33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109",
      "Wrong original Vista executable");
  const auto encoded=read(argv[2]),decoded=vista_loading_audio_decode(encoded);
  check(decoded.size()==65408,"Wrong original decoded tone size");
  const std::string expected_pcm=argv[3];
  if(expected_pcm.starts_with("sha256:")) {
    std::array<unsigned char,SHA256_DIGEST_LENGTH> digest{};
    SHA256(decoded.data(),decoded.size(),digest.data());
    std::ostringstream hex;hex<<std::hex<<std::setfill('0');
    for(auto byte:digest)hex<<std::setw(2)<<unsigned(byte);
    check(expected_pcm=="sha256:"+hex.str(),"Bundled tone differs from original instruction-decoded PCM");
  }else check(decoded==read(argv[3]),"Native adapter bank differs from original instruction-decoded PCM");
  check(SDL_setenv("SDL_AUDIODRIVER","dummy",1)==0,"Cannot select SDL dummy device");
  unsigned worker_cases=0,gain_cases=0;
  for(const auto& row:fixture.at("worker_cases")) {
    const auto clocks=row.at("supplied").at("clocks").get<std::vector<std::uint32_t>>();
    const auto expected=original_frames(row);check(clocks.size()==expected.size(),"Original wait/frame count");
    std::atomic<std::uint32_t> clock{clocks.front()};std::atomic<unsigned> queries{};
    NativeVistaLoadingAudio audio(encoded,manual(clock,queries));audio.begin(false);
    eventually([&]{return all_playing(audio.snapshot()) && queries.load()>0;},"Loading worker did not start");
    auto current=audio.snapshot();
    check(current.frequencies==vista_loading_audio_frequencies,"Native SetFrequency controls differ");
    check(current.volumes==expected.front() && current.source.cached_volumes==expected.front(),
        "Initial native SetVolume controls differ from original worker");
    for(std::size_t i=1;i<clocks.size();++i) {
      const auto previous_queries=queries.load();clock=clocks[i];
      eventually([&]{const auto state=audio.snapshot();return queries.load()>previous_queries &&
          state.volumes==expected[i] && state.source.cached_volumes==expected[i];},
          "Native worker clock/envelope controls differ from original, case "+std::to_string(worker_cases));
    }
    finite_stereo(audio.render(2048));const auto before_gain=audio.snapshot().submitted_source_bytes;
    for(const auto& gain:fixture.at("gain_cases")) {
      audio.gain(gain.at("gain_bits"));
      check(audio.snapshot().source.attenuation==gain.at("attenuation"),"Native gain word changed");
      ++gain_cases;
    }
    check(audio.snapshot().submitted_source_bytes==before_gain,"Gain rewound or resubmitted a source loop");
    audio.end();const auto stopped=audio.snapshot();released(stopped);
    check(stopped.source.event!=0,"Original auto-reset event was not retained");
    audio.end();check(audio.snapshot().source==stopped.source,"Repeated shutdown mutated retained owner");
    audio.begin(true);released(audio.snapshot());
    check(audio.snapshot().source.event==stopped.source.event,"Muted admission discarded retained event");
    audio.begin(false,false);released(audio.snapshot());
    audio.begin(false);eventually([&]{return all_playing(audio.snapshot());},"Retained event restart failed");
    check(audio.snapshot().source.event==stopped.source.event,"Restart replaced retained source event");
    audio.end();released(audio.snapshot());++worker_cases;
  }
  // Cross multiple periods of every source voice. The continuous SDL stream
  // must produce the same output regardless of callback/render partitioning.
  constexpr std::size_t frames=48000*7+137;
  std::vector<float> whole,partitioned;
  for(bool split:{false,true}) {
    std::atomic<std::uint32_t> clock{};std::atomic<unsigned> queries{};
    NativeVistaLoadingAudio audio(std::filesystem::path(argv[2]),manual(clock,queries));
    audio.begin(false);eventually([&]{return all_playing(audio.snapshot()) && queries.load()>0;},"Loop worker startup");
    audio.begin(false);check(audio.snapshot().mixed_output_frames==0,"Duplicate begin consumed/restarted samples");
    if(!split)whole=audio.render(frames);
    else {
      const std::array<std::size_t,7> sizes{1,37,511,512,1009,4093,32000};
      std::size_t count=0,index=0;
      while(count<frames) {
        const auto next=std::min(sizes[index++%sizes.size()],frames-count);
        const auto block=audio.render(next);partitioned.insert(partitioned.end(),block.begin(),block.end());count+=next;
      }
    }
    const auto state=audio.snapshot();
    check(state.failure.empty(),"Continuous native stream failed: "+state.failure);
    check(state.mixed_output_frames==frames,"Native output clock lost frames");
    for(auto bytes:state.submitted_source_bytes)
      check(bytes>=3*65408 && bytes%65408==0,"Native loop was rounded or source bank was only played once");
    audio.end();released(audio.snapshot());
  }
  finite_stereo(whole);check(whole==partitioned,"Native stream changed at callback/loop partition boundaries");
  {
    std::atomic<std::uint32_t> clock{};std::atomic<unsigned> queries{};
    NativeVistaLoadingAudio audio(encoded,manual(clock,queries));audio.begin(false);
    eventually([&]{return all_playing(audio.snapshot()) && queries.load()>0;},"Gain worker startup");
    const auto prefix=audio.render(3000);
    check(std::equal(prefix.begin(),prefix.end(),whole.begin()),"Native source cursor started at a different sample");
    const auto submitted=audio.snapshot().submitted_source_bytes;
    std::array<std::int32_t,4> expected{};bool found=false;
    for(const auto& row:fixture.at("envelope_cases"))if(row.at("seconds_bits")==0 &&
        row.at("attenuation")==-2500 && row.at("crt_pow_selector")==0 && row.at("crt_convert_selector")==0) {
      expected=row.at("cached_volumes").get<decltype(expected)>();found=true;break;
    }
    check(found,"Original half-gain control fixture missing");
    audio.gain(0x3f000000);
    eventually([&]{return audio.snapshot().volumes==expected;},"Native gain did not reach per-voice original SetVolume controls");
    check(audio.snapshot().submitted_source_bytes==submitted,"Native SetVolume reset source stream/cursor");
    const auto after_gain=audio.render(1000);const auto ratio=std::pow(10.,-2500./2000.);
    for(std::size_t i=0;i<after_gain.size();++i)
      check(std::abs(double(after_gain[i])-double(whole[6000+i])*ratio)<1.e-8,
          "Native gain conversion changed sample position or decibel attenuation");
    audio.end();released(audio.snapshot());
  }
  // Actual SDL dummy callbacks plus concurrent gain/shutdown and active-owner
  // destruction exercise device/worker lifetimes, not physical speaker output.
  {
    NativeVistaLoadingAudio audio(encoded);audio.begin(false);
    eventually([&]{const auto state=audio.snapshot();return all_playing(state) && state.mixed_output_frames>=1024;},
        "Realtime SDL callback did not consume loading voices");
    std::atomic<bool> finish{};
    std::thread gains([&]{while(!finish.load()){audio.gain(0x3f800000);audio.gain(0x3f000000);std::this_thread::yield();}});
    audio.end();finish=true;gains.join();released(audio.snapshot());
  }
  {
    NativeVistaLoadingAudio audio(encoded);audio.begin(false);
    eventually([&]{return all_playing(audio.snapshot());},"Active destructor startup");
  }
  {
    check(SDL_setenv("SDL_AUDIODRIVER","halo2-nonexistent-validation-driver",1)==0,"Cannot select failing driver");
    NativeVistaLoadingAudio audio(encoded);audio.begin(false);const auto state=audio.snapshot();
    check(!state.device_open && !state.worker_running && !state.source.device && !state.source.worker &&
        state.source.buffers==std::array<std::uint32_t,4>{} && !state.failure.empty(),
        "Actual native device failure was hidden or leaked source resources");
    audio.end();check(SDL_setenv("SDL_AUDIODRIVER","dummy",1)==0,"Cannot restore dummy driver");
  }
  {
    // A real open failure after successful SDL initialization must not acquire
    // another subsystem reference on an accidental retry. The disk backend's
    // missing parent path creates that specific failure without source mocks.
    const auto missing=std::filesystem::temp_directory_path()/"halo2-loading-audio-no-such-directory"/"output.raw";
    check(!std::filesystem::exists(missing.parent_path()),"Open-failure fixture directory unexpectedly exists");
    check(SDL_setenv("SDL_AUDIODRIVER","disk",1)==0 && SDL_setenv("SDL_DISKAUDIOFILE",missing.c_str(),1)==0,
        "Cannot select disk open-failure fixture");
    NativeVistaLoadingAudio audio(encoded);audio.begin(false);
    check(!audio.snapshot().device_open && !audio.snapshot().failure.empty() && (SDL_WasInit(SDL_INIT_AUDIO)&SDL_INIT_AUDIO),
        "SDL open-failure fixture did not reach initialized partial owner");
    bool rejected=false;try{audio.begin(false);}catch(const std::logic_error&){rejected=true;}
    check(rejected,"Partial failed-open owner admitted a second begin");
    audio.end();check(!(SDL_WasInit(SDL_INIT_AUDIO)&SDL_INIT_AUDIO),"Failed-open shutdown leaked SDL subsystem reference");
    check(SDL_setenv("SDL_AUDIODRIVER","dummy",1)==0,"Cannot restore dummy driver after open failure");
    audio.begin(false);eventually([&]{return all_playing(audio.snapshot());},"Failed-open owner did not restart after end");
    audio.end();released(audio.snapshot());
  }
  {
    auto invalid_bank=encoded;invalid_bank[2]=89;
    NativeVistaLoadingAudio audio(invalid_bank);bool rejected=false;
    try{audio.begin(false);}catch(const std::runtime_error&){rejected=true;}
    check(rejected && audio.snapshot().device_open && !audio.snapshot().source.worker,
        "Invalid source block did not expose partial setup failure");
    rejected=false;try{audio.begin(false);}catch(const std::logic_error&){rejected=true;}
    check(rejected,"Partial decoder failure admitted a second begin");
    audio.end();released(audio.snapshot());
  }
  {
    NativeVistaLoadingAudioConfiguration config;
    config.clock=[]()->std::uint32_t{throw std::runtime_error("native-clock-service-validation-failure");};
    NativeVistaLoadingAudio audio(encoded,config);audio.begin(false);
    eventually([&]{const auto state=audio.snapshot();return !state.worker_running &&
        state.failure=="native-clock-service-validation-failure";},"Worker exception escaped or disappeared");
    // Retained failure is diagnostic; cleanup must still join the terminated
    // worker and stop every playing native stream before destroying the device.
    audio.end();const auto state=audio.snapshot();
    check(!state.device_open && !state.source.worker && !state.worker_running && state.playing==std::array<bool,4>{},
        "Worker exception prevented shutdown");
  }
  NativeVistaLoadingAudioConfiguration invalid;invalid.callback_frames=513;
  bool rejected=false;try {NativeVistaLoadingAudio audio(encoded,invalid);}catch(const std::invalid_argument&){rejected=true;}
  check(rejected,"Invalid SDL callback extent was accepted");
  rejected=false;try {NativeVistaLoadingAudio audio(std::span<const std::uint8_t>{});}catch(const std::invalid_argument&){rejected=true;}
  check(rejected,"Missing original source bank was accepted");
  SDL_version version;SDL_GetVersion(&version);
  std::cout<<"PASS native loading audio original-worker-cases="<<worker_cases<<" gain-forwarding="<<gain_cases
      <<" continuous-output-frames="<<frames<<" SDL="<<unsigned(version.major)<<'.'<<unsigned(version.minor)<<'.'<<unsigned(version.patch)
      <<" dummy-callback teardown\n";return 0;
}catch(const std::exception& error){std::cerr<<"FAIL "<<error.what()<<'\n';return 1;}}
