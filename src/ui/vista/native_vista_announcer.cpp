#include "halo2/native_vista_announcer.hpp"
#include "halo2/cache.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/source_announcer_events_tu15.hpp"
#include <algorithm>
#include <atomic>
#include <limits>
#include <map>
#include <mutex>
#include <set>
#include <thread>
#include <utility>

namespace halo2 {
struct NativeVistaAnnouncer::State:std::enable_shared_from_this<State> {
  struct CompletionRecord {
    MultiplayerAudio::VoiceReceipt voice;
    std::shared_ptr<const unsigned char> publication;
    std::uint32_t token{};
    bool queued{},consumed{};
  };
  NativeVistaAnnouncerConfiguration configuration;
  VistaAnnouncerAssets assets;
  std::shared_ptr<const Cache> sound_cache;
  VistaAnnouncerEventState source;
  std::optional<SourceAnnouncerResponsesTu15> source_responses;
  mutable std::recursive_mutex mutex;
  std::map<std::uint32_t,MultiplayerAudio::Voice> voices;
  std::map<MultiplayerAudio::Voice,std::uint32_t> tokens;
  std::vector<NativeVistaAnnouncerCompletion> completions;
  std::uint32_t next_token=0;
  std::uint8_t published=0;
  bool activation_published=false;
  const std::thread::id creator=std::this_thread::get_id();
  std::atomic_bool enrolled{false};
  bool foreign_producer=false,held=false,alive=true;
  unsigned producer_depth=0;
  std::uint64_t revision=0;
  std::shared_ptr<const unsigned char> publication=std::make_shared<const unsigned char>(0);
  std::map<std::uint32_t,std::shared_ptr<CompletionRecord>> records;
  std::unique_ptr<MultiplayerAudio> audio;
  void producer_thread()const {
    if(enrolled.load() && std::this_thread::get_id()!=creator)
      throw FormatError("Continuable MCC announcer used from foreign producer thread");
  }
  void available()const {
    if(!alive || held)throw FormatError("MCC announcer producer is unavailable during publication");
  }
  struct Operation {
    State& state;
    explicit Operation(State& s):state(s){
      s.producer_thread();
      s.available();
      if(s.revision==std::numeric_limits<std::uint64_t>::max())
        throw FormatError("MCC announcer producer revision exhausted");
      if(std::this_thread::get_id()!=s.creator)s.foreign_producer=true;
      ++s.revision;++s.producer_depth;
    }
    ~Operation(){--state.producer_depth;}
  };
  static bool same_owner(const std::shared_ptr<void>& a,const std::shared_ptr<void>& b) noexcept {
    return a.get()==b.get() && !a.owner_before(b) && !b.owner_before(a);
  }
  static void dispatch(const std::shared_ptr<void>& owner,const std::shared_ptr<void>& branch,
      std::uint64_t token,bool queued,const MultiplayerAudio::VoiceReceipt& voice,
      const SdlSoundCompletion& packet) {
    const auto state=std::static_pointer_cast<State>(owner);
    state->producer_thread();
    const std::lock_guard lock(state->mutex);
    state->available();
    const auto record=std::static_pointer_cast<CompletionRecord>(branch);
    const auto found=state->records.find(std::uint32_t(token));
    if(token>0xffffffffull || found==state->records.end() ||
        !same_owner(found->second,record) || record->consumed ||
        record->publication!=state->publication || record->token!=token || record->queued!=queued ||
        !record->voice.same(voice) || voice.id()!=packet.voice)
      throw FormatError("MCC announcer completion belongs to a retired publication");
    state->complete(std::uint32_t(token),queued,packet);
    record->consumed=true;state->records.erase(found);
  }
  void complete(std::uint32_t id,bool queued,const SdlSoundCompletion& packet) {
    const auto found=voices.find(id);
    const auto reverse=tokens.find(packet.voice);
    if(found==voices.end() || found->second!=packet.voice || reverse==tokens.end() || reverse->second!=id)
      throw FormatError("Native Vista announcer completion lost its registered voice");
    if(queued)vista_announcer_queue_completed(source.queue,id);
    completions.push_back({id,packet});tokens.erase(reverse);voices.erase(found);
  }
  void validate()const {
    if(source.queue.count>source.queue.slots.size() || voices.size()!=tokens.size() ||
        (enrolled.load() && voices.size()!=records.size()))
      throw FormatError("MCC announcer continuation has an incomplete owner census");
    for(const auto& [token,voice]:voices){
      const auto reverse=tokens.find(voice);const auto record=records.find(token);
      if(reverse==tokens.end() || reverse->second!=token || record==records.end() ||
          record->second->token!=token || record->second->voice.id()!=voice || record->second->consumed ||
          record->second->publication!=publication)
        throw FormatError("MCC announcer continuation lost its exact voice mapping");
      if(record->second->queued && (!source.queue.count || !source.queue.slots[0].active ||
          source.queue.slots[0].voice!=token))
        throw FormatError("MCC announcer queued voice lost its actual active front");
    }
    if(source.queue.count && source.queue.slots[0].active){
      const auto record=records.find(source.queue.slots[0].voice);
      if(record==records.end() || !record->second->queued)
        throw FormatError("MCC announcer active queue front has no completion owner");
    }
  }
  explicit State(NativeVistaAnnouncerConfiguration c):configuration(std::move(c)),
      assets(open_vista_announcer_assets(configuration.pc_maps,configuration.scenario,
          configuration.native_audio_constants)) {
    if(configuration.language>=9 || configuration.tick_rate<=0)
      throw FormatError("Native Vista announcer requires actual supported language and positive tick rate");
    // Shared snd! indices belong to shared's ugh!, while a scenario Cache also
    // contains an independent local custom ugh!. The generic playback adapter
    // admits one gestalt. Keep the actual shared bank as a separate owner and
    // require exact definition identity for every response cue before using it.
    // MCC's existing decoder resolves a separate gestalt/resource bank for
    // each sound. Keep the selected scenario's datum namespace instead of
    // interpreting those datums through a separately opened Vista shared bank.
    sound_cache=assets.cache->is_mcc()?assets.cache:
        std::make_shared<Cache>(configuration.pc_maps/"shared.map",
            configuration.pc_maps,configuration.native_audio_constants);
    if(!sound_cache->is_vista() && !sound_cache->is_mcc())
      throw FormatError("PC announcer sound owner is not PC");
    std::set<std::uint32_t> cues;
    if(configuration.source_tu15) {
      source_responses=load_source_announcer_responses_tu15(*assets.cache);
      for(const auto& bank:source_responses->banks) {
        for(const auto& record:bank.definitions)for(unsigned language=0;language<9;++language) {
          const auto cue=Bytes(record).u32(0x48+language*8);if(cue!=vista_announcer_none)cues.insert(cue);
        }
        for(const auto& alternatives:bank.alternatives)for(const auto& alternative:alternatives)
          for(const auto cue:alternative.sounds)if(cue!=vista_announcer_none)cues.insert(cue);
      }
    }
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
    if(!configuration.mute)audio=std::make_unique<MultiplayerAudio>(sound_cache,
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
    if(enrolled.load()){
      auto record=std::make_shared<CompletionRecord>();
      record->token=id;record->queued=queued;record->publication=publication;
      // All registry allocation precedes actual playback. Once the Core
      // returns its receipt, node insertion publishes the links without an
      // allocation failure leaving an unregistered live completion owner.
      decltype(voices) staged_voices;staged_voices.emplace(id,0);
      decltype(tokens) staged_tokens;staged_tokens.emplace(0,id);
      decltype(records) staged_records;staged_records.emplace(id,record);
      auto voice_node=staged_voices.extract(id);auto token_node=staged_tokens.extract(0);
      auto record_node=staged_records.extract(id);
      MultiplayerAudio::CompanionCompletion completion{shared_from_this(),record,id,queued,&State::dispatch};
      const auto voice=audio->play_companion_impulse(sound,scale,MultiplayerAudioKind::Impulse,
          vista_announcer_none,0,std::move(completion));
      if(!voice)return vista_announcer_none;
      record->voice=*voice;
      if(tokens.contains(voice->id()))throw FormatError("Native Vista announcer voice alias");
      voice_node.mapped()=voice->id();token_node.key()=voice->id();
      voices.insert(std::move(voice_node));tokens.insert(std::move(token_node));records.insert(std::move(record_node));
      return id;
    }
    const auto voice=audio->play_impulse(sound,scale,MultiplayerAudioKind::Impulse,
        vista_announcer_none,0,[weak,id,queued](const SdlSoundCompletion& packet){
          if(const auto state=weak.lock()){
            const std::lock_guard lock(state->mutex);
            state->complete(id,queued,packet);
          }
        });
    if(!voice)return vista_announcer_none;
    if(tokens.contains(*voice))throw FormatError("Native Vista announcer voice alias");
    voices.emplace(id,*voice);tokens.emplace(*voice,id);return id;
  }
};
struct NativeVistaAnnouncer::SnapshotData {
  struct SavedRecord {
    MultiplayerAudio::VoiceReceipt voice;
    std::shared_ptr<void> identity;
    bool queued{};
  };
  std::weak_ptr<State> owner;
  std::shared_ptr<const Cache> cache;
  VistaAnnouncerEventState source;
  std::map<std::uint32_t,MultiplayerAudio::Voice> voices;
  std::map<MultiplayerAudio::Voice,std::uint32_t> tokens;
  std::map<std::uint32_t,SavedRecord> records;
  std::vector<NativeVistaAnnouncerCompletion> completions;
  std::uint32_t next_token{};
  std::uint8_t published{};
  bool activation_published{};
  std::optional<MultiplayerAudio::Checkpoint> audio;
};
struct NativeVistaAnnouncer::PreparedData {
  enum class Phase {Prepared,Committing,Committed,Cancelled};
  std::atomic<Phase> phase{Phase::Prepared};
  std::weak_ptr<State> owner;
  std::uint64_t revision{};
  VistaAnnouncerEventState source;
  std::map<std::uint32_t,MultiplayerAudio::Voice> voices;
  std::map<MultiplayerAudio::Voice,std::uint32_t> tokens;
  std::map<std::uint32_t,std::shared_ptr<State::CompletionRecord>> records;
  std::vector<NativeVistaAnnouncerCompletion> completions;
  std::uint32_t next_token{};
  std::uint8_t published{};
  bool activation_published{};
  std::shared_ptr<const unsigned char> publication=std::make_shared<const unsigned char>(0);
  std::optional<MultiplayerAudio::PreparedReplacement> audio;
  void cancel()noexcept {
    auto expected=Phase::Prepared;
    phase.compare_exchange_strong(expected,Phase::Cancelled);
  }
};
struct NativeVistaAnnouncer::HeldPublication::Impl {
  std::shared_ptr<State> state;
  std::shared_ptr<PreparedData> data;
  const std::thread::id executor=std::this_thread::get_id();
  std::unique_lock<std::recursive_mutex> producer;
  std::unique_ptr<MultiplayerAudio::HeldPublication> audio;
  Impl(std::shared_ptr<State> s,std::shared_ptr<PreparedData> d)
      :state(std::move(s)),data(std::move(d)),producer(state->mutex){
    state->available();
    if(!state->enrolled.load() || state->producer_depth || !data ||
        data->owner.lock()!=state || data->phase.load()!=PreparedData::Phase::Prepared)
      throw FormatError("MCC announcer publication requires its quiescent prepared owner");
    state->held=true;
    try{
      if(data->audio)audio.reset(new MultiplayerAudio::HeldPublication(
          state->audio->hold_publication(*data->audio)));
    }catch(...){state->held=false;throw;}
  }
  ~Impl(){
    if(std::this_thread::get_id()!=executor)std::terminate();
    // PreparedData retains both outgoing images through callback exclusion
    // and producer exclusion. Last-owner retirement follows both unlocks.
    audio.reset();state->held=false;producer.unlock();
  }
};
NativeVistaAnnouncer::PreparedReplacement::~PreparedReplacement(){if(data_)data_->cancel();}
NativeVistaAnnouncer::PreparedReplacement::PreparedReplacement(PreparedReplacement&& other)noexcept
    :data_(std::move(other.data_)){}
NativeVistaAnnouncer::PreparedReplacement& NativeVistaAnnouncer::PreparedReplacement::operator=(PreparedReplacement&& other)noexcept {
  if(this!=&other){if(data_)data_->cancel();data_=std::move(other.data_);}return *this;
}
NativeVistaAnnouncer::HeldPublication::HeldPublication(std::unique_ptr<Impl> impl):impl_(std::move(impl)){}
NativeVistaAnnouncer::HeldPublication::~HeldPublication()=default;
NativeVistaAnnouncer::NativeVistaAnnouncer(std::shared_ptr<State> state):state_(std::move(state)){}
std::unique_ptr<NativeVistaAnnouncer> NativeVistaAnnouncer::open(NativeVistaAnnouncerConfiguration c){
  return std::unique_ptr<NativeVistaAnnouncer>(new NativeVistaAnnouncer(std::make_shared<State>(std::move(c))));
}
NativeVistaAnnouncer::~NativeVistaAnnouncer(){
  if(state_->enrolled.load() && std::this_thread::get_id()!=state_->creator)std::terminate();
  try{stop();}catch(...){}
  const std::lock_guard lock(state_->mutex);state_->alive=false;
}
void NativeVistaAnnouncer::pre_world_tick(){
  state_->producer_thread();
  const std::lock_guard lock(state_->mutex);
  State::Operation operation(*state_);
  vista_announcer_queue_update(state_->source.queue,[&](auto sound,auto scale){
    return state_->play(sound,scale,true);
  });
}
std::vector<VistaAnnouncerAudioSelection> NativeVistaAnnouncer::publish_player_activation(
    const VistaAnnouncerEvent& event,std::uint8_t mode,bool over,std::uint32_t tick,
    std::span<const VistaAnnouncerLocalPlayer> players) {
  state_->producer_thread();
  const std::lock_guard lock(state_->mutex);
  State::Operation operation(*state_);
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
  state_->producer_thread();
  const std::lock_guard lock(state_->mutex);
  State::Operation operation(*state_);
  if(event.words[0]!=0 && event.words[0]!=2 && event.words[0]!=6)
    throw FormatError("Native Vista event has no recovered response bank");
  const VistaAnnouncerEventContext context{mode,over,state_->configuration.language,tick,state_->configuration.tick_rate};
  std::vector<VistaAnnouncerAudioSelection> selected;
  for(const auto& player:players)if(const auto result=vista_announcer_event_audio(state_->source,
      state_->assets.responses,event,player,context,state_->configuration.floating_point,
      [&](auto sound,auto scale){return state_->play(sound,scale,false);}))selected.push_back(*result);
  return selected;
}
std::vector<VistaAnnouncerAudioSelection> NativeVistaAnnouncer::publish_source_event(
    const SourceMultiplayerEventTu15& input,std::span<const unsigned> slots) {
  state_->producer_thread();const std::lock_guard lock(state_->mutex);State::Operation operation(*state_);
  if(!state_->source_responses || input.words[0]>=state_->source_responses->banks.size() || slots.size()>4)
    throw FormatError("Xbox event lacks its actual source response owner or local views");
  std::vector<VistaAnnouncerAudioSelection> result;
  if(input.game_over && (input.words[0]!=0 || input.words[1]!=19))return result;
  const VistaAnnouncerEvent event{input.words,input.data};
  const auto& bank=state_->source_responses->banks[input.words[0]];
  auto random=input.random_seed;
  // Original broadcast visits ascending actual datum slots. A targeted event
  // retains its target even when that source row has already departed.
  for(unsigned slot=0;slot<input.players.size();++slot) {
    const auto view=std::find(slots.begin(),slots.end(),slot);if(view==slots.end())continue;
    const auto& row=input.players[slot];if(row.datum==vista_announcer_none)continue;
    if(event.words[2]==vista_announcer_none ? bool(row.flags&2u) : event.words[2]!=row.datum)continue;
    const VistaAnnouncerLocalPlayer player{row.datum,row.flags,row.membership,std::int16_t(view-slots.begin())};
    const auto recipient=source_announcer_recipient_tu15(bank.definitions,event,player);if(!recipient)continue;
    const auto ordinal=recipient->ordinal;const auto& raw=bank.definitions[ordinal];
    const auto choice=source_announcer_response_tu15(raw,bank.alternatives[ordinal],state_->configuration.language,
        state_->configuration.tick_rate,random,state_->configuration.floating_point);
    const Bytes definition(raw);VistaAnnouncerAudioSelection selected{};
    selected.sound=choice.sound;selected.delay_ticks=choice.delay_ticks;selected.announcer=choice.announcer;
    selected.audience=recipient->audience;selected.response_ordinal=ordinal;
    selected.caption_key=definition.u32(0xc);selected.display_key=definition.u32(0x14);
    if(choice.sound!=vista_announcer_none && source_announcer_remember_tu15(state_->source,event,
        choice.sound,input.tick,state_->configuration.tick_rate,state_->configuration.floating_point)) {
      if(choice.announcer) {
        const auto before=state_->source.queue.count;
        vista_announcer_queue_enqueue(state_->source.queue,choice.sound,choice.delay_ticks);
        selected.requested=state_->source.queue.count!=before;
      } else selected.requested=state_->play(choice.sound,1.f,false)!=vista_announcer_none;
    }
    result.push_back(selected);
  }
  return result;
}
std::vector<VistaAnnouncerAudioSelection> NativeVistaAnnouncer::post_world_tick(bool engine,
    std::uint8_t mode,bool over,std::uint32_t tick,std::span<const VistaAnnouncerLocalPlayer> players){
  state_->producer_thread();
  const std::lock_guard lock(state_->mutex);State::Operation operation(*state_);
  std::vector<VistaAnnouncerAudioSelection> selected;
  const auto event=vista_announcer_game_over_event(state_->published,engine,mode,over);
  if(!event)return selected;
  const VistaAnnouncerEventContext context{mode,over,state_->configuration.language,tick,state_->configuration.tick_rate};
  for(const auto& player:players)if(const auto result=vista_announcer_event_audio(state_->source,
      state_->assets.responses,*event,player,context,state_->configuration.floating_point,
      [&](auto sound,auto scale){return state_->play(sound,scale,false);}))selected.push_back(*result);
  return selected;
}
void NativeVistaAnnouncer::audio_update(float volume,const std::array<float,sound_class_count>& gains){
  state_->producer_thread();
  const std::lock_guard lock(state_->mutex);
  State::Operation operation(*state_);
  if(state_->audio){state_->audio->master_volume(volume);state_->audio->class_gains(gains);state_->audio->update();}
}
void NativeVistaAnnouncer::stop(){state_->producer_thread();const std::lock_guard lock(state_->mutex);
  State::Operation operation(*state_);if(state_->audio)state_->audio->stop();}
NativeVistaAnnouncerSnapshot NativeVistaAnnouncer::snapshot()const {
  const std::lock_guard lock(state_->mutex);
  state_->available();
  NativeVistaAnnouncerSnapshot out{state_->source,{},state_->completions,state_->published!=0,!state_->activation_published,state_->configuration.mute};
  for(const auto& [token,voice]:state_->voices)out.voices.push_back({token,voice});
  out.next_token=state_->next_token;out.published_byte=state_->published;
  return out;
}
const VistaAnnouncerAssets& NativeVistaAnnouncer::assets()const{return state_->assets;}
const Cache& NativeVistaAnnouncer::audio_cache()const{return *state_->sound_cache;}
std::vector<float> NativeVistaAnnouncer::render_manual(std::size_t frames){
  state_->producer_thread();
  const std::lock_guard lock(state_->mutex);
  state_->producer_thread();
  state_->available();
  if(state_->configuration.platform.delivery!=SdlSoundPlatformConfiguration::Delivery::Manual)
    throw FormatError("Native Vista announcer manual rendering requires the explicit Manual provider");
  if(!state_->audio)return std::vector<float>(frames*2,0.f);
  return state_->audio->render(frames);
}
void NativeVistaAnnouncer::enroll_continuation(){
  if(std::this_thread::get_id()!=state_->creator)
    throw FormatError("MCC announcer enrollment requires its creating producer thread");
  const std::lock_guard lock(state_->mutex);state_->available();
  if(!state_->sound_cache->is_mcc() || state_->foreign_producer || state_->producer_depth ||
      !state_->voices.empty() || !state_->tokens.empty() || !state_->records.empty())
    throw FormatError("MCC announcer enrollment requires quiescent exclusive MCC ownership");
  if(state_->enrolled.load())return;
  if(state_->audio)state_->audio->enroll_continuation();
  state_->enrolled.store(true);
}
NativeVistaAnnouncer::Checkpoint NativeVistaAnnouncer::checkpoint()const {
  state_->producer_thread();const std::lock_guard lock(state_->mutex);state_->producer_thread();state_->available();
  if(!state_->enrolled.load() || state_->producer_depth)
    throw FormatError("MCC announcer capture requires a quiescent enrolled owner");
  state_->validate();
  auto data=std::make_shared<SnapshotData>();data->owner=state_;data->cache=state_->sound_cache;
  data->source=state_->source;data->voices=state_->voices;data->tokens=state_->tokens;
  data->completions=state_->completions;data->next_token=state_->next_token;
  data->published=state_->published;data->activation_published=state_->activation_published;
  for(const auto& [token,record]:state_->records)
    data->records.emplace(token,SnapshotData::SavedRecord{record->voice,record,record->queued});
  if(state_->audio){
    data->audio=state_->audio->checkpoint();
    const auto census=data->audio->companion_completions();
    if(census.size()!=data->records.size())throw FormatError("MCC announcer Core completion census differs");
    for(const auto& item:census){
      const auto owner=item.completion.owner.lock();
      const auto record=data->records.find(std::uint32_t(item.completion.token));
      if(item.completion.token>0xffffffffull || record==data->records.end() ||
          !State::same_owner(owner,state_) || !State::same_owner(record->second.identity,item.completion.branch) ||
          record->second.queued!=item.completion.queued || !record->second.voice.same(item.voice) ||
          item.completion.dispatch!=&State::dispatch)
        throw FormatError("MCC announcer Core completion lost its exact owner binding");
    }
  }
  Checkpoint result;result.data_=std::move(data);return result;
}
NativeVistaAnnouncer::PreparedReplacement NativeVistaAnnouncer::prepare_restore(const Checkpoint& checkpoint)const {
  state_->producer_thread();const std::lock_guard lock(state_->mutex);state_->producer_thread();state_->available();
  const auto& saved=checkpoint.data_;
  if(!state_->enrolled.load() || state_->producer_depth || !saved || saved->owner.lock()!=state_ ||
      saved->cache.get()!=state_->sound_cache.get() || saved->cache.owner_before(state_->sound_cache) ||
      state_->sound_cache.owner_before(saved->cache) || bool(saved->audio)!=bool(state_->audio) ||
      state_->revision==std::numeric_limits<std::uint64_t>::max())
    throw FormatError("MCC announcer restoration requires its authenticated complete owner image");
  auto data=std::make_shared<PreparedData>();data->owner=state_;data->revision=state_->revision;
  data->source=saved->source;data->completions=saved->completions;
  data->next_token=saved->next_token;data->published=saved->published;
  data->activation_published=saved->activation_published;
  if(saved->audio){
    // The binder is an immediate preparation operation. It never captures or
    // replays the old completion function, and its target is private storage.
    const MultiplayerAudio::CompanionBinder binder=[saved,target=data.get(),owner=std::weak_ptr<State>(state_)](
        const MultiplayerAudio::CompanionCompletion& completion,const MultiplayerAudio::VoiceReceipt& voice){
      const auto state=owner.lock();const auto record=saved->records.find(std::uint32_t(completion.token));
      if(!state || completion.token>0xffffffffull || record==saved->records.end() ||
          !State::same_owner(completion.owner.lock(),state) ||
          !State::same_owner(completion.branch,record->second.identity) ||
          record->second.queued!=completion.queued || completion.dispatch!=&State::dispatch)
        throw FormatError("MCC announcer preparation received a foreign completion descriptor");
      auto next=std::make_shared<State::CompletionRecord>();next->voice=voice;
      next->publication=target->publication;next->token=std::uint32_t(completion.token);next->queued=completion.queued;
      const auto token=next->token;
      if(!target->records.emplace(token,next).second || !target->voices.emplace(token,voice.id()).second ||
          !target->tokens.emplace(voice.id(),token).second)
        throw FormatError("MCC announcer preparation aliased its completion census");
      return MultiplayerAudio::CompanionCompletion{state,next,token,next->queued,&State::dispatch};
    };
    data->audio=state_->audio->prepare_restore(*saved->audio,binder);
  }
  if(data->records.size()!=saved->records.size() || data->voices.size()!=saved->voices.size() ||
      data->tokens.size()!=saved->tokens.size())
    throw FormatError("MCC announcer preparation omitted a saved voice owner");
  for(const auto& [token,record]:saved->records){
    const auto prospective=data->audio->voice(record.voice);
    const auto target=data->records.find(token);
    if(!prospective || target==data->records.end() || !prospective->same(target->second->voice))
      throw FormatError("MCC announcer preparation has a different prospective voice record");
  }
  PreparedReplacement result;result.data_=std::move(data);return result;
}
NativeVistaAnnouncer::HeldPublication NativeVistaAnnouncer::hold_publication(PreparedReplacement& prepared){
  if(std::this_thread::get_id()!=state_->creator)
    throw FormatError("MCC announcer publication requires its creating producer thread");
  return HeldPublication(std::make_unique<HeldPublication::Impl>(state_,prepared.data_));
}
bool NativeVistaAnnouncer::ready(const PreparedReplacement& prepared,const HeldPublication& held)const noexcept {
  const auto* hold=held.impl_.get();const auto& data=prepared.data_;
  if(!hold || std::this_thread::get_id()!=hold->executor || hold->state!=state_ || !data ||
      hold->data!=data || data->phase.load()!=PreparedData::Phase::Prepared ||
      !state_->alive || !state_->held || !state_->enrolled.load() || state_->producer_depth ||
      state_->revision!=data->revision || data->owner.lock()!=state_ ||
      bool(data->audio)!=bool(state_->audio) || bool(hold->audio)!=bool(data->audio))return false;
  return !data->audio || state_->audio->ready(*data->audio,*hold->audio);
}
bool NativeVistaAnnouncer::commit(PreparedReplacement& prepared,HeldPublication& held)noexcept {
  if(!ready(prepared,held))return false;
  auto& data=*prepared.data_;auto expected=PreparedData::Phase::Prepared;
  if(!data.phase.compare_exchange_strong(expected,PreparedData::Phase::Committing))return false;
  if(data.audio && !state_->audio->commit(*data.audio,*held.impl_->audio)){
    data.phase.store(PreparedData::Phase::Cancelled);return false;
  }
  using std::swap;
  swap(state_->source,data.source);state_->voices.swap(data.voices);state_->tokens.swap(data.tokens);
  state_->records.swap(data.records);state_->completions.swap(data.completions);
  swap(state_->next_token,data.next_token);swap(state_->published,data.published);
  swap(state_->activation_published,data.activation_published);state_->publication.swap(data.publication);
  ++state_->revision;data.phase.store(PreparedData::Phase::Committed);return true;
}
} // namespace halo2
