#include "halo2/cache.hpp"
#include "halo2/full_scoreboard_assets.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <nlohmann/json.hpp>

namespace halo2 {
FullScoreboardAssets Cache::load_full_scoreboard_assets() const {
  FullScoreboardAssets out;const Bytes data(cache_bytes());
  const auto find=[&](std::uint32_t datum,const char *group)->const Tag * {
    const auto found=query_tag({.handle=TagHandle{datum}, .group=TagGroup::from_string(group)});
    return !found?nullptr:&*found;
  };
  const auto index=data.u32(16);
  const auto *scenario=find(data.u32(index+12),"scnr");
  out.scenario_present=scenario && scenario->address && scenario->size;
  const auto *globals=find(data.u32(index+16),"matg");
  if(!globals || globals->size<0x1ac)throw FormatError("Missing original scoreboard globals");
  const auto gp=metadata_offset(globals->address,globals->size);
  const auto count=data.u32(gp+0x160);
  if(count>4096)throw FormatError("Original scoreboard palette exceeds import bounds");
  if(count) {
    const auto base=metadata_offset(data.u32(gp+0x164),std::size_t(count)*12);
    for(unsigned i=0;i<count;++i)out.palette.push_back({data.u32(base+i*12),data.u32(base+i*12+4),data.u32(base+i*12+8)});
  }
  const auto *multiplayer=find(data.u32(gp+0x16c),"mulg");
  if(!multiplayer || multiplayer->size<8)return out;
  const auto mp=metadata_offset(multiplayer->address,multiplayer->size);
  if(multiplayer->size>=16 && data.u32(mp+8)) {
    if(data.u32(mp+8)!=1)throw FormatError("Ambiguous scoreboard runtime owner");
    const auto runtime=metadata_offset(data.u32(mp+12),1384);
    out.emblem_shader=data.u32(runtime+0x54c);out.emblem_bitmap=data.u32(runtime+0x554);
    out.dead_shader=data.u32(runtime+0x55c);out.dead_bitmap=data.u32(runtime+0x564);
  }
  const auto universal_count=data.u32(mp);
  if(universal_count>16)throw FormatError("Original scoreboard universal block exceeds import bounds");
  if(!universal_count)return out;
  // The scoreboard follows the first universal record only. Its observed
  // first20 bytes establish this pointer; no unverified full-record stride.
  const auto universal=metadata_offset(data.u32(mp+4),0x20);
  const auto team_count=data.u32(universal+0x10);
  if(team_count>4096)throw FormatError("Original scoreboard team palette exceeds import bounds");
  if(team_count) {
    const auto base=metadata_offset(data.u32(universal+0x14),std::size_t(team_count)*12);
    for(unsigned i=0;i<team_count;++i)out.team_palette.push_back({data.u32(base+i*12),data.u32(base+i*12+4),data.u32(base+i*12+8)});
  }
  const auto lp=gp+0x190;
  const auto locale_count=data.u32(lp),locale_size=data.u32(lp+4);
  const auto indices=data.u32(lp+8),text=data.u32(lp+12);
  if(locale_count>65536 || locale_size>16*1024*1024)throw FormatError("Original scoreboard locale exceeds import bounds");
  data.require(indices,std::size_t(locale_count)*8);const Bytes table(data.slice(text,locale_size));
  const auto read_locale=[&](std::uint32_t datum,const auto &destination) {
    const auto *strings=find(datum,"unic");
    if(!strings || strings->size<52)return;
    const auto sp=metadata_offset(strings->address,strings->size);
    const auto start=data.u16(sp+16),length=data.u16(sp+18);
    if(start>locale_count || length>locale_count-start)throw FormatError("Original scoreboard locale range is invalid");
    for(unsigned i=start;i<unsigned(start)+length;++i) {
      const auto row=indices+i*8,offset=data.u32(row+4);
      if(offset>=locale_size)throw FormatError("Original scoreboard label exceeds locale bytes");
      if(auto *label=destination(data.u32(row))) *label=table.string(offset,locale_size-offset);
    }
  };
  read_locale(data.u32(universal+0x1c),[&](std::uint32_t id)->std::string * {
    switch(id) {
    case 0x07000105:return &out.name_label;
    case 0x070000fd:return &out.wins_label;
    case 0x08000102:return &out.score_label;
    default:return nullptr;
    }
  });
  // Original1A9910's first nine ordinal SIDs are compiled constants. Their
  // strings are resolved in the separately referenced team_names unic slice,
  // rather than the global messages or frontend color labels.
  read_locale(data.u32(universal+0x0c),[&](std::uint32_t id)->std::string * {
    return id>=0x0100030a && id<=0x01000312 ? &out.team_names[id-0x0100030a] : nullptr;
  });
  return out;
}
std::array<std::uint32_t,5> load_full_scoreboard_vista_style(
    FullScoreboardAssets &assets,const std::filesystem::path &directory) {
  const auto path=directory/"scoreboard.json";
  if(!std::filesystem::is_regular_file(path) || std::filesystem::file_size(path)>256*1024)
    throw FormatError("Vista scoreboard artwork is incomplete; rerun asset setup");
  nlohmann::json document;std::ifstream(path)>>document;
  if(document.at("schema")!="H2UI_SCOREBOARD_V1")throw FormatError("Unsupported Vista scoreboard artwork");
  const auto real=[](std::uint32_t bits) {
    const auto value=std::bit_cast<float>(bits);
    if(!std::isfinite(value) || value<0 || value>1)throw FormatError("Invalid Vista scoreboard paint value");
    return bits;
  };
  const auto palette=[&](const char *name) {
    std::vector<FullScoreboardColorBits> result;
    const auto &rows=document.at(name);
    if(!rows.is_array() || rows.empty() || rows.size()>4096)throw FormatError("Invalid Vista scoreboard palette");
    for(const auto &row:rows) {
      if(!row.is_array() || row.size()!=3)throw FormatError("Invalid Vista scoreboard RGB record");
      result.push_back({real(row.at(0).get<std::uint32_t>()),real(row.at(1).get<std::uint32_t>()),real(row.at(2).get<std::uint32_t>())});
    }
    return result;
  };
  auto colors=palette("palette"),teams=palette("team_palette");
  const auto &words=document.at("style_words");
  if(!words.is_array() || words.size()!=5)throw FormatError("Invalid Vista scoreboard style record");
  std::array<std::uint32_t,5> result;
  for(unsigned i=0;i<result.size();++i)result[i]=real(words.at(i).get<std::uint32_t>());
  assets.palette=std::move(colors);assets.team_palette=std::move(teams);
  return result;
}
}
