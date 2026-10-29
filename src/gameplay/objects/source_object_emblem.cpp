#include "halo2/source_object_emblem.hpp"
#include <algorithm>
#include <bit>

namespace halo2 {
namespace {
Vec3 palette_color(std::int8_t index,std::span<const std::array<std::uint32_t,3>> palette) {
  return index>=0 && std::size_t(index)<palette.size()
      ?std::bit_cast<Vec3>(palette[unsigned(index)]):Vec3{1,1,1};
}
}
SourceObjectEmblem source_player_object_emblem(const SourcePlayerConfiguration &profile) {
  SourceObjectEmblem emblem;
  std::copy_n(profile.begin()+0x45,emblem.size(),emblem.begin());
  return emblem;
}
std::optional<FunctionValue> source_object_emblem_input(const SourceObjectEmblem &emblem,
    std::optional<std::int16_t> scenario_type,std::uint32_t name) {
  float value;
  switch(name) {
  case 0x0d0005ff: value=1.f;break;
  case 0x170005fa: value=std::min(float(emblem[0])*.015625f,1.f);break;
  case 0x170005fb: value=std::min(float(emblem[1])*.03125f,1.f);break;
  case 0x180005fc: value=(emblem[2]&1)?0.f:1.f;break;
  case 0x1d0005fd: value=(emblem[2]&2)?1.f:0.f;break;
  case 0x1d0005fe: value=(emblem[2]&4)?1.f:0.f;break;
  default:return {};
  }
  if(scenario_type==0 && !(emblem[2]&8))return FunctionValue{0.f,false};
  return FunctionValue{value,true};
}
std::array<Vec3,2> source_player_emblem_colors(const SourcePlayerConfiguration &profile,
    std::span<const std::array<std::uint32_t,3>> palette) {
  std::array<Vec3,2> colors;
  for(unsigned i=0;i<colors.size();++i) {
    const auto index=std::bit_cast<std::int8_t>(profile[0x42+i]);
    colors[i]=palette_color(index,palette);
  }
  return colors;
}
ObjectColors source_player_appearance_colors(const SourcePlayerConfiguration &profile,
    std::int16_t team,bool neutral,std::span<const std::array<std::uint32_t,3>> palette,
    std::span<const std::array<std::uint32_t,3>> teams) {
  ObjectColors colors;
  for(unsigned i=0;i<colors.size();++i)
    colors[i]=palette_color(std::bit_cast<std::int8_t>(profile[0x40+i]),palette);
  if(neutral)colors[0]=colors[1]={1,1,1};
  else if(team!=-1) {
    colors[1]=colors[0];
    // Literal [474C94] fallback in original83050, shared with retail7F720.
    colors[0]=team>=0 && std::size_t(team)<teams.size()
        ?std::bit_cast<Vec3>(teams[unsigned(team)]):Vec3{1,.41f,.7f};
  }
  return colors;
}
}
