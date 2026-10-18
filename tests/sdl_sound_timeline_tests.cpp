#include "halo2/sdl_sound_platform.hpp"

#include <SDL.h>
#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <thread>

namespace {
using namespace halo2;
void check(bool condition, const char *message) {
  if (!condition) throw std::runtime_error(message);
}
SdlSoundVoiceParameters parameters(float rate=1, bool paused=false) {
  return {std::bit_cast<std::uint32_t>(rate),0,paused,{{0,0,1},{1,1,1}}};
}
SdlSoundPlatformConfiguration configuration(
    SdlSoundPlatformConfiguration::Delivery delivery=SdlSoundPlatformConfiguration::Delivery::Manual) {
  return {delivery,256,std::nullopt};
}
auto pcm(std::initializer_list<float> samples) {
  return std::make_shared<const std::vector<float>>(samples);
}
void close(SdlSoundPlatform &device,SdlSoundPlatform::Voice voice) {
  device.flush(voice);
  while (auto completion=device.completion(voice))
    device.acknowledge(voice,completion->submission);
  device.destroy_voice(voice);
}

struct Cue {std::uint64_t frame;std::uint32_t sound;};
std::vector<Cue> original_cues(const char *path) {
  std::ifstream input(path);
  std::vector<Cue> cues;
  unsigned script,tick;std::uint32_t sound;
  while (input>>std::dec>>script>>tick>>std::hex>>sound)
    cues.push_back({std::uint64_t(tick)*1600,sound});
  check(cues.size()==49,"Original opening fixture does not contain all 49 cues");
  return cues;
}

// This exercises the platform deadline contract using original wake-relative
// script delays. PCM is deliberately tiny and includes authored silence. It
// does not simulate the cinematic request producer or an Xbox audio driver.
void authored_deadlines(const std::vector<Cue> &cues,std::size_t chunk) {
  SdlSoundPlatform device(configuration());
  const auto data=pcm({0,0,.003f,-.002f,.001f,.002f});
  std::vector<SdlSoundPlatform::Voice> voices;
  std::map<std::uint64_t,std::array<float,2>> expected;
  std::uint64_t end=0;
  for (const auto &cue:cues) {
#ifdef HALO2_TIMELINE_BASELINE
    const auto voice=device.create_voice(cue.sound,parameters());
#else
    const auto voice=device.create_voice_at(cue.sound,parameters(),cue.frame);
#endif
    voices.push_back(voice);
    check(device.submit(voice,{data,0,3,cue.sound})==SdlSoundPlatform::Submission::Accepted,
          "Scheduled native packet was not accepted");
    for (unsigned i=0;i<3;++i) {
      auto &sample=expected[cue.frame+i];
      sample[0]+=(*data)[i*2];sample[1]+=(*data)[i*2+1];
    }
    end=std::max(end,cue.frame+4);
  }
  for (std::uint64_t begin=0;begin<end;) {
    const auto count=std::min<std::uint64_t>(chunk,end-begin);
    const auto output=device.render(count);
    for (std::size_t i=0;i<count;++i) {
      const auto found=expected.find(begin+i);
      const auto sample=found==expected.end()?std::array<float,2>{}:found->second;
      check(std::abs(output[i*2]-sample[0])<1e-7f &&
            std::abs(output[i*2+1]-sample[1])<1e-7f,
            "PCM started outside its original script sample deadline");
    }
    begin+=count;
  }
#ifndef HALO2_TIMELINE_BASELINE
  std::map<std::uint32_t,std::uint64_t> starts;
  for (std::size_t i=0;i<cues.size();++i) {
    const auto position=device.playback_position(voices[i]);
    check(position.first_device_frame==cues[i].frame &&
          position.first_presentation_frame==cues[i].frame &&
          position.output_frames==3 && position.source_frames==3,
          "Observed native consumption disagrees with the scheduled zero-valued first sample");
    starts.emplace(cues[i].sound,*position.first_presentation_frame);
  }
  if (starts.contains(0xea13089eu) && starts.contains(0xea12089du))
    check(starts.at(0xea13089eu)-starts.at(0xea12089du)==337600,
          "Native consumed Prophet sample gap differs from 211 original Xbox ticks");
  check(device.presentation_frames()==end,"Native presentation clock lost rendered samples");
#endif
  for (auto voice:voices)close(device,voice);
}

#ifndef HALO2_TIMELINE_BASELINE
void pause_and_cancel() {
  SdlSoundPlatform device(configuration());
  const auto data=pcm({0,0,.25f,.5f,.5f,.25f});
  const auto voice=device.create_voice_at(1,parameters(),5);
  device.submit(voice,{data,0,3,1});
  device.render(2);
  device.paused(true);
  check(device.render(19)==std::vector<float>(38,0),"Global pause did not render silence");
  check(device.rendered_frames()==21 && device.presentation_frames()==2 &&
        device.status(voice).paused && !device.playback_position(voice).first_device_frame,
        "Global pause moved a pending deadline or consumed PCM");
  device.paused(false);
  check(!device.status(voice).paused,"Global resume left an active voice paused");
  check(device.render(4)==std::vector<float>(8,0),"Scheduled leading silence differs");
  auto position=device.playback_position(voice);
  check(position.first_device_frame==24 && position.first_presentation_frame==5 &&
        position.source_frames==1 && position.output_frames==1,
        "Paused deadline did not resume at its exact presentation sample");
  device.paused(true);device.render(11);device.paused(false);
  check(device.playback_position(voice).source_frames==1,"Pause consumed an active source sample");
  const auto output=device.render(2);
  check(output==std::vector<float>({.25f,.5f,.5f,.25f}),"Resume skipped or repeated source PCM");
  close(device,voice);

  const auto first=device.create_voice_at(2,parameters(1,true),device.presentation_frames()+2);
  const auto second=device.create_voice_at(3,parameters(),device.presentation_frames()+2);
  device.submit(first,{data,0,3,2});device.submit(second,{data,0,3,3});
  const auto second_due=device.presentation_frames()+2;
  device.render(8);
  check(!device.playback_position(first).first_device_frame &&
        device.playback_position(second).first_presentation_frame==second_due,
        "Per-voice pause moved an independent voice's deadline");
  device.parameters(first,parameters());
  const auto late=device.presentation_frames();device.render(1);
  check(device.playback_position(first).first_presentation_frame==late &&
        device.playback_position(first).source_frames==1,
        "Late delivery discarded source PCM to conceal lateness");
  close(device,first);close(device,second);

  const auto canceled=device.create_voice_at(4,parameters(),device.presentation_frames()+10);
  device.submit(canceled,{data,0,3,4});device.flush(canceled);
  check(device.completion(canceled)->status==SdlSoundCompletionStatus::Flushed,
        "Cancel before deadline lost the native flushed completion");
  check(device.render(30)==std::vector<float>(60,0) &&
        !device.playback_position(canceled).first_device_frame,
        "Canceled future voice reached output");
  close(device,canceled);
}

void fractional_progress() {
  for (const auto rate:{.25f,.5f,.75f,1.f,1.5f,2.f,3.f,16.f}) {
    SdlSoundPlatform device(configuration());
    const auto data=pcm({.1f,.1f,.2f,.2f,.3f,.3f,.4f,.4f,.5f,.5f,.6f,.6f,.7f,.7f});
    const auto voice=device.create_voice_at(0,parameters(rate),3);
    device.submit(voice,{data,0,3,1});device.submit(voice,{data,3,4,2});
    device.render(4);
    check(device.playback_position(voice).source_frames==std::min(7.0,double(rate)),
          "First consumed source interval differs from the native relative rate");
    device.render(std::size_t(std::ceil(7.0/rate)));
    const auto position=device.playback_position(voice);
    check(position.source_frames==7 && position.output_frames==std::uint64_t(std::ceil(7.0/rate)),
          "Source progress crossed the retained PCM extent or counted an underrun");
    close(device,voice);
  }
}

void actual_callback_deadline() {
  SdlSoundPlatform device(configuration(SdlSoundPlatformConfiguration::Delivery::DeviceCallback));
  device.paused(true);
  const auto due=device.presentation_frames()+4800;
  const auto voice=device.create_voice_at(0,parameters(),due);
  const auto data=pcm({0,0,.25f,.5f});
  device.submit(voice,{data,0,2,1});
  device.paused(false);
  const auto timeout=std::chrono::steady_clock::now()+std::chrono::seconds(2);
  while (!device.completion(voice) && std::chrono::steady_clock::now()<timeout)
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  const auto position=device.playback_position(voice);
  check(device.completion(voice) && position.first_presentation_frame==due &&
        position.source_frames==2 && position.output_frames==2,
        "Real SDL callback did not consume the scheduled first PCM sample exactly");
  close(device,voice);
}
#endif
}

int main(int argc,char **argv)try {
  if (argc!=1 && argc!=2)throw std::runtime_error("Optional input: original opening dialogue timing TSV");
  if (argc==2 && !std::filesystem::is_regular_file(argv[1]))return 77;
  SDL_setenv("SDL_AUDIODRIVER","dummy",1);
  const auto cues=argc==2?original_cues(argv[1]):std::vector<Cue>{{3,1},{17,2}};
  for (auto chunk:{127u,1024u,4096u})authored_deadlines(cues,chunk);
#ifndef HALO2_TIMELINE_BASELINE
  pause_and_cancel();fractional_progress();actual_callback_deadline();
#endif
  std::cout<<"PASS "<<cues.size()<<" native sample deadlines; "
             "render partition independence, actual callback, pause, cancellation and source progress\n";
  std::cout<<"LIMIT explicit native deadlines only; cinematic producer, Xbox driver/DSP and physical audibility unverified\n";
}catch (const std::exception &error) {std::cerr<<error.what()<<'\n';return 1;}
