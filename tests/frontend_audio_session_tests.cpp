#define SDL_MAIN_HANDLED
#include "halo2/frontend_audio.hpp"
#include "halo2/frontend_session.hpp"
#include "halo2/desktop_session.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <set>
#include <stdexcept>

using namespace halo2;
namespace {
unsigned checks{};
void check(bool value,const char* why){++checks;if(!value)throw std::runtime_error(why);}
struct SdlScope {~SdlScope(){SDL_Quit();}};
}
int main(int argc,char** argv){try{
  if(argc!=2 || !std::filesystem::is_regular_file(std::filesystem::path(argv[1])/"mainmenu.map"))return 77;
  SDL_setenv("SDL_AUDIODRIVER","dummy",1);SdlScope sdl;
  FrontendSession session;
  auto& audio=session.audio_state();
  check(&audio==&session.audio_state() && !audio.engine && !audio.music,
      "Fresh retained audio owner opened a device or changed identity");
  audio.desktop=std::make_shared<DesktopSession>();
  std::weak_ptr<DesktopSession> retained_desktop=audio.desktop;
  LoopingSound definition;
  SoundVariationDefinition start,loop;
  std::shared_ptr<PcmSound> intro;
  {
    const std::filesystem::path maps(argv[1]);Cache cache(maps/"mainmenu.map",maps);
    check(cache.is_vista(),"Music asset lifetime test requires actual PC cache");
    const auto content=load_vista_settings(maps/"mainmenu.map");
    // Actual selected kind2 -> matg interface[0]+84 -> wgtz E4D002AE+4
    // -> wigl E4D102AF+1BC/+1C0. Independent inventory and native probe:
    // analysis/frontend/vista-session-flow-audit/music-binding-20261011/.
    check(content->music==0xe53d031b && content->music_silence_ms==5500,
        "Actual selected PC music/silence binding changed");
    definition=cache.load_looping_sound(content->music);
    check(definition.flags==0 && definition.tracks.size()==1 &&
        definition.tracks[0].start==0xe53e031c && definition.tracks[0].loop==0xe53f031d &&
        definition.tracks[0].gain_db==-6 && definition.tracks[0].fade_out==5,
        "Actual PC menu phase definition changed");
    audio.music=std::make_unique<MenuMusic>(cache,content->music,0x146240);
    intro=std::make_shared<PcmSound>(decode_sound(cache.load_sound(definition.tracks[0].start)));
    start=cache.load_sound_variation(definition.tracks[0].start);
    loop=cache.load_sound_variation(definition.tracks[0].loop);
  } // Every phase remains usable after its original PC Cache is destroyed.
  check(!retained_desktop.expired(),"Audio owner lost its platform lifetime");
  const auto* music=audio.music.get();
  const auto seed=audio.music->seed();
  check(&audio==&session.audio_state() && audio.music.get()==music && audio.music->seed()==seed,
      "Repeated renderer access recreated decoded music or its RNG owner");
  const auto& bank=audio.music->loops(0);
  check(bank.size()==4 && intro->frames()==1426635,"Complete actual PC intro/four-loop bank missing");
  const std::array<std::size_t,4> expected_frames{2712834,1700816,3624543,7197815};
  for(unsigned i=0;i<bank.size();++i)
    check(bank[i]->frames()==expected_frames[i],"Actual PC serialized WMA phase was truncated");
  audio.engine=std::make_unique<AudioEngine>(false);AudioEngine reference(false);
  const auto admitted=audio.transitions.request(true,1000);
  check(admitted && *admitted,"Fresh audio owner did not admit initial native playback");
  audio.music_voices=audio.music->play(*audio.engine);
  check(audio.music_voices.size()==1,"PC track did not retain one actual native voice");
  const auto gain=std::pow(10.f,definition.tracks[0].gain_db/20.f);
  auto complete_phase=[&](const std::shared_ptr<const PcmSound>& pcm){
    reference.play(pcm,gain);auto expected=reference.render(pcm->frames());
    check(audio.engine->render(pcm->frames())==expected,
        "Retained music changed a decoded/mixed sample after its cache was destroyed");
    check(&audio==&session.audio_state() && audio.music.get()==music && audio.music_voices.size()==1,
        "Renderer/resource boundary reset actual audio or decoded bank ownership");
  };
  complete_phase(intro);
  auto state_of=[](const SoundVariationDefinition& d){const auto s=imported_sound_range_state(d);
    return LoopingSoundSelection{s.used,s.last};};
  auto start_state=state_of(start),loop_state=state_of(loop);
  std::vector<std::uint16_t> starts,loops;
  for(const auto& p:start.permutations)starts.push_back(Bytes(p).u16(2));
  for(const auto& p:loop.permutations)loops.push_back(Bytes(p).u16(2));
  auto expected_seed=0x146240u;expected_seed=expected_seed*0x19660du+0x3c6ef35fu;
  source_looping_sound_permutation(starts,start_state,expected_seed);
  std::set<unsigned> visited;
  // The current native selector is independently source-tested against Xbox.
  // This test proves complete PC payload/lifetime continuation, not a Vista
  // selector/DMO/device or global-RNG-order equivalence claim.
  for(unsigned i=0;i<5;++i){const auto selected=unsigned(source_looping_sound_permutation(loops,loop_state,expected_seed));
    complete_phase(bank.at(selected));if(i<4)visited.insert(selected);
    std::cout<<"loop="<<selected<<" frames="<<bank[selected]->frames()<<'\n';}
  check(visited.size()==4,"Retained native scheduler repeated before completing actual PC bank");
  audio.world_update(7000); // Exactly6000ms since initial request at1000.
  check(!audio.enabled && audio.transitions.playing() && audio.transitions.last_change()==1000 &&
      audio.music_voices.size()==1 && audio.engine->active_voices()==1,
      "Real world update bypassed the strict6000ms source debounce");
  audio.world_update(7001);
  check(!audio.transitions.playing() && audio.transitions.last_change()==7001 &&
      audio.music_voices.empty() && audio.engine->active_voices()==1,
      "World update did not admit stop at6001ms or destroyed authored fade");
  // Compare the native adapter's cutoff to the independently original-tested
  // 12A810 kernel with actual track gain and retained mixer frame phase. A
  // unity tone's5001ms endpoint is not an authored PC-driver cutoff proof.
  SoundPlaybackFade expected_fade{
      std::uint16_t(!(Bytes(loop.sound_class).u16(10)&0x400)),0,
      std::int32_t(definition.tracks[0].fade_out*1000.f),-1,true};
  const auto mixed_before=audio.engine->mixed_frames();
  std::size_t cutoff{};
  const auto limit=std::size_t(definition.tracks[0].fade_out*1000.f+2)*48;
  for(;cutoff<limit;++cutoff){
    const auto time=std::int32_t((mixed_before+cutoff)/48);
    const auto db=source_sound_playback_fade(expected_fade,time);
    if(db+sound_linear_to_db(gain)<=-64 && expected_fade.start>expected_fade.end)break;
  }
  check(cutoff>0 && cutoff<limit,"Source kernel did not bound real-bank fade cutoff");
  const auto fading=audio.engine->render(cutoff);
  check(std::any_of(fading.begin(),fading.end(),[](float sample){return sample!=0;}) &&
      audio.engine->active_voices()==1,"Actual PC bank retired before source-kernel cutoff");
  audio.world_update(8000);
  check(audio.engine->render(1)==std::vector<float>(2) && audio.engine->active_voices()==0,
      "Actual PC bank missed source-kernel cutoff or restarted after world update");
  AudioEngine unity(false);auto tone=std::make_shared<PcmSound>();tone->stereo.assign(128,.25f);
  const auto tone_voice=unity.play(tone,1,0,true);unity.fade_out(tone_voice,5,1);
  unity.render(48000*5);
  check(unity.active_voices()==1,"Unity fade lost source relative-time anchor before5000ms");
  unity.render(48);
  check(unity.active_voices()==1,"Unity fade lost source +1ms anchor");
  check(unity.render(48)==std::vector<float>(96) && unity.active_voices()==0,
      "Unity fade failed to retire at original5001ms endpoint");
  check(&audio==&session.audio_state() && audio.music.get()==music && !audio.transitions.playing(),
      "Retained frontend reentry recreated or restarted stopped music");
  std::cout<<"PASS "<<checks<<" actual PC audio owner checks: destroyed Cache, complete intro/four loops/repeat, strict debounce and authored fade; no live device/Vista selector equality claim\n";
  return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
