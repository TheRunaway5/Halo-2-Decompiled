#include "halo2/mcc_script_sound_world_access.hpp"
#include "halo2/multiplayer_audio.hpp"
#include "halo2/mcc_sound_variation_provider.hpp"
#include "halo2/mcc_sound_metadata_publication.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/original_data.hpp"
#include "halo2/sound_create.hpp"
#include "halo2/sound_spatial.hpp"
#include "halo2/sound_classes.hpp"
#include <bit>
#include <cmath>
#include <iostream>
#include <thread>
using namespace halo2;
namespace {
constexpr auto none=0xffffffffu;
const SdlSoundPlatformConfiguration manual{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}};
void need(bool v,const char*m){if(!v)throw FormatError(m);}
template<class F>void rejects(F&&f,std::string_view reason,const char*m){bool bad=false;try{f();}catch(const std::exception&e){bad=std::string_view(e.what()).find(reason)!=std::string_view::npos;}need(bad,m);}
struct Expected{std::int32_t volume;std::uint32_t rng,pitch;unsigned permutation;};
Expected expected(const Cache&cache,const SoundVariationDefinition&d,std::uint32_t seed,float distance){
 SoundLocalRandom r(seed);const float gain=source_sound_gain(d,r)+source_sound_control_gain(d,1);
 const float pitch=source_sound_control_pitch(d,float(source_sound_pitch(d,r)),1);const auto after=r.seed;
 const auto range=source_sound_pitch_range(d.pitch_ranges,-1,pitch);need(range==0,"Fixture changed pitch range");
 const auto center=std::bit_cast<std::int16_t>(Bytes(d.pitch_ranges[0].interval).u16(0));
 const float sample_pitch=pitch-float(center);SoundLocalRandom selection(after);auto state=imported_sound_range_state(d);
 const auto p=source_sound_permutation(d,state,selection);const auto&c=*cache.original_data()->sound_create_constants;
 const auto&driver=*cache.original_data()->sound_driver_parameter_constants;const Bytes cls(d.sound_class),play(d.playback);
 const auto flags=std::to_integer<unsigned>(d.sound[1]);const auto minimum=flags&4u?cls.f32(0x18):play.f32(0);
 const auto maximum=flags&8u?cls.f32(0x1c):play.f32(4);const float epsilon=std::bit_cast<float>(c.fade_epsilon_bits);
 const float bounded_min=minimum>epsilon?minimum:epsilon;
 const auto spatial=source_sound_distance_gain(std::bit_cast<unsigned>(bounded_min),std::bit_cast<unsigned>(maximum),std::bit_cast<unsigned>(distance),driver,{0x027f,0x1f80});
 const SoundSpatialConstants spatial_constants{c.one_bits,c.minimum_gain_bits,c.linear_db_scale_bits};
 const float spatial_db=std::bit_cast<float>(source_sound_spatial_gain_db(spatial,spatial_constants,{0x027f,0x1f80}));
 const float total=(((gain+sound_linear_to_db(1.f))+0.f)+spatial_db)+0.f;
 const float attenuation=total+float(std::bit_cast<std::int8_t>(d.permutations.at(p)[4]));
 return {source_sound_driver_volume(std::bit_cast<unsigned>(attenuation),driver,{0x027f,0x1f80}),after,std::bit_cast<unsigned>(sample_pitch),unsigned(p)};
}
struct Fixture{std::uint32_t sound;float distance;Expected native,raw;};
void same_metadata(const SoundVariationDefinition &a,const SoundVariationDefinition &b){
 need(a.datum==b.datum && a.sound==b.sound && a.playback==b.playback && a.sound_class==b.sound_class &&
     a.range==b.range && a.scale==b.scale && a.permutations==b.permutations && a.used==b.used &&
     a.sample_rate_index==b.sample_rate_index && a.pitch_ranges.size()==b.pitch_ranges.size(),
     "Immutable publication differs from genuine initialized native World metadata");
 for(unsigned n=0;n<a.pitch_ranges.size();++n)need(a.pitch_ranges[n].interval==b.pitch_ranges[n].interval &&
     a.pitch_ranges[n].permutations==b.pitch_ranges[n].permutations,"Immutable publication changed genuine World pitch intervals");
}
Fixture fixture(const Cache&cache,const MccSoundVariationProvider&p){
 for(const auto&t:cache.select_tags({.group=TagGroup{"snd!"},.require_address=true})){
  const auto raw_bytes=cache.read_tag_bytes(t.address,20);const Bytes raw(raw_bytes);if(std::to_integer<unsigned>(raw.slice(10,1)[0])!=1 || !raw.u32(16) || raw.u32(16)>10000)continue;
  SoundVariationDefinition n;
  try{n=p.load(cache,t.handle());}catch(const FormatError&e){if(std::string_view(e.what())=="Unrecovered native sound permutation count")continue;throw;}
  if(n.permutations.size()>6 || std::to_integer<unsigned>(n.sound[2])>=sound_class_count)continue;const auto original=cache.load_sound_variation(t.datum);
  for(float distance:{1.f,5.f,10.f,20.f,40.f,100.f}){
   const auto a=expected(cache,n,0x1872c4d1,distance),b=expected(cache,original,0x1872c4d1,distance);
   if(a.volume!=b.volume && a.volume>-6400 && b.volume>-6400){std::cout<<"FIXTURE "<<t.name<<" full="<<t.datum<<" distance="<<distance<<" native="<<a.volume<<" raw="<<b.volume<<'\n';return {t.datum,distance,a,b};}
  }
 }
 throw FormatError("No bounded actual native/raw spatial metadata witness");
}
}
int main(int argc,char**argv)try{
 static_assert(sizeof(MultiplayerAudio)==sizeof(void*));if(argc!=3 || !*argv[1] || !std::filesystem::is_regular_file(argv[1]))return 77;std::filesystem::create_directories(argv[2]);
 const auto cache_owner=std::make_shared<const Cache>(argv[1]);const auto &cache=*cache_owner;
 const auto publication=MccSoundMetadataPublication::create(cache_owner);
 auto world=std::make_unique<PhysicsLiveWorldTu15>(cache,argv[2]);world->initialize_map();world->configure_session(source_game_variant_default(0),1,30,0x6a19d3e1);
 auto provider=world->sound_variation_provider(cache);const auto f=fixture(cache,provider);const std::array<Vec3,1>listeners{Vec3{0,0,0}};
 same_metadata(provider.load(cache,TagHandle{f.sound}),publication.load(cache,TagHandle{f.sound}));
 unsigned lift_metadata{};
 for(const auto &tag:cache.tags())if(tag.group=="snd!" && (tag.name.find("midship_grav_lift")!=std::string::npos || tag.name.find("midship_activate_lift")!=std::string::npos)) {
  const auto first=provider.load(cache,tag.handle());
  for(unsigned pitch=0;pitch<first.pitch_ranges.size();++pitch){same_metadata(provider.load(cache,tag.handle(),pitch),publication.load(cache,tag.handle(),pitch));++lift_metadata;}
 }
 need(!lift_metadata || lift_metadata>=2,"Actual World metadata fixture omitted one of the base/activation lift sounds");
 std::cout<<"PASS immutable publication matches "<<lift_metadata<<" genuine initialized native World lift pitch definitions exactly\n";
 {
  std::uint32_t seed=0x1872c4d1;MultiplayerAudio replica(cache_owner,7,manual,{},&seed);
  replica.enroll_continuation();replica.bind_native_loop_metadata(publication);replica.listeners(listeners);
  const auto voice=replica.play_impulse_at(f.sound,{f.distance,0,0});
  need(voice && replica.voice_parameters(*voice).volume_hundredths_db==f.native.volume,"Immutable Core binding fell back to raw MCC spatial metadata");
  rejects([&]{replica.bind_native_loop_metadata(publication);},"quiescent","Active immutable Core replaced its metadata");
  replica.stop();replica.bind_native_sound_metadata(provider);
 }
 {
  std::uint32_t seed=0x1872c4d1;MultiplayerAudio raw(cache,7,manual,{},&seed);raw.listeners(listeners);const auto voice=raw.play_impulse_at(f.sound,{f.distance,0,0});need(voice.has_value(),"Raw control failed actual playback");need(raw.voice_parameters(*voice).volume_hundredths_db==f.raw.volume,"Raw baseline calculation drift");raw.stop();
 }

 {
  // Owned metadata import controls. Fixed row values exercise exact checkpoint
  // representation; actual World row capture/publication is tested separately.
  SourceScriptSoundClockRowsTu15 rows{};
  rows[1]={f.sound,0xfffffff0u,{0xff,5},0x23,0xa7};
  rows[7]={f.sound,19,{0,0},0,0x55};rows[9].start_tick_bits=47;
  std::uint32_t seed=0x1872c4d1;MultiplayerAudio restored(cache,7,manual,{},&seed);
  restored.bind_native_sound_metadata(provider);
  auto bad=rows;bad[7].sound^=0x10000u;bool rejected=false;
  try{(void)restored.make_script_sound_clock(bad);}catch(const FormatError&){rejected=true;}
  need(rejected && seed==0x1872c4d1 && restored.voices()==0,"Foreign restored sound changed Audio");
  std::thread other([&]{rejects([&]{(void)restored.make_script_sound_clock(rows);},"foreign thread","Foreign metadata clock import admitted");});other.join();
  auto imported=restored.make_script_sound_clock(rows);
  need(imported->rows()==rows && seed==0x1872c4d1 && restored.voices()==0 && restored.presentation_frames()==0,"Restored metadata normalized bytes, consumed RNG or fabricated voice");
  rejects([&]{(void)restored.make_script_sound_clock(rows);},"fresh clock epoch","Second restored clock admitted same epoch");
  restored.bind_native_sound_metadata(provider);rejects([&]{imported->rows();},"closed","Metadata rebind kept imported clock live");
  auto fresh=restored.make_script_sound_clock(rows);need(fresh->rows()==rows,"Fresh epoch lost restored metadata");
  MultiplayerAudio playing(cache,3,manual);playing.bind_native_sound_metadata(provider);const auto live=playing.play_impulse(f.sound);need(bool(live),"Live import guard fixture lacked PCM voice");
  rejects([&]{(void)playing.make_script_sound_clock(rows);},"quiescent","Live Audio imported checkpoint metadata");playing.stop();
  need(playing.make_script_sound_clock(rows)->rows()==rows,"Stopped fresh Audio could not import metadata");
  std::cout<<"PASS exact12B restored metadata/duplicates/partial selectors, foreign salt/thread, fresh epoch and real live-voice guards; no sourcevoice imported\n";
 }

 {
  // Use actual source-owned rows and selection, then the existing PCM owner.
  auto access=world->script_sound_access(cache);const auto initial=access.clock_rows(cache);
  need(initial==SourceScriptSoundClockRowsTu15{},"Actual World did not initialize clock rows");
  const auto untouched=world->authority_checkpoint();
  MultiplayerAudio actual(cache,7,manual);actual.bind_native_sound_metadata(provider);
  auto imported=actual.make_script_sound_clock(initial);
  const auto stop=[&](const auto&voice){if(voice)(void)actual.request_script_stop(*voice);};
  auto pending=actual.prepare_script_impulse(imported,f.sound,access.policy(cache),stop,access.floating_point_context(cache));
  need(bool(pending),"Genuine World policy failed actual Audio admission");
  const auto admitted=imported->rows();need(admitted!=initial,"Actual admission retained empty clock");
  access.publish_clock_rows(cache,initial,admitted);
  need(access.clock_rows(cache)==admitted,"Actual World lost Audio admission metadata");
  const auto voice=actual.play_prepared_script_impulse(*pending);need(bool(voice)&&actual.voices()==1,"Actual source selection failed real PCM publication");
  need(access.clock_rows(cache)==imported->rows(),"PCM publication changed source metadata or serialized a host voice");
  actual.stop();actual.update();need(imported->rows()==admitted&&actual.voices()==0,"PCM retirement changed source timing metadata");
  const auto saved=world->authority_checkpoint();
  {auto stage=world->prepare_authority(saved);need(world->commit_authority(std::move(stage)),"Actual metadata checkpoint restore failed");}
  rejects([&]{access.clock_rows(cache);},"stale epoch","Restore retained old World access");
  rejects([&]{actual.prepare(f.sound);},"stale memory owner","Restore retained old Audio metadata");
  provider=world->sound_variation_provider(cache);actual.bind_native_sound_metadata(provider);
  rejects([&]{imported->rows();},"closed","Actual restore rebind kept old clock live");
  auto fresh=world->script_sound_access(cache);need(fresh.clock_rows(cache)==admitted,"Authority restore failed actual metadata preservation");
  auto restored=actual.make_script_sound_clock(fresh.clock_rows(cache));
  need(restored->rows()==admitted&&actual.voices()==0,"Restored metadata imported voice or changed bytes");
  need(restored->stop_first(f.sound,stop),"Restored source row could not stop without a fabricated voice");
  fresh.publish_clock_rows(cache,admitted,restored->rows());
  need(fresh.clock_rows(cache)==restored->rows(),"Restored stop metadata failed World publication");
  {auto stage=world->prepare_authority(untouched);need(world->commit_authority(std::move(stage)),"Original World control restore failed");}
  provider=world->sound_variation_provider(cache);
  std::cout<<"PASS actual World policy/selection to real PCM, metadata CAS, authority restore/fresh Audio rebind and voice-free restored stop\n";
 }
 std::uint32_t random=0x1872c4d1;unsigned events=0,completions=0;bool observer_guard=false,completion_guard=false;
 MultiplayerAudio*active=nullptr;MultiplayerAudio audio(cache,7,manual,[&](const auto&s){++events;if(events==1){need(s.source_pitch_bits==f.native.pitch && s.permutation==f.native.permutation,"Native packet metadata/selection differ");rejects([&]{active->bind_native_sound_metadata(provider);},"quiescent","Pre-emplace observer replaced metadata");observer_guard=true;}},&random);active=&audio;
 auto old_clock=audio.make_script_sound_clock();audio.bind_native_sound_metadata(provider);rejects([&]{old_clock->rows();},"closed","Metadata binding left prior clock live");auto clock=audio.make_script_sound_clock();audio.listeners(listeners);
 const auto voice=audio.play_impulse_at(f.sound,{f.distance,0,0});
 need(voice && observer_guard && random==f.native.rng && audio.voice_parameters(*voice).volume_hundredths_db==f.native.volume,"Bound Audio failed exact native metadata witness");
 rejects([&]{audio.bind_native_sound_metadata(provider);},"quiescent","Live Audio allowed metadata rebind");need(clock->rows()[0].sound==none,"Rejected bind changed current clock");
 std::thread foreign([&]{rejects([&]{audio.bind_native_sound_metadata(provider);},"foreign thread","Foreign metadata binder admitted");});foreign.join();audio.stop();
 const auto callback_voice=audio.play_impulse(f.sound,1,MultiplayerAudioKind::Impulse,none,0,[&](const auto&){++completions;rejects([&]{audio.bind_native_sound_metadata(provider);},"quiescent","Completion dispatch replaced metadata");completion_guard=true;});need(bool(callback_voice),"Actual completion fixture voice unavailable");audio.stop();need(completions==1&&completion_guard,"Actual completion phase guard unexercised");
 std::cout<<"PASS native/raw spatial parameter witness, observer/live/completion/thread guards and unchanged raw PCM owner\n";
 {
  const Cache foreign_cache(argv[1]);MultiplayerAudio other(foreign_cache,3,manual);
  rejects([&]{other.bind_native_sound_metadata(provider);},"another Cache","Distinct same-file Cache bound native metadata");
 }
 // Fixed-time/selector controls exercise a genuine pending admission lifetime;
 // this does not assert original World timing or native prefix writeback.
 SourceScriptSoundClockPolicyTu15 policy;
 policy.time=[]{return SourceScriptSoundClockTimeTu15{100,30,0x3d088889};};
 policy.selected_seconds=[](auto,auto,const auto&){return 0x3f800000u;};
 policy.select=[](auto,auto&selection){selection={0xff,0xff};};
 auto pending=audio.prepare_script_impulse(clock,f.sound,policy,[](const auto&voice){need(!voice,"Pending control unexpectedly stopped a voice");},{0x027f,0x1f80});
 need(bool(pending),"Actual pending metadata-bind fixture missing");const auto pending_rows=clock->rows();
 rejects([&]{audio.bind_native_sound_metadata(provider);},"quiescent","Pending admission allowed native metadata rebind");
 need(clock->rows()==pending_rows,"Rejected pending rebind changed clock metadata");audio.cancel_prepared_script_impulse(*pending);audio.update();
 std::cout<<"PASS distinct same-file Cache and genuine pending admission reject metadata rebind\n";
 audio.bind_native_sound_metadata(provider);rejects([&]{clock->rows();},"closed","Quiescent rebind kept prior clock live");clock=audio.make_script_sound_clock();audio.prepare(f.sound);
 const auto saved=world->authority_checkpoint();{auto restore=world->prepare_authority(saved);need(world->commit_authority(std::move(restore)),"Actual identical-byte restore failed");}
 const auto seed=random,packet_count=events;const auto frames=audio.presentation_frames();
 rejects([&]{audio.prepare(f.sound);},"stale memory owner","Stale cached bank prepared");
 rejects([&]{audio.play_impulse(f.sound);},"stale memory owner","Stale cached bank played");
 rejects([&]{audio.render(48);},"stale memory owner","Stale metadata mixed PCM");
 rejects([&]{audio.update();},"stale memory owner","Stale metadata advanced update");
 rejects([&]{audio.master_volume(.5f);},"stale memory owner","Stale metadata changed master gain");
 rejects([&]{audio.paused(true);},"stale memory owner","Stale metadata changed paused state");
 rejects([&]{audio.listeners(listeners);},"stale memory owner","Stale metadata changed listener inputs");
 rejects([&]{audio.bind_native_sound_metadata(provider);},"stale memory owner","Stale provider rebound");
 need(random==seed&&events==packet_count&&audio.presentation_frames()==frames&&audio.voices()==0,"Stale metadata changed RNG/backend/events");audio.stop();
 provider=world->sound_variation_provider(cache);audio.bind_native_sound_metadata(provider);rejects([&]{clock->rows();},"closed","Restore rebind kept old clock live");random=0x1872c4d1;const auto fresh=audio.play_impulse_at(f.sound,{f.distance,0,0});need(fresh&&audio.voice_parameters(*fresh).volume_hundredths_db==f.native.volume && !audio.voice_parameters(*fresh).paused && audio.voice_parameters(*fresh).routes[0].linear_gain==1,"Fresh restore rebind failed exact output or stale mutation guard");
 world.reset();rejects([&]{audio.prepare(f.sound);},"stale memory owner","Destroyed World metadata used cached bank");audio.stop();need(audio.voices()==0,"Destroyed World prevented actual playback cleanup");
 std::cout<<"PASS cached-bank stale rejection before RNG/backend, identical-byte restore/fresh binding and cleanup after World destruction\n";
 std::cout<<"LIMIT actual native metadata/Audio handoff only; source selection-state writeback, original timing, Main/Sim and physical audibility remain separate\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
