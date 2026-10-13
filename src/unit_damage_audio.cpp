#include "halo2/unit_damage_audio.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>
#include <cmath>
#include <set>

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
  struct Reference {std::uint32_t group{},tag=none;};
  struct HudEntry {Reference normal,elite;std::uint32_t mask{};float control{};};
  std::vector<HudEntry> hud_entries;
  struct Hud {std::uint32_t unit=none;float previous=-1,trend{};std::uint64_t stamp{};bool elite{};
    std::vector<std::vector<MultiplayerAudio::Voice>> voices;std::vector<bool> playing;};
  std::map<unsigned,Hud> hud;
  struct Voice {DamageVoiceState state;std::optional<MultiplayerAudio::Voice> voice;
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
    :impl_(std::make_unique<Impl>(c,a,r)){}
NativeUnitDamageAudio::~NativeUnitDamageAudio() {
  for(auto &[view,h]:impl_->hud)for(auto &row:h.voices)for(const auto id:row)
    try{impl_->audio.stop_voice(id);}catch(...){}
  for(auto &[unit,v]:impl_->voices)if(v.voice)try{impl_->audio.stop_voice(*v.voice);}catch(...){}
}
void NativeUnitDamageAudio::advance(std::uint64_t tick) {
  auto &i=*impl_;
  if(i.started && tick<i.tick)throw FormatError("Damage audio clock reversed");
  const auto elapsed=i.started?tick-i.tick:0;i.started=true;i.tick=tick;
  for(auto &[unit,v]:i.voices) {
    for(std::uint64_t n=0;n<elapsed;++n)advance_damage_voice_state(v.state);
    if(v.voice && !i.audio.voice_active(*v.voice)) {
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
  auto &i=*impl_;auto [at,inserted]=i.hud.try_emplace(view);auto &h=at->second;
  // A prepared frontend world can publish NONE before player birth. The
  // default handle also equals NONE, so handle equality cannot prove that
  // this view's authored sound slots have been initialized.
  if(inserted || h.unit!=unit || h.elite!=elite) {
    for(auto &row:h.voices)for(const auto id:row)i.audio.stop_voice(id);
    h={};h.unit=unit;h.elite=elite;h.voices.resize(i.hud_entries.size());h.playing.resize(i.hud_entries.size());
  }
  const auto mask=shield_sound_mask(h.previous,health,shield,maximum,dead,charging,unit!=none);
  for(unsigned n=0;n<i.hud_entries.size();++n) {
    const auto &entry=i.hud_entries[n];const auto &ref=elite?entry.elite:entry.normal;
    auto &voices=h.voices[n];
    if(entry.mask&mask) {
      const bool active=std::any_of(voices.begin(),voices.end(),[&](auto id){return i.audio.voice_active(id);});
      if(ref.group==0x736e6421u && (!active || !h.playing[n])) {
        for(const auto id:voices)i.audio.stop_voice(id);
        voices.clear();
        if(const auto id=i.audio.play_impulse(ref.tag,entry.control,MultiplayerAudioKind::Shield,unit,view))voices.push_back(*id);
      }else if(ref.group==0x6c736e64u && voices.empty() && ref.tag!=none)
        voices=i.audio.interface_loop(ref.tag,unit,view);
      h.playing[n]=true;
    }else {
      if(ref.group==0x6c736e64u)for(const auto id:voices)i.audio.stop_voice(id);
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
  auto &i=*impl_;const auto &d=i.definition(biped);auto &v=i.voices[e.target];
  int type=0;bool special=false;
  if(e.definition!=none) {
    const auto tag=i.cache.load_source_tag_metadata(e.definition);const auto b=root(tag);
    type=std::int16_t(b.u16(0x12));special=b.u32(0x14)&0x80u;
  }
  const auto selected=select_player_damage_voice(v.state,e,type,special,d.first,d.second,i.random);
  if(!selected || d.sounds.at(*selected)==none)return;
  if(v.voice)i.audio.stop_voice(*v.voice);
  v.voice=i.audio.play_impulse_at(d.sounds[*selected],e.position,1,MultiplayerAudioKind::DamageVoice,e.target);
  if(!v.voice)return;
  v.end.reset();v.state.priority=(e.report_flags&1u)?15:12;
  const bool outside=!(e.report_flags&1u) && e.visible_body>d.second;
  if(outside)v.state.outside_cooldown=60;
  else {v.state.cooldown=15;v.state.decay=45;if(++v.state.count==3)v.state.cooldown=60;}
}
void NativeUnitDamageAudio::remove(std::uint32_t unit) {
  auto &i=*impl_;const auto found=i.voices.find(unit);
  if(found!=i.voices.end()){if(found->second.voice)i.audio.stop_voice(*found->second.voice);i.voices.erase(found);}
}
}
