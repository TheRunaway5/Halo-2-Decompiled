#pragma once
#include "halo2/source_live_model_geometry_tu15.hpp"
#include "halo2/source_transparent_sort.hpp"
#include <algorithm>
#include <cstring>
#include <optional>

namespace halo2 {
// A complete existing forward-only, single-part model draw. Its material,
// interpolated geometry, lighting/colors and function owner move together.
struct SourceModelTransparentDraw {
  std::uint32_t object{}, definition{}, section{}, part{}, shader{};
  std::size_t draw{};
};
// Returns an ordering of these existing draw slots, or no ordering when the
// complete queue cannot establish a unique ordinary main-unit/part join.
// This does not execute callbacks, bind resources or order other draw owners.
inline std::optional<std::vector<std::size_t>> source_model_transparent_order_tu15(
    std::span<const SourceTransparentCommand> commands,
    std::span<const SourceLiveModelBaseDrawTu15> units,
    std::span<const SourceModelTransparentDraw> draws, Vec3 eye) {
  if(commands.size()>1024 || units.size()>850 || draws.size()>850)return {};
  std::array<int,850> by_ordinal;by_ordinal.fill(-1);
  for(std::size_t i=0;i<units.size();++i) {
    const auto ordinal=units[i].ordinal;
    if(ordinal>=by_ordinal.size() || by_ordinal[ordinal]!=-1)return {};
    by_ordinal[ordinal]=int(i);
  }
  for(std::size_t i=0;i<draws.size();++i) {
    const auto &d=draws[i];
    if(d.object==0xffffffffu || d.definition==0xffffffffu || d.shader==0xffffffffu ||
       d.section>511 || d.part>255)return {};
    for(std::size_t j=0;j<i;++j)
      if(draws[j].object==d.object || draws[j].draw==d.draw)return {};
  }
  std::array<int,1024> command_draw;command_draw.fill(-1);
  std::array<bool,850> seen{};
  for(std::size_t i=0;i<commands.size();++i) {
    const auto &command=commands[i];
    // Untouched TU1.5 41010 dispatches 40F50 with4CBE0/4D0A0. The latter
    // selects the signed16-bit main ordinal from token>>8 and part from AL.
    if(command.callback!=0x41010u)continue;
    std::array<std::uint32_t,2> payload;
    std::memcpy(payload.data(),command.payload.data(),sizeof(payload));
    const auto ordinal=(payload[1]>>8)&65535u;
    if(ordinal>=by_ordinal.size() || by_ordinal[ordinal]<0)continue;
    const auto &unit=units[std::size_t(by_ordinal[ordinal])];
    const auto at=std::ranges::find(draws,unit.object,&SourceModelTransparentDraw::object);
    if(at==draws.end())continue;
    const auto selected=std::size_t(at-draws.begin());
    if(seen[selected] || unit.definition!=at->definition || unit.unit.words[1]==0xffffffffu ||
       ((unit.unit.words[3]>>9)&511u)!=at->section ||
       (payload[1]&255u)!=at->part || (payload[1]>>24)!=255u || payload[0]!=at->shader)return {};
    seen[selected]=true;command_draw[i]=int(selected);
  }
  for(std::size_t i=0;i<draws.size();++i)if(!seen[i])return {};
  // Sort the COMPLETE original bank first. Filtering before this unstable
  // source ordinal sorter changes equal-key order, including these mattes.
  std::vector<std::size_t> result;result.reserve(draws.size());
  for(const auto ordinal:source_transparent_sort(commands,eye,0))
    if(command_draw[ordinal]>=0)result.push_back(draws[std::size_t(command_draw[ordinal])].draw);
  return result;
}
} // namespace halo2
