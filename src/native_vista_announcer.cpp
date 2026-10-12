#include "halo2/native_vista_announcer.hpp"
#include "halo2/cache.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>
#include <map>
#include <mutex>
#include <set>
#include <utility>

namespace halo2 {
struct NativeVistaAnnouncer::State:std::enable_shared_from_this<State> {
  NativeVistaAnnouncerConfiguration configuration;
  VistaAnnouncerAssets assets;
  std::shared_ptr<const Cache> sound_cache;
  VistaAnnouncerEventState source;
  mutable std::recursive_mutex mutex;
  std::map<std::uint32_t,MultiplayerAudio::Voice> voices;
  std::map<MultiplayerAudio::Voice,std::uint32_t> tokens;
  std::vector<NativeVistaAnnouncerCompletion> completions;
  std::uint32_t next_token=0;
  std::uint8_t published=0;
  bool activation_published=false;
  std::unique_ptr<MultiplayerAudio> audio;
  explicit State(NativeVistaAnnouncerConfiguration c):configuration(std::move(c)),
      assets(open_vista_announcer_assets(configuration.pc_maps,configuration.scenario,
          configuration.native_audio_constants)) {
    if(configuration.language>=9 || configuration.tick_rate<=0)
      throw FormatError("Native Vista announcer requires actual supported language and positive tick rate");
    // Shared snd! indices belong to shared's ugh!, while a scenario Cache also
    // contains an independent local custom ugh!. The generic playback adapter
    // admits one gestalt. Keep the actual shared bank as a separate owner and
    // require exact definition identity for every response cue before using it.
    sound_cache=std::make_shared<Cache>(configuration.pc_maps/"shared.map",
        configuration.pc_maps,configuration.native_audio_constants);
    if(!sound_cache->is_vista())throw FormatError("Vista announcer shared sound owner is not PC");
    std::set<std::uint32_t> cues;
    for(const auto* bank:{&assets.responses.general,&assets.responses.slayer,&assets.responses.king})
      for(const auto& record:*bank){const Bytes response(record);
        for(unsigned language=0;language<9;++language){const auto cue=response.u32(0x48+language*8);
          if(cue!=vista_announcer_none)cues.insert(cue);}}
    for(const auto cue:cues){
      const auto selected=assets.cache->load_source_tag_metadata(cue);
      const auto shared=sound_cache->load_source_tag_metadata(cue);
      if(selected.group()!="snd!" || shared.group()!="snd!" ||
          selected.source_address()!=shared.source_address() || selected.size()!=shared.size() ||
          !std::equal(selected.bytes_at(selected.source_address(),selected.size()).begin(),
              selected.bytes_at(selected.source_address(),selected.size()).end(),
              shared.bytes_at(shared.source_address(),shared.size()).begin()))
        throw FormatError("Vista announcer cue is outside the exact shared sound owner");
    }
    vista_announcer_events_initialize(source,configuration.tick_rate);
    if(!configuration.mute)audio=std::make_unique<MultiplayerAudio>(*sound_cache,
        configuration.native_secondary_seed,configuration.platform);
  }
  std::uint32_t token() {
    for(std::uint64_t n=0;n<0xffffffffull;++n){const auto candidate=next_token++;
      if(candidate!=vista_announcer_none && !voices.contains(candidate))return candidate;}
    throw FormatError("Native Vista announcer queue token space exhausted");
  }
  std::uint32_t play(std::uint32_t sound,float scale,bool queued) {
    if(!audio || sound==vista_announcer_none)return vista_announcer_none;
    const auto id=token();std::weak_ptr<State> weak=shared_from_this();
    const auto voice=audio->play_impulse(sound,scale,MultiplayerAudioKind::Impulse,
        vista_announcer_none,0,[weak,id,queued](const SdlSoundCompletion& packet){
          if(const auto state=weak.lock()){
            const std::lock_guard lock(state->mutex);
            const auto found=state->voices.find(id);
            if(found==state->voices.end() || found->second!=packet.voice ||
                state->tokens.at(packet.voice)!=id)
              throw FormatError("Native Vista announcer completion lost its registered voice");
            if(queued)vista_announcer_queue_completed(state->source.queue,id);
            state->completions.push_back({id,packet});
            state->tokens.erase(packet.voice);state->voices.erase(found);
          }
        });
    if(!voice)return vista_announcer_none;
    if(tokens.contains(*voice))throw FormatError("Native Vista announcer voice alias");
    voices.emplace(id,*voice);tokens.emplace(*voice,id);return id;
  }
};
NativeVistaAnnouncer::NativeVistaAnnouncer(std::shared_ptr<State> state):state_(std::move(state)){}
std::unique_ptr<NativeVistaAnnouncer> NativeVistaAnnouncer::open(NativeVistaAnnouncerConfiguration c){
  return std::unique_ptr<NativeVistaAnnouncer>(new NativeVistaAnnouncer(std::make_shared<State>(std::move(c))));
}
NativeVistaAnnouncer::~NativeVistaAnnouncer(){try{stop();}catch(...){}}
void NativeVistaAnnouncer::pre_world_tick(){
  const std::lock_guard lock(state_->mutex);
  vista_announcer_queue_update(state_->source.queue,[&](auto sound,auto scale){
    return state_->play(sound,scale,true);
  });
}
std::vector<VistaAnnouncerAudioSelection> NativeVistaAnnouncer::publish_player_activation(
    const VistaAnnouncerEvent& event,std::uint8_t mode,bool over,std::uint32_t tick,
    std::span<const VistaAnnouncerLocalPlayer> players) {
  const std::lock_guard lock(state_->mutex);
  auto expected=vista_announcer_event_construct(event.words[0],0);expected.words[2]=event.words[2];
  if((event.words[0]!=2 && event.words[0]!=6) || (mode!=1 && mode!=3 && mode!=5) || event.words[2]==vista_announcer_none || event.words!=expected.words ||
      event.data!=expected.data)
    throw FormatError("Native Vista activation requires the actual normal engine publication");
  const auto found=std::find_if(players.begin(),players.end(),[&](const auto& player){
    return player.datum==event.words[2];});
  if(found==players.end() || (found->flags&3u)!=1 || found->output_user<0 || found->output_user>=4)
    throw FormatError("Native Vista activation lacks its actual active local output binding");
  state_->activation_published=true;
  const VistaAnnouncerEventContext context{mode,over,state_->configuration.language,tick,state_->configuration.tick_rate};
  std::vector<VistaAnnouncerAudioSelection> selected;
  for(const auto& player:players)if(const auto result=vista_announcer_event_audio(state_->source,
      state_->assets.responses,event,player,context,state_->configuration.floating_point,
      [&](auto sound,auto scale){return state_->play(sound,scale,false);}))selected.push_back(*result);
  return selected;
}
std::vector<VistaAnnouncerAudioSelection> NativeVistaAnnouncer::publish_king_event(
    const VistaAnnouncerEvent& event,std::uint8_t mode,bool over,std::uint32_t tick,
    std::span<const VistaAnnouncerLocalPlayer> players) {
  if(event.words[0]!=6)throw FormatError("Native Vista King publication requires source category6");
  return publish_engine_event(event,mode,over,tick,players);
}
std::vector<VistaAnnouncerAudioSelection> NativeVistaAnnouncer::publish_engine_event(
    const VistaAnnouncerEvent& event,std::uint8_t mode,bool over,std::uint32_t tick,
    std::span<const VistaAnnouncerLocalPlayer> players) {
  const std::lock_guard lock(state_->mutex);
  if(event.words[0]!=0 && event.words[0]!=2 && event.words[0]!=6)
    throw FormatError("Native Vista event has no recovered response bank");
  const VistaAnnouncerEventContext context{mode,over,state_->configuration.language,tick,state_->configuration.tick_rate};
  std::vector<VistaAnnouncerAudioSelection> selected;
  for(const auto& player:players)if(const auto result=vista_announcer_event_audio(state_->source,
      state_->assets.responses,event,player,context,state_->configuration.floating_point,
      [&](auto sound,auto scale){return state_->play(sound,scale,false);}))selected.push_back(*result);
  return selected;
}
std::vector<VistaAnnouncerAudioSelection> NativeVistaAnnouncer::post_world_tick(bool engine,
    std::uint8_t mode,bool over,std::uint32_t tick,std::span<const VistaAnnouncerLocalPlayer> players){
  const std::lock_guard lock(state_->mutex);std::vector<VistaAnnouncerAudioSelection> selected;
  const auto event=vista_announcer_game_over_event(state_->published,engine,mode,over);
  if(!event)return selected;
  const VistaAnnouncerEventContext context{mode,over,state_->configuration.language,tick,state_->configuration.tick_rate};
  for(const auto& player:players)if(const auto result=vista_announcer_event_audio(state_->source,
      state_->assets.responses,*event,player,context,state_->configuration.floating_point,
      [&](auto sound,auto scale){return state_->play(sound,scale,false);}))selected.push_back(*result);
  return selected;
}
void NativeVistaAnnouncer::audio_update(float volume,const std::array<float,sound_class_count>& gains){
  const std::lock_guard lock(state_->mutex);
  if(state_->audio){state_->audio->master_volume(volume);state_->audio->class_gains(gains);state_->audio->update();}
}
void NativeVistaAnnouncer::stop(){const std::lock_guard lock(state_->mutex);if(state_->audio)state_->audio->stop();}
NativeVistaAnnouncerSnapshot NativeVistaAnnouncer::snapshot()const {
  const std::lock_guard lock(state_->mutex);
  NativeVistaAnnouncerSnapshot out{state_->source,{},state_->completions,state_->published!=0,!state_->activation_published,state_->configuration.mute};
  for(const auto& [token,voice]:state_->voices)out.voices.push_back({token,voice});
  return out;
}
const VistaAnnouncerAssets& NativeVistaAnnouncer::assets()const{return state_->assets;}
const Cache& NativeVistaAnnouncer::audio_cache()const{return *state_->sound_cache;}
std::vector<float> NativeVistaAnnouncer::render_manual(std::size_t frames){
  const std::lock_guard lock(state_->mutex);
  if(state_->configuration.platform.delivery!=SdlSoundPlatformConfiguration::Delivery::Manual)
    throw FormatError("Native Vista announcer manual rendering requires the explicit Manual provider");
  if(!state_->audio)return std::vector<float>(frames*2,0.f);
  return state_->audio->render(frames);
}
} // namespace halo2
