#include "halo2/effect_sound.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>
#include <bit>
#include <cmath>

namespace halo2 {
namespace {
std::vector<EffectSoundCue> sound_parts(const Cache& cache,std::uint32_t datum,
    float a,float b,bool fixed){
  const auto tag=cache.load_source_tag_metadata(datum);
  if(tag.group()!="effe")throw FormatError("Effect sound requires authored effe definition");
  const Bytes root(tag.bytes_at(tag.source_address(),0x1c));
  if(fixed && std::bit_cast<std::int16_t>(root.u16(4))!=-1)
    throw FormatError("Effect sound requires an original looping event owner");
  const auto count=root.u32(0x14),locations=root.u32(0xc);
  if(count>65536 || locations>16)throw FormatError("Effect sound event/location limit exceeded");
  // Stock effects share literal event/part banks with other resident tags.
  // Original17BDF0 follows those pointers; the effect's own extent is not
  // their storage owner (plasma lower/brute firing parts reside in upper).
  std::map<std::uint32_t,SourceTagMetadata> owned;
  const auto read=[&](std::uint32_t address,std::size_t extent)->std::span<const std::byte> {
    if(!extent)return {};
    const Tag *owner=nullptr;
    for(const auto &entry:cache.select_tags({.contains=TagAddressRange{address,extent},
        .require_address=true,.minimum_size=1})) {
      if(owner)throw FormatError("Ambiguous effect sound metadata backing");
      owner=&entry;
    }
    if(!owner)throw FormatError("Effect sound pointer escapes actual resident tag payloads");
    auto found=owned.find(owner->datum);
    if(found==owned.end())found=owned.emplace(owner->datum,cache.load_source_tag_metadata(owner->datum)).first;
    return found->second.bytes_at(address,extent);
  };
  const Bytes events(read(root.u32(0x18),std::size_t(count)*0x38));
  std::vector<EffectSoundCue> result;float elapsed{};
  for(unsigned i=0;i<count;++i){
    const auto p=std::size_t(i)*0x38;
    if(fixed && (events.f32(p+4)>0 || events.f32(p+8)!=events.f32(p+12) ||
        events.f32(p+16)!=events.f32(p+20)))
      throw FormatError("Effect sound requires original random skip/delay/duration selection");
    const float delay=events.f32(p+8),duration=events.f32(p+16);
    if(!std::isfinite(delay) || !std::isfinite(duration) || delay<0 || duration<0)
      throw FormatError("Invalid authored effect sound timeline");
    elapsed+=delay;
    const auto parts=events.u32(p+0x18);
    if(parts>65536)throw FormatError("Effect sound part limit exceeded");
    if(parts){const Bytes rows(read(events.u32(p+0x1c),std::size_t(parts)*0x38));
      for(unsigned j=0;j<parts;++j){const auto q=std::size_t(j)*0x38;
        if(rows.u32(q+8)!=0x736e6421u || rows.u32(q+0x10)==0xffffffffu)continue;
        const auto location=std::bit_cast<std::int16_t>(rows.u16(q+4));
        if(location<0 || unsigned(location)>=locations)continue;
        // The current prop response is the dry environment branch. Original
        //17BDF0 excludes environment2 there. It does not choose by tag name.
        if(rows.u16(q+2)==2)continue;
        if(rows.u16(q+2)>2)throw FormatError("Invalid authored effect sound environment");
        if(rows.u16(q+6)&2)throw FormatError("Effect sound requires original randomized location");
        float gain=1;
        if(rows.u32(q+0x30)&0x20)gain=a;
        if(rows.u32(q+0x34)&0x20)gain=b*gain;
        result.push_back({fixed?elapsed:0.f,gain,rows.u32(q+0x10),i,
            bool(rows.u32(q+0x30)&0x20),bool(rows.u32(q+0x34)&0x20)});
      }
    }
    elapsed+=duration;
  }
  return result;
}
}
std::vector<EffectSoundCue> fixed_effect_sound_cues(const Cache& cache,std::uint32_t datum,float a,float b){
  return sound_parts(cache,datum,a,b,true);
}
std::vector<EffectSoundCue> authored_effect_sound_parts(const Cache& cache,std::uint32_t datum){
  return sound_parts(cache,datum,1,1,false);
}
void NativeEffectSounds::spawn(std::uint32_t effect,std::uint32_t owner,Vec3 position,float a,float b){
  if(effect==0xffffffffu)return;
  auto definition=definitions_.find(effect);
  if(definition==definitions_.end()) {
    auto cues=fixed_effect_sound_cues(cache_,effect);
    const auto tag=cache_.load_source_tag_metadata(effect);
    const Bytes root(tag.bytes_at(tag.source_address(),4));
    //177A10 normally supplies0. Its literal1 branch additionally requires
    //definition bit1 and two actual runtime flags. The current detached
    //callback bank does not export those flags. A time-zero-only sound bank
    //has identical audible output on either branch; later cues need the
    //actual creation owner. In particular E70A0595 flags1C select0.
    if((root.u32(0)&2) && std::any_of(cues.begin(),cues.end(),[](const auto &cue){return cue.elapsed>0;}))
      throw FormatError("Effect sound initial advance requires actual creation flags");
    definition=definitions_.emplace(effect,std::move(cues)).first;
  }
  for(auto cue:definition->second){
    cue.gain=cue.scale_a?a:1.f;
    if(cue.scale_b)cue.gain=b*cue.gain;
    pending_.push_back({cue,owner,position});
  }
}
void NativeEffectSounds::advance(float seconds,const Submit& submit){
  if(!std::isfinite(seconds) || seconds<0)throw FormatError("Invalid native effect sound elapsed");
  for(auto i=pending_.begin();i!=pending_.end();){
    i->cue.elapsed-=seconds;
    if(i->cue.elapsed<=0){submit(i->cue,i->owner,i->position);i=pending_.erase(i);}
    else ++i;
  }
}
}
