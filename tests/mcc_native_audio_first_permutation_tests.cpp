#include "halo2/multiplayer_audio.hpp"
#include "halo2/mcc_script_sound_world_access.hpp"
#include "halo2/mcc_sound_variation_provider.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/sound_variation.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <iostream>
using namespace halo2;
namespace {
constexpr auto none=0xffffffffu;
const SdlSoundPlatformConfiguration manual{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}};
void need(bool x,const char* message){if(!x)throw FormatError(message);}
unsigned word(PhysicsLiveWorldTu15& w,unsigned p){return Bytes(w.memory().view(p,4)).u32(0);}
unsigned aux(PhysicsLiveWorldTu15& w){return word(w,word(w,0x4f3cd8u)+4);}
}
int main(int argc,char**argv)try{
 if(argc!=3||!*argv[1]||!std::filesystem::is_regular_file(argv[1]))return 77;
 std::filesystem::create_directories(argv[2]);Cache cache(argv[1]);PhysicsLiveWorldTu15 world(cache,argv[2]);
 world.initialize_map();world.configure_session(source_game_variant_default(0),1,30,0x6a19d3e1);
 auto provider=world.sound_variation_provider(cache);auto access=world.script_sound_access(cache);
 const auto rock=std::ranges::find(cache.tags(),std::string("sound\\materials\\particles\\rocks\\rocks_small"),&Tag::name);
 need(rock!=cache.tags().end(),"Missing actual multipitch FF witness");
 unsigned shared=aux(world);std::vector<MultiplayerAudioSubmission> events;
 MultiplayerAudio actual(cache,7,manual,[&](const auto& e){events.push_back(e);},&shared);
 actual.bind_native_sound_metadata(provider);auto clock=actual.make_script_sound_clock(access.clock_rows(cache));
 const auto stop=[&](const auto& voice){if(voice)(void)actual.request_script_stop(*voice);};
 for(unsigned iteration=0;iteration<2;++iteration){
  const auto old_rows=access.clock_rows(cache);
  auto pending=actual.prepare_script_impulse(clock,rock->datum,access.policy(cache),stop,access.floating_point_context(cache));
  need(bool(pending)&&pending->selection().pitch_range==0xff&&pending->selection().permutation==0xff,"Genuine213410 did not select FF");
  access.publish_clock_rows(cache,old_rows,clock->rows());shared=aux(world);
  const auto before=shared;unsigned baseline_seed=shared;std::vector<MultiplayerAudioSubmission> baseline_events;
  MultiplayerAudio baseline(cache,7,manual,[&](const auto& e){baseline_events.push_back(e);},&baseline_seed);
  baseline.bind_native_sound_metadata(provider);
  const auto baseline_voice=baseline.play_impulse(rock->datum);need(bool(baseline_voice)&&baseline_events.size()==1,"Existing Audio FF witness failed PCM publication");
  const auto expected=baseline_events.back();const auto definition=provider.load(cache,rock->handle(),expected.pitch_range);
  auto expected_state=imported_sound_range_state(definition);SoundLocalRandom selection(expected.rng_before);
  const auto selected=source_sound_permutation(definition,expected_state,selection);
  need(selected>=0&&unsigned(selected)==expected.permutation&&selection.seed==expected.rng_after,"Existing Audio differs from verified source selector");
  const auto voice=actual.play_prepared_script_impulse(*pending,access);
  need(bool(voice)&&events.size()==iteration+1,"Typed firstFF failed one actual packet publication");const auto& got=events.back();
  need(got.pitch_range==expected.pitch_range&&got.permutation==expected.permutation&&got.frames==expected.frames&&
       got.rng_before==expected.rng_before&&got.rng_after==expected.rng_after&&got.source_pitch_bits==expected.source_pitch_bits,
       "Typed firstFF selected twice or changed range/packet metadata");
  need(shared==expected.rng_after&&aux(world)==shared&&shared!=before,"Initial FF did not publish actual shared Aux");
  const auto after=provider.load(cache,rock->handle(),got.pitch_range);auto expected_range=definition.range;
  expected_range[4]=std::byte(std::uint8_t(expected_state.last));expected_range[5]=std::byte(std::uint8_t(expected_state.forced));
  need(after.range==expected_range,"Initial FF did not publish genuine shared range state");
  for(unsigned n=0;n<after.used.size();++n)need(after.used[n]==std::byte(expected_state.used>>(8*n)),"Initial FF packed-used state differs");
  need(actual.voice_parameters(*voice).volume_hundredths_db==baseline.voice_parameters(*baseline_voice).volume_hundredths_db,
       "Typed firstFF changed actual selected permutation gain");
  const auto actual_pcm=actual.render(4096),baseline_pcm=baseline.render(4096);
  need(actual_pcm==baseline_pcm,"Typed firstFF changed selected PCM output");
  need(std::ranges::any_of(actual_pcm,[](float sample){return sample!=0;}),"FirstFF PCM witness is silent");
  actual.stop();actual.update();baseline.stop();need(access.clock_rows(cache)==clock->rows(),"PCM retirement changed source metadata/sourcevoice");
  std::cout<<"PASS genuine FF iteration"<<iteration<<" range="<<got.pitch_range<<" permutation="<<got.permutation<<" shared="<<shared<<" exact4096PCMframes\n";
 }
 // Declared valid source-selector control: exact bypass of ordinary state.
 std::optional<TagHandle> forced_sound;
 for(const auto& t:cache.select_tags({.group=TagGroup{"snd!"},.require_address=true}))try{
  const auto d=provider.load(cache,t.handle());if(d.pitch_ranges.size()!=1||d.permutations.empty()||d.permutations.size()>6)continue;
  (void)access.selected_seconds(cache,t.handle(),{0,0});forced_sound=t.handle();break;
 }catch(const FormatError&){}
 need(bool(forced_sound),"No finite forced packet fixture");auto policy=access.policy(cache);policy.select=[](auto,auto& s){s={0,0};};
 const auto rows=access.clock_rows(cache);auto pending=actual.prepare_script_impulse(clock,forced_sound->value,policy,stop,access.floating_point_context(cache));
 need(bool(pending),"Forced control lacked pending admission");access.publish_clock_rows(cache,rows,clock->rows());shared=aux(world);
 const auto seed=shared;const auto initial=provider.load(cache,*forced_sound);const auto count=events.size();
 const auto voice=actual.play_prepared_script_impulse(*pending,access);need(bool(voice)&&events.size()==count+1&&events.back().permutation==0,"Valid forced pair did not bypass ordinary selection");
 const auto after=provider.load(cache,*forced_sound);need(after.range==initial.range&&after.used==initial.used&&aux(world)==seed,"Valid forced pair changed World ordinary selector state");
 SoundLocalRandom creation(seed);(void)source_sound_gain(initial,creation);(void)source_sound_pitch(initial,creation);
 need(shared==creation.seed&&events.back().rng_before==creation.seed&&events.back().rng_after==creation.seed,"Forced path altered two-draw creation/captured seed");
 actual.stop();actual.update();std::cout<<"PASS forced-valid bypass retains shared range/used; original creation two draws remain local for caller publication\n";
 std::cout<<"LIMIT initial packet PCM/shared-state compatibility only; no Main, original allocation/cull failure, later packet cadence, physical output or full gameplay parity claim\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
