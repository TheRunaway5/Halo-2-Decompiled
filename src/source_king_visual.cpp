#include "halo2/source_king_visual.hpp"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace halo2 {
namespace {
constexpr SourceKingColor neutral{0x3f000000,0x3f000000,0x3f000000};
constexpr SourceKingColor view_default{0x3eeaeaeb,0x3f3ababb,0x3f800000};
constexpr SourceKingColor view_alternate{0x3f4ececf,0x3f0f8f90,0x3f5ededf};
constexpr SourceKingColor team_fallback{0x3f800000,0x3ed1eb85,0x3f333333};
const SourceKingVisualPlayer& raw_player(const SourceKingColorInput& in,std::uint32_t datum) {
  const auto slot=datum&65535u;
  if(slot>=in.players.size())throw std::invalid_argument("King color requires an available original raw player slot");
  return in.players[slot];
}
SourceKingColor view_color(const SourceKingColorInput& in) {
  if(in.render_view==-1 || in.local_player==0xffffffffu)return view_default;
  const auto kind=raw_player(in,in.local_player).appearance_type;
  return kind==1 || kind==3 ? view_alternate : view_default;
}
}
SourceKingColor source_king_hill_color(const SourceKingColorInput& in,
    std::span<const SourceKingColor> teams) {
  if(in.query==0xffffffffu)return view_color(in);
  const auto membership=raw_player(in,in.query).membership;
  bool same=false,different=false;
  std::int8_t last{};
  for(unsigned slot=0;slot<in.players.size();++slot) {
    const auto bit=std::uint16_t(1u<<slot);
    const auto& player=in.players[slot];
    if(!(in.occupied_players&bit) || player.current_unit==0xffffffffu || !(in.hill_occupants&bit))continue;
    last=player.membership;
    if(last==membership)same=true;else different=true;
  }
  if(same==different)return neutral;
  if(!in.engine_present || !in.team_mode)return view_color(in);
  if(last>=0 && std::size_t(last)<teams.size())return teams[std::size_t(last)];
  return team_fallback;
}
bool source_king_hill_marker(const SourceKingState& state,std::int32_t marker_view,
    std::uint32_t marker_player,const SourceKingColorInput& color_input,
    std::span<const SourceKingColor> teams,const SourceKingPosition& position,
    std::span<std::byte,0x90> storage) {
  if(state.marker_count<4 || (state.marker_count&1))return false;
  float fade=1;
  if(marker_view!=-1 && marker_player!=0xffffffffu) {
    const auto unit=raw_player(color_input,marker_player).current_unit;
    if(unit!=0xffffffffu) {
      if(!position)throw std::invalid_argument("King marker requires the actual current-unit position owner");
      const auto xyz=position(unit);
      const double dx=double(state.center[0])-double(xyz[0]);
      const double dy=double(state.center[1])-double(xyz[1]);
      const double value=std::sqrt(dx*dx+dy*dy)/double(state.radius)-1.;
      fade=value<0 ? 0.f : float(value);
      if(fade>1)fade=1;
    }
  }
  auto query=color_input;query.query=marker_view==-1 ? 0xffffffffu : marker_player;
  const auto rgb=source_king_hill_color(query,teams);
  const auto word=[&](unsigned at,std::uint32_t value) {
    for(unsigned i=0;i<4;++i)storage[at+i]=std::byte((value>>(8*i))&255u);
  };
  storage[0]=std::byte{1};storage[1]=std::byte{0};word(4,1);
  for(unsigned axis=0;axis<3;++axis)word(8+axis*4,std::bit_cast<std::uint32_t>(state.center[axis]));
  word(0x14,0);word(0x18,0x3dcccccd);word(0x1c,0);word(0x20,0xffffffffu);
  for(unsigned at:{0x24u,0x30u,0x4cu,0x58u})for(unsigned axis=0;axis<3;++axis)word(at+axis*4,rgb[axis]);
  word(0x3c,std::bit_cast<std::uint32_t>(fade));word(0x40,0x3f800000);
  word(0x44,1);word(0x48,6);word(0x64,0x3f800000);word(0x68,0xffffffffu);return true;
}
} // namespace halo2
