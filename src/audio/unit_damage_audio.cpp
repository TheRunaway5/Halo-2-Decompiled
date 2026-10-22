#include "halo2/unit_damage_audio.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <exception>
#include <limits>
#include <set>
#include <thread>

namespace halo2 {
namespace { constexpr auto none=0xffffffffu;
Bytes root(const SourceTagMetadata &t){return Bytes(t.bytes_at(t.source_address(),t.size()));}
Bytes block(const SourceTagMetadata &t,const Bytes &b,unsigned at,unsigned stride) {
  const auto d=read_tag_block_descriptor(b,at);
  return Bytes(d.count?t.bytes_at(d.address,d.extent(stride,65536)):std::span<const std::byte>{});
}
}
std::uint32_t shield_sound_mask(float previous,float health,float shield,
    float maximum,bool dead,bool charging,bool playing) {
  if(dead || !(maximum>0) || previous==-1 || !playing)return 0;
  std::uint32_t flags=charging?1u:0u;
  if(previous>shield && shield<1.f)flags|=2u; //13A720 multiplayer full-shield gate.
  if(shield<.25f && shield>0)flags|=4u;
  if(shield==0)flags|=8u;
  (void)health; // Original status+28 is maximum shield, not health.
  return flags;
}
void advance_damage_voice_state(DamageVoiceState &s) {
  if(s.decay>0 && --s.decay==0 && s.count>0){--s.count;s.decay=45;}
  if(s.cooldown>0)--s.cooldown;
  if(s.outside_cooldown>0)--s.outside_cooldown;
}
std::optional<unsigned> select_player_damage_voice(DamageVoiceState &s,
    const UnitDamageVoiceEvent &e,int type,bool special,float first,float second,
    std::uint32_t &random,bool source_mode_four) {
  if(e.report_flags&0x20u)return {}; //CA0B0 suppresses the request before1147E0.
  const bool hard=e.report_flags&1u;
  bool outside=false;unsigned selection{};
  if(hard) {
    const bool strong=e.recent_body>e.visible_body+.2f;
    if(type==7 || special)selection=3;
    else if(type==1)selection=source_mode_four?0:1; //1147E0 tests options+0C, not game type+08.
    else if(type==3 || type==4 || type==9)selection=2;
    else if(!strong && type==6)selection=5;
    else selection=(e.report_flags&0x40u)?4:0;
  }else {
    if(s.outside_cooldown)return {};
    outside=e.visible_body>second; // C96B0's actor-free unit-definition branch.
    const bool body=e.recent_body>0;
    if(type==1)selection=11;
    else if(outside) {
      if(body) {
        random=random*1664525u+1013904223u;
        const float value=float(random>>16)*(1.f/65535.f);
        selection=value<.6667f?8:9;
      }else selection=10;
    }else {
      if(s.cooldown || s.count>=3)return {};
      if(s.priority) {
        random=random*1664525u+1013904223u;
        if(!(float(random>>16)*(1.f/65535.f)<.4f))return {};
      }
      selection=body?7:10;
    }
    (void)first; // Range1 and range0 share1147E0's ordinary branch.
  }
  const int priority=hard?15:12;
  if(s.priority && priority<=s.priority)return {};
  return selection;
}
struct NativeUnitDamageAudio::Impl {
  const Cache &cache;MultiplayerAudio &audio;std::uint32_t &random;
  struct Control {std::atomic_bool alive{true};};
  std::shared_ptr<Control> control=std::make_shared<Control>();
  const std::thread::id thread=std::this_thread::get_id();
  bool enrolled{},active{};std::uint64_t revision{};
  void change()noexcept{if(revision!=std::numeric_limits<std::uint64_t>::max())++revision;}
  void require()const{
    if(!control->alive.load())throw FormatError("Damage audio owner closed");
    if(enrolled && thread!=std::this_thread::get_id())
      throw FormatError("Continuable damage audio used from foreign thread");
  }
  struct Operation {
    Impl&owner;
    explicit Operation(Impl&i):owner(i){
      i.require();
      if(i.enrolled && i.active)throw FormatError("Continuable damage audio reentered");
      if(i.enrolled)i.active=true;
      i.change();
    }
    ~Operation(){if(owner.enrolled)owner.active=false;}
  };
  struct Handle {
    MultiplayerAudio::Voice raw{};
    std::optional<MultiplayerAudio::VoiceReceipt> receipt;
    bool expired{};
  };
  bool voice_active(const Handle&h)const{return !h.expired && (h.receipt?audio.voice_active(*h.receipt):audio.voice_active(h.raw));}
  void stop_voice(const Handle&h,bool teardown=false){
    if(h.expired)return;
    if(h.receipt){if(audio.voice_active(*h.receipt))audio.stop_voice(*h.receipt);}
    else audio.stop_voice(h.raw);
    if(!teardown)require();
  }
  std::optional<Handle> impulse(std::uint32_t sound,float gain,MultiplayerAudioKind kind,
      std::uint32_t unit,unsigned view){
    std::optional<Handle> result;
    if(enrolled){if(auto voice=audio.play_impulse_receipt(sound,gain,kind,unit,view))result=Handle{voice->id(),*voice};}
    else if(auto voice=audio.play_impulse(sound,gain,kind,unit,view))result=Handle{*voice,{}};
    if(!control->alive.load()){
      if(result)try{stop_voice(*result,true);}catch(...){}
      throw FormatError("Damage audio closed during packet publication");
    }
    return result;
  }
  std::optional<Handle> impulse_at(std::uint32_t sound,Vec3 point,MultiplayerAudioKind kind,std::uint32_t unit){
    std::optional<Handle> result;
    if(enrolled){if(auto voice=audio.play_impulse_at_receipt(sound,point,1,kind,unit))result=Handle{voice->id(),*voice};}
    else if(auto voice=audio.play_impulse_at(sound,point,1,kind,unit))result=Handle{*voice,{}};
    if(!control->alive.load()){
      if(result)try{stop_voice(*result,true);}catch(...){}
      throw FormatError("Damage audio closed during packet publication");
    }
    return result;
  }
  std::vector<Handle> loop(std::uint32_t sound,std::uint32_t unit,unsigned view){
    std::vector<Handle> out;
    if(enrolled){
      const auto voices=audio.interface_loop_receipts(sound,unit,view);
      try{
        require();
        out.reserve(voices.size());
        for(const auto& voice:voices)out.push_back({voice.id(),voice});
      }catch(...){
        const auto failure=std::current_exception();
        for(const auto& voice:voices)try{audio.stop_voice(voice);}catch(...){}
        std::rethrow_exception(failure);
      }
    }else{
      const auto voices=audio.interface_loop(sound,unit,view);
      try{
        require();
        out.reserve(voices.size());
        for(const auto voice:voices)out.push_back({voice,{}});
      }catch(...){
        const auto failure=std::current_exception();
        for(const auto voice:voices)try{audio.stop_voice(voice);}catch(...){}
        std::rethrow_exception(failure);
      }
    }
    return out;
  }
  struct Reference {std::uint32_t group{},tag=none;};
  struct HudEntry {Reference normal,elite;std::uint32_t mask{};float control{};};
  std::vector<HudEntry> hud_entries;
  struct Hud {std::uint32_t unit=none;float previous=-1,trend{};std::uint64_t stamp{};bool elite{};
    std::vector<std::vector<Handle>> voices;std::vector<bool> playing;};
  std::map<unsigned,Hud> hud;
  struct Voice {DamageVoiceState state;std::optional<Handle> voice;
    std::optional<std::uint64_t> end;};
  std::map<std::uint32_t,Voice> voices;
  struct Definition {float first{},second{};std::array<std::uint32_t,12> sounds;};
  std::map<std::uint32_t,Definition> definitions;
  std::uint64_t tick{};bool started{};
  Impl(const Cache &c,MultiplayerAudio &a,std::uint32_t &r):cache(c),audio(a),random(r) {
    const auto tag=c.query_tag({.group=TagGroup::from_string("hudg"),.require_address=true});
    if(!tag)throw FormatError("Shield audio requires authored HUD globals");
    const auto metadata=c.load_source_tag_metadata(tag->datum);const auto b=root(metadata);
    const auto entries=block(metadata,b,0x418,24);
    if(b.u32(0x418)>16)throw FormatError("HUD sound bank exceeds original playing mask");
    for(unsigned n=0;n<b.u32(0x418);++n) {
      const unsigned p=n*24;
      hud_entries.push_back({{entries.u32(p),entries.u32(p+4)},
          {entries.u32(p+16),entries.u32(p+20)},entries.u32(p+8),entries.f32(p+12)});
    }
  }
  const Definition &definition(const BipedDefinition &biped) {
    if(const auto at=definitions.find(biped.datum);at!=definitions.end())return at->second;
    const auto metadata=cache.load_source_tag_metadata(biped.datum);const auto b=root(metadata);
    Definition d;d.first=b.f32(0x104);d.second=b.f32(0x10c);d.sounds.fill(none);
    const auto model=cache.load_source_tag_metadata(b.u32(0x38));const auto m=root(model);
    //114EC0: native multiplayer unit births supply variant-name0, retaining
    //hlmt+90 unless an actual name0 variant overrides it.
    auto dialogue=m.u32(0x90);const auto variants=block(model,m,0x50,0x38);
    for(unsigned n=0;n<m.u32(0x50);++n) {
      const unsigned at=n*0x38;const auto candidate=variants.u32(at+0x34);
      if(candidate!=none && (variants.u32(at)==0 || dialogue==none))dialogue=candidate;
      if(candidate!=none && variants.u32(at)==0)break;
    }
    if(dialogue!=none) {
      const auto udlg=cache.load_source_tag_metadata(dialogue);const auto u=root(udlg);
      const auto sounds=block(udlg,u,0xc,16);
      const auto globals=cache.query_tag({.group=TagGroup::from_string("matg"),.require_address=true});
      if(!globals)throw FormatError("Damage voice requires original globals");
      const auto gm=cache.load_source_tag_metadata(globals->datum);const auto g=root(gm);
      const auto ai=block(gm,g,0xc8,0x88);
      if(!g.u32(0xc8))throw FormatError("Damage voice requires original dialogue selector");
      const auto adlg=cache.load_source_tag_metadata(ai.u32(0x64));const auto a=root(adlg);
      const auto lookup=block(adlg,a,0x24,4);
      for(unsigned n=0;n<d.sounds.size() && n<a.u32(0x24);++n) {
        const auto index=std::int16_t(lookup.u16(n*4));
        if(index>=0 && unsigned(index)<u.u32(0xc) && sounds.u32(unsigned(index)*16+8)==0x736e6421u)
          d.sounds[n]=sounds.u32(unsigned(index)*16+12);
      }
    }
    return definitions.emplace(biped.datum,std::move(d)).first->second;
  }
};
NativeUnitDamageAudio::NativeUnitDamageAudio(const Cache &c,MultiplayerAudio &a,std::uint32_t &r)
    :impl_(std::make_shared<Impl>(c,a,r)){}
NativeUnitDamageAudio::~NativeUnitDamageAudio() {
  const auto owner=std::move(impl_);
  if(owner->enrolled && owner->thread!=std::this_thread::get_id())std::terminate();
  owner->control->alive.store(false);
  for(auto &[view,h]:owner->hud)for(auto &row:h.voices)for(const auto& id:row)
    try{owner->stop_voice(id,true);}catch(...){}
  for(auto &[unit,v]:owner->voices)if(v.voice)try{owner->stop_voice(*v.voice,true);}catch(...){}
}
void NativeUnitDamageAudio::advance(std::uint64_t tick) {
  const auto owner=impl_;auto &i=*owner;Impl::Operation operation(i);
  if(i.started && tick<i.tick)throw FormatError("Damage audio clock reversed");
  const auto elapsed=i.started?tick-i.tick:0;i.started=true;i.tick=tick;
  for(auto &[unit,v]:i.voices) {
    for(std::uint64_t n=0;n<elapsed;++n)advance_damage_voice_state(v.state);
    if(v.voice && !i.voice_active(*v.voice)) {
      v.voice.reset();v.end=tick+8; //114C60 retains request.delay after sound completion.
    }
    if(v.end && tick>=*v.end){v.state.priority=0;v.end.reset();}
  }
  std::erase_if(i.voices,[](const auto &entry) {
    const auto &v=entry.second;
    return !v.voice && !v.end && !v.state.priority && !v.state.count &&
        !v.state.cooldown && !v.state.outside_cooldown;
  });
}
void NativeUnitDamageAudio::hud(unsigned view,std::uint32_t unit,bool dead,float health,
    float shield,float maximum,bool charging,bool elite) {
  const auto owner=impl_;auto &i=*owner;Impl::Operation operation(i);auto [at,inserted]=i.hud.try_emplace(view);auto &h=at->second;
  // A prepared frontend world can publish NONE before player birth. The
  // default handle also equals NONE, so handle equality cannot prove that
  // this view's authored sound slots have been initialized.
  if(inserted || h.unit!=unit || h.elite!=elite) {
    for(auto &row:h.voices)for(const auto& id:row)i.stop_voice(id);
    h={};h.unit=unit;h.elite=elite;h.voices.resize(i.hud_entries.size());h.playing.resize(i.hud_entries.size());
  }
  const auto mask=shield_sound_mask(h.previous,health,shield,maximum,dead,charging,unit!=none);
  for(unsigned n=0;n<i.hud_entries.size();++n) {
    const auto &entry=i.hud_entries[n];const auto &ref=elite?entry.elite:entry.normal;
    auto &voices=h.voices[n];
    if(entry.mask&mask) {
      const bool active=std::any_of(voices.begin(),voices.end(),[&](const auto& id){return i.voice_active(id);});
      if(ref.group==0x736e6421u && (!active || !h.playing[n])) {
        for(const auto& id:voices)i.stop_voice(id);
        voices.clear();
        voices.reserve(1);
        if(const auto id=i.impulse(ref.tag,entry.control,MultiplayerAudioKind::Shield,unit,view))voices.push_back(*id);
      }else if(ref.group==0x6c736e64u && voices.empty() && ref.tag!=none)
        voices=i.loop(ref.tag,unit,view);
      h.playing[n]=true;
    }else {
      if(ref.group==0x6c736e64u)for(const auto& id:voices)i.stop_voice(id);
      voices.clear();h.playing[n]=false;
    }
  }
  if(dead){h.previous=-1;h.trend=0;h.stamp=i.tick;}
  else if(h.previous<=shield) {
    const auto old=h.previous;h.previous=shield;
    if(old<shield)h.trend=-1;
    else if(h.trend>0)h.trend+=float(i.tick-h.stamp)*(1.f/30.f);
    h.stamp=i.tick;
  }else {
    //13A838 retains the pre-hit shield display for .5 seconds.13A720
    //runs first, so the hit sound sees that same retained value.
    if(h.trend<0 || h.trend>1)h.stamp=i.tick;
    const float elapsed=float(i.tick-h.stamp)*(1.f/30.f);
    if(elapsed<.5f)h.trend=0;
    else {h.previous=shield;h.trend+=elapsed;h.stamp=i.tick;}
  }
}
void NativeUnitDamageAudio::damage(const BipedDefinition &biped,const UnitDamageVoiceEvent &e) {
  const auto owner=impl_;auto &i=*owner;Impl::Operation operation(i);const auto &d=i.definition(biped);auto &v=i.voices[e.target];
  int type=0;bool special=false;
  if(e.definition!=none) {
    const auto tag=i.cache.load_source_tag_metadata(e.definition);const auto b=root(tag);
    type=std::int16_t(b.u16(0x12));special=b.u32(0x14)&0x80u;
  }
  const auto selected=select_player_damage_voice(v.state,e,type,special,d.first,d.second,i.random);
  if(!selected || d.sounds.at(*selected)==none)return;
  if(v.voice)i.stop_voice(*v.voice);
  v.voice=i.impulse_at(d.sounds[*selected],e.position,MultiplayerAudioKind::DamageVoice,e.target);
  if(!v.voice)return;
  v.end.reset();v.state.priority=(e.report_flags&1u)?15:12;
  const bool outside=!(e.report_flags&1u) && e.visible_body>d.second;
  if(outside)v.state.outside_cooldown=60;
  else {v.state.cooldown=15;v.state.decay=45;if(++v.state.count==3)v.state.cooldown=60;}
}
void NativeUnitDamageAudio::remove(std::uint32_t unit) {
  const auto owner=impl_;auto &i=*owner;Impl::Operation operation(i);const auto found=i.voices.find(unit);
  if(found!=i.voices.end()){if(found->second.voice)i.stop_voice(*found->second.voice);i.voices.erase(found);}
}

struct NativeUnitDamageAudio::ContinuationSnapshot {
  struct Image {
    std::vector<Impl::HudEntry> hud_entries;
    std::map<unsigned,Impl::Hud> hud;
    std::map<std::uint32_t,Impl::Voice> voices;
    std::map<std::uint32_t,Impl::Definition> definitions;
    std::uint64_t tick{};bool started{};
  } image;
  std::weak_ptr<Impl::Control> owner;
  const Impl* identity{};
  const Cache* cache{};
  const MultiplayerAudio* audio{};
  const std::uint32_t* auxiliary_binding{};
  std::uint32_t auxiliary_at_capture{};
};
struct NativeUnitDamageAudio::ContinuationPrepared {
  ContinuationSnapshot::Image image;
  std::weak_ptr<Impl::Control> owner;
  const Impl* identity{};
  std::uint64_t revision{};
  MultiplayerAudio::PreparationIdentity audio_identity;
  std::vector<std::pair<MultiplayerAudio::VoiceReceipt,MultiplayerAudio::VoiceReceipt>> mappings;
  const std::uint32_t* auxiliary_binding{};
  std::uint32_t auxiliary_at_capture{};
  bool consumed{};
};

void NativeUnitDamageAudio::enroll_continuation(){
  auto& i=*impl_;
  if(i.thread!=std::this_thread::get_id() || i.active)
    throw FormatError("Damage audio enrollment requires quiescent creator");
  if(i.enrolled)return;
  if(!i.audio.continuation_enrolled())throw FormatError("Damage audio requires enrolled Audio");
  for(const auto& [view,h]:i.hud)for(const auto& row:h.voices)
    if(!row.empty())throw FormatError("Damage audio enrollment requires no retained handles");
  for(const auto& [unit,v]:i.voices)if(v.voice)
    throw FormatError("Damage audio enrollment requires no retained handles");
  i.change();i.enrolled=true;
}

NativeUnitDamageAudio::Checkpoint NativeUnitDamageAudio::checkpoint() const {
  auto& i=*impl_;i.require();
  if(!i.enrolled || i.active || i.revision==std::numeric_limits<std::uint64_t>::max())
    throw FormatError("Damage audio checkpoint requires quiescent enrolled creator");
  auto data=std::make_shared<ContinuationSnapshot>();
  data->owner=i.control;data->identity=&i;data->cache=&i.cache;data->audio=&i.audio;
  data->auxiliary_binding=&i.random;data->auxiliary_at_capture=i.random;
  data->image={i.hud_entries,i.hud,i.voices,i.definitions,i.tick,i.started};
  const auto classify=[&](Impl::Handle& handle){
    if(!handle.receipt)throw FormatError("Damage audio checkpoint contains legacy handle");
    // Historical stale handles stay stale; never refresh a reused numeric ID.
    handle.expired=handle.expired || !i.audio.voice_active(*handle.receipt);
  };
  for(auto& [view,h]:data->image.hud)for(auto& row:h.voices)for(auto& h:row)classify(h);
  for(auto& [unit,v]:data->image.voices)if(v.voice)classify(*v.voice);
  Checkpoint out;out.data_=std::move(data);return out;
}

NativeUnitDamageAudio::PreparedReplacement NativeUnitDamageAudio::prepare_restore(
    const Checkpoint& saved,const MultiplayerAudio::PreparedReplacement& audio) const {
  auto& i=*impl_;i.require();
  if(!i.enrolled || i.active || !saved.data_ || saved.data_->owner.lock()!=i.control ||
     saved.data_->identity!=&i || saved.data_->cache!=&i.cache || saved.data_->audio!=&i.audio ||
     saved.data_->auxiliary_binding!=&i.random || !audio.for_audio(i.audio) ||
     !audio.auxiliary_agrees(saved.data_->auxiliary_binding,saved.data_->auxiliary_at_capture) ||
     i.revision==std::numeric_limits<std::uint64_t>::max())
    throw FormatError("Foreign or stale damage audio preparation");
  auto data=std::make_unique<ContinuationPrepared>();
  data->owner=i.control;data->identity=&i;data->revision=i.revision;
  data->audio_identity=audio.identity();
  data->image=saved.data_->image;data->auxiliary_binding=saved.data_->auxiliary_binding;
  data->auxiliary_at_capture=saved.data_->auxiliary_at_capture;
  const auto remap=[&](Impl::Handle& handle){
    if(!handle.receipt)throw FormatError("Damage audio replacement contains legacy handle");
    if(handle.expired)return;
    const auto target=audio.voice(*handle.receipt);
    if(!target)throw FormatError("Damage audio requires exact prospective Core voice");
    data->mappings.emplace_back(*handle.receipt,*target);
    handle.raw=target->id();handle.receipt=*target;
  };
  for(auto& [view,h]:data->image.hud)for(auto& row:h.voices)for(auto& h:row)remap(h);
  for(auto& [unit,v]:data->image.voices)if(v.voice)remap(*v.voice);
  PreparedReplacement out;out.data_=std::move(data);return out;
}
NativeUnitDamageAudio::PreparedReplacement::PreparedReplacement()=default;
NativeUnitDamageAudio::PreparedReplacement::~PreparedReplacement()=default;
NativeUnitDamageAudio::PreparedReplacement::PreparedReplacement(PreparedReplacement&&) noexcept=default;
NativeUnitDamageAudio::PreparedReplacement& NativeUnitDamageAudio::PreparedReplacement::operator=(PreparedReplacement&&) noexcept=default;

bool NativeUnitDamageAudio::ready(const PreparedReplacement& replacement,
    const MultiplayerAudio::PreparedReplacement& audio) const noexcept {
  const auto& i=*impl_;const auto* data=replacement.data_.get();
  if(i.thread!=std::this_thread::get_id())return false;
  if(!data || data->consumed || !i.enrolled || i.active ||
     data->owner.lock()!=i.control ||
     !i.control->alive.load() || data->identity!=&i || data->revision!=i.revision ||
     i.revision==std::numeric_limits<std::uint64_t>::max() ||
     data->auxiliary_binding!=&i.random || !audio.for_audio(i.audio) ||
     !audio.matches(data->audio_identity) ||
     !audio.auxiliary_agrees(data->auxiliary_binding,data->auxiliary_at_capture))return false;
  for(const auto& [source,target]:data->mappings){
    const auto mapped=audio.voice(source);
    if(!mapped || !mapped->same(target))return false;
  }
  return true;
}
bool NativeUnitDamageAudio::commit(PreparedReplacement& replacement,
    const MultiplayerAudio::PreparedReplacement& audio) noexcept {
  if(!ready(replacement,audio) || !audio.published())return false;
  auto& i=*impl_;auto& image=replacement.data_->image;
  i.hud_entries.swap(image.hud_entries);i.hud.swap(image.hud);
  i.voices.swap(image.voices);i.definitions.swap(image.definitions);
  std::swap(i.tick,image.tick);std::swap(i.started,image.started);
  i.change();replacement.data_->consumed=true;
  // The shared auxiliary word is an agreement witness, never restored here.
  // The enclosing effects owner is its sole writer. Outgoing handles retire
  // as plain values after all aggregate locks, without calling Audio.stop().
  return true;
}
}
