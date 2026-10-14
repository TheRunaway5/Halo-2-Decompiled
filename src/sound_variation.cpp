#include "halo2/sound_variation.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>

namespace halo2 {
namespace {
float scalar() {return std::bit_cast<float>(0x37800080u);} // original [4650C0]
std::int8_t signed_byte(std::byte b) {return std::bit_cast<std::int8_t>(b);}
std::int16_t signed_short(const Bytes &b,std::size_t p) {return std::bit_cast<std::int16_t>(b.u16(p));}
}
SoundVariationDefinition Cache::load_sound_variation(std::uint32_t datum,unsigned pitch) const {
  const Bytes data(cache_bytes());
  auto find=[&](const std::string &group,std::optional<std::uint32_t> id={}) -> const Tag & {
    const auto t=query_tag({.handle=id ? std::optional<TagHandle>{TagHandle{*id}} : std::nullopt, .group=TagGroup::from_string(group), .require_address=true});
    if(!t)throw FormatError("Missing original sound variation metadata: "+group);
    return *t;
  };
  const auto &s=find("snd!",datum), &g=mcc_?mcc_sound_gestalt(datum):find("ugh!");
  const auto so=metadata_offset(s.address,20),go=metadata_offset(g.address,mcc_?0x78:0x48);
  const auto &c=find("sncl",mcc_?std::optional<std::uint32_t>{mcc_sound_class_tag((data.u16(so)&0x800)!=0)}:std::nullopt);
  const auto co=metadata_offset(c.address,8),tables=go+(mcc_?8:0);
  auto entry=[&](std::size_t header,std::size_t stride,std::size_t index) {
    const auto count=data.u32(header);
    if(index>=count || count>0x100000)throw FormatError("Sound variation table index outside resident metadata");
    return metadata_offset(data.u32(header+4),std::size_t(count)*stride)+index*stride;
  };
  SoundVariationDefinition out;out.datum=datum;
  auto copy=[&](auto &target,std::size_t offset) {
    const auto input=data.slice(offset,target.size());std::copy(input.begin(),input.end(),target.begin());
  };
  copy(out.sound,so);
  const Bytes sound(out.sound);
  const auto cls=signed_byte(out.sound[2]);
  if(cls<0 || pitch>=std::to_integer<unsigned>(out.sound[10]))throw FormatError("Invalid sound class/pitch range");
  copy(out.sound_class,entry(co,mcc_?0x78:0x5c,unsigned(cls)));
  const auto playback=signed_short(sound,6),first_range=signed_short(sound,8);
  if(playback<0 || first_range<0)throw FormatError("Invalid signed sound variation index");
  copy(out.playback,entry(tables,0x38,unsigned(playback)));
  const auto scale=signed_byte(out.sound[11]);
  if(scale<0)throw FormatError("Invalid signed sound scale index");
  copy(out.scale,entry(tables+8,20,unsigned(scale)));
  for(unsigned n=0;n<std::to_integer<unsigned>(out.sound[10]);++n){
    const auto row=entry(tables+0x20,12,unsigned(first_range)+n);
    const auto interval=signed_short(data,row+2);
    if(interval<0)throw FormatError("Invalid signed sound pitch interval index");
    auto &r=out.pitch_ranges.emplace_back();
    copy(r.interval,entry(tables+0x18,10,unsigned(interval)));
    r.permutations=signed_short(data,row+10);
  }
  copy(out.range,entry(tables+0x20,12,unsigned(first_range)+pitch));
  const Bytes range(out.range);const auto first=signed_short(range,8),count=signed_short(range,10);
  // The recovered helper copies into a 32-bit local and uses x86 SHL masks.
  // Only the bounded 1..31 domain is supported here; larger/invalid ranges
  // are unresolved instead of inventing a different bitset algorithm.
  if(first<0 || count<1 || count>31)throw FormatError("Unrecovered sound permutation count");
  out.permutations.resize(unsigned(count));
  for(unsigned n=0;n<out.permutations.size();++n)copy(out.permutations[n],entry(tables+0x28,16,unsigned(first)+n));
  if(count>1 && !(std::to_integer<unsigned>(out.sound_class[10])&0x20)) {
    const auto bytes=unsigned(count+7)/8,offset=unsigned(range.u16(6))>>3;
    const auto flags=go+(mcc_?0x58:0x38);
    if(offset+bytes>data.u32(flags))throw FormatError("Sound variation used bank outside resident metadata");
    const auto bank=metadata_offset(data.u32(flags+4),data.u32(flags));
    const auto raw=data.slice(bank+offset,bytes);out.used.assign(raw.begin(),raw.end());
  }
  return out;
}
std::uint16_t SoundLocalRandom::advance(std::uint32_t instruction) {
  const auto before=seed;seed=seed*0x0019660du+0x3c6ef35fu;
  stores.push_back({instruction,before,seed});return std::uint16_t(seed>>16);
}
SoundRangeState imported_sound_range_state(const SoundVariationDefinition &d) {
  SoundRangeState state{signed_byte(d.range[4]),signed_byte(d.range[5]),0};
  if(d.used.size()>4)throw FormatError("Unrecovered used-bank extent");
  for(unsigned i=0;i<d.used.size();++i)state.used|=std::to_integer<std::uint32_t>(d.used[i])<<(8*i);
  return state;
}
float source_sound_gain(const SoundVariationDefinition &d,SoundLocalRandom &random) {
  const Bytes playback(d.playback);
  const double scaled=double(random.advance(0x1269d8))*double(scalar());
  const float draw=float(scaled),zero=0.0f;
  const float variance=playback.f32(0x14)-zero;
  const float product=variance*draw;
  const float offset=product+zero;
  return offset+playback.f32(0x10);
}
double source_sound_pitch(const SoundVariationDefinition &d,SoundLocalRandom &random) {
  const Bytes playback(d.playback);
  const double lower=signed_short(playback,0x18),upper=signed_short(playback,0x1a);
  const double draw=double(random.advance(0x126a7a))*double(scalar());
  const double difference=upper-lower,product=difference*draw;
  return lower+product;
}
float source_sound_control_pitch(const SoundVariationDefinition &d,float base,float control) {
  const Bytes s(d.scale);
  const float lower=signed_short(s,8),upper=signed_short(s,10);
  const float difference=upper-lower,scaled=difference*control,offset=scaled+base;
  return offset+lower;
}
float source_sound_control_gain(const SoundVariationDefinition &d,float control) {
  const Bytes s(d.scale),c(d.sound_class);
  const float lower=s.f32(0)+c.f32(0x20),upper=s.f32(4)+c.f32(0x24);
  const auto linear=[](float db){return std::exp(double(std::clamp(db,-64.f,0.f))*double(.05f)*double(2.3025851f));};
  const float low=float(linear(lower));
  const float selected=float((linear(upper)-double(low))*double(control)+double(low));
  return selected>0?std::clamp(float(20.0*std::log10(double(std::min(selected,1.f)))),-64.f,0.f):-64.f;
}
std::int16_t source_sound_pitch_range(std::span<const SoundPitchRange> ranges,
                                     std::int16_t previous,float pitch) {
  const auto clamp=[&](const SoundPitchRange &r){const Bytes b(r.interval);
    const float lower=signed_short(b,6),upper=signed_short(b,8);
    if(lower>pitch)return lower;
    if(pitch>upper)return upper;
    return pitch;
  };
  if(previous>=0 && std::size_t(previous)<ranges.size()){
    const auto &r=ranges[unsigned(previous)];
    if(clamp(r)==pitch && r.permutations>0)return previous;
  }
  float best=std::numeric_limits<float>::max();std::int16_t selected=-1;
  for(unsigned n=0;n<ranges.size();++n){const auto &r=ranges[n];
    if(r.permutations<=0)continue;
    if(clamp(r)==pitch)return std::int16_t(n);
    const Bytes b(r.interval);const float center=signed_short(b,4);
    const float delta=pitch>center?pitch-center:float(signed_short(b,2))-pitch;
    if(best>delta){selected=std::int16_t(n);best=delta;}
  }
  return selected;
}
std::int16_t source_sound_permutation(const SoundVariationDefinition &d,SoundRangeState &state,SoundLocalRandom &random) {
  const Bytes range(d.range);const int count=signed_short(range,10);
  if(count<1 || count>31 || d.permutations.size()!=unsigned(count))throw FormatError("Unrecovered sound permutation domain");
  const bool persistent=!(std::to_integer<unsigned>(d.sound_class[10])&0x20);
  if(persistent && state.forced!=-1) {
    const auto selected=state.forced;state.forced=-1;state.last=selected;return selected;
  }
  auto used=persistent && count>1?state.used:0u;
  int last=persistent?state.last:-1,retries=0,selected;
  const auto mask=(1u<<unsigned(count))-1;
  for(;;) {
    selected=(unsigned(random.advance(0x21b281))*unsigned(count))>>16;
    if(((~used)&mask)==0) {
      used=0;
      if(count>1 && last!=-1)used=1u<<(unsigned(last)&31);
      last=-1;
    }
    const auto bit=1u<<(unsigned(selected)&31);
    if(used&bit)continue;
    used|=bit;
    const Bytes permutation(d.permutations.at(unsigned(selected)));
    const float skip=float(permutation.u16(2))*scalar();
    const double draw=double(random.advance(0x21b349))*double(scalar());
    if(draw>=double(skip) || ++retries==count*2)break;
  }
  if(persistent) {
    if(count>1) {
      const auto bytes=unsigned(count+7)/8;
      const auto write_mask=bytes==4?0xffffffffu:((1u<<(bytes*8))-1);
      state.used=(state.used&~write_mask)|(used&write_mask);
    }
    state.last=std::int8_t(selected);
  }
  return std::int16_t(selected);
}
} // namespace halo2
